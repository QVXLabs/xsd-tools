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
	/* recursive fixtures live beside the negative corpus but must not be
	   globbed by it (they are accepted, not rejected) */
	const std::string RECURSIVE(XSD_NEGATIVE_DIR "/../xsd-recursive/");
	/* fixtures referenced explicitly (no golden, no round-trip glob) */
	const std::string PARSE(XSDPARSE_DIR "/");
}

/* D2b: a 0..255 integer narrows to the smallest fixed-width C type (uint8_t);
 * the DOM target picks the narrowed singleton (xml_unsignedByte). */
TEST(Narrowing, CTargetsUint8) {
	const std::string xsd = CORPUS + "facet_uint8.xsd";
	EXPECT_TRUE(has(gen("c-json-jsonc", xsd), "uint8_t"));
	EXPECT_TRUE(has(gen("c-xml-expat", xsd), "uint8_t"));
	EXPECT_TRUE(has(gen("c-xml-expat-dom", xsd), "unsignedByte"));
}

/* D2b: a -32768..32767 integer narrows to int16_t. */
TEST(Narrowing, CTargetsInt16) {
	const std::string xsd = CORPUS + "facet_int16.xsd";
	EXPECT_TRUE(has(gen("c-json-jsonc", xsd), "int16_t"));
	EXPECT_TRUE(has(gen("c-xml-expat", xsd), "int16_t"));
}

/* D2b: Java has no unsigned types, so 0..255 narrows to the smallest SIGNED
 * boxed type that holds it (Short, not Byte). */
TEST(Narrowing, JavaTargetsShort) {
	const std::string xsd = CORPUS + "facet_uint8.xsd";
	EXPECT_TRUE(has(gen("java-json.org", xsd), "Short"));
	EXPECT_TRUE(has(gen("java-xml-stax", xsd), "Short"));
}

/* D2b boundary: 0..256 just exceeds uint8_t (255), so it narrows to uint16_t. */
TEST(Narrowing, CUint16Boundary) {
	EXPECT_TRUE(has(gen("c-json-jsonc", CORPUS + "facet_uint16.xsd"),
	                "uint16_t"));
}

/* D2 attribute validation: a faceted ATTRIBUTE emits a real check (this was
 * dead code until attribute facets were bridged). */
TEST(Facets, AttributeValidationEmitted) {
	const std::string out = gen("python-sax", CORPUS + "facet_attr.xsd");
	EXPECT_TRUE(has(out, "_size"));
	EXPECT_TRUE(has(out, "not permitted"));
}

/* cpp-xml-expat enum validation: a large string enum (>8 values) uses the O(1)
 * sdbm hash-switch; a small enum keeps the cheap == chain. The validation switch
 * is distinguished from element/attribute dispatch by hashing a .c_str(). */
TEST(Facets, CppEnumHashSwitchForLargeEnum) {
	const std::string out = gen("cpp-xml-expat", CORPUS + "facet_enum_large.xsd");
	EXPECT_TRUE(has(out, "sdbmHash_(((*state_)).c_str())"));
}
TEST(Facets, CppEnumChainForSmallEnum) {
	const std::string out = gen("cpp-xml-expat", CORPUS + "facet_attr.xsd");
	EXPECT_TRUE(has(out, "size_ == \"small\""));
	EXPECT_FALSE(has(out, ".c_str())) {"));  // no enum-validation hash-switch
}

/* Regression (bug C): a defaulted attribute (default="0") emits the default
 * initializer rather than silently dropping it. */
TEST(Bug, JavaJsonAttributeDefaultApplied) {
	const std::string out = gen("java-json.org", CORPUS + "testA015.xsd");
	EXPECT_TRUE(has(out, "= 0;"));
}

/* D2 leanness: the java-json adapters emit only the accessors the schema uses.
 * A string/integer-only schema must not ship the binary accessors. */
TEST(DeBloat, JavaJsonOmitsUnusedAccessors) {
	const std::string out = gen("java-json.org", CORPUS + "testA001.xsd");
	EXPECT_FALSE(has(out, "getBase64"));
	EXPECT_FALSE(has(out, "getHex"));
}

/* J: a genuine type-DERIVATION cycle (A extends B extends A) cannot be
 * inline-expanded and is still rejected with a clean exception rather than
 * overflowing the stack. */
TEST(Cycle, MutualExtensionRejected) {
	EXPECT_THROW(gen("python-sax", NEGATIVE + "cyclic_extension.xsd"),
	             XSD::XMLException);
}

/* Recursive element STRUCTURE (self- and mutual-recursive element types) is
 * now represented by-reference and generates instead of being rejected; a deep
 * recursive document round-trips through the generated code (verified out of
 * band). Here we assert generation succeeds and the recursive element is
 * present in the dispatch. */
TEST(Cycle, SelfReferenceGenerates) {
	std::string out;
	EXPECT_NO_THROW(out = gen("python-sax", RECURSIVE + "cyclic_self.xsd"));
	EXPECT_TRUE(has(out, "'child'"));
}
TEST(Cycle, MutualReferenceGenerates) {
	EXPECT_NO_THROW(gen("python-sax", RECURSIVE + "cyclic_type.xsd"));
}

/* Bug: <simpleType><union> used to fall through to throw (dead duplicate
 * branch in SimpleType::GetParentType); it now resolves and generates. */
TEST(Bug, SimpleTypeUnionResolves) {
	EXPECT_NO_THROW(gen("python-sax", CORPUS + "bug_union.xsd"));
	EXPECT_NO_THROW(gen("java-json.org", CORPUS + "bug_union.xsd"));
}

/* Bug: an <annotation> inside a string restriction (and inside an
 * <enumeration>) used to be gated behind the numeric type check and threw;
 * it now generates. */
TEST(Bug, AnnotationInStringRestriction) {
	EXPECT_NO_THROW(
		gen("python-sax", CORPUS + "bug_restriction_annotation.xsd"));
}

/* E3: every XML target references the targetNamespace URI for a namespaced
 * schema (emitted as xmlns/xmlns:tns at marshal time — literal in the C/python
 * output, via the StAX namespace API in Java). Non-namespaced output stays
 * bare. */
TEST(Namespace, EmittedAcrossXmlTargets) {
	const std::string xsd = CORPUS + "nsTargetPrefixed.xsd";
	const char* tmpls[] = { "c-xml-expat", "c-xml-expat-dom",
	                        "python-sax", "java-xml-stax" };
	for (size_t i = 0; i < sizeof(tmpls) / sizeof(*tmpls); ++i)
		EXPECT_TRUE(has(gen(tmpls[i], xsd), "urn:example:phase0")) << tmpls[i];
	/* a non-namespaced schema emits no xmlns */
	EXPECT_FALSE(has(gen("c-xml-expat", CORPUS + "testA001.xsd"), "xmlns="));
}

/* Attribute storage: an optional attribute with no default/fixed becomes a
 * unique_ptr<T> (C++) so absent != present, and is marshalled only when set;
 * a required attribute stays a plain value. testA013's myAttr is optional. */
TEST(Attr, CppOptionalIsUniquePtr) {
	const std::string out = gen("cpp-xml-expat", CORPUS + "testA013.xsd");
	EXPECT_TRUE(has(out, "std::unique_ptr<std::string> myAttr_;"));
	/* marshalled only when present -> absent optional round-trips as absent */
	EXPECT_TRUE(has(out, "if (obj.myAttr_) {"));
}

/* A required attribute is a plain (always-present) const member, not a pointer. */
TEST(Attr, CppRequiredIsPlainValue) {
	const std::string out = gen("cpp-xml-expat", CORPUS + "facet_attr.xsd");
	EXPECT_TRUE(has(out, "const std::string size_;"));
	EXPECT_FALSE(has(out, "unique_ptr<std::string> size_"));
}

/* Attribute dispatch routes through the sdbm hash, not a per-class
 * setAttribute string-compare chain. */
TEST(Attr, CppDispatchUsesHashNotSetAttribute) {
	const std::string out = gen("cpp-xml-expat", CORPUS + "facet_attr.xsd");
	EXPECT_TRUE(has(out, "switch (sdbmHash_(local))"));
	EXPECT_FALSE(has(out, "setAttribute"));
}

/* A defaulted attribute (default="0") seeds its parse local with the codegen-
 * known literal, so an absent attribute is constructed with its default. */
TEST(Attr, CppDefaultValueInitialized) {
	const std::string out = gen("cpp-xml-expat", CORPUS + "testA015.xsd");
	EXPECT_TRUE(has(out, "int32_t a_myAttr = 0;"));
}

/* Bug: a ref= element's minOccurs/maxOccurs live on the referencing site
 * (the global element it resolves to legally cannot carry them); the site's
 * occurs must land in the model. ref_occurs.xsd refs with 0..unbounded. */
TEST(Bug, RefElementKeepsSiteOccurs) {
	const std::string out = gen("test", PARSE + "ref_occurs.xsd");
	EXPECT_TRUE(has(out, "minOccurs = 0"));
	EXPECT_TRUE(has(out, "maxOccurs = -1"));
}

/* Bug: a derived restriction's scalar facet must win over the base's (D
 * restricts B with maxLength=10 where B has maxLength=100 -> 10 applies),
 * a derived enumeration REPLACES the base's, and pattern facets from every
 * derivation step accumulate (they are ANDed in XSD). */
TEST(Bug, DerivedFacetOverridesBase) {
	const std::string out = gen("test", PARSE + "facet_override.xsd");
	EXPECT_TRUE(has(out, "maxLength = 10\n"));
	EXPECT_FALSE(has(out, "maxLength = 100"));
	EXPECT_TRUE(has(out, "enumeration = { alpha, beta }"));
	EXPECT_FALSE(has(out, "gamma"));
	EXPECT_TRUE(has(out, "pattern = { [ab].*, [a-z]+ }"));
}

/* Bug: numeric facet bounds were rounded to 6 significant digits
 * (4294967295 -> "4.29497e+09"). Integral bounds print in plain integer
 * form; fractional ones keep round-trip precision. */
TEST(Bug, NumericFacetFullPrecision) {
	const std::string out = gen("test", PARSE + "facet_precision.xsd");
	EXPECT_TRUE(has(out, "maxInclusive = 4294967295"));
	EXPECT_FALSE(has(out, "e+09"));
	EXPECT_TRUE(has(out, "minInclusive = 2.5"));
}

/* Java centralizes attribute unmarshalling in Element.unmarshal via a native
 * switch (compiler-lowered to hashCode dispatch); no per-class readAttributes,
 * and the field is package-private so the driver can assign it. */
TEST(Attr, JavaDispatchCentralized) {
	const std::string out = gen("java-xml-stax", CORPUS + "testA013.xsd");
	EXPECT_TRUE(has(out, "switch (localName())"));
	EXPECT_TRUE(has(out, "myAttr\": o._myAttr = av;"));
	EXPECT_FALSE(has(out, "readAttributes"));
	EXPECT_TRUE(has(out, "\tString _myAttr;"));
}
