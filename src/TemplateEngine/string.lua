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

function string:split(sep)
	sep = sep or ":"
	local fields = {}
	-- escape pattern-magic in sep so it is matched literally inside the class
	local pattern = "([^"..sep:gsub("(%W)", "%%%1").."]+)"
	self:gsub(pattern, function(c) fields[#fields+1] = c end)
	return fields
end