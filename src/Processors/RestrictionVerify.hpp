/*
 * RestrictionVerify.hpp
 *
 *  Created on: 3/3/14
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
 
#ifndef RESTRICTIONVERIFY_HPP_
#define RESTRICTIONVERIFY_HPP_

#include <vector>
#include "./src/XSDParser/Types.hpp"
#include "./src/Processors/LuaProcessorBase.hpp"
#include "./src/XSDParser/Exception.hpp"
#include "./src/XSDParser/Elements/Node.hpp"

namespace Processors {
	class RestrictionVerify : public LuaProcessorBase {
	public:
		RestrictionVerify();
		RestrictionVerify(const RestrictionVerify& rProccessor);
		virtual ~RestrictionVerify();
		bool Verify(const XSD::Elements::Restriction* pRestriciton, const XSD::Elements::ComplexType* pParentType) noexcept(false);
		virtual void ProcessElement(const XSD::Elements::Element* pNode);
		virtual void ProcessRestriction(const XSD::Elements::Restriction* pNode);
		virtual void ProcessSequence(const XSD::Elements::Sequence* pNode);
		virtual void ProcessChoice(const XSD::Elements::Choice* pNode);
		virtual void ProcessGroup(const XSD::Elements::Group* pNode);
		virtual void ProcessAll(const XSD::Elements::All* pNode);
	protected:
		RestrictionVerify(const XSD::Elements::Node* pSubTree);
	private:
		const XSD::Elements::Node * m_pSubTree;
	};
}
#endif /* RESTRICTIONVERIFY_HPP_ */
