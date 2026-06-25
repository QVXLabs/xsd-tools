/*
 * Import.cpp
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

#include <memory>
#include <unistd.h>
#include "./src/XSDParser/Elements/Import.hpp"
#include "./src/XSDParser/Elements/Element.hpp"
#include "./src/XSDParser/Elements/Schema.hpp"
#include "./src/XSDParser/Elements/Annotation.hpp"
#include "./src/XSDParser/Parser.hpp"

using namespace XSD;
using namespace XSD::Elements;

Import::Import(const TiXmlElement& elm, const Parser& rParser)
	: Node(elm, rParser), pSchema_(NULL)
{ }

Import::Import(const Import& rCpy)
	: Node(rCpy), pSchema_(rCpy.pSchema_)
{ }

/* virtual */
Import::~Import() {
	delete pSchema_;
}

void
Import::ParseChildren(BaseProcessor& rProcessor) const noexcept(false) {
	/* does nothing, doesn't have child elements */
}

void
Import::ParseElement(BaseProcessor& rProcessor) const noexcept(false) {
	rProcessor.ProcessImport(this);
}

const Schema*
Import::QuerySchema() const noexcept(false) {
	if (NULL == pSchema_ && HasSchema()) {
		std::string uri = resolveSchemaURI_("schemaLocation");
		pSchema_ = Node::GetParser().Parse(uri);
		/* index the imported namespace so cross-doc refs resolve by URI */
		Node::GetParser().RegisterNamespace(pSchema_->TargetNamespace(), uri);
	}
	return pSchema_;
}

std::string
Import::ImportNamespace() const noexcept {
	return HasNamespace()
		? std::string(GetXMLElm().Attribute("namespace")) : std::string("");
}
