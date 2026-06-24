/*
 * AttributeGroup.cpp
 *
 *  Created on: Aug 4, 2011
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

#include <memory>
#include <string.h>
#include <string>
#include <tinyxml.h>
#include "./src/XSDParser/Elements/AttributeGroup.hpp"
#include "./src/XSDParser/Elements/Attribute.hpp"
#include "./src/XSDParser/Elements/Annotation.hpp"

using namespace XSD;
using namespace XSD::Elements;

AttributeGroup::AttributeGroup(const TiXmlElement& elm, const Parser& rParser)
	: Node(elm, rParser)
{ }

AttributeGroup::AttributeGroup(const AttributeGroup& cpy)
	: Node(cpy)
{ }

void
AttributeGroup::ParseChildren(BaseProcessor& rProcessor) const noexcept(false) {
	/* process children */
	_eachChild([&rProcessor](const Node& rNode) {
		if (XSD_ISELEMENT(&rNode, Attribute) ||
			XSD_ISELEMENT(&rNode, Annotation)) {
			rNode.ParseElement(rProcessor);
		} else
			throw XMLException(rNode.GetXMLElm(), XMLException::InvallidChildXMLElement);
	});
}

void
AttributeGroup::ParseElement(BaseProcessor& rProcessor) const noexcept(false) {
	/* a ref and a name are not allowed */
	if (HasName() && HasRef())
		throw XMLException(Node::GetXMLElm(), XMLException::InvalidAttribute);
	/* a name is only allowed when it's parent element is a schema */
	if (HasName()) {
		if (Node::GetXMLElm().Parent()->Type() != TiXmlNode::TINYXML_DOCUMENT)
			throw XMLException(Node::GetXMLElm(), XMLException::InvalidAttribute);
	}
	if (HasRef()) {
		std::unique_ptr<AttributeGroup> pRefGroup(RefGroup());
		/* a name is only allowed when it's parent element is a schema */
		if (pRefGroup->GetXMLElm().Parent()->Type() != TiXmlNode::TINYXML_DOCUMENT)
			throw XMLException(pRefGroup->GetXMLElm(), XMLException::InvalidAttribute);
	}
	rProcessor.ProcessAttributeGroup(this);
}

Types::BaseType * 
AttributeGroup::GetParentType() const noexcept(false) {
	std::unique_ptr<Node> pParent(Node::Parent());
	return pParent->GetParentType();
}

std::string
AttributeGroup::Name() const noexcept(false) {
	return std::string(Node::GetAttribute<const char*>("name"));
}

AttributeGroup*
AttributeGroup::RefGroup() const noexcept(false) {
	return Node::FindXSDRef<AttributeGroup>("ref");
}

bool
AttributeGroup::HasName() const {
	return Node::HasAttribute("name");
}

bool
AttributeGroup::HasRef() const {
	return Node::HasAttribute("ref");
}
