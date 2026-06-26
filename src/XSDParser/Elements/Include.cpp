/*
 * Include.cpp
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

#include <memory>
#include "./src/XSDParser/Elements/Include.hpp"
#include "./src/XSDParser/Elements/Element.hpp"
#include "./src/XSDParser/Elements/Schema.hpp"
#include "./src/XSDParser/Elements/Annotation.hpp"
#include "./src/XSDParser/Parser.hpp"

using namespace XSD;
using namespace XSD::Elements;

Include::Include(const TiXmlElement& elm, const Parser& rParser)
	: Node(elm, rParser), pSchema_(NULL)
{ }

Include::Include(const Include& rCpy)
	: Node(rCpy), pSchema_(rCpy.pSchema_)
{ }

/* virtual */
Include::~Include() {
	delete pSchema_;
}

void
Include::ParseChildren(BaseProcessor& rProcessor) const noexcept(false) {
	/* does nothing, doesn't have child elements */
}

void
Include::ParseElement(BaseProcessor& rProcessor) const noexcept(false) {
	rProcessor.ProcessInclude(this);
}

const Schema*
Include::QuerySchema() const noexcept(false) {
	if (NULL == pSchema_) {
		pSchema_ = Node::GetParser().Parse(resolveSchemaURI_("schemaLocation"));
		/* Parse returns NULL for an unsupported/unfetchable location; throw
		   cleanly instead of returning a NULL schema callers dereference. */
		if (NULL == pSchema_)
			throw XMLException(GetXMLElm(), XMLException::ProtocolNotSupported);
	}
	return pSchema_;
}
