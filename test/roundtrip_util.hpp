/*
 * roundtrip_util.hpp
 *
 * Helpers for the auto-generated round-trip gtest cases: generate bindings
 * in-process via libxsdb, then build/run the generated code in its target
 * language and report success. Paths to templates / toolchain bits arrive as
 * compile definitions from CMake (TEMPLATES_DIR, LIBB64_*, EXPAT_*, ...).
 *
 *  This file is part of xsd-tools.
 */
#ifndef XSDB_ROUNDTRIP_UTIL_HPP
#define XSDB_ROUNDTRIP_UTIL_HPP

#include <string>
#include <gtest/gtest.h>

namespace xsdtest {
	/* xsd path -> basename without directory or extension. */
	std::string baseName(const std::string& xsdPath);

	/* True if `prog` resolves on PATH (used to GTEST_SKIP() gated legs). */
	bool programAvailable(const std::string& prog);

	/* Generate `templatePath` against `xsdPath` via libxsdb, returning the
	 * generated text. Throws on generation failure. */
	std::string generate(const std::string& templatePath,
	                     const std::string& xsdPath);

	/* python-sax: emit binding + self-checking driver, run the driver
	 * (marshal(unmarshal(x)) == x), assert clean exit. */
	::testing::AssertionResult pythonRoundtrip(const std::string& xsdPath);

	/* python-json: emit binding + self-checking driver, run the driver
	 * (marshal(unmarshal(doc)) == doc), assert clean exit. */
	::testing::AssertionResult pythonJsonRoundtrip(const std::string& xsdPath);

	/* c-xml-expat / c-xml-expat-dom: emit + split multi-file C binding and
	 * driver, compile (libb64 + expat) and run. */
	::testing::AssertionResult cExpatRoundtrip(const std::string& xsdPath);
	::testing::AssertionResult cExpatDomRoundtrip(const std::string& xsdPath);

	/* java-json.org: emit binding + driver into a Maven project, mvn package
	 * and run. Caller GTEST_SKIPs when mvn/JDK are unavailable. */
	::testing::AssertionResult javaRoundtrip(const std::string& xsdPath);
}

#endif /* XSDB_ROUNDTRIP_UTIL_HPP */
