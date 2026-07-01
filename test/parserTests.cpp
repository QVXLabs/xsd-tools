/*
 * parserTests.cpp — XSD::Parser internals.
 *
 *  This file is part of xsd-tools.
 */
#include <sstream>
#include <string>
#include <gtest/gtest.h>
#include "src/XSDParser/Parser.hpp"
#include "src/XSDParser/Elements/Schema.hpp"
#include "src/XSDParser/Exception.hpp"
#include "src/XSDParser/XsdQName.hpp"
#include "src/xsdtools.hpp"

TEST(Parser, ParsesPositiveSchema) {
	XSD::Parser parser;
	XSD::Elements::Schema* root = NULL;
	ASSERT_NO_THROW(
		root = parser.Parse(std::string(XSD_CORPUS_DIR "/testA001.xsd")));
	EXPECT_NE((XSD::Elements::Schema*)NULL, root);
	delete root;
}

TEST(Parser, ThrowsOnMissingFile) {
	XSD::Parser parser;
	EXPECT_THROW(parser.Parse(std::string("/nonexistent/missing.xsd")),
	             XSD::XMLException);
}

/* An unsupported protocol must throw — never return NULL — and must not
 * poison the document cache: a retry of the same URI reports the same
 * clear error rather than a stale TIXML_NO_ERROR. */
TEST(Parser, ThrowsOnUnsupportedProtocol) {
	XSD::Parser parser;
	for (int attempt = 0; attempt < 2; ++attempt) {
		try {
			delete parser.Parse(std::string("http://example.com/x.xsd"));
			FAIL() << "expected XMLException on attempt " << attempt;
		} catch (const XSD::XMLException& e) {
			EXPECT_EQ((int)XSD::XMLException::ProtocolNotSupported,
			          e.QueryError().errorId_)
				<< "attempt " << attempt << ": " << e.what();
		}
	}
}

/* xs:include resolution with a prefix-qualified root schema: the include tag
 * must be searched under the root's XSD-lang prefix, and the searched type
 * tag re-qualified per included document (xsd:-prefixed and default-xmlns
 * included docs both resolve). */
TEST(Schema, PrefixedIncludeResolvesAcrossPrefixes) {
	std::ostringstream out;
	ASSERT_NO_THROW(XsdTools::Generate(TEMPLATES_DIR "/test",
		std::string(XSDPARSE_DIR "/inclPrefixed.xsd"), out));
	/* content of the included types must appear in the generated model */
	EXPECT_NE(std::string::npos, out.str().find("FullName"));
	EXPECT_NE(std::string::npos, out.str().find("Sku"));
}

/* Phase 0 regression: a schema declaring no namespace at all must still
 * resolve bare builtin refs (testA010 uses base="string" with no xmlns).
 * ns-by-URI builtin detection would otherwise reject it. */
TEST(Schema, NoNamespaceResolvesBareBuiltins) {
	XSD::Parser parser;
	XSD::Elements::Schema* root = NULL;
	ASSERT_NO_THROW(
		root = parser.Parse(std::string(XSD_CORPUS_DIR "/testA010.xsd")));
	ASSERT_NE((XSD::Elements::Schema*)NULL, root);
	EXPECT_EQ(std::string(""), root->Namespace());
	EXPECT_EQ(std::string(""), root->TargetNamespace());
	EXPECT_EQ(std::string(""), root->ResolvePrefix(""));
	delete root;
}

/* The XSD-lang declared as the bare default xmlns: the default prefix
 * resolves to XSD_NS, and Namespace() preserves its legacy "xmlns" tag
 * sentinel (QualifyElementName specifically skips "xmlns", so no prefix is
 * prepended). */
TEST(Schema, DefaultNamespaceIsXsdLang) {
	XSD::Parser parser;
	XSD::Elements::Schema* root = NULL;
	ASSERT_NO_THROW(
		root = parser.Parse(std::string(XSD_CORPUS_DIR "/testA034.xsd")));
	ASSERT_NE((XSD::Elements::Schema*)NULL, root);
	EXPECT_EQ(std::string("xmlns"), root->Namespace());
	EXPECT_EQ(std::string(XSD::XSD_NS), root->ResolvePrefix(""));
	delete root;
}

/* Prefixed XSD-lang form: Namespace() returns the "xs" tag prefix and the
 * prefix resolves to XSD_NS; an undeclared prefix resolves to "". */
TEST(Schema, PrefixedXsdLangResolves) {
	XSD::Parser parser;
	XSD::Elements::Schema* root = NULL;
	ASSERT_NO_THROW(
		root = parser.Parse(std::string(XSD_CORPUS_DIR "/bug_union.xsd")));
	ASSERT_NE((XSD::Elements::Schema*)NULL, root);
	EXPECT_EQ(std::string("xs"), root->Namespace());
	EXPECT_EQ(std::string(XSD::XSD_NS), root->ResolvePrefix("xs"));
	EXPECT_EQ(std::string(""), root->ResolvePrefix("undeclared"));
	delete root;
}

/* Multi-namespace positive case: XSD-lang on "xs", the target on "tns".
 * A prefixed self-ref type="tns:RecordType" resolves to the same-document
 * target NS; the whole schema generates without throwing. */
TEST(Schema, TargetNamespacePrefixedSelfRefResolves) {
	XSD::Parser parser;
	XSD::Elements::Schema* root = NULL;
	ASSERT_NO_THROW(root = parser.Parse(
		std::string(XSD_CORPUS_DIR "/nsTargetPrefixed.xsd")));
	ASSERT_NE((XSD::Elements::Schema*)NULL, root);
	EXPECT_EQ(std::string("urn:example:phase0"), root->TargetNamespace());
	EXPECT_EQ(std::string("urn:example:phase0"), root->ResolvePrefix("tns"));
	EXPECT_EQ(std::string(XSD::XSD_NS), root->ResolvePrefix("xs"));
	delete root;
}

/* Multi-namespace positive case: the default xmlns equals the target, so an
 * unprefixed self-ref resolves through the default-xmlns URI to the target. */
TEST(Schema, DefaultNamespaceEqualsTargetResolves) {
	XSD::Parser parser;
	XSD::Elements::Schema* root = NULL;
	ASSERT_NO_THROW(root = parser.Parse(
		std::string(XSD_CORPUS_DIR "/nsTargetDefault.xsd")));
	ASSERT_NE((XSD::Elements::Schema*)NULL, root);
	EXPECT_EQ(std::string("urn:example:phase0default"),
	          root->TargetNamespace());
	EXPECT_EQ(std::string("urn:example:phase0default"), root->ResolvePrefix(""));
	EXPECT_EQ(std::string(XSD::XSD_NS), root->ResolvePrefix("xs"));
	delete root;
}

/* E2: the importing schema declares the imported namespace on a prefix; the
 * prefix resolves to the imported namespace URI (end-to-end cross-document
 * type resolution is covered by the nsImport golden in generateTests). */
TEST(Schema, ImportedNamespacePrefixResolves) {
	XSD::Parser parser;
	XSD::Elements::Schema* root = NULL;
	ASSERT_NO_THROW(root = parser.Parse(
		std::string(XSD_CORPUS_DIR "/nsImport.xsd")));
	ASSERT_NE((XSD::Elements::Schema*)NULL, root);
	EXPECT_EQ(std::string("urn:example:order"), root->TargetNamespace());
	EXPECT_EQ(std::string("urn:example:address"), root->ResolvePrefix("addr"));
	delete root;
}
