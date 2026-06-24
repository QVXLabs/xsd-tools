/*
 * XSDTypes.cpp
 *
 *  Created on: Jun 25, 2011
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

#include <memory>
#include <boost/foreach.hpp>
#include "./src/XSDParser/Types.hpp"
#include "./src/XSDParser/Elements/SimpleType.hpp"
#include "./src/XSDParser/Elements/ComplexType.hpp"
#include "./src/XSDParser/Elements/Restriction.hpp"
#include "./src/XSDParser/Elements/Union.hpp"
#include "./src/XSDParser/Elements/List.hpp"

using namespace XSD;
using namespace XSD::Types;
/*
 * XSDSimpleTypeType - XSD type object for XSD simple types
 */
/* virtual */ BaseType*
SimpleType::clone() const {
	return new SimpleType(this->m_pValue);
}

/* virtual */ bool
SimpleType::isTypeRelated(const BaseType* pType) const {
	/* check if pType is the same type as this */
	if (XSD_ISTYPE(pType, SimpleType)) {
		const SimpleType *pCmpSimpleType = static_cast<const SimpleType*>(pType);
		if (*m_pValue == *pCmpSimpleType->m_pValue)
				return true;
	}
	/* break down this simpleType to its parent type */
	std::unique_ptr<BaseType> pBaseType(m_pValue->GetParentType());
	if (NULL == pBaseType.get())
		return false;
	else
		return pBaseType->isTypeRelated(pType);
}

/* virtual */ const char*
SimpleType::Name() const {
  m_name = (m_pValue->HasName()) ? m_pValue->Name() : "Unnamed";
  return m_name.c_str();
}

/* virtual */
SimpleType::~SimpleType() {
	delete m_pValue;
}

/*
 * XSDComplexTypeType - XSD type object for XSD complex types
 */
/* virtual */ BaseType*
ComplexType::clone() const {
	return new ComplexType(this->m_pValue);
}

/* virtual */ bool
ComplexType::isTypeRelated(const BaseType* pType) const {
	/* check if pType is the same type as this */
	if (typeid(*pType) == typeid(ComplexType)) {
		const ComplexType *pCmpCmplxType = static_cast<const ComplexType*>(pType);
		if (*m_pValue == *pCmpCmplxType->m_pValue)
				return true;
	}
	/* break down complexType to its parent type */
	std::unique_ptr<BaseType> pBaseType(m_pValue->GetParentType());
	if (NULL == pBaseType.get())
		return false;
	else
		return pBaseType->isTypeRelated(pType);
}

/* virtual */ const char*
ComplexType::Name() const {
  m_name = (m_pValue->HasName())
    ? m_pValue->Name() : "Unnamed";
  return m_name.c_str();
}

/* virtual */
ComplexType::~ComplexType() {
	delete m_pValue;
}
