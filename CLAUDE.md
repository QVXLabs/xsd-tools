# Project instructions

## Code comments

Comments must be concise. One short line is the default. Reserve multi-line
comments for cases where the *why* genuinely needs more than a sentence (a
non-obvious invariant, a workaround for a specific bug, a measured tradeoff).
Don't narrate the *what* — well-named identifiers do that already.

## Architecture

xsd-tools generates marshalling code from XSD schemas. A C++ core parses the XSD
into a model; the model is exposed to a Lua template engine; on-disk Lua
**templates** emit the target-language code. **Adding an output target is
template-only** (no C++ change).

**Pipeline** (`src/xsdtools.cpp` `XsdTools::Generate`):
1. **Parse** — `XSD::Parser::Parse` (`src/XSDParser/Parser.cpp`) loads the XML via
   TinyXML into a `Node` tree rooted at `Elements::Schema`.
2. **Process** — `root->ParseElement(LuaProcessor)` walks the tree by
   visitor/double-dispatch; `LuaProcessor`+`LuaAdapter` imperatively build a global
   Lua `schema` table.
3. **Generate** — `LuaScriptAdapter` loads the embedded engine bytecode and runs
   the chosen template; the engine reads `schema` and writes output through an
   `io.write` hook captured into a `std::ostream`.

**Two Lua layers (don't conflate):** the **engine** is precompiled
`luascript.luac` bytecode (`src/TemplateEngine/*.lua` → luac → embedded into
`src/resource.cpp` via incbin); the **templates** are runtime-interpreted files
under `templates/` that the engine reads off disk.

**Parser model** (`src/XSDParser/`): `Node` (`Elements/Node.hpp`) wraps a
`TiXmlElement`+`Parser&`; navigates via `FirstChild`/`NextSibling`;
`_ConstructNode` is a case-insensitive tag→subclass factory; one `Node` subclass
per XSD construct in `Elements/*.cpp`. **Visitor:** `BaseProcessor`
(`ProcessorBase.hpp`) declares ~30 `ProcessXxx` callbacks; `Node::ParseElement(p)`
calls the matching `p.ProcessXxx(this)`, `ParseChildren(p)` recurses — compile-time
dispatch, no RTTI. **Types:** `TypesDB` (builtin XSD primitives, keyed by **local
name only — no namespace**) + a `Types::BaseType` hierarchy; refs resolve
structurally in `Node::Type_`/`FindXSDElm_`, scoped to the current doc +
`xs:include`s.

**Processors** (`src/Processors/`): `LuaProcessorBase` (BaseProcessor impl, most
callbacks no-op, holds a `LuaAdapter`); `LuaProcessor` (the real one — builds
`schema` via the `LuaSchema`/`LuaContent`/`LuaType`/`LuaAttribute` stack; the
facet callbacks record onto `facets_` and `ProcessRestriction` calls
`walkFacets_`, so facets propagate to the leaf type and are enforced in the
generated unmarshallers); plus `ElementExtracter`,
`SimpleTypeExtracter`, `RestrictionVerify` passes.

**Template engine** (`src/TemplateEngine/*.lua`): literal text with embedded
`[@lua … ]` blocks (backslash-escape to emit literally); `parser.lua` runs each
block in a sandbox (`safeEnv.lua`, `setfenv`) and substitutes its return value.
Template globals: `schema`, `__CMD_ARGS__`, `__SCHEMA_NAME__`, `sdbm_hash`,
`include`, and table/string helpers (`map`/`merge`/`filter`/`find`, `split`).
`include 'shared/x'` loads `templates/shared/x` (e.g. `schemaEx` adds
`sortDependencies()`/`c_safeNames()`/`uniqueTypes()`; `trnsfrm_old` normalizes).
Multi-file output uses `/* FILE: name */` markers.

## Conventions

- **Match the existing spacing and naming of the file/area you edit; don't
  reformat surrounding code.** C++ uses **tab indentation**; pointers are `p`-
  prefixed (`pNode`), references `r`-prefixed (`rProcessor`), class members and
  private helpers are `_`-**suffixed** (`facets_`, `FindXSDElm_`) — **not** the
  old `m_`/leading-`_` forms — methods PascalCase (`ParseElement`), macros
  UPPER_CASE. CMake uses 2-space indent; Lua templates follow their existing
  local style. New files mirror their closest neighbor.
- **Prefer `static` (file-local) over anonymous namespaces** for
  internal-linkage helpers; use a function-local type where a helper struct is
  only needed inside one function.
- **Parse lazily — only what's needed; build no second full model.** The
  processing pass expands only the types reachable from the root element, on
  demand: referenced groups/attributeGroups are parsed only when referenced,
  `maxOccurs=0` elements are skipped, and types are expanded on first use in
  `parseType_`. Don't add eager whole-schema walks or a parallel in-memory
  model of the schema. (Cyclic schemas are rejected via the active-path guard
  in `parseType_` rather than expanded — the model has no reference form.)
- **Output targets are template-only** — copy the closest existing target and
  adapt only format-specific bodies; reuse `*_type_info` dispatch tables, shared
  helpers, FILE markers, and the `-test` driver.
- **XSD-tree traversal uses functional-style recursion** over
  `FirstChild`/`NextSibling` via the shared sibling helper — not imperative
  for/do-while sibling loops.
- **Run tests via the gtest binary** (`build/test/xsdb-test --gtest_filter=…`),
  not ctest.
- **After each feature pass, run a simplification pass** (reuse / simplify /
  efficiency / altitude; behavior-preserving, suite green) before moving on.
- **Build:** CMake; deps `find_package`-first then FetchContent
  (lua/tinyxml/json-c via the `fetch_third_party()` helper) or submodule
  (boost/expat/gtest). The bundled Lua is LNUM-patched 5.1.5 and its `luac` must
  match the linked `liblua` (force the bundled path in CI).

## Release process

The base version is **`VERSION.txt`** (single source of truth).
`cmake/Version.cmake` reads it for `project(VERSION ...)` and stamps
`src/version.h`; git decorates non-release dev builds with a suffix. The
`release` workflow (`.github/workflows/release.yml`) fires when a GitHub
Release is **published**, takes the version from the release **tag** (leading
`v` stripped), builds the `.deb`/`.rpm` (CPack, via `pkg/build-packages.sh`)
and `.flatpak` (`pkg/flatpak/build-flatpak.sh`), and attaches them to the
release.

To cut release `X.Y.Z`:
1. **Bump the version** — set `VERSION.txt` to `X.Y.Z`.
2. **Update docs** — rename the CHANGELOG `## [Unreleased]` block to
   `## [X.Y.Z] - YYYY-MM-DD` (add a fresh empty `[Unreleased]` above it) and fix
   the compare links at the file's bottom. The README uses `<ver>` placeholders,
   so it needs no per-version edit. Open a PR and merge to `master`.
3. **Tag & release** — after merge, publish a GitHub Release on `master` with
   tag `vX.Y.Z` (must match `VERSION.txt`, so the stamped binary version and the
   package filenames agree). Publishing triggers the `release` workflow.
4. **Verify** — confirm the workflow's `deb-rpm` and `flatpak` jobs ran and the
   artifacts are attached to the release.

`workflow_dispatch` on the release workflow is a dry run: packages upload as
workflow artifacts instead of attaching to a release.
