/*
 * roundtrip_util.cpp
 *
 *  This file is part of xsd-tools.
 */
#include "roundtrip_util.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#include <unistd.h>

#include "src/xsdtools.hpp"

#ifndef TEMPLATES_DIR
#define TEMPLATES_DIR "templates"
#endif

namespace {
	struct CommandResult {
		bool launched;
		int exitCode;
		std::string output;
	};

	/* Run `cmd` in `workdir`, capturing combined stdout+stderr. */
	CommandResult runCommand(const std::string& cmd,
	                         const std::string& workdir) {
		std::string full = "cd '" + workdir + "' && " + cmd + " 2>&1";
		CommandResult res = { false, -1, std::string() };
		FILE* pipe = popen(full.c_str(), "r");
		if (NULL == pipe)
			return res;
		res.launched = true;
		char buf[4096];
		size_t n = 0;
		while (0 != (n = fread(buf, 1, sizeof(buf), pipe)))
			res.output.append(buf, n);
		int status = pclose(pipe);
		res.exitCode = (status == -1) ? -1 : WEXITSTATUS(status);
		return res;
	}

	/* A -I flag for `dir`, or empty if `dir` is. An empty dir would emit a
	 * bare `-I` (the quotes collapse in the shell), which then swallows the
	 * next argument as its value — e.g. the driver .c, leaving no main(). */
	std::string includeFlag(const std::string& dir) {
		return dir.empty() ? std::string() : " -I'" + dir + "'";
	}

	void writeFile(const std::string& path, const std::string& content) {
		std::ofstream ofs(path.c_str(), std::ios::binary);
		ofs.write(content.data(),
		          static_cast<std::streamsize>(content.size()));
	}

	std::string makeTempDir(const std::string& tag) {
		std::string tmpl = "/tmp/xsdb-rt-" + tag + "-XXXXXX";
		std::vector<char> buf(tmpl.begin(), tmpl.end());
		buf.push_back('\0');
		const char* dir = mkdtemp(&buf[0]);
		return dir ? std::string(dir) : std::string();
	}

	/* Split a c-xml-expat binding blob on its FILE markers (lines of the
	 * form: slash-star FILE: <name> star-slash), writing each section into
	 * `dir`. Returns the written file names. */
	std::vector<std::string> splitMarkedFiles(const std::string& blob,
	                                          const std::string& dir) {
		std::vector<std::string> files;
		std::istringstream in(blob);
		std::string line;
		std::ofstream out;
		const std::string marker = "/* FILE: ";
		while (std::getline(in, line)) {
			size_t pos = line.find(marker);
			if (std::string::npos != pos) {
				size_t start = pos + marker.size();
				size_t end = line.find(' ', start);
				std::string name = line.substr(start, end - start);
				if (out.is_open())
					out.close();
				out.open((dir + "/" + name).c_str(), std::ios::binary);
				files.push_back(name);
			} else if (out.is_open()) {
				out << line << "\n";
			}
		}
		return files;
	}

	::testing::AssertionResult cRoundtripImpl(const std::string& xsdPath,
	                                          const std::string& bindingTmpl,
	                                          const std::string& driverTmpl) {
		const std::string base = xsdtest::baseName(xsdPath);
		const std::string dir = xsdtest::baseName(xsdPath).empty()
			? std::string() : makeTempDir(base);
		if (dir.empty())
			return ::testing::AssertionFailure() << "could not create tempdir";
		try {
			std::string binding = xsdtest::generate(
				TEMPLATES_DIR "/" + bindingTmpl, xsdPath);
			std::string driver = xsdtest::generate(
				TEMPLATES_DIR "/" + driverTmpl, xsdPath);
			splitMarkedFiles(binding, dir);
			writeFile(dir + "/" + base + "-bin.c", driver);
		} catch (std::exception& e) {
			return ::testing::AssertionFailure()
				<< "generation failed: " << e.what();
		}
		/* compile: <base>-bin.c + xml_<base>.c + libb64 + expat */
		std::ostringstream cc;
		cc << C_COMPILER
		   << " -std=c11"  /* match the project's pinned C standard */
		   << includeFlag(dir)
		   << includeFlag(LIBB64_INCLUDE_DIR)
		   << includeFlag(EXPAT_INCLUDE_DIR)
		   << " '" << dir << "/" << base << "-bin.c'"
		   << " '" << dir << "/xml_" << base << ".c'"
		   << " '" << LIBB64_ARCHIVE << "'"
		   << " " << EXPAT_LINK
		   << " -o '" << dir << "/" << base << "'";
		CommandResult build = runCommand(cc.str(), dir);
		if (0 != build.exitCode)
			return ::testing::AssertionFailure()
				<< "compile failed:\n" << build.output;
		CommandResult run = runCommand("./" + base, dir);
		if (0 != run.exitCode)
			return ::testing::AssertionFailure()
				<< "run failed (exit " << run.exitCode << "):\n"
				<< run.output;
		return ::testing::AssertionSuccess();
	}
}

namespace xsdtest {
	std::string baseName(const std::string& xsdPath) {
		size_t slash = xsdPath.find_last_of("/\\");
		std::string name = (std::string::npos == slash)
			? xsdPath : xsdPath.substr(slash + 1);
		size_t dot = name.find_last_of('.');
		return (std::string::npos == dot) ? name : name.substr(0, dot);
	}

	bool programAvailable(const std::string& prog) {
		std::string cmd = "command -v '" + prog + "' >/dev/null 2>&1";
		return 0 == system(cmd.c_str());
	}

	std::string generate(const std::string& templatePath,
	                     const std::string& xsdPath) {
		std::ostringstream oss;
		XsdTools::Generate(templatePath, xsdPath, oss);
		return oss.str();
	}

	::testing::AssertionResult pythonRoundtrip(const std::string& xsdPath) {
		const std::string base = baseName(xsdPath);
		const std::string dir = makeTempDir(base);
		if (dir.empty())
			return ::testing::AssertionFailure() << "could not create tempdir";
		try {
			/* driver does `import <base>`, so name the binding <base>.py */
			writeFile(dir + "/" + base + ".py",
			          generate(TEMPLATES_DIR "/python-sax", xsdPath));
			writeFile(dir + "/driver.py",
			          generate(TEMPLATES_DIR "/python-sax-test", xsdPath));
		} catch (std::exception& e) {
			return ::testing::AssertionFailure()
				<< "generation failed: " << e.what();
		}
		CommandResult run = runCommand("python3 driver.py", dir);
		if (0 != run.exitCode)
			return ::testing::AssertionFailure()
				<< "round-trip failed (exit " << run.exitCode << "):\n"
				<< run.output;
		return ::testing::AssertionSuccess();
	}

	::testing::AssertionResult cExpatRoundtrip(const std::string& xsdPath) {
		return cRoundtripImpl(xsdPath, "c-xml-expat", "c-xml-expat-test");
	}

	::testing::AssertionResult cExpatDomRoundtrip(const std::string& xsdPath) {
		return cRoundtripImpl(xsdPath, "c-xml-expat-dom.template",
		                      "c-xml-expat-dom.template-test");
	}

	::testing::AssertionResult javaRoundtrip(const std::string& xsdPath) {
		const std::string base = baseName(xsdPath);
		const std::string root = makeTempDir(base);
		if (root.empty())
			return ::testing::AssertionFailure() << "could not create tempdir";
		const std::string pkg = root + "/src/main/java/com/mobitv/app";
		if (0 != runCommand("mkdir -p '" + pkg + "'", root).exitCode)
			return ::testing::AssertionFailure() << "mkdir failed";
		if (0 != runCommand("cp '" JAVA_POM "' pom.xml", root).exitCode)
			return ::testing::AssertionFailure() << "could not copy pom.xml";
		try {
			splitMarkedFiles(
				generate(TEMPLATES_DIR "/java-json.org.tmpl", xsdPath), pkg);
			writeFile(pkg + "/RunTest.java",
			          generate(TEMPLATES_DIR "/java-json.org.tmpl-test",
			                   xsdPath));
		} catch (std::exception& e) {
			return ::testing::AssertionFailure()
				<< "generation failed: " << e.what();
		}
		CommandResult build = runCommand("mvn -q package", root);
		if (0 != build.exitCode)
			return ::testing::AssertionFailure()
				<< "mvn package failed:\n" << build.output;
		CommandResult run = runCommand(
			"java -cp target/my-app-1.0-SNAPSHOT-jar-with-dependencies.jar"
			" com.mobitv.app.RunTest", root);
		/* RunTest prints one "true"/"false" per root element. A non-zero
		 * exit is a crash; any "false" is a marshal mismatch. Empty output
		 * (e.g. an abstract element with nothing to marshal) is a pass. */
		if (0 != run.exitCode)
			return ::testing::AssertionFailure()
				<< "RunTest failed (exit " << run.exitCode << "):\n"
				<< run.output;
		if (run.output.find("false") != std::string::npos)
			return ::testing::AssertionFailure()
				<< "round-trip mismatch; output:\n" << run.output;
		return ::testing::AssertionSuccess();
	}
}
