# Changelog

All notable changes to xsd-tools are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/), and this project adheres to
[Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added
- Output targets completing the C/Python/Java × XML/JSON grid: **Python→JSON**
  (stdlib `json`), **C→JSON** (json-c), **Java→XML** (StAX). Each constructs
  types through overridable factory methods so consumers can inject subtypes.
- Three more language targets (nine total): **C++11→XML** (`cpp-xml-expat`),
  **C++11→JSON** (`cpp-json-jsonc`) — RAII, classes, overridable factory
  methods — and **TypeScript→XML** (`ts-xml`, fast-xml-parser). All inherit
  facet validation and namespace emission, and are round-trip tested.
- CLI: `--out-dir` (split multi-file `/* FILE: */` output into real files),
  `--list` (enumerate templates), `--version`, `-h/--help`, and distinct
  error/exit codes for missing XSD vs template vs parse error.
- Public library API: `XsdTools::Generate()` / `XsdTools::SplitMarkedFiles()`,
  installed header + exported `xsdtools::xsdtools` CMake target.
- Version derived from the git tag (`cmake/GitVersion.cmake`), reported by
  `xsdb --version`.
- Generated unmarshallers now enforce XSD restriction facets (range, length,
  enumeration) on element content **and** attributes, across all seven targets.
  Violations are rejected per language: C returns an error / skips / aborts,
  Python raises `ValueError`, Java throws `IllegalArgumentException`.
- Smallest-type selection: a facet-bounded integer field narrows to the
  smallest type that holds its range — C `uint8_t`…`int64_t`, Java
  `Byte`/`Short`/`Integer`/`Long`; Python is unaffected (arbitrary precision).
- Namespace awareness: schemas with a `targetNamespace` resolve by namespace
  URI (builtins detected by URI, not the `xs` prefix), the XML targets emit the
  correct `xmlns`/prefix declarations, and `xs:import` resolves types across
  namespaces (contrast `xs:include`'s same-namespace merge). Cyclic schemas are
  rejected instead of overflowing the stack. Closes #10.
- `xs:documentation` (annotation) text is emitted as a comment at the matching
  location in generated code — above the type/struct/class/interface for an
  element or complex type, and above the field for a scalar element or an
  attribute — across all targets (C-style `/* */`, Python `#`). Closes #4.
- (planned) Prebuilt binary releases (Linux/macOS), Homebrew formula, Docker image.

### Changed
- Build migrated from SCons to CMake with multi-platform CI
  (Linux/macOS, x86_64/arm64; Windows experimental). Dependencies resolve via
  `find_package` → FetchContent (lua 5.1.5, tinyxml 2.6.2) or git submodule
  (boost, expat, googletest).
- Parser tree traversal (`ParseChildren`) refactored to functional sibling
  recursion via `Node::_eachChild`; behavior-preserving.

### Fixed
- (planned) Duplicate generated structs for same-named child elements of
  different types. Closes #11.

### Removed
- (planned) Legacy Python 2-era test scripts, superseded by the gtest +
  Python 3 round-trip harness. Closes #33.

[Unreleased]: https://github.com/QVXLabs/xsd-tools/commits/master
