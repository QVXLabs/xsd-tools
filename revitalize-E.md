# Workstream E — XML namespace awareness + `xs:import`

PLANNING ONLY — no source/template edits. Paths absolute. Branch
`revitalize`. Re-verified against current `src/` (not `cmake-migration`).

**State of prior work (confirmed in code, not assumed):**
- **F (functional-recursion) is LANDED** — `Node::_eachChild`/`_findChild`
  exist (`Node.hpp:121-127`, `Node.cpp:336-347` over a static `_findSibling`
  recursion). `Schema::ParseChildren` is an `_eachChild` lambda
  (`Schema.cpp:50-56`). The `_FindXSDElm` include-walk
  (`Node.cpp:220-229`) is a **raw `TiXmlElement` `for`/`NextSiblingElement`**
  loop — F left it imperative because it iterates DOM elements, not `Node`s.
- **D (facets + minOccurs) is LANDED** (committed `cbdb4b5`..`9f70c62`,
  goldens regenerated `15dc4b1`). `LuaType::Facets`/`LuaAttribute::Facets`
  exist (`LuaAdapter.cpp:169-192` / `231-255`, both `empty()`-gated);
  `MINOCCURS_TAG`/`MAXOCCURS_TAG` emit **unconditionally** on every type
  (`LuaAdapter.cpp:142-144`); `trnsfrm_old` carries `content.facets`
  (line 63) and `attr.facets` (line 83).
- **An in-flight parser-simplify pass is UNCOMMITTED** (`FacetNode.hpp`,
  modified facet `*.cpp/hpp`, `revitalize-parser-simplify.md`). Its doc
  **explicitly defers the `Namespace()` cache to E** ("Defer — overlaps
  Workstream E … do it there"). **That cache is an E0 deliverable, not a
  separate task** (see E0). E must rebase onto whatever this pass lands;
  the resolution surface (`Node`/`Schema`/`Parser`/`TypesDB`) is untouched
  by it.
- **7 round-trip targets** (`gen_roundtrip_tests.py:17-32`): PythonSax,
  PythonJson, CXmlExpat, CXmlExpatDom, CJson, JavaJson, JavaXml. **4 are
  XML** (PythonSax, CXmlExpat, CXmlExpatDom, JavaXml).

---

## 1. Surgical surface (traced)

Every cross-reference funnels through **exactly two methods** plus the two
prefix helpers they call:
- **`Node::_Type(pType)`** (`Node.cpp:271-291`): the single funnel for
  `type=`/`base=`/`itemType=`. (a) `_StripNamespace`s the QName (line 274);
  (b) `m_rParser.QueryTypesDb().FindType(local)` (line 275); (c) on
  `Unknown`, `_FindXSDElm` for a `SimpleType` **and** a `ComplexType`
  (277-278); else throws `MissingElement`.
- **`Node::_FindXSDElm(pName, pTypeName)`** (`Node.cpp:212-233`):
  `QualifyElementName`s the *tag* (214), searches the root `Schema` via
  `_FindChildXMLElement` (218, match is **local `name` attr only**,
  `boost::iequals`, `Node.cpp:128-136`), then walks `<xs:include>` siblings
  re-searching each (220-229). **No namespace key anywhere.**
- **`_StripNamespace`** (`Node.cpp:293-306`): strips the one XSD-language
  prefix (from `Schema::Namespace()`) so `xs:string`→`string`. A `tns:`
  prefix is left intact → fails both the builtin lookup and the local DOM
  match. **Also called by `_ConstructNode` at `Node.cpp:140`** for purely
  syntactic *tag* dispatch (`_StripNamespace(pElm->ValueStr())`).
- **`QualifyElementName`** (`Node.cpp:408-421`): **prepends** that same
  prefix to XSD *tag* names so TinyXML can find `<xs:complexType>`. Guards:
  empty / `=="xmlns"` / already-present.

**The conflation bug:** `Schema::Namespace()` (`Schema.cpp:74-87`) does
**not** return `targetNamespace` — it returns the **prefix** bound to the
XSD-language URI (`http://www.w3.org/2001/XMLSchema`) via a substring
scan. Load-bearing assumption: *user-defined types live in the same prefix
space as `xs:` builtins*. This is exactly why `type="tns:MyType"` fails.
Confirmed unread anywhere in `src/`: `targetNamespace`,
`elementFormDefault`, `attributeFormDefault`.

The `QName` value type clashes by token with the **XSD builtin**
`TypesDB` registers as `"QName"` → `XSD::Types::QName` (`TypesDB.cpp:63`).
Name the new value type **`XsdQName`** to avoid an ADL/overload clash in
`using namespace XSD::Types;` TUs.

---

## E0 — Disentangle (no behavior change; regress full suite)

Add `src/XSDParser/XsdQName.hpp`: `struct XsdQName { std::string ns,
local; bool operator==; }` + `constexpr char XSD_NS[] =
"http://www.w3.org/2001/XMLSchema"`.

`Schema.{hpp,cpp}` additions (signatures frozen first):
- `std::string TargetNamespace() const` — read `targetNamespace` attr
  (default `""`).
- `std::string ResolvePrefix(const std::string& prefix) const` —
  prefix→URI over the root element's `xmlns:*` attrs (empty prefix ⇒
  default `xmlns`). Generalizes the single-prefix scan at `Schema.cpp:78-86`.
- **Re-derive `Namespace()` as "the prefix whose URI == `XSD_NS`"** using
  that same map. **Invariant: it must return the byte-identical string
  (`"xs"` or `""`) it returns today** — `QualifyElementName`,
  `ContentElement`, `HasContent`, `_FindChildXSDNode` all build `<xs:tag>`
  strings from it; any drift breaks every DOM tag lookup. This *fixes the
  conflation invisibly* without touching tag-finding.

`Node.cpp` changes:
- Split `_StripNamespace` into two: a pure-syntactic **`_stripPrefix(tag)`**
  for the `_ConstructNode` tag dispatch (line 140 — do NOT route this
  through resolution; it runs for every node built), and a context-aware
  **`_ResolveQName(raw) → XsdQName`** replacing the resolution use at line
  274. `_ResolveQName`: split `prefix:local`; resolve `prefix` via
  `Schema::ResolvePrefix` (no-prefix → default `xmlns` URI, which may be
  `XSD_NS`, the target NS, or `""`).
- `_Type`: **detect builtins by URI, not the `xs` prefix** — if
  `qname.ns == XSD_NS` → `FindType(qname.local)` (TypesDB stays keyed by
  bare local name — **no TypesDB rewrite**); else fall through to the user
  search. `QualifyElementName` **contract unchanged**.

**`Namespace()` cache (the deferred parser-simplify item — done HERE):**
`GetSchema()` allocates a fresh `Schema` per call (`Node.cpp:363-366`);
`_StripNamespace`, `QualifyElementName`, and the new `ResolvePrefix`/
`TargetNamespace` getters each call it on the hot resolution path. Build
the prefix→URI map once per document (key on the `TiXmlDocument*`, or read
attributes off the already-held root element rather than re-`GetSchema()`).
This is the cache `revitalize-parser-simplify.md` deferred — it lands as
part of E0, not separately.

**Guardrail:** full `xsdb-test` green; `Generate.MatchesGoldenCorpus`
byte-identical (`test/xsdparse/*.out` **unchanged** — no edits allowed in
E0); all 7 round-trips green.

## E1 — Single-`targetNamespace` correctness

Thread the resolved `XsdQName` (ns + local), not a bare local string,
through `_FindXSDElm` and its callers. **This is wider than "the two
funnels":** `_Type` currently passes `typeName.c_str()` (the stripped
local) into `_FindXSDElm` (`Node.cpp:277-278`); the namespace must travel
through `_FindXSDElm` → `_FindXSDNode` → `_FindXSDRef` and the
`FindXSDElm<T>`/`FindXSDRef<T>` templates (`Node.hpp:87-92`), which today
pass only `T::XSDTag()` + a name. Either widen those signatures to take a
`const XsdQName&` or add a namespace parameter.

In `_FindXSDElm`, add a predicate to the local match: the **owning
document's** `TargetNamespace()` must equal `qname.ns` (root branch:
`GetSchema()`; include branch: the `QuerySchema()` result `pSchema`, line
224). Same-NS default form (`xmlns=` == target) and prefixed `tns:` form of
the same schema must both resolve to the same `TargetNamespace()` — verify
with a fixture pair. Genuinely cross-NS refs throw a clear error (add an
`XMLException` code if one isn't suitable).

**Guardrail:** new prefixed-`targetNamespace` positives resolve; existing
suite green; goldens unchanged (no model field yet).

## E2 — `xs:import` (multi-namespace)

- **`src/XSDParser/Elements/Import.{hpp,cpp}`**, mirroring `Include`
  (`Include.hpp:37-56` / `Include.cpp:36-130`). **Do NOT copy the dead
  `Parser m_parser;` member** (`Include.hpp:41` — declared, never
  inited/read; the parser-simplify pass H4 flags it for deletion). Mirror
  only the live members (`mutable Schema* m_pSchema`, URI helpers,
  `QuerySchema`). Attrs: `namespace` (imported target URI) + **optional**
  `schemaLocation`.
- **Factor URI helpers into `src/XSDParser/Elements/SchemaURI.hpp`** so
  `Include` and `Import` share one copy: `_schemaURI` (instance,
  `Include.cpp:78-102`, needs the base URI → factor as a free fn taking it),
  `_extractURIPath` (104-112), `_isFileURI` (114-121), `_extractQuery`
  (123-130 — present, the original plan omitted it).
- `_ConstructNode` import branch beside the `Include` arm (`Node.cpp:197`).
- `Schema::ParseChildren` filter (`Schema.cpp:50-56`): add
  `|| XSD_ISELEMENT(&rNode, Import)` to the `_eachChild` `if` (F-era form —
  no loop to rewrite).
- **`ProcessImport` — non-pure no-op base.** Every `ProcessXxx` in
  `BaseProcessor` is **pure virtual** (`ProcessorBase.hpp:66-98`,
  `ProcessInclude` at 94 is `= 0`). A pure `ProcessImport` would force an
  override in all 5 processors. Add instead a **non-pure**
  `virtual void ProcessImport(const Elements::Import*) {}` in `BaseProcessor`
  (after line 94) + a `class Import;` fwd-decl (lines 29-61). **Zero ripple,
  one `.cpp` untouched.**
- `LuaProcessor::ProcessImport` (new): load the imported schema and register
  it in the Parser NS index; **do NOT** `ParseChildren(*this)` (that is
  Include's flat-merge, `LuaProcessor.cpp:332-335`). Imported types are
  pulled in lazily only when a cross-NS `type=`/`ref=` resolves to them.
- **Parser NS index** (`Parser.{hpp,cpp}`): add `std::string
  m_targetNamespace` to `DocumentRecord` (`Parser.hpp:37-46`), populated in
  `Parse` from `pDoc->RootElement()` after `LoadFile` (`Parser.cpp:60/66`).
  Add `const TiXmlDocument* GetDocumentForNamespace(const std::string&)
  const` (mirror `GetDocument`, `Parser.cpp:107-114`, match on
  `m_targetNamespace`). `_FindXSDElm`'s cross-NS branch hands off to it to
  pick the owning doc, then searches there. **Caveat:** the `HasDocument`
  early-return (`Parser.cpp:51-52`) reuses the cached record — fine, NS was
  read at first parse.

| | `xs:include` | `xs:import` |
|---|---|---|
| Namespace | **same** target | **foreign** |
| Lua merge | flat into same table | none; lazy cross-NS |
| Required | `schemaLocation` | `namespace`; loc optional |
| Resolution | re-walk includes | jump via Parser NS index |

**Guardrail:** two-NS fixtures resolve; `test/xsd-positive/include/`
(Person/Product/RefTest1) still green — import must not perturb include.

## E3 — Form-defaults + template prefix emission

- Read `elementFormDefault`/`attributeFormDefault` on `Schema` (default
  `unqualified`). Expose per element/attribute **optional** Lua fields
  `namespace` (URI) + `qualified` (bool) in `LuaAdapter`/`LuaProcessor`,
  **emitted only when non-default**, mirroring `LuaType::Facets`'
  `empty()`-gate (`LuaAdapter.cpp:171-172,189`). Keep Lua table **keys =
  bare local names** to contain template churn.
- **`shared/trnsfrm_old` must carry the new fields too.** D2 carries facets
  through it (`content.facets` line 63, `attr.facets` line 83) — they stay
  out of the golden dump only because they are **nil** when absent. Add
  `outTbl.namespace = v.namespace` etc. with the **same nil-when-absent
  discipline**; a `false`/`""` would churn every golden, so gate strictly
  on non-default.
- **XML templates only** consume the fields (write `xmlns:`/prefixes,
  qualify names): `templates/c-xml-expat` (file),
  `templates/c-xml-expat-dom` (dir), `templates/python-sax` (file),
  `templates/java-xml-stax.tmpl` (file). **Re-read each fresh — D2 already
  edited all four** to emit facet validation; prefix emission must coexist.
  **JSON/Python-JSON ignore NS — no change:** `c-json-jsonc`,
  `python-json.tmpl`, `java-json.org.tmpl`.
- If E adds a template namespace helper, **mirror `shared/facets`**: pure
  functions, zero literal text, `include 'shared/<name>'`, and **return
  ordered data** (D's `c-xml-expat` determinism fix sorted object-keyed
  tables because Lua iterates them nondeterministically — any NS→element /
  prefix→type map must sort by a stable string key before emitting).

**Guardrail:** `test/xsdparse/*.out` regenerated **deliberately** as one
reviewed commit; diff must touch **only** fixtures that declare a
`targetNamespace`. XML round-trips re-verify prefixed/`xmlns` output.

## E4 — Fixtures + goldens

New `test/xsd-positive/*.xsd` (the glob at the round-trip CMake step
auto-expands each across **all 7** targets):
1. single `targetNamespace` + `xmlns:tns=` + a **prefixed self-ref**
   (`type="tns:MyType"`);
2. `elementFormDefault="qualified"` **and** a twin `="unqualified"`;
3. **default-ns-as-target** (`xmlns=` == `targetNamespace`, no prefix);
4. **two-file `xs:import`** — a primary XSD + an imported schema in a
   different namespace, with a cross-NS `type=`/`ref=` (put the imported
   file beside the include corpus, e.g. `test/xsd-positive/import/`).

New `test/xsd-negative/*.xsd`: (a) unresolved cross-NS ref (no matching
import), (b) missing import / undeclared prefix.

**Auto-expansion + gating:** `gen_roundtrip_tests.py` globs
`xsd-positive/*.xsd` and emits one `TEST` per target. XML targets must emit
prefixes; JSON/Python-JSON ignore NS. Where a target genuinely can't
represent a fixture (e.g. JSON has no attr/elem distinction colliding on a
qualified name), add the basename to **`KNOWN_FAILING[suite]`** (`gen_…:42`,
a dict suite→set; the generator emits it `DISABLED_`-prefixed so it's
visible but non-failing). Regenerate `test/xsdparse/*.out` for the new
positives. The two-file import fixture: the imported schema is **not** a
standalone corpus root — keep it out of the top-level `xsd-positive/` glob
(subdir) so the harness doesn't try to round-trip it alone.

---

## Risks

- **`QualifyElementName` is the highest-risk edit** (`Node.cpp:408-421`) — it
  drives *all* DOM tag traversal. E0 changes only QName *resolution*, never
  the tag-prefixing contract; `Namespace()` must re-derive to the
  byte-identical prefix. Regress the full suite before any later phase.
- **Default-namespace ambiguity** (`xmlns=` for the XSD language vs for the
  target NS): `_ResolveQName` must map no-prefix via the default `xmlns`
  URI, never assume `XSD_NS`. Fixture for each case (E4 #3).
- **Behavior change:** NS-correct resolution means schemas that "worked by
  accident" (a `tns:` ref that currently fails, or a cross-NS bare-local
  match) resolve differently or now error. **CHANGELOG `Added` + drop
  README:36 "Currently the tools are not namespace aware."**
- **`test`-template golden churn:** the `test` template dumps the whole
  `schema` table and `Generate.MatchesGoldenCorpus` compares it verbatim, so
  any leaked field rewrites every golden. Keep new fields **out of the model
  until E3**; gate strictly on non-default in *both* `LuaAdapter` and
  `trnsfrm_old`.
- **Rebase risk:** the uncommitted parser-simplify pass touches facet
  classes and may land `FacetNode`/`_ConstructNode`-table/`m_parser`
  removal. E2's `_ConstructNode` import branch and `SchemaURI.hpp` extraction
  must rebase onto whichever form lands; the `Namespace()` cache moves into
  E0 regardless.

## Verification

Run the **gtest binary directly** (`build/test/xsdb-test --gtest_filter=…`),
never ctest:
- **E0:** full suite green; `Generate.MatchesGoldenCorpus` byte-identical;
  7 round-trips green.
- **E1:** `--gtest_filter='Generate.*:*Roundtrip*'` plus new prefixed-NS
  positives resolve; goldens unchanged.
- **E2:** two-NS fixtures resolve; include fixtures green; negative cases
  throw.
- **E3:** XML round-trips re-verify prefixes/`xmlns`; goldens regenerated
  and diff scoped to namespaced fixtures only.
- **E4:** full corpus round-trips green across all 7 targets;
  `KNOWN_FAILING` only for genuinely unrepresentable combos.
