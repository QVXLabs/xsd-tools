/*
 * Union.cpp
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

#ifndef TIXML_USE_STL
#	define TIXML_USE_STL
#endif /* TIXML_USE_STL */
#include <string>
#include <tinyxml.h>
#include <boost/tokenizer.hpp>
#include "./src/XSDParser/Elements/Union.hpp"
#include "./src/XSDParser/Elements/SimpleType.hpp"
#include "./src/XSDParser/Elements/Annotation.hpp"

using namespace XSD;
using namespace XSD::Elements;

typedef boost::char_separator<std::string::value_type> SpaceCommaSeperator;
typedef boost::tokenizer<SpaceCommaSeperator> SpaceCommaTokenizer;

Union::Union(const TiXmlElement& elm, const Parser& rParser)
	: Node(elm, rParser)
{ }

Union::Union(const Union& rCpy)
	: Node(rCpy)
{ }

void
Union::ParseChildren(BaseProcessor& rProcessor) const noexcept(false) {
	/* process children */
	eachChild_([&rProcessor](const Node& rNode) {
		if (XSD_ISELEMENT(&rNode, SimpleType) ||
			XSD_ISELEMENT(&rNode, Annotation)) {
			rNode.ParseElement(rProcessor);
		} else
			throw XMLException(rNode.GetXMLElm(), XMLException::InvallidChildXMLElement);
	});
}

void
Union::ParseElement(BaseProcessor& rProcessor) const noexcept(false) {
	rProcessor.ProcessUnion(this);
}

Types::BaseType * 
Union::GetParentType() const noexcept(false) {
	return new Types::String();
}

Union::TypeLst*
Union::MemberTypes() const noexcept(false) {
	Union::TypeLst* retLst	= new TypeLst();
	/* tokenize each item in the list */
	const std::string typeLst(this->GetAttribute<const char*>("memberTypes"));
	SpaceCommaSeperator sep(", ");
	SpaceCommaTokenizer tokens(typeLst, sep);
	for (SpaceCommaTokenizer::iterator itr = tokens.begin();
		 itr != tokens.end();
		 ++itr) {
		const std::string& token = *itr;
		retLst->push_back(Node::LookupType(token.c_str()));
	}
	return retLst;
}

bool
Union::HasMemberTypes() const {
	return Node::HasAttribute("memberTypes");
}
