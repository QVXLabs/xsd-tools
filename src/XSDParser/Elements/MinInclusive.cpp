/*
 * MinInclusive.cpp
 *
 *  Created on: Jul 29, 2011
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
#include <string.h>
#include <string>
#include <tinyxml.h>
#include "./src/XSDParser/Elements/Annotation.hpp"
#include "./src/XSDParser/Elements/MinInclusive.hpp"

using namespace XSD;
using namespace XSD::Elements;

MinInclusive::MinInclusive(const TiXmlElement& elm, const Parser& rParser)
	: Node(elm, rParser)
{ }

MinInclusive::MinInclusive(const MinInclusive& cpy)
	: Node(cpy)
{ }

void
MinInclusive::ParseChildren(BaseProcessor& rProcessor) const noexcept(false) {
	/* no children allowed */
	std::unique_ptr<Node> pNode(Node::FirstChild());
	if (NULL != pNode.get()) {
		if (XSD_ISELEMENT(pNode.get(), Annotation))
			pNode->ParseElement(rProcessor);
		else
			throw XMLException(pNode->GetXMLElm(), XMLException::InvallidChildXMLElement);
	}
}

void
MinInclusive::ParseElement(BaseProcessor& rProcessor) const noexcept(false) {
	rProcessor.ProcessMinInclusive(this);
}

Types::BaseType * 
MinInclusive::GetParentType() const noexcept(false) {
	std::unique_ptr<Node> pParent(Node::Parent());
	return pParent->GetParentType();
}

long double
MinInclusive::Value() const noexcept(false) {
	return Node::GetAttribute<long double>("value");
}

bool
MinInclusive::HasValue() const {
	return Node::HasAttribute("value");
}
