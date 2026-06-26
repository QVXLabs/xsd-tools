# Capability map & limitations

What xsd-tools does and doesn't do with an XSD, and how it fails when it can't.

## Fully supported

Elements, attributes (incl. `default`/`fixed`/`use`), `complexType` /
`simpleType`, `sequence` / `choice` / `all`, named **group** and
**attributeGroup** definitions and references (including refs inside model
groups), `extension` / `restriction` over simple and complex content,
`simpleType` `union` / `list`, **substitutionGroup**, **abstract** types,
restriction **facets** (enforced in generated code), `xs:include` and
`xs:import` (same- and cross-namespace), namespaces / `targetNamespace` /
`elementFormDefault`, and `xs:documentation` (emitted as comments).

## Parsed but ignored (no effect on generated code)

These are accepted so real schemas that contain them work, but they don't
influence the generated marshalling code:

- Identity constraints — `key` / `keyref` / `unique` / `selector` / `field`
- `anyAttribute`
- `notation`
- `redefine` — **skipped** (its redefinitions are dropped, not applied); a
  schema that depends on `redefine` for its types will generate incomplete code
- `nillable`, `xsi:type` (runtime type substitution), and element-level
  `default` / `fixed`
- Attributes on **mixed-content** complex types (mixed content is modelled as a
  string; such attributes are dropped)
- `xs:documentation` embedded markup — the text is gathered, tags are ignored

## Not supported — fails with a clean error (never crashes)

- A genuinely-unknown element → `InvallidChildXMLElement`
- An `xs:import` / `xs:include` / `redefine` `schemaLocation` that can't be
  loaded (e.g. an `http://` URL, or a missing local file) →
  `ProtocolNotSupported` / a file-open error. Imports are resolved from the
  local filesystem only.
- Genuinely cyclic type derivation/reference → `CyclicTypeDefinition` (a
  self-referential *definition* like a recursive `tree` is also rejected,
  because the model inline-expands types)
- Documents the bundled **TinyXML 2.6.2** can't parse (notably a `<!DOCTYPE>`
  with an internal subset, and some encodings) → a parse error rather than
  generated code. TinyXML is the EOL XML layer; replacing it is out of scope.

All of the above are reported as a non-zero exit (see `xsdb`'s exit codes) — the
parser does not crash on hostile or malformed input.
