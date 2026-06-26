/*
 * luaScriptAdapter.cpp
 *
 *  Created on: 01/27/12
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
 
#include <string>
#include <sstream>
#include <exception>
#include <lua.hpp>
#include "./src/luaScriptAdapter.hpp"
#include "./src/util.hpp"

using namespace std;
using namespace Core;

LuaException::LuaException(const std::string& rMsg) {
	stringstream msgStrm(ios_base::out|ios_base::in);
	msgStrm << rMsg << endl;
	errorMsg_ = msgStrm.str();
}

LuaException::LuaException(const std::string& rMsg, int err_number) { 
	stringstream msgStrm(ios_base::out|ios_base::in);
	msgStrm << rMsg << "(" << err_number << ")" << endl;
	errorMsg_ = msgStrm.str();
}

/* virtual */
LuaException::~LuaException() noexcept
{ }

/* virtual */ const char*
LuaException::what() const throw() {
	return errorMsg_.c_str();
}

LuaScriptAdapter::LuaScriptAdapter(lua_State* pLuaState)
	: pLuaState_(pLuaState)
{ }

/* virtual */
LuaScriptAdapter::~LuaScriptAdapter()
{ }

void
LuaScriptAdapter::Open() noexcept {
	/* load std lua libs */
	luaL_openlibs(pLuaState_);
	/* add custom helper hash function */
	lua_pushcfunction(pLuaState_, LuaScriptAdapter::luaSDBMHash);
	lua_setglobal(pLuaState_, "sdbm_hash");
}

bool
LuaScriptAdapter::ParseCommandLineArgs(const char* pArgv[], int nArgs) {
	/* verify that there are an odd number of command line args
	 * 3 for exe-name/tempalate/schema & 2 for every additional option (flag/value) */
	if (0 == (nArgs & 0x1)) {
		return true;
	}
	/* create a lua table & parse additional template command line args */
	lua_newtable(pLuaState_);
	for (int ndx = 3; ndx < nArgs; ndx+=2) {
		lua_pushstring(pLuaState_, pArgv[ndx + 1]);
		lua_setfield(pLuaState_, -2, pArgv[ndx]);
	}
	/* add argument table to globals */
	lua_setglobal(pLuaState_, "__CMD_ARGS__");
	return false;
}

void
LuaScriptAdapter::Load(const uint8_t* pBuf, size_t bufSz) noexcept(false) {
	int err = luaL_loadbuffer(pLuaState_, (const char*)pBuf, bufSz, "TemplateEngine");
	switch (err) {
	case 0: break; /* no error */
	case LUA_ERRSYNTAX: {
		const char* pDetail = lua_tostring(pLuaState_, -1);
		throw LuaException(pDetail ? pDetail
			: "Syntax Error Loading TemplateEngine", err);
		break;
	}
	case LUA_ERRMEM:
		throw LuaException("Memory Error Loading TemplateEngine", err);
		break;
	default: 
		throw LuaException("Lua Error Loading TemplateEngine", err);
		break;
	}
}

void
LuaScriptAdapter::Execute(const std::string& templateName) noexcept(false) {
	int err = 0;
	/* force a run of the script to load global symbols */
	if (0 != (err = lua_pcall(pLuaState_, 0, 0, 0)))
		goto LuaScriptAdapterExecute_error;
	/* execute main function */
	lua_getglobal(pLuaState_, "main");
	lua_pushstring(pLuaState_, templateName.c_str());
	if (0 != (err = lua_pcall(pLuaState_, 1, 0, 0)))
		goto LuaScriptAdapterExecute_error;
	return;
LuaScriptAdapterExecute_error:
	switch(err) {
	case 0: break; /* no error */
	case LUA_ERRMEM:
		throw LuaException("Memory Error Executing TemplateEngine", err);
		break;
	case LUA_ERRERR:
		throw LuaException("Error running lua error handler", err);
		break;
	default: {
		/* lua_tostring returns NULL when the error object isn't a string (e.g.
		   `error({})`); don't pass NULL to the LuaException(std::string) ctor. */
		const char* pMsg = lua_tostring(pLuaState_, -1);
		throw LuaException(pMsg ? pMsg : "non-string Lua error", err);
		break;
	}
	}
}

void
LuaScriptAdapter::Close() noexcept {
	lua_close(pLuaState_);
}

void 
LuaScriptAdapter::SetSchemaName(const std::string& schemaName) noexcept {
	lua_pushstring(pLuaState_, (Util::StripFileExtension(Util::ExtractResourceName(schemaName)) + "_xsd").c_str());
	lua_setglobal(pLuaState_, "__SCHEMA_NAME__");
}

lua_State*
LuaScriptAdapter::LuaState() noexcept {
	return pLuaState_;
}

/* static */ int
LuaScriptAdapter::luaSDBMHash(lua_State* pLuaState) {
	const char* pStr = lua_tostring(pLuaState, -1);
	/* push an integer number with the lua-number patch is enabled */
#if defined(LUA_INTEGER)
	lua_pushinteger(pLuaState, Util::SDBMHash(std::string(pStr)));
#else	
	lua_pushnumber(pLuaState, Util::SDBMHash(std::string(pStr)));
#endif
	return 1;
}
