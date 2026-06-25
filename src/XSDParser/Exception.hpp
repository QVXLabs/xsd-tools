/*
 * XSDException.hpp
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

#ifndef XSDEXCEPTION_HPP_
#define XSDEXCEPTION_HPP_
#ifndef TIXML_USE_STL
  #define TIXML_USE_STL
#endif /* TIXML_USE_STL */
#include <string>
#include <tinyxml.h>

namespace XSD {
	class XMLException : public std::exception {
	public:
		enum {
			/* include tinyxml error codes */
			NodeNotFound = TiXmlBase::TIXML_ERROR_STRING_COUNT + 1,
			MissingAttribute,
			MissingElement,
			MissingChildXMLElement,
			SubstitutionGroupTypeMismatch,
			InvallidChildXMLElement,
			UndefiniedXSDType,
			UnsupportedXSDType,
			InvalidAttribute,
			RestrictionTypeMismatch,
			ProtocolNotSupported,
			URINotValid,
			NamespaceMismatch,
			InvalidAttributeValue,
		};
		struct ErrorInfo {
			const int m_errorId;
			const int m_docRow;
			const int m_docCol;
			ErrorInfo(int id, int row, int col);
			ErrorInfo(const ErrorInfo&);
		};
		XMLException(const TiXmlDocument& doc) throw ();
		XMLException(const XMLException& excpt) throw ();
		XMLException(const TiXmlBase& elem, int id) throw();
		virtual ~XMLException() throw ();
		ErrorInfo QueryError() const throw() ;
		virtual const char* what() const throw();
	private:
		std::string		m_errorMsg;
		const ErrorInfo	m_errorInfo;
		XMLException();
		XMLException& operator= (const XMLException&) throw();
		void createErrorString_() throw();
	};
}	/* namespace XSD */
#endif /* XSDEXCEPTION_HPP_ */
