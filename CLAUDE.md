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
`schema` via the `LuaSchema`/`LuaContent`/`LuaType`/`LuaAttribute` stack; facet
callbacks are no-ops and `ProcessRestriction` collapses to the base without
walking facets, hence no validation today); plus `ElementExtracter`,
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
  prefixed (`pNode`), references `r`-prefixed (`rProcessor`), members `m_`,
  private helpers `_`-**suffixed** (`FindXSDElm_`), methods PascalCase
  (`ParseElement`), macros UPPER_CASE. CMake uses 2-space indent; Lua templates
  follow their existing local style. New files mirror their closest neighbor.
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
