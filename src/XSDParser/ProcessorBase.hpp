/*
 * ProcessorBase.hpp
 *
 *  Created on: Jun 24, 2011
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

#ifndef PROCESSORBASE_HPP_
#define PROCESSORBASE_HPP_

namespace XSD {
	namespace Elements {
		class Schema;
		class Element;
		class Union;
		class Restriction;
		class List;
		class Sequence;
		class Choice;
		class Attribute;
		class SimpleType;
		class ComplexType;
		class Group;
		class Any;
		class ComplexContent;
		class Extension;
		class SimpleContent;
		class MinExclusive;
		class MaxExclusive;
		class MinInclusive;
		class MaxInclusive;
		class MinLength;
		class MaxLength;
		class Length;
		class Enumeration;
		class FractionDigits;
		class Pattern;
		class TotalDigits;
		class WhiteSpace;
		class AttributeGroup;
		class Include;
		class Annotation;
		class Documentation;
		class All;
		class AppInfo;
	}
	class BaseProcessor {
	public:
		virtual ~BaseProcessor() {};
		virtual void ProcessSchema(const Elements::Schema* pNode) = 0;
		virtual void ProcessElement(const Elements::Element* pNode) = 0;
		virtual void ProcessUnion(const Elements::Union* pNode) = 0;
		virtual void ProcessRestriction(const Elements::Restriction* pNode ) = 0;
		virtual void ProcessList(const Elements::List* pNode) =0;
		virtual void ProcessSequence(const Elements::Sequence* pNode) = 0;
		virtual void ProcessChoice(const Elements::Choice* pNode ) = 0;
		virtual void ProcessAttribute(const Elements::Attribute* pNode) = 0;
		virtual void ProcessSimpleType(const Elements::SimpleType* pNode) = 0;
		virtual void ProcessComplexType(const Elements::ComplexType* pNode) = 0;
		virtual void ProcessGroup(const Elements::Group* pNode) = 0;
		virtual void ProcessAny(const Elements::Any* pNode) = 0;
		virtual void ProcessComplexContent(const Elements::ComplexContent* pNode) = 0;
		virtual void ProcessExtension(const Elements::Extension* pNode) = 0;
		virtual void ProcessSimpleContent(const Elements::SimpleContent* pNode) = 0;
		virtual void ProcessMinExclusive(const Elements::MinExclusive* pNode) = 0;
		virtual void ProcessMaxExclusive(const Elements::MaxExclusive* pNode) = 0;
		virtual void ProcessMinInclusive(const Elements::MinInclusive* pNode) = 0;
		virtual void ProcessMaxInclusive(const Elements::MaxInclusive* pNode) = 0;
		virtual void ProcessMinLength(const Elements::MinLength* pNode) = 0;
		virtual void ProcessMaxLength(const Elements::MaxLength* pNode) = 0;
		virtual void ProcessLength(const Elements::Length* pNode) = 0;
		virtual void ProcessEnumeration(const Elements::Enumeration* pNode) = 0;
		virtual void ProcessFractionDigits(const Elements::FractionDigits* pNode) = 0;
		virtual void ProcessPattern(const Elements::Pattern* pNode) = 0;
		virtual void ProcessTotalDigits(const Elements::TotalDigits* pNode) = 0;
		virtual void ProcessWhiteSpace(const Elements::WhiteSpace* pNode) = 0;
		virtual void ProcessAttributeGroup(const Elements::AttributeGroup* pNode) = 0;
		virtual void ProcessInclude(const Elements::Include* pNode) = 0;
		virtual void ProcessAnnotation(const Elements::Annotation* pNode) = 0;
		virtual void ProcessDocumentation(const Elements::Documentation* pNode) = 0;
		virtual void ProcessAll(const Elements::All* pNode) = 0;
		virtual void ProcessAppInfo(const Elements::AppInfo* pNode) = 0;
	};
} /* namespace XSD */
#endif /* PROCESSORBASE_HPP_ */
