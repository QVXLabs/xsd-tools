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
  along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
]]--

stringBuffer = {
	__index = function(tbl, key)
		return stringBuffer[key]
	end,
	__concat = function(sb1, sb2)
		local ret = stringBuffer:new()
		local buf = ret._buf
		for _, v in pairs(sb1._buf) do
			buf[#buf+1] = v
		end
		for _, v in pairs(sb2._buf) do
			buf[#buf+1] = v
		end
		return ret
	end,
	__tostring = function(sb)
		return table.concat(sb._buf)
	end,
	__eq = function(sb1, sb2)
		local max = (function() if #sb1._buf < #sb2._buf then return #sb1._buf else return #sb2._buf end end)()
		for i = 1, max, 1 do
			if sb1[i] ~= sb2[i] then return false end
		end
		return true
	end
}

function stringBuffer:new(obj)
	local retObj = {_buf ={}}
	if "string" == type(obj) then
		retObj = { _buf = {[1]=obj}}
	elseif "number" == type(obj) then
		retObj = { _buf = {[1]=tostring(obj)}}
	else
		retObj = obj or { _buf = {} }
	end
	setmetatable(retObj, self)
	self.__index = self
	return retObj
end

function stringBuffer:append(str)
	local buf = self._buf
	buf[#buf + 1] = str
end

function stringBuffer:str()
	local buf = self._buf
	return table.concat(buf)
end