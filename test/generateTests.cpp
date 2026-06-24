/*
 * generateTests.cpp — XsdTools::Generate output matches the recorded golden
 * corpus (the gtest replacement for xsdparse-test.py's snapshot intent).
 *
 *  This file is part of xsd-tools.
 */
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "src/xsdtools.hpp"

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
