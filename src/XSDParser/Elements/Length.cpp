/*
 * Length.cpp
 *
 *  Created on: Feb 21, 2014
 *      Author: QVXLabs LLC
 *   Copyright: (c)2014 QVXLabs LLC
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

#ifndef TIXML_USE_STL
#	define TIXML_USE_STL
#endif /* TIXML_USE_STL */
#include <memory>
#include <string.h>
#include <string>
#include <tinyxml.h>
#include "./src/XSDParser/Elements/Annotation.hpp"
#include "./src/XSDParser/Elements/Length.hpp"

using namespace XSD;
using namespace XSD::Elements;

Length::Length(const TiXmlElement& elm, const Parser& rParser)
	: FacetNode(elm, rParser)
{ }

Length::Length(const Length& cpy)
	: FacetNode(cpy)
{ }

void
Length::ParseElement(BaseProcessor& rProcessor) const noexcept(false) {
	rProcessor.ProcessLength(this);
}

