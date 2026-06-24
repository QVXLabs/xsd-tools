/*
 * List.cpp
 *
 *  Created on: Jun 26, 2011
 *      Author: Ardavon Falls
 *   Copyright: (c)2011 Ardavon Falls
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

#ifndef TIXML_USE_STL
#	define TIXML_USE_STL
#endif /* TIXML_USE_STL */
#include <memory>
#include <tinyxml.h>
#include "./src/XSDParser/Elements/List.hpp"
#include "./src/XSDParser/Elements/SimpleType.hpp"
#include "./src/XSDParser/Elements/Annotation.hpp"

using namespace XSD;
using namespace XSD::Elements;

List::List(const TiXmlElement& elm, const Parser& rParser)
	: Node(elm, rParser)
{}

List::List(const List& lst)
	: Node(lst)
{ }

void
List::ParseChildren(BaseProcessor& rProcessor) const noexcept(false) {
	/* process children */
	_eachChild([&rProcessor](const Node& rNode) {
		if (XSD_ISELEMENT(&rNode, SimpleType) ||
			XSD_ISELEMENT(&rNode, Annotation)) {
			rNode.ParseElement(rProcessor);
		} else
			throw XMLException(rNode.GetXMLElm(), XMLException::InvallidChildXMLElement);
	});
}

void
List::ParseElement(BaseProcessor& rProcessor) const noexcept(false) {
	/* verify that is has an item type defined or it has a sub type defined */
	if (HasItemType()) {
		if (Node::HasContent(SimpleType::XSDTag()))
			throw XMLException(Node::GetXMLElm(), XMLException::InvallidChildXMLElement);
	} else {
		if (!Node::HasContent(SimpleType::XSDTag()))
			throw XMLException(Node::GetXMLElm(), XMLException::MissingChildXMLElement);
	}
	rProcessor.ProcessList(this);
}

Types::BaseType * 
List::GetParentType(void) const noexcept(false) {
	return this->ItemType();
}

bool
List::HasItemType() const {
	return Node::HasAttribute("itemType");
}

Types::BaseType*
List::ItemType() const noexcept(false) {
	if (HasContent(SimpleType::XSDTag()))
		return new Types::SimpleType(FindXSDChildElm<SimpleType>());
	else
		return _type();
};

Types::BaseType*
List::_type() const noexcept(false) {
	Types::BaseType* pType = Node::GetAttribute<Types::BaseType*>("itemType");
	if (XSD_ISTYPE(pType, Types::Unknown)) {
		delete pType;
		return new Types::String();
	} else if (XSD_ISTYPE(pType, Types::ComplexType) ||
			   XSD_ISTYPE(pType, Types::Unsupported))
		throw  XMLException(Node::GetXMLElm(), XMLException::UndefiniedXSDType);
	return pType;
}
