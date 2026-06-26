# c-json-jsonc

Generates C marshalling/unmarshalling code that maps an XSD schema onto JSON,
using [json-c](https://github.com/json-c/json-c) for the JSON DOM.

## Generate

```sh
# print all files to stdout (separated by /* FILE: name */ markers)
xsdb c-json-jsonc schema.xsd

# split the marked output into real files under build/
xsdb --out-dir build c-json-jsonc schema.xsd
```

A schema named `schema.xsd` produces three files:

- `json_common.h` — shared `json_buffer` type, helper macros.
- `json_schema.h` — per-element structs, the `json_marshaller`, the factory
  table, and the public marshal/unmarshal prototypes.
- `json_schema.c` — the implementation.

The schema name is the root type with a trailing `_xsd` stripped (e.g.
`testA026.xsd` → `testA026`).

## Toolchain & dependencies

- A C11 compiler. The round-trip test harness compiles the generated code with
  the project C compiler at `-std=c11`.
- **json-c** — the generated `.c` includes `<json.h>` and links `libjson-c`.
- **libb64** — only required when the schema uses `base64Binary` or `hexBinary`;
  the generated `.c` then includes `b64/cencode.h` / `b64/cdecode.h` and must
  link libb64.

## Generated API

### Type model

Each XSD element becomes a `json_<name>` struct. Attributes and element content
are struct members; the content member is always named `Content_`. Members are
suffixed `_`; string (`char*`) members are additionally `p`-prefixed
(`pattrib1Required_`). XSD scalar types map to fixed-width C types
(`integer`→`int64_t`, `int`→`int32_t`, `boolean`→`int`, `string`→`char*`,
`decimal`→`long double`, …). An element that has child elements (or is otherwise
empty) carries a `uint32_t eid_` filler, used to seed children's `parentId`.

```c
typedef struct {
	uint32_t	eid_;
	int64_t		attrib2Optional_;
	char*		pattrib1Required_;
	char*		pattrib1Optional_;
	int64_t		attrib2Required_;
} json_foo;
```

### Overridable factory

`json_<schema>_factories` holds an Allocator / Constructor / Destructor triple
per element type. Every element struct on the unmarshal path is allocated,
constructed, and destroyed through this triple, so a caller can inject custom
construction by supplying their own table to `json_unmarshal_ex`. The dispatch
is NULL-safe: a NULL table, or any NULL member, falls back to the defaults
(zeroed `malloc` allocate, no-op construct, `realloc(p,0)` free). The default
table is returned by `json_<schema>_default_factories()`; the default
Constructor pointer is NULL (no-op).

### json-c build / parse

Per element, `build_<name>_()` creates a fresh `json_object` and adds each
attribute under its own key and the content under the key `"$"`. Numbers use
`json_object_new_int64` / `_double`, strings `json_object_new_string`. Unmarshal
reads back with `json_object_object_get_ex` + `json_object_get_*`.

### Marshal / unmarshal entry points

```c
json_marshal_<schema>_xsd json_marshal(json_marshaller* pMarshaller);
json_buffer json_marshal_flush(json_marshaller* pMarshaller, int isDocEnd);
void json_unmarshal(json_marshaller* pMarshaller, const json_buffer* pBuf,
                    int isDocEnd);
void json_unmarshal_ex(json_marshaller* pMarshaller, const json_buffer* pBuf,
                       int isDocEnd, const json_<schema>_factories* pFactories);
```

`json_marshal` returns a struct of `marshal_<child>` function pointers plus the
parent `json_object* pSelf`; you walk the tree by calling a child's marshaller
with its parent's `pSelf`, which (for non-leaf children) returns the next level
of marshallers. `json_marshal_flush` serializes the root to a `json_buffer` and,
when `isDocEnd` is set, releases the root. Unmarshalling is push-style: you
populate `json_marshaller`'s `unmarshal_<name>` callbacks; the parser calls each
callback with a fully-populated, stack-lifetime `json_<name>*` and the parent's
`eid_`. The object is destroyed immediately after the callback returns — copy
out anything you need to keep. `realloc` on the marshaller is the single
required hook (allocator for the whole pipeline).

### Facet validation

For an element whose content/attributes carry restriction facets, a
`static int validate_<name>_(const json_<name>*)` is emitted, returning non-zero
on violation. Numeric `minInclusive`/`maxInclusive` become a range check;
string `length`/`min`/`maxLength` become a `strlen` check; enumerations switch
on the seed-0 SDBM hash of the value (strcmp-confirmed on hash collision). On the
unmarshal path the validation guards only the notify callback: an invalid object
is **silently skipped** (the `unmarshal_<name>` callback never fires) but is
still destroyed normally. Marshalling does not validate. Pattern facets are not
enforced. Integer ranges with both bounds known also narrow the C member to the
smallest fixed-width int that holds the range (the `1..5` `integer` below becomes
`uint8_t`).

### base64 / hex / list

`base64Binary` → `json_base64`, `hexBinary` → `json_hex` (both
`{ uint8_t* pBuffer_; uint32_t size_; }`); base64 marshals through libb64's
`base64_encode_block` to a JSON string, hex similarly. An XSD `list` becomes a
`json_<base>List` struct (`{ <base>* pArray_; uint32_t nElements_; }`) marshalled
as a JSON array.

## Example

`rating.xsd`:

```xml
<schema>
	<element name="rating" type="ratingType" />
	<simpleType name="ratingType">
		<restriction base="integer">
			<minInclusive value="1"/>
			<maxInclusive value="5"/>
		</restriction>
	</simpleType>
</schema>
```

```sh
xsdb --out-dir build c-json-jsonc rating.xsd
```

The range narrows the member to `uint8_t` and emits a validator:

```c
typedef struct {
	uint8_t	Content_;
} json_rating;

static int validate_rating_(const json_rating* pObj) {
	if (!((pObj->Content_) >= 1 && (pObj->Content_) <= 5))
		return -1;
	return 0;
}
```

Driving marshal then unmarshal:

```c
static void* rc(struct json_marshaller* m, void* p, size_t s) {
	(void)m; return realloc(p, s);
}
static void on_rating(struct json_marshaller* m, const json_rating* o,
                      uint32_t parent) {
	(void)m; (void)parent;
	printf("rating = %u\n", o->Content_);   /* not called if out of 1..5 */
}

int main(void) {
	json_marshaller m = { .realloc = rc, .unmarshal_rating = on_rating };
	json_rating r = { .Content_ = 3 };

	json_marshal_rating_xsd s = json_marshal(&m);
	s.marshal_rating(&m, &r, s.pSelf);
	json_buffer out = json_marshal_flush(&m, 1);

	json_unmarshal(&m, &out, 1);          /* fires on_rating with 3 */
	out.pBuf_ = rc(&m, out.pBuf_, 0);
	return 0;
}
```

## Notes

- **JSON has no attribute/element distinction.** Attributes and child elements
  alike become object keys; element *content* (simple-type text) is stored under
  the reserved key `"$"`. A schema that uses `"$"` as an attribute name would
  collide.
- **Unmarshal is push/callback-style and objects are transient.** The
  `json_<name>*` handed to your callback is freed as soon as the callback
  returns; deep-copy anything you keep.
- **Attribute defaults** are applied before reading the JSON: the default is
  parsed as a JSON literal and assigned, then overwritten if the key is present.
- **Invalid values are dropped, not reported.** Facet validation failure skips
  the notify callback with no error signal; there is no way to distinguish "field
  absent" from "field present but invalid" at the callback boundary.
- Adding a new output target is template-only — no C++ change. This target lives
  in `templates/c-json-jsonc` plus the per-type subtemplates under
  `templates/includes/c-json-jsonc/`.
