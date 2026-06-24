/*
 * ElementExtracter.cpp
 *
 *  Created on: 02/28/14
 *      Author: Ardavon Falls
 *   Copyright: (c)2012 Ardavon Falls
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
 *  along with Xsd-Tools.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "./src/XSDParser/Elements/Element.hpp"
#include "./src/XSDParser/Elements/Schema.hpp"
#include "./src/XSDParser/Elements/Annotation.hpp"
#include "./src/XSDParser/Elements/SimpleType.hpp"
#include "./src/XSDParser/Elements/ComplexType.hpp"
#include "./src/XSDParser/Elements/Group.hpp"
#include "./src/XSDParser/Elements/Include.hpp"
#include "./src/Processors/ElementExtracter.hpp"

using namespace std;
using namespace Processors;
using namespace XSD::Elements;

ElementExtracter::ElementExtracter()
	: LuaProcessorBase(NULL), m_elementLst()
{ }

ElementExtracter::ElementExtracter(const ElementExtracter& rProcessor)
 	: LuaProcessorBase(rProcessor), m_elementLst(rProcessor.m_elementLst)
 {}
 
/* virtual */
ElementExtracter::~ElementExtracter() {
	m_elementLst.clear();
}

const ElementExtracter::ElementLst&
ElementExtracter::Extract(const Schema& rDocRoot) {
	ProcessSchema(&rDocRoot);
	return m_elementLst;
}

/* virtual */ void 
ElementExtracter::ProcessSchema(const Schema* pNode) {
	/* different iterator accross the schema children */
	pNode->_eachChild([&](const Node& rNode) {
		if (XSD_ISELEMENT(&rNode, Element) ||
			XSD_ISELEMENT(&rNode, Annotation) ||
			XSD_ISELEMENT(&rNode, Include) ||
			XSD_ISELEMENT(&rNode, SimpleType) ||
			XSD_ISELEMENT(&rNode, ComplexType) ||
			XSD_ISELEMENT(&rNode, Group)) {
			rNode.ParseElement(*this);
		}
	});
}

/* virtual */ void 
ElementExtracter::ProcessElement(const Element* pNode) {
	m_elementLst.push_back(new Element(*pNode));
}

/* virtual */ void 
ElementExtracter::ProcessInclude(const Include* pNode) {
	const Schema * pSchema = pNode->QuerySchema();
	pSchema->ParseChildren(*this);
}
