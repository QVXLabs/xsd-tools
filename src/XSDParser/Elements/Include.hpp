/*
 * Include.hpp
 *
 *  Created on: Aug 10, 2011
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

#ifndef INCLUDE_HPP_
#define INCLUDE_HPP_
#ifndef TIXML_USE_STL
#	define TIXML_USE_STL
#endif /* TIXML_USE_STL */
#include <string>
#include <tinyxml.h>
#include "./src/XSDParser/Elements/Node.hpp"
#include "./src/XSDParser/Elements/Schema.hpp"

namespace XSD {
	namespace Elements {
		class Include : public Node {
			XSD_ELEMENT_TAG("include")
		private:
			mutable Schema*	m_pSchema;
			Include();
			std::string schemaURI_() const noexcept(false);;
			static std::string extractURIPath_(const std::string& uri);
			static bool isFileURI_(const std::string& uri);
			static std::string extractQuery_(const std::string& uri);
		public:
			Include(const TiXmlElement& elm, const Parser& rParser);
			Include(const Include& elm);
			virtual ~Include();
			void ParseChildren(BaseProcessor& rProcessor) const noexcept(false);;
			void ParseElement(BaseProcessor& rProcessor) const noexcept(false);;
			const Schema* QuerySchema() const noexcept(false);;
			bool HasSchema() const;
		};
	}	/* namespace Elements */
}	/* namespace XSD */

#endif /* INCLUDE_HPP_ */
