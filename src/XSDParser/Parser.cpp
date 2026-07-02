/*
 * Parser.cpp
 *
 *  Created on: Jul 3, 2011
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
#include "./src/XSDParser/Parser.hpp"
#include "./src/XSDParser/Elements/Schema.hpp"
#define PROTO_FILE	"file://"


using namespace XSD;

Parser::Parser()
  : typesDb_(), docLst_(), nsIndex_()
{ }

/* virtual */
Parser::~Parser() {
	for (XmlDocList::iterator i = docLst_.begin(); i != docLst_.end(); ++i) {
		delete *i;
	}
}

Elements::Schema*
Parser::Parse(const char* pUri) const noexcept(false) {
	std::string uri(pUri);
	return Parse(uri);
}

Parser::DocumentRecord*
Parser::findByUri_(const std::string& rUri) const {
	for (XmlDocList::iterator i = docLst_.begin(); i != docLst_.end(); ++i) {
		if ((*i)->uri_ == rUri)
			return *i;
	}
	return NULL;
}

Parser::DocumentRecord*
Parser::findByDoc_(const TiXmlDocument& document) const {
	for (XmlDocList::iterator i = docLst_.begin(); i != docLst_.end(); ++i) {
		if ((*i)->pDocument_ == &document)
			return *i;
	}
	return NULL;
}

Elements::Schema*
Parser::Parse(const std::string& rUri) const noexcept(false) {
	/* search if the document has already been parsed */
	if (DocumentRecord* pRec = findByUri_(rUri)) {
		if (NULL == pRec->pDocument_->RootElement())
			throw XMLException(*pRec->pDocument_);
		return new Elements::Schema(*(pRec->pDocument_->RootElement()),
			*this, rUri);
	}
	/* only file:// or bare local paths are loadable; reject any other
	   protocol before touching the cache so a retry reports the same error */
	std::string path(rUri);
	if (0 == rUri.find(PROTO_FILE))
		path = rUri.substr(sizeof(PROTO_FILE) - 1);
	else if (std::string::npos != rUri.find("://"))
		throw XMLException(TiXmlDocument(),
			XMLException::ProtocolNotSupported);
	std::unique_ptr<TiXmlDocument> pDoc(new TiXmlDocument());
	pDoc->SetTabSize(4);
	/* LoadFile can return true yet leave no root element (TinyXML silently
	   mis-parses some DOCTYPE/encoding cases); guard RootElement() so a
	   malformed document throws instead of binding a null reference into
	   Schema and segfaulting later. Cache only successfully loaded docs. */
	if( !pDoc->LoadFile(path, TIXML_ENCODING_UTF8)
	    || NULL == pDoc->RootElement() ) {
		throw XMLException(*pDoc);
	}
	docLst_.push_back(new DocumentRecord(pDoc.release(), rUri));
	return new Elements::Schema(
		*docLst_.back()->pDocument_->RootElement(), *this, rUri);
}

const Types::TypesDB&
Parser::QueryTypesDb() const throw() {
	return typesDb_;
}

bool
Parser::HasDocument(const TiXmlDocument& document) const {
	return NULL != findByDoc_(document);
}

bool
Parser::HasDocument(const std::string& rUri) const {
	return NULL != findByUri_(rUri);
}

std::string
Parser::GetUri(const TiXmlDocument& document) const {
	DocumentRecord* pRec = findByDoc_(document);
	return pRec ? pRec->uri_ : std::string("unknown");
}

const TiXmlDocument *
Parser::GetDocument(const std::string& rUri) const {
	DocumentRecord* pRec = findByUri_(rUri);
	return pRec ? pRec->pDocument_ : NULL;
}

bool
Parser::isRootDocument(const TiXmlDocument& document) const {
	return ((*(docLst_.begin()))->pDocument_ == &document);
}

void
Parser::RegisterNamespace(const std::string& rNamespace,
		const std::string& rUri) const {
	if (!rNamespace.empty())
		nsIndex_[rNamespace] = rUri;
}

std::string
Parser::GetNamespaceUri(const std::string& rNamespace) const {
	std::map<std::string, std::string>::const_iterator it =
		nsIndex_.find(rNamespace);
	return (it != nsIndex_.end()) ? it->second : std::string("");
}

