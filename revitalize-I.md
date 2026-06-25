# Workstream I — TypeScript & C++11 output targets

Template-only (no C++ parser change). Each target copies its closest analogue
and adapts only format-specific bodies, reusing `/* FILE: */` markers,
`shared/schemaEx` (`c_safeNames`/`sortDependencies`/`uniqueTypes`),
`shared/facets` (`facets_checks`/`facets_sampleString`/`facets_narrowType`/
`facets_narrowestSigned`), `shared/trnsfrm_old`, the `*_type_info` dispatch
shape, the Allocator/Constructor/Destructor factory triple, and the
`validate(varName, facets)` contract (D). Lands **after E** (inherits the
optional `namespace` Lua fields; XML targets emit prefixes/`xmlns`, no extra
work for JSON) and **before H**. Order: **I0 → I1** (reuse the C expat/json-c
toolchain) then **I2** (new node/tsc toolchain).

## I0 — `cpp-xml-expat` (C++11, expat)
Copy `templates/c-xml-expat` (single-file template) → `templates/cpp-xml-expat`
+ `cpp-xml-expat-test`. Reuse its `c-xml-expat-sax/c_type_info` mapping and the
`cLit`/`sdbmCases`/`sdbm_hash` helpers, the prsElm/mrshllElm stack machinery,
the expat memory-suite (`expatMalloc_`/`Realloc_`/`Free_`), and the facet
validator emitter (lines 646-716). FILE markers stay: `xml_common.h`,
`xml_<schema>.hpp`, `xml_<schema>.cpp`.

Modernizations vs the C analogue:
- **Element structs → classes.** Each `xml_<name>` becomes a class with
  `std::string` (replacing `char*` + `CStrCpy_`/`destroy_charp_`),
  `std::vector<uint8_t>` for base64/hex, and members default-initialized — drop
  the manual `*delete*` statements and the union/`xml_typeUnion`; element
  storage becomes `std::unique_ptr<xml_<name>>` on the parse stack.
- **Factory triple → virtual methods / `std::function` table.** Keep the
  overridable Allocator/Constructor/Destructor contract but express it as a
  `xml_typeFactory` struct of `std::function`s (default = `make_unique` /
  no-op / reset); the `AllocateElement`/`ConstructElement`/`DestroyElement`
  macros collapse to a NULL-safe inline template helper.
- **expat static-thunk → member-handler bridge** (highest C++ risk): keep
  `expatStartElementHandler_`/`EndElementHandler_`/`CharacterDataHandler_` as
  free `static` functions (expat is a C ABI; member-function pointers cannot be
  passed). Each casts `pCtx`/`XML_GetUserData` back to `xml_marshaller*` and
  forwards to a member (`onStart`/`onEnd`/`onChars`). Replace the
  `__thread gtlsspMarshler` global with the user-data pointer threaded through
  the suite, or keep it but typed to the C++ marshaller.
- **Buffers:** replace `xml_buffer`/`buffer_append_` with `std::string`/
  `std::vector`; `xml_marshal_flush` returns a `std::string`.
- **Facets:** `validate_<name>_` becomes a member `bool validate() const`;
  keep the numeric/length/enum (sdbm-switch) checks verbatim, throw
  `std::runtime_error` instead of `abort()` so the `-test` driver can catch.

## I1 — `cpp-json-jsonc` (C++11, json-c)
Copy `templates/c-json-jsonc.template` (+ the whole `c-json-jsonc/` subtemplate
dir and its `c_type_info`) → `templates/cpp-json-jsonc.template` +
`cpp-json-jsonc/` + `-test`. FILE markers: `json_common.h`,
`json_<schema>.hpp`, `json_<schema>.cpp`. Same C→C++11 modernization as I0
(classes, `std::string`/`std::unique_ptr`/`std::vector`, `std::function`
factory triple, member `validate()`). json-c-specific:
- **`json_object*` RAII wrapper** — a small `struct JsonRef { json_object* p;
  ~JsonRef(){ if(p) json_object_put(p); } }` (move-only) so the `pRoot_`/`pElm`
  lifetimes from the C template's manual `json_object_put` become scope-bound.
  The per-element `build_<name>_` and `unmarshalElm_<name>_` keep their shape;
  attribute/content read still uses `json_object_object_get_ex`.
- Keep the base64/hex subtemplates and the `c_narrowCType` narrowing; the
  validate emitter (lines 374-543) ports as-is into a member returning
  `bool`/throwing.

## I2 — `ts-xml` (strict TypeScript, fast-xml-parser)
No direct analogue — model the tree like `python-sax`/`java-xml-stax.tmpl`
(text buffer + ordered children + an overridable factory dispatching local
name → element). Copy the *structure* of `java-xml-stax.tmpl` (its `Element`
base, `Factory.create(localName)`, `Document` marshal/unmarshal, per-type FILE
blocks) and translate to TS. New: `templates/ts-xml/` (a `ts_type_info`
subtemplate mapping XSD→`string`/`number`/`boolean`/`Uint8Array`, mirroring
`java_type_info`), `templates/ts-xml.tmpl`, `templates/ts-xml.tmpl-test`. FILE
markers emit one `.ts` per type plus `index.ts`.

Design:
- **`interface`/`class` per type.** An abstract `Element` base
  (`localName()`, `children: Element[]`, `marshalAttributes`/`marshalContent`,
  `readAttributes`/`readContent`, `validate()`), one `class <Type> extends
  Element` per complex type, optional `interface` for the plain-data shape.
- **Parse/build via fast-xml-parser** (no native deps): unmarshal with
  `new XMLParser({ ignoreAttributes:false, attributeNamePrefix:'@_',
  preserveOrder:true })` → walk the ordered node array through `Factory.create`
  (mirror StAX's pull loop); marshal with `new XMLBuilder({...same opts})` from
  a plain object the classes build.
- **Factory + facet validate.** Overridable `Factory` like Java's; `validate()`
  throws on a facet miss (range/length/enum). Reuse `facets_checks`; enums via
  a module-level `Set`/`ReadonlySet`.
- **Strict TS:** all fields typed (no implicit `any`), `strictNullChecks`-clean
  (optional fields `?:` / `| undefined`); must pass `tsc --strict`.

## I3 — Test wiring
Add three `TARGETS` rows + the matching `ENABLED` entries in
`gen_roundtrip_tests.py`, three `GENERATED_ROUNDTRIP_SOURCES` + their stable
empty-TU names in `test/CMakeLists.txt`, and helpers/decls in
`roundtrip_util.{hpp,cpp}`. Run via the gtest binary
(`build/test/xsdb-test --gtest_filter=…`), never ctest.

```
("CppXmlExpatRoundtrip","cppXmlRoundtrip",["c++"],"roundtrip_cpp_xml.cpp"),
("CppJsonRoundtrip",    "cppJsonRoundtrip",["c++"],"roundtrip_cpp_json.cpp"),
("TsXmlRoundtrip",      "tsXmlRoundtrip", ["node","tsc"],"roundtrip_ts_xml.cpp"),
```

- **`cppXmlRoundtrip`/`cppJsonRoundtrip`:** generalize `cRoundtripImpl` — add a
  `compiler`+`std` param (or a sibling `cppRoundtripImpl`) that swaps
  `C_COMPILER`/`-std=c11` for `CXX_COMPILER`/`-std=c++11`, splits the multi-file
  binding (`xml_`/`json_` prefix, `.cpp`/`.hpp`), and reuses `LIBB64_*` +
  `EXPAT_*`/`JSONC_*` include/link defs already in CMake. Add
  `CXX_COMPILER="${CMAKE_CXX_COMPILER}"` to `target_compile_definitions`. No new
  find-or-fetch (expat/json-c blocks already present); just add the C++
  compiler define and the two empty-TU source names.
- **`tsXmlRoundtrip`:** mirror `javaRoundtripImpl`'s pom-cache pattern. New
  pinned `test/ts-xml/package.json` (fast-xml-parser + typescript at fixed
  versions — the `java-xml-stax.tmpl/pom.xml` precedent) + a `tsconfig.json`
  (`strict:true`). Helper: tempdir → copy package.json/tsconfig → split binding
  `.ts` files + a `RunTest.ts` driver → `npm ci` (or cache `node_modules`/an
  `npm`-prefix like `~/.m2`) → `tsc` → `node RunTest.js`; assert clean exit and
  no `false` in output (same contract as Java). Add
  `TS_PACKAGE_JSON`/`TS_TSCONFIG` compile defs.
- **KNOWN_FAILING:** gate genuinely unsupported XSD/format combos per suite
  (e.g. an attribute/element name collision a target can't disambiguate) with
  the `DISABLED_` prefix — empty otherwise.
- **Negative (D4) tests:** add `cppXmlRejects`/`cppJsonRejects` (throw → exit
  nonzero, mirroring `cExpatRejects`/`cJsonRejectsRange`) and `tsXmlRejects`
  (catch the validate throw → exit 0, mirroring `javaXmlRejects`).

## I4 — Toolchain
- **C++11 (I0/I1):** reuses the existing C++ compiler + already-required
  expat/json-c. Only new CMake bit is the `CXX_COMPILER` define; the test
  already pins `-std=c++11` at compile. No README requirement change beyond a
  note.
- **TS (I2):** needs `node` + `tsc` (typescript) + `fast-xml-parser`. Add to
  README Requirements; `gen_roundtrip_tests.py` lists `["node","tsc"]` so
  `gates_for` emits `GTEST_SKIP() << "<tool> not available"` — mirrors the
  Python/Java `programAvailable` probes. Tests must skip cleanly, never fail,
  when node/tsc are absent.

## I5 — Docs
- README "Output targets" table → nine rows: add `cpp-xml-expat` (C++11/XML/
  expat), `cpp-json-jsonc` (C++11/JSON/json-c), `ts-xml` (TypeScript/XML/
  fast-xml-parser). Drop the "Six built-in output targets" line (now nine) and
  the "C / Python / Java" languages sentence (add C++ and TypeScript).
- CHANGELOG `Added`: the three new targets + node/tsc requirement note.

## Risks
- **I2 ts-xml is highest-risk** — no direct analogue and a brand-new toolchain.
  fast-xml-parser's `preserveOrder` shape and attribute prefixing differ from
  StAX's pull model; the parse-walk and round-trip equality (attribute order,
  whitespace, self-closing tags) are the failure surface. Validate on **one
  nested XSD first** before fanning out across the corpus.
- **I0 expat static-callback → member bridge** — member-function pointers can't
  cross expat's C ABI; the thunk-via-user-data pattern (or a typed TLS
  marshaller) is load-bearing. Get the bridge right on a small schema first.
- **I1 json-c RAII wrapping** — double-`put`/leak risk when wrapping
  `json_object*` lifetimes; the move-only `JsonRef` and unchanged
  `build_`/`unmarshalElm_` shapes contain it. Validate alloc/destroy balance via
  the `-test` driver's override-factory counters (mirrors the C json `-test`).

## Order of operations
1. **I0** — copy `c-xml-expat`, modernize incrementally: first get the type
   classes + buffers compiling on a leaf-only XSD, then the expat member bridge,
   then factory triple, then port the facet validator. Verify:
   `xsdb-test --gtest_filter='CppXmlExpatRoundtrip*'` over a few corpus XSDs
   (`testA002`, a nested one like `testA021`/`testA022`), then the `Reject`
   negative cases.
2. **I1** — copy `c-json-jsonc` + subtemplate dir; reuse I0's class/factory
   patterns; add the `JsonRef` RAII wrapper. Verify `CppJsonRoundtrip*` +
   factory-counter asserts in the `-test` driver.
3. Run the **simplification pass** (reuse/simplify/efficiency/altitude) on I0+I1
   with the suite green before starting I2.
4. **I2** — build `Element`/`Factory`/`Document` + `ts_type_info`, prove
   round-trip on one nested XSD with fast-xml-parser, then fan out; add
   `validate()` + negative tests. Verify `TsXmlRoundtrip*` (skips cleanly
   without node/tsc).
5. **I5** docs + CHANGELOG land with the feature, then a final simplification
   pass. Wire each target into `gen_roundtrip_tests.py`/CMake as it's built so
   the corpus glob auto-generates its per-XSD cases.
