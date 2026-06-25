/*
 * TypesDB.cpp
 *
 *  Created on: Jun 27, 2011
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
 *  along with xsd-tools.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "./src/XSDParser/TypesDB.hpp"

using namespace XSD::Types;

TypesDB::TypesDB() : typeTbl_() {
	/* Register every builtin from the shared XSD_BUILTIN_TYPES list,
	 * keyed by its lookup name. Unsupported/Unknown are sentinels and
	 * intentionally not registered. */
#define XSD_REGISTER_TYPE(TYPE,BASE,CTYPE,NAME,KEY)	\
	typeTbl_[KEY] = new TYPE();
	XSD_BUILTIN_TYPES(XSD_REGISTER_TYPE)
#undef XSD_REGISTER_TYPE
}

/* virtual*/
TypesDB::~TypesDB() {
	/* release type table */
	for (TypeTbl::iterator itr = typeTbl_.begin(); itr != typeTbl_.end(); ++itr) {
		delete (*itr).second;
	}
}

BaseType*
TypesDB::FindType(const char* pTypeName) const throw() {
	TypeTbl::const_iterator itr = typeTbl_.find(pTypeName);
	if (itr != typeTbl_.end())
		return itr->second->clone();
	else
		return new Unknown();
}

BaseType*
TypesDB::FindType(const std::string& rTypeName) const throw() {
	return FindType(rTypeName.c_str());
}
