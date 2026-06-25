# java-xml-stax

Generates Java marshalling/unmarshalling code that maps an XSD schema onto XML
using the JDK's built-in StAX API (`javax.xml.stream`). No external XML
dependency — marshal goes through `XMLStreamWriter`, unmarshal through a pull
loop over `XMLStreamReader`.

## Generate

```sh
# print all files to stdout (separated by /* FILE: name */ markers)
xsdb java-xml-stax.tmpl schema.xsd

# split the marked output into real files under build/
xsdb --out-dir build java-xml-stax.tmpl schema.xsd
```

The output package defaults to `com.mobitv.app`; override with
`-package <name>`. (`-h` prints the template's own argument help.)

## Toolchain & dependencies

- **JDK only.** StAX (`XMLStreamWriter` / `XMLStreamReader` /
  `XMLInputFactory` / `XMLOutputFactory`) is part of the standard library, so
  the generated code compiles and runs against a bare JDK — no XML library on
  the classpath. The test harness builds at `source`/`target` 1.8.
- **`java.util.Base64`** — used (fully qualified) only when the schema has a
  `base64Binary` content/attribute.
- A `Hex.java` helper class is emitted only when the schema uses `hexBinary`.

## Generated API

For a schema, the template always emits four scaffold classes plus one class
per complex type:

- **`Element.java`** — abstract base. Holds a text `StringBuilder _buf`, a lazy
  `List<Element> _children`, and a `Factory _factory`. `marshal(XMLStreamWriter)`
  writes the start element (namespaced or not), then namespaces, attributes,
  content, recurses into children, and writes the end element.
  `unmarshal(XMLStreamReader)` is the StAX pull loop: on `START_ELEMENT` it
  creates a child via the factory by local name and recurses; `CHARACTERS`/`CDATA`
  append to `_buf`; `END_ELEMENT` calls `readContent()` then `validate()`.
  Subclasses override only the hooks that differ (`localName`, `namespaceURI`,
  `marshalNamespaces`, `marshal/readAttributes`, `marshal/readContent`,
  `validate`).
- **`Factory.java`** — overridable factory. Has one `protected create<Type>()`
  per non-root type and a `create(String localName)` dispatcher. Subclass and
  override a `create<Type>()` to inject a custom subtype into unmarshalling.
- **`Document.java`** — document entry points. `String marshal(Element root)`
  serializes to a string via `XMLOutputFactory`; `Element unmarshal(String xml)`
  reads via `XMLInputFactory`, creating the root through the (overridable)
  factory. Construct with `new Document()` or `new Document(Factory)`.
- **`<Type>.java`** — one per complex type. Private attribute fields, an
  optional `_text` content field, a constructor that applies attribute
  defaults, getters/`set_*` setters, and the overridden marshal/unmarshal/
  validate hooks.

### Marshal / unmarshal

Marshal is pure StAX: `w.writeStartElement(...)`, `w.writeAttribute(...)`,
`w.writeCharacters(...)`. Unmarshal dispatches children by **URI-less local
name** (`r.getLocalName()`); attributes are read by local name in a
`getAttributeCount()` loop. Numeric content/attributes parse with
`Long.parseLong(... .trim())` (or the narrowed type's parser, below).

### Facet validation

When a content/attribute carries restriction facets, the type's `validate()`
override runs after unmarshal. Each value's checks are scoped in their own block
(local `_v`); a violation throws `IllegalArgumentException` (unlike the C target,
which silently drops). `minInclusive`/`maxInclusive` → range check;
`length`/`min`/`maxLength` → `String.valueOf(_v).length()` check;
`enumeration` → numeric `_v == ...` comparisons for boxed-numeric types, or a
shared static `HashSet<String>.contains(_v)` for strings. **Pattern facets are
not enforced.**

### Smallest-type narrowing

For an `integer`/`long` field whose facets bound it to a known int range, the
member, accessors, and parse call narrow to the smallest signed boxed type that
holds the range (`Byte`/`Short`/`Integer`/`Long`). Java has no unsigned types,
so e.g. `0..255` widens to `Short`.

### Namespaces

When the schema has a `targetNamespace`, qualified elements override
`namespaceURI()` to return the URI, so `marshal` emits a namespaced start tag.
The single document-root element also overrides `marshalNamespaces()` to
`w.writeNamespace("tns", <uri>)`, and `Document.marshal` calls
`w.setPrefix("tns", <uri>)` before writing the root — binding a synthesized
`tns:` prefix (the original XSD source prefix is not in the model). Local
elements/attributes are qualified only when their XSD `form` is qualified.
**Unmarshal matches on bare local names**, ignoring the URI. A non-namespaced
schema emits byte-identical output to the pre-namespace template (no
`namespaceURI`/`marshalNamespaces`/`setPrefix`).

### base64 / hex / list

`base64Binary` marshals via `java.util.Base64.getEncoder()`/`getDecoder()`;
`hexBinary` via the emitted `Hex` helper. XSD `list` types map to Java
`List`-backed fields (string/int/long/bool/float/base64/hex variants).

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

```sh
xsdb --out-dir build java-xml-stax.tmpl record.xsd
```

The root type (`record`, sanitized to `record_` since it collides with no Java
keyword but is reserved here) emits the namespace overrides:

```java
public class record_ extends Element {
	private String _id;

	@Override public String localName() { return "record"; }

	@Override protected String namespaceURI() { return "urn:example:phase0"; }

	@Override protected void marshalNamespaces(XMLStreamWriter w)
			throws XMLStreamException {
		w.writeNamespace("tns", "urn:example:phase0");
	}

	@Override protected void marshalAttributes(XMLStreamWriter w)
			throws XMLStreamException {
		if (_id != null) w.writeAttribute("id", _id);
	}
	// ... readAttributes, getters/setters ...
}
```

Driving marshal then unmarshal:

```java
record_ r = new record_();
r.set_id("r1");

Document doc = new Document();
String xml = doc.marshal(r);   // <tns:record xmlns:tns="urn:example:phase0" id="r1"/>
Element back = doc.unmarshal(xml);  // matched by local name "record"
```

## Notes

- **Type names are sanitized for Java**, which can rename a class relative to
  the XSD element (`record` → `record_` above). `localName()` still returns the
  original XSD name, so on-the-wire element names are unaffected.
- **Unmarshal ignores namespace URIs** — it dispatches purely on local name, so
  same-local-name elements from different namespaces are not distinguished.
- **A single `tns:` prefix is synthesized** for the one target namespace; the
  original source prefix from the XSD is not carried in the model.
- **Pattern facets are not enforced**, and the `LuaProcessor` collapses
  restrictions to their base, so only the facet kinds listed above validate.
- **Facet violations throw `IllegalArgumentException`** from `validate()` on the
  unmarshal path; marshalling does not validate.
- Adding a new output target is template-only — no C++ change. This target lives
  in `templates/java-xml-stax.tmpl` plus the per-type subtemplates under
  `templates/java-xml-stax/`.
```