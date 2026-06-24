/*
 * Parser.hpp
 *
 *  Created on: Jun 27, 2011
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
#ifndef PARSER_HPP_
#define PARSER_HPP_

#include <string>
#include <vector>
#include "./src/XSDParser/TypesDB.hpp"
#include "./src/XSDParser/Exception.hpp"

namespace XSD {
	namespace Elements {
		class Schema;
	}
	class Parser {
	private:
		struct DocumentRecord {
			TiXmlDocument *	m_pDocument;
			std::string 	m_uri;
			DocumentRecord(TiXmlDocument *	pDocument, const std::string& uri) 
				: m_pDocument(pDocument), m_uri(uri) 
			{ }
			virtual ~DocumentRecord() {
				delete m_pDocument;
			}
		};
		typedef std::vector<DocumentRecord *> XmlDocList;
		Types::TypesDB 		m_typesDb;
	    mutable XmlDocList	m_docLst;
	public:
		Parser();
		virtual ~Parser();
		Elements::Schema* Parse(const char * uri) const noexcept(false);
		Elements::Schema* Parse(const std::string& rURI) const noexcept(false);
		const Types::TypesDB& QueryTypesDb() const throw();
		bool HasDocument(const TiXmlDocument& document) const;
		bool HasDocument(const std::string& uri) const;
		std::string GetUri(const TiXmlDocument& document) const;
		const TiXmlDocument * GetDocument(const std::string& rUri) const;
		bool isRootDocument(const TiXmlDocument& document) const;
	};
}	/* namespace XSD */

#endif /* PARSER_HPP_ */
