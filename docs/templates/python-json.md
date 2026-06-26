# python-json

Generates a Python module that marshals/unmarshals XSD-described data to and
from JSON using only the standard-library `json` module. No external
dependencies.

## Generate

```sh
xsdb python-json <schema.xsd>
```

The output is a single Python source file written to stdout.

## Toolchain & dependencies

- **python3** only. The generated code imports `json` (always), plus `base64`
  and/or `binascii` when the schema uses `base64Binary` / `hexBinary` types.
  Nothing outside the standard library is required.

## Generated API

For each element the generator emits a `json_<name>` class deriving from
`_jsonelement`, plus one root class `json_<schemaName>`.

- **Element classes** (`json_<name>`) hold attributes as `self._<attr>` fields
  and child content in `self._content`. Each attribute gets a getter
  `<attr>(self)` and a setter `set_<attr>(self, value)`. Attribute `default`
  values from the XSD are seeded in `__init__`.
- **marshal()** builds a dict
  `{"name": <name>, "attrs": {...}, "content": <content>}` and returns it.
  Attributes are typed-coerced (e.g. `int(self._attrib)`). For complex content,
  `content` is a list of child element dicts (each child's `marshal()`); for
  simple content it is the typed scalar/list.
- **unmarshal(obj)** reads the dict back, coercing attribute and content values
  to their Python types and reconstructing children.
- **Root class** `json_<schemaName>`: `marshal()` returns
  `json.dumps([...])` over its top-level children (the only place JSON text is
  produced); `unmarshal(obj)` consumes the parsed list. So marshal yields a JSON
  string and unmarshal takes already-parsed Python objects — pair the root's
  `marshal()` with `json.loads` on the consumer side.
- **Overridable factory registry.** A module-level `factories` dict maps each
  element name to its class. Every unmarshal path constructs children via
  `factories[name]()`, so assigning `factories['foo'] = MySubclass` injects a
  subclass without touching generated code.
- **Facet validation.** When an element or attribute carries facets, the class
  gains a `validate()` method (called at the end of `unmarshal`). It raises
  `ValueError` on violations: `range` (min/maxInclusive), `length`
  (length/min/maxLength, measured on `len(str(_v))`), and `enumeration`
  (numeric enums compare against a module-level `_ENUM_*` frozenset; string
  enums compare stringified). Pattern facets are not enforced.
- **Binary & list handling.** `base64Binary` marshals via
  `base64.b64encode(...).decode('utf-8')` and unmarshals via `b64decode`;
  `hexBinary` uses `binascii.hexlify(...).decode('utf-8')` / `bytes.fromhex`.
  XSD `list` item types map to per-element list comprehensions of the
  corresponding scalar coercion.

## Example

```xsd
<!-- rating.xsd: integer 1..5 -->
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
xsdb python-json rating.xsd > rating.py
```

Generated (abridged):

```python
class json_rating(_jsonelement):
    def marshal(self):
        attrs = {}
        content = self._content[0]
        return {"name": "rating", "attrs": attrs, "content": content}

    def unmarshal(self, obj):
        content = obj["content"]
        self._content = [int(content)]
        self.validate()

    def validate(self):
        _v = self._content[0] if self._content else None
        if _v is not None and not (1 <= _v <= 5):
            raise ValueError("content: out of range")
```

Round-trip:

```python
import json, rating

r = rating.json_rating()
r.set_... # populate via setters / _content as appropriate

doc = rating.json_rating_xsd()
doc.getContent().append(r)
text = doc.marshal()            # -> JSON string (json.dumps inside)

doc2 = rating.json_rating_xsd()
doc2.unmarshal(json.loads(text))  # validate() runs; bad values raise ValueError
```

## Notes

- **JSON has no attribute/element distinction.** Each element is represented as
  a dict with explicit `"attrs"` and `"content"` keys: XSD attributes go into
  the `"attrs"` sub-dict (keyed by attribute name), child elements go into the
  `"content"` list as nested element dicts, and simple-content values go
  directly into `"content"` as the typed scalar or list. The element's own name
  is carried in the `"name"` key, which is how the factory registry picks the
  class on unmarshal.
- Only the **root** class emits/consumes JSON text (`json.dumps`). Inner element
  classes work in plain Python dicts; feed the root's output through
  `json.loads` before calling its `unmarshal`.
- Facet validation runs only on `unmarshal`, not on `marshal` or via setters.
  Pattern facets are accepted by the parser but not enforced in generated code.
