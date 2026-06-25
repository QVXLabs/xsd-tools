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
	:pBaseType_(rType.clone()), name_("list(") { 
	name_ += rType.Name();
	name_ += ")";
}

/* virtual */
ArrayType::~ArrayType() {
	delete pBaseType_;
}

/* virtual */ XSD::Types::BaseType* 
ArrayType::clone() const {
	return new ArrayType(*pBaseType_);
}

/*virtual */ bool 
ArrayType::isTypeRelated(const BaseType* pType) const {
	/* check if pType is the same type as this */
	if (XSD_ISTYPE(pType, ArrayType)) {
		if (XSD_ISTYPE(pBaseType_, *pType))
			return true;
	}
	return false;
}

/* virtual */ const char*
Types::ArrayType::Name() const {
	return name_.c_str();
}

const XSD::Types::BaseType&
Types::ArrayType::Type() const {
	return *pBaseType_;
}
