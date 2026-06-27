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

	/* Like includeFlag, but `dirs` may hold several `|`-separated paths
	 * (CMake list form re-joined with '|' so it survives a -D), emitting one
	 * -I per non-empty entry. */
	std::string includeFlags(const std::string& dirs) {
		std::string out;
		std::string::size_type start = 0;
		while (start <= dirs.size()) {
			std::string::size_type sep = dirs.find('|', start);
			std::string::size_type end =
				(sep == std::string::npos) ? dirs.size() : sep;
			std::string dir = dirs.substr(start, end - start);
			if (!dir.empty())
				out += " -I'" + dir + "'";
			if (sep == std::string::npos)
				break;
			start = sep + 1;
		}
		return out;
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

	/* Emit + split a multi-file C/C++ binding and self-checking driver, compile
	 * (libb64 + one extra lib) and run. `compiler`/`std` select the toolchain
	 * (C_COMPILER/-std=c11 vs CXX_COMPILER/-std=c++11); `srcExt` is the split
	 * source extension (".c"/".cpp"); `srcPrefix` is the generated source's
	 * filename prefix (e.g. "xml_" / "json_"); `extraInclude`/`extraLink` are
	 * the -I dir and link flags for the marshalling library. */
	::testing::AssertionResult ccRoundtripImpl(const std::string& xsdPath,
	                                           const std::string& bindingTmpl,
	                                           const std::string& driverTmpl,
	                                           const std::string& compiler,
	                                           const std::string& std,
	                                           const std::string& srcExt,
	                                           const std::string& srcPrefix,
	                                           const std::string& extraInclude,
	                                           const std::string& extraLink) {
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
			XsdTools::SplitMarkedFiles(binding, dir);
			writeFile(dir + "/" + base + "-bin" + srcExt, driver);
		} catch (std::exception& e) {
			return ::testing::AssertionFailure()
				<< "generation failed: " << e.what();
		}
		/* compile: <base>-bin<ext> + <srcPrefix><base><ext> + libb64 + lib */
		std::ostringstream cc;
		cc << compiler
		   << " " << std
		   << includeFlag(dir)
		   << includeFlag(LIBB64_INCLUDE_DIR)
		   << includeFlags(extraInclude)
		   << " '" << dir << "/" << base << "-bin" << srcExt << "'"
		   << " '" << dir << "/" << srcPrefix << base << srcExt << "'"
		   << " '" << LIBB64_ARCHIVE << "'"
		   << " " << extraLink
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

	/* C round-trip: project C compiler at -std=c11, sources split as .c. */
	::testing::AssertionResult cRoundtripImpl(const std::string& xsdPath,
	                                          const std::string& bindingTmpl,
	                                          const std::string& driverTmpl,
	                                          const std::string& srcPrefix,
	                                          const std::string& extraInclude,
	                                          const std::string& extraLink) {
		return ccRoundtripImpl(xsdPath, bindingTmpl, driverTmpl,
		                       C_COMPILER, "-std=c11", ".c",
		                       srcPrefix, extraInclude, extraLink);
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

	/* Generate the binding (named <base>.py for `import <base>`) + driver,
	 * run `python3 driver.py`. */
	static ::testing::AssertionResult pythonRoundtripImpl(
			const std::string& xsdPath, const std::string& bindingTmpl,
			const std::string& driverTmpl) {
		const std::string base = baseName(xsdPath);
		const std::string dir = makeTempDir(base);
		if (dir.empty())
			return ::testing::AssertionFailure() << "could not create tempdir";
		try {
			writeFile(dir + "/" + base + ".py",
			          generate(TEMPLATES_DIR "/" + bindingTmpl, xsdPath));
			writeFile(dir + "/driver.py",
			          generate(TEMPLATES_DIR "/" + driverTmpl, xsdPath));
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

	::testing::AssertionResult pythonRoundtrip(const std::string& xsdPath) {
		return pythonRoundtripImpl(xsdPath, "python-sax", "python-sax-test");
	}

	::testing::AssertionResult pythonJsonRoundtrip(const std::string& xsdPath) {
		return pythonRoundtripImpl(xsdPath, "python-json",
		                           "python-json-test");
	}

	::testing::AssertionResult cExpatRoundtrip(const std::string& xsdPath) {
		return cRoundtripImpl(xsdPath, "c-xml-expat", "c-xml-expat-test",
		                      "xml_", EXPAT_INCLUDE_DIR, EXPAT_LINK);
	}

	::testing::AssertionResult cExpatDomRoundtrip(const std::string& xsdPath) {
		return cRoundtripImpl(xsdPath, "c-xml-expat-dom",
		                      "c-xml-expat-dom-test",
		                      "xml_", EXPAT_INCLUDE_DIR, EXPAT_LINK);
	}

	::testing::AssertionResult cJsonRoundtrip(const std::string& xsdPath) {
		return cRoundtripImpl(xsdPath, "c-json-jsonc",
		                      "c-json-jsonc-test",
		                      "json_", JSONC_INCLUDE_DIR, JSONC_LINK);
	}

	::testing::AssertionResult cppXmlRoundtrip(const std::string& xsdPath) {
		return ccRoundtripImpl(xsdPath, "cpp-xml-expat", "cpp-xml-expat-test",
		                       CXX_COMPILER, "-std=c++11", ".cpp",
		                       "xml_", EXPAT_INCLUDE_DIR, EXPAT_LINK);
	}

	::testing::AssertionResult cppJsonRoundtrip(const std::string& xsdPath) {
		return ccRoundtripImpl(xsdPath, "cpp-json-jsonc",
		                       "cpp-json-jsonc-test",
		                       CXX_COMPILER, "-std=c++11", ".cpp",
		                       "json_", JSONC_INCLUDE_DIR, JSONC_LINK);
	}

	/* ts-xml: emit the TS binding + RunTest driver into a pinned npm project,
	 * tsc-compile, run via node. Mirrors javaRoundtripImpl's pom-cache shape:
	 * deps resolve once into a shared node_modules, reused across cases. */
	::testing::AssertionResult tsXmlRoundtrip(const std::string& xsdPath) {
		const std::string base = baseName(xsdPath);
		const std::string root = makeTempDir(base);
		if (root.empty())
			return ::testing::AssertionFailure() << "could not create tempdir";
		if (0 != runCommand("cp '" TS_PACKAGE_JSON "' package.json && "
		                     "cp '" TS_TSCONFIG "' tsconfig.json",
		                     root).exitCode)
			return ::testing::AssertionFailure()
				<< "could not copy package.json/tsconfig.json";
		/* Reuse a shared node_modules (resolved once) like Maven's ~/.m2. */
		CommandResult deps = runCommand(
			"npm install --no-audit --no-fund --prefer-offline"
			" --cache /tmp/xsdb-ts-npm-cache", root);
		if (0 != deps.exitCode)
			return ::testing::AssertionFailure()
				<< "npm install failed:\n" << deps.output;
		try {
			XsdTools::SplitMarkedFiles(
				generate(TEMPLATES_DIR "/ts-xml", xsdPath), root);
			writeFile(root + "/RunTest.ts",
			          generate(TEMPLATES_DIR "/ts-xml-test", xsdPath));
		} catch (std::exception& e) {
			return ::testing::AssertionFailure()
				<< "generation failed: " << e.what();
		}
		CommandResult build = runCommand(
			"./node_modules/.bin/tsc -p tsconfig.json", root);
		if (0 != build.exitCode)
			return ::testing::AssertionFailure()
				<< "tsc failed:\n" << build.output;
		CommandResult run = runCommand("node RunTest.js", root);
		if (0 != run.exitCode)
			return ::testing::AssertionFailure()
				<< "RunTest failed (exit " << run.exitCode << "):\n"
				<< run.output;
		if (run.output.find("false") != std::string::npos)
			return ::testing::AssertionFailure()
				<< "round-trip mismatch; output:\n" << run.output;
		return ::testing::AssertionSuccess();
	}

	/* Generate the binding into a Maven package + RunTest driver, `mvn
	 * package`, run. RunTest prints one true/false per root element: any
	 * "false" is a marshal mismatch, a non-zero exit a crash, empty a pass. */
	static ::testing::AssertionResult javaRoundtripImpl(
			const std::string& xsdPath, const std::string& bindingTmpl,
			const std::string& driverTmpl, const std::string& pomPath) {
		const std::string base = baseName(xsdPath);
		const std::string root = makeTempDir(base);
		if (root.empty())
			return ::testing::AssertionFailure() << "could not create tempdir";
		const std::string pkg = root + "/src/main/java/com/mobitv/app";
		if (0 != runCommand("mkdir -p '" + pkg + "'", root).exitCode)
			return ::testing::AssertionFailure() << "mkdir failed";
		if (0 != runCommand("cp '" + pomPath + "' pom.xml", root).exitCode)
			return ::testing::AssertionFailure() << "could not copy pom.xml";
		try {
			XsdTools::SplitMarkedFiles(
				generate(TEMPLATES_DIR "/" + bindingTmpl, xsdPath), pkg);
			writeFile(pkg + "/RunTest.java",
			          generate(TEMPLATES_DIR "/" + driverTmpl, xsdPath));
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
		if (0 != run.exitCode)
			return ::testing::AssertionFailure()
				<< "RunTest failed (exit " << run.exitCode << "):\n"
				<< run.output;
		if (run.output.find("false") != std::string::npos)
			return ::testing::AssertionFailure()
				<< "round-trip mismatch; output:\n" << run.output;
		return ::testing::AssertionSuccess();
	}

	::testing::AssertionResult javaRoundtrip(const std::string& xsdPath) {
		return javaRoundtripImpl(xsdPath, "java-json.org",
		                         "java-json.org-test", JAVA_POM);
	}

	::testing::AssertionResult javaXmlRoundtrip(const std::string& xsdPath) {
		return javaRoundtripImpl(xsdPath, "java-xml-stax",
		                         "java-xml-stax-test", JAVA_XML_POM);
	}
}

/*
 * D4 negative-instance tests — prove the generated validate() REJECTS an
 * out-of-facet value (the round-trips only feed valid, facet-respecting data).
 * Rejection manifests per language; python-sax raises ValueError, so the driver
 * exits 0 iff the invalid value was rejected.
 */
namespace {
	::testing::AssertionResult pythonSaxRejects(const std::string& xsd,
			const std::string& rootClass, const std::string& invalidXml) {
		const std::string dir = makeTempDir("neg");
		if (dir.empty())
			return ::testing::AssertionFailure() << "could not create tempdir";
		try {
			writeFile(dir + "/m.py",
			          xsdtest::generate(TEMPLATES_DIR "/python-sax", xsd));
		} catch (std::exception& e) {
			return ::testing::AssertionFailure() << "generation: " << e.what();
		}
		writeFile(dir + "/neg.py",
			"import io, sys, m\n"
			"try:\n"
			"    m." + rootClass + "().unmarshal(io.StringIO('" + invalidXml + "'))\n"
			"    sys.exit(1)\n"
			"except ValueError:\n"
			"    sys.exit(0)\n");
		CommandResult run = runCommand("python3 neg.py", dir);
		if (0 != run.exitCode)
			return ::testing::AssertionFailure()
				<< "invalid value not rejected (exit " << run.exitCode
				<< "):\n" << run.output;
		return ::testing::AssertionSuccess();
	}
}

TEST(Reject, PythonSaxRange) {
	EXPECT_TRUE(pythonSaxRejects(XSD_CORPUS_DIR "/facet_range.xsd",
		"xml_facet_range_xsd", "<rating>99</rating>"));
}
TEST(Reject, PythonSaxEnum) {
	EXPECT_TRUE(pythonSaxRejects(XSD_CORPUS_DIR "/facet_enum.xsd",
		"xml_facet_enum_xsd", "<color>purple</color>"));
}
TEST(Reject, PythonSaxLength) {
	EXPECT_TRUE(pythonSaxRejects(XSD_CORPUS_DIR "/facet_strlen.xsd",
		"xml_facet_strlen_xsd", "<code>x</code>"));
}
TEST(Reject, PythonSaxAttribute) {
	EXPECT_TRUE(pythonSaxRejects(XSD_CORPUS_DIR "/facet_attr.xsd",
		"xml_facet_attr_xsd",
		"<catalog><widget size=\"huge\"/></catalog>"));
}

namespace {
	/* c-xml-expat aborts (exit != 0) on a facet violation; feed an invalid
	 * instance and assert the process did NOT exit cleanly. */
	::testing::AssertionResult cExpatRejects(const std::string& xsd,
			const std::string& header, const std::string& elem,
			const std::string& invalidXml) {
		const std::string base = xsdtest::baseName(xsd);
		const std::string dir = makeTempDir(base);
		if (dir.empty())
			return ::testing::AssertionFailure() << "could not create tempdir";
		try {
			XsdTools::SplitMarkedFiles(
				xsdtest::generate(TEMPLATES_DIR "/c-xml-expat", xsd), dir);
		} catch (std::exception& e) {
			return ::testing::AssertionFailure() << "generation: " << e.what();
		}
		writeFile(dir + "/" + base + "-bin.c",
			"#include <string.h>\n#include <stdlib.h>\n#include \"" + header + "\"\n"
			"static void* rc(struct xml_marshaller* m, void* p, size_t s)"
			"{(void)m;return realloc(p,s);}\n"
			"static void cb(struct xml_marshaller* m, const xml_" + elem +
			"* o, uint32_t pa){(void)m;(void)o;(void)pa;}\n"
			"int main(void){\n"
			"  xml_marshaller m = { .realloc = rc, .unmarshal_" + elem + " = cb };\n"
			"  const char* x = \"" + invalidXml + "\";\n"
			"  xml_buffer b = { .pBuf_=(uint8_t*)x, .size_=(uint32_t)strlen(x),"
			" .used_=(uint32_t)strlen(x) };\n"
			"  xml_unmarshal(&m, &b, 1);\n"
			"  return 0;\n}\n");
		std::ostringstream cc;
		cc << C_COMPILER << " -std=c11" << includeFlag(dir)
		   << includeFlag(LIBB64_INCLUDE_DIR) << includeFlags(EXPAT_INCLUDE_DIR)
		   << " '" << dir << "/" << base << "-bin.c'"
		   << " '" << dir << "/xml_" << base << ".c'"
		   << " '" << LIBB64_ARCHIVE << "' " << EXPAT_LINK
		   << " -o '" << dir << "/" << base << "'";
		CommandResult build = runCommand(cc.str(), dir);
		if (0 != build.exitCode)
			return ::testing::AssertionFailure() << "compile:\n" << build.output;
		CommandResult run = runCommand("./" + base, dir);
		if (0 == run.exitCode)
			return ::testing::AssertionFailure()
				<< "invalid value was NOT rejected (exit 0):\n" << run.output;
		return ::testing::AssertionSuccess();
	}
}

TEST(Reject, CExpatRange) {
	EXPECT_TRUE(cExpatRejects(XSD_CORPUS_DIR "/facet_range.xsd",
		"xml_facet_range.h", "rating", "<rating>99</rating>"));
}
TEST(Reject, CExpatEnum) {
	EXPECT_TRUE(cExpatRejects(XSD_CORPUS_DIR "/facet_enum.xsd",
		"xml_facet_enum.h", "color", "<color>purple</color>"));
}

namespace {
	/* c-xml-expat-dom: xml_unmarshal aborts (exit != 0) on a facet violation. */
	::testing::AssertionResult cDomRejects(const std::string& xsd,
			const std::string& header, const std::string& invalidXml) {
		const std::string base = xsdtest::baseName(xsd);
		const std::string dir = makeTempDir(base);
		if (dir.empty())
			return ::testing::AssertionFailure() << "could not create tempdir";
		try {
			XsdTools::SplitMarkedFiles(xsdtest::generate(
				TEMPLATES_DIR "/c-xml-expat-dom", xsd), dir);
		} catch (std::exception& e) {
			return ::testing::AssertionFailure() << "generation: " << e.what();
		}
		writeFile(dir + "/" + base + "-bin.c",
			"#include <string.h>\n#include \"" + header + "\"\n"
			"int main(void){ const char* x = \"" + invalidXml + "\";"
			" xml_unmarshal(x, strlen(x), 0); return 0; }\n");
		std::ostringstream cc;
		cc << C_COMPILER << " -std=c11" << includeFlag(dir)
		   << includeFlag(LIBB64_INCLUDE_DIR) << includeFlags(EXPAT_INCLUDE_DIR)
		   << " '" << dir << "/" << base << "-bin.c'"
		   << " '" << dir << "/xml_" << base << ".c'"
		   << " '" << LIBB64_ARCHIVE << "' " << EXPAT_LINK
		   << " -o '" << dir << "/" << base << "'";
		CommandResult build = runCommand(cc.str(), dir);
		if (0 != build.exitCode)
			return ::testing::AssertionFailure() << "compile:\n" << build.output;
		CommandResult run = runCommand("./" + base, dir);
		if (0 == run.exitCode)
			return ::testing::AssertionFailure()
				<< "invalid value was NOT rejected (exit 0):\n" << run.output;
		return ::testing::AssertionSuccess();
	}

	/* python-json: unmarshal of an out-of-facet JSON value raises ValueError. */
	::testing::AssertionResult pythonJsonRejects(const std::string& xsd,
			const std::string& rootClass, const std::string& invalidJson) {
		const std::string dir = makeTempDir("njson");
		if (dir.empty())
			return ::testing::AssertionFailure() << "could not create tempdir";
		try {
			writeFile(dir + "/m.py",
				xsdtest::generate(TEMPLATES_DIR "/python-json", xsd));
		} catch (std::exception& e) {
			return ::testing::AssertionFailure() << "generation: " << e.what();
		}
		writeFile(dir + "/neg.py",
			"import json, sys, m\n"
			"try:\n"
			"    m." + rootClass + "().unmarshal(json.loads('" + invalidJson + "'))\n"
			"    sys.exit(1)\n"
			"except ValueError:\n"
			"    sys.exit(0)\n");
		CommandResult run = runCommand("python3 neg.py", dir);
		if (0 != run.exitCode)
			return ::testing::AssertionFailure()
				<< "invalid value not rejected (exit " << run.exitCode
				<< "):\n" << run.output;
		return ::testing::AssertionSuccess();
	}
}

TEST(Reject, CDomRange) {
	EXPECT_TRUE(cDomRejects(XSD_CORPUS_DIR "/facet_range.xsd",
		"xml_facet_range.h", "<rating>99</rating>"));
}
TEST(Reject, CDomEnum) {
	EXPECT_TRUE(cDomRejects(XSD_CORPUS_DIR "/facet_enum.xsd",
		"xml_facet_enum.h", "<color>purple</color>"));
}
TEST(Reject, PythonJsonRange) {
	EXPECT_TRUE(pythonJsonRejects(XSD_CORPUS_DIR "/facet_range.xsd",
		"json_facet_range_xsd",
		"[{\"name\": \"rating\", \"attrs\": {}, \"content\": 99}]"));
}
TEST(Reject, PythonJsonEnum) {
	EXPECT_TRUE(pythonJsonRejects(XSD_CORPUS_DIR "/facet_enum.xsd",
		"json_facet_enum_xsd",
		"[{\"name\": \"color\", \"attrs\": {}, \"content\": [\"purple\"]}]"));
}

namespace {
	/* java-xml: unmarshalling an out-of-facet value throws
	 * IllegalArgumentException; the driver exits 0 only when it does. */
	::testing::AssertionResult javaXmlRejects(const std::string& xsd,
			const std::string& invalidXml) {
		const std::string base = xsdtest::baseName(xsd);
		const std::string root = makeTempDir(base);
		if (root.empty())
			return ::testing::AssertionFailure() << "could not create tempdir";
		const std::string pkg = root + "/src/main/java/com/mobitv/app";
		if (0 != runCommand("mkdir -p '" + pkg + "'", root).exitCode)
			return ::testing::AssertionFailure() << "mkdir failed";
		if (0 != runCommand("cp '" JAVA_XML_POM "' pom.xml", root).exitCode)
			return ::testing::AssertionFailure() << "could not copy pom.xml";
		try {
			XsdTools::SplitMarkedFiles(xsdtest::generate(
				TEMPLATES_DIR "/java-xml-stax", xsd), pkg);
		} catch (std::exception& e) {
			return ::testing::AssertionFailure() << "generation: " << e.what();
		}
		writeFile(pkg + "/RunTest.java",
			"package com.mobitv.app;\n"
			"public class RunTest {\n"
			"  public static void main(String[] a) {\n"
			"    try { new Document().unmarshal(\"" + invalidXml + "\");"
			" System.exit(1); }\n"
			"    catch (IllegalArgumentException e) { System.exit(0); }\n"
			"    catch (Exception e) { System.exit(2); }\n"
			"  }\n}\n");
		CommandResult build = runCommand("mvn -q package", root);
		if (0 != build.exitCode)
			return ::testing::AssertionFailure()
				<< "mvn package failed:\n" << build.output;
		CommandResult run = runCommand(
			"java -cp target/my-app-1.0-SNAPSHOT-jar-with-dependencies.jar"
			" com.mobitv.app.RunTest", root);
		if (0 != run.exitCode)
			return ::testing::AssertionFailure()
				<< "invalid value not rejected (exit " << run.exitCode
				<< "):\n" << run.output;
		return ::testing::AssertionSuccess();
	}
}

TEST(Reject, JavaXmlRange) {
	EXPECT_TRUE(javaXmlRejects(XSD_CORPUS_DIR "/facet_range.xsd",
		"<rating>99</rating>"));
}
TEST(Reject, JavaXmlEnum) {
	EXPECT_TRUE(javaXmlRejects(XSD_CORPUS_DIR "/facet_enum.xsd",
		"<color>purple</color>"));
}

namespace {
	/* c-json SKIPS an invalid value (validate returns nonzero => the notify
	 * callback is not called). Marshal an out-of-range value (marshal does not
	 * validate), unmarshal it, and assert the callback never fired. */
	::testing::AssertionResult cJsonRejectsRange(const std::string& xsd,
			const std::string& header, const std::string& elem) {
		const std::string base = xsdtest::baseName(xsd);
		const std::string dir = makeTempDir(base);
		if (dir.empty())
			return ::testing::AssertionFailure() << "could not create tempdir";
		try {
			XsdTools::SplitMarkedFiles(xsdtest::generate(
				TEMPLATES_DIR "/c-json-jsonc", xsd), dir);
		} catch (std::exception& e) {
			return ::testing::AssertionFailure() << "generation: " << e.what();
		}
		writeFile(dir + "/" + base + "-bin.c",
			"#include <string.h>\n#include <stdlib.h>\n#include \"" + header + "\"\n"
			"static int g_called = 0;\n"
			"static void* rc(struct json_marshaller* m, void* p, size_t s)"
			"{(void)m;return realloc(p,s);}\n"
			"static void cb(struct json_marshaller* m, const json_" + elem +
			"* o, uint32_t pa){(void)m;(void)o;(void)pa;g_called=1;}\n"
			"int main(void){\n"
			"  json_buffer output;\n"
			"  json_marshaller m = { .realloc=rc, .unmarshal_" + elem + "=cb };\n"
			"  json_" + elem + " bad = { .Content_ = 99 };\n"
			"  json_marshal_" + base + "_xsd s = json_marshal(&m);\n"
			"  s.marshal_" + elem + "(&m, &bad, s.pSelf);\n"
			"  output = json_marshal_flush(&m, 1);\n"
			"  json_unmarshal_ex(&m, &output, 1, json_" + base +
			"_default_factories());\n"
			"  output.pBuf_ = realloc(output.pBuf_, 0);\n"
			"  return g_called ? 1 : 0;\n}\n");
		std::ostringstream cc;
		cc << C_COMPILER << " -std=c11" << includeFlag(dir)
		   << includeFlag(LIBB64_INCLUDE_DIR) << includeFlags(JSONC_INCLUDE_DIR)
		   << " '" << dir << "/" << base << "-bin.c'"
		   << " '" << dir << "/json_" << base << ".c'"
		   << " '" << LIBB64_ARCHIVE << "' " << JSONC_LINK
		   << " -o '" << dir << "/" << base << "'";
		CommandResult build = runCommand(cc.str(), dir);
		if (0 != build.exitCode)
			return ::testing::AssertionFailure() << "compile:\n" << build.output;
		CommandResult run = runCommand("./" + base, dir);
		if (0 != run.exitCode)
			return ::testing::AssertionFailure()
				<< "invalid value was NOT skipped (callback fired):\n"
				<< run.output;
		return ::testing::AssertionSuccess();
	}

	/* java-json: constructing an element from out-of-facet JSON throws
	 * IllegalArgumentException. */
	::testing::AssertionResult javaJsonRejects(const std::string& xsd,
			const std::string& className, const std::string& invalidJson) {
		const std::string base = xsdtest::baseName(xsd);
		const std::string root = makeTempDir(base);
		if (root.empty())
			return ::testing::AssertionFailure() << "could not create tempdir";
		const std::string pkg = root + "/src/main/java/com/mobitv/app";
		if (0 != runCommand("mkdir -p '" + pkg + "'", root).exitCode)
			return ::testing::AssertionFailure() << "mkdir failed";
		if (0 != runCommand("cp '" JAVA_POM "' pom.xml", root).exitCode)
			return ::testing::AssertionFailure() << "could not copy pom.xml";
		try {
			XsdTools::SplitMarkedFiles(xsdtest::generate(
				TEMPLATES_DIR "/java-json.org", xsd), pkg);
		} catch (std::exception& e) {
			return ::testing::AssertionFailure() << "generation: " << e.what();
		}
		writeFile(pkg + "/RunTest.java",
			"package com.mobitv.app;\n"
			"import org.json.JSONObject;\n"
			"import com.mobitv.generated.json." + className + ";\n"
			"public class RunTest {\n"
			"  public static void main(String[] a) {\n"
			"    try { new " + className + "(new JSONObject(\"" + invalidJson +
			"\")); System.exit(1); }\n"
			"    catch (IllegalArgumentException e) { System.exit(0); }\n"
			"    catch (Exception e) { System.exit(2); }\n"
			"  }\n}\n");
		CommandResult build = runCommand("mvn -q package", root);
		if (0 != build.exitCode)
			return ::testing::AssertionFailure()
				<< "mvn package failed:\n" << build.output;
		CommandResult run = runCommand(
			"java -cp target/my-app-1.0-SNAPSHOT-jar-with-dependencies.jar"
			" com.mobitv.app.RunTest", root);
		if (0 != run.exitCode)
			return ::testing::AssertionFailure()
				<< "invalid value not rejected (exit " << run.exitCode
				<< "):\n" << run.output;
		return ::testing::AssertionSuccess();
	}
}

TEST(Reject, CJsonRange) {
	EXPECT_TRUE(cJsonRejectsRange(XSD_CORPUS_DIR "/facet_range.xsd",
		"json_facet_range.h", "rating"));
}
TEST(Reject, JavaJsonRange) {
	EXPECT_TRUE(javaJsonRejects(XSD_CORPUS_DIR "/facet_range.xsd",
		"Rating", "{'$':99}"));
}
TEST(Reject, JavaJsonEnum) {
	EXPECT_TRUE(javaJsonRejects(XSD_CORPUS_DIR "/facet_enum.xsd",
		"Color", "{'$':'purple'}"));
}

/* --- Quickstart interop relay (examples/quickstart) ----------------------- */
namespace {
	/* Generate the three bindings from examples/quickstart/message.xsd, build
	   the C++ and Java endpoints, run the five-endpoint relay, and assert the
	   message crossed all five (ep5 exits 0 and the final message has five
	   hops). Mirrors what `run-quickstart` does, but as a gtest. */
	::testing::AssertionResult quickstartRelay() {
		const std::string qs = QUICKSTART_DIR;
		const std::string schema = qs + "/message.xsd";
		const std::string dir = makeTempDir("quickstart");
		if (dir.empty())
			return ::testing::AssertionFailure() << "could not create tempdir";
		if (0 != runCommand("mkdir -p c cpp java javac", dir).exitCode)
			return ::testing::AssertionFailure() << "mkdir failed";
		try {
			writeFile(dir + "/message.py",
			          xsdtest::generate(TEMPLATES_DIR "/python-sax", schema));
			XsdTools::SplitMarkedFiles(
			    xsdtest::generate(TEMPLATES_DIR "/c-xml-expat", schema),
			    dir + "/c");
			XsdTools::SplitMarkedFiles(
			    xsdtest::generate(TEMPLATES_DIR "/cpp-xml-expat", schema),
			    dir + "/cpp");
			std::ostringstream java;
			XsdTools::Generate(TEMPLATES_DIR "/java-xml-stax", schema, java,
			                   { "-package", "interop" });
			XsdTools::SplitMarkedFiles(java.str(), dir + "/java");
		} catch (std::exception& e) {
			return ::testing::AssertionFailure() << "generation failed: "
			                                     << e.what();
		}
		/* ep2 is C (c-xml-expat); ep5 is C++ (cpp-xml-expat) */
		{
			std::ostringstream cc;
			cc << C_COMPILER << " -std=c11"
			   << includeFlag(dir + "/c")
			   << includeFlags(EXPAT_INCLUDE_DIR)
			   << " '" << qs << "/ep2_relay.c'"
			   << " '" << dir << "/c/xml_message.c'"
			   << " " << EXPAT_LINK
			   << " -o '" << dir << "/ep2'";
			CommandResult b = runCommand(cc.str(), dir);
			if (0 != b.exitCode)
				return ::testing::AssertionFailure()
				    << "ep2_relay compile failed:\n" << b.output;
		}
		{
			std::ostringstream cc;
			cc << CXX_COMPILER << " -std=c++11"
			   << includeFlag(dir + "/cpp") << includeFlag(qs)
			   << includeFlags(EXPAT_INCLUDE_DIR)
			   << " '" << qs << "/ep5_sink.cpp'"
			   << " '" << dir << "/cpp/xml_message.cpp'"
			   << " " << EXPAT_LINK
			   << " -o '" << dir << "/ep5'";
			CommandResult b = runCommand(cc.str(), dir);
			if (0 != b.exitCode)
				return ::testing::AssertionFailure()
				    << "ep5_sink compile failed:\n" << b.output;
		}
		CommandResult jc = runCommand(
		    "javac -d javac java/*.java '" + qs + "/Ep3Relay.java'", dir);
		if (0 != jc.exitCode)
			return ::testing::AssertionFailure() << "javac failed:\n"
			                                     << jc.output;
		CommandResult run = runCommand(
		    "PYTHONPATH='" + dir + "' python3 '" + qs + "/ep1_producer.py'"
		    " | ./ep2"
		    " | java -cp javac interop.Ep3Relay"
		    " | PYTHONPATH='" + dir + "' python3 '" + qs + "/ep4_relay.py'"
		    " | ./ep5", dir);
		if (0 != run.exitCode)
			return ::testing::AssertionFailure()
			    << "relay failed (exit " << run.exitCode << "):\n" << run.output;
		int hops = 0;
		for (std::string::size_type p = run.output.find("<hop ");
		     p != std::string::npos; p = run.output.find("<hop ", p + 1))
			++hops;
		if (5 != hops)
			return ::testing::AssertionFailure()
			    << "expected 5 hops in final message, got " << hops << ":\n"
			    << run.output;
		return ::testing::AssertionSuccess();
	}
}

/* The quickstart example, exercised end to end through the gtest binary. Skips
   when python3 or a JDK is absent (the C++ toolchain that built this binary is
   always present). */
TEST(Quickstart, Relay) {
	if (!xsdtest::programAvailable("python3"))
		GTEST_SKIP() << "python3 not on PATH";
	if (!xsdtest::programAvailable("javac") ||
	    !xsdtest::programAvailable("java"))
		GTEST_SKIP() << "JDK (javac/java) not on PATH";
	EXPECT_TRUE(quickstartRelay());
}
