/*
 * Annotation.cpp
 *
 *  Created on: Aug 27, 2011
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
 *  along with Xsd-Tools.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TIXML_USE_STL
#	define TIXML_USE_STL
#endif /* TIXML_USE_STL */
#include <memory>
#include <string.h>
#include <string>
#include <tinyxml.h>
#include "./src/XSDParser/Elements/Annotation.hpp"
#include "./src/XSDParser/Elements/Documentation.hpp"
#include "./src/XSDParser/Elements/AppInfo.hpp"

using namespace XSD;
using namespace XSD::Elements;

Annotation::Annotation(const TiXmlElement& elm, const Parser& rParser)
	: Node(elm, rParser)
{ }

Annotation::Annotation(const Annotation& cpy)
	: Node(cpy)
{ }

void
Annotation::ParseChildren(BaseProcessor& rProcessor) const noexcept(false) {
	std::unique_ptr<Node> pNode(Node::FirstChild());
	if (NULL != pNode.get()) {
		if (XSD_ISELEMENT(pNode.get(), Documentation) ||
			XSD_ISELEMENT(pNode.get(), AppInfo)) {
			pNode->ParseElement(rProcessor);
		} else
			throw XMLException(pNode->GetXMLElm(), XMLException::InvallidChildXMLElement);
	}
}

void
Annotation::ParseElement(BaseProcessor& rProcessor) const noexcept(false) {
	rProcessor.ProcessAnnotation(this);
}

Types::BaseType * 
Annotation::GetParentType() const noexcept(false) {
	std::unique_ptr<Node> pParent(Node::Parent());
	return pParent->GetParentType();
}
