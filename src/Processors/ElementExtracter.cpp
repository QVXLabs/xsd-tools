/*
 * ElementExtracter.cpp
 *
 *  Created on: 02/28/14
 *      Author: QVXLabs LLC
 *   Copyright: (c)2012 QVXLabs LLC
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
	: LuaProcessorBase(NULL), elementLst_()
{ }

ElementExtracter::ElementExtracter(const ElementExtracter& rProcessor)
 	: LuaProcessorBase(rProcessor), elementLst_(rProcessor.elementLst_)
 {}
 
/* virtual */
ElementExtracter::~ElementExtracter() {
	elementLst_.clear();
}

const ElementExtracter::ElementLst&
ElementExtracter::Extract(const Schema& rDocRoot) {
	ProcessSchema(&rDocRoot);
	return elementLst_;
}

/* virtual */ void 
ElementExtracter::ProcessSchema(const Schema* pNode) {
	/* different iterator accross the schema children */
	pNode->eachChild_([&](const Node& rNode) {
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
	elementLst_.push_back(new Element(*pNode));
}

/* virtual */ void 
ElementExtracter::ProcessInclude(const Include* pNode) {
	const Schema * pSchema = pNode->QuerySchema();
	pSchema->ParseChildren(*this);
}
