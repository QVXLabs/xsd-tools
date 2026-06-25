# Workstream E — XML namespace awareness + `xs:import`

Planning doc. PLANNING ONLY — no source/template edits. All paths absolute.
Branch `revitalize`. Line/symbol references re-verified against current code
on `cmake-migration` (post Workstream F functional-recursion refactor —
`Node::_eachChild`/`_findChild` exist; post Workstream D facet bridge —
`LuaFacets`/`m_facets`/`LuaType::Facets` landed; post Workstream D2 —
`shared/trnsfrm_old` now propagates facets onto the normalized model,
`shared/facets` exists as a pure helper, and every XML-emitting binding
template consumes it to emit validation. **Workstream D is now COMPLETE
(uncommitted on `cmake-migration`)** — beyond the content-facet bridge it
added: (1) an **attribute-facet bridge** (`LuaProcessor::ProcessAttribute`
walks facets via the new `_walkAttributeFacets`, attaches via new
`LuaAttribute::Facets`, reaching the restriction through the **new
`SimpleType::GetRestriction()` accessor**; `trnsfrm_old` carries
`attr.<type>.facets`); (2) a **minOccurs bridge** (`Element::MinOccurs()`/
`HasMinOccurs()` thread through `LuaProcessor::ProcessElement` + `LuaContent::
Type`/`LuaType` via `MINOCCURS_TAG`); (3) `shared/facets` narrowing helpers
(`facets_narrowKey`/`facets_narrowType`/`facets_narrowestSigned`/
`sdbm_group`); (4) a `c-xml-expat` determinism fix (`uniqueCTypes` sorted by
name). D's edits shifted some line anchors below (re-confirmed); the core
resolution surface (Node.cpp/Schema.cpp/Parser/TypesDB) held byte-for-byte —
D was bridge + templates + tests, not a resolution-path change. **Three
model-mutation precedents now exist** (content facets, attr facets,
minOccurs) — all `empty()`/present-gated to avoid golden churn; E3's
`namespace`/`qualified` fields must follow the same gating (the goldens were
just regenerated for attr-facets + minOccurs + a `pairs()` reorder, so E3
churn discipline matters more, not less). 7 output targets.

---

## 1. Findings (traced)

### 1.1 Two distinct "type tables"
- `/Users/afalls/code/xsd-tools/src/XSDParser/TypesDB.cpp` is **only the
  builtin XSD primitive registry** (`anyURI`, `string`, `int`, …),
  populated in `TypesDB::TypesDB()` (TypesDB.cpp:28-78). Keyed by **bare
  local name** in a `std::map`. `FindType(const char*)` (TypesDB.cpp:89-95;
  a `std::string` overload at 98-100 forwards to it) is a plain local-name
  lookup; miss → `new Unknown()`. **No namespace dimension; no user-defined
  types.**
- User-defined `simpleType`/`complexType` are resolved structurally by
  walking the TinyXML DOM in
  `/Users/afalls/code/xsd-tools/src/XSDParser/Elements/Node.cpp`.

### 1.2 The resolution funnels (Node.cpp)
- `Node::_Type(pType)` (Node.cpp:271-291): single funnel for
  `type=`/`base=`/`itemType=`. (a) `_StripNamespace`s the QName
  (line 274); (b) tries builtin `TypesDB` (line 275); (c) on `Unknown`,
  searches the DOM for a `simpleType` **and** a `complexType` named
  `typeName` via `_FindXSDElm` (lines 277-284); else throws
  `MissingElement`.
- `Node::_FindXSDElm(pName, pTypeName)` (Node.cpp:212-233):
  `QualifyElementName`s
  the *tag* (line 214), searches the **root schema** via
  `_FindChildXMLElement` (line 218), then iterates the root's
  `<xs:include>` children, lazily `QuerySchema()`-parsing each and
  re-searching (lines 220-229). This include loop is a raw
  `TiXmlElement` `for`/`NextSiblingElement` walk over the DOM, **not**
  `_eachChild`/`_findChild` — F left it imperative because it iterates
  TinyXML elements, not `Node`s. **Match is local-name only**
  (`_FindChildXMLElement`, Node.cpp:128-136, also a raw `for` loop,
  `boost::iequals` on the `name` attr). No namespace key anywhere.
- `Node::_FindXSDRef` (Node.cpp:253-260) resolves `ref=` by recursively
  following `_FindXSDNode`.
- The `GetAttribute<BaseType*>` specialization (Node.cpp:86-91) routes
  `type=` through `_Type`. `Restriction::Base`/`Extension::Base` route
  `base=` the same way; `ref=` sites route through
  `FindXSDElm<T>`/`FindXSDRef<T>` (Node.hpp:87-92).

**Every cross-reference funnels through exactly `Node::_Type` and
`Node::_FindXSDElm`, plus the `_StripNamespace` / `QualifyElementName`
helpers they call.** That is the entire surgical surface.

- **New reusable accessor (landed in D):** `SimpleType::GetRestriction()`
  (`/Users/afalls/code/xsd-tools/src/XSDParser/Elements/SimpleType.cpp:83-86`,
  decl SimpleType.hpp:50; `noexcept`, returns the restriction child or
  `NULL` via `SearchXSDChildElm<Restriction>`). D added it so the
  attr-facet walk could reach a restriction node **without touching `Node`
  internals**. E's `_ResolveQName`/type-walk work may leverage it where it
  needs the restriction of a named `simpleType` (e.g. resolving the leaf NS
  of a restricted type) rather than re-deriving the child search.

### 1.3 The conflation bug (`Schema::Namespace`)
`Schema::Namespace()`
(`/Users/afalls/code/xsd-tools/src/XSDParser/Elements/Schema.cpp:74-87`)
does **not** return `targetNamespace`. It scans attributes
(Schema.cpp:78-86) for the value containing
`http://www.w3.org/2001/XMLSchema` and returns the **prefix** bound to the
XSD-language (`xs` for `xmlns:xs=…`, or `""` for default `xmlns=`). That
prefix is used two ways in Node.cpp:
- `_StripNamespace` (Node.cpp:293-306): strips that prefix so
  `xs:string` → `string` before the builtin lookup. It strips **any**
  leading match of that one prefix; a `tns:` prefix is left intact and
  then fails the builtin lookup *and* the local-name DOM match.
- `QualifyElementName` (Node.cpp:408-421): **prepends** that prefix to XSD
  *tag* names (`complexType` → `xs:complexType`) so TinyXML (which stores
  tags verbatim with prefix) can find them. Guards: skip if namespace is
  empty, equals `"xmlns"`, or already present (line 415-418).

So the load-bearing assumption is *user-defined types live in the same
prefix space as `xs:` builtins* — `targetNamespace` is conflated with the
XSD-language namespace. This is exactly why `type="tns:MyType"` fails.

Confirmed unread today (grep of `src/`):
- `targetNamespace` — never read.
- `elementFormDefault` / `attributeFormDefault` — never read.
- `Any::Namespace()` (Any.cpp:121) is unrelated (the `namespace` attr of
  `xs:any` wildcard matching).

### 1.4 Multi-document loading
- `/Users/afalls/code/xsd-tools/src/XSDParser/Parser.cpp`: `m_docLst` is a
  `vector<DocumentRecord*>` (Parser.hpp:37-49); each record is
  `{TiXmlDocument*, uri}` — **no namespace recorded**. `Parse` dedups by
  URI (Parser.cpp:48-73). `isRootDocument` = doc[0] (Parser.cpp:116-119).
- `xs:include`
  (`/Users/afalls/code/xsd-tools/src/XSDParser/Elements/Include.cpp`):
  `QuerySchema` (Include.cpp:65-71) lazily `Parse`s `schemaLocation`
  (relative-path resolution in `_schemaURI`, Include.cpp:78-102) and
  caches the `Schema*`. **Latent field:** `Include` also declares a
  by-value `Parser m_parser;` (Include.hpp:41) that is never initialized
  or read — don't replicate it in `Import` (see E2 notes). `LuaProcessor::ProcessInclude`
  (LuaProcessor.cpp:332-335) flat-merges the included schema's children
  into the **same** Lua table. `LuaProcessorBase::ProcessInclude`
  (LuaProcessorBase.cpp:219-221) just `ParseChildren`. Include assumes
  **same `targetNamespace`** (spec-correct) — no prefix translation.
- **No `Import` element class** (no `Elements/Import.*`); no
  `ProcessImport` in `ProcessorBase.hpp` (66-98). `xs:import` is entirely
  unimplemented.

### 1.5 ProcessImport ripple surface
`BaseProcessor` (ProcessorBase.hpp:63-99) declares every `ProcessXxx` as
**pure virtual** (`= 0`). Subclasses:
- `LuaProcessorBase`
  (`/Users/afalls/code/xsd-tools/src/Processors/LuaProcessorBase.hpp:34`)
  — the only direct `BaseProcessor` subclass; **re-declares an override
  for all 32 `ProcessXxx` callbacks** (LuaProcessorBase.hpp:39-71) and
  supplies `ParseChildren`-style bodies for them in the `.cpp`.
- `LuaProcessor`, `ElementExtracter`, `SimpleTypeExtracter`,
  `RestrictionVerify` all derive from `LuaProcessorBase`.
- `ArrayType` (Processors/ArrayType.hpp:32) derives from `BaseType`, **not**
  a processor — irrelevant.

A **pure-virtual** `ProcessImport` would force an override in every
processor. A **no-op base impl** in `BaseProcessor` (or a virtual default
in `LuaProcessorBase`) keeps the ripple to one line. Use the latter.

**Implementation notes (re-verified):**
- Every callback in `BaseProcessor` is **pure virtual** (`= 0`),
  ProcessorBase.hpp:66-98 — confirmed; there is no default body anywhere
  in this class.
- `LuaProcessorBase` is the **only** direct subclass and it re-declares an
  override for **all 32** callbacks (LuaProcessorBase.hpp:39-71). The four
  real processors derive from `LuaProcessorBase`, not `BaseProcessor`.
- Therefore the minimal-ripple options are equivalent in reach but differ
  in placement: (a) `virtual void ProcessImport(const Elements::Import*) {}`
  **non-pure** in `BaseProcessor` (ProcessorBase.hpp, after line 94 beside
  `ProcessInclude`) leaves `LuaProcessorBase` untouched — but the fwd-decl
  block (lines 29-61) must gain `class Import;`. (b) A pure decl in
  `BaseProcessor` + one body in `LuaProcessorBase` adds **two** edit sites.
  Prefer (a): one decl, one fwd-decl, zero `.cpp` change, and the five
  processors are untouched. **The plan's claim holds, but the gate is
  `LuaProcessorBase` re-declaring all callbacks — not `BaseProcessor`
  having defaults.**
- **Naming collision risk:** `TypesDB` already registers an XSD builtin
  keyed `"QName"` → `XSD::Types::QName` (TypesDB.cpp:63, class in
  `Types.hpp`). The new value type `XSD::QName{ns,local}` lives in a
  different namespace (`XSD::` vs `XSD::Types::`) so it compiles, but the
  bare token `QName` is ambiguous inside `using namespace XSD::Types;`
  TUs (TypesDB.cpp, Node.cpp via includes). Consider naming it `XsdQName`
  or fully-qualifying every use to avoid a surprise ADL/overload-set clash.

### 1.6 Template exposure (how names reach Lua)
- The Lua `schema` global is built in
  `/Users/afalls/code/xsd-tools/src/Processors/LuaAdapter.cpp`. Type/
  element/attribute table **keys are bare local-name strings**:
  `LuaType` keys by `rTypeName` (LuaAdapter.cpp:126-132);
  `LuaContent::Type` (now `(rTypeName, maxOccurs, minOccurs)` post the D
  minOccurs bridge, LuaAdapter.cpp:99-103) and
  `LuaProcessor::ProcessElement` pass `pNode->Name()`
  (LuaProcessor.cpp:118); attributes by `rName` (LuaAdapter.cpp:225).
  **No namespace reaches Lua today.**
- `/Users/afalls/code/xsd-tools/templates/shared/schemaEx` post-processes
  that table: `c_safeNames`/`python_safeNames` (schemaEx:54-65) and the
  topo-sort over `schema.types[schema.root]` (schemaEx:16-17) all operate
  on local-name keys. `renameNCName` (schemaEx:37,45) renames NCNames to
  language-safe identifiers.
- **The `test` template
  (`/Users/afalls/code/xsd-tools/templates/test`) dumps the entire
  `schema` table verbatim** (`printTable(schema, 0)`, template line 29).
  The `Generate.MatchesGoldenCorpus` golden test
  (`test/generateTests.cpp:43-56`) regenerates each schema via the `test`
  template and `EXPECT_EQ`s the dump byte-for-byte against
  `test/xsdparse/*.out` (the XSD corpus dir is the `XSD_CORPUS_DIR`
  compile-def, mapped from CMake `CORPUS_DIR=xsd-positive` at
  test/CMakeLists.txt:86,165). **Any new field that leaks into the Lua
  `schema` table changes every golden** — D's `facets` block already
  demonstrated this (see §7). This is the single biggest template-churn
  lever — see Risks.

### 1.7 Test harness (for guardrails)
- `/Users/afalls/code/xsd-tools/test/parserTests.cpp` — parser smoke
  tests (positive parse, throw-on-missing).
- `/Users/afalls/code/xsd-tools/test/generateTests.cpp` —
  `Generate.MatchesGoldenCorpus` vs `test/xsdparse/*.out` goldens (the
  schema-table snapshot).
- `/Users/afalls/code/xsd-tools/test/CMakeLists.txt:134` globs
  `${CORPUS_DIR}/*.xsd` (`CONFIGURE_DEPENDS`); lines 136-143 run
  `gen_roundtrip_tests.py`; lines 144-151 list the **7** generated
  roundtrip drivers (python-sax, python-json, c-xml-expat,
  c-xml-expat-dom, c-json, java-json, java-xml). CMake vars
  `CORPUS_DIR=xsd-positive` / `XSDPARSE_DIR=xsdparse` (CMakeLists.txt:86-87)
  reach the test as compile-defs `XSD_CORPUS_DIR` / `XSDPARSE_DIR`
  (CMakeLists.txt:165-166).
- No fixture currently uses `targetNamespace` or `xs:import` (grep of
  `test/` is empty for both). Negative corpus: `test/xsd-negative/`
  (5 files, testA001-005). `xs:include` fixtures live in
  `test/xsd-positive/include/` (Person/Product/RefTest1).

---

## 2. Design

### 2.1 `QName` value type
New files
`/Users/afalls/code/xsd-tools/src/XSDParser/QName.hpp` (+ `.cpp` if needed):

```cpp
namespace XSD {
  struct QName {
    std::string ns;     // namespace URI; "" == no namespace
    std::string local;  // NCName
    bool operator==(const QName&) const;
  };
  static const char* const XSD_NS = "http://www.w3.org/2001/XMLSchema";
}
```

Thread it through the **two funnels only**. Keep `TypesDB` keyed by local
name (builtins are one fixed namespace) — **no TypesDB rewrite**. Builtin
detection becomes: *if `qname.ns == XSD_NS`, look up `qname.local` in the
existing `TypesDB`* — by **URI**, not by the `xs` prefix.

### 2.2 Namespace context on `Schema`
Add to
`/Users/afalls/code/xsd-tools/src/XSDParser/Elements/Schema.{hpp,cpp}`:
- `std::string TargetNamespace() const` — read the `targetNamespace`
  attribute (default `""`).
- `std::string ResolvePrefix(const std::string& prefix) const` — map a
  prefix → URI by scanning the schema element's `xmlns:*` attributes, with
  default `xmlns` for the empty prefix. The current single-prefix scan
  (Schema.cpp:76-86) generalizes to a prefix→URI walk.
- `std::string ElementFormDefault() const` /
  `AttributeFormDefault() const` (default `"unqualified"`) — Phase E3.
- **Keep `Namespace()` but re-derive it** as "the prefix whose URI ==
  `XSD_NS`" (so `QualifyElementName`'s tag-prefixing is unchanged in
  behavior but no longer relies on the fragile substring search). This
  *fixes the conflation without touching tag-finding*.

### 2.3 New resolution path in Node.cpp
Replace `_StripNamespace(rawQName) -> std::string` with
`_ResolveQName(rawQName) -> QName`:
- split `prefix:local`; resolve `prefix` via `Schema::ResolvePrefix`
  (no-prefix → default `xmlns` URI, which may be `XSD_NS`, the
  `targetNamespace`, or `""`).
- `_Type` (Node.cpp:271): if `qname.ns == XSD_NS` → `TypesDB.FindType(local)`;
  else search user types with `qname` (local **and** owning-doc
  `targetNamespace == qname.ns`).
- `_FindXSDElm` (Node.cpp:212): match `name == qname.local` **and** the
  target document's `TargetNamespace() == qname.ns`. When `qname.ns`
  differs from the current doc's target NS, hand off to the import index
  (§3) to pick the owning document, then search there. The existing
  include-walk (Node.cpp:220-229) is a raw `TiXmlElement` `for` loop, not a
  `Node` `_eachChild` — leave its iteration form as-is; only add the
  namespace predicate and the cross-NS hand-off.
- `QualifyElementName` (Node.cpp:408) is **unchanged in contract**; it
  still consumes `Namespace()` (the XSD-language prefix) to build
  `<xs:complexType>` tags. This is the highest-risk file — see Risks.

`_ConstructNode` (Node.cpp:138-210) already calls `_StripNamespace` on the
*tag* `pElm->ValueStr()` (line 140) to dispatch element class. Keep a
thin `_stripPrefix(tag)` helper for that purely-syntactic strip so the tag
dispatch is decoupled from QName *resolution*.

### 2.4 Lua model — keep local-name keys
- Lua table keys stay **bare local names**. Add **optional sibling
  fields** `node.namespace` (URI string) and `node.qualified` (bool) only
  where an XML-emitting template needs them (Phase E3). JSON/Python-JSON
  templates ignore them.
- **Follow D's precedent for adding a model field without mass golden
  churn — there are now THREE such precedents.** D added the `facets`
  sub-table via `LuaType::Facets` (LuaAdapter.cpp:169-192), gated on
  `LuaFacets::empty()` (171-172) so a type with no facets emits **nothing**
  and its golden is unchanged. D then repeated the pattern twice more:
  `LuaAttribute::Facets` (LuaAdapter.cpp:231-255, same `empty()` gate at
  233-234) for attribute restrictions, and the minOccurs bridge (threaded
  through `ProcessElement`, LuaProcessor.cpp:114-118 — only the value reaches
  Lua, no new conditional key). All three emit only when the feature is
  present. Mirror that: emit `namespace`/`qualified` only when non-default
  (qualified form / non-empty URI), so unnamespaced schemas keep
  byte-identical goldens.
- **`shared/trnsfrm_old` is a second concrete touch point (D2 added it).**
  It normalizes the raw Lua `schema` into the per-type model the binding
  templates read, and D2 now carries facets through it: it sets
  `node.content.facets` (`/Users/afalls/code/xsd-tools/templates/shared/`
  `trnsfrm_old:63`, `outTbl.content = { type = k, facets = v.facets }`) and
  `attr.facets` (trnsfrm_old:82, `outTbl.facets = attribDef.facets`) — the
  leaf type's facet sub-table. E3 plans to add `namespace`/`qualified` onto
  this **same** normalized model, so `trnsfrm_old` must also be edited to
  carry them through. Hold the same discipline there: set the fields only
  when present / non-default (the way D carries facets, which are simply
  absent when the leaf has none), so the `test`-template dump and
  `test/xsdparse/*.out` goldens don't churn for unnamespaced schemas.
- `schemaEx` rename/topo logic untouched unless cross-namespace local-name
  collisions arise; handle that with an optional namespace-aware rename in
  `schemaEx`, not in C++.
- **Gate the new fields out of the default model** to keep the `test`
  golden dump stable until E3 (see §6).

---

## 3. `xs:import`

### 3.1 New `Import` element class
Mirror `Include`:
`/Users/afalls/code/xsd-tools/src/XSDParser/Elements/Import.{hpp,cpp}`.
- `XSD_ELEMENT_TAG("import")`; register in `Node::_ConstructNode`
  (Node.cpp:197-198 sits the `Include` branch — add an `Import` branch
  beside it).
- Attributes: `namespace` (the imported target NS URI) + **optional**
  `schemaLocation`. `ParseElement` → `rProcessor.ProcessImport(this)`.
- `QuerySchema()` reuses Include's URI-resolution; **factor the URI
  helpers** out of `Include.cpp` into a shared free-function header
  (`src/XSDParser/Elements/SchemaURI.hpp`) so both classes share one copy.
  `_schemaURI` is the instance method (Include.cpp:78-102); the statics
  `_extractURIPath`/`_isFileURI`/`_extractQuery` are at 104-130 (note the
  `_extractQuery` helper the original plan omitted).

### 3.2 `ProcessImport` — no-op base, opt-in override
- Add `virtual void ProcessImport(const Elements::Import*) {}` as a
  **non-pure** member in
  `/Users/afalls/code/xsd-tools/src/XSDParser/ProcessorBase.hpp` (forward-
  declare `class Import;` at line ~57). Non-pure ⇒ **zero ripple** to the
  five processors.
- `LuaProcessor::ProcessImport` (new) loads the imported schema and
  registers it in the namespace index but does **not** flat-merge its types
  into the current Lua table (unlike `ProcessInclude`). Imported types are
  pulled in lazily only when a cross-namespace `type=`/`ref=` resolves to
  them — this keeps the importing schema's emitted model scoped to its own
  namespace.
- `Schema::ParseChildren` (Schema.cpp:48-57) is now an `_eachChild`
  lambda whose `XSD_ISELEMENT` filter dispatches only
  `Element`/`Annotation`/`Include`; **add an `Import` arm** to that
  `if`/`||` filter (F-era form — no loop to rewrite).

### 3.3 Parser namespace index
Extend `/Users/afalls/code/xsd-tools/src/XSDParser/Parser.{hpp,cpp}`:
- Add a cached `targetNamespace` to `DocumentRecord` (Parser.hpp:37-46),
  populated at `Parse` time (read once from the root element).
- Add `const TiXmlDocument* GetDocumentForNamespace(const std::string& uri)
  const` so `_FindXSDElm` answers "which loaded doc owns URI X" without
  re-walking. Used by the cross-namespace branch in §2.3.

### 3.4 Import vs Include (contrast — keep separate classes)
| | `xs:include` | `xs:import` |
|---|---|---|
| Namespace | **same** targetNamespace | **foreign** namespace |
| Lua merge | flat into same table | none; lazy cross-NS resolve |
| Required attrs | `schemaLocation` | `namespace`; `schemaLocation` optional |
| Resolution | re-walk includes in `_FindXSDElm` | jump via Parser NS index |

---

## 4. Form-defaults + template impact

- Read `elementFormDefault`/`attributeFormDefault` on `Schema` (default
  `unqualified`). Surface per element/attribute a `qualified` bool + owning
  `namespace` URI as **optional Lua sibling fields** in
  `LuaAdapter`/`LuaProcessor` — emit them additively the way
  `LuaType::Facets` emits its `facets` block (LuaAdapter.cpp:169-192), set
  on the type table only when non-default.
- **Only XML-emitting templates** consume them (write `xmlns:`/prefixes,
  qualify element/attribute names):
  - `/Users/afalls/code/xsd-tools/templates/c-xml-expat`
  - `/Users/afalls/code/xsd-tools/templates/c-xml-expat-dom`
  - `/Users/afalls/code/xsd-tools/templates/python-sax`
  - `/Users/afalls/code/xsd-tools/templates/java-xml-stax.tmpl`
- **D2 already touched these same four XML templates** to emit per-type/
  element validation (e.g. `c_facetChecks` /`ctype:test(...)` in
  `c-xml-expat-dom/complexType`, `facets_checks` in `c-xml-expat`,
  `python-sax`, `java-xml-stax`). E3's prefix/`xmlns:` emission must
  **coexist** with that validation emission in the same files — **re-read
  each template fresh when E3 starts** (these are post-D2, not the versions
  current at the last refresh).
- **If E adds namespace-resolution helpers for templates, mirror
  `shared/facets`** (`/Users/afalls/code/xsd-tools/templates/shared/`
  `facets`, added in D2): a pure, no-emission shared helper — it returns
  abstract data (int ranges, ordered `{kind=...}` checks, sample values)
  that each target renders in its own vocabulary, and is pulled in via
  `include 'shared/facets'` + plain function calls. A namespace helper
  (e.g. prefix→URI resolution, qualified-name building) should follow the
  same shape: `include 'shared/<name>'`, pure functions, zero literal text.
- **JSON targets ignore namespaces — no change**:
  `c-json-jsonc`, `python-json.tmpl`, `java-json.org.tmpl`.
- `templates/shared/schemaEx` `c_safeNames`/`python_safeNames`/topo-sort
  unchanged; new fields are additive.

---

## 5. Phasing

### E0 — Disentangle (no behavior change; regress full suite)
- **Files:** `Schema.{hpp,cpp}` (add `ResolvePrefix`/`TargetNamespace`,
  re-derive `Namespace()` from prefix→URI map); new `QName.hpp`;
  `Node.cpp` (`_StripNamespace` → `_ResolveQName`, builtin lookup by URI,
  `_stripPrefix` helper for tag dispatch). `QualifyElementName` untouched
  in contract.
- **Guardrail:** entire existing gtest suite green — especially
  `Generate.MatchesGoldenCorpus` (byte-identical `test/xsdparse/*.out`) and
  all 7 roundtrip targets. No golden edits allowed in E0.
- **Blast radius:** `Node.cpp` resolution funnels + `Schema`. High value,
  low risk — fixes the prefix/NS conflation invisibly.
- **Implementation notes (code-grounded):**
  - `_StripNamespace` is **Node.cpp:293-306** (decl Node.hpp:67). Its only
    caller for QName *resolution* is `_Type` (Node.cpp:274). It is **also
    called by `_ConstructNode`** at **Node.cpp:140**
    (`_StripNamespace(pElm->ValueStr())`) for *tag* dispatch. These two
    uses must split: a pure-syntactic `_stripPrefix(tag)` for line 140 and
    a context-aware `_ResolveQName(qname)->QName` for line 274. Do **not**
    route `_ConstructNode` through the new resolver — at construction time
    `GetSchema()` is callable but it is wasted work for every node built.
  - Builtin lookup today: `_Type` calls
    `m_rParser.QueryTypesDb().FindType(typeName.c_str())` (Node.cpp:275)
    after the strip. New form: resolve to `QName`; if `qname.ns == XSD_NS`
    → `FindType(qname.local)`; **keep `TypesDB` keyed by local name**
    (TypesDB.cpp:30-77 are all bare locals; `FindType` is local-only,
    TypesDB.cpp:89-95) — no TypesDB change.
  - `Schema::Namespace()` is **Schema.cpp:74-87**; it returns the XSD-lang
    *prefix* via a substring scan for the URI. `QualifyElementName`
    (Node.cpp:408-421) and `HasContent`/`ContentElement`/`_FindChildXSDNode`
    all consume that prefix to build `<xs:tag>` strings. Re-deriving
    `Namespace()` from a `ResolvePrefix` map must return the **same string**
    (`xs` or `""`) or every DOM tag lookup breaks — this is the load-bearing
    invariant E0 must preserve byte-for-byte.
  - **`GetSchema()` allocates a fresh `Schema` per call** (Node.cpp:363-366,
    `new Elements::Schema(...)`); `_StripNamespace`/`QualifyElementName`
    each call it (Node.cpp:295, 411). Adding `ResolvePrefix`/
    `TargetNamespace` getters that also `GetSchema()` multiplies these
    short-lived allocations across the hot resolution path. Not a
    correctness issue, but if E0 adds getters callable from `_ResolveQName`,
    prefer reading attributes off the already-held root element rather than
    re-`GetSchema()`-ing.

### E1 — Single-`targetNamespace` correctness
- **Files:** `Node.cpp` `_FindXSDElm`/`_Type` honor
  `qname.ns == Schema::TargetNamespace()`; clear error on genuinely
  cross-NS refs (reuse `XMLException::NamespaceMismatch`,
  Exception.hpp:49).
- **Guardrail:** new positive fixtures with prefixed `targetNamespace` +
  `xmlns:tns=` + `type="tns:MyType"`; existing suite still green.
- **Blast radius:** resolution funnels only.
- **Implementation notes (code-grounded):**
  - `_FindXSDElm` is **Node.cpp:212-233**. It `QualifyElementName`s the
    *tag* (line 214), then `_FindChildXMLElement(tag,"name",pName)`
    (line 218) against `GetSchema()` (a fresh root `Schema`, line 217),
    then walks `<xs:include>` siblings (lines 220-229) re-searching each.
    The match key is **only** the `name` attribute via `boost::iequals`
    (`_FindChildXMLElement`, Node.cpp:128-136). To honor `qname.ns`, the
    predicate must additionally compare the **owning document's**
    `targetNamespace` to `qname.ns`. The owning doc for the root branch is
    `GetSchema()`; for the include branch it is `pSchema` (line 224, the
    `QuerySchema()` result). Both need a `TargetNamespace()` read.
  - **`pName` reaching `_FindXSDElm` is already a bare local name**: `_Type`
    passes `typeName.c_str()` (Node.cpp:277-278) which is the
    post-`_StripNamespace` local. So E1 must thread the *resolved* `QName`
    (ns + local) into `_FindXSDElm`, not just the local string — its
    signature `(pName, pTypeName)` (Node.hpp:60) gains a namespace arg, or
    takes a `const QName&`. This touches the `FindXSDElm<T>` /
    `_FindXSDNode` / `_FindXSDRef` call chain (Node.hpp:87-92,
    Node.cpp:235-260) — wider than "resolution funnels only" implies, since
    those templates pass `T::XSDTag()` and a name; the name now carries a
    namespace. **This is the resolution path the plan under-scopes: the
    QName must travel through four methods, not stay local to `_Type`.**
  - Same-NS default: a schema whose `targetNamespace` equals the no-prefix
    `xmlns` resolves no-prefix refs to its own NS; a prefixed `tns:` ref
    resolves via `ResolvePrefix("tns")`. Both must land on the same
    `TargetNamespace()` for the local search to match — verify with a
    fixture pair (prefixed vs default form of the same schema).

### E2 — `xs:import` (multi-namespace)
- **Files:** new `Elements/Import.{hpp,cpp}` + `Elements/SchemaURI.hpp`
  (factored from `Include.cpp`); `Node::_ConstructNode` import branch;
  `ProcessorBase.hpp` no-op `ProcessImport` + fwd-decl; `LuaProcessor`
  `ProcessImport`; `Schema::ParseChildren` filter; `Parser.{hpp,cpp}`
  (`DocumentRecord.targetNamespace`, `GetDocumentForNamespace`).
- **Guardrail:** two-namespace fixtures resolve; include fixtures
  (`test/xsd-positive/include/`) still green (import must not perturb
  include).
- **Blast radius:** new files + Parser + one Node branch; processors
  untouched thanks to the non-pure default.
- **Implementation notes (code-grounded):**
  - `Include` mirror target: **Include.hpp:37-56 / Include.cpp:36-130.**
    Note `Include` holds a **by-value `Parser m_parser;`** member
    (Include.hpp:41) that is **never used** (ctor doesn't init it, no method
    reads it) — a latent field. Do **not** copy it into `Import`; it would
    silently default-construct an empty `Parser`. Mirror only the live
    members (`mutable Schema* m_pSchema`, the URI helpers, `QuerySchema`).
  - URI helpers to factor into `SchemaURI.hpp`: `_schemaURI`
    (Include.cpp:78-102, **instance** method — needs `GetSchema()->URI()`
    for relative resolution, so factor as a free fn taking the base URI),
    `_extractURIPath` (104-112), `_isFileURI` (114-121), `_extractQuery`
    (123-130, the helper the original plan omitted — confirmed present).
  - `_ConstructNode` import branch: insert beside the `Include` arm at
    **Node.cpp:197-198** (`boost::iequals(Include::XSDTag(), …)`).
  - `Schema::ParseChildren` filter is **Schema.cpp:48-57**, an `_eachChild`
    lambda whose `if` ORs `Element || Annotation || Include`
    (Schema.cpp:51-53). Add `|| XSD_ISELEMENT(&rNode, Import)`. **F-era
    form confirmed — no loop rewrite.**
  - `ProcessInclude` flat-merge is **LuaProcessor.cpp:332-335**
    (`pNode->QuerySchema(); pSchema->ParseChildren(*this)`). `ProcessImport`
    must **not** call `ParseChildren(*this)` — that is the flat-merge that
    pulls foreign types into the current Lua table. It should only register
    the imported `Schema` in the Parser NS index for lazy cross-NS resolve.
    `LuaProcessorBase::ProcessInclude` (LuaProcessorBase.cpp:219-221) just
    `ParseChildren`; a matching no-op `ProcessImport` there is **not**
    needed if the `BaseProcessor` default is non-pure (see §1.5).
  - Parser index: `DocumentRecord` is **Parser.hpp:37-46** (`{m_pDocument,
    m_uri}`); add `std::string m_targetNamespace`, populated in `Parse`
    (Parser.cpp:48-73) by reading `targetNamespace` off
    `pDoc->RootElement()` right after a successful `LoadFile`
    (lines 60/66). `GetDocumentForNamespace(uri)` mirrors `GetDocument`
    (Parser.cpp:107-114) but matches `m_targetNamespace`. **Caveat:** the
    `HasDocument(rUri)` early-return path (Parser.cpp:51-52) builds a
    `Schema` from an already-parsed doc without re-reading NS — fine, since
    the record already cached it at first parse.

### E3 — Form-defaults + template prefix emission
- **Files:** `Schema` form-default getters; optional Lua fields in
  `LuaAdapter.cpp`/`LuaProcessor.cpp`; `shared/trnsfrm_old` (carry the new
  fields through the normalized model, as it now carries facets — §2.4);
  the four XML templates (§4). **Re-read trnsfrm_old and the four XML
  templates fresh — D2 edited all of them after this plan's last refresh.**
- **Guardrail:** roundtrip targets re-verify marshalled XML; `Generate`
  goldens updated **deliberately** here (the new fields appear in the
  `test`-template dump — regenerate `test/xsdparse/*.out` and review the
  diff). Set-only-when-non-default in both `LuaAdapter` and `trnsfrm_old`
  so unnamespaced fixtures stay byte-identical.
- **Blast radius:** XML templates + Lua model + `trnsfrm_old`; JSON
  untouched.
- **Implementation notes (code-grounded, D2 re-verified):**
  - The `empty()`-gate to mirror is **`LuaType::Facets`,
    LuaAdapter.cpp:169-192** (`if (rFacets.empty()) return;` at 171-172,
    then `lua_setfield(... FACETS_TAG)` at 189). It is attached in
    **`LuaProcessor::_parseType` at LuaProcessor.cpp:447**
    (`pLeaf->Facets(m_facets)`), inside the leaf-type branch (441-449) — D's
    attr-facet + minOccurs work shifted this from the previously-cited 420.
    The facet-recording walk `_walkFacets` is now at
    **LuaProcessor.cpp:156-174** (header at 156). **The attr-facet twin** is
    `LuaAttribute::Facets` (LuaAdapter.cpp:231-255), attached at
    `LuaProcessor::ProcessAttribute` via `_walkAttributeFacets`
    (LuaProcessor.cpp:242-243; helper 253-267, which itself calls
    `SimpleType::GetRestriction()` at 259). Use these current anchors.
  - **`trnsfrm_old` facet propagation confirmed at the cited sites, with a
    subtlety:** `trnsfrm_old:63` is `outTbl.content = { type = k,
    facets = v.facets }` and `:82` is `outTbl.facets = attribDef.facets`.
    Neither is `empty()`-gated in Lua — they rely on **`v.facets` being
    `nil`** when the C++ side emitted no `facets` block, so the field is
    simply absent from the table dump. E3's `namespace`/`qualified` must
    follow the **same nil-when-absent discipline**: emit nothing C++-side
    (gate in `LuaAdapter`), then `outTbl.namespace = v.namespace` in
    `trnsfrm_old` is safe (nil propagates, no dump churn). An explicit
    `if v.namespace then` guard is **not** required for the dump to stay
    stable, but add one if a `false`/`""` could ever be set.
  - The `MINOCCURS_TAG`/`MAXOCCURS_TAG` fields ARE emitted unconditionally
    on every `LuaType` (LuaAdapter.cpp:141-144) and DO appear in every
    golden — so the "unnamespaced schemas stay byte-identical" guarantee
    depends entirely on the new fields being **absent**, not on the type
    table being minimal. Confirm by diffing `test/xsdparse/*.out` after E3:
    only fixtures that declare a `targetNamespace` should change.
  - Golden mechanism re-confirmed: `Generate.MatchesGoldenCorpus`
    (generateTests.cpp:43-56) regenerates each `XSD_CORPUS_DIR/*.xsd` via
    the `test` template and `EXPECT_EQ`s vs `XSDPARSE_DIR/*.out`,
    byte-for-byte (line 53-54).

### E4 — Fixtures + goldens
- **Files:** new `test/xsd-positive/*.xsd` (namespaced single-NS,
  importing pair, default-`xmlns` ambiguity case); negative cases under
  `test/xsd-negative/` (undeclared prefix, import of missing NS, cross-NS
  ref without import). Regenerate goldens; `gen_roundtrip_tests.py` picks
  up new `xsd-positive/*.xsd` automatically (CMakeLists.txt:134 glob).
- **Guardrail:** full ctest green; negative fixtures throw the expected
  exception.

---

## 6. Risks

- **`QualifyElementName` is the highest-risk edit** (Node.cpp:408-421). It
  drives *all* DOM tag traversal (`_FindXSDElm`, `_FindChildXSDNode`,
  `ContentElement`, `HasContent`). E0 must change only QName *resolution*,
  never the tag-prefixing contract. Mitigation: derive the XSD-language
  prefix from the prefix→URI map (prefix whose URI == `XSD_NS`); regress
  the entire suite before any further phase.
- **Default-namespace ambiguity** (`xmlns=` with no prefix): schemas using
  default `xmlns` for the XSD language vs for the `targetNamespace` must
  both keep resolving. `_ResolveQName` must map no-prefix via the default
  `xmlns` URI, not assume `XSD_NS`. Add a fixture for each case in E4.
- **Behavior change / previously-accepted schemas:** making resolution
  NS-correct means a schema that *only worked by accident* (e.g. a `tns:`
  ref that currently fails, or a cross-NS ref that currently matches by
  bare local name) may now resolve differently or error. This is a
  semantic change — **document in CHANGELOG/README** (drop the "not
  namespace aware" line, README.md:31).
- **`test`-template golden churn:** because
  `templates/test` dumps the whole `schema` table and `generateTests.cpp`
  compares it verbatim, any field leaking into the Lua model rewrites every
  `test/xsdparse/*.out`. Keep new fields **out of the model until E3**, and
  regenerate goldens as a reviewed, isolated commit in E3/E4.
- **Include vs import confusion:** keep them as separate element classes;
  include stays a same-NS flat merge, import never flat-merges.

---

## 7. Interaction with prior work (D — facet bridge, COMPLETE)

- **D is COMPLETE** on `cmake-migration` (uncommitted, no longer in-flight).
  It added the content-facet plumbing: `LuaFacets` + `m_facets` member,
  facet `ProcessXxx` overrides recording into `m_facets`, `_walkFacets`
  (LuaProcessor.cpp:156-174), `_parseType` attaching facets via
  `LuaType::Facets` (LuaProcessor.cpp:447, was 420 before D's later edits),
  and the `facets` sub-table emit `lua_setfield … FACETS_TAG`
  (LuaAdapter.cpp:189; whole block 169-192, empty()-gate at 171-172). D then
  added **two further bridges**: (a) the **attribute-facet bridge** —
  `LuaProcessor::ProcessAttribute` (LuaProcessor.cpp:196) walks via
  `_walkAttributeFacets` (253-267), attaches with `LuaAttribute::Facets`
  (LuaAdapter.cpp:231-255), and reaches the restriction through the new
  `SimpleType::GetRestriction()` accessor (SimpleType.cpp:83-86); and (b)
  the **minOccurs bridge** — `Element::MinOccurs()`/`HasMinOccurs()`
  (Element.cpp:150-153 / 186-189) threaded through
  `LuaProcessor::ProcessElement` (114-118) and `LuaContent::Type`/`LuaType`
  (`MINOCCURS_TAG`, LuaAdapter.cpp:144). The Lua `schema` model now carries a
  `facets` field on restricted types **and** restricted attributes, plus a
  `minOccurs` field on every type.
- **E touches** `Node`/`Schema`/`Parser`/`TypesDB` (E0–E2) and only reaches
  `LuaProcessor`/`LuaAdapter` in **E3** (optional `namespace`/`qualified`
  fields) and **E2** (`LuaProcessor::ProcessImport`).
- **Overlap with D is only in `LuaProcessor.cpp`/`LuaAdapter.cpp` and the
  `test` goldens, and only at E3.** Since D already merged, E3 simply adds
  its fields **after** D's `facets` work and regenerates
  `test/xsdparse/*.out` on top of the already-D-regenerated goldens — no
  three-way merge. Reuse D's `empty()`-gated, set-only-when-non-default
  emission pattern (§2.4) so E3's golden diff is confined to the fixtures
  that actually use namespaces.
- **No conflict at all between D and E0/E1/E2** — they edit disjoint files
  (`LuaProcessor`/`LuaAdapter` vs `Node`/`Schema`/`Parser`/`TypesDB`), so
  E0–E2 carry zero Lua-model/golden change and land independently of D.
- **Risk surfaced by D — now with THREE precedents.** D proved an additive
  field need not churn unrelated goldens *because* it is present-gated. It
  did this three times: content facets (`LuaType::Facets`, empty()-gated),
  attribute facets (`LuaAttribute::Facets`, empty()-gated), and minOccurs
  (only the value reaches Lua, no conditional key). **The minOccurs field is
  the cautionary counter-example:** `MINOCCURS_TAG`/`MAXOCCURS_TAG` ARE
  emitted unconditionally on every `LuaType` (LuaAdapter.cpp:142,144), so
  the goldens were just regenerated to include them on every type — i.e. the
  "byte-identical for unnamespaced schemas" guarantee depends on E3's new
  fields being **absent**, not on the type table being minimal. E3 must hold
  the discipline: if `namespace`/`qualified` were emitted unconditionally
  (even as `""`/`false`), every `test/xsdparse/*.out` would rewrite, swamping
  the review on top of the already-regenerated attr-facet + minOccurs +
  `pairs()`-reorder diff. Gate strictly on non-default. D does **not**
  otherwise complicate E's namespace work: facets sit on the leaf type/
  attribute table, a different key from where E's `namespace`/`qualified`
  siblings go.
- **D2 extends this discipline to two further model-mutation sites.** D2
  carried facets not only into the raw Lua model (D1's `LuaType::Facets`)
  but also through `shared/trnsfrm_old`'s normalization (`node.content.
  facets` at trnsfrm_old:63, `attr.facets` at trnsfrm_old:82-83; §2.4),
  and D's later attr-facet bridge added `attr.<type>.facets` (the same
  `outTbl.facets = attribDef.facets` path). E3 adds `namespace`/`qualified`
  to that **same** normalized model, so it must apply the
  set-only-when-present gate in *both* places — the C++ emit (`LuaAdapter`)
  and the Lua normalizer (`trnsfrm_old`) — or the `test`-template dump
  churns even if the C++ side is gated. D2's facets are absent (not
  `nil`-set) on leaves with no facets, exactly the pattern E3's namespace
  fields should follow.
- **Nondeterministic object-keyed iteration pitfall (surfaced by D's
  `c-xml-expat` determinism fix).** D fixed `c-xml-expat`/`-dom` to sort
  `uniqueCTypes` by name because Lua tables keyed by **object** iterate in
  nondeterministic order, which made emitted output (and goldens) unstable.
  If E3 adds namespace data keyed by objects in any XML emitter (e.g. a
  prefix→type or NS→element map), it must impose a deterministic order the
  same way — sort by a stable string key before emitting — or the goldens
  will flake across runs. A namespace-resolution **shared helper** (mirroring
  `shared/facets`, §4) should return ordered data rather than rely on table
  iteration order.

---

## 8. Sub-agent decomposition

The phases are **mostly serial** (E1 needs E0's `QName`/`Schema` API; E2
needs E1's resolver; E3 needs E2's import). Parallelism is available
*within* phases and across the fixture/golden work. Strategy: the parent
freezes a small set of **contract headers first** (sync), then fans out
sub-agents that build against those frozen interfaces, then the parent
runs integration (full ctest) at each phase boundary.

### Frozen contracts (parent writes these stubs first, before any fanout)
- **C1 `src/XSDParser/QName.hpp`** — `struct QName{ns,local}`, `XSD_NS`
  constant, `operator==`. Header-only or trivial `.cpp`.
- **C2 `Schema.hpp` additions** — signatures only:
  `TargetNamespace()`, `ResolvePrefix(prefix)`, `ElementFormDefault()`,
  `AttributeFormDefault()` (bodies stubbed to current behavior).
- **C3 `Parser.hpp` additions** — `DocumentRecord.targetNamespace` field +
  `GetDocumentForNamespace(uri)` signature (stub returns nullptr).
- **C4 `ProcessorBase.hpp`** — `class Import;` fwd-decl + non-pure
  `virtual void ProcessImport(const Elements::Import*) {}`.
- **C5 `src/XSDParser/Elements/SchemaURI.hpp`** — free-function signatures
  for `schemaURI/isFileURI/extractURIPath` (the helpers factored out of
  `Include.cpp`).

Freezing C1–C5 first means every sub-agent compiles against stable
symbols and merges cleanly.

### Wave 0 — E0 (serial-ish; 2 agents)
- **A0a (Schema NS context):** implement C2 bodies in `Schema.cpp` —
  `ResolvePrefix`/`TargetNamespace`, re-derive `Namespace()` from the
  prefix→URI map. Owns `Schema.{hpp,cpp}`. Must NOT touch `Node.cpp`.
  Report: `ctest -R Generate` green + the 7 roundtrip targets green.
- **A0b (Node resolver):** implement `_ResolveQName` (replacing
  `_StripNamespace`), `_stripPrefix` tag helper, URI-based builtin lookup
  in `_Type`. Owns `Node.{cpp,hpp}`. Depends on C1+C2 signatures only.
  Must NOT change `QualifyElementName`'s contract.
  Report: full ctest green, zero golden diffs.
- **Parent merge/gate:** full ctest; assert `test/xsdparse/*.out`
  unchanged (E0 is behavior-preserving).

### Wave 1 — E1 (1 agent + 1 fixture agent, parallel)
- **A1 (single-NS resolver):** `_FindXSDElm`/`_Type` honor
  `qname.ns == TargetNamespace()`; throw `NamespaceMismatch` on stray
  cross-NS refs. Owns `Node.cpp`. Depends on Wave 0 merged.
- **A1f (fixtures, parallel):** author positive `targetNamespace`+`tns:`
  fixtures under `test/xsd-positive/` and expected goldens. Owns new test
  files only; no source. Can start as soon as the resolver contract is
  agreed (runs concurrently with A1, integrated at the gate).
- **Parent gate:** full ctest.

### Wave 2 — E2 (3 agents, parallel after C4/C5 frozen)
- **A2a (Import class):** `Elements/Import.{hpp,cpp}` + `SchemaURI.hpp`
  extraction from `Include.cpp` + `_ConstructNode` import branch +
  `Schema::ParseChildren` filter. Owns those files.
- **A2b (Parser index):** implement C3 bodies — `DocumentRecord` NS field,
  `GetDocumentForNamespace`, populate target-NS at `Parse`. Owns
  `Parser.{hpp,cpp}`.
- **A2c (cross-NS resolve + LuaProcessor::ProcessImport):** wire
  `_FindXSDElm` cross-NS branch to the Parser index; add
  `LuaProcessor::ProcessImport`. Depends on A2a+A2b symbols (frozen via
  C3/C5 stubs), so all three compile in parallel; integrate at gate.
- **Parent gate:** two-NS fixtures resolve; include fixtures still green.

### Wave 3 — E3 (1–2 agents; D already landed)
- **A3 (form-defaults + Lua fields):** Schema form-default getters;
  optional `namespace`/`qualified` Lua fields. Owns `LuaAdapter.cpp`/
  `LuaProcessor.cpp` additions. D's facet field is **already merged**, so
  A3 builds on top of it directly; mirror D's `empty()`-gated emit
  (LuaAdapter.cpp:169-192, or its attr twin `LuaAttribute::Facets` at
  231-255) to keep unrelated `test/xsdparse/*.out` byte-identical.
- **A3t–A3j (XML templates, parallel):** one agent per XML template
  (`c-xml-expat`, `c-xml-expat-dom`, `python-sax`, `java-xml-stax.tmpl`).
  Each owns exactly one template dir, consumes the new optional fields,
  reports its own roundtrip target green. JSON templates untouched.
- **Parent gate:** regenerate `test/xsdparse/*.out` as one reviewed commit;
  full ctest.

### Wave 4 — E4 (parallel fixture agents)
- One agent per fixture family: (a) namespaced single-NS, (b) importing
  pair, (c) default-`xmlns` ambiguity, (d) negative cases. Each owns its
  own `.xsd` + golden files; no source. `gen_roundtrip_tests.py` auto-picks
  positives via the CMake glob. Parent runs full ctest to close.

### Parallelization economics (per the global rule)
- Worth fanning out **within** Wave 2 (3 disjoint files, ~real work) and
  Wave 3 templates (4 disjoint dirs) — wall-clock drops to ~25%, the
  cold-context cost is justified.
- **Not** worth splitting E0 beyond the 2 agents, or splitting single-file
  phases. Contract design (C1–C5) is ~30 min up front and pays for itself
  across Waves 2–3.
- Hard serialization points (cannot parallelize across): E0→E1→E2 resolver
  evolution (same `Node.cpp` funnels). D has landed, so the former
  E3-after-D golden serialization is no longer a live constraint — E3 just
  builds on D's merged model.
