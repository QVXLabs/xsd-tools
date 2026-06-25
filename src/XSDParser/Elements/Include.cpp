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
#include <unistd.h>
#include <boost/filesystem.hpp>
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
		pSchema_ = Node::GetParser().Parse(schemaURI_());
	}
	return pSchema_;
}

std::string
Include::schemaURI_() const noexcept(false) {
	std::string uri(Node::GetAttribute<const char*>("schemaLocation"));
	/* handle file URIs differently */
	if (isFileURI_(uri)) {
		std::string retStr("file://");
		boost::filesystem::path path(extractURIPath_(uri));
		if (path.has_root_path()) {
			retStr += (path.string() + extractQuery_(uri));
			return retStr;
		} else {
			std::unique_ptr<Schema> pDocRoot(Node::GetSchema());
#if defined(BOOST_FILESYSTEM_VERSION) && (BOOST_FILESYSTEM_VERSION > 2)
			boost::filesystem::path schemaPath = (boost::filesystem::absolute(extractURIPath_(pDocRoot->URI()))).branch_path();
#else
			boost::filesystem::path schemaPath = (boost::filesystem::complete(extractURIPath_(pDocRoot->URI()))).branch_path();
#endif
			(schemaPath /= extractURIPath_(uri)).normalize();
			retStr += schemaPath.string();
			return retStr;
		}
	} else {
		return uri;
	}
}

/* static */std::string
Include::extractURIPath_(const std::string& uri) {
	/* strip off query section */
	std::string noQuery		= uri.substr(0, uri.find("?"));
	/* strip off protocol section */
	const size_t	 protoNdx	= noQuery.find("://");
	std::string noProto		= noQuery.substr((std::string::npos == protoNdx) ? 0 : protoNdx + 3);
	return noProto;
}

/* static */ bool
Include::isFileURI_(const std::string& uri) {
	/* strip off query section */
	std::string noQuery	= uri.substr(0, uri.find("?"));
	bool hasFileProto	= (std::string::npos != noQuery.find("file://"));
	bool hasProto		= (std::string::npos != noQuery.find("://"));
	return (hasFileProto || !hasProto);
}

/* static */ std::string
Include::extractQuery_(const std::string& uri) {
	const size_t queryNdx = uri.find("?");
	if (std::string::npos == queryNdx)
		return std::string("");
	else
		return uri.substr(queryNdx);
}
