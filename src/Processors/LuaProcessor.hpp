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
 *  along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LUAPROCESSOR_HPP_
#define LUAPROCESSOR_HPP_

#include "./src/XSDParser/Types.hpp"
#include "./src/Processors/LuaProcessorBase.hpp"

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
	protected:
		LuaProcessor(LuaAdapter * pLuaAdapter);
		virtual void _parseType(const XSD::Types::BaseType& rXSDType);
	private:
		LuaProcessor();
	};
}	/* using namespace Processors */
#endif /* LUAPROCESSOR_HPP_ */
