[@lua
local function UpperFirstChar(str)
   return (str:gsub("^%l", string.upper))
end

ListItemStrategy = {
   declaration = function(type, var)
		    local fmt = '\tprivate List<%s> _%s = new ArrayList<%s>();\n'
		    return fmt:format(type.typename, var, type.typename)
		 end,
   marshal = function(type, var, tag)
		 local str = {}
		 local fmt = {
		    '\t\t{\n',
		    '\t\t\tJSONArrayAdapter jArray = new JSONArrayAdapter(new JSONArray());\n',
		    '\t\t\tfor(%s obj : _%s)\n',
		    '\t\t\t\tjArray.%s(obj);\n',
		    '\t\t\tretObj.put(\"%s\", jArray);\n',
		    '\t\t}\n'
		 }
		 str[1] = fmt[1]
		 str[2] = fmt[2]
		 str[3] = fmt[3]:format(type.typename, var)
		 str[4] = fmt[4]:format(type.marshal)
		 str[5] = fmt[5]:format(tag)
		 str[6] = fmt[6]
		 return table.concat(str)
	      end,
   unmarshal = function(type, var, tag)
		   local str = {}
		   if type.metaInfo['primative'] then
			local fmt = {
				'\t\t{\n',
				'\t\t\tJSONArrayAdapter jArray = jObj.getList(\"%s\");\n',
				'\t\t\tfor (int i = 0; i < jArray.length(); ++i)\n',
				'\t\t\t\t_%s.add(jArray.%s(i));\n',
				'\t\t}\n'
			}
			str[1] = fmt[1]
			str[2] = fmt[2]:format(tag)
			str[3] = fmt[3]
			str[4] = fmt[4]:format(var, type.unmarshal)
			str[5] = fmt[5]
		   else
			local fmt = {
				'\t\t{\n',
				'\t\t\tJSONArrayAdapter jArray = jObj.getList(\"%s\");\n',
				'\t\t\tfor (int i = 0; i < jArray.length(); ++i)\n',
				'\t\t\t\t_%s.add(new %s(jArray.%s(i)));\n',
				'\t\t}\n'
			}
			str[1] = fmt[1]
			str[2] = fmt[2]:format(tag)
			str[3] = fmt[3]
			str[4] = fmt[4]:format(var, type.typename, type.unmarshal)
			str[5] = fmt[5]
		   end
		   return table.concat(str)
		end,
   geter = function(type, var)
	      local str = {}
	      local fmt = {
		 '\tpublic List<%s> get%s() {\n',
		 '\t\treturn _%s;\n',
		 '\t}\n'
	      }
	      str[1] = fmt[1]:format(type.typename, UpperFirstChar(var))
	      str[2] = fmt[2]:format(var)
	      str[3] = fmt[3]
	      return table.concat(str)
	   end,
   seter = function(type, var)
	      local str = {}
	      local fmt = {
		 '\tpublic void set%s(List<%s> %sVal) {\n',
		 '\t\t_%s = %sVal;\n',
		 '\t}\n'
	      }
	      str[1] = fmt[1]:format(UpperFirstChar(var), type.typename, var)
	      str[2] = fmt[2]:format(var, var)
	      str[3] = fmt[3]
	      return table.concat(str)	      
	   end
}
]