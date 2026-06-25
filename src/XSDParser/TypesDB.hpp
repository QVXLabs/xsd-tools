/*
 * TypesDB.hpp
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

#include <string.h>
#include <string>
#include <map>
#include "./src/XSDParser/Types.hpp"

#ifndef TYPESDB_HPP_
#define TYPESDB_HPP_
namespace XSD {
	namespace Types {
		class TypesDB {
		private:
			//struct StrCp { bool operator()(const char* p1, const char* p2) const { return (strcmp(p1,p2) < 0); } };
			//typedef std::map<const char*, const BaseType*, StrCp> TypeTbl;
			typedef std::map<std::string, const BaseType*> TypeTbl;
			TypeTbl	typeTbl_;
		public:
			TypesDB();
			virtual~ TypesDB();
			BaseType* FindType(const char* pTypeName ) const throw();
			BaseType* FindType(const std::string &rTypeName ) const throw();
		};
	}	/* namespace Types */
}	/* namespace XSD */
#endif /* TYPESDB_HPP_ */
