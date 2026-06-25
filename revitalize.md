# Revitalize xsd-tools — Action Plan

> Architecture overview and coding conventions for this project live in
> `CLAUDE.md`. This file is the work plan.

## Context

xsd-tools is a C++ XSD→code generator: a C++ parser (`src/XSDParser/`) builds a
model, `src/Processors/` bridges it to Lua, and Lua **templates** emit code.

**Goal:** "modern & maintained." Scope expanded (user-directed) beyond fast wins
to include core parser work (namespaces) and cross-cutting codegen changes (facet
enforcement) plus a parser-wide style refactor. **This is a multi-week program
that rewrites parser internals and touches every template.** Sequencing is
dependency-aware so the big core changes don't fight each other.

**Hard constraint — tests use only native runner toolchains, never Docker.** The
round-trip harness shells out to `cc`/`python3`/`mvn`/`java`; json-c builds
in-tree via CMake. No test may depend on a container. The GHCR image (A3) is a
user-facing distribution artifact only, never in the build/test path.

**Documentation requirement (cross-cutting).** Every workstream updates
`README.md` and `CHANGELOG.md` as part of its own completion (not deferred):
B → new targets; C → CLI flags; D → facet behavior-change in `Changed`; E → drop
"not namespace aware" + document `xs:import`; A → install/badges. CHANGELOG is
Keep-a-Changelog with an `Unreleased` section; A2 release notes derive from it.
"Docs updated" is part of each workstream's verification.

**Process requirement (cross-cutting).** After each major feature pass (each
workstream / meaningful sub-phase), run a **simplification pass** (reuse /
simplify / efficiency / altitude) before moving on — behavior-preserving, gtest
suite green. This is a gate, not optional.

## Decisions (resolved)
- **C→JSON lib:** **json-c 0.17** (recipe in `~/code/conan-repo/json-c`; ships
  CMake → `find_package(json-c)` → FetchContent fallback, no overlay). Resolve via
  the `fetch_third_party()` helper added in master #35.
- **Java→XML API:** **pure StAX both directions** (`XMLStreamWriter`/`Reader`).
- **Releases:** Unix-first; Windows after the `resource.cpp` POSIX-guard fix.
- **Round-trip coverage:** full corpus; mitigate Maven cost by caching `~/.m2`.
- **Java base64:** `java.util.Base64` (no commons-codec). **Tags:** `v*`.

---

## Workstream A — Distribution & discoverability
- **A0. Portable template discovery** (`src/resource.cpp`, ~30 lines): in
  `GetTemplatePath`, add an exe-relative fallback (`/proc/self/exe` /
  `_NSGetExecutablePath` / `GetModuleFileNameW` → probe `<exedir>/../share/
  xsdtools/templates` and `<exedir>/templates`), and guard `<unistd.h>`/`<pwd.h>`/
  `getpwuid` behind `#ifndef _WIN32` (uses `%USERPROFILE%` on Windows).
- **A1. Versioning:** `cmake/GitVersion.cmake` (git describe → `project(VERSION)`,
  `-DXSD_VERSION=` override); `XSDTOOLS_VERSION` into `resource_config.hpp.in`;
  replace the 2012 banner in `src/main.cpp` + add `--version`; `CHANGELOG.md`.
- **A1b. C standard.** C++ is already correctly pinned to C++11
  (`CMakeLists.txt:31-33`; verified the sources use no >C++11 features). The gap is
  **C**: project-level C standard is unset (only the bundled-lua overlay sets one).
  Add `CMAKE_C_STANDARD`/`CMAKE_C_STANDARD_REQUIRED` (C11), document the standard
  the generated C output requires, and pass an explicit `-std=` in the round-trip
  C compile (`test/roundtrip_util.cpp` `cRoundtripImpl`).
- **A2. Release workflow** `.github/workflows/release.yml` on `v*`: factor build
  into composite action `.github/actions/build-xsdb` reused by both workflows
  (**must carry forward the current CI fixes:** Java 21,
  `-DCMAKE_DISABLE_FIND_PACKAGE_Lua=ON`, stray-lua handling). `cmake --install`
  into `bin/`+`share/`, tar `xsdb-<ver>-<os>-<arch>.tar.gz` + `SHA256SUMS`;
  `-static-libstdc++ -static-libgcc` on Linux; macOS deployment target.
- **A3. Packages:** Homebrew formula (download release tarball) → Docker/GHCR
  image → RPM last.
- **AQ. Quality gates (before A4 docs):**
  - **Coverage report** — build with `--coverage` (llvm-cov/gcov), run the gtest
    suite, produce an HTML/summary report to find untested code paths and
    test-case holes; feed the gaps back as new fixtures/tests.
  - **Perf analysis** — profile the generator on large/representative schemas
    (the parser is recursive post-F; the C→JSON/templates path); find hot spots
    before publishing. Frame pointers are kept on Debug for this.
  Both run **before** A4 docs (user-requested ordering).
- **A4. Docs:** README install quickstart, fix dead wiki link, `examples/`.

## Workstream B — Three new output targets (template-only)
Completes the C/Python/Java × XML/JSON grid (existing: C→XML, Python→XML,
Java→JSON). **Binding design principle:** copy the closest existing analogue and
adapt only format-specific bodies; reuse the `/* FILE: */` markers, `shared/
schemaEx` helpers, the `*_type_info` dispatch-table shape, and the `*-test`
driver; match the analogue's schema model.

**Cross-cutting codegen requirement — overridable type factories.** Every
template (the three new targets **and** the existing C/Python/Java ones) must
construct schema types through **overloadable/overridable factory methods**
(e.g. an `xsd::element`-style `createElement()` per type) rather than inline/
direct construction, so consumers can override a factory to inject a custom
subtype or construction logic. The generated marshal/unmarshal code and the
`*-test` drivers must build instances *only* through these factories (the
round-trip tests then exercise the factory path). Per-language shape: C → a
function-pointer / struct-of-constructors table the caller can override; Python
→ overridable classmethods/factory functions; Java → overridable factory
methods (protected, subclassable). Add this to the `*_type_info` contract so all
targets emit it uniformly, and document it per target in README.
- **B0. Python → JSON** (mirror `python-sax`, stdlib `json`, no dep):
  `templates/python-json/` + `python-json.tmpl` (+`-test`). Cheapest; `python3`-only.
- **B1. C → JSON** (mirror `c-xml-expat`, lib json-c): `templates/c-json-jsonc/`
  emitting `json_object_*`; `c-json-jsonc.template` (+`-test`); reuse C
  struct-emission Lua; keep libb64 for base64/hex.
- **B2. Java → XML** (mirror `python-sax` tree model): pure StAX read+write;
  `templates/java-xml-stax/` + `.tmpl` + `-test`; `test/java-xml-stax.tmpl/pom.xml`
  (json/commons-codec removed). StAX *read* is the highest-risk item — validate on
  one nested XSD first.
- **B3. Test wiring:** add three `TARGETS` rows + helpers in
  `gen_roundtrip_tests.py` / `roundtrip_util.{hpp,cpp}` (`pythonJsonRoundtrip`,
  `cJsonRoundtrip` — generalize `cRoundtripImpl`'s `xml_`→`json_` prefix,
  `javaXmlRoundtrip`); resolve json-c in `test/CMakeLists.txt` like the expat
  find-or-fetch block.

## Workstream C — CLI UX polish (`src/main.cpp`, `src/xsdtools.cpp`)
- **Multi-file output:** add `--out-dir` that splits `/* FILE: */` markers to real
  files; move `splitMarkedFiles` out of `test/roundtrip_util.cpp` into the library
  so CLI and tests share one impl.
- `--list` templates; distinct errors for bad/missing XSD vs missing template
  (`src/XSDParser/Exception.cpp`); real `-h`. Pairs with A1's `--version`.
- Install `src/xsdtools.hpp` + export the `xsdb` library target (embeddable SDK).

## Workstream D — Facet enforcement in generated code
Facets are parsed but dropped: `LuaProcessor::ProcessRestriction`
(`LuaProcessor.cpp:136`) collapses a restriction to its base **without walking
facets**; base facet callbacks are no-ops (`LuaProcessorBase.cpp:154-209`).
- **D1. Bridge (load-bearing):** override facet callbacks in `LuaProcessor`;
  accumulate facets **across the derivation chain**; attach a `facets` sub-table to
  the per-type Lua object in `LuaAdapter.cpp` (omit for unrestricted types to avoid
  golden churn). Update `test/xsdparse/*.out` goldens.
- **D2. Per-target emission:** uniform `validate(varName, facets)` member on each
  `*_type_info` type; driver calls it after unmarshal. C → error/NULL, Python →
  `ValueError`, Java → exception. Shared emitter helper in `templates/shared/`.
- **D2b. Facet-driven smallest-type selection:** the `*_type_info` dispatch must
  pick the **narrowest representation the facets allow**, not just the XSD base
  type — e.g. an integer constrained `minInclusive=0 maxInclusive=255` → `uint8_t`
  (C already ships int8/uint8/16/32/64 type files); `[-128,127]` → `int8_t`, etc.
  A facet→type narrowing helper maps base type + numeric bounds (and length where
  it applies) to the concrete sized type; analogous per language (C fixed-width;
  Java byte/short/int/long; Python keeps int but still validates). Marshal/
  unmarshal follow the chosen type.
- **D3. Phased facets:** P1 numeric range + length + enumeration (no deps); P2
  totalDigits/fractionDigits/whiteSpace; P3 `pattern` (free in Python/Java, vendor
  engine or flag-gate in C).
- **D4. Tests (dedicated fixtures):** `test/xsd-facets/` schemas (created) —
  `facet_uint8`/`int16`/`range`/`enum`/`strlen`/`pattern`. Two gtest checks per
  target: (a) **smallest-type** — generated field uses the narrowest type the
  bounds allow (e.g. assert `uint8_t` in the C output); (b) **validation** —
  feed an out-of-bounds/non-member/pattern-mismatch instance and assert unmarshal
  rejects it (C error/NULL, Python `ValueError`, Java exception), while a valid
  instance round-trips. Kept out of `xsd-positive/` so the generic round-trip
  globs don't run them with facet-violating random values.

## Workstream E — Namespace awareness (+ xs:import) — core parser
Today resolution is local-name only; `Schema::Namespace()` returns the *XSD-lang*
prefix, and `targetNamespace`/`elementFormDefault` are never read.
- **E0. Disentangle (no behavior change):** add `QName{ns,local}`; replace
  `_StripNamespace` with `_ResolveQName`; detect builtins by namespace **URI** not
  the `xs` prefix; keep `QualifyElementName` (`Node.cpp`) traversal working.
- **E1. Single-targetNamespace correctness:** `_FindXSDElm`/`_Type` honor
  `qname.ns == targetNamespace`.
- **E2. `xs:import`:** new `Import` element + `ProcessImport` (no-op base impl) + a
  `Parser` namespace index for cross-document resolution.
- **E3.** Read form-defaults; expose optional `namespace` Lua fields; XML templates
  write prefixes/`xmlns` (JSON ignores NS); keep local-name keys to contain churn.
- **E4. New test XSD fixtures** (with the feature): `targetNamespace` + prefixed
  self-ref; `elementFormDefault` qualified *and* unqualified; default-namespace-as-
  target; a two-file `xs:import` cross-namespace case; negative cases (unresolved
  cross-namespace ref, missing import). Corpus is globbed → all targets exercise
  them, `KNOWN_FAILING`-gate genuine gaps. Update `xsdparse/*.out` goldens.

## Workstream F — Functional-recursion traversal refactor
~41 sibling-iteration sites use the imperative `do { … } while(NextSibling())`
idiom (35 in `src/XSDParser/Elements/*.cpp` `ParseChildren`, plus `Node.cpp`
finders and `Processors/RestrictionVerify.cpp`/`ElementExtracter.cpp`).
Functional-recursion examples exist (`Extension.cpp` `_find`,
`_checkForDuplicateNamedParticles`).
- Add a shared recursive sibling fold/visitor helper near `Node::FirstChild`/
  `NextSibling` and convert the ~41 sites. Pure behavior-preserving refactor —
  `test/xsdparse/*.out` goldens + parser tests are the safety net.
- Add the traversal style rule to `CLAUDE.md` (done at execution start).

## Workstream G — Close open issues
- **#10 namespaces** — closed by E.
- **G1 · #33** finish Py3 migration: remove the legacy Py2 scripts
  (`test/test_framework.py`, `test/conio.py`, `test/*-test.py`) the gtest+python3
  harness supersedes; confirm nothing references them. Land early with F.
- **G2 · #11** duplicate structs for same-named children of different types:
  disambiguate generated names (`shared/schemaEx` `c_safeNames`/`uniqueTypes` +
  each `*_type_info`; add type identity in the `LuaAdapter` bridge if needed) +
  collision fixture. With B/D.
- **G3 · #4** annotation/documentation in output: emit already-parsed
  `xs:annotation`/`xs:documentation` as comments/docstrings per target. With B.

Each fix lands a fixture/test and a `Closes #N` in CHANGELOG/PR.

## Workstream H — Announcement blog post (qvxlabs.com) — LAST
**Separate repo** `~/code/qvxlabs/web/QVXLabs` (static HTML, GAE). Mirror
`static/posts/zopfli.html` exactly → new `static/posts/xsd-tools.html` + index
entry in `static/blog.html`; canonical `https://www.qvxlabs.com/blog/xsd-tools-
<slug>`. Honest "revived & maintained" tone (per Announcement readiness), candid
about the EOL-dep caveat. **Guardrails:** claim only what shipped; publish only
after the release is live and with explicit user sign-off.

## Sequencing & dependencies
0. Branch off up-to-date master (done: `revitalize` even with master incl. #34/#35).
1. **F** first — behavior-preserving, gives D/E a clean traversal base. + CLAUDE.md.
2. **A0 + A1 + A1b** — small, unblock releases; parallel with F.
3. **A2** — release workflow.
4. **B** (Python→JSON, C→JSON, then Java→XML) — builds on F.
5. **D** — bridge + one target end-to-end, then fan out.
6. **E** — largest core effort; E0 → E1 → E2/E3 + fixtures.
7. **A3 + A4** packages/docs. **G** issues ride along (G1 with F; G2/G3 with B/D).
8. **H** blog post last, post-release, with sign-off.

A (distribution) is independent of D/E/F. Core parser order F → D → E shares one
clean base. All four open issues (#4/#10/#11/#33) close this release.

## Risks & mitigations
- **A0 single point of failure for releases:** test extract-and-run on a clean
  runner before publishing; `XSDTOOLS_DATA` backup in Docker.
- **Java→XML StAX read** riskiest codegen: validate on one nested XSD first;
  `KNOWN_FAILING`-gate stubborn cases.
- **Composite action must preserve current CI fixes** (Java 21, disable-Lua,
  stray-lua).
- **Facet bridge** (`ProcessRestriction` + chain accumulation) shared by every
  type/target; guard with `xsdparse` goldens before touching templates; emit
  `facets` only for restricted types; absent/empty `validate` is a no-op.
- **Namespaces blast radius** across `src/XSDParser` + templates;
  `QualifyElementName` is the highest-risk edit; behavior change → changelog it.
- **F 41-site refactor:** strictly behavior-preserving; lean on goldens; land
  before D/E.
- **Maven cost** (full corpus): cache `~/.m2`. **Homebrew** is post-release.

## Announcement readiness
A–F clears a **"revived / new release"** announcement (installable, namespace +
`xs:import` aware, facet-validating output, full C/Python/Java × XML/JSON grid,
all open issues closed). It does **not** fully back a **"modern"** splash to a
critical audience: EOL Lua 5.1 + unmaintained TinyXML remain; existing Python/Java
idioms dated; no TS/Go/Rust; C `pattern` deferred; identity constraints /
`xsi:type` / `nillable` / `redefine` unsupported.

## Verification
All test runs invoke the gtest binary directly (`build/test/xsdb-test
--gtest_filter=…`), not ctest.
- **F:** full `xsdb-test` green + `xsdparse` goldens unchanged.
- **B:** `xsdb-test --gtest_filter='PythonJsonRoundtrip*:CJsonRoundtrip*:JavaXmlRoundtrip*'`;
  manual `xsdb --out-dir … c-json-jsonc.template test/xsd-positive/testA022.xsd`.
- **C:** `xsdb --list`/`--version`/`--out-dir`; distinct bad-XSD vs bad-template errors.
- **D:** negative-instance tests assert rejection; positive round-trips still pass.
- **E:** namespaced/imported fixtures round-trip; generated XML emits prefixes/`xmlns`.
- **Releases:** push `v0.2.0-rc` → per-OS tarballs + `SHA256SUMS`; extract on a
  clean machine, run with no env; `brew install`; `xsdb --version`.
