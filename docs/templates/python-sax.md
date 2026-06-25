# python-sax

Generates a self-contained Python module that marshals/unmarshals XML using
only the standard library `xml.sax` parser — no external dependencies.

## Generate

```sh
xsdb python-sax <schema.xsd>
```

Emits a single Python module to **stdout**; redirect it to a `.py` file.

## Toolchain & dependencies

Just **python3**. The generated module imports only `xml.sax` from the standard
library. `import base64` / `import binascii` are added on demand, but only when
the schema actually uses `base64Binary` / `hexBinary` types.

## Generated API

The module is a flat set of classes plus a SAX handler.

- **`_xmlelement`** — base class for every element. Holds an ordered
  `_content` list (mixed child `_xmlelement`s and text strings); `getContent()`
  returns it.
- **`xml_<element>`** — one class per element/type in the schema, derived from
  `_xmlelement`. Provides:
  - `marshal(self, stream)` — writes the element's XML to a writable stream.
  - `unmarshal(self, attributes, content)` — populates the instance from a SAX
    attribute dict and a content list.
  - per-attribute getter `attr()` / setter `set_attr(value)`; attribute
    defaults are set in `__init__`.
- **`_handler(xml.sax.ContentHandler)`** — the SAX content handler. It drives
  parsing with **namespace-aware** callbacks (`startElementNS` / `endElementNS`),
  maintaining an element stack and building the tree bottom-up by calling each
  element's `unmarshal`.
- **`xml_<schema>_xsd(_handler, _xmlelement)`** — the document root class,
  combining the handler and an element. This is the entry point:
  - `marshal(self, stream)` — writes the XML declaration then marshals each
    top-level element.
  - `unmarshal(self, stream)` — builds a parser, enables
    `feature_namespaces`, parses `stream`, and stores the result in
    `_content`.

### Overridable element factory

The handler maps each element's **local name** to a constructor via the
`self._elemTbl` dict, built in `_handler.__init__`:

```python
self._elemTbl = {
    'foo': lambda: xml_foo(),
    'name': lambda: xml_name(),
}
```

`startElementNS` looks up `self._elemTbl[name[1]]()` (where `name` is the
`(uri, localname)` pair). To inject a subtype, subclass the document root and
reassign an entry in `_elemTbl` after the base `__init__`.

### Facet validation

When an element's content or an attribute carries XSD facets, the class gets a
`validate()` method that `unmarshal` calls automatically. Checks raise
`ValueError` on violation. Supported facet kinds: numeric **range**
(`minInclusive`/`maxInclusive`), **length** (`length`/`minLength`/`maxLength`),
and **enumeration** (membership against a module-level `frozenset`). Pattern
facets are not enforced.

```python
def validate(self):
    _v = self._content[0] if self._content else None
    if _v is not None and not (1 <= _v <= 5):
        raise ValueError("content: out of range")
```

### Namespaces

If the schema has a target namespace, the document-root element emits an
`xmlns` (or `xmlns:<prefix>`) declaration on its open tag during marshalling;
qualified elements are written with a synthesized prefix (`ns`, or `ns1`, `ns2`
… for multiple URIs). Because parsing uses `feature_namespaces`, elements are
matched by **(uri, localname)**, so the wire prefix is irrelevant on input.

## Example

`record.xsd`:

```xml
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

Generate:

```sh
xsdb python-sax record.xsd > record.py
```

A generated class (note the root-level `xmlns` and the `id` attribute):

```python
class xml_record(_xmlelement):
    def __init__(self):
        _xmlelement.__init__(self)
        self._id = None

    def marshal(self, stream):
        stream.write('<record xmlns="urn:example:phase0"')
        stream.write(' id="' + str(self._id) + '"')
        stream.write('>')
        for node in filter(lambda x: isinstance(x, _xmlelement), self.getContent()):
            node.marshal(stream)
        stream.write('</record>')

    def unmarshal(self, attributes, content):
        if "id" in attributes:
            self._id = attributes["id"]
        self._content = content
```

Use it:

```python
import sys
from record import xml_record_xsd

# unmarshal from a file
doc = xml_record_xsd()
with open('in.xml') as f:
    doc.unmarshal(f)

# inspect / mutate the tree via getContent(), then marshal back out
doc.marshal(sys.stdout)
```

## Notes

- `unmarshal` takes a **stream**, not a string; wrap text in `io.StringIO`.
- The element tree is the mixed `getContent()` list (child elements and text);
  there are no typed child accessors — walk `getContent()` and check
  `isinstance(node, _xmlelement)`.
- The handler's `startDocument` seeds the stack with a fixed placeholder tuple;
  the real root content is collected from the stack after parsing.
- Pattern facets are accepted by the schema but not validated.
- Marshalling does no validation; `validate()` runs only on `unmarshal`.
