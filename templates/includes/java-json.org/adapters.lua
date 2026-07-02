[@lua
   -- Generation-time accessor selection for the two adapter classes. The
   -- adapters historically emitted every type accessor; here we collect the
   -- leaf types the transformed schema actually references and emit only the
   -- matching get/put blocks (mirrors python-json's per-type dispatch).

   -- Object-adapter accessor blocks, keyed by a token. getJSONObject/has and
   -- the ctor are structural and emitted unconditionally by the emitter.
   local objBlocks = {
      getBoolean =
'\tpublic boolean getBoolean(String key) throws JSONException {\n\t\treturn _jObj.getBoolean(key);\n\t}\t\n',
      getString =
'\tpublic String getString(String key) throws JSONException {\n\t\treturn _jObj.getString(key);\n\t}\t\n',
      getInt =
'\tpublic int getInt(String key) throws JSONException {\n\t\treturn _jObj.getInt(key);\n\t}\n',
      getLong =
'\tpublic long getLong(String key) throws JSONException {\n\t\treturn _jObj.getLong(key);\n\t}\n',
      getBigInteger =
'\tpublic java.math.BigInteger getBigInteger(String key) throws JSONException {\n\t\treturn _jObj.getBigInteger(key);\n\t}\n',
      getDouble =
'\tpublic double getDouble(String key) throws JSONException {\n\t\treturn _jObj.getDouble(key);\n\t}\t\n',
      getObject =
'\tpublic JSONObjectAdapter getObject(String key) throws JSONException {\n\t\treturn new JSONObjectAdapter(_jObj.getJSONObject(key));\n\t}\n',
      getList =
'\tpublic JSONArrayAdapter getList(String key) throws JSONException {\n\t\treturn new JSONArrayAdapter(_jObj.getJSONArray(key));\n\t}\t\t\n',
      getBase64 =
'\tpublic byte[] getBase64(String key) throws JSONException {\n\t\treturn Base64.decodeBase64(_jObj.getString(key));\n\t}\n',
      getHex =
'\tpublic byte[] getHex(String key) throws JSONException, DecoderException {\n\t\treturn Hex.decodeHex(_jObj.getString(key).toCharArray());\n\t}\n',
      putBoolean =
'\tpublic JSONObjectAdapter put(String key, Boolean value) throws JSONException {\n\t\treturn new JSONObjectAdapter(_jObj.put(key, value.booleanValue()));\n\t}\t\n',
      putLong =
'\tpublic JSONObjectAdapter put(String key, Long value) throws JSONException {\n\t\treturn new JSONObjectAdapter(_jObj.put(key, value.longValue()));\n\t}\t\n',
      putInteger =
'\tpublic JSONObjectAdapter put(String key, Integer value) throws JSONException {\n\t\treturn new JSONObjectAdapter(_jObj.put(key, value.intValue()));\n\t}\n',
      putBigInteger =
'\tpublic JSONObjectAdapter put(String key, java.math.BigInteger value) throws JSONException {\n\t\treturn new JSONObjectAdapter(_jObj.put(key, (Object) value));\n\t}\n',
      putString =
'\tpublic JSONObjectAdapter put(String key, String value) throws JSONException {\n\t\treturn new JSONObjectAdapter(_jObj.put(key, value));\n\t}\n',
      putInt =
'\tpublic JSONObjectAdapter put(String key, int value) throws JSONException {\n\t\treturn new JSONObjectAdapter(_jObj.put(key, value));\n\t}\n',
      putDouble =
'\tpublic JSONObjectAdapter put(String key, double value) throws JSONException {\n\t\treturn new JSONObjectAdapter(_jObj.put(key, value));\n\t}\n',
      putArray =
'\tpublic JSONObjectAdapter put(String key, JSONArrayAdapter value) throws JSONException {\n\t\treturn new JSONObjectAdapter(_jObj.put(key, value.getJSONArray()));\n\t}\t\n',
      putJSONObject =
'\tpublic JSONObjectAdapter put(String key, JSONObject value) throws JSONException {\n\t\treturn new JSONObjectAdapter(_jObj.put(key, value));\n\t}\t\n',
      putBase64 =
'\tpublic JSONObjectAdapter putBase64(String key, byte [] buffer) throws JSONException {\n\t\treturn new JSONObjectAdapter(_jObj.put(key, Base64.encodeBase64String(buffer)));\n\t}\n',
      putHex =
'\tpublic JSONObjectAdapter putHex(String key, byte [] buffer) throws JSONException {\n\t\treturn new JSONObjectAdapter(_jObj.put(key, Hex.encodeHexString(buffer)));\n\t}\n',
   }
   -- Ordered keys so emitted output keeps the historical block order.
   local objOrder = {
      'getBoolean','getString','getInt','getLong','getBigInteger','getDouble',
      'getObject','getList','getBase64','getHex','putBoolean','putLong',
      'putInteger','putBigInteger','putString','putInt','putDouble','putArray',
      'putJSONObject','putBase64','putHex',
   }

   -- Array-adapter accessor blocks (indexed by int position).
   local arrBlocks = {
      getBoolean =
'\tpublic boolean getBoolean(int ndx) throws JSONException {\n\t\treturn _jObj.getBoolean(ndx);\n\t}\n',
      getString =
'\tpublic String getString(int ndx) throws JSONException {\n\t\treturn _jObj.getString(ndx);\n\t}\t\n',
      getInt =
'\tpublic int getInt(int ndx) throws JSONException {\n\t\treturn _jObj.getInt(ndx);\n\t}\n',
      getLong =
'\tpublic long getLong(int ndx) throws JSONException {\n\t\treturn _jObj.getLong(ndx);\n\t}\n',
      getBigInteger =
'\tpublic java.math.BigInteger getBigInteger(int ndx) throws JSONException {\n\t\treturn _jObj.getBigInteger(ndx);\n\t}\n',
      getDouble =
'\tpublic double getDouble(int ndx) throws JSONException {\n\t\treturn _jObj.getDouble(ndx);\n\t}\t\n',
      getObject =
'\tpublic JSONObjectAdapter getObject(int ndx) throws JSONException {\n\t\treturn new JSONObjectAdapter(_jObj.getJSONObject(ndx));\n\t}\t\t\n\t\n',
      getBase64 =
'\tpublic byte[] getBase64(int ndx) throws JSONException {\n\t\treturn Base64.decodeBase64(_jObj.getString(ndx));\n\t}\n',
      getHex =
'\tpublic byte[] getHex(int ndx) throws JSONException, DecoderException {\n\t\treturn Hex.decodeHex(_jObj.getString(ndx).toCharArray());\n\t}\t\n',
      putBoolean =
'\tpublic JSONArrayAdapter put(Boolean value) throws JSONException {\n\t\treturn new JSONArrayAdapter(_jObj.put(value.booleanValue()));\n\t}\n',
      putString =
'\tpublic JSONArrayAdapter put(String value) throws JSONException {\n\t\treturn new JSONArrayAdapter(_jObj.put(value));\n\t}\n',
      putBigInteger =
'\tpublic JSONArrayAdapter put(java.math.BigInteger value) throws JSONException {\n\t\treturn new JSONArrayAdapter(_jObj.put((Object) value));\n\t}\n',
      putInt =
'\tpublic JSONArrayAdapter put(int value) throws JSONException {\n\t\treturn new JSONArrayAdapter(_jObj.put(value));\n\t}\n',
      putLong =
'\tpublic JSONArrayAdapter put(long value) throws JSONException {\n\t\treturn new JSONArrayAdapter(_jObj.put(value));\n\t}\n',
      putDouble =
'\tpublic JSONArrayAdapter put(double value) throws JSONException {\n\t\treturn new JSONArrayAdapter(_jObj.put(value));\n\t}\n',
      putMarshalable =
'\tpublic JSONArrayAdapter put(Marshalable value) throws JSONException {\n\t\treturn new JSONArrayAdapter(_jObj.put(value.marshal()));\n\t}\n',
      putBase64 =
'\tpublic JSONArrayAdapter putBase64(byte [] buffer) throws JSONException {\n\t\treturn new JSONArrayAdapter(_jObj.put(Base64.encodeBase64String(buffer)));\n\t}\n',
      putHex =
'\tpublic JSONArrayAdapter putHex(byte [] buffer) throws JSONException {\n\t\treturn new JSONArrayAdapter(_jObj.put(Hex.encodeHexString(buffer)));\n\t}\n',
   }
   local arrOrder = {
      'getBoolean','getString','getInt','getLong','getBigInteger','getDouble',
      'getObject','getBase64','getHex','putBoolean','putString','putBigInteger',
      'putInt','putLong','putDouble','putMarshalable','putBase64','putHex',
   }

   -- Map a primitive leaf typename to its (objGet,objPut,arrGet,arrPut) tokens.
   -- Reflects the fixed marshal/unmarshal expressions the strategies emit.
   local leafTokens = {
      Boolean   = {objGet='getBoolean', objPut='putBoolean',
                   arrGet='getBoolean', arrPut='putBoolean'},
      String    = {objGet='getString', objPut='putString',
                   arrGet='getString', arrPut='putString'},
      Integer   = {objGet='getInt', objPut='putInteger',
                   arrGet='getInt', arrPut='putInt'},
      Long      = {objGet='getLong', objPut='putLong',
                   arrGet='getLong', arrPut='putLong'},
      Double    = {objGet='getDouble', objPut='putDouble',
                   arrGet='getDouble', arrPut='putDouble'},
      ['java.math.BigInteger'] = {objGet='getBigInteger',
                   objPut='putBigInteger', arrGet='getBigInteger',
                   arrPut='putBigInteger'},
   }

   local function isListType(t) return (t:match('^list%(.+%)') ~= nil) end
   local function getListType(t) return t:match('list%((.+)%)') end
   local function isSimpleLeaf(typedef) return (next(typedef.fields) == nil) end

   -- Walk every field of every emitted type; record which adapter blocks the
   -- generated marshal/unmarshal code will reference. `effective` is the
   -- effectiveType helper from parse.lua (narrows numeric members).
   function collectAdapterUsage(schema, effective)
      local obj, arr = {}, {}
      local seen = {}
      -- byte [] is shared by base64/hexBinary; distinguish via marshal token.
      local function markScalar(eff)
         if not eff.metaInfo.primative then
            obj.getObject, obj.putJSONObject = true, true
            return
         end
         if eff.marshal == 'putBase64' then
            obj.getBase64, obj.putBase64 = true, true
         elseif eff.marshal == 'putHex' then
            obj.getHex, obj.putHex = true, true
         elseif eff.marshalVal == '.intValue()' then
            -- narrowed Byte/Short: (byte) getInt + put(String,int)
            obj.getInt, obj.putInt = true, true
         else
            local tok = leafTokens[eff.typename]
            if tok then obj[tok.objGet], obj[tok.objPut] = true, true end
         end
      end
      local function markListLeaf(leafName)
         obj.getList, obj.putArray = true, true
         local base = types[leafName]
         if not base.metaInfo.primative then
            arr.getObject, arr.putMarshalable = true, true
            return
         end
         if base.marshal == 'putBase64' then
            arr.getBase64, arr.putBase64 = true, true
         elseif base.marshal == 'putHex' then
            arr.getHex, arr.putHex = true, true
         else
            local tok = leafTokens[base.typename]
            if tok and tok.arrGet then
               arr[tok.arrGet], arr[tok.arrPut] = true, true
            end
         end
      end
      local function _traverse(node)
         for _, typetable in pairs(node.fields) do
            local typename, typedef = next(typetable)
            local inner = typename:match('%((.-)%)')
            local key = inner or typename
            if isListType(typename) then
               markListLeaf(getListType(typename))
            elseif isSimpleLeaf(typedef) then
               markScalar(effective(typename, typedef))
            else
               -- complex scalar field: new X(jObj.getObject(...)) +
               -- retObj.put(tag, _x.marshal())
               obj.getObject, obj.putJSONObject = true, true
            end
            if not (seen[key] or isSimpleLeaf(typedef)) then
               seen[key] = true
               _traverse(typedef)
            end
         end
      end
      _traverse({fields = schema})
      return obj, arr
   end

   local function emitBlocks(blocks, order, used)
      local out = stringBuffer:new()
      for _, k in ipairs(order) do
         if used[k] then out:append(blocks[k]) end
      end
      return out:str()
   end

   function outputObjectAdapter(used)
      local s = stringBuffer:new()
      s:append('/* FILE: JSONObjectAdapter.java */\n')
      s:append('/* This file was generated by xsdb */\n')
      s:append(('package %s;\n'):format(javaPKGName))
      s:append('import org.apache.commons.codec.DecoderException;\n')
      s:append('import org.apache.commons.codec.binary.Base64;\n')
      s:append('import org.apache.commons.codec.binary.Hex;\n')
      s:append('import org.json.JSONException;\n')
      s:append('import org.json.JSONObject;\n\n\n')
      s:append('public class JSONObjectAdapter {\n')
      s:append('\tprivate JSONObject _jObj;\n\t\n')
      s:append('\tpublic JSONObjectAdapter(JSONObject jObj) {\n')
      s:append('\t\t_jObj = jObj;\n\t}\n')
      s:append('\tJSONObject getJSONObject() {\n\t\treturn _jObj;\n\t}\n')
      s:append(emitBlocks(objBlocks, objOrder, used))
      s:append('\tpublic boolean has(String key) {\n')
      s:append('\t   return _jObj.has(key);\n\t}\n}\n')
      return s:str()
   end

   function outputArrayAdapter(used)
      local s = stringBuffer:new()
      s:append('/* FILE: JSONArrayAdapter.java */\n')
      s:append('/* This file was generated by xsdb */\n')
      s:append(('package %s;\n'):format(javaPKGName))
      s:append('import org.apache.commons.codec.DecoderException;\n')
      s:append('import org.apache.commons.codec.binary.Base64;\n')
      s:append('import org.apache.commons.codec.binary.Hex;\n')
      s:append('import org.json.JSONArray;\n')
      s:append('import org.json.JSONException;\n\n\n')
      s:append('public class JSONArrayAdapter {\n')
      s:append('\tprivate JSONArray _jObj;\n\t\n')
      s:append('\tpublic JSONArrayAdapter(JSONArray jObj) {\n')
      s:append('\t\t_jObj = jObj;\n\t}\n')
      s:append('\tJSONArray getJSONArray() {\n\t\treturn _jObj;\n\t}\n')
      s:append(emitBlocks(arrBlocks, arrOrder, used))
      -- put(JSONArrayAdapter) is needed for any nested list; always emit it
      -- since getList() returns one and put(String,JSONArrayAdapter) consumes
      -- it on the object side. Cheap and avoids nested-list edge cases.
      s:append('\tpublic JSONArrayAdapter put(JSONArrayAdapter value) throws JSONException {\n')
      s:append('\t\treturn new JSONArrayAdapter(_jObj.put(value.getJSONArray()));\n\t}\n')
      s:append('\tpublic int length() {\n\t\treturn _jObj.length();\n\t}\n}\n')
      return s:str()
   end
]
