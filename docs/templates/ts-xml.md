# ts-xml

Generates strict TypeScript that marshals/unmarshals XML using
**fast-xml-parser**. One module per type, plus a small runtime base and an
overridable factory.

## Generate

```sh
xsdb --out-dir <dir> ts-xml <schema.xsd>
```

Output is multi-file: the template emits `/* FILE: ... */` markers and
`--out-dir` splits them into `Element.ts`, `Factory.ts`, `Document.ts`,
`index.ts`, and one `<Type>.ts` per element/type. (Without `--out-dir` the
combined stream goes to stdout with a leading `csplit` recipe.)

## Toolchain & dependencies

- **node** + **typescript** — compiles under `tsc --strict`.
- **fast-xml-parser** — `XMLParser` / `XMLBuilder` for parse and serialize.

The harness pins these in `test/ts-xml/package.json` (fast-xml-parser 4.4.1,
typescript 5.4.5). `test/ts-xml/tsconfig.json` targets **ES2019** — the oldest
target the generated code supports — with `strict: true` and CommonJS modules.

## Generated API

- **`<Type>.ts`** — one `class <Type> extends Element` per type. Attributes and
  leaf text are optional strict fields (`_attr?: T`, `_text?: T`); attribute
  defaults are assigned in the constructor. Methods: `localName()`,
  `marshalAttributes()` / `readAttributes()`, `marshalText()` / `readText()`
  for leaves, and `validate()`.
- **`Element.ts`** — abstract base. Holds ordered `children` and a `factory`
  reference, and converts to/from the fast-xml-parser `preserveOrder` node shape
  (`toNode()` / `readNode()`). `FxpNode` is the node type; `ATTR_PREFIX` is
  `"@_"`, `TEXT_KEY` is `"#text"`.
- **`Factory.ts`** — the overridable `Factory`. `create(localName)` dispatches a
  tag to a per-type `create<Type>()` (e.g. `createrecord()`). Subclass and
  override a `create<Type>()` to inject a custom subtype; `unmarshal` routes all
  element construction through it.
- **`Document.ts`** — `Document` wraps an `XMLParser`/`XMLBuilder` pair with
  shared `FXP_OPTIONS` (`preserveOrder`, symmetric attribute prefix,
  `trimValues: false`, `suppressEmptyNode`) so marshal -> parse -> marshal is
  byte-stable. `marshal(root)` / `unmarshal(xml)` are the entry points.
- **`index.ts`** — re-exports everything.

Feature mapping:

- **Facet validate()** — range / length / enumeration facets emit guards that
  `throw new Error(...)` on violation; `readNode` calls `validate()` after a
  node is populated. Enumerations hoist a module-level `ReadonlySet`. Pattern
  facets are not enforced.
- **Namespace** — when the schema has a `targetNamespace`, the document-root
  class declares `xmlns:tns` in `marshalAttributes()` and reports its
  `namespaceURI()`; locals are qualified only when their XSD form is.
- **base64 / hex** — `base64Binary` and `hexBinary` map to `Uint8Array`.
- **Lists** — XSD list types map to typed arrays (`number[]`, `string[]`,
  `boolean[]`).

## Example

```xml
<!-- nsTargetPrefixed.xsd -->
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
xsdb --out-dir ./out ts-xml nsTargetPrefixed.xsd
```

Generated `record.ts` (root, qualified in the target namespace):

```ts
import { Element } from "./Element";

export class record extends Element {
	_id?: string;

	constructor() { super(); }

	localName(): string { return "record"; }
	namespaceURI(): string | undefined { return "urn:example:phase0"; }

	protected marshalAttributes(): Record<string, string> {
		const a: Record<string, string> = {};
		a["xmlns:tns"] = "urn:example:phase0";
		if (this._id !== undefined) a["id"] = this._id;
		return a;
	}
	protected readAttributes(attrs: Record<string, string>): void {
		if ("id" in attrs) this._id = attrs["id"];
	}
}
```

`Factory.ts` (override a `create<Type>()` to swap in a subtype):

```ts
export class Factory {
	protected createrecord(): record { return new record(); }
	// ...createlabel(), createcount()...
	create(localName: string): Element | undefined {
		let e: Element | undefined;
		if (localName === "record") e = this.createrecord();
		else e = undefined;
		if (e) e.factory = this;
		return e;
	}
}
```

Marshal / unmarshal:

```ts
import { Document } from "./out";
import { record } from "./out/record";

const r = new record();
r._id = "r1";
const xml = new Document().marshal(r);     // serialize
const back = new Document().unmarshal(xml); // parse back through the Factory
```

## Notes

- Integer-class XSD types (`integer`, `long`, `int`, `decimal`, ...) map to
  `number`, not `bigint`. Values are exact only up to 2^53; this is the cost of
  the ES2019 target.
- fast-xml-parser `preserveOrder` keeps attribute order, whitespace, and
  self-closing tags stable, so round-trips are byte-stable.
- A `validate()` is emitted only for types carrying enforceable facets;
  pattern facets are accepted but not checked.
