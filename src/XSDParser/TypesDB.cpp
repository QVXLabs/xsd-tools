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
 *  along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "./src/XSDParser/TypesDB.hpp"

using namespace XSD::Types;

TypesDB::TypesDB() : m_typeTbl() {
	/* fill out type table */
	m_typeTbl["anyURI"] 			= new AnyURI();
	m_typeTbl["base64Binary"]		= new Base64Binary();
	m_typeTbl["boolean"]			= new Boolean();
	m_typeTbl["date"]				= new Date();
	m_typeTbl["dateTime"]			= new DateTime();
	m_typeTbl["dateTimeStamp"]		= new DateTimeStamp();
	m_typeTbl["decimal"]			= new Decimal();
	m_typeTbl["integer"]			= new Integer();
	m_typeTbl["long"]				= new Long();
	m_typeTbl["int"]				= new Int();
	m_typeTbl["short"]				= new Short();
	m_typeTbl["byte"]				= new Byte();
	m_typeTbl["nonNegativeInteger"]	= new NonNegativeInteger();
	m_typeTbl["positiveInteger"]	= new PositiveInteger();
	m_typeTbl["unsignedLong"]		= new UnsignedLong();
	m_typeTbl["unsignedInt"]		= new UnsignedInt();
	m_typeTbl["unsignedShort"]		= new UnsignedShort();
	m_typeTbl["unsignedByte"]		= new UnsignedByte();
	m_typeTbl["nonPositiveInteger"]	= new NonPositiveInteger();
	m_typeTbl["negativeInteger"] 	= new NegativeInteger();
	m_typeTbl["double"] 			= new Double();
	m_typeTbl["duration"] 			= new Duration();
	m_typeTbl["dayTimeDuration"] 	= new DayTimeDuration();
	m_typeTbl["yearMonthDuration"]	= new YearMonthDuration();
	m_typeTbl["float"] 				= new Float();
	m_typeTbl["gDay"] 				= new gDay();
	m_typeTbl["gMonth"] 			= new gMonth();
	m_typeTbl["gMonthDay"] 			= new gMonthDay();
	m_typeTbl["gYear"] 				= new gYear();
	m_typeTbl["gYearMonth"] 		= new gYearMonth();
	m_typeTbl["hexBinary"] 			= new HexBinary();
	m_typeTbl["NOTATION"] 			= new Notation();
	m_typeTbl["precisionDecimal"] 	= new PrecisionDecimal();
	m_typeTbl["QName"]			 	= new QName();
	m_typeTbl["string"]			 	= new String();
	m_typeTbl["normalizedString"]	= new NormalizedString();
	m_typeTbl["token"]				= new Token();
	m_typeTbl["language"]			= new Language();
	m_typeTbl["Name"]				= new XSDName();
	m_typeTbl["NCName"]				= new NCName();
	m_typeTbl["ENTITY"]				= new Entity();
	m_typeTbl["ID"]					= new ID();
	m_typeTbl["IDREF"]				= new IDRef();
	m_typeTbl["NMTOKEN"]			= new NMToken();
	m_typeTbl["time"]				= new Time();
	m_typeTbl["ENTITIES"]			= new Entities();
	m_typeTbl["IDREFS"]				= new IDRefs();
	m_typeTbl["NMTOKENS"]			= new NMTokens();
}

/* virtual*/
TypesDB::~TypesDB() {
	/* release type table */
	for (TypeTbl::iterator itr = m_typeTbl.begin(); itr != m_typeTbl.end(); ++itr) {
		delete (*itr).second;
	}
}

BaseType*
TypesDB::FindType(const char* pTypeName) const throw() {
	TypeTbl::const_iterator itr = m_typeTbl.find(pTypeName);
	if (itr != m_typeTbl.end())
		return itr->second->clone();
	else
		return new Unknown();
}

BaseType*
TypesDB::FindType(const std::string& rTypeName) const throw() {
	return FindType(rTypeName.c_str());
}
