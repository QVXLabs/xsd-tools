# cpp-json-jsonc

Modern C++11 marshalling/unmarshalling to and from JSON, built on the
[json-c](https://github.com/json-c/json-c) library.

## Generate

```sh
xsdb cpp-json-jsonc path/to/schema.xsd            # to stdout
xsdb --out-dir gen/ cpp-json-jsonc path/to/schema.xsd
```

Output is a single stream with `/* FILE: name */` markers. The header banner
gives the csplit command to break it into files:

```sh
xsdb cpp-json-jsonc schema.xsd | csplit - '/\/\* FILE: /' '{*}'
```

For a schema named `foo.xsd` you get three files: `json_common.h` (shared
`JsonRef` + `json_buffer`), `json_foo.hpp`, and `json_foo.cpp`.

## Toolchain & dependencies

- A C++11 compiler (`-std=c++11`): the code uses `std::unique_ptr`,
  `std::function`, move semantics, and in-class member initializers.
- Link **json-c** (`-ljson-c`); the generated `.cpp` includes `<json.h>`.
- Link **libb64** only when the schema uses `base64Binary`. In that case the
  `.cpp` pulls in `b64/cencode.h` / `b64/cdecode.h` under an `extern "C"`
  guard. (`hexBinary` needs no extra library — hex is encoded inline.)

## Generated API

Per schema, the header declares:

- **Element classes** — one `class json_<name>` per complex type. Members are
  value types (`std::string`, fixed-width ints, `std::vector<...>` for lists,
  `std::vector<uint8_t>` for binary), default-initialized in-class. Each class
  has `bool validate() const` and a `virtual ~json_<name>()` so subtypes
  destroy correctly. Scalar content lands in a `Content_` member; attributes
  become `<attrName>_` members.
- **`class Factory`** — one overridable
  `virtual std::unique_ptr<json_<name>> create_<name>() const` per element.
  The default returns the generated class; subclass `Factory` and override a
  `create_*` to inject your own subtype. One virtual covers allocate+construct;
  the returned `unique_ptr` covers destroy. Same idiom as cpp-xml-expat.
- **`struct JsonRef`** (in `json_common.h`) — a move-only RAII owner around a
  `json_object*` that calls `json_object_put` on destruction, so json-c's
  manual refcounting becomes scope-bound. Copy is deleted; `get()`,
  `release()`, and `reset()` are provided. No double-put, no leak.
- **`struct json_marshaller`** — holds the document root (`JsonRef pRoot_`) plus
  one `unmarshal_<name>` `std::function` callback slot per element; set the
  slots for the elements you want delivered during unmarshalling.
- **Marshal API** — `json_marshal(marshaller)` resets the root and returns a
  `json_marshal_<root>` struct of nested `marshal_<child>` callbacks; call them
  to attach element objects under their parent. `json_marshal_flush(marshaller,
  isDocEnd)` serializes the root to a `json_buffer` (`std::string`) via
  `json_object_to_json_string_ext(..., JSON_C_TO_STRING_PLAIN)`.
- **Unmarshal API** — `json_unmarshal(marshaller, buf, isDocEnd)` parses `buf`
  with `json_tokener_parse`, walks matching keys, and fires your callbacks.
  `json_unmarshal_ex(..., factory)` takes an explicit `Factory` override.

Build and parse go through json-c calls: `json_object_new_object`,
`json_object_object_add`, `json_object_object_get_ex`, `json_object_get_*`.

**Facet validate()** — `validate()` enforces facets recorded on the schema and
**throws `std::runtime_error("facet violation")`** on a breach (returning
`true` otherwise). Numeric ranges become `>=`/`<=` checks, string lengths use
`.size()`, numeric enumerations compare directly, and string enumerations
switch on a seed-0 SDBM hash (with a strcmp fallback for hash collisions).
During unmarshalling `validate()` gates the callback; pattern facets are not
enforced.

**base64 / hex / list** — `base64Binary` and `hexBinary` map to
`std::vector<uint8_t>`; base64 routes through libb64, hex is encoded inline. A
`list` of items maps to `std::vector<...>` and marshals as a JSON array via
generated `_marshal_list_*` / `_unmarshal_list_*` helpers.

## Example

`rating.xsd`:

```xml
<schema>
  <element name="foo">
    <complexType>
      <attribute name="attrib1Required" type="string" use="required"/>
      <attribute name="attrib2Optional" type="integer" default="5"/>
      <sequence><element name="name" minOccurs="0"/></sequence>
    </complexType>
  </element>
</schema>
```

```sh
xsdb cpp-json-jsonc foo.xsd | csplit - '/\/\* FILE: /' '{*}'
```

Generated class + Factory (excerpt):

```cpp
class json_foo {
public:
    uint32_t    eid_ = 0;
    int64_t     attrib2Optional_ = 0;
    std::string attrib1Required_;
    std::string attrib1Optional_;
    bool validate() const;
    virtual ~json_foo() {}
};

class Factory {
public:
    virtual ~Factory() {}
    virtual std::unique_ptr<json_foo> create_foo() const {
        return std::unique_ptr<json_foo>(new json_foo());
    }
    // ... create_name(), etc.
};
```

Marshal drive (build a document, attach children, flush):

```cpp
json_marshaller m;
json_marshal_foo_xsd doc = json_marshal(m);   // resets m.pRoot_

json_foo foo;
foo.attrib1Required_ = "hi";
json_marshal_foo fooFns = doc.marshal_foo(&m, &foo, doc.pSelf);

json_name child;
child.Content_ = "world";
fooFns.marshal_name(&m, &child, fooFns.pSelf);

json_buffer out = json_marshal_flush(m, /*isDocEnd=*/true);
```

Unmarshal drive (parse, receive via callbacks):

```cpp
json_marshaller m;
m.unmarshal_foo = [](json_marshaller*, const json_foo* p, uint32_t parent) {
    use(p->attrib1Required_);          // p->validate() already passed
};
json_unmarshal(m, out, /*isDocEnd=*/true);
```

Subclass `Factory` and pass it to `json_unmarshal_ex` to allocate your own
subtypes; the `JsonRef`/`unique_ptr` RAII handles all freeing.

## Notes

- **JSON shape** — each element is a JSON object keyed by element name; nested
  elements are nested objects. Attributes are plain keys; simple **content** is
  stored under the reserved key `"$"`. There is no XML-style attribute/element
  namespace distinction, so an attribute and a child element with the same name
  would collide in the JSON.
- `eid_` is an internal id used to thread parent/child relationships; it is not
  part of the serialized JSON.
- `isDocEnd` controls whether the root `JsonRef` is reset after a
  flush/unmarshal; pass `true` for a complete single document.
