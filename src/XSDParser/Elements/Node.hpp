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
/* Define an attribute-presence predicate inline (METHOD -> HasAttribute). */
#define XSD_HAS_ATTR(METHOD,ATTR)		bool METHOD() const noexcept { return Node::HasAttribute(ATTR); }

namespace XSD { 
  namespace Elements { 
	class Schema; 
  }   
}


namespace XSD {
	namespace Elements {
		class Node {
		private:
			const TiXmlElement&		rXmlElm_;	// <-- Tinyxml dom node
			const Parser&			rParser_;	// <-- XSD parser engine

			Node();
			const TiXmlElement* ContentElement(const char* pElemName) const noexcept(false);
			const TiXmlElement* FindChildXMLElement_(const char* pXMLElmTag, const char* pAttrib, const char* pName) const noexcept(false);
			Node* FindXSDElm_(const char* pName, const char* pTypeName) const noexcept(false);
			Node* ConstructNode_(const TiXmlElement* pElm, const Parser& rParser) const;
			Node* FindXSDNode_(const char* pName, const char* pTypeName) const noexcept(false);
			Node* FindChildXSDNode_(const char* pXMLTag) const noexcept(false);
			Node* FindXSDRef_(const char* pRefAttribStr, const char* pTypeName) const noexcept(false);
			std::string Attribute_(const char* pAttrib) const noexcept(false);
			const std::string StripNamespace_(const std::string& rQName) const noexcept(false);
		protected:
			Node(const TiXmlElement& elm, const Parser& rParser);
			Node(const Node& rCpy);
			Types::BaseType* Type_(const char* pType) const noexcept(false);
			/* QueryRootElement(): returns the root element of the document containg the node*/
			const TiXmlElement& QueryRootElement() const;
			Schema * GetSchema() const noexcept(false);
			const Parser& GetParser() const { return rParser_; }
			bool HasAttribute(const char* pAttrib) const noexcept;
			bool HasContent(const char* pElemName) const noexcept;
			bool HasContent() const noexcept;
			bool IsRootNode() const noexcept;
			/* Shared maxOccurs/minOccurs reads: absent -> dflt, "unbounded"
			 * -> -1. Used by the byte-identical Choice/Group/Any overrides. */
			int maxOccurs_(int dflt) const noexcept(false);
			int minOccurs_(int dflt) const noexcept(false);
			/* GetParentType for nodes whose type comes from a single child
			 * restriction xor extension (simpleContent/complexContent). */
			Types::BaseType* delegateToSoleChild_() const noexcept(false);
			/* The "base" attribute as a type; shared by restriction/extension. */
			Types::BaseType* baseType_() const noexcept(false);
			/* The "name" attribute as a string; shared by named nodes. */
			std::string name_() const noexcept(false);
			std::string QualifyElementName(const char* pElemName) const noexcept;
			template<typename T> T GetAttribute(const char* pAttrib) const noexcept(false) {
				T retVal;
				std::stringstream sstrm(Attribute_(pAttrib));
				sstrm >> retVal;
				return retVal;
			}
			template<typename T> T* FindXSDElm(const char* pName) const noexcept(false) {
				return static_cast<T*>(FindXSDNode_(pName, T::XSDTag()));
			}
			template<typename T> T* FindXSDRef(const char* pRefAttribStr) const noexcept(false) {
				return static_cast<T*>(FindXSDRef_(pRefAttribStr, T::XSDTag()));
			}
			template<typename T> T* FindXSDChildElm() const noexcept(false) {
				return static_cast<T*>(FindChildXSDNode_(T::XSDTag()));
			}
			template<typename T> T* SearchXSDChildElm() const noexcept {
				try {
					return static_cast<T*>(FindChildXSDNode_(T::XSDTag()));
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
			 * for all other elements: return the type they are enclosed in
			 * (the default — delegate to the enclosing parent) */
			virtual Types::BaseType * GetParentType() const noexcept(false);
			Node* Parent() const noexcept(false);
			Node* FirstChild() const noexcept(false);
			Node* NextSibling() const noexcept(false);
			/* Recurse over the child sibling chain, invoking rFn on each
			 * child. Functional replacement for the do/while NextSibling
			 * loops in the ParseChildren overrides. */
			void eachChild_(
				const std::function<void(const Node&)>& rFn) const noexcept(false);
			/* Recurse the child sibling chain until rFn returns true (a
			 * short-circuiting search); returns true if some child matched.
			 * Functional replacement for the do/while search loops. */
			bool findChild_(
				const std::function<bool(const Node&)>& rFn) const noexcept(false);
			bool operator == (const Node& elm) const;
			bool operator == (const Node& elm);
			inline const TiXmlElement& GetXMLElm() const { return rXmlElm_;}
			const TiXmlDocument& GetXmlDocument() const;
		};
		template<> bool Node::GetAttribute<bool>(const char* pAttrib) const noexcept(false);
		template<> const char* Node::GetAttribute<const char*>(const char* pAttrib) const noexcept(false);
		template<> Types::BaseType* Node::GetAttribute<Types::BaseType*>(const char* pAttrib) const noexcept(false);
	}	/* namespace Elements */
}	/* namespace XSD */

//#include "./src/XSDParser/Elements/Node.inc"
#endif /* NODE_HPP_ */
