/*
 * XSDException.cpp
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
#include "./src/XSDParser/Exception.hpp"
#define ENUM_STR(ENUM) #ENUM

using namespace XSD;

static const char* ERRORTBL_[] ={
		ENUM_STR(TIXML_NO_ERROR),
		ENUM_STR(TIXML_ERROR),
		ENUM_STR(TIXML_ERROR_OPENING_FILE),
		ENUM_STR(TIXML_ERROR_PARSING_ELEMENT),
		ENUM_STR(TIXML_ERROR_FAILED_TO_READ_ELEMENT_NAME),
		ENUM_STR(TIXML_ERROR_READING_ELEMENT_VALUE),
		ENUM_STR(TIXML_ERROR_READING_ATTRIBUTES),
		ENUM_STR(TIXML_ERROR_PARSING_EMPTY),
		ENUM_STR(TIXML_ERROR_READING_END_TAG),
		ENUM_STR(TIXML_ERROR_PARSING_UNKNOWN),
		ENUM_STR(TIXML_ERROR_PARSING_COMMENT),
		ENUM_STR(TIXML_ERROR_PARSING_DECLARATION),
		ENUM_STR(TIXML_ERROR_DOCUMENT_EMPTY),
		ENUM_STR(TIXML_ERROR_EMBEDDED_NULL),
		ENUM_STR(TIXML_ERROR_PARSING_CDATA),
		ENUM_STR(TIXML_ERROR_DOCUMENT_TOP_ONLY),
		ENUM_STR(TIXML_ERROR_STRING_COUNT),
		ENUM_STR(NodeNotFound),
		ENUM_STR(MissingAttribute),
		ENUM_STR(MissingElement),
		ENUM_STR(MissingChildXMLElement),
		ENUM_STR(SubstitutionGroupTypeMismatch),
		ENUM_STR(InvallidChildXMLElement),
		ENUM_STR(UndefiniedXSDType),
		ENUM_STR(UnsupportedXSDType),
		ENUM_STR(InvalidAttribute),
		ENUM_STR(RestrictionTypeMismatch),
		ENUM_STR(ProtocolNotSupported),
		ENUM_STR(URINotValid),
		ENUM_STR(NamespaceMismatch),
		ENUM_STR(InvalidAttributeValue),
		ENUM_STR(CyclicTypeDefinition),
};

/*
 * XMLException - thrown due to tinyXML errors
 */
XMLException::ErrorInfo::ErrorInfo(int id, int row, int col)
	: errorId_(id), docRow_(row), docCol_(col)
{ }

XMLException::ErrorInfo::ErrorInfo(const ErrorInfo& err)
	: errorId_(err.errorId_), docRow_(err.docRow_), docCol_(err.docCol_)
{ }

XMLException::XMLException(const TiXmlDocument& doc) throw ()
	: errorInfo_(doc.ErrorId(), doc.ErrorRow(), doc.ErrorCol())
{ createErrorString_(); }

XMLException::XMLException(const XMLException& excpt) throw ()
	: errorInfo_(excpt.errorInfo_)
{ createErrorString_(); }

XMLException::XMLException(const TiXmlBase& elem, int id) throw()
	: errorInfo_(id, elem.Row(), elem.Column())
{ createErrorString_(); }

/* virtual */
XMLException::~XMLException() throw ()
{ }

XMLException::ErrorInfo
XMLException::QueryError() const throw() {
	return errorInfo_;
}

/*virtual */const char*
XMLException::what() const throw() {
	return errorMsg_.c_str();
}

void
XMLException::createErrorString_() throw (){
	std::stringstream strStrm(errorMsg_);
	strStrm << "XSD Parse Error: " << ERRORTBL_[errorInfo_.errorId_]
	        << "(" << errorInfo_.errorId_ << ") row:" << errorInfo_.docRow_
	        << " col: " << errorInfo_.docCol_ << std::endl;
	errorMsg_ = strStrm.str();
}
