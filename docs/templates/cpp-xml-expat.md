# cpp-xml-expat

Modern C++11 XML marshalling bindings (expat-driven SAX parse), one element
class per XSD element with an overridable factory and per-element hooks.

## Generate

```sh
xsdb cpp-xml-expat <schema.xsd>            # to stdout (FILE markers)
xsdb --out-dir <dir> cpp-xml-expat <schema.xsd>
```

`--out-dir` splits the output into three files (the multi-file stream is
delimited by `/* FILE: ... */` markers, splittable with the emitted
`csplit` command):

- `xml_common.h` — shared includes (`<cstdint>`, `<string>`, `<vector>`,
  `<memory>`, `<stdexcept>`).
- `xml_<schema>.hpp` — `namespace xml` public API: `Element`, the element
  classes, `Factory`, `Marshaller`.
- `xml_<schema>.cpp` — implementation (validators, marshal/unmarshal,
  expat thunks).

`<schema>` is the schema's root element name with any trailing `_xsd`
stripped.

## Toolchain & dependencies

- A C++11 compiler — compile with `-std=c++11`.
- Link **expat** (`#include <expat.h>`); the bindings call
  `XML_ParserCreateNS`, so namespace-aware expat is required.
- Link **libb64** *only when* the schema uses `xs:base64Binary` — its C
  headers (`b64/cencode.h`, `b64/cdecode.h`) are pulled in under
  `extern "C"`. `xs:hexBinary` needs no extra library (hex codec is
  emitted inline). No-binary schemas link neither.

Round-trip harness (`test/roundtrip_util.cpp`, `cppXmlRoundtrip` /
`ccRoundtripImpl`): generates the `cpp-xml-expat` binding plus the
`cpp-xml-expat-test` driver, splits the FILE-marked output, then compiles
`<base>-bin.cpp` + `xml_<base>.cpp` + the libb64 archive against the
expat include/link flags with the project C++ compiler at `-std=c++11`,
and runs the self-checking binary.

## Generated API

All in `namespace xml`. Modern C++11 throughout: `std::string` /
fixed-width `int*_t` / `std::vector` members, RAII, no manual `free`.

- **`class Element`** — abstract base. Pure virtual `setContent`,
  `setAttribute`; virtual `validate()` (default no-op). Lets the parse
  stack hold heterogeneous elements as `unique_ptr<Element>` without a
  union or type switch.

- **Element classes** — one `class <name> : public Element` per element.
  Members are default-initialised (`= {}`): scalar content as `content_`,
  each attribute as `<attr>_`. Type mapping (`cpp_type_info`): string-like
  XSD types → `std::string`, `xs:boolean` → `bool`, integers → narrowest
  fixed-width `int*_t`/`uint*_t` for a known facet range, `xs:decimal` →
  `long double`, `xs:base64Binary`/`xs:hexBinary` →
  `std::vector<uint8_t>`, `xs:list` → `std::vector<T>` (space-separated).
  `setContent`/`setAttribute` parse text into the typed members.

- **`class Factory`** — overridable. One
  `virtual std::unique_ptr<<name>> create_<name>() const` per element,
  returning the generated class by default. Subclass and override a
  `create_*` to inject a derived element subtype into the parse.

- **`class Marshaller`** — constructed with an optional `const Factory*`
  (null → an internal default factory).
  - Marshal: `marshalBegin()` writes the XML prolog, `marshal_<name>(const
    <name>&)` appends each element (it first closes open tags down to the
    element's parent so nesting is correct), `flush(bool isDocEnd)` closes
    any still-open tags and returns + clears the accumulated text.
  - Unmarshal: `unmarshal(const char* data, size_t len, bool isDocEnd)`
    feeds a (possibly partial) chunk to expat; the overridable
    `virtual void on_<name>(const <name>&)` hook fires as each element
    closes.

- **expat C-ABI bridge (load-bearing).** Member function pointers can't
  cross expat's C ABI, so `unmarshal` registers `static XMLCALL`
  thunks (`startThunk_`/`endThunk_`/`charsThunk_`) that recover `this`
  from `XML_GetUserData` and forward to the `onStart`/`onEnd`/`onChars`
  members. A C++ exception must **not** unwind through expat's C frames:
  each thunk catches, stashes the exception in `pending_`
  (`std::exception_ptr`), and calls `XML_StopParser`; `unmarshal`
  rethrows it once control is back in C++. `localName_` strips expat's
  `uri:local` NS expansion to the local name for dispatch.

- **Facet validation.** Each element's `validate()` (out-of-line in the
  `.cpp`) checks range/length/enum facets and throws `std::runtime_error`
  on a miss. `onEnd` calls `validate()` before the `on_<name>` hook, so a
  violation surfaces (via the thunk → `pending_` path) as a thrown
  exception out of `unmarshal`. With no facets, `validate()` is empty.

- **Namespaces.** A `targetNamespace` is emitted as an `xmlns:tns="..."`
  declaration on the document-root start tag; qualified element/attribute
  tags are prefixed `tns:`. Parsing uses namespace-aware expat and matches
  on local names, so prefixes round-trip. Non-namespaced schemas emit no
  NS at all.

## Example

```xml
<!-- record.xsd -->
<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema"
           xmlns:tns="urn:example:phase0"
           targetNamespace="urn:example:phase0">
  <xs:element name="record" type="tns:RecordType"/>
  <xs:complexType name="RecordType">
    <xs:sequence>
      <xs:element name="label" type="xs:string"/>
      <xs:element name="count" type="xs:integer"/>
    </xs:sequence>
    <xs:attribute name="id" type="xs:string"/>
  </xs:complexType>
</xs:schema>
```

```sh
xsdb --out-dir gen cpp-xml-expat record.xsd
# -> gen/xml_common.h, gen/xml_record.hpp, gen/xml_record.cpp
```

Generated class, factory entry, and the namespaced start tag (abridged):

```cpp
class record : public xml::Element {
public:
    std::string id_ = {};
    void setAttribute(const std::string& name,
                      const std::string& value) override {
        if (name == "id") { id_ = value; }
        else (void)value;
    }
    // ...
};

// in xml::Factory
virtual std::unique_ptr<record> create_record() const {
    return std::unique_ptr<record>(new record());
}

// in Marshaller::marshal_record
out_ += "<tns:record xmlns:tns=\"urn:example:phase0\"";
out_ += " id=\""; out_ += obj.id_; out_ += "\"";
```

Drive marshal then round-trip back via a hook:

```cpp
#include "xml_record.hpp"
using namespace xml;

struct Sink : Marshaller {
    void on_record(const record& r) override { /* got it */ }
};

Sink m;
record r; r.id_ = "r1";
m.marshalBegin();
m.marshal_record(r);
std::string doc = m.flush(true);          // complete XML text

m.unmarshal(doc.data(), doc.size(), true); // fires on_record(...)
```

To inject a subtype, subclass `Factory`, override `create_record()` to
return your derived type, and pass `&factory` to the `Marshaller`
constructor.

## Notes

- Marshalling is **caller-ordered**: you must call `marshal_*` parent
  before child (matching the schema's nesting); `marshal_*` only closes
  tags down to the element's parent, it does not reorder. `flush(true)`
  closes whatever remains open.
- `validate()` (and thus facet enforcement) runs on **unmarshal**, not on
  marshal — building an out-of-range object and marshalling it does not
  throw.
- Facet messages are intentionally terse (`throw std::runtime_error(
  "facet")` / `"mismatch"`); they identify a violation, not which field.
- Integer parsing goes through `std::stol`/`std::stoll`/`std::stoul`/
  `std::stoull` and is cast to the narrowed width — out-of-range text
  throws `std::out_of_range` from the standard library, surfaced through
  the same `unmarshal` rethrow path.
- One `xml` namespace and one `Element`/`Factory`/`Marshaller` triple per
  schema; generating two schemas into the same compilation unit would
  collide.
