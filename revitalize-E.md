# Workstream E — XML namespace awareness + `xs:import` (PARALLELIZED)

PLANNING ONLY — no source/template edits. Paths absolute. Symbols cited by
role/name; the C++ member style is settling to the **trailing-underscore**
convention (`StripNamespace_`, `Type_`, `FindXSDElm_`, `FindXSDNode_`,
`FindXSDRef_`, `FindChildXMLElement_`, `ConstructNode_`, `pSchema_`,
`docLst_`, `typesDb_`) — do NOT anchor on line numbers or the old `m_` prefix.

## Dependency graph

```
            ┌────────────────── Phase 0 (ONE agent, sequential) ──────────────────┐
            │  XsdQName + QName resolution + URI-builtin detect + Namespace() cache │
            │  freezes the API CONTRACT below                                       │
            └───────────────────────────────┬──────────────────────────────────────┘
                                             │ (contract frozen)
        ┌──────────────┬───────────────┬─────┴──────┬───────────────┬──────────────┐
        ▼              ▼               ▼            ▼               ▼              ▼
   E2 import      E3a c-xml      E3b expat-dom  E3c python-sax  E3d java-stax   E4 fixtures
   (Node/Parser/  -expat                                                       +goldens
    Import/proc)  ───────────────  all 4 E3 lanes independent files ──────────  (own files)
        └──────────────┴───────────────┴────────────┴───────────────┴──────────────┘
                                             │ (all lanes complete)
                                             ▼
                              Integrate (ONE agent): regen goldens,
                              full suite, reconcile Lua-model fields
```

E1 (single-targetNamespace correctness) is **folded into Phase 0** — it is
the same `XsdQName`-threading edit and cannot be split from the contract.

---

## Phase 0 — Foundation (ONE agent, must land first, no behavior change)

Owns the **resolution core**: `src/XSDParser/Elements/Node.{hpp,cpp}`,
`src/XSDParser/Elements/Schema.{hpp,cpp}`, new
`src/XSDParser/XsdQName.hpp`. Nothing else may touch these in parallel.

**Why sequential:** every cross-NS lane resolves through `Type_`/`FindXSDElm_`.
Phase 0 freezes their signatures; lanes code against the frozen contract.

### Work
1. **`XsdQName.hpp`** — `struct XsdQName { std::string ns, local; bool
   operator==(const XsdQName&) const; }` + `constexpr char XSD_NS[] =
   "http://www.w3.org/2001/XMLSchema";`. Name is **`XsdQName`, not `QName`** —
   `TypesDB` registers the builtin `"QName"` → `XSD::Types::QName`
   (`TypesDB.cpp`); a bare `QName` clashes under `using namespace XSD::Types;`.
2. **`Schema` additions** (signatures frozen first):
   - `std::string TargetNamespace() const` — read `targetNamespace` attr,
     default `""`.
   - `std::string ResolvePrefix(const std::string& prefix) const` —
     prefix→URI over the root's `xmlns:*` attrs; empty prefix ⇒ default
     `xmlns`. Generalizes the single-prefix substring scan in `Namespace()`.
   - **Re-derive `Namespace()`** as "the prefix whose URI == `XSD_NS`" via
     that map. **INVARIANT: byte-identical output** (`"xs"` or `""`) to today
     — `QualifyElementName`, `ContentElement`, `HasContent`,
     `FindChildXSDNode_` all build `<xs:tag>` strings from it. Any drift
     breaks every DOM tag lookup. This fixes the conflation invisibly.
3. **`Node` QName resolution** — split today's `StripNamespace_` (which strips
   the one XSD-lang prefix) into:
   - **`stripPrefix_(tag)`** — pure syntactic, used by `ConstructNode_`'s tag
     dispatch (do NOT route this through resolution; it runs per node built).
   - **`ResolveQName_(raw) → XsdQName`** — split `prefix:local`; resolve
     `prefix` via `Schema::ResolvePrefix` (no-prefix ⇒ default `xmlns` URI,
     which may be `XSD_NS`, the target NS, or `""`). Replaces the resolution
     use inside `Type_`.
   - `Type_`: **detect builtins by URI** — `qname.ns == XSD_NS` ⇒
     `QueryTypesDb().FindType(qname.local)` (TypesDB stays keyed by bare local
     — **no TypesDB rewrite**); else fall through to user search.
4. **Thread `XsdQName` (E1)** through the finder chain:
   `Type_` → `FindXSDElm_` → `FindXSDNode_` → `FindXSDRef_` and the
   `FindXSDElm<T>`/`FindXSDRef<T>` templates (`Node.hpp`). Today they pass a
   bare local `const char*`; widen to carry `const XsdQName&` (ns + local).
   In `FindXSDElm_`, add to the local-name match the predicate **owning
   document's `TargetNamespace()` == `qname.ns`** (root branch: `GetSchema()`;
   include branch: the `QuerySchema()` `pSchema_`). Same-NS default form
   (`xmlns=` == target) and prefixed `tns:` form must both resolve to the same
   `TargetNamespace()`. Genuine cross-NS ref with no resolver throws a clear
   `XMLException` (add a code if none fits) — the import handoff in E2 hooks
   exactly here.
5. **`QualifyElementName` contract UNCHANGED.** It still prepends
   `Namespace()` to *tag* names. E0 touches QName *resolution* only.
6. **`Namespace()` cache** (the deferred parser-simplify item, done HERE):
   `GetSchema()` allocates a fresh `Schema` per call and the hot path
   (`QualifyElementName`, `ResolvePrefix`, `TargetNamespace`) re-reads it.
   Build the prefix→URI map once per document (key on `TiXmlDocument*`, or
   read attrs off the already-held root element). Confine to Phase 0 files.

### FROZEN CONTRACT (lanes code against these)
- `struct XsdQName { std::string ns, local; }`, `XSD_NS`.
- `Schema::TargetNamespace() -> std::string`,
  `Schema::ResolvePrefix(prefix) -> std::string`, `Namespace()` unchanged.
- `Node::ResolveQName_(raw) -> XsdQName`, finder chain takes `const XsdQName&`.
- **New Lua-model fields (defined here, EMITTED in E3):** per element/attribute
  optional `namespace` (URI string) and `qualified` (bool), **nil when default
  / absent** — same `empty()`-gate discipline as `LuaType::Facets`. Table keys
  stay **bare local names**. Phase 0 does NOT add these to the model (would
  churn goldens); it only fixes the field names/semantics in the contract.

### Phase 0 verification
Full `build/test/xsdb-test` green; `Generate.MatchesGoldenCorpus`
**byte-identical** (`test/xsdparse/*.out` unedited); all 7 round-trips green.

---

## Parallel lanes (fan out AFTER Phase 0 freezes)

### Lane E2 — `xs:import` (multi-namespace)
**OWNS:** new `src/XSDParser/Elements/Import.{hpp,cpp}`, new
`src/XSDParser/Elements/SchemaURI.hpp`, `Parser.{hpp,cpp}`,
`src/Processors/LuaProcessor.cpp` (new `ProcessImport`), `ProcessorBase.hpp`
(non-pure `ProcessImport` + fwd-decl), the `ConstructNode_` import-branch and
`Schema::ParseChildren` filter line.
**MUST NOT TOUCH:** `Type_`/`FindXSDElm_` bodies, `Schema::Namespace/
ResolvePrefix/TargetNamespace` (Phase 0 froze them — call, don't edit). The
`ConstructNode_` factory table and `Schema::ParseChildren` `_eachChild` filter
are **single-line additions** beside the `Include` arm — coordinate (shares
`Node.cpp`/`Schema.cpp` with Phase 0, so E2 starts only after Phase 0 lands).
**DEPENDS ON:** frozen finder chain + `TargetNamespace()`.

- `Import.{hpp,cpp}` mirror `Include` but **drop the dead `Parser` member** if
  present; keep `mutable Schema* pSchema_`, URI helpers, `QuerySchema`. Attrs:
  `namespace` (required, imported target URI) + **optional** `schemaLocation`.
- Factor `Include`'s URI helpers (`schemaURI_`, `extractURIPath_`,
  `isFileURI_`, `extractQuery_`) into `SchemaURI.hpp` so both share one copy.
- **`ProcessImport` — NON-pure no-op base.** Every `ProcessXxx` in
  `BaseProcessor` is pure virtual (`ProcessInclude` is `= 0`). A pure
  `ProcessImport` forces an override in all 5 processors. Add a **non-pure**
  `virtual void ProcessImport(const Elements::Import*) {}` + `class Import;`
  fwd-decl. Zero ripple.
- `LuaProcessor::ProcessImport`: register the imported schema in the Parser NS
  index; **do NOT `ParseChildren`** (that is Include's flat merge). Imported
  types are pulled lazily when a cross-NS ref resolves.
- **Parser NS index:** add `targetNamespace_` to `DocumentRecord`, populated in
  `Parse` from `pDoc->RootElement()` after load. Add
  `const TiXmlDocument* GetDocumentForNamespace(const std::string&) const`
  (mirror `GetDocument`/`findByUri_`, match on `targetNamespace_`).
  `FindXSDElm_`'s cross-NS branch (the Phase-0 throw point) hands off here to
  pick the owning doc, then searches it.

| | `xs:include` | `xs:import` |
|---|---|---|
| Namespace | same target | foreign |
| Lua merge | flat into same table | none; lazy cross-NS |
| Required | `schemaLocation` | `namespace`; loc optional |
| Resolution | re-walk includes | jump via Parser NS index |

**Verify:** two-NS fixtures resolve; `test/xsd-positive/include/`
(Person/Product/RefTest1) still green; negatives throw.

### Lanes E3a–E3d — template prefix emission (one agent EACH, fully independent)
Each lane OWNS exactly one XML target's files and touches nothing else. All
four files are disjoint → truly parallel, no serialization between them.
- **E3a** — `templates/c-xml-expat` (file)
- **E3b** — `templates/c-xml-expat-dom` (dir) + `.template`
- **E3c** — `templates/python-sax` (file; already references namespaces today)
- **E3d** — `templates/java-xml-stax.tmpl` (file)

Each lane consumes the Phase-0 optional Lua fields `namespace`/`qualified`,
writing `xmlns:`/prefixes and qualifying names. **Re-read your file fresh** —
the facet pass (D) already edited all four; prefix emission must coexist.
**JSON/Python-JSON targets are NO-OPS** (`c-json-jsonc`, `python-json.tmpl`,
`java-json.org.tmpl`) — not in any lane. If a lane adds a namespace helper,
**mirror `shared/facets`**: pure functions, zero literal text,
`include 'shared/<name>'`, and **sort any NS→element / prefix→type map by a
stable string key** before emitting (Lua iterates object-keyed tables
nondeterministically; D's c-xml-expat determinism fix did the same).
**MUST NOT TOUCH:** `shared/trnsfrm_old`, `LuaAdapter`, any other target, or
each other's files.
**Verify (per lane):** that target's round-trips re-verify prefixed/`xmlns`
output for the E4 namespaced fixtures.

### Lane E4 — fixtures + goldens
**OWNS:** new `test/xsd-positive/*.xsd`, new `test/xsd-positive/import/*`, new
`test/xsd-negative/*.xsd`, `test/gen_roundtrip_tests.py` (`KNOWN_FAILING`
additions only). **MUST NOT TOUCH** existing goldens or `TARGETS`.
**DEPENDS ON:** nothing in the lanes — can start immediately after Phase 0 and
run wholly in parallel; its goldens are regenerated in the integrate step.

New positives: (1) single `targetNamespace` + `xmlns:tns=` + prefixed self-ref
`type="tns:MyType"`; (2) `elementFormDefault="qualified"` and a twin
`="unqualified"`; (3) **default-ns-as-target** (`xmlns=` == `targetNamespace`,
no prefix); (4) **two-file `xs:import`** under `test/xsd-positive/import/`
(primary + imported schema, foreign NS, cross-NS `type=`/`ref=`). New
negatives: unresolved cross-NS ref; missing import / undeclared prefix.
`gen_roundtrip_tests.py` globs `xsd-positive/*.xsd` → one TEST per target;
keep the **imported** schema in the `import/` subdir so the harness doesn't
round-trip it standalone. Gate genuinely unrepresentable combos (e.g. JSON
collapsing a qualified attr/elem) via `KNOWN_FAILING[suite]` (`DISABLED_`).

---

## Integrate (ONE agent, after all lanes)
- Add the optional `namespace`/`qualified` fields to the model in **both**
  `src/Processors/LuaAdapter.cpp` (read form-defaults off `Schema`; emit only
  when non-default, mirroring `LuaType::Facets`' `empty()`-gate) and
  `templates/shared/trnsfrm_old` (`outTbl.namespace = v.namespace` etc., same
  nil-when-absent discipline). This is the ONE shared write deferred out of the
  lanes to avoid a merge conflict — both files are edited here, by one agent.
- Regenerate `test/xsdparse/*.out` as one reviewed commit; diff must touch
  **only** fixtures declaring a `targetNamespace`.
- Run the full `build/test/xsdb-test`; all 7 round-trips across the new corpus.
- Docs: drop README "Currently the tools are not namespace aware."; CHANGELOG
  `Added` (namespace + `xs:import`) with the behavior-change callout.

---

## Risks
- **`QualifyElementName`/`Namespace()` prefix must stay byte-identical** —
  drives all DOM tag traversal; Phase 0's `Namespace()` re-derivation is the
  single highest-risk edit. Regress full suite before fanning out.
- **`XsdQName` vs `Types::QName`** token clash — use `XsdQName`.
- **Golden churn** — the `test` template dumps the whole `schema` table
  verbatim; any leaked field rewrites every golden. Keep new fields out of the
  model until Integrate, gated strictly non-default in BOTH `LuaAdapter` and
  `trnsfrm_old`.
- **Default-namespace ambiguity** (`xmlns=` for XSD-lang vs target NS):
  `ResolveQName_` maps no-prefix via the default `xmlns` URI, never assumes
  `XSD_NS`. Covered by E4 fixture #3.
- **Behavior change** — NS-correct resolution: schemas that "worked by
  accident" now resolve differently or error → CHANGELOG `Added` + drop the
  README "not namespace aware" line (in Integrate).
- **Serialization constraint:** E2 and Phase 0 both edit `Node.cpp`/
  `Schema.cpp` (factory table + `ParseChildren` filter), so **E2 cannot run
  concurrently with Phase 0** — it starts only once Phase 0 lands. The E3a–E3d
  template lanes and E4 are fully independent of each other and of E2.

## Verification (gtest binary, never ctest)
- **Phase 0:** full suite green; `Generate.MatchesGoldenCorpus` byte-identical;
  7 round-trips green.
- **E2:** two-NS fixtures resolve; include fixtures green; negatives throw.
- **E3a–d:** each target's XML round-trips emit correct prefixes/`xmlns`.
- **E4:** corpus globs into all 7 targets; `KNOWN_FAILING` only for genuinely
  unrepresentable combos.
- **Integrate:** goldens regenerated, diff scoped to namespaced fixtures only.
