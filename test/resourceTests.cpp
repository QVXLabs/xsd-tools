/*
 * resourceTests.cpp — Core::Resource (embedded engine + template lookup).
 *
 *  This file is part of xsd-tools.
 */
#include <cstdlib>
#include <string>
#include <gtest/gtest.h>
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
