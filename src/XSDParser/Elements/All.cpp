/*
 * All.cpp
 *
 *  Created on: Aug 31, 2011
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
#include "./src/XSDParser/Elements/All.hpp"
#include "./src/XSDParser/Elements/Annotation.hpp"
#include "./src/XSDParser/Elements/Element.hpp"

using namespace XSD;
using namespace XSD::Elements;

All::All(const TiXmlElement& elm, const Parser& rParser)
	: Node(elm, rParser)
{ }

All::All(const All& cpy)
	: Node(cpy)
{ }

void
All::ParseChildren(BaseProcessor& rProcessor) const noexcept(false) {
	std::unique_ptr<Node> pNode(Node::FirstChild());
	if (NULL != pNode.get()) {
		if (XSD_ISELEMENT(pNode.get(), Element) ||
			XSD_ISELEMENT(pNode.get(), Annotation))
			pNode->ParseElement(rProcessor);
		else
			throw XMLException(pNode->GetXMLElm(), XMLException::InvallidChildXMLElement);
	}
}

void
All::ParseElement(BaseProcessor& rProcessor) const noexcept(false) {
	/* verify that optional maxOccurs attribute is 1 */
	if (HasMaxOccurs()) {
		if (1 != MaxOccurs())
			throw XMLException(Node::GetXMLElm(), XMLException::InvalidAttributeValue);
	}
	/* verify that optional minOccurs attribute is 0 or 1 */
	if (HasMinOccurs()) {
		if (0 > MinOccurs() || 1 < MinOccurs())
			throw XMLException(Node::GetXMLElm(), XMLException::InvalidAttributeValue);
	}
	/* process element */
	rProcessor.ProcessAll(this);
}

Types::BaseType * 
All::GetParentType() const noexcept(false) {
	std::unique_ptr<Node> pParent(Node::Parent());
	return pParent->GetParentType();
}

int
All::MaxOccurs() const {
	if (HasMaxOccurs())
		return Node::GetAttribute<int>("maxOccurs");
	else
		return 1;
}

int
All::MinOccurs() const {
	if (HasMinOccurs())
		return Node::GetAttribute<int>("minOccurs");
	else
		return 1;
}

bool
All::HasMaxOccurs() const {
	return Node::HasAttribute("maxOccurs");
}

bool
All::HasMinOccurs() const {
	return Node::HasAttribute("minOccurs");
}
