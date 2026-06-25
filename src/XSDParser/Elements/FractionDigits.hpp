/*
 * FractionDigits.hpp
 *
 *  Created on: Dec 30, 2011
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

#ifndef FRACTIONDIGITS_HPP_
#define FRACTIONDIGITS_HPP_

#ifndef TIXML_USE_STL
#	define TIXML_USE_STL
#endif /* TIXML_USE_STL */
#include <tinyxml.h>
#include "./src/XSDParser/Elements/Node.hpp"
#include "./src/XSDParser/Elements/FacetNode.hpp"
namespace XSD {
	namespace Elements {
		class FractionDigits
				: public FacetNode<FractionDigits, uint64_t> {
			XSD_ELEMENT_TAG("fractionDigits")
		private:
			FractionDigits();
		public:
			FractionDigits(const TiXmlElement& elm, const Parser& rParser);
			FractionDigits(const FractionDigits& cpy);
			void ParseElement(BaseProcessor& rProcessor) const noexcept(false);
		};
	}	/* namespace Elements */
}	/* namespace XSD */


#endif /* FRACTIONDIGITS_HPP_ */
