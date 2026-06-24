/*
 * xsdtools.hpp
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
 *  along with xsd-tools.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef XSDTOOLS_HPP_
#define XSDTOOLS_HPP_

#include <iosfwd>
#include <string>
#include <vector>

namespace XsdTools {
	/* Parse `xsdPath`, run `templatePath` through the Lua template engine,
	 * and write the generated code to `out`. `templateArgs` is a flat list
	 * of alternating key/value strings exposed to templates as __CMD_ARGS__.
	 * Throws XSD::XMLException, Core::LuaException, or
	 * Core::ResourceException on failure. */
	void Generate(const std::string& templatePath,
	              const std::string& xsdPath,
	              std::ostream& out,
	              const std::vector<std::string>& templateArgs =
	                  std::vector<std::string>());
}
#endif /* XSDTOOLS_HPP_ */
