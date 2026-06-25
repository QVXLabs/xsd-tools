[@lua
   -- helper functions
   local function isSimpleType(element) 
      if element.content and element.attributes then
         return ((next(element.attributes) or next(element.content)) == nil)
      elseif element.fields then
         for k, _ in pairs(element.fields) do
            if k ~= '$' then return false end
         end
         return true
      end
   end

   local function isListType(typeName)
      return (typeName:match("^(list)%(.+%)") ~= nil)
   end

   local function isSingular(typeName)
      return (typeName == 1)
   end

   local function isRootElement(schema, type)
      for _, v in pairs(schema) do
         local elemName, elemType = next(v)
         if elemType == type then return true end
      end  
      return false;
   end

   local function isTypeFoldable(schema, typedef)
      return (not isRootElement(schema, typedef) and 
           isSimpleType(typedef) and typedef.fields['$'])
   end

   local function getListType(typeName)
      return typeName:match("list%((.+)%)")
   end

   local function makeJavaSafeName(name)
      if isListType(name) then
         return 'list('..makeJavaSafeName(getListType(name))..')'
      else
         return name:gsub('[%$,/,%-,%%,#,@,!,%^,&,*,%(,%),  ,]', '_')
      end
   end

   -- assigns content field names (seperates element names from types)
   local function assignContentFieldNames(XSDTree)
      local typeTbl = {}
      local function _cmpType(typedef1, typedef2)
         local ty1 = type(typedef1)
         local ty2 = type(typedef2)
         if ty1 ~= ty2 then return false end
         -- non-table types can be directly compared
         if ty1 ~= 'table' and ty2 ~= 'table' then return t1 == t2 end
         -- compare table types
         -- facets don't affect Java type identity (not enforced here)
         for k1,v1 in pairs(typedef1) do
            if k1 ~= 'facets' then
               local v2 = typedef2[k1]
               if v2 == nil or not _cmpType(v1,v2) then return false end
            end
         end
         for k2,v2 in pairs(typedef2) do
            if k2 ~= 'facets' then
               local v1 = typedef1[k2]
               if v1 == nil or not _cmpType(v1,v2) then return false end
            end
         end
         return true
      end
      local function _getType(className, typedef, nCalls)
         nCalls = nCalls or 2
         if typeTbl[className] then
            if _cmpType(typeTbl[className], typedef) then
               return className, typedef
            else
               return _getType(className..nCalls, typedef, nCalls + 1)
            end
         else
            typeTbl[className] = typedef
            return className, typedef
         end
      end
      local function _traverse(element, outElem) 
         for field, value in pairs(element) do
            if "content" == field then
               outElem.content = {}
               for name, typedef in pairs(element.content) do
                  -- handle elements being defined multiple times
                  local className, classTypeDef = _getType(name, typedef)
                  -- handle xs:simpleType content value -> JSON tag names
                  if isSimpleType(classTypeDef) then name = '$' end
                  -- modify schema
                  outElem.content[name] = {}
                  outElem.content[name][className] = {}
                  _traverse(typedef, outElem.content[name][className])
               end
            else
               outElem[field] = value
            end
         end
         return outElem
      end
      return _traverse({ attributes = {}, content = XSDTree }, {}).content
   end

  
   -- assign occurrence values
   local function assignTypeOccurence(XSDTree)

      local function _addOccurrenceModifier(className, typedef)
         if not isSimpleType(typedef) and not isRootElement(XSDTree, typedef) 
		and not isSingular(typedef["maxOccurs"])  then
            return 'list('..className..')'
         else
            return className
         end
      end
      local function _traverse(element, outElem) 
         for field, value in pairs(element) do
            if "content" == field then
               outElem.content = {}         
               for jsonTag, classNameTbl in pairs(element.content) do
                  local className, typedef = next(classNameTbl)
                  -- add occurrence modification
                  className = _addOccurrenceModifier(className, typedef)
                  -- modify schema
                  outElem.content[jsonTag] = {}            
                  outElem.content[jsonTag][className] = {}
                  _traverse(typedef, outElem.content[jsonTag][className])
               end
            else
               outElem[field] = value
            end
         end
         return outElem
      end
      return _traverse({ attributes = {}, content = XSDTree }, {}).content
   end

   -- fold 'content' & 'attribtues' into 'fields' & add json-jackson/jersey 
   -- attribute prefix (@)
   local function assignJSONFields(XSDTree)
      local function _traverse(element, outElem) 
         outElem.fields = {}
         for field, value in pairs(element) do
            if "attributes" == field then
               for jsonTag, classNameTbl in pairs(value) do
                  local className, typedef = next(classNameTbl)
                  outElem.fields['@'..jsonTag] = classNameTbl
                  _traverse(typedef, outElem.fields['@'..jsonTag][className])
               end
               outElem.attributes = nil
            elseif "content" == field then
               for jsonTag, classNameTbl in pairs(value) do
                  local className, typedef = next(classNameTbl)
                  outElem.fields[jsonTag] = classNameTbl                  
                  _traverse(typedef, outElem.fields[jsonTag][className])
               end
               outElem.content = nil
            end
         end
         return outElem
      end
      return _traverse({ attribtues = {}, content = XSDTree }, {}).fields
   end

   -- fold simpletypes that are not root elements
   local function foldSimpleTypes(XSDTree)
      local function _traverse(element, outElem) 
         for field, value in pairs(element) do
            if "fields" == field then
               outElem.fields = {}
               for jsonTag, classNameTbl in pairs(value) do
                  local className, typedef = next(classNameTbl)
                  if isTypeFoldable(XSDTree, typedef) then
                     local foldedTypeName, foldedTypedef = next(typedef.fields['$'])
                     outElem.fields[jsonTag] = {}
                     if isListType(className) then
                        outElem.fields[jsonTag]['list('..foldedTypeName..')'] = foldedTypedef
                     else
                        outElem.fields[jsonTag][foldedTypeName] = foldedTypedef
                     end
                  else
                     outElem.fields[jsonTag] = classNameTbl
                     _traverse(typedef, outElem.fields[jsonTag][className])
                  end
               end
            end
         end
         return outElem
      end
      return _traverse({ fields = XSDTree }, {}).fields
   end

    -- fixup element/attribute type names which may be invalid java class names. 
    -- Also capitalize the first character of the java class names if they are 
    -- complex types
    local function fixupTypeNames(XSDTree)
        local function _traverse(element, outElem)
            for field, value in pairs(element) do
                if "fields" == field then           
                    outElem.fields = {}
                    for jsonTag, classNameTbl in pairs(value) do
                        local className, typedef = next(classNameTbl)
                        outElem.fields[jsonTag] = {}
                        if not isSimpleType(typedef) then
                            if isListType(className) then
                                local listType       = className:match("list%((.+)%)")
                                local fixedListType  = makeJavaSafeName(listType)
                                local upcasedListType= fixedListType:gsub("^%l", string.upper)
                                local upcasedType    = 'list('..upcasedListType..')'
                                outElem.fields[jsonTag][upcasedType] = typedef
                                _traverse(typedef, outElem.fields[jsonTag][upcasedType])
                            else
                                local fixedType   = makeJavaSafeName(className)
                                local upcasedType = fixedType:gsub("^%l", string.upper)
                                outElem.fields[jsonTag][upcasedType] = typedef
                                _traverse(typedef, outElem.fields[jsonTag][upcasedType])
                            end
                        elseif isRootElement(XSDTree, typedef) then
                            local simpleType     = makeJavaSafeName(className)
                            local upcasedType    = simpleType:gsub("^%l", string.upper)
                            outElem.fields[jsonTag][upcasedType] = typedef
                            _traverse(typedef, outElem.fields[jsonTag][upcasedType])
                        else
                            local fixedTypeName = makeJavaSafeName(className)
                            outElem.fields[jsonTag][fixedTypeName] = typedef
                            _traverse(typedef, outElem.fields[jsonTag][fixedTypeName])
                        end
                    end
                end
            end
            return outElem
        end
        return _traverse({ fields = XSDTree }, {}).fields
    end

-- coalesce tree transformation passes into one function
   function JSONTransform(schema)
      return fixupTypeNames(
         foldSimpleTypes(
            assignJSONFields(
               assignTypeOccurence(
                  assignContentFieldNames(schema)))))
   end
]
