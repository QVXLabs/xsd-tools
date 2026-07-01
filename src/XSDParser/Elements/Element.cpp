/*
 * Element.cpp
 *
 *  Created on: Jun 25, 2011
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
#include <string.h>
#include <string>
#include <tinyxml.h>
#include "./src/XSDParser/Types.hpp"
#include "./src/XSDParser/Elements/Element.hpp"
#include "./src/XSDParser/Elements/Schema.hpp"
#include "./src/XSDParser/Elements/SimpleType.hpp"
#include "./src/XSDParser/Elements/ComplexType.hpp"
#include "./src/XSDParser/Elements/Annotation.hpp"

using namespace XSD;
using namespace XSD::Elements;

Element::Element(const TiXmlElement& elm, const Parser& rParser)
	: Node(elm, rParser)
{ }

Element::Element(const Element& elm)
	: Node(elm)
{ }

void
Element::ParseChildren(BaseProcessor& rProcessor) const noexcept(false) {
	/* process children */
	eachChild_([&rProcessor](const Node& rNode) {
		if (XSD_ISELEMENT(&rNode, SimpleType) ||
			XSD_ISELEMENT(&rNode, Annotation) ||
			XSD_ISELEMENT(&rNode, ComplexType)) {
			rNode.ParseElement(rProcessor);
		} else
			throw XMLException(rNode.GetXMLElm(), XMLException::InvallidChildXMLElement);
	});
}

void
Element::ParseElement(BaseProcessor& rProcessor) const noexcept(false) {
	/* don't process if node is abstract */
	if (Abstract()) return;
	/* if an element is a substitution group verify types */
	if (HasSubstitutionGroup() && !VerifySubstitutionGroup()) {
		throw XMLException(Node::GetXMLElm(), XMLException::SubstitutionGroupTypeMismatch);
	}
	/* a name is only allowed when it's parent element is a schema */
	if (HasName()) {
		if (HasRef())
			throw XMLException(Node::GetXMLElm(), XMLException::InvalidAttribute);
	} else if (Node::IsRootNode()) {
		throw XMLException(Node::GetXMLElm(), XMLException::InvalidAttribute);
	}
	/* the "maxOccurs" attribute is prohibited if the parent element is a schema */
	if (HasMaxOccurs() && Node::IsRootNode())
		throw XMLException(Node::GetXMLElm(), XMLException::InvalidAttribute);
	/* if the node is a reference, check its reference */
	if (HasRef()) {
		std::unique_ptr<XSD::Elements::Element> pRefElm(RefElement());
		if (pRefElm->Abstract()) return;
		/* a name is only allowed when it's parent element is a schema */
		if (!pRefElm->IsRootNode())
			throw XMLException(pRefElm->GetXMLElm(), XMLException::InvalidAttribute);
		/* the "maxOccurs" attribute is prohibited if the parent element is a schema */
		if (pRefElm->HasMaxOccurs() && pRefElm->IsRootNode())
			throw XMLException(pRefElm->GetXMLElm(), XMLException::InvalidAttribute);
		/* if an element is a substitution group verify types */
		if (pRefElm->HasSubstitutionGroup() &&
			!pRefElm->VerifySubstitutionGroup()) {
			throw XMLException(Node::GetXMLElm(), XMLException::SubstitutionGroupTypeMismatch);
		}
	}
	return rProcessor.ProcessElement(this);
}

Types::BaseType * 
Element::GetParentType(void) const noexcept(false) {
	return this->Type();
}

bool
Element::Abstract() const {
	if (Node::HasAttribute("abstract"))
		return Node::GetAttribute<bool>("abstract");
	else
		return false;
}

std::string
Element::Name() const noexcept(false) {
	return Node::name_();
}

Element*
Element::SubstitutionGroup() const noexcept(false) {
	return Node::FindXSDElm<Element>(Node::GetAttribute<const char*>("substitutionGroup"));
}

bool
Element::VerifySubstitutionGroup() const noexcept(false) {
	std::unique_ptr<Element> pSubElm(this->SubstitutionGroup());
	std::unique_ptr<Types::BaseType> pType(this->Type());
	std::unique_ptr<Types::BaseType> pSubElmType(pSubElm->Type());
	return pType->isTypeRelated(pSubElmType.get());
}

Types::BaseType*
Element::Type() const noexcept(false) {
	if (HasRef()) {
		std::unique_ptr<XSD::Elements::Element> pRefElm(RefElement());
		return ParseType_(*pRefElm.get());
	} else {
		return ParseType_(*this);
	}
}

std::string
Element::Namespace() const noexcept(false) {
	std::unique_ptr<Schema> pSchema(Node::GetSchema());
	return pSchema->TargetNamespace();
}

bool
Element::Qualified() const noexcept(false) {
	/* a local form= attribute overrides the schema's elementFormDefault */
	if (Node::HasAttribute("form"))
		return "qualified" == Node::GetAttribute<std::string>("form");
	std::unique_ptr<Schema> pSchema(Node::GetSchema());
	const char* pVal =
		pSchema->GetXMLElm().Attribute("elementFormDefault");
	return pVal && 0 == strcmp(pVal, "qualified");
}

Element*
Element::RefElement() const noexcept(false) {
	return Node::FindXSDRef<Element>("ref");
}

int
Element::MaxOccurs() const {
	if (strcmp(Node::GetAttribute<const char*>("maxOccurs"), "unbounded"))
		return Node::GetAttribute<int>("maxOccurs");
	return -1;
}

int
Element::MinOccurs() const {
	return Node::GetAttribute<int>("minOccurs");
}

bool
Element::HasChildType() const {
	bool hasSmplType = Node::HasContent(SimpleType::XSDTag());
	bool hasCmplxType = Node::HasContent(ComplexType::XSDTag());
	return (hasSmplType | hasCmplxType);
}

Types::BaseType*
Element::Type_() const noexcept(false) {
	Types::BaseType* pRetType = Node::GetAttribute<Types::BaseType*>("type");
	if (XSD_ISTYPE(pRetType, Types::Unknown)) {
		delete pRetType;
		return new Types::String();
	} else if(XSD_ISTYPE(pRetType, Types::Unsupported))
		throw XMLException(Node::GetXMLElm(), XMLException::UndefiniedXSDType);
	return pRetType;
}

/* static */ Types::BaseType*
Element::ParseType_(const Element& rElm) noexcept(false) {
	if (rElm.HasChildType() &&
		(rElm.HasContent(SimpleType::XSDTag()) || rElm.HasContent(ComplexType::XSDTag()))) {
		if (rElm.HasContent(SimpleType::XSDTag())) {
			return new Types::SimpleType(rElm.FindXSDChildElm<SimpleType>());
		} else {
			return new Types::ComplexType(rElm.FindXSDChildElm<ComplexType>());
		}
	} else if (rElm.HasType()) {
		return rElm.Type_();
	} else if (rElm.HasSubstitutionGroup()) {
		std::unique_ptr<Element> pElm(rElm.SubstitutionGroup());
		return ParseType_(*(pElm.get()));
	} else {
		return new Types::String();
	}
}
