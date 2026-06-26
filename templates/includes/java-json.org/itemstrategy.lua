[@lua
local function UpperFirstChar(str)
   return (str:gsub("^%l", string.upper))
end

ItemStrategy = {
   declaration = function(type, var, default)
		    --dbgPrint('ItemStrategy.declaraion(',type,')')
		    if type.metaInfo['primative'] then
		       if nil ~= default then
				  -- `default` is the caller-formatted Java literal
				  local fmt = '\tprivate %s _%s = %s;\n'
				  return fmt:format(type.typename, var, default)
		       else
				  local fmt = '\tprivate %s _%s;\n'
				  return fmt:format(type.typename, var)
		       end
		    else
		       if nil ~= default then
				  local fmt = '\tprivate %s _%s = new %s(%s);\n'
				  return fmt:format(
					 type.typename,
					 var,
					 type.typename,
					 type.declrFmt:format(default)
				  )
		       else
				  local fmt = '\tprivate %s _%s = null;\n'
				  return fmt:format(
					 type.typename, 
					 var
				  )
		       end
		    end
		 end,
   marshal = function(type, var, tag)
         if type.metaInfo['primative'] then
		    local fmt = '\t\tretObj.%s(\"%s\", _%s);\n'
		    return fmt:format(type.marshal, tag, var)
         else
		    local fmt = '\t\tretObj.%s(\"%s\", _%s.marshal());\n'
		    return fmt:format(type.marshal, tag, var)

         end
	      end,
   unmarshal = function(type, var, tag)
           if type.metaInfo['primative'] then
		        local fmt = '\t\t_%s= jObj.%s(\"%s\");\n'
		        return fmt:format(var, type.unmarshal, tag)
            else
		        local fmt = '\t\t_%s= new %s(jObj.%s(\"%s\"));\n'
                return fmt:format(var, type.typename, type.unmarshal, tag)
           end

		end,
   geter = function(type, var)
	      local str = {}
	      local fmt = {
		 '\tpublic %s get%s() {\n',
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
		 '\tpublic void set%s(%s %sVal) {\n',
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
