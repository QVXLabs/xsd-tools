/*
 * AttributeGroup.hpp
 *
 *  Created on: Aug 4, 2011
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

#ifndef ATTRIBUTEGROUP_HPP_
#define ATTRIBUTEGROUP_HPP_
#ifndef TIXML_USE_STL
#	define TIXML_USE_STL
#endif /* TIXML_USE_STL */
#include <tinyxml.h>
#include "./src/XSDParser/Elements/Node.hpp"
namespace XSD {
	namespace Elements {
		class AttributeGroup : public Node {
			XSD_ELEMENT_TAG("attributeGroup")
		private:
			AttributeGroup();
		public:
			AttributeGroup(const TiXmlElement& elm, const Parser& rParser);
			AttributeGroup(const AttributeGroup& cpy);
			void ParseChildren(BaseProcessor& rProcessor) const noexcept(false);;
			void ParseElement(BaseProcessor& rProcessor) const noexcept(false);;
			std::string Name() const noexcept(false);;
			AttributeGroup* RefGroup() const noexcept(false);;
			bool HasName() const;
			bool HasRef() const;
		};
	}	/* namespace Elements */
}	/* namespace XSD */
#endif /* ATTRIBUTEGROUP_HPP_ */
