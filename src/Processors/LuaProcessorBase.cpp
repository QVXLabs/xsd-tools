/*
 * LuaProcBase.hpp
 *
 *  Created on: Dec 16, 2011
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
 
#include <memory>
#include <lua.hpp>
#include "./src/XSDParser/Parser.hpp"
#include "./src/Processors/LuaProcessorBase.hpp"
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
#include "./src/XSDParser/Elements/Length.hpp"
#include "./src/XSDParser/Elements/FractionDigits.hpp"
#include "./src/XSDParser/Elements/Pattern.hpp"
#include "./src/XSDParser/Elements/TotalDigits.hpp"
#include "./src/XSDParser/Elements/WhiteSpace.hpp"
#include "./src/XSDParser/Elements/AttributeGroup.hpp"
#include "./src/XSDParser/Elements/Include.hpp"
#include "./src/XSDParser/Elements/Annotation.hpp"
#include "./src/XSDParser/Elements/Documentation.hpp"
#include "./src/XSDParser/Elements/Enumeration.hpp"
#include "./src/XSDParser/Elements/All.hpp"
#include "./src/XSDParser/Elements/AppInfo.hpp"

using namespace std;
using namespace Processors;

LuaProcessorBase::LuaProcessorBase(LuaAdapter * pLuaAdapter)
	: m_pLuaAdapter(pLuaAdapter)
{ }

LuaProcessorBase::LuaProcessorBase(const LuaProcessorBase& rProcessor)
	: m_pLuaAdapter(rProcessor.m_pLuaAdapter)
{ }

/* virtual */
LuaProcessorBase::~LuaProcessorBase() {
	delete m_pLuaAdapter;
}

/* virtual */ void
LuaProcessorBase::ProcessSchema(const XSD::Elements::Schema* pNode) {
	pNode->ParseChildren(*this);
}
	
/* virtual */ void 
LuaProcessorBase::ProcessElement(const XSD::Elements::Element* pNode) {
	pNode->ParseChildren(*this);
}
	
/* virtual */ void
LuaProcessorBase::ProcessUnion(const XSD::Elements::Union* pNode) {
	pNode->ParseChildren(*this);
}
	
/* virtual */ void
LuaProcessorBase::ProcessRestriction(const XSD::Elements::Restriction* pNode ) {
	pNode->ParseChildren(*this);
}
	
/* virtual */ void
LuaProcessorBase::ProcessList(const XSD::Elements::List* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessorBase::ProcessSequence(const XSD::Elements::Sequence* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessorBase::ProcessChoice(const XSD::Elements::Choice* pNode ) {
	pNode->ParseChildren(*this);
}
	
/* virtual */ void
LuaProcessorBase::ProcessAttribute(const XSD::Elements::Attribute* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessorBase::ProcessSimpleType(const XSD::Elements::SimpleType* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessorBase::ProcessComplexType(const XSD::Elements::ComplexType* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessorBase::ProcessGroup(const XSD::Elements::Group* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessorBase::ProcessAny(const XSD::Elements::Any* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessorBase::ProcessComplexContent(const XSD::Elements::ComplexContent* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessorBase::ProcessExtension(const XSD::Elements::Extension* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessorBase::ProcessSimpleContent(const XSD::Elements::SimpleContent* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessorBase::ProcessMinExclusive(const XSD::Elements::MinExclusive* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessorBase::ProcessMaxExclusive(const XSD::Elements::MaxExclusive* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessorBase::ProcessMinInclusive(const XSD::Elements::MinInclusive* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessorBase::ProcessMaxInclusive(const XSD::Elements::MaxInclusive* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessorBase::ProcessMinLength(const XSD::Elements::MinLength* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessorBase::ProcessMaxLength(const XSD::Elements::MaxLength* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessorBase::ProcessLength(const XSD::Elements::Length* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessorBase::ProcessEnumeration(const XSD::Elements::Enumeration* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void 
LuaProcessorBase::ProcessFractionDigits(const XSD::Elements::FractionDigits* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void 
LuaProcessorBase::ProcessPattern(const XSD::Elements::Pattern* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void 
LuaProcessorBase::ProcessTotalDigits(const XSD::Elements::TotalDigits* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void 
LuaProcessorBase::ProcessWhiteSpace(const XSD::Elements::WhiteSpace* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessorBase::ProcessAttributeGroup(const XSD::Elements::AttributeGroup* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessorBase::ProcessInclude(const XSD::Elements::Include* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessorBase::ProcessAnnotation(const XSD::Elements::Annotation* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessorBase::ProcessDocumentation(const XSD::Elements::Documentation* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessorBase::ProcessAll(const XSD::Elements::All* pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessorBase::ProcessAppInfo(const XSD::Elements::AppInfo* pNode) {
	pNode->ParseChildren(*this);
}

LuaAdapter *
LuaProcessorBase::_luaAdapter() const {
	return m_pLuaAdapter;
}
