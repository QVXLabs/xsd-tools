/*
 * LuaProcessor.cpp
 *
 *  Created on: Jun 29, 2011
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
 *  along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sstream>
#include <memory>
#include <assert.h>
#include "./src/XSDParser/Parser.hpp"
#include "./src/XSDParser/Elements/Schema.hpp"
#include "./src/XSDParser/Elements/Element.hpp"
#include "./src/XSDParser/Elements/Sequence.hpp"
#include "./src/XSDParser/Elements/Union.hpp"
#include "./src/XSDParser/Elements/Restriction.hpp"
#include "./src/XSDParser/Elements/Choice.hpp"
#include "./src/XSDParser/Elements/Attribute.hpp"
#include "./src/XSDParser/Elements/List.hpp"
#include "./src/XSDParser/Elements/SimpleType.hpp"
#include "./src/XSDParser/Elements/ComplexType.hpp"
#include "./src/XSDParser/Elements/Group.hpp"
#include "./src/XSDParser/Elements/Any.hpp"
#include "./src/XSDParser/Elements/ComplexContent.hpp"
#include "./src/XSDParser/Elements/Extension.hpp"
#include "./src/XSDParser/Elements/SimpleContent.hpp"
#include "./src/XSDParser/Elements/MinExclusive.hpp"
#include "./src/XSDParser/Elements/MaxExclusive.hpp"
#include "./src/XSDParser/Elements/MinInclusive.hpp"
#include "./src/XSDParser/Elements/MaxInclusive.hpp"
#include "./src/XSDParser/Elements/MinLength.hpp"
#include "./src/XSDParser/Elements/MaxLength.hpp"
#include "./src/XSDParser/Elements/FractionDigits.hpp"
#include "./src/XSDParser/Elements/Pattern.hpp"
#include "./src/XSDParser/Elements/TotalDigits.hpp"
#include "./src/XSDParser/Elements/WhiteSpace.hpp"
#include "./src/XSDParser/Elements/AttributeGroup.hpp"
#include "./src/XSDParser/Elements/Include.hpp"
#include "./src/XSDParser/Elements/Annotation.hpp"
#include "./src/XSDParser/Elements/Documentation.hpp"
#include "./src/XSDParser/Elements/All.hpp"
#include "./src/Processors/LuaProcessor.hpp"
#include "./src/Processors/LuaAdapter.hpp"
#include "./src/Processors/SimpleTypeExtracter.hpp"
#include "./src/Processors/ArrayType.hpp"

using namespace std;
using namespace Processors;

LuaProcessor::LuaProcessor(lua_State * pLuaState)
	: LuaProcessorBase(new LuaAdapter(pLuaState))
{ }

LuaProcessor::LuaProcessor(const LuaProcessor& rProcessor)
	: LuaProcessorBase(rProcessor)
{ }

LuaProcessor::LuaProcessor(LuaAdapter * pLuaAdapter)
	: LuaProcessorBase(pLuaAdapter)
{ }

/* virtual */
LuaProcessor::~LuaProcessor() {
}

/* virtual */ void
LuaProcessor::ProcessSchema(const XSD::Elements::Schema* pNode) {
	if (pNode->isRootSchema()) {
		/* create root schema table and append it to stack */
		LuaProcessor processor(_luaAdapter()->Schema());
		pNode->ParseChildren(processor);
	} else {
		/* parse schema children */
		pNode->ParseChildren(*this);
	}
}

/* virtual */ void
LuaProcessor::ProcessElement(const XSD::Elements::Element* pNode) {
	/* don't process element if it can't occur */
	if (pNode->HasMaxOccurs() && (0 == pNode->MaxOccurs()))
		return;
	/* output the element */
	if (!pNode->HasRef()) {
		unique_ptr<XSD::Types::BaseType> pElmType(pNode->Type());
		/* process element type */
		/* inserts basic type. Handle array types the same as basic types */
		LuaContent * pLuaContent = dynamic_cast<LuaContent*>(_luaAdapter());
		if (NULL == pLuaContent) {
			/* enter the type content table & add elements, then leave */
			LuaType * pLuaType = dynamic_cast<LuaType*>(_luaAdapter());
			LuaProcessor processor(pLuaType->Content());
			processor.ProcessElement(pNode);
		} else {
			/* Default maxOccurs is 1 */
			int maxOccurs = pNode->HasMaxOccurs() ?	pNode->MaxOccurs() : 1;
			LuaProcessor luaPrcssr(pLuaContent->Type(pNode->Name(), maxOccurs));
			luaPrcssr._parseType(*pElmType);
		}
	} else {
		unique_ptr<XSD::Elements::Element> pRefElm(pNode->RefElement());
		ProcessElement(pRefElm.get());
	}
}

/* virtual */ void
LuaProcessor::ProcessUnion(const XSD::Elements::Union* pNode) {
	_parseType(XSD::Types::String());
}

/* virtual */ void
LuaProcessor::ProcessRestriction(const XSD::Elements::Restriction* pNode ) { 
	
	if (pNode->isParentComplexContent()) {
		pNode->ParseChildren(*this);
	} else if (pNode->isParentSimpleContent()) {
		pNode->ParseChildren(*this);
		unique_ptr<XSD::Types::BaseType> pType(pNode->Base());
		_parseType(*pType);
	} else {
		unique_ptr<XSD::Types::BaseType> pType(pNode->Base());
		_parseType(*pType);
	}
}

/* virtual */ void
LuaProcessor::ProcessList(const XSD::Elements::List* pNode) {
	unique_ptr<XSD::Types::BaseType> pTypeLst(pNode->ItemType());
	/* extract xsd native type from simple type */
	SimpleTypeExtracter typeXtr;
	/* create array type */
	_parseType(Types::ArrayType(typeXtr.Extract(*pTypeLst)));
}

/* virtual */ void 
LuaProcessor::ProcessSequence(const XSD::Elements::Sequence * pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void 
LuaProcessor::ProcessChoice(const XSD::Elements::Choice * pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessor::ProcessAttribute(const XSD::Elements::Attribute* pNode) {
	if (pNode->HasRef()) {
		unique_ptr<XSD::Elements::Attribute> pRefAtt(pNode->RefAttribute());
		ProcessAttribute(pRefAtt.get());
	} else {
		/* extract xsd native type from simple type and add it */
		SimpleTypeExtracter typeXtr;
		unique_ptr<XSD::Types::BaseType> pType(pNode->Type());
		LuaType * pLuaType = dynamic_cast<LuaType*>(_luaAdapter());
		unique_ptr<string> pDefault(nullptr);
		unique_ptr<string> pFixed(nullptr);
		unique_ptr<string> pUse(new string("optional"));
		if (pNode->HasDefault()) {
			pDefault.reset(new string(pNode->Default()));
		}
		if (pNode->HasFixed()) {
			pFixed.reset(new string(pNode->Fixed()));
		}
		if (pNode->HasUse()) {
			switch (pNode->Use()) {
			case XSD::Elements::Attribute::OPTIONAL:
				pUse.reset(new string("optional"));
				break;
			case XSD::Elements::Attribute::PROHIBITIED:
				pUse.reset(new string("prohibited"));
				break;
			case XSD::Elements::Attribute::REQUIRED:
				pUse.reset(new string("required"));
				break;
			default:
				assert(	(pNode->Use() != XSD::Elements::Attribute::OPTIONAL) &&
						(pNode->Use() != XSD::Elements::Attribute::PROHIBITIED) &&
						(pNode->Use() != XSD::Elements::Attribute::REQUIRED));
			}
		}
		unique_ptr<LuaAttribute> pAttribute(	
			pLuaType->Attribute(pNode->Name(), 
								typeXtr.Extract(*pType), 
								pDefault.get(),
								pFixed.get(),
								pUse.get()
							)
			);
	}
}

/* virtual */ void 
LuaProcessor::ProcessComplexType(const XSD::Elements::ComplexType* pNode) {
	/* add a string to the content type if it is mixed mode */
	if (pNode->Mixed()) {
		/* inserts basic type. Handle array types the same as basic types */
		LuaContent * pLuaContent = dynamic_cast<LuaContent*>(_luaAdapter());
		if (NULL == pLuaContent) {
			/* enter the type content table & add elements, then leave */
			LuaType * pLuaType = dynamic_cast<LuaType*>(_luaAdapter());
			LuaProcessor processor(pLuaType->Content());
			processor.ProcessComplexType(pNode);
		} else {
			unique_ptr<XSD::Types::String> pType(new XSD::Types::String());
			delete (pLuaContent->Type(pType->Name(), 1));
			pNode->ParseChildren(*this);
		}
	} else {
		pNode->ParseChildren(*this);
	}
}

/* virtual */ void
LuaProcessor::ProcessGroup(const XSD::Elements::Group* pNode) {
	/* only parse groups when they are referred */
	if (pNode->HasRef()) {
		unique_ptr<XSD::Elements::Group> pGroup(pNode->RefGroup());
		pGroup->ParseChildren(*this);
	}
}

/* virtual */ void
LuaProcessor::ProcessAny(const XSD::Elements::Any* pNode) {
	/* an any type can be any element in the schema as a child */
	pNode->ParseChildren(*this);
	/* if any is strict, then only the elements allowed in the schema doc tree are valid */
	if (XSD::Elements::Any::STRICT == pNode->ProcessContents()) {
		ElementExtracter::ElementLst elmLst = pNode->GetAllowedElements();
		for (	ElementExtracter::ElementLst::iterator itr = elmLst.begin();
				itr != elmLst.end();
				++itr) {
			ProcessElement(*itr);
			delete *itr;
		}
	}
}

/* virtual */ void
LuaProcessor::ProcessExtension(const XSD::Elements::Extension* pNode) {
	unique_ptr<XSD::Types::BaseType> pBase(pNode->Base());
	_parseType(*pBase);
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessor::ProcessAttributeGroup(const XSD::Elements::AttributeGroup* pNode) {
	/* only parse attribute groups when they are referred */
	if (pNode->HasRef()) {
		unique_ptr<XSD::Elements::AttributeGroup> pGroup(pNode->RefGroup());
		pGroup->ParseChildren(*this);
	}
}

/* virtual */ void
LuaProcessor::ProcessInclude(const XSD::Elements::Include* pNode) {
	const XSD::Elements::Schema * pSchema = pNode->QuerySchema();
	pSchema->ParseChildren(*this);
}

/* virtual */ void 
LuaProcessor::ProcessAll(const XSD::Elements::All * pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessor::_parseType(const XSD::Types::BaseType& rXSDType) {
	if(XSD_ISTYPE(&rXSDType, XSD::Types::SimpleType)) {
		const XSD::Types::SimpleType* pSimpleType = 
			dynamic_cast<const XSD::Types::SimpleType*>(&rXSDType);
		pSimpleType->m_pValue->ParseElement(*this);
	} else if(XSD_ISTYPE(&rXSDType, XSD::Types::ComplexType)) {
		const XSD::Types::ComplexType* pComplexType = 
			dynamic_cast<const XSD::Types::ComplexType*>(&rXSDType);
		pComplexType->m_pValue->ParseElement(*this);
	} else {
		/* inserts basic type. Handles array types the same as basic types */
		LuaType * pLuaType = dynamic_cast<LuaType*>(_luaAdapter());
		unique_ptr<LuaContent> pLuaContent(pLuaType->Content());
		delete (pLuaContent->Type(rXSDType.Name(), 1));
	}
}
