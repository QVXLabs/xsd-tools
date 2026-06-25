/*
 * Import.hpp
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

#ifndef IMPORT_HPP_
#define IMPORT_HPP_
#ifndef TIXML_USE_STL
#	define TIXML_USE_STL
#endif /* TIXML_USE_STL */
#include <string>
#include <tinyxml.h>
#include "./src/XSDParser/Elements/Node.hpp"
#include "./src/XSDParser/Elements/Schema.hpp"

namespace XSD {
	namespace Elements {
		/* xs:import brings in a DIFFERENT-namespace schema; unlike xs:include
		 * (a same-namespace merge) the loaded document keeps its own target
		 * namespace, registered against the importing Parser. */
		class Import : public Node {
			XSD_ELEMENT_TAG("import")
		private:
			mutable Schema*	pSchema_;
			Import();
		public:
			Import(const TiXmlElement& elm, const Parser& rParser);
			Import(const Import& elm);
			virtual ~Import();
			void ParseChildren(BaseProcessor& rProcessor) const noexcept(false);
			void ParseElement(BaseProcessor& rProcessor) const noexcept(false);
			/* The imported document's schema; NULL if no schemaLocation. */
			const Schema* QuerySchema() const noexcept(false);
			/* The declared import namespace attr value ("" if absent). */
			std::string ImportNamespace() const noexcept;
			XSD_HAS_ATTR(HasNamespace, "namespace")
			XSD_HAS_ATTR(HasSchema, "schemaLocation")
		};
	}	/* namespace Elements */
}	/* namespace XSD */

#endif /* IMPORT_HPP_ */
