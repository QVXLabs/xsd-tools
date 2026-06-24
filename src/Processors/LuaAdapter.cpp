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
#define DEBUG_LUASTACK (0)

using namespace std;
using namespace Processors;

/* helper functions */
static void _luaStackDump(lua_State * pLuaState);
#if (DEBUG_LUASTACK)
static void _luaStackDumpRec(lua_State * pLuaState, int stackIndex = 0);
#endif
/* Class method definitions */
/* Class LuaAdapter */
LuaAdapter::LuaAdapter(lua_State* pLuaState)
	: m_pLuaState(pLuaState)
{ }

/* virtual */
LuaAdapter::~LuaAdapter()
{ }

LuaSchema *
LuaAdapter::Schema() {
	return new LuaSchema(m_pLuaState);
}

lua_State *
LuaAdapter::_getLuaState() {
  return m_pLuaState;
}

void
LuaAdapter::_setLuaState(lua_State * pLuaState) {
	m_pLuaState = pLuaState;
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
	lua_pop(_getLuaState(), 1);
}

LuaType *
LuaContent::Type(const std::string& rTypeName, const int maxOccurs) {
	return new LuaType(_getLuaState(), rTypeName, maxOccurs);
}

/* Class LuaSchema */
LuaSchema::LuaSchema(lua_State* pLuaState) {
	/* set the lua state without calling the content constructor */
	_setLuaState(pLuaState);
	/* test if global table has been made & make it if not */
	lua_getglobal(pLuaState, SCHEMA_TAG);
	if (lua_isnil(pLuaState, -1)) {
		lua_pop(pLuaState, 1);
		lua_newtable(pLuaState);
		lua_setglobal(pLuaState, SCHEMA_TAG);
		lua_getglobal(pLuaState, SCHEMA_TAG);
	}
	/* debug */
	_luaStackDump(pLuaState);
}

/* virtual */
LuaSchema::~LuaSchema() 
{ }

/* Class LuaType */
LuaType::LuaType(lua_State* pLuaState, const std::string& rTypeName, const int maxOccurs)
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
	/* debug */
	_luaStackDump(pLuaState);
}

/* virtual */
LuaType::~LuaType() { 
	lua_pop(_getLuaState(), 1);
}

LuaAttribute *
LuaType::Attribute(	const string& rName, 
					const XSD::Types::BaseType& rType, 
					const std::string * pDefault,
					const std::string * pFixed,
					const std::string * pUse) {
	return new LuaAttribute(_getLuaState(), rName, rType, pDefault, pFixed, pUse);
}

LuaContent *
LuaType::Content() {
	return new LuaContent(_getLuaState()); 
}

/* Class LuaAttribute */
LuaAttribute::LuaAttribute(	lua_State * pLuaState, 
							const std::string& rName, 
							const XSD::Types::BaseType& rType,
							const std::string * pDefault,
							const std::string * pFixed,
							const std::string * pUse) 
	: LuaAdapter(pLuaState) {
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
	_luaStackDump(pLuaState);
}

/* virtual */
LuaAttribute::~LuaAttribute() {
	lua_pop(_getLuaState(), 1);
}

static void _luaStackDump(lua_State * pLuaState) {
#if (DEBUG_LUASTACK)
	cout << "LStack:[";
	_luaStackDumpRec(pLuaState, 0);
	cout << "]" << endl;
#endif
}

#if (DEBUG_LUASTACK)
static void _luaStackDumpRec(lua_State * pLuaState, int stackIndex) {
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
	_luaStackDumpRec(pLuaState, stackIndex + 1);
}
#endif
