[@lua
   include 'includes/java-json.org/itemstrategy.lua'
   include 'includes/java-json.org/listitemstrategy.lua'
   include 'includes/java-json.org/types.lua'
   include 'includes/java-json.org/adapters.lua'
   include 'shared/doc'

   -- TODO: 1) use optJSONArray/opt... for optional fields
   --       2) make unmarshalling/marshalling handle fields which can be lists
   --          of one, in which case they are not in brackets

   -- helper functions
   function isSimpleType(typedef) 
	  return (next(typedef.fields) == nil)
   end

    function isRootElement(schema, type)                                   
       for _, v in pairs(schema) do                                              
          local elemName, elemType = next(v)                                     
          if elemType == type then return true end                               
       end                                                                       
       return false;                                                             
    end  

   local function isListType(typename)
	  return (typename:match("^(list)%(.+%)") ~= nil)
   end

   local function isFieldRequired(typedef)
	  return (typedef.use and "required" == typedef.use)
   end

   local function getListType(typeName)
	  return typeName:match("list%((.+)%)")
   end
   
   local function fieldIterator(tbl)
	  local function itr(tbl, i)
		 local jsonTag, typeTbl = next(tbl, i)
		 return jsonTag, unpack(jsonTag and {next(typeTbl)} or {})
	  end
	  return itr, tbl, nil
   end

   -- Java keywords + java.lang.Object methods whose bare name (or, via the
   -- get-prefix, `getClass`) would collide with a generated accessor. Seeded
   -- into every per-class field namespace so a JSON tag like `class` can't
   -- produce an illegal field or an override of a final Object method.
   local javaReserved = {
	  ['abstract']=true, ['assert']=true, ['boolean']=true, ['break']=true,
	  ['byte']=true, ['case']=true, ['catch']=true, ['char']=true,
	  ['class']=true, ['const']=true, ['continue']=true, ['default']=true,
	  ['do']=true, ['double']=true, ['else']=true, ['enum']=true,
	  ['extends']=true, ['final']=true, ['finally']=true, ['float']=true,
	  ['for']=true, ['goto']=true, ['if']=true, ['implements']=true,
	  ['import']=true, ['instanceof']=true, ['int']=true, ['interface']=true,
	  ['long']=true, ['native']=true, ['new']=true, ['package']=true,
	  ['private']=true, ['protected']=true, ['public']=true, ['return']=true,
	  ['short']=true, ['static']=true, ['strictfp']=true, ['super']=true,
	  ['switch']=true, ['synchronized']=true, ['this']=true, ['throw']=true,
	  ['throws']=true, ['transient']=true, ['try']=true, ['void']=true,
	  ['volatile']=true, ['while']=true, ['true']=true, ['false']=true,
	  ['null']=true, ['record']=true, ['var']=true, ['yield']=true,
	  ['getClass']=true, ['hashCode']=true, ['equals']=true, ['clone']=true,
	  ['toString']=true, ['wait']=true, ['notify']=true, ['finalize']=true,
   }

   -- Map an arbitrary JSON tag to a valid, collision-free Java identifier.
   -- Mirrors shared/schemaEx sanitizeIdent: illegal chars -> '_', leading
   -- digit prefixed, then de-collided against `reserved` (which is mutated so
   -- repeated calls within a class stay distinct).
   local function sanitizeIdent(name, reserved)
	  if '$' == name then name = 'value' end
	  local ident = tostring(name):gsub('[^%w_]', '_')
	  if ident:match('^%d') then ident = '_'..ident end
	  if ident == '' then ident = '_' end
	  local base, out, n = ident, ident, 0
	  while reserved[out] do n = n + 1; out = base..'_'..n end
	  reserved[out] = true
	  return out
   end

   -- Stable per-class map from JSON tag -> Java member name. Built once in
   -- sorted-tag order so de-collision suffixes are deterministic and every
   -- emission loop sees the same identifier for a given field.
   local function classMemberNames(fields)
	  local reserved = {}
	  for k in pairs(javaReserved) do reserved[k] = true end
	  local tags = {}
	  for tag in pairs(fields) do tags[#tags + 1] = tag end
	  table.sort(tags)
	  local names = {}
	  for _, tag in ipairs(tags) do
		 names[tag] = sanitizeIdent(tag, reserved)
	  end
	  return names
   end

   -- Escape a raw value for a Java double-quoted string literal (D2/#4/#9).
   local function javaStrLit(raw)
	  local s = tostring(raw):gsub('\\', '\\\\'):gsub('"', '\\"')
		 :gsub('\n', '\\n'):gsub('\r', '\\r'):gsub('\t', '\\t')
	  return '"'..s..'"'
   end

   local function outputGeterJavadoc(typename, memberName)
	  local str = stringBuffer:new()
	  str:append('\t/**\n')
	  str:append(
		 ('\t * Gets the value of the %s property.\n'):format(memberName)
	  )
	  str:append('\t *\n')
	  str:append('\t * @return\n')
	  str:append('\t *     possible object is\n')
	  str:append(('\t *     {@link %s}\n'):format(typename))
	  str:append('\t */\n')
	  return str:str()
   end

   local function outputSeterJavadoc(typename, memberName)
	  local str = stringBuffer:new()
	  str:append('\t/**\n')
	  str:append(
		 ('\t * Sets the value of the %sVal property.\n'):format(memberName)
	  )
	  str:append('\t *\n')
	  str:append(('\t * @param %sVal\n'):format(memberName))
	  str:append('\t *     allowed object is\n')
	  str:append(('\t *     {@link %s}\n'):format(typename))
	  str:append('\t */\n')
	  return str:str()
   end


   -- Narrowest signed Java boxed type whose range holds the facet int range,
   -- or nil when no numeric facet constrains the field. Java has no unsigned,
   -- so e.g. 0..255 widens to Short.
   local javaNarrowType = facets_narrowestSigned

   -- Effective Java type for a field: narrows numeric members to the smallest
   -- signed boxed type the facet range allows. Byte/Short lack adapter
   -- accessors, so they carry an int cast (unmarshal) / intValue (marshal); the
   -- JSON number is identical, so round-trips are preserved.
   local function effectiveType(fieldType, fieldTypedef)
	  local base = types[fieldType]
	  if base.typename ~= 'Integer' and base.typename ~= 'Long' then
		 return base
	  end
	  local narrow = fieldTypedef and javaNarrowType(fieldTypedef.facets)
	  if not narrow or narrow == base.typename then return base end
	  local t = {typename = narrow, marshal = 'put', metaInfo = {primative = true}}
	  if narrow == 'Byte' or narrow == 'Short' then
		 t.unmarshalExpr = ('(%s) jObj.getInt'):format(narrow:lower())
		 t.marshalVal = '.intValue()'
	  else
		 t.unmarshal = (narrow == 'Integer') and 'getInt' or 'getLong'
	  end
	  return t
   end
   -- Expose for the binding template's adapter de-bloat collector.
   _effectiveType = effectiveType

   -- Java literal for an attribute `default` (C). Strings quote; Long needs an
   -- L suffix so `Long x = 5` boxes; narrowed/int types emit the bare number.
   local function defaultLiteral(eff, raw)
	  if eff.declrFmt == '"%s"' then return javaStrLit(raw) end
	  if eff.typename == 'java.math.BigInteger' then
		 return 'new java.math.BigInteger('..javaStrLit(raw)..')'
	  end
	  if eff.typename == 'Long' then return raw .. 'L' end
	  return raw
   end

   -- Java numeric boxed types we can enum-compare without stringifying.
   local _numericJavaType = {
	  Byte = true, Short = true, Integer = true, Long = true,
	  ['java.math.BigInteger'] = true,
   }

   -- Render facet checks (D2) on member `_var` into a validate() body. Range,
   -- length and enum are enforced; pattern (P3) is not. No-op for nil facets.
   -- `jType` is the member's Java typename; numeric enums compare numerically
   -- (no allocation), string enums test a `static final Set` recorded in
   -- `sets` (built once, not per call).
   local function javaChecks(var, facets, jType, sets)
	  local out = stringBuffer:new()
	  -- guard that throws unless the value-condition `cond` holds
	  local function guard(cond, msg)
		 out:append(('\t\tif (_%s != null && !(%s))\n'):format(var, cond))
		 out:append(('\t\t\tthrow new IllegalArgumentException("%s: %s");\n')
			:format(var, msg))
	  end
	  -- BigInteger has no relational operators; compare via compareTo/equals.
	  local isBig = (jType == 'java.math.BigInteger')
	  local function bigLit(v) return 'new java.math.BigInteger("'..v..'")' end
	  table.map(facets_checks(facets), function(_, c)
		 if c.kind == 'range' then
			local cond
			if isBig then
			   local lo = c.lo and ('_%s.compareTo(%s) >= 0')
				  :format(var, bigLit(c.lo))
			   local hi = c.hi and ('_%s.compareTo(%s) <= 0')
				  :format(var, bigLit(c.hi))
			   cond = (lo and hi and lo..' && '..hi) or lo or hi
			else
			   cond = c.lo and c.hi and
				  ('%s <= _%s && _%s <= %s'):format(c.lo, var, var, c.hi)
				  or c.lo and ('%s <= _%s'):format(c.lo, var)
				  or ('_%s <= %s'):format(var, c.hi)
			end
			guard(cond, 'out of range')
		 elseif c.kind == 'length' then
			local n, parts = ('String.valueOf(_%s).length()'):format(var), {}
			if c.exact then parts = {n .. ' == ' .. c.exact} end
			if c.min then parts[#parts + 1] = n .. ' >= ' .. c.min end
			if c.max then parts[#parts + 1] = n .. ' <= ' .. c.max end
			guard(table.concat(parts, ' && '), 'bad length')
		 elseif c.kind == 'enum' then
			if _numericJavaType[jType] then
			   local vals = {}
			   table.map(c.values, function(_, v)
				  vals[#vals + 1] = isBig
					 and ('_%s.equals(%s)'):format(var, bigLit(v))
					 or ('_%s == %s'):format(var, v)
			   end)
			   guard('(' .. table.concat(vals, ' || ') .. ')', 'not permitted')
			else
			   -- string enum: membership in a once-built static Set. Values are
			   -- Java-escaped so quotes/backslashes yield legal source (#4).
			   local setName = '_ENUM_' .. var
			   local quoted = {}
			   table.map(c.values, function(_, v)
				  quoted[#quoted + 1] = javaStrLit(v)
			   end)
			   sets[setName] = table.concat(quoted, ', ')
			   guard(('%s.contains(_%s)'):format(setName, var), 'not permitted')
			end
		 end
	  end)
	  return out:str()
   end

   -- json output function
   local function objectOutput(typename, typedef)
	  local str = stringBuffer:new()
	  -- one stable, collision-free member name per JSON tag for this class
	  local memberNames = classMemberNames(typedef.fields)
	  -- generate file and import headers
	  str:append(('/* FILE: %s.java */\n'):format(typename))
	  str:append('/* This file was generated by xsdb */\n')
	  str:append(('package %s;\n'):format(javaPKGName))
	  str:append('import java.util.List;\n')
	  str:append('import java.util.ArrayList;\n')
	  str:append('import org.json.JSONObject;\n')
	  str:append('import org.apache.commons.codec.DecoderException;\n')
	  str:append('import org.json.JSONArray;\n')
	  str:append('import org.json.JSONException;\n')
	  str:append(('import %s.Marshalable;\n'):format(javaPKGName))
	  str:append(('import %s.JSONObjectAdapter;\n'):format(javaPKGName))
	  str:append(('import %s.JSONArrayAdapter;\n\n'):format(javaPKGName))
	  str:append(('import org.apache.commons.codec.DecoderException;\n\n'))
	  -- generate class definition
	  -- #4: emit the element/type annotation above its class
	  str:append(docComment(typedef.documentation, 'c', ''))
	  str:append(
		 ('public class %s implements Marshalable {\n'):format(typename)
	  )
	  -- generate private members
	  for JSONFieldName, fieldTypename, fieldTypedef in fieldIterator(typedef.fields) do
		 local memberName  = memberNames[JSONFieldName]
		 -- #4: emit the field annotation above its declaration
		 str:append(docComment(fieldTypedef.documentation, 'c', '\t'))
		 if isListType(fieldTypename) then
			local lstTypename = getListType(fieldTypename)
			str:append(
			   ListItemStrategy.declaration(
				  types[lstTypename], memberName
			   )
			)
		 else
			local declEff = effectiveType(fieldTypename, fieldTypedef)
			local declDef = fieldTypedef.default
			   and defaultLiteral(declEff, fieldTypedef.default)
			str:append(
			   ItemStrategy.declaration(declEff, memberName, declDef)
			)
		 end
	  end
	  -- generate default constructor
	  str:append(('\n\tpublic %s() {\n\t}\n\n'):format(typename))
	  -- generate default unmarshal constructor
	  local fmt = '\tpublic %s(JSONObject jObj) throws JSONException, DecoderException {\n'
	  str:append(fmt:format(typename))
	  str:append('\t\tthis(new JSONObjectAdapter(jObj));\n')
	  str:append('\t}\n\n')
	  -- generate unmarshal constructor
	  local fmt = '\tpublic %s(JSONObjectAdapter jObj) throws JSONException, DecoderException {\n'
	  str:append(fmt:format(typename))
	  for fieldName, fieldType, fieldTypedef in fieldIterator(typedef.fields) do
		 local memberName  = memberNames[fieldName]
		 if not isFieldRequired(fieldTypedef) then
			str:append(('\t\tif (!jObj.has("%s"))\n'):format(fieldName))
			-- a declared default (C) survives an absent field; else null
			if fieldTypedef.default and not isListType(fieldType) then
			   local eff = effectiveType(fieldType, fieldTypedef)
			   local lit = defaultLiteral(eff, fieldTypedef.default)
			   str:append(('\t\t\t_%s = %s;\n'):format(memberName, lit))
			else
			   str:append(('\t\t\t_%s = null;\n'):format(memberName))
			end
			str:append('\t\telse\n')
			if not isListType(fieldType) then str:append('\t') end
		 end

		 if isListType(fieldType) then
			local lstType = getListType(fieldType)
			str:append(
			   ListItemStrategy.unmarshal(
				  types[lstType], memberName, fieldName
			   )
			)
		 else
			local eff = effectiveType(fieldType, fieldTypedef)
			if eff.unmarshalExpr then
			   str:append(('\t\t_%s= %s("%s");\n'):format(
				  memberName, eff.unmarshalExpr, fieldName))
			else
			   str:append(ItemStrategy.unmarshal(eff, memberName, fieldName))
			end
		 end
	  end
	  -- enforce facet validation (D2) at the end of unmarshalling
	  str:append('\t\tvalidate();\n')
	  str:append('\t}\n\n')
	  -- generate validate() body from each field's facets; string-enum sets
	  -- are accumulated and emitted once as static fields above validate()
	  local sets = {}
	  local checksBody = stringBuffer:new()
	  for fieldName, fieldType, fieldTypedef in fieldIterator(typedef.fields) do
		 if fieldTypedef.facets then
			local jType
			if isListType(fieldType) then
			   jType = types[getListType(fieldType)].typename
			else
			   jType = effectiveType(fieldType, fieldTypedef).typename
			end
			checksBody:append(javaChecks(
			   memberNames[fieldName], fieldTypedef.facets, jType, sets))
		 end
	  end
	  for setName, vals in pairs(sets) do
		 str:append(('\tprivate static final java.util.Set<String> %s =\n')
			:format(setName))
		 str:append(('\t\tnew java.util.HashSet<String>(java.util.Arrays.asList(%s));\n')
			:format(vals))
	  end
	  str:append('\tpublic void validate() {\n')
	  str:append(checksBody:str())
	  str:append('\t}\n\n')
	  -- generate 'set'ers
	  for fieldName, fieldType, fieldTypedef in fieldIterator(typedef.fields) do
		 local memberName  = memberNames[fieldName]
		 if isListType(fieldType) then
			local lstType  = getListType(fieldType)
			local javaType = types[lstType]
			str:append(
			   outputSeterJavadoc(
				  ('Vector<%s>'):format(javaType.typename), memberName
			   )
			)
			str:append(ListItemStrategy.seter(javaType, memberName))
		 else
			local javaType = effectiveType(fieldType, fieldTypedef)
			str:append(outputSeterJavadoc(javaType.typename, memberName))
			str:append(ItemStrategy.seter(javaType, memberName))
		 end
	  end
	  -- generate 'get'ers
	  for fieldName, fieldType, fieldTypedef in fieldIterator(typedef.fields) do
		 local memberName  = memberNames[fieldName]
		 if isListType(fieldType) then
			local lstType  = getListType(fieldType)
			local javaType = types[lstType]
			str:append(
			   outputGeterJavadoc(
				  ('Vector<%s>'):format(javaType.typename), memberName
			   )
			)
			str:append(ListItemStrategy.geter(javaType, memberName))
		 else
			local javaType = effectiveType(fieldType, fieldTypedef)
			str:append(outputGeterJavadoc(javaType.typename, memberName))
			str:append(ItemStrategy.geter(javaType, memberName))
		 end
	  end
	  -- generate marshal funciton
	  str:append('\tpublic JSONObject marshal() throws JSONException {\n')
	  str:append(
		 '\t\tJSONObjectAdapter retObj = new JSONObjectAdapter(new JSONObject());\n'
	  )
	  for fieldName, fieldType, fieldTypedef in fieldIterator(typedef.fields) do
		 local memberName  = memberNames[fieldName]
		 if not isFieldRequired(fieldTypedef) then
            if types[fieldType].typename == "String" then
			    str:append(
			       ('\t\tif (null != _%s && 0 < _%s.length())\n'):format(
			    	  memberName, memberName
			       )
			    )
            elseif types[fieldType].typename == "byte []" then
			    str:append(
			       ('\t\tif (null != _%s && 0 < _%s.length)\n'):format(
			    	  memberName, memberName
			       )
			    )

            elseif types[fieldType].typename == "Integer" then
			    str:append(
			       ('\t\tif (null != _%s )\n'):format(
			    	  memberName, memberName
			       )
			    )

            elseif types[fieldType].typename == "Long" then
			    str:append(
			       ('\t\tif (null != _%s )\n'):format(
			    	  memberName, memberName
			       )
			    )
            else
			    str:append(
			       ('\t\tif (null != _%s )\n'):format(
			    	  memberName, memberName
			       )
			    )
            end
			if not isListType(fieldType) then str:append('\t') end
		 end

		 if isListType(fieldType) then
			local lstType = getListType(fieldType)
			str:append(
			   ListItemStrategy.marshal(
				  types[lstType], memberName, fieldName
			   )
			)
		 else
			local eff = effectiveType(fieldType, fieldTypedef)
			if eff.marshalVal then
			   str:append(('\t\tretObj.put("%s", _%s%s);\n'):format(
				  fieldName, memberName, eff.marshalVal))
			else
			   str:append(ItemStrategy.marshal(eff, memberName, fieldName))
			end
		 end
	  end
	  str:append('\t\treturn retObj.getJSONObject();\n')
	  str:append('\t}\n')
	  -- add end brace
	  str:append('}\n')
	  return str:str()
   end

   function printTbl(tbl, depth)
	  depth = depth or 0
	  for k,v in pairs(tbl) do
		 if "table" == type(v) then
			if nil == next(v) then 
			   dbgPrint(string.rep("  ", depth)..k.." = {}")
			else
			   dbgPrint(string.rep("  ", depth)..k.." = {")
			   printTbl(v, depth + 1)
			   dbgPrint(string.rep("  ", depth).."}")
			end
		 else
			print(string.rep("  ", depth)..k.." = "..v)
		 end
	  end
   end

   function RandomString(length)
       length = length or 1
       if length < 1 then return nil end
       local array = {}
       for i = 1, length do
           -- ascii range for capital and lower case alphabet
           local randomNum = math.random(65, 122)
           if randomNum >= 91 and randomNum <= 96 then
                randomNum = randomNum + 10
           end
           array[i] = string.char(randomNum)
       end
       return table.concat(array)
   end        

   local function hexBinary(length)
       length = length or 1
       if length < 1 then return nil end
       local array = {}
       for i = 1, length do
           -- ascii range for capital and lower case alphabet
           local randomNum = math.random(1, 16)
           hex = { "a","b","c","d","e","f", "0", "1", "2", "3", "4", "5", "6", 
                   "7", "8", "9"}
           array[i] = hex[randomNum]
       end
       return table.concat(array)
    end

    local function generateInteger()
        local positiveOrNegative = 1
        if math.random() > 0.5 then posNegRandInt = -1 end
        if math.random() > 0.5 then positiveOrNegative = -1 end
        return tostring(positiveOrNegative*math.random(1000000))
   end

   local function generateDouble()
        local positiveOrNegative = 1
        if math.random() > 0.5 then positiveOrNegative = -1 end
        return tostring(positiveOrNegative*math.random(10000000)/1000)
   end

   local function generateBoolean()
        local trueOrFalse = 1
        if math.random() > 0.5 then trueOrFalse= -1 end
        if trueOrFalse > 0 then return "true" else return "false" end
   end

   local function generatePositiveInteger()
        return tostring(math.random(1000000))
   end

   local function generateString()
        return RandomString(8)
   end

   local function generateDate()
        return tostring(math.random(12)).."\/"..tostring(math.random(30)).."\/"..tostring(math.random(1800,2100))
   end

   local function generateAddress()
        return tostring(math.random(1000))..generateString()
    end

    local function generateTime()
        return tostring(math.random(0,2400))
    end

   -- Facet-satisfying JSON literal for a leaf, or nil to fall back to the
   -- random generators. Numerics emit bare; strings emit single-quoted (the
   -- form org.json round-trips and matches the random path).
   local function facetSample(leafTypeName, facets)
      if not facets then return nil end
      local jType = types[leafTypeName].typename
      if jType == 'Integer' or jType == 'Long' then
         return facets_sampleInt(facets)
      end
      local s = facets_sampleString(facets)
      if s then return "'"..s.."'" end
      return facets_sampleInt(facets)
   end

   local function elementOutput(typename, typedef, visitType)
      local generateData = {}
      generateData["boolean"] = generateBoolean()
      generateData["double"] = generateDouble()
      generateData["long"] = generateInteger()

      generateData["integer"] = generateInteger()
      generateData["int"] = generateInteger()
      generateData["short"] = generateInteger()
      generateData["byte"] = generateInteger()
      generateData["positiveInteger"] = generatePositiveInteger()
      generateData["nonNegativeInteger"] = generatePositiveInteger()
      generateData["unsignedLong"] = generatePositiveInteger()
      generateData["unsignedInt"] = generatePositiveInteger()
      generateData["unsignedShort"] = generatePositiveInteger()
      generateData["unsignedByte"] = generatePositiveInteger()
      generateData["string"] = "'"..generateString().."'"
      generateData["date"] = "'"..generateDate().."'"
      generateData["dateTime"] = "'"..generateDate()..generateTime().."'"
      generateData["address"] = "'"..generateAddress().."'"
      generateData["time"] = generateTime()
      generateData["base64Binary"] = "'"..generateString().."'"
      generateData["hexBinary"] = "'"..hexBinary(8).."'"

	  local str = stringBuffer:new()
      -- length of typedef.fields
      local tableLength = 0
      local count = 1
      for _ in pairs(typedef.fields) do tableLength = tableLength + 1 end

       for tableName, tableDef in pairs(typedef.fields) do
			local nextTableName, nextTable = next(tableDef)
            str:append("'"..tableName.."':")
            if (isListType(nextTableName)) then
                local lstLeaf = getListType(nextTableName)
                local sample = facetSample(lstLeaf, nextTable.facets)
                if generateData[lstLeaf] == nil then
                    str:append("[{"..elementOutput(nextTableName, nextTable, visitType).."}]")
                    visitType[nextTableName] = true
                else
                    str:append( "["..(sample or generateData[lstLeaf]).."]" )
                end
            else
                local sample = facetSample(nextTableName, nextTable.facets)
                if not isSimpleType(nextTable) then
                    str:append ("{")
                    str:append(elementOutput(nextTableName, nextTable, visitType))
                    visitType[nextTableName] = true
                    str:append ("}")
                elseif generateData[nextTableName] == nil then
                    str:append(elementOutput(nextTableName, nextTable, visitType))
                    visitType[nextTableName] = true
                else
                    str:append( sample or generateData[nextTableName] )
                end
            end
            if count < tableLength then str:append(",") end
            count = count + 1
        end
        return str:str()
   end

   function outputKeys(JSONSchema)
        local outputStr = stringBuffer:new()
        for k in pairs(JSONSchema) do
            outputStr:append(k)
        end
        return(outputStr:str())
   end

    function marshalUnmarshalTest(JSONSchema)
        local visitType = {}
        local outputStr = stringBuffer:new()

        local function _traverse(JSONType)

            for tagname, typetable in pairs(JSONType.fields) do
                local typename, typedef = next(typetable)
                -- if type is not simple and not already referenced
                if not (visitType[typename] or isSimpleType(typedef)) then 
                    visitType[typename] = true
                    local variableName = RandomString(5)
                    local variableName2 = RandomString(5)
                    if isRootElement( JSONType.fields, typedef) then
                        outputStr:append("JSONObject "..variableName.." = new ")
                        outputStr:append("JSONObject(".."\"")
                        outputStr:append("{")
                    end

                    outputStr:append(elementOutput(typename, typedef, visitType))
                    _traverse(typedef)

                    if isRootElement( JSONType.fields, typedef) then
                        outputStr:append("}\");\n        ")
                        outputStr:append(typename.." "..variableName2.." = new "
                            ..typename.."( "..variableName.." );".."\n        ")
                        outputStr:append("System.out.println("..variableName..
                            ".toString().equals("..variableName2..".marshal().toString() ));\n\n")
                    end
                end
            end
        end
        _traverse({fields = JSONSchema})

        return outputStr:str()
    end

   -- json iteration function
   function outputJSON(JSONSchema)
	  local visitType = {}
	  local outputStr = stringBuffer:new()
	  local function _traverse(JSONType)
		 for tagname, typetable in pairs(JSONType.fields) do
			local typename, typedef = next(typetable)
            -- try to caputure anything inside of parenthesis                
            fList = typename:match("%((.-)%)")
            if fList ~= nil then typename = fList end 
			-- if type is not simple and not already referenced
			if not (visitType[typename] or isSimpleType(typedef)) then 
			   visitType[typename] = true
			   outputStr:append(objectOutput(typename, typedef))
			   _traverse(typedef)
			end
		 end
	  end
	  _traverse({fields = JSONSchema})
	  return outputStr:str()
   end
]
