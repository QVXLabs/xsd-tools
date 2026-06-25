/*
 * FacetNode.hpp
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

#ifndef FACETNODE_HPP_
#define FACETNODE_HPP_

#ifndef TIXML_USE_STL
#	define TIXML_USE_STL
#endif /* TIXML_USE_STL */
#include <memory>
#include <tinyxml.h>
#include "./src/XSDParser/Elements/Node.hpp"
#include "./src/XSDParser/Elements/Annotation.hpp"

namespace XSD {
	namespace Elements {
		/* CRTP base shared by the leaf facet classes. Holds the parts that
		 * are byte-identical across all facets; subclasses keep only their tag,
		 * ParseElement dispatch, and any value-type override. ValueT selects
		 * the numeric Value() return type. */
		template<typename Derived, typename ValueT>
		class FacetNode : public Node {
		protected:
			FacetNode(const TiXmlElement& elm, const Parser& rParser)
				: Node(elm, rParser)
			{ }
			FacetNode(const Derived& cpy)
				: Node(cpy)
			{ }
		public:
			void ParseChildren(BaseProcessor& rProcessor)
					const noexcept(false) {
				/* no children allowed */
				std::unique_ptr<Node> pNode(Node::FirstChild());
				if (NULL != pNode.get()) {
					if (XSD_ISELEMENT(pNode.get(), Annotation))
						pNode->ParseElement(rProcessor);
					else
						throw XMLException(pNode->GetXMLElm(),
							XMLException::InvallidChildXMLElement);
				}
			}
			ValueT Value() const noexcept(false) {
				return Node::GetAttribute<ValueT>("value");
			}
			XSD_HAS_ATTR(HasValue, "value")
		};
	}	/* namespace Elements */
}	/* namespace XSD */

#endif /* FACETNODE_HPP_ */
