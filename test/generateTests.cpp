/*
 * generateTests.cpp — XsdTools::Generate output matches the recorded golden
 * corpus (the gtest replacement for xsdparse-test.py's snapshot intent).
 *
 *  This file is part of xsd-tools.
 */
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
#include <gtest/gtest.h>
#include "src/luaScriptAdapter.hpp"
#include "src/xsdtools.hpp"
#include "src/XSDParser/Exception.hpp"

namespace {
	std::string readFile(const std::string& path) {
		std::ifstream f(path.c_str(), std::ios::binary);
		std::ostringstream s;
		s << f.rdbuf();
		return s.str();
	}

	std::vector<std::string> listWithSuffix(const std::string& dir,
	                                        const std::string& suffix) {
		std::vector<std::string> out;
		DIR* d = opendir(dir.c_str());
		if (NULL == d)
			return out;
		for (struct dirent* e = readdir(d); NULL != e; e = readdir(d)) {
			std::string name(e->d_name);
			if (name.size() >= suffix.size() &&
			    0 == name.compare(name.size() - suffix.size(),
			                      suffix.size(), suffix))
				out.push_back(name);
		}
		closedir(d);
		return out;
	}

	/* One-off on-disk template for the engine error/scanner tests; the
	 * engine only loads templates from files. */
	class TempTemplate {
	public:
		explicit TempTemplate(const std::string& text) {
			char tmpl[] = "/tmp/xsdb-gen-XXXXXX";
			const char* d = mkdtemp(tmpl);
			dir_ = d ? d : "";
			path_ = dir_ + "/tpl";
			std::ofstream f(path_.c_str(), std::ios::binary);
			f << text;
		}
		~TempTemplate() {
			remove(path_.c_str());
			rmdir(dir_.c_str());
		}
		const std::string& Path() const { return path_; }
	private:
		std::string dir_;
		std::string path_;
	};

	/* Any small valid schema works: these templates never read `schema`. */
	const char* const kAnyXsd = XSD_CORPUS_DIR "/testA001.xsd";

	std::string generateWith(const std::string& templateText) {
		TempTemplate tpl(templateText);
		std::ostringstream out;
		XsdTools::Generate(tpl.Path(), kAnyXsd, out);
		return out.str();
	}

	std::string generateError(const std::string& templateText) {
		TempTemplate tpl(templateText);
		std::ostringstream out;
		try {
			XsdTools::Generate(tpl.Path(), kAnyXsd, out);
		} catch (const Core::LuaException& e) {
			return e.what();
		}
		ADD_FAILURE() << "generation succeeded; output: " << out.str();
		return "";
	}
}

/* For every xsdparse/<schema>.out golden, regenerate via the "test" template
 * and compare verbatim. */
TEST(Generate, MatchesGoldenCorpus) {
	std::vector<std::string> golds = listWithSuffix(XSDPARSE_DIR, ".out");
	ASSERT_FALSE(golds.empty()) << "no goldens under " XSDPARSE_DIR;
	for (size_t i = 0; i < golds.size(); ++i) {
		const std::string base = golds[i].substr(0, golds[i].size() - 4);
		const std::string xsd =
			std::string(XSD_CORPUS_DIR "/") + base + ".xsd";
		std::ostringstream out;
		ASSERT_NO_THROW(
			XsdTools::Generate(TEMPLATES_DIR "/test", xsd, out)) << base;
		EXPECT_EQ(readFile(std::string(XSDPARSE_DIR "/") + golds[i]),
		          out.str()) << "schema " << base;
	}
}

/* Every schema under xsd-negative is malformed or unresolvable and must be
 * rejected with a thrown XMLException — never silently generated, never a
 * crash (the cyclic fixtures would overflow the stack without the guard). */
TEST(Generate, RejectsNegativeCorpus) {
	std::vector<std::string> xsds = listWithSuffix(XSD_NEGATIVE_DIR, ".xsd");
	ASSERT_FALSE(xsds.empty()) << "no fixtures under " XSD_NEGATIVE_DIR;
	for (size_t i = 0; i < xsds.size(); ++i) {
		const std::string xsd =
			std::string(XSD_NEGATIVE_DIR "/") + xsds[i];
		std::ostringstream out;
		EXPECT_THROW(XsdTools::Generate(TEMPLATES_DIR "/test", xsd, out),
		             XSD::XMLException) << xsds[i];
	}
}

/* A block that raises at runtime must abort generation, not have its error
 * message pasted into the output. */
TEST(Generate, BlockRuntimeErrorAborts) {
	std::string msg = generateError("A[@lua return nil .. 1]B");
	EXPECT_NE(std::string::npos, msg.find("concatenate")) << msg;
	EXPECT_NE(std::string::npos, msg.find("line 1")) << msg;
}

/* Same for a block that fails to even compile. */
TEST(Generate, BlockLoadErrorAborts) {
	std::string msg = generateError("\n\nA[@lua return ( ]B");
	EXPECT_NE(std::string::npos, msg.find("line 3")) << msg;
}

/* include of a nonexistent template must fail naming the missing path. */
TEST(Generate, MissingIncludeAborts) {
	std::string msg =
		generateError("[@lua return include 'no-such-template']");
	EXPECT_NE(std::string::npos, msg.find("no-such-template")) << msg;
}

/* Brackets inside string literals (quoted and long) and comments must not
 * terminate or unbalance the block scan. */
TEST(Generate, BracketsInsideBlockLiterals) {
	EXPECT_EQ("a]b[c", generateWith(
		"a[@lua return \"]\" ]b[@lua return '[' ]c"));
	EXPECT_EQ("x]y", generateWith("[@lua return [[x]y]] ]"));
	EXPECT_EQ("z", generateWith("[@lua -- ]]]\nreturn 'z']"));
	EXPECT_EQ("q\\]", generateWith("[@lua return 'q\\\\' .. ']' ]"));
}

/* A block that never closes must fail with a scanner diagnostic instead of
 * a misleading loadstring error. */
TEST(Generate, UnterminatedBlockAborts) {
	std::string msg = generateError("\nx[@lua return \"unclosed");
	EXPECT_NE(std::string::npos, msg.find("unterminated")) << msg;
	EXPECT_NE(std::string::npos, msg.find("line 2")) << msg;
}

/* Documented marker adjacency: a backslash before [@lua emits the block
 * verbatim; a literal '[' just before it (arr[[@lua..]]) stays intact. */
TEST(Generate, PreservesMarkerAdjacency) {
	EXPECT_EQ("e\\[@lua return 1]f",
	          generateWith("e\\[@lua return 1]f"));
	EXPECT_EQ("q[7]r", generateWith("q[[@lua return 7]]r"));
}
