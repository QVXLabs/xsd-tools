/*
 * Node.cpp
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

#include <memory>
#include <string.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include "./src/XSDParser/Elements/Node.hpp"
#include "./src/XSDParser/Elements/Attribute.hpp"
#include "./src/XSDParser/Elements/Choice.hpp"
#include "./src/XSDParser/Elements/Element.hpp"
#include "./src/XSDParser/Elements/List.hpp"
#include "./src/XSDParser/Elements/Restriction.hpp"
#include "./src/XSDParser/Elements/Sequence.hpp"
#include "./src/XSDParser/Elements/Union.hpp"
#include "./src/XSDParser/Elements/SimpleType.hpp"
#include "./src/XSDParser/Elements/ComplexType.hpp"
#include "./src/XSDParser/Elements/Schema.hpp"
#include "./src/XSDParser/Elements/Group.hpp"
#include "./src/XSDParser/Elements/Any.hpp"
#include "./src/XSDParser/Elements/ComplexContent.hpp"
#include "./src/XSDParser/Elements/Extension.hpp"
#include "./src/XSDParser/Elements/SimpleContent.hpp"
#include "./src/XSDParser/Elements/MinExclusive.hpp"
#include "./src/XSDParser/Elements/MaxExclusive.hpp"
#include "./src/XSDParser/Elements/MinInclusive.hpp"
#include "./src/XSDParser/Elements/MaxInclusive.hpp"
#include "./src/XSDParser/Elements/MinLength.hpp"
#include "./src/XSDParser/Elements/MaxLength.hpp"
#include "./src/XSDParser/Elements/Length.hpp"
#include "./src/XSDParser/Elements/Enumeration.hpp"
#include "./src/XSDParser/Elements/FractionDigits.hpp"
#include "./src/XSDParser/Elements/Pattern.hpp"
#include "./src/XSDParser/Elements/TotalDigits.hpp"
#include "./src/XSDParser/Elements/WhiteSpace.hpp"
#include "./src/XSDParser/Elements/AttributeGroup.hpp"
#include "./src/XSDParser/Elements/Include.hpp"
#include "./src/XSDParser/Elements/Import.hpp"
#include "./src/XSDParser/Elements/Annotation.hpp"
#include "./src/XSDParser/Elements/Documentation.hpp"
#include "./src/XSDParser/Elements/All.hpp"
#include "./src/XSDParser/Elements/AppInfo.hpp"

#define DEBUG_CONSTRUCTION (0)

using namespace XSD;
using namespace XSD::Elements;

/* template specialization first */
namespace XSD{
	namespace Elements {
		template<> bool
		Node::GetAttribute<bool>(const char* pAttrib) const noexcept(false) {
			bool retVal;
			std::string attribStr(Attribute_(pAttrib));
			std::transform(attribStr.begin(), attribStr.end(), attribStr.begin(),::tolower);
			std::stringstream sstrm(attribStr);
			sstrm >> std::boolalpha >> retVal;
			return retVal;
		}

		template<> const char*
		Node::GetAttribute<const char*>(const char* pAttrib) const noexcept(false) {
			const char* pRetVal = rXmlElm_.Attribute(pAttrib);
			if (!pRetVal) throw XMLException(rXmlElm_, XMLException::MissingAttribute);
			return pRetVal;
		}

		template<> Types::BaseType*
		Node::GetAttribute<Types::BaseType*>(const char* pAttrib) const noexcept(false) {
			const char* pTypeStr = GetAttribute<const char*>(pAttrib);
			if (!pTypeStr) throw XMLException(rXmlElm_, XMLException::MissingAttribute);
			return Type_(pTypeStr);
		}
	}
}

/* Non-specialized methods */
Node::Node(const TiXmlElement& rElm, const Parser& rParser)
	: rXmlElm_(rElm), rParser_(rParser)
{ 
#if (DEBUG_CONSTRUCTION)
	std::cout << "Created: " << rXmlElm_.ValueStr() << "[ ";
	for (const TiXmlAttribute * pAttr = rXmlElm_.FirstAttribute(); pAttr; pAttr = pAttr->Next())
		std::cout << pAttr->Name() << ":" << pAttr->Value() << " ";
	std::cout << "]" << std::endl;
#endif /* DEBUG_CONSTRUCTION */
}

Node::Node(const Node& rCpy)
	: rXmlElm_(rCpy.rXmlElm_), rParser_(rCpy.rParser_)
{
#if (DEBUG_CONSTRUCTION)
	std::cout << "Created: " << rXmlElm_.ValueStr() << "[ ";
	for (const TiXmlAttribute * pAttr = rXmlElm_.FirstAttribute(); pAttr; pAttr = pAttr->Next())
		std::cout << pAttr->Name() << ":" << pAttr->Value() << " ";
	std::cout << "]" << std::endl;  
#endif /* DEBUG_CONSTRUCTION */
}

/* virtual */
Node::~Node() {
#if (DEBUG_CONSTRUCTION)
	std::cout << "Deleted: " << rXmlElm_.ValueStr() << "[ ";
	for (const TiXmlAttribute * pAttr = rXmlElm_.FirstAttribute(); pAttr; pAttr = pAttr->Next())
		std::cout << pAttr->Name() << ":" << pAttr->Value() << " ";
	std::cout << "]" << std::endl;
#endif /* DEBUG_CONSTRUCTION */
}

const TiXmlElement*
Node::FindChildXMLElement_(const char* pXMLElmTag, const char* pAttrib, const char* pName) const noexcept(false) {
	/* search all nodes in root document */
	const TiXmlElement* pElm = rXmlElm_.FirstChildElement(pXMLElmTag);
	for ( ;
		pElm && pElm->Attribute(pAttrib) && !boost::iequals(std::string(pElm->Attribute(pAttrib)), std::string(pName));
		pElm = pElm->NextSiblingElement(pXMLElmTag) ) {}
	return pElm;
}

Node*
Node::ConstructNode_(const TiXmlElement* pElm, const Parser& rParser) const {
	typedef Node* (*Factory)(const TiXmlElement&, const Parser&);
	/* tag -> factory; scanned case-insensitively, same order as before. */
	static const struct { const char* pTag; Factory pMake; } kTable[] = {
		{ Attribute::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new Attribute(e, p); } },
		{ Choice::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new Choice(e, p); } },
		{ Element::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new Element(e, p); } },
		{ List::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new List(e, p); } },
		{ Restriction::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new Restriction(e, p); } },
		{ Sequence::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new Sequence(e, p); } },
		{ Union::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new Union(e, p); } },
		{ Schema::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new Schema(e, p, std::string("")); } },
		{ SimpleType::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new SimpleType(e, p); } },
		{ ComplexType::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new ComplexType(e, p); } },
		{ Group::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new Group(e, p); } },
		{ Any::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new Any(e, p); } },
		{ ComplexContent::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new ComplexContent(e, p); } },
		{ Extension::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new Extension(e, p); } },
		{ SimpleContent::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new SimpleContent(e, p); } },
		{ MinExclusive::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new MinExclusive(e, p); } },
		{ MaxExclusive::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new MaxExclusive(e, p); } },
		{ MinInclusive::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new MinInclusive(e, p); } },
		{ MaxInclusive::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new MaxInclusive(e, p); } },
		{ MinLength::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new MinLength(e, p); } },
		{ MaxLength::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new MaxLength(e, p); } },
		{ Length::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new Length(e, p); } },
		{ Enumeration::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new Enumeration(e, p); } },
		{ FractionDigits::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new FractionDigits(e, p); } },
		{ Pattern::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new Pattern(e, p); } },
		{ TotalDigits::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new TotalDigits(e, p); } },
		{ WhiteSpace::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new WhiteSpace(e, p); } },
		{ AttributeGroup::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new AttributeGroup(e, p); } },
		{ Include::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new Include(e, p); } },
		{ Import::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new Import(e, p); } },
		{ Annotation::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new Annotation(e, p); } },
		{ Documentation::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new Documentation(e, p); } },
		{ All::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new All(e, p); } },
		{ AppInfo::XSDTag(),
			[](const TiXmlElement& e, const Parser& p) -> Node*
				{ return new AppInfo(e, p); } },
	};
	const std::string elementName = stripPrefix_(pElm->ValueStr());
	for (const auto& rEntry : kTable) {
		if (boost::iequals(std::string(rEntry.pTag), elementName))
			return rEntry.pMake(*pElm, rParser);
	}
	throw XMLException(*pElm, XMLException::InvallidChildXMLElement);
	return NULL;
}

Node*
Node::FindXSDElm_(const XsdQName& rName, const char* pTypeName) const noexcept(false) {
	std::string elementName = QualifyElementName(pTypeName);
	const char* pLocal = rName.local.c_str();
	/* search root document */
	Node* pRetNode = NULL;
	std::unique_ptr<Schema> pSchemaRoot(GetSchema());
	const TiXmlElement* pElm =
		(pSchemaRoot->TargetNamespace() == rName.ns)
			? pSchemaRoot->FindChildXMLElement_(elementName.c_str(), "name", pLocal)
			: NULL;
	/* search all included documents (same-namespace merge) */
	if (!pElm) {
		const TiXmlElement* pIncludElm = pSchemaRoot->GetXMLElm().FirstChildElement(XSD::Elements::Include::XSDTag());
		for ( ; pIncludElm; pIncludElm = pIncludElm->NextSiblingElement(XSD::Elements::Include::XSDTag()) ) {
			std::unique_ptr<Include> pNode(static_cast<Include*>(ConstructNode_(pIncludElm, rParser_)));
			const Schema* pSchema = pNode->QuerySchema();
			if (pSchema->TargetNamespace() != rName.ns)
				continue;
			if (NULL != (pElm = pSchema->FindChildXMLElement_(elementName.c_str(), "name", pLocal))) {
				pRetNode = ConstructNode_(pElm, pSchema->rParser_);
				break;
			}
		}
	} else
		pRetNode = ConstructNode_(pElm, rParser_);
	/* cross-namespace reference: resolve against an xs:import whose loaded
	 * target namespace matches rName.ns. The imported doc keeps a distinct
	 * namespace, unlike xs:include's same-namespace merge above. */
	if (!pRetNode)
		pRetNode = FindImportedXSDElm_(rName, pTypeName);
	return pRetNode;
}

Node*
Node::FindImportedXSDElm_(const XsdQName& rName, const char* pTypeName) const noexcept(false) {
	const char* pLocal = rName.local.c_str();
	std::unique_ptr<Schema> pSchemaRoot(GetSchema());
	std::string importTag = QualifyElementName(XSD::Elements::Import::XSDTag());
	const TiXmlElement* pImportElm = pSchemaRoot->GetXMLElm().FirstChildElement(importTag.c_str());
	for ( ; pImportElm; pImportElm = pImportElm->NextSiblingElement(importTag.c_str()) ) {
		std::unique_ptr<Import> pNode(static_cast<Import*>(ConstructNode_(pImportElm, rParser_)));
		const Schema* pSchema = pNode->QuerySchema();
		if (NULL == pSchema || pSchema->TargetNamespace() != rName.ns)
			continue;
		/* qualify the type tag against the imported doc's own XSD-lang prefix */
		std::string elementName = pSchema->QualifyElementName(pTypeName);
		const TiXmlElement* pElm =
			pSchema->FindChildXMLElement_(elementName.c_str(), "name", pLocal);
		if (NULL != pElm)
			return ConstructNode_(pElm, pSchema->rParser_);
	}
	return NULL;
}

Node*
Node::FindXSDNode_(const XsdQName& rName, const char* pTypeName) const noexcept(false) {
	Node* pNode = FindXSDElm_(rName, pTypeName);
	if (!pNode)
		throw XMLException(rXmlElm_, XMLException::MissingElement);
	return pNode;
}

Node*
Node::FindChildXSDNode_(const char* pXMLTag) const noexcept(false) {
	std::string elementName = QualifyElementName(pXMLTag);
	/* search all nodes in root document */
	const TiXmlElement* pElm = rXmlElm_.FirstChildElement(elementName.c_str());
	if (NULL == pElm)
		throw XMLException(rXmlElm_, XMLException::MissingChildXMLElement);
	return ConstructNode_(pElm, rParser_);
}

Node*
Node::FindXSDRef_(const char* pRefAttribStr, const char* pTypeName) const noexcept(false) {
	if (HasAttribute(pRefAttribStr)) {
		XsdQName ref = ResolveQName_(GetAttribute<const char*>(pRefAttribStr));
		std::unique_ptr<Node> pRefElm(FindXSDNode_(ref, pTypeName));
		return pRefElm->FindXSDRef_(pRefAttribStr, pTypeName);
	} else
		return ConstructNode_(&rXmlElm_, rParser_);
}

std::string
Node::Attribute_(const char* pAttrib) const noexcept(false) {
	if (HasAttribute(pAttrib)) {
		return std::string(rXmlElm_.Attribute(pAttrib));
	} else
		throw XMLException(rXmlElm_, XMLException::MissingAttribute);
	return std::string("");
}

Types::BaseType*
Node::Type_(const char* pType) const noexcept(false) {
	return Type_(ResolveQName_(std::string(pType)));
}

Types::BaseType*
Node::Type_(const XsdQName& rName) const noexcept(false) {
	/* Builtins are detected by namespace URI, not by the "xs" prefix.
	 * TypesDB stays keyed by bare local name. An empty ns (a schema that
	 * declares no namespace at all) also probes builtins, preserving the
	 * legacy bare-name lookup. */
	if (rName.ns == XSD_NS || rName.ns.empty()) {
		Types::BaseType* pBuiltin =
			rParser_.QueryTypesDb().FindType(rName.local.c_str());
		if (typeid(*pBuiltin) != typeid(Types::Unknown))
			return pBuiltin;
		delete pBuiltin;
	}
	/* user-defined type search */
	Node* pSimpleType = FindXSDElm_(rName, SimpleType::XSDTag());
	if (NULL != pSimpleType)
		return new Types::SimpleType(static_cast<SimpleType*>(pSimpleType));
	Node* pComplexType = FindXSDElm_(rName, ComplexType::XSDTag());
	if (NULL != pComplexType)
		return new Types::ComplexType(
			static_cast<ComplexType*>(pComplexType));
	throw XMLException(rXmlElm_, XMLException::MissingElement);
}

/* static */ std::string
Node::stripPrefix_(const std::string& rQName) noexcept {
	const size_t ndx = rQName.find(":");
	return (std::string::npos == ndx) ? rQName : rQName.substr(ndx + 1);
}

XsdQName
Node::ResolveQName_(const std::string& rRaw) const noexcept(false) {
	std::unique_ptr<Schema> pSchema(GetSchema());
	const size_t ndx = rRaw.find(":");
	std::string prefix = (std::string::npos == ndx)
		? std::string("") : rRaw.substr(0, ndx);
	std::string local = (std::string::npos == ndx)
		? rRaw : rRaw.substr(ndx + 1);
	return XsdQName{ pSchema->ResolvePrefix(prefix), local };
}

Node*
Node::Parent() const noexcept(false) {
	const TiXmlElement* pElm = rXmlElm_.Parent()->ToElement();
	return pElm ? ConstructNode_(pElm, rParser_) : NULL;
}

/* virtual */ Types::BaseType*
Node::GetParentType() const noexcept(false) {
	std::unique_ptr<Node> pParent(Node::Parent());
	return pParent->GetParentType();
}

Node*
Node::FirstChild() const noexcept(false) {
	const TiXmlElement* pElm = rXmlElm_.FirstChildElement();
	return pElm ? ConstructNode_(pElm, rParser_) : NULL;
}
Node*
Node::NextSibling() const noexcept(false) {
	const TiXmlElement* pElm = rXmlElm_.NextSiblingElement();
	return pElm ? ConstructNode_(pElm, rParser_) : NULL;
}

/* Recurse the sibling chain from pNode until rFn returns true. */
static bool
findSibling_(std::unique_ptr<Node> pNode,
		const std::function<bool(const Node&)>& rFn) noexcept(false) {
	if (NULL == pNode.get())
		return false;
	if (rFn(*pNode))
		return true;
	return findSibling_(std::unique_ptr<Node>(pNode->NextSibling()), rFn);
}

bool
Node::findChild_(
		const std::function<bool(const Node&)>& rFn) const noexcept(false) {
	return findSibling_(std::unique_ptr<Node>(FirstChild()), rFn);
}

void
Node::eachChild_(
		const std::function<void(const Node&)>& rFn) const noexcept(false) {
	/* visit-all expressed via the short-circuiting search */
	findChild_([&rFn](const Node& rNode) -> bool { rFn(rNode); return false; });
}

const TiXmlElement*
Node::ContentElement(const char* pElemName) const noexcept(false) {
	std::string elementName = QualifyElementName(pElemName);
	const TiXmlElement* pElm = rXmlElm_.FirstChildElement(elementName.c_str());
	if (!pElm) throw XMLException(rXmlElm_,XMLException::MissingChildXMLElement);
	return pElm;
}

const TiXmlElement& 
Node::QueryRootElement() const {
	const TiXmlDocument * pNode = GetXmlDocument().ToDocument();
	return *(pNode->RootElement());
}

Schema * 
Node::GetSchema() const noexcept(false) {
	return new Elements::Schema(QueryRootElement(), rParser_, rParser_.GetUri(GetXmlDocument()));
}

bool
Node::HasAttribute(const char* pAttrib) const throw() {
	const char* pAttribVal = rXmlElm_.Attribute(pAttrib);
	return (NULL != pAttribVal && *pAttribVal != 0);
}

bool
Node::HasContent(const char* pElemName) const throw() {
	std::string elementName = QualifyElementName(pElemName);
	return (NULL != rXmlElm_.FirstChildElement(elementName.c_str()));
}

bool
Node::HasContent() const throw() {
	/* search for a text node */
	const TiXmlNode* pXmlNode = rXmlElm_.FirstChild();
	for ( ; NULL != pXmlNode; pXmlNode = pXmlNode->NextSibling()) {
		if (TiXmlNode::TINYXML_TEXT == pXmlNode->Type())
			return true;
	}
	return false;
}

int
Node::maxOccurs_(int dflt) const noexcept(false) {
	if (HasAttribute("maxOccurs")) {
		if (strcmp(GetAttribute<const char*>("maxOccurs"), "unbounded"))
			return GetAttribute<int>("maxOccurs");
		else
			return -1;
	}
	return dflt;
}

int
Node::minOccurs_(int dflt) const noexcept(false) {
	if (HasAttribute("minOccurs"))
		return GetAttribute<int>("minOccurs");
	return dflt;
}

Types::BaseType*
Node::delegateToSoleChild_() const noexcept(false) {
	std::unique_ptr<Restriction> pRestriction(
		SearchXSDChildElm<Restriction>());
	std::unique_ptr<Extension> pExtension(SearchXSDChildElm<Extension>());
	if ((NULL != pRestriction.get()) && (NULL == pExtension.get())) {
		return pRestriction->GetParentType();
	} else if ((NULL == pRestriction.get()) && (NULL != pExtension.get())) {
		return pExtension->GetParentType();
	} else /* content can't have multiple child modifiers */
		throw XMLException(GetXMLElm(), XMLException::InvallidChildXMLElement);
}

Types::BaseType*
Node::baseType_() const noexcept(false) {
	return GetAttribute<Types::BaseType*>("base");
}

std::string
Node::name_() const noexcept(false) {
	return std::string(GetAttribute<const char*>("name"));
}

bool
Node::IsRootNode() const throw () {
	const TiXmlNode* 	pParent	= rXmlElm_.Parent();
	const TiXmlElement* pElm 	= (pParent) ? pParent->ToElement() : NULL;
	return pElm && (pElm == &QueryRootElement());
}

bool
Node::operator == (const Node& elm) const {
	return &rXmlElm_ == &elm.rXmlElm_;
}

bool
Node::operator == (const Node& elm) {
	return &rXmlElm_ == &elm.rXmlElm_;
}

std::string 
Node::QualifyElementName(const char* pElemName) const throw() {
	std::string elementName(pElemName);
	std::unique_ptr<Schema> pDocRoot(GetSchema());
	std::string xmlNamespace = pDocRoot->Namespace();
	/* add namespace if element name isn't fully qualified and namespace 
	   exists and is not unnamed */
	if (0 < xmlNamespace.length() &&
	    0 != xmlNamespace.compare("xmlns") &&
	    std::string::npos == elementName.find(xmlNamespace)) {
		return xmlNamespace + ":" + elementName;
	}
	return elementName;
}

const TiXmlDocument&
Node::GetXmlDocument() const {
	const TiXmlNode * pNode = &rXmlElm_;
	for (; pNode->Parent(); pNode = pNode->Parent()) {}
	return *(pNode->ToDocument());
}

/* Shared schemaLocation -> loadable-URI resolution for xs:include/xs:import. */
namespace {
	std::string extractURIPath_(const std::string& uri) {
		std::string noQuery = uri.substr(0, uri.find("?"));
		const size_t protoNdx = noQuery.find("://");
		return noQuery.substr(
			(std::string::npos == protoNdx) ? 0 : protoNdx + 3);
	}
	bool isFileURI_(const std::string& uri) {
		std::string noQuery = uri.substr(0, uri.find("?"));
		bool hasFileProto = (std::string::npos != noQuery.find("file://"));
		bool hasProto = (std::string::npos != noQuery.find("://"));
		return (hasFileProto || !hasProto);
	}
	std::string extractQuery_(const std::string& uri) {
		const size_t queryNdx = uri.find("?");
		return (std::string::npos == queryNdx)
			? std::string("") : uri.substr(queryNdx);
	}
}

std::string
Node::resolveSchemaURI_(const char* pAttrib) const noexcept(false) {
	std::string uri(GetAttribute<const char*>(pAttrib));
	if (!isFileURI_(uri))
		return uri;
	std::string retStr("file://");
	boost::filesystem::path path(extractURIPath_(uri));
	if (path.has_root_path()) {
		retStr += (path.string() + extractQuery_(uri));
		return retStr;
	}
	std::unique_ptr<Schema> pDocRoot(GetSchema());
#if defined(BOOST_FILESYSTEM_VERSION) && (BOOST_FILESYSTEM_VERSION > 2)
	boost::filesystem::path schemaPath = (boost::filesystem::absolute(extractURIPath_(pDocRoot->URI()))).branch_path();
#else
	boost::filesystem::path schemaPath = (boost::filesystem::complete(extractURIPath_(pDocRoot->URI()))).branch_path();
#endif
	(schemaPath /= extractURIPath_(uri)).normalize();
	retStr += schemaPath.string();
	return retStr;
}
