/*
 * XSDTypes.hpp
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
 *  along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#ifndef XSDTYPES_HPP_
#define XSDTYPES_HPP_
#include <stdint.h>
#include <string>
#include <typeinfo>

#define XSD_VTYPEDEF(TYPE,BASE,NAME) struct TYPE : public BASE { \
    virtual const char* Name() const {				 \
      return #NAME;						 \
    }								 \
  }
#define XSD_TYPEDEF(TYPE,BASE,CTYPE,NAME)	struct TYPE : public BASE { \
    typedef CTYPE cType;						\
    virtual BaseType* clone() const {					\
      return new TYPE();						\
    }									\
    virtual bool isTypeRelated(const BaseType* pType) const {		\
      return (NULL != dynamic_cast<const TYPE*>(pType));		\
    }									\
    virtual const char* Name() const {					\
      return #NAME;							\
    }									\
  }
#define XSD_ISTYPE(TYPE_PTR,TYPE)		typeid(*TYPE_PTR) == typeid(TYPE)


/* forward declared to remove cyclical header dependencies */
namespace XSD {
  namespace Elements {
    class SimpleType;
    class ComplexType;
  }
}
/* XSD type definitions */
namespace XSD {
  namespace Types {
    struct BaseType {
      virtual ~BaseType() {}
      virtual BaseType* clone() const = 0;
      virtual bool isTypeRelated(const BaseType*) const = 0;
      virtual const char* Name() const = 0;
    };
    XSD_VTYPEDEF(AnySimpleType,BaseType, anySimpleType);
    XSD_VTYPEDEF(AnyAtomicType,AnySimpleType, anyAtomicType);
    XSD_TYPEDEF(AnyURI,AnyAtomicType,std::string, anyURI);
    XSD_TYPEDEF(Base64Binary,AnyAtomicType,std::string, base64Binary);
    XSD_TYPEDEF(Boolean,AnyAtomicType,bool, boolean);
    XSD_TYPEDEF(Date,AnyAtomicType,std::string, date);
    XSD_TYPEDEF(DateTime,AnyAtomicType,std::string, dateTime);
    XSD_TYPEDEF(DateTimeStamp,DateTime,std::string, dateTimeStamp);
    XSD_TYPEDEF(Decimal,AnyAtomicType,long double, decimal);
    XSD_TYPEDEF(Integer,Decimal,int64_t, integer);
    XSD_TYPEDEF(Long,Integer,int64_t, long);
    XSD_TYPEDEF(Int,Long,int32_t, int);
    XSD_TYPEDEF(Short,Int,int16_t, short);
    XSD_TYPEDEF(Byte,Short,int8_t, byte);
    XSD_TYPEDEF(NonNegativeInteger,Integer,uint64_t, nonNegativeInteger);
    XSD_TYPEDEF(PositiveInteger,NonNegativeInteger,uint64_t, positiveInteger);
    XSD_TYPEDEF(UnsignedLong,NonNegativeInteger,uint64_t, unsignedLong);
    XSD_TYPEDEF(UnsignedInt,UnsignedLong,uint32_t, unsignedInt);
    XSD_TYPEDEF(UnsignedShort,UnsignedInt,uint16_t, unsignedShort);
    XSD_TYPEDEF(UnsignedByte,UnsignedShort,uint8_t, unsignedByte);
    XSD_TYPEDEF(NonPositiveInteger,Integer,int64_t, nonPositiveInteger);
    XSD_TYPEDEF(NegativeInteger,NonPositiveInteger,int64_t, negativeInteger);
    XSD_TYPEDEF(Double,AnyAtomicType,double, double);
    XSD_TYPEDEF(Duration,AnyAtomicType,std::string, duration);
    XSD_TYPEDEF(DayTimeDuration,Duration,std::string, dayTimeDuration);
    XSD_TYPEDEF(YearMonthDuration,Duration,std::string, yearMonthDuration);
    XSD_TYPEDEF(Float,AnyAtomicType,float, float);
    XSD_TYPEDEF(gDay,AnyAtomicType,std::string, gDay);
    XSD_TYPEDEF(gMonth,AnyAtomicType,std::string, gMonth);
    XSD_TYPEDEF(gMonthDay,AnyAtomicType,std::string, gMonthDay);
    XSD_TYPEDEF(gYear,AnyAtomicType,std::string, gYear);
    XSD_TYPEDEF(gYearMonth,AnyAtomicType,std::string, gYearMonth);
    XSD_TYPEDEF(HexBinary,AnyAtomicType,std::string, hexBinary);
    XSD_TYPEDEF(Notation,AnyAtomicType,std::string, NOTATION);
    XSD_TYPEDEF(PrecisionDecimal,AnyAtomicType,long double, precisionDecimal);
    XSD_TYPEDEF(QName,AnyAtomicType,std::string, QName);
    XSD_TYPEDEF(String,AnyAtomicType,std::string, string);
    XSD_TYPEDEF(NormalizedString,String,std::string, normalizedString);
    XSD_TYPEDEF(Token,String,std::string, token);
    XSD_TYPEDEF(Language,Token,std::string, language);
    XSD_TYPEDEF(XSDName,Token,std::string, name);
    XSD_TYPEDEF(NCName,XSDName,std::string, NCName);
    XSD_TYPEDEF(Entity,NCName,std::string, ENTITY);
    XSD_TYPEDEF(ID,NCName,std::string, ID);
    XSD_TYPEDEF(IDRef,NCName,std::string, IDREF);
    XSD_TYPEDEF(NMToken,Token,std::string, NMTOKEN);
    XSD_TYPEDEF(Time,AnyAtomicType,std::string, time);
    XSD_TYPEDEF(Entities,AnyAtomicType,std::string, ENTITIES);
    XSD_TYPEDEF(IDRefs,AnyAtomicType,std::string, IDREFS);
    XSD_TYPEDEF(NMTokens,AnyAtomicType,std::string, NMTOKENS);
    XSD_TYPEDEF(Unsupported,AnySimpleType,void*, Unsupported);
    XSD_TYPEDEF(Unknown,AnySimpleType,void*, Unknown);
    struct SimpleType : public BaseType {
      Elements::SimpleType* m_pValue;
      mutable std::string   m_name;
      inline SimpleType(Elements::SimpleType* pElm)
	: m_pValue(pElm), m_name("Unnamed") {} 
      virtual BaseType* clone() const;
      virtual bool isTypeRelated(const BaseType* pType) const;
      virtual const char* Name() const;
      virtual ~SimpleType();
    };
    struct ComplexType : public BaseType {
      Elements::ComplexType* m_pValue;
      mutable std::string    m_name;
      inline ComplexType(Elements::ComplexType* pElm)
        : m_pValue(pElm), m_name("Unnamed") {}
      virtual BaseType* clone() const;
      virtual bool isTypeRelated(const BaseType* pType) const;
      virtual const char* Name() const;
      virtual ~ComplexType();
    };
  } /* namespace Types */
} /* namespace XSD */
#endif /* XSDTYPES_HPP_ */
