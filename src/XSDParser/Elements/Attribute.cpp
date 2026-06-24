/*
 * Attribute.cpp
 *
 *  Created on: Jun 26, 2011
 *      Author: Ardavon Falls
 *   Copyright: (c)2011 Ardavon Falls
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

#ifndef TIXML_USE_STL
#	define TIXML_USE_STL
#endif /* TIXML_USE_STL */
#include <typeinfo>
#include <memory>
#include <tinyxml.h>
#include "./src/XSDParser/Elements/Attribute.hpp"
#include "./src/XSDParser/Elements/SimpleType.hpp"
#include "./src/XSDParser/Elements/Schema.hpp"
#include "./src/XSDParser/Elements/Annotation.hpp"
#include "./src/XSDParser/ProcessorBase.hpp"

using namespace XSD;
using namespace XSD::Elements;

Attribute::Attribute(const TiXmlElement& elm, const Parser& rParser)
	: Node(elm, rParser)
{ }

Attribute::Attribute(const Attribute& rAttrib)
	: Node(rAttrib)
{ }

void
Attribute::ParseChildren(BaseProcessor& rProcessor) const noexcept(false) {
	/* process children */
	_eachChild([&rProcessor](const Node& rNode) {
		if (XSD_ISELEMENT(&rNode, SimpleType) ||
			XSD_ISELEMENT(&rNode, Annotation)) {
			rProcessor.ProcessSimpleType(static_cast<const SimpleType*>(&rNode));
		} else
			throw XMLException(rNode.GetXMLElm(), XMLException::InvallidChildXMLElement);
	});
}

void
Attribute::ParseElement(BaseProcessor& rProcessor) const noexcept(false) {
	if (!HasName()) {
		/* if the attribute is a child of the schema root, it must have a name */
		std::unique_ptr<Node> pParent(Node::Parent());
		if (XSD_ISELEMENT(pParent.get(), SimpleType))
			throw XMLException(Node::GetXMLElm(), XMLException::InvalidAttribute);
	} else if (HasRef()) {
		/* an attribute cannot have a name and ref */
		throw XMLException(Node::GetXMLElm(), XMLException::InvalidAttribute);
	}
	if (HasType()) {
		/* an attribute cannot have a type field and a type defined as child */
		if (Node::HasContent(SimpleType::XSDTag()))
			throw XMLException(Node::GetXMLElm(), XMLException::InvalidAttribute);
	} else if (!Node::HasContent(SimpleType::XSDTag())) {
		/* an attribute must have a type defined as a child if a type is not defined */
		throw XMLException(Node::GetXMLElm(), XMLException::MissingChildXMLElement);
	}
	if (HasDefault() && HasFixed()) {
		/* attributes cannot have fixed values and default values at the same time */
		throw XMLException(Node::GetXMLElm(), XMLException::InvalidAttribute);
	}
	/* if the node is a reference, check its reference */
	if (HasRef()) {
		std::unique_ptr<Node> pRefElm(RefAttribute());
		pRefElm->ParseElement(rProcessor);
	} else {
		rProcessor.ProcessAttribute(this);
	}
}

Types::BaseType * 
Attribute::GetParentType() const noexcept(false) {
	std::unique_ptr<Node> pParent(Node::Parent());
	return pParent->GetParentType();
}

std::string
Attribute::Name() const noexcept(false) {
	return std::string(Node::GetAttribute<const char*>("name"));
}

Attribute*
Attribute::RefAttribute() const noexcept(false) {
	return Node::FindXSDRef<Attribute>("ref");
}

Types::BaseType*
Attribute::Type() const noexcept(false) {
	if (HasRef()) {
		std::unique_ptr<XSD::Elements::Attribute> pRefAttrib(RefAttribute());
		return _parseType(*pRefAttrib);
	} else {
		return _parseType(*this);
	}
}

std::string
Attribute::Default() const noexcept(false) {
	return std::string(Node::GetAttribute<const char*>("default"));
}

std::string 
Attribute::Fixed() const noexcept(false) {
	return std::string(Node::GetAttribute<const char*>("fixed"));
}

Attribute::AttributeUse 
Attribute::Use() const noexcept(false) {
	if (HasUse()) {
		std::string use(Node::GetAttribute<const char*>("use"));
		if (!use.compare("optional")) {
			return Attribute::OPTIONAL;
		} else if (!use.compare("prohibited")) {
			return Attribute::PROHIBITIED;
		} else if (!use.compare("required")) {
			return Attribute::REQUIRED;
		} else
			throw XMLException(Node::GetXMLElm(), XMLException::InvalidAttributeValue);
	}
	return Attribute::OPTIONAL;
}

bool
Attribute::HasName() const {
	return this->HasAttribute("name");
}

bool
Attribute::HasRef() const {
	return this->HasAttribute("ref");
}

bool
Attribute::HasType() const {
	return this->HasAttribute("type");
}

bool
Attribute::HasDefault() const {
	return this->HasAttribute("default");
}

bool
Attribute::HasFixed() const {
	return this->HasAttribute("fixed");
}

bool
Attribute::HasUse() const {
	return this->HasAttribute("use");
}
			
Types::BaseType*
Attribute::_type() const noexcept(false) {
	Types::BaseType* pType = Node::GetAttribute<Types::BaseType*>("type");
	if (XSD_ISTYPE(pType, Types::Unknown)) {
		delete pType;
		return new Types::String();
	} else if (XSD_ISTYPE(pType, Types::ComplexType) ||
			   XSD_ISTYPE(pType, Types::Unsupported))
		throw  XMLException(Node::GetXMLElm(), XMLException::UndefiniedXSDType);
	return pType;
}

/* static */ Types::BaseType*
Attribute::_parseType(const Attribute& rAttrib) noexcept(false) {
	if (rAttrib.HasContent(SimpleType::XSDTag()))
		return new Types::SimpleType(rAttrib.FindXSDChildElm<SimpleType>());
	else
		return rAttrib._type();
}
