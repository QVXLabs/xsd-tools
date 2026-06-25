/*
 * LuaProcessor.hpp
 *
 *  Created on: Jun 25, 2011
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

#ifndef LUAPROCESSOR_HPP_
#define LUAPROCESSOR_HPP_

#include <memory>
#include <set>
#include "./src/XSDParser/Types.hpp"
#include "./src/Processors/LuaProcessorBase.hpp"
#include "./src/Processors/LuaAdapter.hpp"

namespace XSD { namespace Elements { class Node; } }

namespace Processors {
	class LuaProcessor : public LuaProcessorBase {
	public:
		LuaProcessor(lua_State * pLuaState);
		LuaProcessor(const LuaProcessor& rProccessor);
		virtual ~LuaProcessor();
		virtual void ProcessSchema(const XSD::Elements::Schema* pNode);
		virtual void ProcessElement(const XSD::Elements::Element* pNode);
		virtual void ProcessUnion(const XSD::Elements::Union* pNode);
		virtual void ProcessRestriction(const XSD::Elements::Restriction* pNode );
		virtual void ProcessList(const XSD::Elements::List* pNode);
		virtual void ProcessSequence(const XSD::Elements::Sequence* pNode);
		virtual void ProcessChoice(const XSD::Elements::Choice* pNode );
		virtual void ProcessAttribute(const XSD::Elements::Attribute* pNode);
		virtual void ProcessComplexType(const XSD::Elements::ComplexType* pNode);
		virtual void ProcessGroup(const XSD::Elements::Group* pNode);
		virtual void ProcessAny(const XSD::Elements::Any* pNode);
		virtual void ProcessExtension(const XSD::Elements::Extension* pNode);
		virtual void ProcessAttributeGroup(const XSD::Elements::AttributeGroup* pNode);
		virtual void ProcessInclude(const XSD::Elements::Include* pNode);
		virtual void ProcessAll(const XSD::Elements::All* pNode);
		/* facet callbacks: record values into the facet accumulator */
		virtual void ProcessMinExclusive(const XSD::Elements::MinExclusive* pNode);
		virtual void ProcessMaxExclusive(const XSD::Elements::MaxExclusive* pNode);
		virtual void ProcessMinInclusive(const XSD::Elements::MinInclusive* pNode);
		virtual void ProcessMaxInclusive(const XSD::Elements::MaxInclusive* pNode);
		virtual void ProcessMinLength(const XSD::Elements::MinLength* pNode);
		virtual void ProcessMaxLength(const XSD::Elements::MaxLength* pNode);
		virtual void ProcessLength(const XSD::Elements::Length* pNode);
		virtual void ProcessEnumeration(const XSD::Elements::Enumeration* pNode);
		virtual void ProcessPattern(const XSD::Elements::Pattern* pNode);
		virtual void ProcessTotalDigits(const XSD::Elements::TotalDigits* pNode);
		virtual void ProcessFractionDigits(const XSD::Elements::FractionDigits* pNode);
		virtual void ProcessWhiteSpace(const XSD::Elements::WhiteSpace* pNode);
	protected:
		LuaProcessor(LuaAdapter * pLuaAdapter);
		virtual void parseType_(const XSD::Types::BaseType& rXSDType);
	private:
		LuaProcessor();
		/* dispatch a restriction's facet children to this processor */
		void walkFacets_(const XSD::Elements::Node* pNode);
		/* accumulate an attribute's restriction facets onto m_facets */
		void walkAttributeFacets_(const XSD::Types::BaseType* pType);
		/* facets accumulated across a restriction derivation chain */
		LuaFacets m_facets;
		/* types currently being expanded on the active path; shared across
		 * the nested processors of one generation so a type that re-enters
		 * its own expansion (a cyclic schema) is rejected instead of
		 * recursing until the stack overflows */
		std::shared_ptr<std::set<const void*> > m_activePath;
	};
}	/* using namespace Processors */
#endif /* LUAPROCESSOR_HPP_ */
