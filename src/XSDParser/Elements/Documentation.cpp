/*
 * Documentation.cpp
 *
 *  Created on: Aug 27, 2011
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
#include <memory>
#include <string.h>
#include <string>
#include <tinyxml.h>
#include "./src/XSDParser/Elements/Documentation.hpp"

using namespace XSD;
using namespace XSD::Elements;

Documentation::Documentation(const TiXmlElement& elm, const Parser& rParser)
	: Node(elm, rParser)
{ }

Documentation::Documentation(const Documentation& cpy)
	: Node(cpy)
{ }

void
Documentation::ParseChildren(BaseProcessor& rProcessor) const noexcept(false) {
	/* xs:documentation content is opaque mixed content (it routinely embeds
	   HTML — <div>, <p>, ... — as in xml.xsd / XMLSchema.xsd), so do NOT parse
	   it as XSD. The text is read separately via DocumentationStr(); calling
	   FirstChild() here would feed that markup to the strict node factory and
	   throw. No-op. */
}

void
Documentation::ParseElement(BaseProcessor& rProcessor) const noexcept(false) {
	rProcessor.ProcessDocumentation(this);
}

/* Collect all descendant text, descending into any embedded markup.
   xs:documentation is opaque mixed content and commonly wraps text in HTML
   (<p>, <em>, ...); gather the text rather than throwing on element children. */
static std::string collectText_(const TiXmlNode& rNode) {
	std::string txt;
	for (const TiXmlNode* p = rNode.FirstChild(); NULL != p;
	     p = p->NextSibling()) {
		if (TiXmlNode::TINYXML_TEXT == p->Type())
			txt += p->Value();
		else if (TiXmlNode::TINYXML_ELEMENT == p->Type())
			txt += collectText_(*p);
	}
	return txt;
}

std::string
Documentation::DocumentationStr() const noexcept(false) {
	return collectText_(Node::GetXMLElm());
}
