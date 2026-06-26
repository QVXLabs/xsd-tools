# c-xml-expat-dom

C marshaller/unmarshaller for **XML** built on the **expat** parser, **DOM
style**: unmarshalling builds an in-memory document tree (`xml_typelist`) that
you traverse and can re-marshal whole. Scalars that fit in a pointer are stored
**inline** in the tree slot (no per-scalar heap allocation). For the
streaming/event variant see [c-xml-expat](c-xml-expat.md).

## Generate

Generate the DOM binding (multi-file output):

```sh
build/xsdb c-xml-expat-dom test/xsd-positive/testA026.xsd      # stdout
build/xsdb --out-dir out/ c-xml-expat-dom test/xsd-positive/testA026.xsd
# -> out/xml_testA026.h  out/xml_testA026.c
```

Output is split on `/* FILE: */` markers (`xml_<schema>.h`, `xml_<schema>.c`).

## Toolchain & dependencies

- A C compiler (round-trip harness uses `C_COMPILER` at **`-std=c11`**).
- Link against **expat** (`<expat.h>`, `-lexpat`).
- **libb64** is always pulled in here — the `.c` unconditionally
  `#include`s `b64/cencode.h` / `b64/cdecode.h`, so link the libb64 archive
  even for schemas without base64/hex.
- Standard headers used: `<inttypes.h>`, `<ctype.h>`, `<alloca.h>`.

Harness flow (`test/roundtrip_util.cpp`, `cExpatDomRoundtrip`): generate
binding + `c-xml-expat-dom-test` driver, split, then
`cc -std=c11 -I<dir> -I<libb64> <base>-bin.c xml_<base>.c <libb64.a> <expat>`
and run.

## Generated API

**Type model (the DOM tree).** The universal node is:

```c
typedef struct { unsigned type_; void* pData_; } xml_typelist;
```

A document is a `type_`-tagged, sentinel-terminated (`{0,NULL}`) array of these.
`type_` is a per-type enum (`XML_FOO`, `XML_NAME`, `XML_STRING`, ... — values
are `sdbm` type hashes). Each complex element is a heap struct whose `pContent_`
is itself an `xml_typelist*` child array, e.g.:

```c
typedef struct xml_foo {
    xml_typelist* pContent_;
    xml_integer   attrib2Optional;   /* int64_t */
    xml_integer   attrib2Required;
    xml_string    attrib1Optional;   /* char*   */
    xml_string    attrib1Required;
} * xml_foo;            /* note: the typedef is a POINTER */
```

**Inline scalar storage (the key difference vs SAX).** A scalar whose
`sizeof <= sizeof(void*)` is stored **directly in the `xml_typelist.pData_`
slot** via `memcpy` — no malloc per scalar. A wider scalar (e.g. a 64-bit value
on a 32-bit target) falls back to heap, with the slot holding the pointer.
`sizeof` folds the choice at compile time (`XML_SCALAR_FITS_`,
`XML_SCALAR_ENCODE_`/`LOAD_`/`FREE_`). The dispatch tables in
`xml_typelist_marshal_` / `xml_destroy` choose `MarshalContentScalar` /
`DestroyScalar` (pass the slot straight through) vs `MarshalContent` /
`Destroy` (cast the pointer) per type — `ctype:inlineScalar()` decides. Scalar
**attributes** that are required or have a default are stored by value;
optional-no-default scalar attributes keep a nullable pointer.

**Factory (Allocator/Constructor/Destructor triple).** `xml_typeFactory` holds
`xml_realloc(F, ptr, size, file, line)` plus, per element type, an
`xml_<name>Allocator` / `Constructor` / `Destructor`. NULL-safe via
`ReallocateMemory` / `AllocateElement` / `ConstructElement` / `DestroyElement`:
a missing factory or member falls back to `realloc`/`alloc_`/no-op.

**Marshal / unmarshal entry points** (the only three public functions):

```c
xml_typelist* xml_unmarshal(const char* pBuf, size_t size, const xml_typeFactory*);
char*         xml_marshal(const xml_typelist* pDoc, const xml_typeFactory*);
void          xml_destroy(xml_typelist** ppDoc, const xml_typeFactory*);
```

`xml_unmarshal` runs expat over the whole buffer in one shot (not streaming) and
returns the root node; the callbacks `startElement_`/`endElement_`/
`elementContent_` build the tree on an `xml_context_t_` stack (fixed depth =
schema's max element depth). `xml_marshal` walks the tree and returns a freshly
`malloc`'d `char*` (plain buffer, no hidden header — `free()` it). `xml_destroy`
tears the tree down through the factory.

**Facet validation.** Range / length / enum checks are emitted inline in
`endElement_` (via `c_facetChecks`) on the element's content string. **On
violation it calls `xml_invalid_`, which prints
`xml: facet violation (<label>)` to stderr and `exit(1)`s** — a hard abort, no
recovery. Pattern facets are skipped. Numeric enums/ranges are checked against
the `atoll`-parsed value, string enums via a collision-safe `sdbm` switch.

**Namespaces.** Gated on `targetNamespace`; non-namespaced schemas are
unchanged. A deterministic synthesized prefix (`c_nsTagName`/`namespace`
template) is used since the XSD source prefix isn't in the model — qualified
nodes are emitted `prefix:name` with `xmlns:` declared on the root; an
unqualified root binds the default `xmlns`. Element dispatch hashes the wire tag
(prefixed when qualified); hash collisions add a `strcmp` confirm.

**base64/hex & lists.** `base64Binary`/`hexBinary` use libb64 codecs.
`list`-typed values get a `xml_list_t_` growable array + a whitespace tokenizer
(`tokenizer_*`); the content list is sentinel-terminated.

## Example

```sh
build/xsdb --out-dir out/ c-xml-expat-dom test/xsd-positive/testA026.xsd
```

Unmarshal, traverse, re-marshal, destroy:

```c
xml_typelist* doc = xml_unmarshal(xml_text, len, NULL);  /* NULL = default factory */

for (xml_typelist* p = doc; p->type_; ++p) {
    if (p->type_ == XML_FOO) {
        xml_foo foo = (xml_foo)p->pData_;
        /* foo->attrib2Required is an int64_t read inline from the slot;
           foo->attrib1Required is a char*; foo->pContent_ is the child array */
    }
}

char* xml = xml_marshal(doc, NULL);   /* re-serialize; free(xml) when done */
free(xml);
xml_destroy(&doc, NULL);
```

To build a tree for marshalling by hand, populate `xml_foo` structs, point each
`pContent_` at a `{0,NULL}`-terminated `xml_typelist` child array, and pass the
root array to `xml_marshal`.

## Notes

- `xml_unmarshal` parses the **entire** buffer at once — it is not a streaming
  API (unlike SAX's chunked `xml_unmarshal`).
- The parse stack is a fixed-size array sized to the schema's max element depth;
  documents nested deeper than the schema allows are not supported.
- Element/attribute typedefs are **pointer** types (`xml_foo` is
  `struct xml_foo *`).
- Facet violations call `exit(1)`; don't feed untrusted input expecting graceful
  failure.
- libb64 is linked unconditionally for this target, even without base64/hex.
- **DOM vs SAX:** pick **c-xml-expat-dom** when you want the full document in
  memory as a navigable `xml_typelist` tree (random access, whole-document
  re-marshal, inline scalar storage minimizing allocations). Pick
  [c-xml-expat](c-xml-expat.md) for streaming / low-memory parsing where you
  handle each element via a callback as it closes.
