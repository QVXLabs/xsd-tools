/*
 * SimpleTypeExtracter.hpp
 *
 *  Created on: 01/23/12
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
 
#ifndef SIMPLETYPEEXTRACTER_HPP_
#define SIMPLETYPEEXTRACTER_HPP_

#include "./src/XSDParser/Types.hpp"
#include "./src/Processors/LuaProcessorBase.hpp"

namespace Processors {
	class SimpleTypeExtracter : public LuaProcessorBase {
	public:
		SimpleTypeExtracter();
		SimpleTypeExtracter(const SimpleTypeExtracter& rProccessor);
		virtual ~SimpleTypeExtracter();
		const XSD::Types::BaseType& Extract(const XSD::Types::BaseType& rXSDType);
		virtual void ProcessUnion(const XSD::Elements::Union* pNode);
		virtual void ProcessRestriction(const XSD::Elements::Restriction* pNode );
		virtual void ProcessList(const XSD::Elements::List* pNode);
		virtual void ProcessInclude(const XSD::Elements::Include* pNode);
	private:
		XSD::Types::BaseType* m_pType;
		void parseType_(const XSD::Types::BaseType& rXSDType);
	};
}
#endif /* SIMPLETYPEEXTRACTER_HPP_ */
