# c-xml-expat

C marshaller/unmarshaller for **XML** built on the **expat** SAX parser. This is
the **SAX-style** target: unmarshalling is event-driven — element structs are
filled on `expat` start/end/character callbacks and handed to your code one at a
time. No document tree is built. For the tree-building variant see
[c-xml-expat-dom](c-xml-expat-dom.md).

## Generate

Single-file to stdout:

```sh
build/xsdb c-xml-expat test/xsd-positive/testA026.xsd
```

The output is one stream containing three `/* FILE: */`-marked units
(`xml_common.h`, `xml_<schema>.h`, `xml_<schema>.c`). Split it to disk with
`--out-dir`:

```sh
build/xsdb --out-dir out/ c-xml-expat test/xsd-positive/testA026.xsd
# -> out/xml_common.h  out/xml_testA026.h  out/xml_testA026.c
```

(`csplit - '/\/\* FILE: /' {*}` does the same from a piped stream.)

## Toolchain & dependencies

- A C compiler (round-trip harness uses the project `C_COMPILER` at
  **`-std=c11`**).
- Link against **expat** (`#include <expat.h>`, `-lexpat`).
- **libb64** only when the schema uses `base64Binary`/`hexBinary` — the `.c`
  then `#include`s `b64/cencode.h` / `b64/cdecode.h` and you link the libb64
  archive.

Harness flow (`test/roundtrip_util.cpp`, `cExpatRoundtrip`): generate the
binding + the `c-xml-expat-test` driver, `SplitMarkedFiles` into a temp dir,
then `cc -std=c11 -I<dir> -I<libb64> <base>-bin.c xml_<base>.c <libb64.a>
<expat>` and run.

## Generated API

**Type model.** Each non-root element becomes a flat C struct `xml_<name>` with
a `uint32_t eid_` id, one field per attribute, and (for simple-content
elements) a `Content` field. Scalars use fixed C types (`int64_t` for
`integer`, `char*` for `string`, etc.); strings are `char*` (`p`-prefixed
field names). Example from `testA026.xsd`:

```c
typedef struct { uint32_t eid_; char* pContent_; } xml_name;
typedef struct {
    uint32_t eid_;
    int64_t  attrib2Optional_;
    char*    pattrib1Required_;
    char*    pattrib1Optional_;
    int64_t  attrib2Required_;
} xml_foo;
```

**Factory (Allocator/Constructor/Destructor triple).** `xml_typeFactory` holds
an `xml_realloc` plus, per element type, an `xml_<name>Allocator` /
`Constructor` / `Destructor`. All are optional: a NULL factory or NULL member
falls back to **in-place** construction (element structs live by value inside
the parse-stack record, so the default Allocator returns the in-place pointer
rather than heap-allocating) and scalar free. Dispatched through the NULL-safe
`AllocateElement` / `ConstructElement` / `DestroyElement` macros.

**Marshal (serialize).** `xml_marshal(xml_marshaller*)` returns a struct of
function pointers (`xml_marshal_<root>`) that you call top-down to walk into
child elements; leaf children get a `void` `marshal_<name>` pointer, branch
children return their own nested function-pointer struct. Each call appends the
element's start tag + attributes + content to an internal buffer.
`xml_marshal_flush(m, isDocEnd)` returns the accumulated `xml_buffer` (and on
`isDocEnd` closes any still-open tags via `marshal_adjustElmStk_`). The
marshaller is stack-driven: open tags are closed lazily when a sibling/parent
of a different element type is emitted.

**Unmarshal (parse).** `xml_unmarshal(m, &buf, isDocEnd)` feeds bytes to expat;
call repeatedly for streaming, with `isDocEnd != 0` on the last chunk (it then
frees the parser). You supply per-element callbacks
`m->unmarshal_<name>(m, const xml_<name>* pObj, uint32_t parent)` that fire on
each element's **end** tag, receiving the fully-populated struct and its
parent's id. Element dispatch is by `sdbm` hash of the (prefix-stripped) tag
name; attributes likewise. Attribute `default`s are pre-applied before the
attribute loop overwrites them.

**Facet validation.** When an element's content or an attribute carries facets
(range / length / enum), a `validate_<name>_` function is emitted and invoked in
`prsElmRec_invoke_` before your callback. **On an invalid value it prints
`<name>: facet validation failed` to stderr and `abort()`s** — there is no
soft-error path. Pattern facets are not checked.

**Namespaces.** All NS emission is gated on the schema having a
`targetNamespace`; non-namespaced schemas are byte-identical to before the NS
feature. When present, a single `tns` prefix is declared via
`xmlns:tns="<uri>"` on the document-root start tag, and qualified
elements/attributes are emitted as `tns:name`. On unmarshal expat is created
NS-aware (separator `':'`) and tags/attrs are matched on the **local** name
(prefix stripped via `strrchr(..., ':')`).

**base64/hex & lists.** `base64Binary`/`hexBinary` pull in libb64 and get
dedicated codec functions. `list`-typed values get list marshal/unmarshal
helpers; the type table separates list from non-list C types deterministically
(sorted by C type name) so output is reproducible.

## Example

Schema (`testA026.xsd`): a `foo` element with four attributes (two with
defaults, two required) and an optional `name` child.

```sh
build/xsdb --out-dir out/ c-xml-expat test/xsd-positive/testA026.xsd
```

Marshal:

```c
xml_marshaller m = { .realloc = my_realloc };
xml_marshal_testA026_xsd root = xml_marshal(&m);
xml_foo foo = { .eid_ = 1, .attrib1Required_ = "r",
                .attrib2Required_ = 7, .attrib1Optional_ = "o",
                .attrib2Optional_ = 5 };
xml_marshal_foo f = root.marshal_foo(&m, &foo);
xml_name nm = { .pContent_ = "hi" };
f.marshal_name(&m, &nm);
xml_buffer out = xml_marshal_flush(&m, 1);   /* out.pBuf_ = the XML text */
```

Unmarshal:

```c
void on_foo(xml_marshaller* m, const xml_foo* o, uint32_t parent) { /* use o */ }
xml_marshaller m = { .realloc = my_realloc, .unmarshal_foo = on_foo };
xml_buffer in = { (uint8_t*)xml_text, 0, (uint32_t)len };
xml_unmarshal(&m, &in, 1);   /* fires on_foo at </foo> */
```

You must supply `m.realloc` (and may supply `m.pFactory_`).

## Notes

- Element structs are embedded by value in the parse stack, so the default
  (factory-less) path does no per-element heap allocation; only `char*`
  attributes/content are heap-copied.
- Marshalling closes tags lazily — you must call `xml_marshal_flush(m, 1)` at
  document end to emit the final closing tags.
- Facet violations **abort the process**; do not feed untrusted input expecting
  graceful failure.
- **SAX vs DOM:** pick **c-xml-expat** when you want low memory / streaming and
  are happy to handle elements via callbacks as they close. Pick
  [c-xml-expat-dom](c-xml-expat-dom.md) when you want the whole document as an
  in-memory `xml_typelist` tree you can traverse and re-marshal.
