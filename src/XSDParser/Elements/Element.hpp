/*
 * Element.hpp
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

#ifndef ELEMENT_HPP_
#define ELEMENT_HPP_
#ifndef TIXML_USE_STL
#	define TIXML_USE_STL
#endif /* TIXML_USE_STL */
#include <string>
#include <tinyxml.h>
#include "./src/XSDParser/Exception.hpp"
#include "./src/XSDParser/Elements/Node.hpp"

namespace XSD {
	namespace Elements {
		class Element : public Node {
			XSD_ELEMENT_TAG("element")
		private:
			Element();
			Types::BaseType* _Type() const noexcept(false);;
			static Types::BaseType* _ParseType(const Element& rElm) noexcept(false);;
		public:
			Element(const TiXmlElement& elm, const Parser& rParser);
			Element( const Element& elm);
			void ParseChildren(BaseProcessor& rProcessor) const noexcept(false);;
			void ParseElement(BaseProcessor& rProcessor) const noexcept(false);;
			Types::BaseType * GetParentType(void) const noexcept(false);;
			bool Abstract() const;
			std::string Name() const noexcept(false);;
			Element* SubstitutionGroup() const noexcept(false);;
			bool VerifySubstitutionGroup() const noexcept(false);;
			Types::BaseType* Type() const noexcept(false);;
			Element* RefElement() const noexcept(false);;
			int MaxOccurs() const;
			int MinOccurs() const;
			bool HasName() const;
			bool HasSubstitutionGroup() const;
			bool HasRef() const;
			bool HasType() const;
			bool HasChildType() const;
			bool HasMaxOccurs() const;
			bool HasMinOccurs() const;
		};
	}
}
#endif /* ELEMENT_HPP_ */
