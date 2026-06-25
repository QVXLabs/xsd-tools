/*
 * SimpleTypeExtracter.cpp
 *
 *  Created on: 01/23/12
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

#include "./src/XSDParser/Elements/Union.hpp"
#include "./src/XSDParser/Elements/Restriction.hpp"
#include "./src/XSDParser/Elements/List.hpp"
#include "./src/XSDParser/Elements/Include.hpp"
#include "./src/XSDParser/Elements/SimpleType.hpp"
#include "./src/XSDParser/Elements/ComplexType.hpp"
#include "./src/Processors/SimpleTypeExtracter.hpp"
#include "./src/Processors/ArrayType.hpp"

using namespace std;
using namespace Processors;

SimpleTypeExtracter::SimpleTypeExtracter()
	: LuaProcessorBase(NULL), pType_(NULL)
{ }

SimpleTypeExtracter::SimpleTypeExtracter(const SimpleTypeExtracter& rProcessor)
 	: LuaProcessorBase(rProcessor), pType_(rProcessor.pType_->clone())
 {}
 
/* virtual */
SimpleTypeExtracter::~SimpleTypeExtracter() {
	delete pType_;
}

const XSD::Types::BaseType&
SimpleTypeExtracter::Extract(const XSD::Types::BaseType& rXSDType) {
	parseType_(rXSDType);
	return *pType_;
}

/* virtual */ void
SimpleTypeExtracter::ProcessUnion(const XSD::Elements::Union* pNode) {
	pType_ = new XSD::Types::String();
}

/* virtual */ void
SimpleTypeExtracter::ProcessRestriction(const XSD::Elements::Restriction* pNode ) {
	if (pNode->isParentSimpleContent() || pNode->isParentSimpleContent())
		pNode->ParseChildren(*this);
	else {
		unique_ptr<XSD::Types::BaseType> pType(pNode->Base());
		parseType_(*pType);
	}
}

/* virtual */ void
SimpleTypeExtracter::ProcessList(const XSD::Elements::List* pNode) {
	unique_ptr<XSD::Types::BaseType> pTypeLst(pNode->ItemType());
	/* extract xsd native type from simple type */
	SimpleTypeExtracter typeXtr;
	parseType_(Types::ArrayType(typeXtr.Extract(*pTypeLst)));
}

/* virtual */ void
SimpleTypeExtracter::ProcessInclude(const XSD::Elements::Include* pNode) {
	const XSD::Elements::Schema * pSchema = pNode->QuerySchema();
	pSchema->ParseChildren(*this);
}

void 
SimpleTypeExtracter::parseType_(const XSD::Types::BaseType& rXSDType) {
	if(XSD_ISTYPE(&rXSDType, XSD::Types::SimpleType)) {
		const XSD::Types::SimpleType* pSimpleType = static_cast<const XSD::Types::SimpleType*>(&rXSDType);
		pSimpleType->pValue_->ParseElement(*this);
	} else if(XSD_ISTYPE(&rXSDType, XSD::Types::ComplexType)) {
		const XSD::Types::ComplexType* pComplexType = static_cast<const XSD::Types::ComplexType*>(&rXSDType);
		pComplexType->pValue_->ParseElement(*this);
	} else {
		pType_ = rXSDType.clone();
	}
}
