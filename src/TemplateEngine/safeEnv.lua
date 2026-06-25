--[[
  Copyright: (c)2012 QVXLabs LLC

  This file is part of xsd-tools.

  xsd-tools is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  xsd-tools is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with xsd-tools.  If not, see <http://www.gnu.org/licenses/>.
]]--

safeEnv = {
	__index = function(tbl, key)
		return safeEnv[key]
	end
}

function safeEnv:new(env, obj)
	obj = obj or { _gEnv = env or {} }
	setmetatable(obj, self)
	self.__index = self
	return obj
end

function safeEnv:invoke(code)
	local f, err = loadstring(code)
	if not f then return false, err end
	setfenv(f , self._gEnv)
	return pcall(f)
end