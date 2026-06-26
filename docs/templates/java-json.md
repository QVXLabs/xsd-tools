# java-json

Generates Java marshalling/unmarshalling code that maps an XSD schema onto JSON
using the `org.json` library (`JSONObject` / `JSONArray`). Marshal builds a
`JSONObject`; unmarshal consumes one. Binary content is base64/hex-encoded via
Apache Commons Codec.

## Generate

```sh
# print all files to stdout (separated by /* FILE: name */ markers)
xsdb java-json.org schema.xsd

# split the marked output into real files under build/
xsdb --out-dir build java-json.org schema.xsd
```

The output package defaults to `com.mobitv.generated.json`; override with
`-package <name>`. (`-h` prints the template's own argument help.)

## Toolchain & dependencies

- **JDK.** The test harness builds at `source`/`target` 1.8; CI runs Temurin
  JDK 21. Java 11+ is fine.
- **`org.json`** — the JSON model. Maven coordinates:
  `org.json:json:20231013`.
- **`org.apache.commons.codec`** — base64/hex encoding. Maven coordinates
  `org.apache.directory.studio:org.apache.commons.codec:1.8` in the test pom
  (the package provides `org.apache.commons.codec.binary.Base64` and `Hex`).
  Note this target uses Commons Codec, **not** `java.util.Base64`.

## Generated API

The template always emits three scaffold classes plus one class per complex
type:

- **`Marshalable.java`** — interface with `JSONObject marshal()`.
- **`JSONObjectAdapter.java`** — thin wrapper over `JSONObject` with typed
  `getString`/`getLong`/`getObject`/`getList`/`has` and overloaded `put`. Only
  the accessor/`put` overloads the schema actually needs are emitted.
- **`JSONArrayAdapter.java`** — wrapper over `JSONArray` with `length()`, `put`,
  and (when the schema uses binary lists) `getBase64`/`getHex` /
  `putBase64`/`putHex`.
- **`<Type>.java`** — one per complex type. Private `_`-prefixed fields,
  getters/`set*` setters, a `marshal()`, a `validate()`, and three constructors:
  a default one, `T(JSONObject)`, and `T(JSONObjectAdapter)` (the latter does
  the real unmarshal).

### Marshal / unmarshal

- **Unmarshal** is the `T(JSONObjectAdapter)` constructor: each field is read by
  key, optional fields guarded by `jObj.has(key)` (absent → declared default, or
  `null`), then `validate()` runs at the end. Attributes are keyed with a `@`
  prefix (`@attrib1Required`); simple-content text uses the key `$`.
- **Marshal** builds a fresh `JSONObjectAdapter` and `put`s each present field.
  Optional `String` fields are emitted only when non-null and non-empty;
  optional boxed numerics only when non-null.
- **Nested complex types** are unmarshalled by direct construction:
  `_item = new Item(jObj.getObject("item"))`, and marshalled via
  `_item.marshal()`. (There is no overridable factory layer in this target.)

### Facet validation

When a field carries restriction facets, `validate()` enforces them after
unmarshal and throws `IllegalArgumentException` on violation:

- `minInclusive`/`maxInclusive` → range check
  (`if (_v != null && !(lo <= _v && _v <= hi)) throw ...`).
- `length`/`minLength`/`maxLength` → string length check.
- `enumeration` → a shared `private static final Set<String>` is emitted and
  checked with `.contains(_v)` for strings.

**Pattern facets are not enforced.** Marshalling does not validate.

### Smallest-type narrowing

For an `integer`/`long` field whose facets bound it to a known int range, the
member, accessors, and unmarshal call narrow to the smallest signed boxed type
that holds the range (`Byte`/`Short`/`Integer`/`Long`). Java has no unsigned
types, so e.g. `0..255` widens to `Short`. `Byte`/`Short` read via
`(byte) jObj.getInt(...)` / `(short) jObj.getInt(...)` and marshal via
`.intValue()` (those two lack dedicated adapter methods).

### base64 / hex / list

XSD `list` types map to `List`-backed fields. `base64Binary` items decode with
`Base64.decodeBase64` and encode with `Base64.encodeBase64String`; `hexBinary`
items decode with `Hex.decodeHex` and encode with `Hex.encodeHexString` (which
can throw `DecoderException`, so unmarshal constructors are declared
`throws JSONException, DecoderException`). A binary field's Java type is
`List<byte []>`.

## Example

`order.xsd`:

```xml
<schema>
  <element name="order">
    <complexType>
      <sequence>
        <element name="qty">
          <simpleType>
            <restriction base="int">
              <minInclusive value="1"/>
              <maxInclusive value="100"/>
            </restriction>
          </simpleType>
        </element>
        <element name="item">
          <complexType>
            <attribute name="sku" type="string" use="required"/>
          </complexType>
        </element>
      </sequence>
    </complexType>
  </element>
</schema>
```

```sh
xsdb --out-dir build java-json.org order.xsd
```

`qty` narrows to `Byte` (range 1..100) and gets a range check; `item` becomes a
nested `Item` class:

```java
public class Order implements Marshalable {
	private Byte _qty;
	private Item _item = null;

	public Order(JSONObjectAdapter jObj)
			throws JSONException, DecoderException {
		if (!jObj.has("qty")) _qty = null;
		else                  _qty = (byte) jObj.getInt("qty");
		if (!jObj.has("item")) _item = null;
		else                   _item = new Item(jObj.getObject("item"));
		validate();
	}

	public void validate() {
		if (_qty != null && !(1 <= _qty && _qty <= 100))
			throw new IllegalArgumentException("qty: out of range");
	}

	public JSONObject marshal() throws JSONException {
		JSONObjectAdapter retObj =
			new JSONObjectAdapter(new JSONObject());
		if (null != _qty) retObj.put("qty", _qty.intValue());
		if (null != _item) retObj.put("item", _item.marshal());
		return retObj.getJSONObject();
	}
}
```

Driving marshal then unmarshal:

```java
Order o = new Order();
o.setQty((byte) 5);

JSONObject json = o.marshal();         // {"qty":5}
Order back = new Order(json);          // re-validates qty range
```

## Notes

- **Attribute keys carry a `@` prefix and simple-content text uses `$`** in the
  JSON object — chosen so attributes and element content can coexist as distinct
  keys.
- **Adapter methods are emitted on demand** — `JSONObjectAdapter` /
  `JSONArrayAdapter` only get the getters/`put` overloads the schema uses, so two
  schemas can produce different adapter classes.
- **The base64/hex decoders may throw `DecoderException`**, which propagates out
  of the unmarshal constructors (declared `throws JSONException,
  DecoderException`).
- **Pattern facets are not enforced**, and the `LuaProcessor` collapses
  restrictions to their base, so only the facet kinds listed above validate.
- **Facet violations throw `IllegalArgumentException`** from `validate()` on the
  unmarshal path; marshalling does not validate.
- **There is no factory/subtype-injection layer** in this target — nested types
  are constructed directly. (The `java-xml-stax` target is the one with an
  overridable `Factory`.)
- Adding a new output target is template-only — no C++ change. This target lives
  in `templates/java-json.org` plus the subtemplates under
  `templates/includes/java-json.org/`.
```