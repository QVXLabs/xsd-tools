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

function table:map(func)
	for k,v in pairs(self) do
		func(k, v)
	end
end

function table:merge(t)
	local ret = {unpack(self)}
	table.map(t, (function(k,v)table.insert(ret,v)end))
	return ret
end

function table:filter(func)
	local ret = {}
	for k, v in pairs(self) do
		ret = table.merge(ret, { func(k,v) })
	end
	return ret
end

function table:find(func)
	for k,v in pairs(self) do
		if func(k, v) then return k, v end
	end
end

function table:isEmpty()
	return (nil == next(self))
end