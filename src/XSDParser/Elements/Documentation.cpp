/*
 * Documentation.cpp
 *
 *  Created on: Aug 27, 2011
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

#ifndef TIXML_USE_STL
#	define TIXML_USE_STL
#endif /* TIXML_USE_STL */
#include <memory>
#include <string.h>
#include <string>
#include <tinyxml.h>
#include "./src/XSDParser/Elements/Documentation.hpp"

using namespace XSD;
using namespace XSD::Elements;

Documentation::Documentation(const TiXmlElement& elm, const Parser& rParser)
	: Node(elm, rParser)
{ }

Documentation::Documentation(const Documentation& cpy)
	: Node(cpy)
{ }

void
Documentation::ParseChildren(BaseProcessor& rProcessor) const noexcept(false) {
	/* no children allowed */
	std::unique_ptr<Node> pNode(Node::FirstChild());
	if (NULL != pNode.get()) {
		throw XMLException(pNode->GetXMLElm(), XMLException::InvallidChildXMLElement);
	}
}

void
Documentation::ParseElement(BaseProcessor& rProcessor) const noexcept(false) {
	rProcessor.ProcessDocumentation(this);
}

Types::BaseType * 
Documentation::GetParentType() const noexcept(false) {
	std::unique_ptr<Node> pParent(Node::Parent());
	return pParent->GetParentType();
}

std::string
Documentation::DocumentationStr() const noexcept(false) {
	if (Node::HasContent()) {
		std::string retTxt;
		const TiXmlNode* pXmlNode = Node::GetXMLElm().FirstChild();
		for ( ; NULL != pXmlNode; pXmlNode = pXmlNode->NextSibling()) {
			if (TiXmlNode::TINYXML_TEXT == pXmlNode->Type())
				retTxt += pXmlNode->Value();
			else
				throw XMLException(GetXMLElm(), XMLException::InvallidChildXMLElement);
		}
		return retTxt;
	} else {
		return std::string("");
	}
}
