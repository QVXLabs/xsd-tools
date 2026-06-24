/*
 * AppInfo.cpp
 *
 *  Created on: Feb 27, 2014
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
 *  along with Xsd-Tools.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TIXML_USE_STL
#	define TIXML_USE_STL
#endif /* TIXML_USE_STL */
#include <memory>
#include <string.h>
#include <string>
#include <tinyxml.h>
#include "./src/XSDParser/Elements/AppInfo.hpp"

using namespace XSD;
using namespace XSD::Elements;

AppInfo::AppInfo(const TiXmlElement& elm, const Parser& rParser)
	: Node(elm, rParser)
{ }

AppInfo::AppInfo(const AppInfo& cpy)
	: Node(cpy)
{ }

void
AppInfo::ParseChildren(BaseProcessor& rProcessor) const noexcept(false) {
	/* does nothing */
}

void
AppInfo::ParseElement(BaseProcessor& rProcessor) const noexcept(false) {
	rProcessor.ProcessAppInfo(this);
}

Types::BaseType * 
AppInfo::GetParentType() const noexcept(false) {
	std::unique_ptr<Node> pParent(Node::Parent());
	return pParent->GetParentType();
}