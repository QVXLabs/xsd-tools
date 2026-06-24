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


--require not needed because build concats lua chunks
--require 'src/Processors/safeEnv' 

parser = {
	__index = function(tbl, key)
		return parser[key]
	end
}

function parser:new(obj)
	local _os = {
		time = _G.os.time,
		date = _G.os.date,
		difftime = _G.os.difftime,
		setlocale = _G.os.setlocale,
		time = _G.os.time,
		exit = _G.os.exit,
	}
	obj = obj or { prvtEnv = safeEnv:new({
		_VERSION=_G._VERSION,
		tostring=_G.tostring, 
		tonumber=_G.tonumber,
		type=_G.type,
		select=_G.select,
		pairs=_G.pairs,
		ipairs=_G.ipairs,
		unpack=_G.unpack,
		next=_G.next,
		setmetatable=_G.setmetatable,
		getmetatable=_G.getmetatable,
		rawget=_G.rawget,
		rawset=_G.rawset,
		table=_G.table,
		string=_G.string,
		math=_G.math, 
		os=_os,
		stringBuffer=_G.stringBuffer, 
		schema=_G.schema,
		dbgPrint=_G.print,
		sdbm_hash=_G.sdbm_hash,
		__SCHEMA_NAME__=_G.__SCHEMA_NAME__,
		__CMD_ARGS__=_G.__CMD_ARGS__,
		include= (function(file) return obj:include(file) end)
	})}
	setmetatable(obj, self)
	self.__index = self
	return obj
end

function parser:parse(str)
	return str:gsub("(.?)(%[@lua.*)", function(s1, s2)
			s1 = s1 or ""
			local luaBlk, remStr = s2:match("(%b[])(.*)")
			local msg = luaBlk
			if luaBlk then
				if s1 ~= '\\' then
					_, msg = self.prvtEnv:invoke(luaBlk:match("%[@lua(.*)%]$"))
				end
				return s1..(msg or "")..self:parse(remStr)
			end
			return s1..remStr
		end)
end

function parser:include(str)
	return self:parse(template.load(template.path:gsub('?', str)))
end