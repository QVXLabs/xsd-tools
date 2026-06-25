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
 *  along with xsd-tools.  If not, see <http://www.gnu.org/licenses/>.
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
    /* Single source of truth for the registerable builtin primitives:
     * X(TYPE, BASE, CTYPE, NAME, KEY). NAME is the type's Name() string;
     * KEY is its TypesDB lookup name (equal to NAME except XSDName). Both
     * Types.hpp (struct defs below) and TypesDB.cpp (registration) expand
     * this list, so the two can never drift. */
    #define XSD_BUILTIN_TYPES(X)						\
      X(AnyURI,AnyAtomicType,std::string, anyURI, "anyURI")		\
      X(Base64Binary,AnyAtomicType,std::string, base64Binary, "base64Binary")	\
      X(Boolean,AnyAtomicType,bool, boolean, "boolean")			\
      X(Date,AnyAtomicType,std::string, date, "date")			\
      X(DateTime,AnyAtomicType,std::string, dateTime, "dateTime")	\
      X(DateTimeStamp,DateTime,std::string, dateTimeStamp, "dateTimeStamp")	\
      X(Decimal,AnyAtomicType,long double, decimal, "decimal")		\
      X(Integer,Decimal,int64_t, integer, "integer")			\
      X(Long,Integer,int64_t, long, "long")				\
      X(Int,Long,int32_t, int, "int")					\
      X(Short,Int,int16_t, short, "short")				\
      X(Byte,Short,int8_t, byte, "byte")				\
      X(NonNegativeInteger,Integer,uint64_t, nonNegativeInteger, "nonNegativeInteger")	\
      X(PositiveInteger,NonNegativeInteger,uint64_t, positiveInteger, "positiveInteger")	\
      X(UnsignedLong,NonNegativeInteger,uint64_t, unsignedLong, "unsignedLong")	\
      X(UnsignedInt,UnsignedLong,uint32_t, unsignedInt, "unsignedInt")	\
      X(UnsignedShort,UnsignedInt,uint16_t, unsignedShort, "unsignedShort")	\
      X(UnsignedByte,UnsignedShort,uint8_t, unsignedByte, "unsignedByte")	\
      X(NonPositiveInteger,Integer,int64_t, nonPositiveInteger, "nonPositiveInteger")	\
      X(NegativeInteger,NonPositiveInteger,int64_t, negativeInteger, "negativeInteger")	\
      X(Double,AnyAtomicType,double, double, "double")			\
      X(Duration,AnyAtomicType,std::string, duration, "duration")	\
      X(DayTimeDuration,Duration,std::string, dayTimeDuration, "dayTimeDuration")	\
      X(YearMonthDuration,Duration,std::string, yearMonthDuration, "yearMonthDuration")	\
      X(Float,AnyAtomicType,float, float, "float")			\
      X(gDay,AnyAtomicType,std::string, gDay, "gDay")			\
      X(gMonth,AnyAtomicType,std::string, gMonth, "gMonth")		\
      X(gMonthDay,AnyAtomicType,std::string, gMonthDay, "gMonthDay")	\
      X(gYear,AnyAtomicType,std::string, gYear, "gYear")		\
      X(gYearMonth,AnyAtomicType,std::string, gYearMonth, "gYearMonth")	\
      X(HexBinary,AnyAtomicType,std::string, hexBinary, "hexBinary")	\
      X(Notation,AnyAtomicType,std::string, NOTATION, "NOTATION")	\
      X(PrecisionDecimal,AnyAtomicType,long double, precisionDecimal, "precisionDecimal")	\
      X(QName,AnyAtomicType,std::string, QName, "QName")		\
      X(String,AnyAtomicType,std::string, string, "string")		\
      X(NormalizedString,String,std::string, normalizedString, "normalizedString")	\
      X(Token,String,std::string, token, "token")			\
      X(Language,Token,std::string, language, "language")		\
      X(XSDName,Token,std::string, name, "Name")			\
      X(NCName,XSDName,std::string, NCName, "NCName")			\
      X(Entity,NCName,std::string, ENTITY, "ENTITY")			\
      X(ID,NCName,std::string, ID, "ID")				\
      X(IDRef,NCName,std::string, IDREF, "IDREF")			\
      X(NMToken,Token,std::string, NMTOKEN, "NMTOKEN")			\
      X(Time,AnyAtomicType,std::string, time, "time")			\
      X(Entities,AnyAtomicType,std::string, ENTITIES, "ENTITIES")	\
      X(IDRefs,AnyAtomicType,std::string, IDREFS, "IDREFS")		\
      X(NMTokens,AnyAtomicType,std::string, NMTOKENS, "NMTOKENS")
    /* Define a struct per builtin (drop the registration KEY field). */
    #define XSD_DEFINE_TYPE(TYPE,BASE,CTYPE,NAME,KEY)	\
      XSD_TYPEDEF(TYPE,BASE,CTYPE,NAME);
    XSD_BUILTIN_TYPES(XSD_DEFINE_TYPE)
    #undef XSD_DEFINE_TYPE
    /* Sentinels: not registered in TypesDB (see TypesDB.cpp). */
    XSD_TYPEDEF(Unsupported,AnySimpleType,void*, Unsupported);
    XSD_TYPEDEF(Unknown,AnySimpleType,void*, Unknown);
    /* Shared body for the two XSD-element-backed wrapper types. Derived is
     * the concrete wrapper (for clone()/typeid); Elm is its element type.
     * clone/isTypeRelated/Name/dtor live in Types.cpp via the WRAPPED_TYPE
     * macro so the vtable is keyed on the concrete type. */
    template<typename Derived, typename Elm>
    struct WrappedType : public BaseType {
      Elm* pValue_;
      mutable std::string name_;
      inline WrappedType(Elm* pElm)
        : pValue_(pElm), name_("Unnamed") {}
    };
    struct SimpleType : public WrappedType<SimpleType, Elements::SimpleType> {
      inline SimpleType(Elements::SimpleType* pElm) : WrappedType(pElm) {}
      virtual BaseType* clone() const;
      virtual bool isTypeRelated(const BaseType* pType) const;
      virtual const char* Name() const;
      virtual ~SimpleType();
    };
    struct ComplexType
        : public WrappedType<ComplexType, Elements::ComplexType> {
      inline ComplexType(Elements::ComplexType* pElm) : WrappedType(pElm) {}
      virtual BaseType* clone() const;
      virtual bool isTypeRelated(const BaseType* pType) const;
      virtual const char* Name() const;
      virtual ~ComplexType();
    };
  } /* namespace Types */
} /* namespace XSD */
#endif /* XSDTYPES_HPP_ */
