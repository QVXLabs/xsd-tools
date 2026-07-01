/*
 * resourceTests.cpp — Core::Resource (embedded engine + template lookup).
 *
 *  This file is part of xsd-tools.
 */
#include <cstdlib>
#include <fstream>
#include <string>
#include <gtest/gtest.h>
#if !defined(_WIN32)
#	include <sys/stat.h>
#endif
#include "src/resource.hpp"

TEST(Resource, EngineScriptNonEmpty) {
	Core::Resource resource;
	size_t sz = 0;
	const uint8_t* p = resource.GetEngineScript(&sz);
	EXPECT_NE((const uint8_t*)NULL, p);
	EXPECT_GT(sz, 0u);
}

TEST(Resource, ResolvesAbsoluteTemplatePath) {
	Core::Resource resource;
	const std::string path(TEMPLATES_DIR "/test");
	EXPECT_EQ(path, resource.GetTemplatePath(path));
}

#if !defined(_WIN32)
/* A template installed under $HOME/.xsdtools/templates must resolve. */
TEST(Resource, ResolvesHomeInstalledTemplate) {
	char homeTmpl[] = "/tmp/xsdb-home-XXXXXX";
	ASSERT_NE((char*)NULL, mkdtemp(homeTmpl));
	const std::string home(homeTmpl);
	const std::string dir = home + "/.xsdtools/templates";
	ASSERT_EQ(0, mkdir((home + "/.xsdtools").c_str(), 0700));
	ASSERT_EQ(0, mkdir(dir.c_str(), 0700));
	const std::string name("xsdb-home-template-test");
	{
		std::ofstream f((dir + "/" + name).c_str());
		f << "-- template stub\n";
	}
	unsetenv("XSDTOOLS_DATA");
	const char* pOldHome = getenv("HOME");
	setenv("HOME", home.c_str(), 1);
	Core::Resource resource;
	std::string resolved;
	EXPECT_NO_THROW(resolved = resource.GetTemplatePath(name));
	EXPECT_EQ(dir + "/" + name, resolved);
	if (pOldHome)
		setenv("HOME", pOldHome, 1);
	else
		unsetenv("HOME");
}
#endif

TEST(Resource, ThrowsOnMissingTemplate) {
	/* clear the env override so the search falls through to a throw */
#if defined(_WIN32)
	_putenv("XSDTOOLS_DATA=");
#else
	unsetenv("XSDTOOLS_DATA");
#endif
	Core::Resource resource;
	EXPECT_THROW(resource.GetTemplatePath("xsdb-no-such-template-xyz"),
	             Core::ResourceException);
}
