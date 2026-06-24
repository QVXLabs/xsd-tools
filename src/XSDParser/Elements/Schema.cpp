/*
 * Document.cpp
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
 *  along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <memory>
#include <tinyxml.h>
#include "./src/util.hpp"
#include "./src/XSDParser/Elements/Schema.hpp"
#include "./src/XSDParser/Elements/Element.hpp"
#include "./src/XSDParser/Elements/Include.hpp"
#include "./src/XSDParser/Elements/Annotation.hpp"

using namespace XSD;
using namespace XSD::Elements;

Schema::Schema(const TiXmlElement& elm, const Parser& rParser, const std::string& name )
	: Node(elm, rParser), m_documentURI(name)
{ }

Schema::Schema(const Schema& rDoc)
	: Node(rDoc), m_documentURI(rDoc.m_documentURI)
{ }

/* virtual */
Schema::~Schema() 
{ }

void
Schema::ParseChildren(BaseProcessor& rProcessor) const noexcept(false) {
	/* process children */
	_eachChild([&rProcessor](const Node& rNode) {
		if (XSD_ISELEMENT(&rNode, Element) ||
			XSD_ISELEMENT(&rNode, Annotation) ||
			XSD_ISELEMENT(&rNode, Include)) {
			rNode.ParseElement(rProcessor);
		}
	});
}

void
Schema::ParseElement(BaseProcessor& rProcessor) const noexcept(false) {
	rProcessor.ProcessSchema(this);
}

const std::string
Schema::Name() const noexcept(false) {
	return _extractName(m_documentURI);
}

const std::string&
Schema::URI() const noexcept(false) {
	return m_documentURI;
}

const std::string
Schema::Namespace() const noexcept(false) {
	/* search for xmlns attribute (sans the namespace prefix) */
	const TiXmlAttribute * pAttrib = Node::GetXMLElm().FirstAttribute();
	for ( ; pAttrib && (std::string::npos == std::string(pAttrib->Value()).find("http://www.w3.org/2001/XMLSchema"));
			pAttrib = pAttrib->Next()) { 
	}
	/* extract the namespace prefix if attribute found */
	if (pAttrib) {
		std::string attribName(pAttrib->Name());
		return attribName.substr(attribName.find(":") + 1);
	}
	return std::string("");
}

Types::BaseType * 
Schema::GetParentType() const noexcept(false) {
	std::unique_ptr<Node> pParent(Node::Parent());
	return (pParent.get()) ? pParent->GetParentType() : NULL;
}

bool
Schema::isRootSchema() const {
	const Parser& rParser = Node::GetParser();
	if (rParser.HasDocument(Node::GetXmlDocument()))
		return rParser.isRootDocument(Node::GetXmlDocument());
	return false;
}

/* static */ std::string
Schema::_extractName(const std::string& uri) {
	return Util::StripFileExtension(Util::ExtractResourceName(uri));
}
