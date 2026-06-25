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

/* The two XSD-element-backed wrapper types share identical bodies (only the
 * wrapper type name differs). isTypeRelated checks identity then walks the
 * value's parent type. */
#define WRAPPED_TYPE_DEFN(TYPE) \
	/* virtual */ BaseType* \
	TYPE::clone() const { \
		return new TYPE(this->pValue_); \
	} \
	/* virtual */ bool \
	TYPE::isTypeRelated(const BaseType* pType) const { \
		if (typeid(*pType) == typeid(TYPE)) { \
			const TYPE* pCmp = static_cast<const TYPE*>(pType); \
			if (*pValue_ == *pCmp->pValue_) \
				return true; \
		} \
		std::unique_ptr<BaseType> pBaseType(pValue_->GetParentType()); \
		if (NULL == pBaseType.get()) \
			return false; \
		else \
			return pBaseType->isTypeRelated(pType); \
	} \
	/* virtual */ const char* \
	TYPE::Name() const { \
		name_ = (pValue_->HasName()) ? pValue_->Name() : "Unnamed"; \
		return name_.c_str(); \
	} \
	/* virtual */ \
	TYPE::~TYPE() { \
		delete pValue_; \
	}

WRAPPED_TYPE_DEFN(SimpleType)
WRAPPED_TYPE_DEFN(ComplexType)
#undef WRAPPED_TYPE_DEFN
