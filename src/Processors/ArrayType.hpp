/*
 * ArrayType.hpp
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
 *  along with Xsd-Tools.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#ifndef ARRAYTYPE_HPP_
#define ARRAYTYPE_HPP_

#include <string>
#include "./src/XSDParser/Types.hpp"

namespace Processors {
	namespace Types {
		class ArrayType : public XSD::Types::BaseType {
		public:
			ArrayType(const XSD::Types::BaseType& rType);
			virtual ~ArrayType();
			virtual XSD::Types::BaseType* clone() const;
			virtual bool isTypeRelated(const BaseType* pType) const;
			virtual const char* Name() const;
			const XSD::Types::BaseType& Type() const;
		private:
			XSD::Types::BaseType* m_pBaseType;
			std::string m_name;
		};
	}
}
#endif /* ARRAYTYPE_HPP_ */