/*
 * LuaAdapter.hpp
 *
 *  Created on: 01/22/12
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

#ifndef LUAADAPTER_HPP_
#define LUAADAPTER_HPP_
#include <string>
#include <vector>
#include <utility>
#include <lua.hpp>
#include "./src/XSDParser/Types.hpp"

namespace Processors {
	/* accumulated XSD restriction facets destined for a Lua type's
	   `facets` sub-table. Scalar facets are single name/value pairs;
	   pattern/enumeration accumulate as ordered lists. */
	struct LuaFacets {
		/* single-valued facets, in insertion order */
		std::vector<std::pair<std::string, std::string> > scalars;
		/* multi-valued facets keyed by name (e.g. pattern, enumeration) */
		std::vector<std::pair<std::string, std::vector<std::string> > > lists;
		bool empty() const {
			return scalars.empty() && lists.empty();
		}
		void clear() {
			scalars.clear();
			lists.clear();
		}
		void addScalar(const std::string& rName, const std::string& rValue) {
			scalars.push_back(std::make_pair(rName, rValue));
		}
		void addToList(const std::string& rName, const std::string& rValue);
	};

	/* forward declare classes */
	class LuaAdapter;
	class LuaType;
	class LuaAttribute;
	class LuaContent;
	class LuaSchema;
	/* The lua adapter classes enable the stack of recursive processors to maintain an object stack that represents
	   the output schema. Each time the processor adds a new item (element type def or attribute) on to the schema 
	   table, it creates a new processor which intern references one of these objects */
	/* core class */
	class LuaAdapter {
	public:
		LuaAdapter(lua_State * pLuaState);
		virtual ~LuaAdapter();
		LuaSchema * Schema();
	protected:
		lua_State * getLuaState_();
		void setLuaState_(lua_State * pLuaState);
	private:
		lua_State *	pLuaState_;
		LuaAdapter(const LuaAdapter&);
	};
	/* lua content class */
	class LuaContent : public LuaAdapter {
		friend class LuaType;
	public:
		virtual ~LuaContent();
		LuaType * Type(const std::string& rTypeName, const int maxOccurs,
		               const int minOccurs = 1);
	protected:
		LuaContent();
		LuaContent(lua_State* pLuaState);
	};
	/* lua schema class */
	class LuaSchema : public LuaContent {
		friend class LuaAdapter;
	public:
		virtual ~LuaSchema();
	protected:
		LuaSchema(lua_State* pLuaState);
	};

	/* lua type class */
	class LuaType : public LuaAdapter {
		friend class LuaSchema;
		friend class LuaContent;
		friend class LuaAttribute;
	public:
		virtual ~LuaType();
		LuaAttribute * Attribute(	const std::string& rName, 
									const XSD::Types::BaseType& rType,
									const std::string * pDefault,
									const std::string * pFixed,
									const std::string * pUse);
		LuaContent * Content();
		/* attach a `facets` sub-table; no-op when rFacets is empty */
		void Facets(const LuaFacets& rFacets);
		/* attach the resolved namespace URI; no-op when rNamespace is empty */
		void Namespace(const std::string& rNamespace);
		/* attach the qualified flag; no-op when unqualified (the default) */
		void Qualified(bool qualified);
		/* attach the xs:documentation text; no-op when empty, and does not
		   overwrite a doc already set (element-level wins over type-level) */
		void Documentation(const std::string& rText);
	protected:
		LuaType(lua_State * pLuaState, const std::string& rTypeName,
		        const int maxOccurs, const int minOccurs = 1);
	};
	/* lua attribute class */
	class LuaAttribute : public LuaAdapter {
		friend class LuaType;
	public:
		virtual ~LuaAttribute();
		/* attach a `facets` sub-table; no-op when rFacets is empty */
		void Facets(const LuaFacets& rFacets);
		/* attach the resolved namespace URI; no-op when rNamespace is empty */
		void Namespace(const std::string& rNamespace);
		/* attach the qualified flag; no-op when unqualified (the default) */
		void Qualified(bool qualified);
		/* attach the xs:documentation text; no-op when empty */
		void Documentation(const std::string& rText);
	protected:
		LuaAttribute(	lua_State * pLuaState,
						const std::string& rAttribName,
						const XSD::Types::BaseType& rType,
						const std::string * pDefault,
						const std::string * pFixed,
						const std::string * pUse
					);
	private:
		/* attribute + type-table keys; let Facets() re-descend to the type
		   sub-table where targets read `attr.<type>.facets` */
		std::string name_;
		std::string typeName_;
	};
}
#endif /* LUAADAPTER_HPP_ */
