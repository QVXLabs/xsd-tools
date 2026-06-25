/*
 * XsdQName.hpp
 *
 *  Created on: Jun 25, 2026
 *      Author: QVXLabs LLC
 *   Copyright: (c)2026 QVXLabs LLC
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

#ifndef XSDQNAME_HPP_
#define XSDQNAME_HPP_
#include <string>

namespace XSD {
	/* The XML Schema language namespace URI; builtins live here. Named
	 * XsdQName (not QName) to avoid clashing with the Types::QName builtin
	 * under "using namespace XSD::Types;". */
	constexpr char XSD_NS[] = "http://www.w3.org/2001/XMLSchema";

	struct XsdQName {
		std::string ns;
		std::string local;
		bool operator==(const XsdQName& rOther) const noexcept {
			return ns == rOther.ns && local == rOther.local;
		}
	};
}	/* namespace XSD */

#endif /* XSDQNAME_HPP_ */
