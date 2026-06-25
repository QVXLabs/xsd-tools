/*
 * LuaAdapter.cpp
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

#include <iostream>
#include "./src/Processors/LuaAdapter.hpp"
#define SCHEMA_TAG     "schema"
#define ATTRIBUTE_TAG  "attributes"
#define CONTENT_TAG    "content"
#define DEFAULT_TAG    "default"
#define FIXED_TAG      "fixed"
#define USE_TAG        "use"
#define MAXOCCURS_TAG  "maxOccurs"
#define MINOCCURS_TAG  "minOccurs"
#define FACETS_TAG     "facets"
#define DEBUG_LUASTACK (0)

using namespace std;
using namespace Processors;

/* class LuaFacets */
void
LuaFacets::addToList(const std::string& rName, const std::string& rValue) {
	for (size_t i = 0; i < lists.size(); ++i) {
		if (lists[i].first == rName) {
			lists[i].second.push_back(rValue);
			return;
		}
	}
	lists.push_back(
		std::make_pair(rName, std::vector<std::string>(1, rValue)));
}

/* helper functions */
static void luaStackDump_(lua_State * pLuaState);
#if (DEBUG_LUASTACK)
static void luaStackDumpRec_(lua_State * pLuaState, int stackIndex = 0);
#endif
/* Class method definitions */
/* Class LuaAdapter */
LuaAdapter::LuaAdapter(lua_State* pLuaState)
	: pLuaState_(pLuaState)
{ }

/* virtual */
LuaAdapter::~LuaAdapter()
{ }

LuaSchema *
LuaAdapter::Schema() {
	return new LuaSchema(pLuaState_);
}

lua_State *
LuaAdapter::getLuaState_() {
  return pLuaState_;
}

void
LuaAdapter::setLuaState_(lua_State * pLuaState) {
	pLuaState_ = pLuaState;
}

/* Class LuaContent */
LuaContent::LuaContent() 
	: LuaAdapter(NULL) 
{ }

LuaContent::LuaContent(lua_State* pLuaState) 
	: LuaAdapter(pLuaState) { 
	/* push content table to stack top */
	lua_getfield(pLuaState, -1, CONTENT_TAG);	
}

/* virtual */
LuaContent::~LuaContent() {  
	lua_pop(getLuaState_(), 1);
}

LuaType *
LuaContent::Type(const std::string& rTypeName, const int maxOccurs,
                 const int minOccurs) {
	return new LuaType(getLuaState_(), rTypeName, maxOccurs, minOccurs);
}

/* Class LuaSchema */
LuaSchema::LuaSchema(lua_State* pLuaState) {
	/* set the lua state without calling the content constructor */
	setLuaState_(pLuaState);
	/* test if global table has been made & make it if not */
	lua_getglobal(pLuaState, SCHEMA_TAG);
	if (lua_isnil(pLuaState, -1)) {
		lua_pop(pLuaState, 1);
		lua_newtable(pLuaState);
		lua_setglobal(pLuaState, SCHEMA_TAG);
		lua_getglobal(pLuaState, SCHEMA_TAG);
	}
	/* debug */
	luaStackDump_(pLuaState);
}

/* virtual */
LuaSchema::~LuaSchema() 
{ }

/* Class LuaType */
LuaType::LuaType(lua_State* pLuaState, const std::string& rTypeName,
                 const int maxOccurs, const int minOccurs)
	: LuaAdapter(pLuaState) {
	/* create new table for type and append it as a child element to the current
	   element on stack */
	lua_newtable(pLuaState);
	lua_setfield(pLuaState, -2, rTypeName.c_str());
	lua_getfield(pLuaState, -1, rTypeName.c_str());
	/* create new table for type attributes */
	lua_newtable(pLuaState);
	lua_setfield(pLuaState, -2, ATTRIBUTE_TAG);
	/* create new table for type contents */
	lua_newtable(pLuaState);
	lua_setfield(pLuaState, -2, CONTENT_TAG);

 	lua_pushnumber(pLuaState, maxOccurs);
	lua_setfield(pLuaState, -2, MAXOCCURS_TAG);
	lua_pushnumber(pLuaState, minOccurs);
	lua_setfield(pLuaState, -2, MINOCCURS_TAG);
	/* debug */
	luaStackDump_(pLuaState);
}

/* virtual */
LuaType::~LuaType() { 
	lua_pop(getLuaState_(), 1);
}

LuaAttribute *
LuaType::Attribute(	const string& rName, 
					const XSD::Types::BaseType& rType, 
					const std::string * pDefault,
					const std::string * pFixed,
					const std::string * pUse) {
	return new LuaAttribute(getLuaState_(), rName, rType, pDefault, pFixed, pUse);
}

LuaContent *
LuaType::Content() {
	return new LuaContent(getLuaState_());
}

void
LuaType::Facets(const LuaFacets& rFacets) {
	/* only emit a `facets` block when there is something to emit */
	if (rFacets.empty())
		return;
	lua_State * pLuaState = getLuaState_();
	/* type table is at stack top; build facets sub-table */
	lua_newtable(pLuaState);
	for (size_t i = 0; i < rFacets.scalars.size(); ++i) {
		lua_pushstring(pLuaState, rFacets.scalars[i].second.c_str());
		lua_setfield(pLuaState, -2, rFacets.scalars[i].first.c_str());
	}
	for (size_t i = 0; i < rFacets.lists.size(); ++i) {
		const std::vector<std::string>& rVals = rFacets.lists[i].second;
		lua_newtable(pLuaState);
		for (size_t j = 0; j < rVals.size(); ++j) {
			lua_pushstring(pLuaState, rVals[j].c_str());
			lua_rawseti(pLuaState, -2, static_cast<int>(j + 1));
		}
		lua_setfield(pLuaState, -2, rFacets.lists[i].first.c_str());
	}
	lua_setfield(pLuaState, -2, FACETS_TAG);
	/* debug */
	luaStackDump_(pLuaState);
}

/* Class LuaAttribute */
LuaAttribute::LuaAttribute(	lua_State * pLuaState, 
							const std::string& rName, 
							const XSD::Types::BaseType& rType,
							const std::string * pDefault,
							const std::string * pFixed,
							const std::string * pUse) 
	: LuaAdapter(pLuaState), name_(rName), typeName_(rType.Name()) {
	/* push attribute table to stack top */
	lua_getfield(pLuaState, -1, ATTRIBUTE_TAG);
	/* create emtpty table for attribute name/type pair */
	lua_newtable(pLuaState);
	LuaType * pType = new LuaType(pLuaState, rType.Name(), 1);
	/* append default value to type definition */
	if (pDefault) {
		lua_pushstring(pLuaState, pDefault->c_str());
		lua_setfield(pLuaState, -2, DEFAULT_TAG);
	}
	/* append fixed value to type definition */
	if (pFixed) {
		lua_pushstring(pLuaState, pFixed->c_str());
		lua_setfield(pLuaState, -2, FIXED_TAG);
	}
	/* append use [optional|prohibited|required] value to type definition */
	if (pUse) {
		lua_pushstring(pLuaState, pUse->c_str());
		lua_setfield(pLuaState, -2, USE_TAG);
	}
	/* pop off attribute type from lua stack */
	delete pType;
	/* append attribute to attribute table */
	lua_setfield(pLuaState, -2, rName.c_str());
	/* debug */
	luaStackDump_(pLuaState);
}

void
LuaAttribute::Facets(const LuaFacets& rFacets) {
	/* only emit a `facets` block when there is something to emit */
	if (rFacets.empty())
		return;
	lua_State * pLuaState = getLuaState_();
	/* the ATTRIBUTE_TAG table is at stack top; descend attributes[name] ->
	   [typeName] so facets land on the type sub-table (where targets read
	   attr.<type>.facets, mirroring content-type facets) */
	lua_getfield(pLuaState, -1, name_.c_str());
	lua_getfield(pLuaState, -1, typeName_.c_str());
	lua_newtable(pLuaState);
	for (size_t i = 0; i < rFacets.scalars.size(); ++i) {
		lua_pushstring(pLuaState, rFacets.scalars[i].second.c_str());
		lua_setfield(pLuaState, -2, rFacets.scalars[i].first.c_str());
	}
	for (size_t i = 0; i < rFacets.lists.size(); ++i) {
		const std::vector<std::string>& rVals = rFacets.lists[i].second;
		lua_newtable(pLuaState);
		for (size_t j = 0; j < rVals.size(); ++j) {
			lua_pushstring(pLuaState, rVals[j].c_str());
			lua_rawseti(pLuaState, -2, static_cast<int>(j + 1));
		}
		lua_setfield(pLuaState, -2, rFacets.lists[i].first.c_str());
	}
	lua_setfield(pLuaState, -2, FACETS_TAG);
	/* pop the type + attribute tables, restoring ATTRIBUTE_TAG at top */
	lua_pop(pLuaState, 2);
	/* debug */
	luaStackDump_(pLuaState);
}

/* virtual */
LuaAttribute::~LuaAttribute() {
	lua_pop(getLuaState_(), 1);
}

static void luaStackDump_(lua_State * pLuaState) {
#if (DEBUG_LUASTACK)
	cout << "LStack:[";
	luaStackDumpRec_(pLuaState, 0);
	cout << "]" << endl;
#endif
}

#if (DEBUG_LUASTACK)
static void luaStackDumpRec_(lua_State * pLuaState, int stackIndex) {
	const int stkTop = lua_gettop(pLuaState);
	/* base case */
	if (stackIndex == stkTop) 
		return;
	/* general case */
	switch (lua_type(pLuaState, stackIndex)) {
	case LUA_TNIL:
		cout << "nil";
		break;
	case LUA_TNUMBER: {
		lua_Number val = lua_tonumber(pLuaState, stackIndex);
		cout << val;
		break;
	}
	case LUA_TBOOLEAN: {
		bool val = (lua_toboolean(pLuaState, stackIndex) == 1);
		cout << val;
		break;
	}
	case LUA_TSTRING: {
		const char * pCStr = lua_tolstring(pLuaState, stackIndex, NULL);
		cout << "\"" << pCStr << "\"";
		break;
	}
	case LUA_TTABLE:
		cout << "tbl";
		break;
	case LUA_TFUNCTION:
		cout << "fnc";
		break;	
	case LUA_TUSERDATA:
		cout << "user_data";
		break;
	case LUA_TTHREAD:
		cout << "thread";
		break;
	case LUA_TLIGHTUSERDATA:
		cout << "light_user_data";
		break;
	default:
		break;
	}
	cout << " ";
	luaStackDumpRec_(pLuaState, stackIndex + 1);
}
#endif
