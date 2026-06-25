/*
 * ElementExtracter.hpp
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
 
#ifndef ELEMENTEXTRACTER_HPP_
#define ELEMENTEXTRACTER_HPP_

#include <vector>
#include "./src/XSDParser/Types.hpp"
#include "./src/Processors/LuaProcessorBase.hpp"

namespace Processors {
	class ElementExtracter : public LuaProcessorBase {
	public:
		typedef std::vector<XSD::Elements::Element*> ElementLst;
		ElementExtracter();
		ElementExtracter(const ElementExtracter& rProccessor);
		virtual ~ElementExtracter();
		const ElementLst& Extract(const XSD::Elements::Schema& rDocRoot);
		virtual void ProcessSchema(const XSD::Elements::Schema* pNode);
		virtual void ProcessElement(const XSD::Elements::Element* pNode);
		virtual void ProcessInclude(const XSD::Elements::Include* pNode);
	private:
		ElementLst elementLst_;
	};
}
#endif /* ELEMENTEXTRACTER_HPP_ */
