/*
 * LuaProcBase.hpp
 *
 *  Created on: Dec 15, 2011
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

#ifndef LUAPROCBASE_HPP_
#define LUAPROCBASE_HPP_
#include <string>
#include <lua.hpp>
#include "./src/util.hpp"
#include "./src/Processors/LuaAdapter.hpp"
#include "./src/XSDParser/Types.hpp"
#include "./src/XSDParser/ProcessorBase.hpp"

namespace Processors {
	class LuaProcessorBase : public XSD::BaseProcessor {
	public:
		LuaProcessorBase(LuaAdapter * pLuaAdapter);
		LuaProcessorBase(const LuaProcessorBase& rProcessor);
		virtual ~LuaProcessorBase();
		virtual void ProcessSchema(const XSD::Elements::Schema* pNode);
		virtual void ProcessElement(const XSD::Elements::Element* pNode);
		virtual void ProcessUnion(const XSD::Elements::Union* pNode);
		virtual void ProcessRestriction(const XSD::Elements::Restriction* pNode );
		virtual void ProcessList(const XSD::Elements::List* pNode);
		virtual void ProcessSequence(const XSD::Elements::Sequence* pNode);
		virtual void ProcessChoice(const XSD::Elements::Choice* pNode );
		virtual void ProcessAttribute(const XSD::Elements::Attribute* pNode);
		virtual void ProcessSimpleType(const XSD::Elements::SimpleType* pNode);
		virtual void ProcessComplexType(const XSD::Elements::ComplexType* pNode);
		virtual void ProcessGroup(const XSD::Elements::Group* pNode);
		virtual void ProcessAny(const XSD::Elements::Any* pNode);
		virtual void ProcessComplexContent(const XSD::Elements::ComplexContent* pNode);
		virtual void ProcessExtension(const XSD::Elements::Extension* pNode);
		virtual void ProcessSimpleContent(const XSD::Elements::SimpleContent* pNode);
		virtual void ProcessMinExclusive(const XSD::Elements::MinExclusive* pNode);
		virtual void ProcessMaxExclusive(const XSD::Elements::MaxExclusive* pNode);
		virtual void ProcessMinInclusive(const XSD::Elements::MinInclusive* pNode);
		virtual void ProcessMaxInclusive(const XSD::Elements::MaxInclusive* pNode);
		virtual void ProcessMinLength(const XSD::Elements::MinLength* pNode);
		virtual void ProcessMaxLength(const XSD::Elements::MaxLength* pNode);
		virtual void ProcessLength(const XSD::Elements::Length* pNode);
		virtual void ProcessEnumeration(const XSD::Elements::Enumeration* pNode);
		virtual void ProcessFractionDigits(const XSD::Elements::FractionDigits* pNode);
		virtual void ProcessAttributeGroup(const XSD::Elements::AttributeGroup* pNode);
		virtual void ProcessPattern(const XSD::Elements::Pattern* pNode);
		virtual void ProcessTotalDigits(const XSD::Elements::TotalDigits* pNode);
		virtual void ProcessWhiteSpace(const XSD::Elements::WhiteSpace* pNode);
		virtual void ProcessInclude(const XSD::Elements::Include* pNode);
		virtual void ProcessAnnotation(const XSD::Elements::Annotation* pNode);
		virtual void ProcessDocumentation(const XSD::Elements::Documentation* pNode);
		virtual void ProcessAll(const XSD::Elements::All* pNode);
		virtual void ProcessAppInfo(const XSD::Elements::AppInfo* pNode);
	protected:
		LuaAdapter *  	luaAdapter_() const;
	private:
		mutable LuaAdapter * pLuaAdapter_;
	};
}
#endif /* LUAPROCBASE_HPP_ */
