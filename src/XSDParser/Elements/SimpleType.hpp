/*
 * SimpleType.hpp
 *
 *  Created on: Jun 26, 2011
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

#ifndef SIMPLETYPE_HPP_
#define SIMPLETYPE_HPP_
#ifndef TIXML_USE_STL
#	define TIXML_USE_STL
#endif /* TIXML_USE_STL */
#include <string>
#include <tinyxml.h>
#include "./src/XSDParser/Exception.hpp"
#include "./src/XSDParser/Elements/Node.hpp"
#include "./src/XSDParser/Elements/Union.hpp"
#include "./src/XSDParser/Elements/Restriction.hpp"
#include "./src/XSDParser/Elements/List.hpp"

namespace XSD {
	namespace Elements {
		class SimpleType : public Node {
			XSD_ELEMENT_TAG("simpleType")
		private:
			SimpleType();
		public:
			SimpleType(const TiXmlElement& elm, const Parser& rParser);
			SimpleType(const SimpleType& rType);
			void ParseChildren(BaseProcessor& rProcessor) const noexcept(false);;
			void ParseElement(BaseProcessor& rProcessor) const noexcept(false);;
			Types::BaseType * GetParentType(void) const noexcept(false);;
			/* the restriction child, or NULL when this simpleType has none */
			Restriction * GetRestriction() const noexcept;
			std::string Name() const noexcept(false);;
			bool HasName() const;
			bool HasUnion() const;
			bool HasRestriction() const;
			bool HasList() const;
		};
	}	/* namespace Elements */
}	/* namespace XSD */
#endif /* SIMPLETYPE_HPP_ */
