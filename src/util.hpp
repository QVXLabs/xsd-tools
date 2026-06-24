/*
 * util.hpp
 *
 *  Created on: May 18, 2011
 *      Author: QVXLabs LLC
 *   Copyright: (c)2011 QVXLabs LLC
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

#ifndef UTIL_HPP_
#define UTIL_HPP_
#include <inttypes.h>
#include <string>
namespace Util {
uint32_t 	SDBMHash(const std::string& string);
std::string	ExtractResourceName(const std::string& uri);
std::string	StripFileExtension(const std::string& filename);
}
#endif /* UTIL_HPP_ */
