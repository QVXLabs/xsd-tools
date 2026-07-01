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

-- Offset of the ']' closing the block opened by the '[' at `s`, or nil if
-- unterminated. Tracks Lua lexical state so brackets inside quoted strings,
-- long strings/comments and line comments don't affect bracket depth.
local function blockEnd_(str, s)
	local n = #str
	local i = s + 1
	local depth = 1
	while i <= n do
		local c = str:sub(i, i)
		if c == '"' or c == "'" then
			i = i + 1
			while i <= n and str:sub(i, i) ~= c do
				i = i + (str:sub(i, i) == '\\' and 2 or 1)
			end
			if i > n then return nil end
		elseif c == '[' then
			local eq = str:match('^%[(=*)%[', i)
			if eq then
				local _, e = str:find(']' .. eq .. ']', i + #eq + 2, true)
				if not e then return nil end
				i = e
			else
				depth = depth + 1
			end
		elseif c == '-' and str:sub(i + 1, i + 1) == '-' then
			local eq = str:match('^%[(=*)%[', i + 2)
			if eq then
				local _, e = str:find(']' .. eq .. ']', i + #eq + 4, true)
				if not e then return nil end
				i = e
			else
				i = (str:find('\n', i + 2, true) or n + 1) - 1
			end
		elseif c == ']' then
			depth = depth - 1
			if depth == 0 then return i end
		end
		i = i + 1
	end
	return nil
end

local function lineOf_(str, pos)
	local _, nl = str:sub(1, pos):gsub('\n', '')
	return nl + 1
end

-- Fold the template into a stringBuffer, recursing over byte offsets: find
-- the next [@lua marker, lex forward to its matching close bracket (anchored
-- on the marker, so a literal '[' just before it -- e.g. arr[[@lua..]] -- is
-- left intact), and substitute it; a backslash before the marker emits it
-- verbatim. A block that fails to load or run, or never closes, aborts the
-- fold so no error text leaks into the output. Threading the offset (not the
-- remainder) keeps it O(template length).
function parser:parse(str)
	local function scan(pos, out)
		local s = str:find("[@lua", pos, true)
		if not s then return out:append(str:sub(pos)) end
		out:append(str:sub(pos, s - 1))
		local e = blockEnd_(str, s)
		if not e then
			error(("unterminated [@lua block at line %d")
				:format(lineOf_(str, s)), 0)
		end
		local blk = str:sub(s, e)
		if str:sub(s - 1, s - 1) == '\\' then
			out:append(blk)
		else
			local ok, res = self.prvtEnv:invoke(blk:match("^%[@lua(.*)%]$"))
			if not ok then
				error(("template error at line %d: %s")
					:format(lineOf_(str, s), tostring(res)), 0)
			end
			out:append(res or "")
		end
		return scan(e + 1, out)
	end
	return scan(1, stringBuffer:new()):str()
end

function parser:include(str)
	return self:parse(template.load(template.path:gsub('?', str)))
end