/*
 * luaScriptAdapter.hpp
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
 *  along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#ifndef LUASCRIPTADAPTER_HPP_
#define LUASCRIPTADAPTER_HPP_
#include <string>
#include <exception>
#include <stdint.h>
#include <lua.hpp>

namespace Core {
	class LuaException : public std::exception {
	public:
		LuaException(const std::string& rMsg);
		LuaException(const std::string& rMsg, int err_number);
		virtual ~LuaException() noexcept;
		virtual const char* what() const noexcept;
	private:
		std::string	m_errorMsg;
	};
	class LuaScriptAdapter {
	public:
		LuaScriptAdapter(lua_State* pLuaState);
		virtual ~LuaScriptAdapter();
		void Open() noexcept;
		/* returns true on error */
		bool ParseCommandLineArgs(const char* pArgv[], int nArgs);
		void Load(const uint8_t* pBuf, size_t bufSz) noexcept(false);
		void Execute(const std::string& templateName) noexcept(false);
		void Close() noexcept;
		void SetSchemaName(const std::string& schemaName) noexcept;
		lua_State* LuaState() noexcept;
	private:
		lua_State*	m_pLuaState;
		LuaScriptAdapter();
		LuaScriptAdapter(const LuaScriptAdapter&);
		static int luaSDBMHash(lua_State*);
	};
}
#endif /* LUASCRIPTADAPTER_HPP_ */
