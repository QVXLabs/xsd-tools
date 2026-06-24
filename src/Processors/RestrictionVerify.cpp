/*
 * ElementExtracter.cpp
 *
 *  Created on: 3/4/14
 *      Author: QVXLabs LLC
 *   Copyright: (c)2012 QVXLabs LLC
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

#include "./src/Processors/RestrictionVerify.hpp"
#include "./src/XSDParser/Elements/Element.hpp"
#include "./src/XSDParser/Elements/Schema.hpp"
#include "./src/XSDParser/Elements/Annotation.hpp"
#include "./src/XSDParser/Elements/SimpleType.hpp"
#include "./src/XSDParser/Elements/ComplexType.hpp"
#include "./src/XSDParser/Elements/Group.hpp"
#include "./src/XSDParser/Elements/Include.hpp"
#include "./src/XSDParser/Elements/All.hpp"
#include "./src/Processors/ElementExtracter.hpp"

using namespace std;
using namespace Processors;
using namespace XSD::Elements;

RestrictionVerify::RestrictionVerify()
	: LuaProcessorBase(NULL), m_pSubTree(NULL)
{ }

RestrictionVerify::RestrictionVerify(const RestrictionVerify& rProcessor)
 	: LuaProcessorBase(rProcessor), m_pSubTree(rProcessor.m_pSubTree)
 { }
 
 RestrictionVerify::RestrictionVerify(const Node* pSubTree)
	: LuaProcessorBase(NULL), m_pSubTree(pSubTree)
{ }
 
/* virtual */
RestrictionVerify::~RestrictionVerify() {
}

bool
RestrictionVerify::Verify(const Restriction* pRestriciton, const ComplexType* pParentType) noexcept(false) {
	RestrictionVerify processor(pParentType);
	try {
		processor.ProcessRestriction(pRestriciton);
	} catch (XSD::XMLException& e) {
		if (XSD::XMLException::RestrictionTypeMismatch == e.QueryError().m_errorId) {
			return false;
		} else {
			throw;	/* rethrow exception */
		}
	}
	return true;
}

/* virtual */ void 
RestrictionVerify::ProcessElement(const Element* pNode) {
	/* find the element in the parent sub-tree */
	bool bFound = m_pSubTree->_findChild([&](const Node& rSearchNode) -> bool {
		if (XSD_ISELEMENT(&rSearchNode, Element)) {
			const Element * pElement = static_cast<const Element *>(&rSearchNode);
			if (pNode->HasRef() && pElement->HasRef()) {
				std::unique_ptr<Element> pRefElm(pNode->RefElement());
				std::unique_ptr<Element> pSubTreeRefElm(pElement->RefElement());
				/* verify the elements refer to the the same element */
				if (*(pRefElm.get()) == *(pSubTreeRefElm.get())) {
					return true;
				}
			} else if (	pNode->HasName() && pElement->HasName() &&
						(0 == pNode->Name().compare(pElement->Name()))) {
				/* verify the elements have the same or related types */
				std::unique_ptr<XSD::Types::BaseType> pElmType(pNode->Type());
				std::unique_ptr<XSD::Types::BaseType> pSubTreeElmType(pElement->Type());
				if (XSD_ISTYPE(pElmType.get(), XSD::Types::SimpleType) ||
					XSD_ISTYPE(pElmType.get(), XSD::Types::ComplexType)) {
					if (pElmType->isTypeRelated(pSubTreeElmType.get())) {
					  return true;
					}
				} else if (typeid(*(pElmType.get())) == typeid(*(pSubTreeElmType.get()))) {
					return true;
				}
			}
		} else if (XSD_ISELEMENT(&rSearchNode, Group)) {
			const Group * pGroup = static_cast<const Group *>(&rSearchNode);
			/* if group is a ref, then enter that group, dive to its child and compare agaist this nodes
			 * parent (sequence/choice/all). */
			std::unique_ptr<Node> pParent(pNode->Parent());
			if (pGroup->HasRef()) {
				/* update sub-tree to point to the referenced group sub-tree */
				std::unique_ptr<Group> pRefGroup(pGroup->RefGroup());
				RestrictionVerify processor(pRefGroup.get());
				pParent->ParseElement(processor);
				return true;
			} else {
				RestrictionVerify processor(pGroup->FirstChild());
				pParent->ParseElement(processor);
				return true;
			}
		}
		return false;
	});
	if (!bFound)
		throw XSD::XMLException(pNode->GetXMLElm(), XSD::XMLException::RestrictionTypeMismatch);
}

/* virtual */ void 
RestrictionVerify::ProcessRestriction(const XSD::Elements::Restriction* pNode) {
	RestrictionVerify processor(m_pSubTree);
	pNode->ParseChildren(processor);
}

/* virtual */ void 
RestrictionVerify::ProcessSequence(const XSD::Elements::Sequence* pNode) {
	/* valid restrictions are xs:sequence to xs:sequence or xs:choice to xs:sequence */
	bool bFound = m_pSubTree->_findChild([&](const Node& rSearchNode) -> bool {
		if (XSD_ISELEMENT(&rSearchNode, Sequence) ||
			XSD_ISELEMENT(&rSearchNode, Choice)) {
			RestrictionVerify processor(&rSearchNode);
			pNode->ParseChildren(processor);
			return true;
		}
		return false;
	});
	if (!bFound)
		throw XSD::XMLException(pNode->GetXMLElm(), XSD::XMLException::RestrictionTypeMismatch);
}

/* virtual */ void 
RestrictionVerify::ProcessChoice(const XSD::Elements::Choice* pNode) {
	/* search for either a xs:choice and verify that it is in the sub-tree */
	bool bFound = m_pSubTree->_findChild([&](const Node& rSearchNode) -> bool {
		if (XSD_ISELEMENT(&rSearchNode, Choice)) {
			RestrictionVerify processor(&rSearchNode);
			pNode->ParseChildren(processor);
			return true;
		}
		return false;
	});
	if (!bFound)
		throw XSD::XMLException(pNode->GetXMLElm(), XSD::XMLException::RestrictionTypeMismatch);
}

/* virtual */ void 
RestrictionVerify::ProcessGroup(const XSD::Elements::Group* pNode) {
	/* search for either a xs:group and verify that it is in the sub-tree */
	bool bFound = m_pSubTree->_findChild([&](const Node& rSearchNode) -> bool {
		if (XSD_ISELEMENT(&rSearchNode, Group)) {
			const Group * pGroup = static_cast<const Group *>(&rSearchNode);
			if (pNode->HasRef() && pGroup->HasName()) {
				std::unique_ptr<Group> pRefGroup(pNode->RefGroup());
				if (*(pRefGroup.get()) == *pGroup) {
					return true;
				}
			}
		}
		return false;
	});
	if (!bFound)
		throw XSD::XMLException(pNode->GetXMLElm(), XSD::XMLException::RestrictionTypeMismatch);
}

/* virtual */ void 
RestrictionVerify::ProcessAll(const XSD::Elements::All* pNode) {
	/* valid restrictions are xs:all to xs:all or xs:all to xs:sequence  */
	bool bFound = m_pSubTree->_findChild([&](const Node& rSearchNode) -> bool {
		if (XSD_ISELEMENT(&rSearchNode, All) ||
			XSD_ISELEMENT(&rSearchNode, Sequence)) {
			RestrictionVerify processor(&rSearchNode);
			pNode->ParseChildren(processor);
			return true;
		}
		return false;
	});
	if (!bFound)
		throw XSD::XMLException(pNode->GetXMLElm(), XSD::XMLException::RestrictionTypeMismatch);
}


