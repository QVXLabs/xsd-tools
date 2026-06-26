/*
 * IgnoredNode.hpp
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

#ifndef IGNOREDNODE_HPP_
#define IGNOREDNODE_HPP_
#ifndef TIXML_USE_STL
#	define TIXML_USE_STL
#endif /* TIXML_USE_STL */
#include <tinyxml.h>
#include "./src/XSDParser/Elements/Node.hpp"
namespace XSD {
	namespace Elements {
		/* A no-op Node for valid XSD constructs that are irrelevant to
		   marshalling codegen — identity constraints (key/keyref/unique/
		   selector/field), anyAttribute, redefine, notation. The factory maps
		   those tags here so the strict parser tolerates (rather than throws on)
		   real-world schemas that merely contain them. It contributes nothing to
		   the model and does not recurse into its children. */
		class IgnoredNode : public Node {
		private:
			IgnoredNode();
		public:
			IgnoredNode(const TiXmlElement& elm, const Parser& rParser);
			IgnoredNode(const IgnoredNode& cpy);
			void ParseChildren(BaseProcessor& rProcessor) const noexcept(false);
			void ParseElement(BaseProcessor& rProcessor) const noexcept(false);
		};
	}	/* namespace Elements */
}	/* namespace XSD */

#endif /* IGNOREDNODE_HPP_ */
