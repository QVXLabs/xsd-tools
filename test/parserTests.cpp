/*
 * parserTests.cpp — XSD::Parser internals.
 *
 *  This file is part of xsd-tools.
 */
#include <string>
#include <gtest/gtest.h>
#include "src/XSDParser/Parser.hpp"
#include "src/XSDParser/Elements/Schema.hpp"
#include "src/XSDParser/Exception.hpp"

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
