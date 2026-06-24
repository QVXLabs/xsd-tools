/*
 * xsdtools.cpp
 *
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
#include <ostream>
#include <string>
#include <vector>
#include <lua.hpp>
#include "./src/xsdtools.hpp"
#include "./src/luaScriptAdapter.hpp"
#include "./src/resource.hpp"
#include "./src/XSDParser/Parser.hpp"
#include "./src/Processors/LuaProcessor.hpp"
#include "./src/XSDParser/Elements/Schema.hpp"

namespace {
	/* io.write replacement: append every string argument to the bound
	 * ostream (passed as a lightuserdata upvalue). The template engine
	 * emits all output through io.write (templateEngine.lua), so this is
	 * the single capture point. */
	int captureWrite(lua_State* pState) {
		std::ostream* pOut = static_cast<std::ostream*>(
			lua_touserdata(pState, lua_upvalueindex(1)));
		int nArgs = lua_gettop(pState);
		for (int ndx = 1; ndx <= nArgs; ++ndx) {
			size_t len = 0;
			const char* pStr = lua_tolstring(pState, ndx, &len);
			if (NULL != pStr && NULL != pOut)
				pOut->write(pStr, static_cast<std::streamsize>(len));
		}
		return 0;
	}

	void installOutputCapture(lua_State* pState, std::ostream& out) {
		lua_getglobal(pState, "io");
		lua_pushlightuserdata(pState, &out);
		lua_pushcclosure(pState, captureWrite, 1);
		lua_setfield(pState, -2, "write");
		lua_pop(pState, 1);
	}

	void setTemplateArgs(lua_State* pState,
	                     const std::vector<std::string>& templateArgs) {
		lua_newtable(pState);
		for (size_t ndx = 0; ndx + 1 < templateArgs.size(); ndx += 2) {
			lua_pushstring(pState, templateArgs[ndx + 1].c_str());
			lua_setfield(pState, -2, templateArgs[ndx].c_str());
		}
		lua_setglobal(pState, "__CMD_ARGS__");
	}
}

namespace XsdTools {
	void Generate(const std::string& templatePath,
	              const std::string& xsdPath,
	              std::ostream& out,
	              const std::vector<std::string>& templateArgs) {
		XSD::Parser              parser;
		XSD::Elements::Schema*   pDocRoot = NULL;
		Core::Resource           resource;
		Core::LuaScriptAdapter   luaScriptAdapter(luaL_newstate());
		Processors::LuaProcessor luaPrcssr(luaScriptAdapter.LuaState());
		size_t                   nginScriptSz = 0;
		const uint8_t*           pNginScript =
			resource.GetEngineScript(&nginScriptSz);
		try {
			/* init lua + redirect generated output to `out` */
			luaScriptAdapter.Open();
			installOutputCapture(luaScriptAdapter.LuaState(), out);
			setTemplateArgs(luaScriptAdapter.LuaState(), templateArgs);
			/* parse xsd document */
			pDocRoot = parser.Parse(xsdPath);
			pDocRoot->ParseElement(luaPrcssr);
			/* execute lua template processing engine */
			luaScriptAdapter.SetSchemaName(xsdPath);
			luaScriptAdapter.Load(pNginScript, nginScriptSz);
			luaScriptAdapter.Execute(resource.GetTemplatePath(templatePath));
		} catch (...) {
			luaScriptAdapter.Close();
			delete pDocRoot;
			throw;
		}
		luaScriptAdapter.Close();
		delete pDocRoot;
	}
}
