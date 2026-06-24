/*
 * Node.hpp
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

#ifndef NODE_HPP_
#define NODE_HPP_
#ifndef TIXML_USE_STL
#	define TIXML_USE_STL
#endif /* TIXML_USE_STL */
#include <typeinfo>
#include <memory>
#include <functional>
#include <sstream>
#include <boost/algorithm/string/predicate.hpp>
#include <tinyxml.h>
#include "./src/XSDParser/Exception.hpp"
#include "./src/XSDParser/Types.hpp"
#include "./src/XSDParser/Parser.hpp"
#include "./src/XSDParser/ProcessorBase.hpp"

#define XSD_ISELEMENT(TYPE_PTR,TYPE)	typeid(*TYPE_PTR) == typeid(TYPE)
#define XSD_ELEMENT_TAG(NAME)			public: static const char* XSDTag() throw() { return NAME; }

namespace XSD { 
  namespace Elements { 
	class Schema; 
  }   
}


namespace XSD {
	namespace Elements {
		class Node {
		private:
			const TiXmlElement&		m_rXmlElm;	// <-- Tinyxml dom node
			const Parser&			m_rParser;	// <-- XSD parser engine

			Node();
			const TiXmlElement* ContentElement(const char* pElemName) const noexcept(false);
			const TiXmlElement* _FindChildXMLElement(const char* pXMLElmTag, const char* pAttrib, const char* pName) const noexcept(false);
			Node* _FindXSDElm(const char* pName, const char* pTypeName) const noexcept(false);
			Node* _ConstructNode(const TiXmlElement* pElm, const Parser& rParser) const;
			Node* _FindXSDNode(const char* pName, const char* pTypeName) const noexcept(false);
			Node* _FindChildXSDNode(const char* pXMLTag) const noexcept(false);
			Node* _FindXSDRef(const char* pRefAttribStr, const char* pTypeName) const noexcept(false);
			std::string _Attribute(const char* pAttrib) const noexcept(false);
			Types::BaseType* _Type(const char* pType) const noexcept(false);
			const std::string _StripNamespace(const std::string& rQName) const noexcept(false);
		protected:
			Node(const TiXmlElement& elm, const Parser& rParser);
			Node(const Node& rCpy);
			Types::BaseType* LookupType(const char* pType) const noexcept(false) { return _Type(pType); }
			/* QueryRootElement(): returns the root element of the document containg the node*/
			const TiXmlElement& QueryRootElement() const;
			Schema * GetSchema() const noexcept(false);
			const Parser& GetParser() const { return m_rParser; }
			bool HasAttribute(const char* pAttrib) const noexcept;
			bool HasContent(const char* pElemName) const noexcept;
			bool HasContent() const noexcept;
			bool IsRootNode() const noexcept;
			std::string QualifyElementName(const char* pElemName) const noexcept;
			template<typename T> T GetAttribute(const char* pAttrib) const noexcept(false) {
				T retVal;
				std::stringstream sstrm(_Attribute(pAttrib));
				sstrm >> retVal;
				return retVal;
			}
			template<typename T> T* FindXSDElm(const char* pName) const noexcept(false) {
				return static_cast<T*>(_FindXSDNode(pName, T::XSDTag()));
			}
			template<typename T> T* FindXSDRef(const char* pRefAttribStr) const noexcept(false) {
				return static_cast<T*>(_FindXSDRef(pRefAttribStr, T::XSDTag()));
			}
			template<typename T> T* FindXSDChildElm() const noexcept(false) {
				return static_cast<T*>(_FindChildXSDNode(T::XSDTag()));
			}
			template<typename T> T* SearchXSDChildElm() const noexcept {
				try {
					return static_cast<T*>(_FindChildXSDNode(T::XSDTag()));
				} catch (XMLException& e) {
					return NULL;
				}
			}
		public:
			virtual ~Node();
			virtual void ParseChildren(BaseProcessor& rProcessor) const noexcept(false) = 0;
			virtual void ParseElement(BaseProcessor& rProcessor) const noexcept(false) = 0;
			/* for "element" : return their type 
			 * for "simpleType/complexType" : return the base type of their child elements
			 * for "simpleContent/complexContent" : return type of their child restriction/extension elements
			 * for "restriction/extension": return base type
			 * for "list" : return type
			 * for "union" : return xs:string HACK
			 * for all other elements: return the type they are enclosed in */
			virtual Types::BaseType * GetParentType() const noexcept(false) = 0;
			Node* Parent() const noexcept(false);
			Node* FirstChild() const noexcept(false);
			Node* NextSibling() const noexcept(false);
			/* Recurse over the child sibling chain, invoking rFn on each
			 * child. Functional replacement for the do/while NextSibling
			 * loops in the ParseChildren overrides. */
			void _eachChild(
				const std::function<void(const Node&)>& rFn) const noexcept(false);
			/* Recurse the child sibling chain until rFn returns true (a
			 * short-circuiting search); returns true if some child matched.
			 * Functional replacement for the do/while search loops. */
			bool _findChild(
				const std::function<bool(const Node&)>& rFn) const noexcept(false);
			bool operator == (const Node& elm) const;
			bool operator == (const Node& elm);
			inline const TiXmlElement& GetXMLElm() const { return m_rXmlElm;}
			const TiXmlDocument& GetXmlDocument() const;
		};
		template<> bool Node::GetAttribute<bool>(const char* pAttrib) const noexcept(false);
		template<> const char* Node::GetAttribute<const char*>(const char* pAttrib) const noexcept(false);
		template<> Types::BaseType* Node::GetAttribute<Types::BaseType*>(const char* pAttrib) const noexcept(false);
	}	/* namespace Elements */
}	/* namespace XSD */

//#include "./src/XSDParser/Elements/Node.inc"
#endif /* NODE_HPP_ */
