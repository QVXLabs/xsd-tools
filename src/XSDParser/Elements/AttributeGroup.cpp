/*
 * AttributeGroup.cpp
 *
 *  Created on: Aug 4, 2011
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
	eachChild_([&rProcessor](const Node& rNode) {
		if (XSD_ISELEMENT(&rNode, Attribute) ||
			XSD_ISELEMENT(&rNode, Annotation)) {
			rNode.ParseElement(rProcessor);
		} else
			throw XMLException(rNode.GetXMLElm(), XMLException::InvallidChildXMLElement);
	});
}

/* top-level == child of the <schema> root element, i.e. grandparent is the
   document. (Its parent is the schema element, not the document — a parent==
   DOCUMENT check wrongly rejected every top-level attributeGroup.) */
static bool isTopLevel_(const TiXmlElement& rElm) {
	const TiXmlNode* pParent = rElm.Parent();
	return NULL != pParent && NULL != pParent->Parent() &&
	       TiXmlNode::TINYXML_DOCUMENT == pParent->Parent()->Type();
}

void
AttributeGroup::ParseElement(BaseProcessor& rProcessor) const noexcept(false) {
	/* a ref and a name are not allowed */
	if (HasName() && HasRef())
		throw XMLException(Node::GetXMLElm(), XMLException::InvalidAttribute);
	/* a named attributeGroup definition must be top-level */
	if (HasName() && !isTopLevel_(Node::GetXMLElm()))
		throw XMLException(Node::GetXMLElm(), XMLException::InvalidAttribute);
	if (HasRef()) {
		std::unique_ptr<AttributeGroup> pRefGroup(RefGroup());
		/* the referenced attributeGroup must likewise be top-level */
		if (!isTopLevel_(pRefGroup->GetXMLElm()))
			throw XMLException(pRefGroup->GetXMLElm(), XMLException::InvalidAttribute);
	}
	rProcessor.ProcessAttributeGroup(this);
}

std::string
AttributeGroup::Name() const noexcept(false) {
	return Node::name_();
}

AttributeGroup*
AttributeGroup::RefGroup() const noexcept(false) {
	return Node::FindXSDRef<AttributeGroup>("ref");
}
