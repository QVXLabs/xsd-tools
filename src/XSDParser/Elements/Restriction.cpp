/*
 * Restriction.cpp
 *
 *  Created on: Jun 26, 2011
 *      Author: QVXLabs LLC
 *   Copyright: (c)2011 QVXLabs LLC
 *
 *  This file is part of xsd-tools.
 *
 *  xsd-tools is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  xsd-tools is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with xsd-tools.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef TIXML_USE_STL
#	define TIXML_USE_STL
#endif /* TIXML_USE_STL */
#include <memory>
#include <tinyxml.h>
#include "./src/XSDParser/Elements/Restriction.hpp"
#include "./src/XSDParser/Elements/Group.hpp"
#include "./src/XSDParser/Elements/Choice.hpp"
#include "./src/XSDParser/Elements/Sequence.hpp"
#include "./src/XSDParser/Elements/Attribute.hpp"
#include "./src/XSDParser/Elements/SimpleType.hpp"
#include "./src/XSDParser/Elements/ComplexType.hpp"
#include "./src/XSDParser/Elements/ComplexContent.hpp"
#include "./src/XSDParser/Elements/SimpleContent.hpp"
#include "./src/XSDParser/Elements/MinExclusive.hpp"
#include "./src/XSDParser/Elements/MaxExclusive.hpp"
#include "./src/XSDParser/Elements/MinInclusive.hpp"
#include "./src/XSDParser/Elements/MaxInclusive.hpp"
#include "./src/XSDParser/Elements/MinLength.hpp"
#include "./src/XSDParser/Elements/MaxLength.hpp"
#include "./src/XSDParser/Elements/Length.hpp"
#include "./src/XSDParser/Elements/Enumeration.hpp"
#include "./src/XSDParser/Elements/FractionDigits.hpp"
#include "./src/XSDParser/Elements/Pattern.hpp"
#include "./src/XSDParser/Elements/TotalDigits.hpp"
#include "./src/XSDParser/Elements/WhiteSpace.hpp"
#include "./src/XSDParser/Elements/Annotation.hpp"
#include "./src/XSDParser/Elements/All.hpp"
#include "./src/Processors/RestrictionVerify.hpp"

using namespace XSD;
using namespace XSD::Elements;

Restriction::Restriction(const TiXmlElement& elm, const Parser& rParser)
	: Node(elm, rParser)
{ }

Restriction::Restriction(const Restriction& rCpy)
	: Node(rCpy)
{ }

void
Restriction::ParseChildren(BaseProcessor& rProcessor) const noexcept(false) {
	if (isParentComplexContent()) {
		/* process children */
		eachChild_([&rProcessor](const Node& rNode) {
			if (XSD_ISELEMENT(&rNode, Group) ||
				XSD_ISELEMENT(&rNode, Choice) ||
				XSD_ISELEMENT(&rNode, Sequence) ||
				XSD_ISELEMENT(&rNode, XSD::Elements::Attribute) ||
				XSD_ISELEMENT(&rNode, All) ||
				XSD_ISELEMENT(&rNode, Annotation) ||
				XSD_ISELEMENT(&rNode, XSD::Elements::ComplexContent)) {
				rNode.ParseElement(rProcessor);
			} else
				throw XMLException(rNode.GetXMLElm(), XMLException::InvallidChildXMLElement);
		});
	} else if (isParentSimpleContent()){
		/* process children */
		eachChild_([&rProcessor](const Node& rNode) {
			if (XSD_ISELEMENT(&rNode, XSD::Elements::Attribute) ||
				XSD_ISELEMENT(&rNode, XSD::Elements::MinExclusive) ||
				XSD_ISELEMENT(&rNode, XSD::Elements::MaxExclusive) ||
				XSD_ISELEMENT(&rNode, XSD::Elements::MinInclusive) ||
				XSD_ISELEMENT(&rNode, XSD::Elements::MaxInclusive) ||
				XSD_ISELEMENT(&rNode, XSD::Elements::FractionDigits) ||
				XSD_ISELEMENT(&rNode, XSD::Elements::TotalDigits) ||
				XSD_ISELEMENT(&rNode, XSD::Elements::MinLength) ||
				XSD_ISELEMENT(&rNode, XSD::Elements::MaxLength) ||
				XSD_ISELEMENT(&rNode, XSD::Elements::Length) ||
				XSD_ISELEMENT(&rNode, XSD::Elements::Pattern) ||
				XSD_ISELEMENT(&rNode, XSD::Elements::WhiteSpace) ||
				XSD_ISELEMENT(&rNode, XSD::Elements::Enumeration) ||
				XSD_ISELEMENT(&rNode, XSD::Elements::Annotation)) {
				  rNode.ParseElement(rProcessor);
			} else
				throw XMLException(rNode.GetXMLElm(), XMLException::InvallidChildXMLElement);
		});
	} else {
		/* process children */
		std::unique_ptr<Types::BaseType> pBase(Base());
		eachChild_([&rProcessor, &pBase](const Node& rNode) {
			if (XSD_ISELEMENT(&rNode, XSD::Elements::Annotation)) {
				/* annotation is always permitted, regardless of base type */
				rNode.ParseElement(rProcessor);
			} else if (XSD_ISELEMENT(&rNode, XSD::Elements::MinExclusive) ||
				XSD_ISELEMENT(&rNode, XSD::Elements::MaxExclusive) ||
				XSD_ISELEMENT(&rNode, XSD::Elements::MinInclusive) ||
				XSD_ISELEMENT(&rNode, XSD::Elements::MaxInclusive) ||
				XSD_ISELEMENT(&rNode, XSD::Elements::FractionDigits) ||
				XSD_ISELEMENT(&rNode, XSD::Elements::TotalDigits)) {
				/* verify that parent restriction type is numeric */
				Types::Decimal	allowedBaseType;
				if (!pBase->isTypeRelated(&allowedBaseType))
					throw XMLException(rNode.GetXMLElm(), XMLException::InvallidChildXMLElement);
				else {
					/* process element */
					rNode.ParseElement(rProcessor);
				}
			} else if ( XSD_ISELEMENT(&rNode, XSD::Elements::MinLength) ||
						XSD_ISELEMENT(&rNode, XSD::Elements::MaxLength) ||
						XSD_ISELEMENT(&rNode, XSD::Elements::Length) ||
						XSD_ISELEMENT(&rNode, XSD::Elements::Pattern) ||
						XSD_ISELEMENT(&rNode, XSD::Elements::WhiteSpace)) {
				/* verify that parent restriction type is a string */
				Types::String	allowedBaseType;
				if (!pBase->isTypeRelated(&allowedBaseType))
					throw XMLException(rNode.GetXMLElm(), XMLException::InvallidChildXMLElement);
				else {
					/* process element */
					rNode.ParseElement(rProcessor);
				}
			} else if (XSD_ISELEMENT(&rNode, XSD::Elements::Enumeration)) {
				/* process element */
				rNode.ParseElement(rProcessor);
			} else
				throw XMLException(rNode.GetXMLElm(), XMLException::InvallidChildXMLElement);
		});
	}
}

void
Restriction::ParseElement(BaseProcessor& rProcessor) const noexcept(false) {
	/* make sure the base element is there */
	if (!Node::HasAttribute("base"))
		throw XMLException(GetXMLElm(), XMLException::MissingAttribute);
	/* make sure that if the parent is a simpleContent element that the base
	 * type is a simple type or base type */
	if (isParentSimpleContent()) {
		std::unique_ptr<Types::BaseType> pType(Base());
		if (XSD_ISTYPE(pType.get(), Types::ComplexType))
			throw XMLException(GetXMLElm(), XMLException::RestrictionTypeMismatch);
	}
	/* make sure the restriction is related to the base element */
	std::unique_ptr<Types::BaseType> pBase(Base());
	if (XSD_ISTYPE(pBase.get(), Types::ComplexType)) {
		Types::ComplexType* pCmplxType = static_cast<Types::ComplexType*>(pBase.get());
		Processors::RestrictionVerify verifyRestriciton;
		if (!verifyRestriciton.Verify(this, pCmplxType->m_pValue))
			throw XMLException(GetXMLElm(), XMLException::RestrictionTypeMismatch);
	}
	/* process element */
	rProcessor.ProcessRestriction(this);
}

Types::BaseType *
Restriction::GetParentType() const noexcept(false) {
	return this->Base();
}

bool
Restriction::isParentComplexContent() const noexcept(false) {
	std::unique_ptr<Node> pParent(Node::Parent());
	return XSD_ISELEMENT(pParent.get(), XSD::Elements::ComplexContent);
}

bool
Restriction::isParentSimpleContent() const noexcept(false) {
	std::unique_ptr<Node> pParent(Node::Parent());
	return XSD_ISELEMENT(pParent.get(), XSD::Elements::SimpleContent);
}

Types::BaseType*
Restriction::Base() const noexcept(false) {
	return Node::baseType_();
}
