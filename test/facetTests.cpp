/*
 * facetTests.cpp — assertions on the actual generated TARGET output for the
 * D2/D2b facet work. The xsdparse goldens only cover the parser-model dump
 * (templates/test), not target code, so these regenerate each target via
 * XsdTools::Generate and inspect the emitted source directly.
 *
 *  This file is part of xsd-tools.
 */
#include <sstream>
#include <string>
#include <gtest/gtest.h>
#include "src/xsdtools.hpp"
#include "src/XSDParser/Exception.hpp"

namespace {
	std::string gen(const std::string& tmpl, const std::string& xsd) {
		std::ostringstream out;
		XsdTools::Generate(std::string(TEMPLATES_DIR "/") + tmpl, xsd, out);
		return out.str();
	}
	bool has(const std::string& hay, const std::string& needle) {
		return hay.find(needle) != std::string::npos;
	}
	const std::string CORPUS(XSD_CORPUS_DIR "/");
	const std::string NEGATIVE(XSD_NEGATIVE_DIR "/");
}

/* D2b: a 0..255 integer narrows to the smallest fixed-width C type (uint8_t);
 * the DOM target picks the narrowed singleton (xml_unsignedByte). */
TEST(Narrowing, CTargetsUint8) {
	const std::string xsd = CORPUS + "facet_uint8.xsd";
	EXPECT_TRUE(has(gen("c-json-jsonc.template", xsd), "uint8_t"));
	EXPECT_TRUE(has(gen("c-xml-expat", xsd), "uint8_t"));
	EXPECT_TRUE(has(gen("c-xml-expat-dom.template", xsd), "unsignedByte"));
}

/* D2b: a -32768..32767 integer narrows to int16_t. */
TEST(Narrowing, CTargetsInt16) {
	const std::string xsd = CORPUS + "facet_int16.xsd";
	EXPECT_TRUE(has(gen("c-json-jsonc.template", xsd), "int16_t"));
	EXPECT_TRUE(has(gen("c-xml-expat", xsd), "int16_t"));
}

/* D2b: Java has no unsigned types, so 0..255 narrows to the smallest SIGNED
 * boxed type that holds it (Short, not Byte). */
TEST(Narrowing, JavaTargetsShort) {
	const std::string xsd = CORPUS + "facet_uint8.xsd";
	EXPECT_TRUE(has(gen("java-json.org.tmpl", xsd), "Short"));
	EXPECT_TRUE(has(gen("java-xml-stax.tmpl", xsd), "Short"));
}

/* D2b boundary: 0..256 just exceeds uint8_t (255), so it narrows to uint16_t. */
TEST(Narrowing, CUint16Boundary) {
	EXPECT_TRUE(has(gen("c-json-jsonc.template", CORPUS + "facet_uint16.xsd"),
	                "uint16_t"));
}

/* D2 attribute validation: a faceted ATTRIBUTE emits a real check (this was
 * dead code until attribute facets were bridged). */
TEST(Facets, AttributeValidationEmitted) {
	const std::string out = gen("python-sax", CORPUS + "facet_attr.xsd");
	EXPECT_TRUE(has(out, "_size"));
	EXPECT_TRUE(has(out, "not permitted"));
}

/* Regression (bug C): a defaulted attribute (default="0") emits the default
 * initializer rather than silently dropping it. */
TEST(Bug, JavaJsonAttributeDefaultApplied) {
	const std::string out = gen("java-json.org.tmpl", CORPUS + "testA015.xsd");
	EXPECT_TRUE(has(out, "= 0;"));
}

/* D2 leanness: the java-json adapters emit only the accessors the schema uses.
 * A string/integer-only schema must not ship the binary accessors. */
TEST(DeBloat, JavaJsonOmitsUnusedAccessors) {
	const std::string out = gen("java-json.org.tmpl", CORPUS + "testA001.xsd");
	EXPECT_FALSE(has(out, "getBase64"));
	EXPECT_FALSE(has(out, "getHex"));
}

/* J: cyclic schemas are rejected with a clean exception instead of recursing
 * until the stack overflows. The model inline-expands types, so mutual and
 * self references alike are cycles. */
TEST(Cycle, MutualExtensionRejected) {
	EXPECT_THROW(gen("python-sax", NEGATIVE + "cyclic_extension.xsd"),
	             XSD::XMLException);
}
TEST(Cycle, MutualTypeReferenceRejected) {
	EXPECT_THROW(gen("python-sax", NEGATIVE + "cyclic_type.xsd"),
	             XSD::XMLException);
}
TEST(Cycle, SelfReferenceRejected) {
	EXPECT_THROW(gen("c-xml-expat", NEGATIVE + "cyclic_self.xsd"),
	             XSD::XMLException);
}

/* Bug: <simpleType><union> used to fall through to throw (dead duplicate
 * branch in SimpleType::GetParentType); it now resolves and generates. */
TEST(Bug, SimpleTypeUnionResolves) {
	EXPECT_NO_THROW(gen("python-sax", CORPUS + "bug_union.xsd"));
	EXPECT_NO_THROW(gen("java-json.org.tmpl", CORPUS + "bug_union.xsd"));
}

/* Bug: an <annotation> inside a string restriction (and inside an
 * <enumeration>) used to be gated behind the numeric type check and threw;
 * it now generates. */
TEST(Bug, AnnotationInStringRestriction) {
	EXPECT_NO_THROW(
		gen("python-sax", CORPUS + "bug_restriction_annotation.xsd"));
}
