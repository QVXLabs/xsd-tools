/*
 * SimpleContent.cpp
 *
 *  Created on: Jul 22, 2011
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
#include <string>
#include <tinyxml.h>
#include "./src/XSDParser/Elements/SimpleContent.hpp"
#include "./src/XSDParser/Elements/Extension.hpp"
#include "./src/XSDParser/Elements/Restriction.hpp"
#include "./src/XSDParser/Elements/Annotation.hpp"

using namespace XSD;
using namespace XSD::Elements;

SimpleContent::SimpleContent(const TiXmlElement& elm, const Parser& rParser)
	: Node(elm, rParser)
{ }

SimpleContent::SimpleContent(const SimpleContent& rType)
	: Node(rType)
{ }

void
SimpleContent::ParseChildren(BaseProcessor& rProcessor) const noexcept(false) {
	/* process children */
	eachChild_([&rProcessor](const Node& rNode) {
		if (XSD_ISELEMENT(&rNode, Restriction) ||
			XSD_ISELEMENT(&rNode, Annotation) ||
			XSD_ISELEMENT(&rNode, Extension)) {
			rNode.ParseElement(rProcessor);
		} else
			throw XMLException(rNode.GetXMLElm(), XMLException::InvallidChildXMLElement);
	});
}

void
SimpleContent::ParseElement(BaseProcessor& rProcessor) const noexcept(false) {
	rProcessor.ProcessSimpleContent(this);
}

Types::BaseType *
SimpleContent::GetParentType() const noexcept(false) {
	return Node::delegateToSoleChild_();
}
