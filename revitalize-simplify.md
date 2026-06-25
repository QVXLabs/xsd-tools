# Workstream D — Deep Simplification / Consolidation (OUTCOME)

> Outcome record. This document now reports the ACTUAL result of the
> simplification pass; the original intended-consolidation analysis is kept
> per item as context. No code is modified by this document.
> Scope: the uncommitted Workstream-D body on branch `revitalize`. The
> parallel per-target fan-outs (D2 emission, D2b narrowing, C-perf,
> factory-triple) produced cross-target duplication that this pass DRYed up
> while keeping the gtest suite green and generated output byte-identical.

## Outcome summary

**Landed (all byte-identical / behavior-preserving):**

- **H1** — C integer-narrowing unified into `facets_narrowKey` +
  `facets_narrowType` in `templates/shared/facets`; all 3 C targets delegate.
- **M1** — Java signed-narrowing unified into `facets_narrowestSigned`; all 3
  Java copies delegate.
- **H3** — sdbm collision-detection + hash grouping extracted as `sdbm_group`;
  all 3 C enum emitters call it but keep their own emit/order (lighter than the
  planned full emitter merge, which would have shifted DOM output).
- **Determinism fix** (beyond the original plan) — c-xml-expat's `uniqueLists`/
  `uniqueNonLists` made sorted arrays so output is reproducible run-to-run.

**Net effect:** the narrowing algorithm, Java signed-narrowing, and sdbm
collision-detection now each live in exactly one place. Every landed item is
byte-identical on the deterministic targets and round-trips green (C 126/0,
Java 84/0).

**Skipped (by judgment):**

- **H2** — over-consolidation; the 3 C validation renderers diverge on error
  mechanism, enum-switch helper, and value access, and a shared
  `c_facetChecks` would change expat/c-json output.
- **M3** — ~2 shared lines (`FactoryMthdName_`), can't be tab/space-neutral
  across targets; value ≈ 0.

(M2, L1, L2 were out of scope for this pass and are left as planning notes
below.)

## Guardrails (verification premise actually used)

- **Behavior-preserving**: full `build/test/xsdb-test` green. Round-trips are
  the real safety net for C/Java emission.
- **Corrected verification premise (this is the basis the landed items were
  proven against):** the `Generate.MatchesGoldenCorpus` gtest and
  `test/xsdparse/*.out` goldens regenerate only via `templates/test` (the
  parser-model dump) — they do **not** exercise any C/Java/Python target
  template. So they could not prove H1/H3/M1; those goldens only catch a
  regression in the parser model. The proofs actually used for each landed
  item were **per-target byte-identical regeneration diffs** (generate the
  affected target before/after, `diff` the emitted source — 0 diffs on the
  deterministic targets) plus the **round-trip gtest** filters. Concretely:
  1. **Byte-identical regeneration diff** — generate the affected target for a
     fixture before and after the change and `diff` the emitted source. This is
     the strongest proof for the golden-safe items (H1, M3) and the way to
     confirm an OUTPUT-AFFECTING item changed nothing it shouldn't.
     Concrete form (run from the build dir, `xsdb` is the built tool):
     `./xsdb ../templates/<target> test/xsd-facets/facet_uint8.xsd > /tmp/after`
     and diff against the same command on the pre-change tree.
  2. **Round-trip gtest** — the `*-test.py`-derived round-trip tests
     (registered into `xsdb-test`) generate real C/Java, compile, round-trip
     data; they
     are what catches a semantic narrowing/validation change.
- **Byte-identical output is the default target.** Any item that *moves a Lua
  helper* (no change to emitted bytes) is golden-safe. Any item that *re-routes
  a target through a different renderer* (e.g. expat → shared `c_facetChecks`)
  can change emitted bytes and needs round-trip re-verification; such items are
  flagged **OUTPUT-AFFECTING** below.
- **Match the file/area's existing spacing/naming** (CLAUDE.md): tabs in C++,
  2-space CMake, per-template local Lua style. `c-xml-expat-dom/shared` uses
  3-space Lua; `shared/facets` uses tab Lua — a helper's style follows its
  destination file.
- **Templates-only** wherever possible; the C++ bridge is already minimal and
  correct (see "C++ bridge" note).

## Where shared code already lives (ground truth)

- `templates/shared/facets` — pure facet model: `facets_intRange`,
  `facets_narrowest`, `facets_checks`, `facets_sampleInt/String`. Language-
  neutral; no emission. This is the natural home for new *language-neutral*
  helpers. **Signature note (verified):** `facets_intRange(facets)` returns two
  values `lo, hi` (not a table); `facets_narrowest(lo, hi)` takes two args.
  Call sites rely on multi-return splice:
  `facets_narrowest(facets_intRange(facets))`. A new key helper must preserve
  that — `facets_narrowKey(facets)` should internally do
  `local n = facets_narrowest(facets_intRange(facets))` and return a string/nil.
- `templates/c-xml-expat-dom/shared` — DOM-local but **already the most
  factored C renderer**: holds `c_narrowSingleton` (l.31), `c_sdbmStringSwitch`
  (l.58), `c_facetChecks` (l.91). These are the consolidation *targets* other C
  templates should converge on (promote to a `templates/shared/c_facets`).
- `c-json-jsonc/c_type_info` & `c-xml-expat-sax/c_type_info` — each carries a
  **byte-identical** `c_narrowMap`/`c_intTypes`/`c_narrowCType` block (verified
  identical, ~22 lines each).

---

## Ranked findings

Rank = (duplication removed × safety). HIGH = large dup + golden-safe (helper
move). MED = real dup but OUTPUT-AFFECTING or moderate size. LOW = small or
mostly-documentation.

### H1 — DONE, byte-identical — unified C integer-narrowing helper

**Outcome:** extracted `facets_narrowKey(facets)` and
`facets_narrowType(base, facets, narrowMap, narrowable)` into
`templates/shared/facets`. The 3 C targets now delegate:
`c-json-jsonc/c_type_info` and `c-xml-expat-sax/c_type_info` `c_narrowCType`
are one-line calls (`facets_narrowType(c_type_info[xsdType], facets,
c_narrowMap, c_intTypes)`); `c-xml-expat-dom/shared` `c_narrowSingleton`
delegates too (passing its DOM-local `byBits`/`narrowable`). Each target keeps
only its own bucket→type vocabulary. Verified byte-identical on the
deterministic targets; C round-trips green.

**Original analysis (intended consolidation) follows.**

**Duplicated, three ways, same math (re-verified against current tree):**
- `c-json-jsonc/c_type_info:122-145` — `c_narrowMap`+`c_intTypes`+
  `c_narrowCType`.
- `c-xml-expat-sax/c_type_info:121-144` — **CONFIRMED byte-identical** to the
  c-json block. `diff` of just those line ranges is empty; the only file-level
  differences are the per-target `include` path prefix
  (`c-json-jsonc/…` vs `c-xml-expat-sax/…`) and one extra `include
  'shared/facets'` line in c-json's header. The narrow block itself is identical
  character-for-character (tabs included).
- `c-xml-expat-dom/shared:31-41` — `c_narrowSingleton(baseClass, facets)`: same
  `facets_narrowest(facets_intRange(facets))` core, **but 3-space indented** and
  it maps the bucket key to a *singleton object* (`byte`/`short`/`int`/`integer`
  /`unsigned*`) rather than a ctype object. Its guard is a `narrowable` set
  built from the `byBits` values (line 37-38), not the ctype `c_intTypes` set.

Plus the **call-site reality** (verified): `c_narrowCType` is referenced widely
— c-xml-expat: 12 sites; c-json-jsonc.template: 13; and **also in the two
`-test` drivers** (`c-xml-expat-test:54,90`, `c-json-jsonc.template-test:50,86`)
and `c-xml-expat-dom/types:36` for the singleton form. The shared helper must be
in scope for the `-test` drivers too (they `include` the same `c_type_info`).

All three reduce to: *given facets, pick the narrowest signed/unsigned 8/16/32/
64 bucket, else fall through to the base type.* The bucket→concrete-type map
(ctype object vs singleton class) is the only genuinely per-template part.

**Consolidated form (exact):** add one neutral helper to
`templates/shared/facets` (tab-indented, matching that file):

```lua
-- "bits|signed" key for the narrowest int holding the facet range, or nil.
function facets_narrowKey(facets)
	local n = facets_narrowest(facets_intRange(facets))
	return n and (n.bits .. '|' .. tostring(n.signed)) or nil
end
```

Return shape: a string like `'8|true'`/`'32|false'`, or `nil` when unbounded.
Lives in `shared/facets` (already imported transitively by every C target).

Per target, what STAYS local vs MOVES:
- **c-json-jsonc & c-xml-expat-sax**: keep the small `c_narrowMap` and the
  `c_intTypes` guard set (both are ctype-object tables — genuinely per-target
  vocabulary). Replace the `c_narrowCType` body's inline
  `facets_narrowest(facets_intRange(facets))` + key-build (lines 142-143) with
  `local key = facets_narrowKey(facets)` then
  `return (key and c_narrowMap[key]) or base`. The duplicated *plumbing* (the
  intRange→narrowest splice + string concat) leaves both files; the lookup
  tables remain. This removes the second byte-identical concat line and keeps
  the byte-identical copies from drifting, but does **not** by itself dedup the
  two `c_narrowMap`/`c_intTypes` tables (see Risk below).
- **c-xml-expat-dom**: rewrite `c_narrowSingleton` (lines 39-40) to
  `local key = facets_narrowKey(facets)` /
  `return (narrowable[baseClass] and key and byBits[key]) or baseClass`. Keep
  the DOM-local `byBits`/`narrowable` tables and the 3-space style.

**Bigger win available (optional):** the two ctype tables `c_narrowMap` +
`c_intTypes` are themselves byte-identical between c-json and c-xml-expat-sax,
so a `facets_narrowCType(c_narrowMap, base, facets)` *generic wrapper* in
`shared/facets` (taking the per-target map + base as args) would let both
targets delete the whole `c_narrowCType` function body and keep only their
`c_narrowMap`/`c_intTypes` literals. This pushes the net removal toward the
plan's high end (~22 more lines) at no extra risk, since the tables are still
per-target — only the function body is shared. Recommended.

**Risk/blast:** low, but **note the "~50-60 lines" is only reached with the
optional wrapper.** The bare `facets_narrowKey` move alone removes ~2 plumbing
lines × 3 ≈ 6 lines (the tables and guards stay). The DOM rewrite is in the
in-flight `c-xml-expat-dom/shared` file — **re-verify after the DOM value-
storage change settles** (`c_narrowSingleton`'s neighbours, not the function
itself, are what's moving). **Golden-safe**: same key → same chosen type →
same emitted ctype/singleton. No emitted bytes change.

**Proof:** golden-safe, so the byte-identical regeneration diff is exhaustive.
Pick fixtures that actually narrow: `test/xsd-facets/facet_int16.xsd`,
`facet_uint8.xsd`, `facet_range.xsd`. For each C target T in
{c-xml-expat, c-xml-expat-sax (via its template), c-json-jsonc.template} and
each fixture F:
`./xsdb ../templates/<T> ../test/xsd-facets/<F> | diff - <baseline>`
must be empty (baseline = same command on the pre-change tree). Then run the C
round-trip filters in `xsdb-test` to confirm compile+behavior.

### H2 — SKIPPED (over-consolidation) — shared `c_facetChecks`

**Outcome — SKIPPED by judgment.** The 3 C validation renderers diverge on
error mechanism (`xml_invalid_` / `return -1` / `return 1`), on the
enum-switch helper, and on value access (DOM `pContentStr`/`ppAttribs` vs
expat buffer vs json `safeCStr_`). A shared `c_facetChecks` needs heavy
parameterization AND would change expat/c-json output. Judged
over-consolidation — it trades 3 clear, self-contained renderers for one
clunky parameterized one. No `templates/shared/c_facets` was created; expat
keeps its inline `cChecks`, c-json its inline switch, DOM its
`c_facetChecks`.

**Original analysis (intended consolidation) follows.**

**Three independent renderers of `facets_checks()` → C validation:**
- `c-xml-expat-dom/shared:91-142` — `c_facetChecks(label,strExpr,facets,
  typedef)`: the clean reference; iterates `facets_checks`, dispatches on
  `kind`, calls `c_sdbmStringSwitch` for enums.
- `c-xml-expat:662-689` (`cChecks`-style inline at the facet block, calling
  `sdbmCases` at l.681) — its own inline iterate+dispatch.
- `c-json-jsonc.template:383-437` — inline `table.map(facets_checks(...))`
  with its own switch/`SDBMHash_` enum tie-break.

The `range`/`length`/`enum` C statement shapes are the same C; the differences
are the **string-accessor** (DOM `pContentStr` / `ppAttribs` vs expat buffer vs
json `safeCStr_`) and the **enum switch helper** (see H3). Parameterize the
accessor + the enum-switch callback and all three collapse onto one renderer.

**Consolidated form:** move `c_facetChecks` (and its dependency `c_sdbmString
Switch`, H3) into a new `templates/shared/c_facets`; have all three C templates
`include 'shared/c_facets'` and pass a per-target `strExpr`/enum-switch. Removes
~40 lines (expat inline) + ~50 lines (json inline). Net ~90 lines.

**Risk/blast:** medium. **OUTPUT-AFFECTING** — routing expat and json through the
DOM renderer can shift whitespace/case-ordering of emitted validation. Verify
with the **facet round-trips** (`test/xsd-facets/`) and the **byte-identical
regeneration diff** per target — NOT the xsdparse goldens, which never
exercise these templates (see corrected Guardrail). Do H3 first (a dependency).
Stage one target at a time.
**DOM-IN-FLIGHT:** the reference `c_facetChecks` lives in `c-xml-expat-dom/shared`
and its `strExpr` accessors (`pContentStr`/`ppAttribs`) are exactly what the
in-flight DOM value-storage (scalar-inlining) change touches. **Re-verify after
the DOM value-storage settles** before promoting this renderer — the parameter
it abstracts (the string accessor) is moving underneath it. Treat the line
numbers `91-142` as approximate.

### H3 — DONE, byte-identical — via a lighter approach than the planned merge

**Outcome — DONE via a lighter, byte-identical approach.** Rather than
unifying the 3 divergent emitters (which would have changed DOM's case order —
the output-shift the analysis below flags), only the shared, error-prone part
was extracted: **collision detection + hash grouping** as
`sdbm_group(keys) -> groups, order, collision` in `templates/shared/facets`.
All three targets call it but keep their OWN emit and order:
`c-xml-expat` `sdbmCases` (keeps its no-collision fast path), `c-xml-expat-dom`
`c_sdbmStringSwitch` (keeps its always-switch hash-order emit), and the c-json
inline enum switch (keeps `SDBMHash_` runtime hashing). Output is therefore
byte-identical (0 diffs; C round-trips 126/0). Chosen because it removes the
duplicated, bug-prone grouping logic without forcing the one-time golden shift
the full-merge would have required.

**Original analysis (intended full-merge consolidation) follows.**

**Three implementations of the same collision-safe hash-switch:**
- `c-xml-expat:59-81` — `sdbmCases(scrut,keys,caseFn,bodyFn,ind)` (codegen-time
  grouping of colliding keys; ~23 lines), used at l.580/598/681.
- `c-xml-expat-dom/shared:58-85` — `c_sdbmStringSwitch(strExpr,values,valExpr)`
  (~28 lines).
- `c-json-jsonc.template:383-437` — inline `SDBMHash_` runtime hash + hand-rolled
  `case`/`strcmp` tie-break (within the facet block).

All emit: hash the string, `switch(hash)`, and on a colliding bucket fall back to
`strcmp`. The callback shape (what each `case` body emits) is the only real
variation.

**CORRECTION — the three are NOT interchangeable; emit *structure* differs:**
- expat `sdbmCases` (l.59-81) keeps an explicit `order` sequence and emits cases
  in **source order**, AND has a **no-collision fast path** (l.69-72) that emits
  bare `caseFn(k)` per key with **no `switch` wrapper at all** when every hash
  is distinct. It only builds a real `switch` + `strcmp` when keys collide.
- DOM `c_sdbmStringSwitch` (l.58-85) **always** emits `switch (sdbmHash_(...))`,
  iterating `byHash` via `table.map` over a **hash-keyed table** — i.e.
  `pairs`-order, which for hash keys is **unordered/non-deterministic in source
  order** (confirmed: `table:map` in `src/TemplateEngine/table.lua:20` is
  `for k,v in pairs`). It has no fast path.
So expat and DOM produce **structurally different C** for the same enum: expat
may emit no switch; DOM always does, in hash order. These cannot be collapsed
"behavior-preserving + byte-identical" onto one emitter — one of the two
output shapes must be chosen and the other target's golden re-baselined.

**Consolidated form:** one `c_sdbmStringSwitch(strExpr, values, caseFn)` in
`templates/shared/c_facets` (generalize the DOM version's interface, which is the
cleanest), with a `caseFn` callback supplying the per-target case body. Replaces
`sdbmCases` and the json inline. Removes ~23 (expat) + ~18 (json switch body) ≈
40 lines, and is the prerequisite for H2.

**Risk/blast:** medium. **OUTPUT-AFFECTING** if the unified emitter orders cases
or formats `strcmp` chains differently from any one target today. Note the DOM
version iterates `byHash` via `table.map` (hash-keyed, **unordered**), so its
case order is already non-deterministic in source order — pinning to "current
per-target order" means pinning to a *stable* iteration (sort the buckets) and
accepting a one-time golden shift for whichever targets it reorders. Verify with
`test/xsd-facets/facet_enum.xsd` (the single enum fixture present — note: not
`facet_enum*`, only one such file exists) via the byte-identical regeneration
diff + round-trips.
**DOM-IN-FLIGHT:** `c_sdbmStringSwitch` is in `c-xml-expat-dom/shared`; it does
not itself read storage but it is co-located with the DOM change. **Re-verify
line numbers (`58-85`) and the surrounding file after the DOM value-storage
settles.**

### Determinism fix — DONE (beyond the original plan) — reproducible c-xml-expat output

**Outcome — DONE, new item not in the original plan.** H1 surfaced that
c-xml-expat's `uniqueCTypes` is keyed by ctype OBJECTS, so iterating it for
emission produced address-hash order — i.e. non-reproducible output run-to-run.
Fixed by deriving `uniqueLists`/`uniqueNonLists` as **sorted arrays** (sorted by
`c_type.name()`, via a `cTypeName` normalizer because `c_type.name` is a
function for scalars but a plain string for list types). c-xml-expat output is
now reproducible; this also made the byte-identical regeneration diff a reliable
proof for the other landed items. Location: `templates/c-xml-expat:36-51`.

### M1 — DONE, byte-identical — unified Java signed-narrowing

**Outcome — DONE, byte-identical** (NOT the output-affecting variant the
analysis below feared). Added `facets_narrowestSigned(facets)` (returns
`'Byte'`/`'Short'`/`'Integer'`/`'Long'`) to `templates/shared/facets`. All
THREE Java copies delegate: `java-json.org/parse.lua` `javaNarrowType` is now an
alias for it; `java-xml-stax.tmpl` and `java-xml-stax.tmpl-test` re-keyed their
`javaSigned` tables by **box name** (`Byte`/`Short`/…) and `javaNumType` calls
the shared helper then looks up the per-target column (`parse` in the main
template, `prim` in the test driver). The signed-only fit is preserved, so the
`facet_uint8` case still maps to Short. Byte-diffs 0; Java round-trips 84/0.

**Original analysis (intended consolidation) follows.**

- `java-xml-stax.tmpl:253-270` — `javaSigned` table + `javaNumType(info,facets)`;
  returns the matched **table** `{box, parse, lo, hi}`. Call sites use both
  `.box` and `.parse` (lines 279, 296, 354, 387, 425).
- `java-json.org/parse.lua:84-91` — `javaNarrowType(facets)`: returns a **bare
  string** `'Byte'/'Short'/'Integer'/'Long'` via inline literal bounds.
- `java-xml-stax.tmpl-test:37-50` — a **third** copy: `javaSigned` with
  `{box, prim, lo, hi}` (note **`prim`, not `parse`**) and its own `javaNumType`
  returning that table; call site `assignTmp` uses `.box` and `.prim`.

**CORRECTION to the plan — `facets_narrowest` is NOT directly reusable for
Java.** It buckets by signedness derived from `lo < 0` and will return an
*unsigned* bucket (e.g. `{bits=8, signed=false}` for `0..255`). Java has no
unsigned type, so all three Java copies map `0..255` to **Short**, not byte.
Routing Java through `facets_narrowest`'s key would pick the 8-bit bucket and
mis-narrow. The Java helpers must keep their **signed-only** ladder
(`lo>=-128 and hi<=127 → Byte`, …) which is a *different* fit than
`facets_narrowest`. So the shared core for Java is NOT `facets_narrowest`; it is
a new signed-only `facets_narrowestSigned(facets)` (or the Java helper keeps its
own ladder). The `facet_uint8.xsd` fixture is exactly the divergence case —
verify it stays Short.

**Consolidated form (revised):** the three copies differ in **return shape**
(table-with-`parse` / table-with-`prim` / bare-string), so a single shared
helper cannot hand back one object all three consume. Two viable shapes:
(a) shared `facets_narrowestSigned(facets)` returning `{box, lo, hi}` (the
signed-only fit, no language column), in `shared/facets`; each Java consumer
keeps its tiny per-target satellite map keyed by `box`
(`{Byte={parse=…}}` for stax main, `{Byte={prim=…}}` for the test driver,
nothing for parse.lua which only needs `box`). (b) leave the three as-is and
only dedup the *literal bounds ladder* into one helper returning the box string,
with each caller looking up its `parse`/`prim` separately. Prefer (a): it
removes the duplicated ladder (~6 lines × 3) and the duplicated `javaSigned`
literals (~5 lines × 2 in the two stax copies), net ~20 lines, while keeping
the genuinely per-target `parse`/`prim` columns local.

**Risk/blast:** **higher than ranked (lift from MED-low to MED).** The
unsigned-bucket trap above means a naive "use `facets_narrowest`" implementation
silently changes Java field types for any `0..2^n-1`-style facet — that *is*
OUTPUT-AFFECTING and would only surface in the Java round-trip, not a unit test.
Also the two stax `javaSigned` tables differ by one column name (`parse` vs
`prim`); the shared helper must drop both and let callers re-attach, or it can't
be neutral. **Proof:** byte-identical Java regeneration diff over
`facet_int16`, `facet_uint8` (the Short-not-byte case), and a plain
`0..127`/`-128..127` boundary fixture, for both `java-xml-stax.tmpl` and
`java-json.org.tmpl`; then the Java round-trip filters in `xsdb-test`. Java is
non-DOM and independent of the C chain — safe to fan out in parallel.

### M2 — Shared facet-check *driver* for the 4 non-C targets (MED, OUTPUT-AFFECTING)

The four non-C renderers are structurally identical (loop `facets_checks`,
switch on `c.kind` ∈ range/length/enum, emit a guard), differing only in the
emitted snippet:
- `python-json.tmpl:56-92` `pyChecks(prefix,label,typeName,facets)` (~37 lines)
- `python-sax:82-115` `pyChecks(label,facets,isNumeric)` (~34 lines)
- `java-xml-stax.tmpl:211-249` `javaChecks(label,jt,facets,statics)` (~39 lines)
- `java-json.org/parse.lua:134-175` `javaChecks(var,facets,jType,sets)` (~42)

**Consolidated form:** a neutral `facets_eachCheck(facets, handlers)` in
`templates/shared/facets` that walks `facets_checks` and dispatches to
`handlers.range/length/enum(c)` callbacks; each target keeps only its ~3 small
language snippets. Removes the repeated loop/dispatch scaffold (~4 targets ×
~10 lines ≈ 40 lines), leaving the genuinely per-language guard text.

**Risk/blast:** medium. **OUTPUT-AFFECTING** if the unified walk changes emission
order vs any target. Lower priority than the C items because each renderer is
already self-contained and the dup is scaffold-only; do it last, one target at a
time, behind the per-language facet round-trips. Also align the two divergent
enum-set naming schemes here (`_ENUM_<prefix>_<label>` in python-json vs
`_enum<seq>` in python-sax) — pick one.

### M3 — SKIPPED — factory macro skeleton not shareable

**Outcome — SKIPPED.** The only logically identical part is the
`FactoryMthdName_` one-liner (~2 shared lines), and even it can't be
tab/space-neutral across targets (expat/json use a tab, DOM a single space), so
a shared include would force a cosmetic golden change for no real gain. The
`Allocate`/`Construct`/`DestroyElement` bodies are genuinely different macros.
`FactoryMthdName_` remains per-target. Net value ≈ 0; dropped as planned.

**Original analysis follows.**

`FactoryMthdName_` + `AllocateElement`/`ConstructElement`/`DestroyElement` exist
in all three C templates (verified line ranges):
- `c-xml-expat:234-243` — `FactoryMthdName_(T,F)`, `AllocateElement(T,P,F)`
  (in-place `?:`, returns `P`), `ConstructElement(T,P,F)` (`if`-guard),
  `DestroyElement(T,P,F)` (`if`-guard). **Tab-indented**, trailing `\` aligned
  with tabs.
- `c-xml-expat-dom.template:446-461` — `FactoryMthdName_`,
  `AllocateElement(T,F)` (`?:` falling to `alloc_(sizeof(struct T), F)`),
  `ConstructElement(T,P,F)` (`if`), `DestroyElement(T,P,F)` (`?:` falling to
  `ReallocateMemory(F,P,0)`). **2-space indented.**
- `c-json-jsonc.template:175-185` — `AllocateElement(T,M,F)` (`?:` →
  `DefaultAlloc_(M,T)`), `ConstructElement(T,P,M,F)` (`if`),
  `DestroyElement(T,P,M,F)` (`?:`). **Tab-indented**, marshaller arg `M` added.

**CORRECTION — the duplication is much thinner than ranked.** Only
`FactoryMthdName_(T,F) → T##F` is logically identical, and even it is **not
byte-identical**: expat/json use a tab between args/expansion, dom uses a single
space (`#define FactoryMthdName_(T, F) T##F`). The `?:`/`if` dispatch shells are
**not** identical across targets — they differ in (1) arg arity (2 vs 3 vs 4
params), (2) the fallback allocation expression, **and** (3) whether the macro
is `?:` or `if` (DestroyElement is `?:` in dom/json but `if` in expat). So the
"shared `?:`-dispatch skeleton" the plan posits does not exist as a single
neutral form.

**Consolidated form (revised down):** the only safe share is `FactoryMthdName_`
itself, as a one-line shared C include emitted by all three. That saves **1 line
× 2 = 2 lines**, not 6-9, and requires normalizing the dom spacing to match (a
cosmetic golden change for dom). Net value is essentially nil. **Recommend
DROP M3** from this pass, or downgrade to an L-item folded into L1 naming
hygiene; it is not worth a shared include. Do **not** attempt to unify the
`AllocateElement`/`ConstructElement`/`DestroyElement` bodies — the plan already
says so and the byte inspection confirms they are genuinely different macros.

**Risk/blast:** low but **value ≈ 0**; the earlier "~6-9 lines × 2" estimate was
optimistic — actual is ~2 lines and forces a dom whitespace change. If done:
golden-safe only by the byte-identical regeneration diff over any fixture for
all three C targets (the `#define` block must come out unchanged, which for dom
means *keeping* its current spacing and NOT importing a tab-spelled shared
macro). That constraint alone (can't be byte-neutral across tab-vs-space targets)
is why this should be dropped.

### L1 — Dead code / hygiene left by the fan-out (LOW)

- `c-xml-expat-dom`: a `list_trim_` helper marked
  `__attribute__((__unused__))` — confirm unreachable and drop, or wire it.
- Inconsistent C helper naming across targets: `SDBMHash_` (json camelCase) vs
  `sdbmHash_` (dom) vs the expat codegen `sdbm_hash`; `_foo` prefix (expat) vs
  `foo_` suffix (dom). CLAUDE.md mandates `foo_` suffix for generated C — align
  when the H2/H3 shared C include is created (one rename point).
- `include 'shared/facets'` ordering differs across the C templates; normalize
  when adding `shared/c_facets`.
- python-sax emits base64/binascii imports unconditionally while python-json
  gates them on `schema.uniqueTypes()` — align (golden-affecting for python-sax
  output header; verify the python round-trips).

**Risk:** low; mostly cosmetic. Fold into the relevant higher item's PR rather
than standalone.

### L2 — Integer subtemplate boilerplate (LOW, NOT RECOMMENDED NOW)

The 8 `int8t…uint64t` files per C target are near-identical boilerplate (differ
by width/signedness/format specifier). A code-generated factory could collapse
16 files to one. **Defer:** these files predate Workstream D (D only touched
narrowing call-sites within them), so this is not D-introduced duplication and a
factory rewrite is OUTPUT-AFFECTING with a large golden surface for little
maintenance gain. Note it; don't do it in this pass.

### C++ bridge — no consolidation needed

`LuaAdapter.{cpp,hpp}` `LuaFacets`/`Facets()` and the `minOccurs` additions
(`Element.cpp:150`, `LuaProcessor.cpp:114`) are minimal and single-sourced. The
`parser.lua` rewrite (offset-threaded `scan`) and `stringbuffer.lua:append`
returning `self` are clean. Nothing to DRY here; leave as-is.

---

## Estimated duplication removed

| Item | Lines removed (approx) | Output-affecting? | Notes (post-investigation) |
|------|------------------------|-------------------|----------------------------|
| H1 narrowing helper | ~6 bare / ~50–60 with the optional `c_narrowCType` wrapper | no (golden-safe) | DOM part (H1b) waits for value-storage |
| H2 shared `c_facetChecks` | ~90 | yes | WAIT for DOM; `strExpr` accessor is in flight |
| H3 shared sdbm switch | ~40 | yes (structure-changing) | WAIT for DOM; expat fast-path vs DOM always-switch — can't be byte-neutral both ways |
| M1 Java narrowing | ~20 | **yes** (unsigned-bucket trap) | use signed-only fit; lifted from low to MED |
| M2 non-C check driver | ~40 | yes | edits `shared/facets`, sequence after H1a |
| M3 factory macro skeleton | ~2 | no | **DROP** — can't be byte-neutral across tab/space targets |
| L1 hygiene | ~10 | partial | |
| **Total** | **~210–250** (M3 dropped; H1 lower bound if no wrapper) | — | |

(Plus the readability win of one C-facet renderer instead of three, and one
narrowing path instead of four.)

---

## Execution order

Scaffold-first, golden-safe-first, then output-affecting behind the fixtures.

Refined after investigation. Key changes: H1 split into a golden-safe non-DOM
part (do now) and a DOM part (wait for value-storage); H2/H3 gated on the DOM
value-storage settling; M3 dropped; M1 lifted to MED with the unsigned trap.

1. **H1a (non-DOM narrowing helper)** — add `facets_narrowKey` (and optionally
   `facets_narrowCType` wrapper) to `shared/facets`; rewire c-json-jsonc and
   c-xml-expat(-sax) `c_narrowCType` + the two `-test` drivers. Golden-safe,
   verify by byte-identical regeneration diff on `facet_int16/uint8/range`. Do
   first, serially; it edits `shared/facets`, the file other items import.
2. **H1b (DOM `c_narrowSingleton` rewrite)** — same key helper, DOM singleton
   map. **WAIT for the in-flight DOM value-storage change to settle**, then
   apply and re-verify DOM line numbers. Small; do whenever DOM is stable.
3. **M1 (Java narrowing)** — independent of the C work. Use a signed-only
   `facets_narrowestSigned` (NOT `facets_narrowest` — the unsigned-bucket trap);
   keep per-target `parse`/`prim` columns local. Verify the `facet_uint8`
   Short-not-byte case. Can run in **parallel** with H1a (Java files only;
   touches `shared/facets` only if `facets_narrowestSigned` lands there — so
   sequence its `shared/facets` write after H1a's, or put the helper in a
   `java_*` file to avoid the shared-file contention entirely — **preferred**).
4. **H3 (sdbm switch into `shared/c_facets`)** — prerequisite for H2.
   **WAIT for DOM value-storage to settle** (the reference lives in
   `c-xml-expat-dom/shared`). OUTPUT-AFFECTING and structure-changing (expat
   fast-path vs DOM always-switch, hash-order vs source-order): choose one emit
   shape, re-baseline the other target. Verify with `facet_enum.xsd` + C
   round-trips.
5. **H2 (shared `c_facetChecks`)** — depends on H3 **and** on the DOM value-
   storage settling (its `strExpr` accessor is what DOM is changing). Migrate one
   C target at a time, re-running `test/xsd-facets/` round-trips + the
   regeneration diff after each. **Not the xsdparse goldens** (they never hit
   these templates).
6. **M2 (non-C check driver)** — last; largest output-affecting surface across
   4 targets, lowest dup-density. One target per step behind round-trips. Edits
   `shared/facets` → sequence after H1a.
7. **L1 hygiene** — fold into whichever item touches the file; no standalone PR.
   **M3 dropped** (≈2-line win, can't be byte-neutral across tab-vs-space
   targets). **L2 deferred** out of this pass.

### Fan-out vs serial

- **Serial (shared `shared/c_facets` file + DOM dependency):** H3 → H2, both
  gated on DOM value-storage settling. Within the chain each edits the new
  shared file then re-routes targets one at a time.
- **`shared/facets` write-ordering:** H1a, M1 (if it lands its helper there),
  and M2 all append to `shared/facets`. Do **H1a first**, then M1/M2 rebase onto
  it — or keep M1's helper out of `shared/facets` (preferred) to remove the
  dependency.
- **Safe to fan out in parallel NOW** (disjoint files, no DOM dependency):
  - track A: **H1a** (non-DOM C narrowing; edits `shared/facets` + c-json +
    c-xml-expat(-sax) + their `-test` drivers) — do this one first/alone since
    it owns `shared/facets`.
  - track B: **M1** Java narrowing (Java files only; helper in a `java_*` file).
  - (M3 dropped; no track.)
- **Deferred until DOM settles (cannot parallelize now):** H1b, H3, H2 — all
  touch `c-xml-expat-dom/*`, whose value-storage is in flight. M2 can start
  after H1a but is lowest priority.
- Merge point: run the **full** `xsdb-test` (not just facet filters) after all
  tracks land, since H1a's `shared/facets` edit is imported by every target.
