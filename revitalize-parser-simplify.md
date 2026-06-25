# C++ parser simplification — findings (analysis pass)

Read-only analysis of `src/XSDParser/` (4 agents: Node core, Element classes,
Types/Restriction, Parser/Schema/Include). Guardrail for any change: full gtest
suite (322/0) + byte-compared `test/xsdparse/*.out` goldens. F already did the
functional-recursion traversal; these are the remaining opportunities.

## High — large, byte-preserving dedup
- **H1 Facet leaf classes → one `FacetNode<T>` base** (~350 lines). The 12 facet
  classes (Min/MaxInclusive/Exclusive, Min/MaxLength, Length, TotalDigits,
  FractionDigits, Enumeration, Pattern, WhiteSpace) are byte-identical except
  the tag, the `ParseElement` dispatch, and `Value()`'s type. A CRTP/base holds
  the ctor, `ParseChildren`, `GetParentType`, `HasValue`, `Value()`.
  CAVEAT: `Enumeration::ParseChildren` calls `ParseChildren` where the others
  call `ParseElement` (pre-existing quirk) — keep it as an override so output is
  unchanged.
- **H2 `Node::GetParentType` default delegation** (~23 four-line bodies). 23
  classes repeat `unique_ptr<Node>(Parent())->GetParentType()`. Make the base
  method non-pure with that default; only the real-type classes override
  (Element/ComplexType/SimpleType/List/Union/Schema/Restriction/Extension).
  Overlaps H1 (subsumes the facets' GetParentType).
- **H3 `_ConstructNode` if/else ladder → table** (Node.cpp:138-210, ~70→~40).
  34 branches `if (iequals(X::XSDTag(), name)) return new X(...)` → a static
  `{tag, factory-lambda}` table + one scan. Schema's 3-arg ctor still fits.
- **H4 Drop dead `Parser m_parser`** (Include.hpp:41) — declared, never inited,
  never used; an embedded Parser per include node. (E investigation flagged it
  too.) + unused `Exception.hpp` include.

## Medium — clear, byte-preserving
- **M1 `Node::_maxOccurs(dflt)`/`_minOccurs(dflt)`** — Choice/Group/Any are
  byte-identical; All/Element keep tailored logic.
- **M2 Parser `m_docLst` finders** — 5 hand-rolled linear scans → 2 private
  `_findByUri`/`_findByDoc`; also kills the double-scan in `Parse`.
- **M3 `GetParentType` sole-child delegation helper** — ComplexContent.cpp:67
  and SimpleContent.cpp:64 are verbatim identical (and SimpleType's variant);
  one `_delegateToSoleChild` helper.
- **M4 Restriction/Extension `Base()`+`GetParentType()`** — identical in both;
  a shared `DerivationNode` base or `_baseType(Node&)` helper.
- **M5 `Name()` → `Node::_name()`** — identical in ~6 classes.
- **M6 `_Type` cleanup** (Node.cpp:271-291) — hoist the repeated
  `delete pRetType`; short-circuit so ComplexType lookup only runs when the
  SimpleType lookup missed (efficiency on the resolution path).
- **M7 LuaProcessorBase 34 identical callbacks** (`pNode->ParseChildren(*this)`,
  163 lines) → X-macro/table over the element list (enumerated 3× today). The
  "default-recurse in BaseProcessor" variant is riskier — keep the X-macro.
- **M8 Type-wrapper twins** (Types.cpp) — `SimpleType`/`ComplexType` `clone()`
  /`isTypeRelated`/`Name()` are copy-paste (ComplexType inlines `typeid` instead
  of the `XSD_ISTYPE` macro) → a `WrappedType<Elm>` template.

## Low / cleanup
- Delete the already-commented `_isElmRelated`/`_findElm` block (Restriction.cpp
  :177-227). Duplicate `operator==` + `LookupType` pass-through (Node). `HasXxx()`
  one-liner macro (~43 wrappers). `_requireChild` find-or-throw helper. TypesDB
  ctor ↔ Types.hpp `XSD_TYPEDEF` X-macro (48 lines, sync hazard). Stale
  `Schema.cpp` header comment ("Document.cpp").

## Defer — overlaps Workstream E
- Per-document `Namespace()` cache for `_StripNamespace`/`QualifyElementName`
  (Node) — E reworks this exact surface with `_ResolveQName`; do it there.

## Latent BUGS surfaced (NOT auto-fix — would shift output, need golden re-bless)
- **SimpleType `<union>` never wired**: SimpleType.cpp:71-80 has a duplicated
  dead `else if` (same condition twice); a `<simpleType><union>` falls through
  to `throw`. Dropping the dead duplicate is byte-preserving; wiring union is a
  behavior change.
- **`Enumeration::ParseChildren`** calls `ParseChildren` not `ParseElement`
  (unlike every sibling facet).
- **`Annotation`** is in all three `XSD_ISELEMENT` groups in
  `Restriction::ParseChildren`, so it's gated behind the numeric `isTypeRelated`
  check (first branch) — likely unintended.
