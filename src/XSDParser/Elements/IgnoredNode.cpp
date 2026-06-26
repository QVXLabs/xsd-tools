/*
 * IgnoredNode.cpp
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

#include "./src/XSDParser/Elements/IgnoredNode.hpp"

using namespace XSD;
using namespace XSD::Elements;

IgnoredNode::IgnoredNode(const TiXmlElement& elm, const Parser& rParser)
	: Node(elm, rParser)
{ }

IgnoredNode::IgnoredNode(const IgnoredNode& cpy)
	: Node(cpy)
{ }

/* no-op: a codegen-irrelevant construct contributes nothing and is not
   recursed into (so e.g. key/selector/field grandchildren are never visited) */
void
IgnoredNode::ParseChildren(BaseProcessor& rProcessor) const noexcept(false)
{ }

void
IgnoredNode::ParseElement(BaseProcessor& rProcessor) const noexcept(false)
{ }
