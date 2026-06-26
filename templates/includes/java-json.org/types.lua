[@lua
include 'includes/java-json.org/string.lua'
include 'includes/java-json.org/int32.lua'
include 'includes/java-json.org/integer.lua'
include 'includes/java-json.org/double.lua'
include 'includes/java-json.org/boolean.lua'
include 'includes/java-json.org/base64Binary.lua'
include 'includes/java-json.org/hexBinary.lua'

local types_meta = {
   __index = function(table, key)
		return {
		   typename = key,
		   marshal = 'put',
		   unmarshal = 'getObject',
		   metaInfo = { primative = false }
		}
	     end
}

types = {
   anySimpleType     = type_string,
   anyAtomicType     = type_string,
   anyURI            = type_string,
   boolean           = type_boolean,
   date              = type_string,
   dateTime          = type_string,
   dateTimeStamp     = type_string,
   duration          = type_string,
   dayTimeDuration   = type_string,
   yearMonthDuration = type_string,
   gDay              = type_string,
   gMonth            = type_string,
   gMonthDay         = type_string,
   gYear             = type_string,
   gMonthYear        = type_string,
   NOTATION          = type_string,
   QName             = type_string,   
   string            = type_string,
   normalizedString  = type_string,
   token             = type_string,
   language          = type_string,
   name              = type_string,
   NCName            = type_string,
   ENTITY            = type_string,
   ID                = type_string,
   IDREF             = type_string,
   NMTOKEN           = type_string,
   time              = type_string,
   ENTITIES          = type_string,
   IDREFS            = type_string,
   NMTOKENS          = type_string,
   decimal           = type_double,   
   integer           = type_integer,
   long              = type_integer,
   int               = type_int32,
   short             = type_int32,
   byte              = type_int32,
   positiveInteger   = type_integer,
   unsignedLong      = type_integer,
   unsignedInt       = type_int32,
   unsignedShort     = type_int32,
   unsignedByte      = type_int32,
   negativeInteger   = type_int32,   
   double            = type_double,
   float             = type_double,
   percisionDecimal  = type_double,
   nonPositiveInteger= type_integer,
   nonNegativeInteger= type_integer,
   base64Binary      = type_base64binary,
   hexBinary         = type_hexBinary
}

setmetatable(types, types_meta)
]