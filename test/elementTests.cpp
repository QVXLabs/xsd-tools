/*
 * elementTests.cpp — regression tests for the XSD element/type model:
 * xs:all child iteration, substitution-group verification (direct and via
 * ref), builtin type-derivation direction, and xs:attribute child dispatch.
 * Fixtures live under xsdparse/ (not the auto-globbed corpora).
 *
 *  This file is part of xsd-tools.
 */
#include <memory>
#include <sstream>
#include <string>
#include <gtest/gtest.h>
#include "src/xsdtools.hpp"
#include "src/XSDParser/Parser.hpp"
#include "src/XSDParser/Exception.hpp"
#include "src/XSDParser/ProcessorBase.hpp"
#include "src/XSDParser/Elements/Node.hpp"
#include "src/XSDParser/Elements/Schema.hpp"
#include "src/XSDParser/Elements/Element.hpp"
#include "src/XSDParser/Elements/ComplexType.hpp"
#include "src/XSDParser/Elements/Sequence.hpp"
#include "src/XSDParser/Elements/All.hpp"
#include "src/XSDParser/Elements/Attribute.hpp"

namespace {
	std::string gen(const std::string& xsd) {
		std::ostringstream out;
		XsdTools::Generate(TEMPLATES_DIR "/test",
		                   std::string(XSDPARSE_DIR "/") + xsd, out);
		return out.str();
	}
	bool has(const std::string& hay, const std::string& needle) {
		return hay.find(needle) != std::string::npos;
	}

	/* Minimal BaseProcessor: recurses containers, counts the leaf callbacks
	 * whose dispatch the tests assert on; everything else is a no-op. */
	#define NOOP_CB(METHOD, TYPE) \
		virtual void METHOD(const XSD::Elements::TYPE*) {}
	struct CountingProcessor : public XSD::BaseProcessor {
		int simpleTypes_;
		int annotations_;
		int elements_;
		CountingProcessor()
			: simpleTypes_(0), annotations_(0), elements_(0) {}
		virtual void ProcessSchema(const XSD::Elements::Schema* pNode) {
			pNode->ParseChildren(*this);
		}
		virtual void ProcessElement(const XSD::Elements::Element* pNode) {
			++elements_;
			pNode->ParseChildren(*this);
		}
		virtual void ProcessComplexType(
			const XSD::Elements::ComplexType* pNode) {
			pNode->ParseChildren(*this);
		}
		virtual void ProcessSequence(const XSD::Elements::Sequence* pNode) {
			pNode->ParseChildren(*this);
		}
		virtual void ProcessAll(const XSD::Elements::All* pNode) {
			pNode->ParseChildren(*this);
		}
		virtual void ProcessAttribute(const XSD::Elements::Attribute* pNode) {
			pNode->ParseChildren(*this);
		}
		virtual void ProcessSimpleType(const XSD::Elements::SimpleType*) {
			++simpleTypes_;
		}
		virtual void ProcessAnnotation(const XSD::Elements::Annotation*) {
			++annotations_;
		}
		NOOP_CB(ProcessUnion, Union)
		NOOP_CB(ProcessRestriction, Restriction)
		NOOP_CB(ProcessList, List)
		NOOP_CB(ProcessChoice, Choice)
		NOOP_CB(ProcessGroup, Group)
		NOOP_CB(ProcessAny, Any)
		NOOP_CB(ProcessComplexContent, ComplexContent)
		NOOP_CB(ProcessExtension, Extension)
		NOOP_CB(ProcessSimpleContent, SimpleContent)
		NOOP_CB(ProcessMinExclusive, MinExclusive)
		NOOP_CB(ProcessMaxExclusive, MaxExclusive)
		NOOP_CB(ProcessMinInclusive, MinInclusive)
		NOOP_CB(ProcessMaxInclusive, MaxInclusive)
		NOOP_CB(ProcessMinLength, MinLength)
		NOOP_CB(ProcessMaxLength, MaxLength)
		NOOP_CB(ProcessLength, Length)
		NOOP_CB(ProcessEnumeration, Enumeration)
		NOOP_CB(ProcessFractionDigits, FractionDigits)
		NOOP_CB(ProcessPattern, Pattern)
		NOOP_CB(ProcessTotalDigits, TotalDigits)
		NOOP_CB(ProcessWhiteSpace, WhiteSpace)
		NOOP_CB(ProcessAttributeGroup, AttributeGroup)
		NOOP_CB(ProcessInclude, Include)
		NOOP_CB(ProcessDocumentation, Documentation)
		NOOP_CB(ProcessAppInfo, AppInfo)
	};
	#undef NOOP_CB
}

/* xs:all iterates the whole sibling chain: every member is emitted even with
 * a leading annotation. */
TEST(All, EmitsEveryMember) {
	std::string out;
	ASSERT_NO_THROW(out = gen("allMembers.xsd"));
	EXPECT_TRUE(has(out, "firstName"));
	EXPECT_TRUE(has(out, "lastName"));
	EXPECT_TRUE(has(out, "age"));
}

/* Model-level check of the same: all three xs:all members reach the
 * processor. */
TEST(All, DispatchesEveryMember) {
	XSD::Parser parser;
	std::unique_ptr<XSD::Elements::Schema> pRoot(
		parser.Parse(std::string(XSDPARSE_DIR "/allMembers.xsd")));
	ASSERT_NE((XSD::Elements::Schema*)NULL, pRoot.get());
	CountingProcessor counter;
	ASSERT_NO_THROW(pRoot->ParseElement(counter));
	/* person + firstName/lastName/age */
	EXPECT_EQ(4, counter.elements_);
	EXPECT_EQ(1, counter.annotations_);
}

/* A ref to a substitution-group member verifies the referenced element (the
 * ref site carries no substitutionGroup attribute), and a member whose type
 * derives from the head's (xs:int under xs:integer, directly or through a
 * user simpleType) is accepted. */
TEST(SubstitutionGroup, RefAndDerivedTypesAccepted) {
	std::string out;
	EXPECT_NO_THROW(out = gen("substRefValid.xsd"));
	EXPECT_TRUE(has(out, "member"));
}

/* The inverse derivation (member typed xs:integer under a head typed xs:int)
 * is a genuine type mismatch and must be rejected. */
TEST(SubstitutionGroup, BaseTypedMemberRejected) {
	try {
		gen("substRefInvalid.xsd");
		FAIL() << "expected XMLException";
	} catch (const XSD::XMLException& e) {
		EXPECT_EQ((int)XSD::XMLException::SubstitutionGroupTypeMismatch,
		          e.QueryError().errorId_);
	}
}

/* xs:attribute children double-dispatch to their own callbacks: the inline
 * simpleType reaches ProcessSimpleType exactly once and the annotation
 * reaches ProcessAnnotation (never ProcessSimpleType via a bogus cast). */
TEST(Attribute, ChildrenDispatchToOwnCallbacks) {
	XSD::Parser parser;
	std::unique_ptr<XSD::Elements::Schema> pRoot(
		parser.Parse(std::string(XSDPARSE_DIR "/attrAnnotation.xsd")));
	ASSERT_NE((XSD::Elements::Schema*)NULL, pRoot.get());
	CountingProcessor counter;
	ASSERT_NO_THROW(pRoot->ParseElement(counter));
	EXPECT_EQ(1, counter.simpleTypes_);
	EXPECT_EQ(1, counter.annotations_);
}

/* End-to-end: the annotated attribute schema generates cleanly. */
TEST(Attribute, AnnotatedAttributeGenerates) {
	EXPECT_NO_THROW(gen("attrAnnotation.xsd"));
}
