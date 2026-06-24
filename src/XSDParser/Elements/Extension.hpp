/*
 * Extension.hpp
 *
 *  Created on: Jul 18, 2011
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
 *  along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EXTENSION_HPP_
#define EXTENSION_HPP_
#ifndef TIXML_USE_STL
#	define TIXML_USE_STL
#endif /* TIXML_USE_STL */
#include <tinyxml.h>
#include "./src/XSDParser/Exception.hpp"
#include "./src/XSDParser/Elements/Node.hpp"

namespace XSD {
	namespace Elements {
		class Extension : public Node {
			XSD_ELEMENT_TAG("extension")
		private:
			Extension();
			static bool _checkForDuplicateNamedParticles(const TiXmlElement* pTreeBase, const TiXmlElement* pBase);
			static bool _find(const char* pName, const TiXmlElement* pBase);
		public:
			Extension(const TiXmlElement& elm, const Parser& rParser);
			Extension(const Extension& rType);
			void ParseChildren(BaseProcessor& rProcessor) const noexcept(false);;
			void ParseElement(BaseProcessor& rProcessor) const noexcept(false);;
			Types::BaseType * GetParentType(void) const noexcept(false);;
			Types::BaseType* Base() const noexcept(false);;
			bool isParentComplex() const noexcept(false);;
		};
	}	/* namespace Elements */
}	/* namespace XSD */

#endif /* EXTENSION_HPP_ */
