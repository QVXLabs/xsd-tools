/*
 * ComplexContent.cpp
 *
 *  Created on: Jul 12, 2011
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
#include <string>
#include <tinyxml.h>
#include "./src/XSDParser/Elements/ComplexContent.hpp"
#include "./src/XSDParser/Exception.hpp"
#include "./src/XSDParser/Elements/Node.hpp"
#include "./src/XSDParser/Elements/Restriction.hpp"
#include "./src/XSDParser/Elements/Schema.hpp"
#include "./src/XSDParser/Elements/Extension.hpp"
#include "./src/XSDParser/Elements/Annotation.hpp"

using namespace XSD;
using namespace XSD::Elements;

ComplexContent::ComplexContent(const TiXmlElement& elm, const Parser& rParser)
	: Node(elm, rParser)
{ }

ComplexContent::ComplexContent(const ComplexContent& rType)
	: Node(rType)
{ }

void
ComplexContent::ParseChildren(BaseProcessor& rProcessor) const noexcept(false) {
	/* process children */
	_eachChild([&rProcessor](const Node& rNode) {
		if (XSD_ISELEMENT(&rNode, Restriction) ||
			XSD_ISELEMENT(&rNode, Annotation) ||
			XSD_ISELEMENT(&rNode, Extension)) {
			rNode.ParseElement(rProcessor);
		} else
			throw XMLException(rNode.GetXMLElm(), XMLException::InvallidChildXMLElement);
	});
}

void
ComplexContent::ParseElement(BaseProcessor& rProcessor) const noexcept(false) {
	rProcessor.ProcessComplexContent(this);
}

Types::BaseType * 
ComplexContent::GetParentType() const noexcept(false) {
	std::unique_ptr<Restriction> pRestriction(Node::SearchXSDChildElm<Restriction>());
	std::unique_ptr<Extension> pExtension(Node::SearchXSDChildElm<Extension>());
	if ((NULL != pRestriction.get()) && (NULL == pExtension.get())) {
		return pRestriction->GetParentType();
	} else if ((NULL == pRestriction.get()) && (NULL != pExtension.get())) {
		return pExtension->GetParentType();
	} else /* complex content can't have multiple child modifiers */
		throw XMLException(Node::GetXMLElm(), XMLException::InvallidChildXMLElement);
}

bool
ComplexContent::Mixed() const {
	if (Node::HasAttribute("mixed")) {
		return Node::GetAttribute<bool>("mixed");
	}
	return false;
}

