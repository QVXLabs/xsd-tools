/*
 * util.c
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
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sstream>
#include "util.hpp"

using namespace std;

uint32_t 
Util::SDBMHash(const string& string) {
	string::const_iterator itr = string.begin();
	uint32_t hash = 0;
	uint32_t c;
	while (0 != (c = *itr++) )
		hash = c + (hash << 6) + (hash << 16) - hash;
	return hash;
}

std::string
Util::ExtractResourceName(const std::string& uri) {
	const size_t queryNdx	= uri.find("?");
	const size_t resNdx		= uri.rfind("/", queryNdx);
	return uri.substr(resNdx + 1, (std::string::npos != queryNdx) ? (queryNdx - resNdx) - 1 : queryNdx);
}

std::string
Util::StripFileExtension(const std::string& filename) {
	return filename.substr(0, filename.rfind("."));
}