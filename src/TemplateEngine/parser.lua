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

-- Fold the template into a stringBuffer, recursing over byte offsets: find the
-- next [@lua marker, balance-match its block from there (%b[] anchored on the
-- marker, so a literal '[' just before it -- e.g. arr[[@lua..]] -- is left
-- intact), and substitute it; a backslash before the marker emits it verbatim.
-- Threading the offset (not the remainder) keeps it O(template length).
function parser:parse(str)
	local function scan(pos, out)
		local s = str:find("[@lua", pos, true)
		if not s then return out:append(str:sub(pos)) end
		out:append(str:sub(pos, s - 1))
		local _, e = str:find("%b[]", s)
		local blk = str:sub(s, e)
		if str:sub(s - 1, s - 1) == '\\' then
			out:append(blk)
		else
			local _, msg = self.prvtEnv:invoke(blk:match("^%[@lua(.*)%]$"))
			out:append(msg or "")
		end
		return scan(e + 1, out)
	end
	return scan(1, stringBuffer:new()):str()
end

function parser:include(str)
	return self:parse(template.load(template.path:gsub('?', str)))
end