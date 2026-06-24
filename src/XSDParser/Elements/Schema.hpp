/*
 * Schema.hpp
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

#ifndef SCHEMA_HPP_
#define SCHEMA_HPP_
#ifndef TIXML_USE_STL
#	define TIXML_USE_STL
#endif /* TIXML_USE_STL */
#include <string>
#include <tinyxml.h>
#include "./src/XSDParser/Exception.hpp"
#include "./src/XSDParser/Elements/Node.hpp"

namespace XSD {
	namespace Elements {
		class Schema : public Node {
			XSD_ELEMENT_TAG("schema")
		private:
			std::string		m_documentURI;
			Schema();
			static std::string _extractName(const std::string& uri);			
		public:
			Schema(const TiXmlElement& elm, const Parser& rParser, const std::string& uri);
			Schema( const Schema& elm);
			virtual ~Schema();
			void ParseChildren(BaseProcessor& rProcessor) const noexcept(false);
			void ParseElement(BaseProcessor& rProcessor) const noexcept(false);
			const std::string Name() const noexcept(false);
			const std::string& URI() const noexcept(false);
			const std::string Namespace() const noexcept(false);
			Types::BaseType * GetParentType() const noexcept(false);
			bool isRootSchema() const;
		};
	}	/* namespace Elements */
}	/* namespace XSD */

#endif /* SCHEMA_HPP_ */
