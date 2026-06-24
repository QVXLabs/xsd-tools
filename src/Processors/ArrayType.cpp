/*
 * ArrayType.cpp
 *
 *  Created on: 01/23/12
 *      Author: QVXLabs LLC
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

#include <string>
#include "./src/Processors/ArrayType.hpp" 

using namespace std;
using namespace Processors;
using namespace Processors::Types;

ArrayType::ArrayType(const XSD::Types::BaseType& rType) 
	:m_pBaseType(rType.clone()), m_name("list(") { 
	m_name += rType.Name();
	m_name += ")";
}

/* virtual */
ArrayType::~ArrayType() {
	delete m_pBaseType;
}

/* virtual */ XSD::Types::BaseType* 
ArrayType::clone() const {
	return new ArrayType(*m_pBaseType);
}

/*virtual */ bool 
ArrayType::isTypeRelated(const BaseType* pType) const {
	/* check if pType is the same type as this */
	if (XSD_ISTYPE(pType, ArrayType)) {
		if (XSD_ISTYPE(m_pBaseType, *pType))
			return true;
	}
	return false;
}

/* virtual */ const char*
Types::ArrayType::Name() const {
	return m_name.c_str();
}

const XSD::Types::BaseType&
Types::ArrayType::Type() const {
	return *m_pBaseType;
}
