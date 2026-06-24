/*
 * Restriction.hpp
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

#ifndef RESTRICTION_HPP_
#define RESTRICTION_HPP_
#ifndef TIXML_USE_STL
#	define TIXML_USE_STL
#endif /* TIXML_USE_STL */
#include <tinyxml.h>
#include "./src/XSDParser/Elements/Node.hpp"

namespace XSD {
	namespace Elements {
		class Restriction : public Node {
			XSD_ELEMENT_TAG("restriction")
		private:
			Restriction();
			// bool _isElmRelated(const Node* pRstrctn, const TiXmlElement* pBase) const noexcept(false);
			//static const TiXmlElement* _findElm(const TiXmlElement* pTreeBase, const TiXmlElement* pNode) noexcept(false);
		public:
			Restriction(const TiXmlElement& elm, const Parser& rParser);
			Restriction(const Restriction& rCpy);
			void ParseChildren(BaseProcessor& rProcessor) const noexcept(false);
			void ParseElement(BaseProcessor& rProcessor) const noexcept(false);
			Types::BaseType * GetParentType() const noexcept(false);
			bool isParentComplexContent() const noexcept(false);
			bool isParentSimpleContent() const noexcept(false);
			Types::BaseType* Base() const noexcept(false);
		};
	}	/* namespace Elements */
}	/* namespace XSD */
#endif /* RESTRICTION_HPP_ */
