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

-- requires not needed because build concats lua chunks
--require 'src/Processors/parser'
--require 'src/Processors/stringbuffer'

local loadTemplate = function(fileName)
	local file, err = io.open(fileName, "r")
	if not file then
		error("cannot open template '" .. fileName .. "': "
			.. tostring(err), 0)
	end
	local template = file:read("*all")
	file:close()
	return template
end
main = function(template)
	_G.template = { 
		load = loadTemplate, 
		path = template:match('.*/')..'?',
	}
	local prsr = parser:new()
	local out, _ = prsr:parse(_G.template.load(template))
	io.write(out)
	io.flush()
end