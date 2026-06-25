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
	: pLuaAdapter_(pLuaAdapter)
{ }

LuaProcessorBase::LuaProcessorBase(const LuaProcessorBase& rProcessor)
	: pLuaAdapter_(rProcessor.pLuaAdapter_)
{ }

/* virtual */
LuaProcessorBase::~LuaProcessorBase() {
	delete pLuaAdapter_;
}

/* Every base callback just recurses into the node's children. Enumerate the
 * (ProcessXxx, element type) pairs once and stamp out the bodies. */
#define LUA_PROC_BASE_CALLBACKS(X) \
	X(ProcessSchema,         Schema) \
	X(ProcessElement,        Element) \
	X(ProcessUnion,          Union) \
	X(ProcessRestriction,    Restriction) \
	X(ProcessList,           List) \
	X(ProcessSequence,       Sequence) \
	X(ProcessChoice,         Choice) \
	X(ProcessAttribute,      Attribute) \
	X(ProcessSimpleType,     SimpleType) \
	X(ProcessComplexType,    ComplexType) \
	X(ProcessGroup,          Group) \
	X(ProcessAny,            Any) \
	X(ProcessComplexContent, ComplexContent) \
	X(ProcessExtension,      Extension) \
	X(ProcessSimpleContent,  SimpleContent) \
	X(ProcessMinExclusive,   MinExclusive) \
	X(ProcessMaxExclusive,   MaxExclusive) \
	X(ProcessMinInclusive,   MinInclusive) \
	X(ProcessMaxInclusive,   MaxInclusive) \
	X(ProcessMinLength,      MinLength) \
	X(ProcessMaxLength,      MaxLength) \
	X(ProcessLength,         Length) \
	X(ProcessEnumeration,    Enumeration) \
	X(ProcessFractionDigits, FractionDigits) \
	X(ProcessPattern,        Pattern) \
	X(ProcessTotalDigits,    TotalDigits) \
	X(ProcessWhiteSpace,     WhiteSpace) \
	X(ProcessAttributeGroup, AttributeGroup) \
	X(ProcessInclude,        Include) \
	X(ProcessAnnotation,     Annotation) \
	X(ProcessDocumentation,  Documentation) \
	X(ProcessAll,            All) \
	X(ProcessAppInfo,        AppInfo)

#define LUA_PROC_BASE_DEFN(METHOD, TYPE) \
	/* virtual */ void \
	LuaProcessorBase::METHOD(const XSD::Elements::TYPE* pNode) { \
		pNode->ParseChildren(*this); \
	}
LUA_PROC_BASE_CALLBACKS(LUA_PROC_BASE_DEFN)
#undef LUA_PROC_BASE_DEFN
#undef LUA_PROC_BASE_CALLBACKS

LuaAdapter *
LuaProcessorBase::luaAdapter_() const {
	return pLuaAdapter_;
}
