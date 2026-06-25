/*
 * Extension.cpp
 *
 *  Created on: Jul 18, 2011
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
#include <tinyxml.h>

#include "./src/XSDParser/Elements/Extension.hpp"
#include "./src/XSDParser/Elements/Group.hpp"
#include "./src/XSDParser/Elements/Choice.hpp"
#include "./src/XSDParser/Elements/Sequence.hpp"
#include "./src/XSDParser/Elements/Attribute.hpp"
#include "./src/XSDParser/Elements/SimpleType.hpp"
#include "./src/XSDParser/Elements/ComplexType.hpp"
#include "./src/XSDParser/Elements/ComplexContent.hpp"
#include "./src/XSDParser/Elements/Annotation.hpp"
#include "./src/XSDParser/Elements/All.hpp"

using namespace XSD;
using namespace XSD::Elements;

Extension::Extension(const TiXmlElement& elm, const Parser& rParser)
	: Node(elm, rParser)
{ }

Extension::Extension(const Extension& rCpy)
	: Node(rCpy)
{ }

void
Extension::ParseChildren(BaseProcessor& rProcessor) const noexcept(false) {
	if (isParentComplex()) {
		/* process children */
		eachChild_([&rProcessor](const Node& rNode) {
			if (XSD_ISELEMENT(&rNode, Group) ||
				XSD_ISELEMENT(&rNode, Choice) ||
				XSD_ISELEMENT(&rNode, Sequence) ||
				XSD_ISELEMENT(&rNode, Annotation) ||
				XSD_ISELEMENT(&rNode, All) ||
				XSD_ISELEMENT(&rNode, Attribute)) {
				rNode.ParseElement(rProcessor);
			} else
				throw XMLException(rNode.GetXMLElm(), XMLException::InvallidChildXMLElement);
		});
	} else {
		/* process children */
		eachChild_([&rProcessor](const Node& rNode) {
			if (XSD_ISELEMENT(&rNode, Attribute) ||
				XSD_ISELEMENT(&rNode, Annotation)) {
				rNode.ParseElement(rProcessor);
			} else
				throw XMLException(rNode.GetXMLElm(), XMLException::InvallidChildXMLElement);
		});
	}
}

void
Extension::ParseElement(BaseProcessor& rProcessor) const noexcept(false) {
	/* make sure the base element is there */
	if (!Node::HasAttribute("base"))
		throw XMLException(GetXMLElm(), XMLException::MissingAttribute);
	/* make sure that no child particles have match base child particles */
	std::unique_ptr<Types::BaseType> pBase(Base());
	if (XSD_ISTYPE(pBase.get(),Types::SimpleType)) {
		const Types::SimpleType* pSmplType = static_cast<const Types::SimpleType*>(pBase.get());
		if (checkForDuplicateNamedParticles_(&Node::GetXMLElm(), &pSmplType->pValue_->GetXMLElm())) {
			throw XMLException(GetXMLElm(), XMLException::InvallidChildXMLElement);
		}
	} else if (XSD_ISTYPE(pBase.get(),Types::ComplexType)) {
		const Types::ComplexType* pCmplxType = static_cast<const Types::ComplexType*>(pBase.get());
		if (checkForDuplicateNamedParticles_(&Node::GetXMLElm(), &pCmplxType->pValue_->GetXMLElm())) {
			throw XMLException(GetXMLElm(), XMLException::InvallidChildXMLElement);
		}
	}
	rProcessor.ProcessExtension(this);
}

Types::BaseType * 
Extension::GetParentType(void) const noexcept(false) {
	return this->Base();
}

Types::BaseType*
Extension::Base() const noexcept(false) {
	return Node::baseType_();
}

bool
Extension::isParentComplex() const noexcept(false) {
	std::unique_ptr<Node> pParent(Node::Parent());
	return XSD_ISELEMENT(pParent.get(), XSD::Elements::ComplexContent);
}

/* static */ bool
Extension::checkForDuplicateNamedParticles_(const TiXmlElement* pTreeBase, const TiXmlElement* pBase) {
	const TiXmlElement* pChld = pTreeBase->FirstChildElement();
	for (; pChld ; pChld = pChld->NextSiblingElement()) {
		/* check to see if element has a name attribute */
		if (NULL != pChld->Attribute("name")) {
			return find_(pChld->Attribute("name"), pBase);
		}
		/* search children */
		if (true ==checkForDuplicateNamedParticles_(pChld, pBase))
			return true;
	}
	return false;
}

/* static */ bool
Extension::find_(const char* pName, const TiXmlElement* pBase) {
	const TiXmlElement* pChld = pBase->FirstChildElement();
	for (; pChld ; pChld = pChld->NextSiblingElement()) {
		/* check to see if element has a name attribute */
		if (NULL != pChld->Attribute("name")) {
			if (0 == strcmp(pChld->Attribute("name"), pName)) {
				return true;
			}
		}
		/* search children */
		if (true == find_(pName, pChld))
			return true;
	}
	return false;
}
