/*
 * Group.cpp
 *
 *  Created on: Jul 8, 2011
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
#include "./src/XSDParser/Elements/Group.hpp"
#include "./src/XSDParser/Elements/Choice.hpp"
#include "./src/XSDParser/Elements/Sequence.hpp"
#include "./src/XSDParser/Elements/Annotation.hpp"
#include "./src/XSDParser/Elements/All.hpp"

using namespace XSD;
using namespace XSD::Elements;

Group::Group(const TiXmlElement& elm, const Parser& rParser)
	: Node(elm, rParser)
{ }

Group::Group(const Group& cpy)
	: Node(cpy)
{ }

void
Group::ParseChildren(BaseProcessor& rProcessor) const noexcept(false) {
	/* process children */
	eachChild_([&rProcessor](const Node& rNode) {
		if (XSD_ISELEMENT(&rNode, Choice) ||
			XSD_ISELEMENT(&rNode, Annotation) ||
			XSD_ISELEMENT(&rNode, All) ||
			XSD_ISELEMENT(&rNode, Sequence)) {
			rNode.ParseElement(rProcessor);
		} else
			throw XMLException(rNode.GetXMLElm(), XMLException::InvallidChildXMLElement);
	});
}

void
Group::ParseElement(BaseProcessor& rProcessor) const noexcept(false) {
	/* a ref and a name are not allowed */
	if (HasName() && HasRef())
		throw XMLException(Node::GetXMLElm(), XMLException::InvalidAttribute);
	/* a name is only allowed when it's parent element is a schema */
	if (HasName()) {
		if (Node::GetXMLElm().Parent()->Type() != TiXmlNode::TINYXML_DOCUMENT)
			throw XMLException(Node::GetXMLElm(), XMLException::InvalidAttribute);
	}
	if (HasRef()) {
		std::unique_ptr<Group> pRefGroup(RefGroup());
		/* a name is only allowed when it's parent element is a schema */
		if (pRefGroup->GetXMLElm().Parent()->Type() != TiXmlNode::TINYXML_DOCUMENT)
			throw XMLException(pRefGroup->GetXMLElm(), XMLException::InvalidAttribute);
	}
	/* verify that 'maxOccurs' is not negative unless its -1 (unbounded) */
	if (-1 > MaxOccurs())
		throw XMLException(Node::GetXMLElm(), XMLException::InvalidAttributeValue);
	/* verify that 'minOccurs' is non-negative */
	if (0 > MinOccurs())
		throw XMLException(Node::GetXMLElm(), XMLException::InvalidAttributeValue);
	/* process element */
	rProcessor.ProcessGroup(this);
}

int
Group::MaxOccurs() const {
	return Node::maxOccurs_(1);
}

int
Group::MinOccurs() const {
	return Node::minOccurs_(1);
}

std::string
Group::Name() const noexcept(false) {
	return Node::name_();
}

Group*
Group::RefGroup() const noexcept(false) {
	return Node::FindXSDRef<Group>("ref");
}

bool
Group::HasMaxOccurs() const {
	return Node::HasAttribute("maxOccurs");
}

bool
Group::HasMinOccurs() const {
	return Node::HasAttribute("minOccurs");
}

bool
Group::HasName() const {
	return Node::HasAttribute("name");
}

bool
Group::HasRef() const {
	return Node::HasAttribute("ref");
}
