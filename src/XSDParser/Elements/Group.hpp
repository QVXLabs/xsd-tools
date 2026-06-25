/*
 * Group.hpp
 *
 *  Created on: Jul 8, 2011
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

#ifndef GROUP_HPP_
#define GROUP_HPP_
#ifndef TIXML_USE_STL
#	define TIXML_USE_STL
#endif /* TIXML_USE_STL */
#include <tinyxml.h>
#include "./src/XSDParser/Elements/Node.hpp"
namespace XSD {
	namespace Elements {
		class Group : public Node {
			XSD_ELEMENT_TAG("group")
		private:
			Group();
		public:
			Group(const TiXmlElement& elm, const Parser& rParser);
			Group(const Group& cpy);
			void ParseChildren(BaseProcessor& rProcessor) const noexcept(false);;
			void ParseElement(BaseProcessor& rProcessor) const noexcept(false);;
			int MaxOccurs() const;
			int MinOccurs() const;
			std::string Name() const noexcept(false);;
			Group* RefGroup() const noexcept(false);;
			XSD_HAS_ATTR(HasMaxOccurs, "maxOccurs")
			XSD_HAS_ATTR(HasMinOccurs, "minOccurs")
			XSD_HAS_ATTR(HasName, "name")
			XSD_HAS_ATTR(HasRef, "ref")
		};
	}	/* namespace Elements */
}	/* namespace XSD */

#endif /* GROUP_HPP_ */
