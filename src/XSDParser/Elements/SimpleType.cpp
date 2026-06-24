/*
 * SimpleType.cpp
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
 *  along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */
#if !defined(TIXML_USE_STL)
#define TIXML_USE_STL
#endif
#include <tinyxml.h>
#include "./src/XSDParser/Elements/SimpleType.hpp"
#include "./src/XSDParser/Elements/Union.hpp"
#include "./src/XSDParser/Elements/Restriction.hpp"
#include "./src/XSDParser/Elements/List.hpp"
#include "./src/XSDParser/Elements/Schema.hpp"
#include "./src/XSDParser/Elements/Annotation.hpp"

using namespace XSD;
using namespace XSD::Elements;

SimpleType::SimpleType(const TiXmlElement& elm, const Parser& rParser)
	: Node(elm, rParser)
{ }

SimpleType::SimpleType(const SimpleType& rType)
	: Node(rType)
{ }

void
SimpleType::ParseChildren(BaseProcessor& rProcessor) const noexcept(false) {
	/* process children */
	_eachChild([&rProcessor](const Node& rNode) {
		if (XSD_ISELEMENT(&rNode, Restriction) ||
			XSD_ISELEMENT(&rNode, List) ||
			XSD_ISELEMENT(&rNode, Annotation) ||
			XSD_ISELEMENT(&rNode, Union)) {
			rNode.ParseElement(rProcessor);
		} else
			throw XMLException(rNode.GetXMLElm(), XMLException::InvallidChildXMLElement);
	});
}

void
SimpleType::ParseElement(BaseProcessor& rProcessor) const noexcept(false) {
	if (HasName() && !Node::IsRootNode())
		throw XMLException(Node::GetXMLElm(), XMLException::InvalidAttribute);
	rProcessor.ProcessSimpleType(this);
}

Types::BaseType * 
SimpleType::GetParentType(void) const noexcept(false) {
	std::unique_ptr<Restriction> pRestriction(Node::SearchXSDChildElm<Restriction>());
	std::unique_ptr<List> pList(Node::SearchXSDChildElm<List>());
	std::unique_ptr<Union> pUnion(Node::SearchXSDChildElm<Union>());
	if ((NULL != pRestriction.get()) && (NULL == pList.get()) && (NULL == pUnion.get())) {
		return pRestriction->GetParentType();
	} else if ((NULL == pRestriction.get()) && (NULL != pList.get()) && (NULL == pUnion.get())) {
		return pList->GetParentType();
	} else if ((NULL == pRestriction.get()) && (NULL != pList.get()) && (NULL == pUnion.get())) {
		return pList->GetParentType();
	} else {
		/* simple type can't have multiple child modifiers */
		throw XMLException(Node::GetXMLElm(), XMLException::InvallidChildXMLElement);
	}
}

std::string
SimpleType::Name() const noexcept(false) {
	return std::string(this->GetAttribute<const char*>("name"));
}

bool
SimpleType::HasName() const {
	return Node::HasAttribute("name");
}

bool
SimpleType::HasRestriction() const {
	return Node::HasContent(Restriction::XSDTag());
}

bool
SimpleType::HasUnion() const {
	return Node::HasContent(Union::XSDTag());
}

bool
SimpleType::HasList() const {
	return Node::HasContent(List::XSDTag());
}
