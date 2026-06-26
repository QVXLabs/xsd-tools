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
- Release packaging: publishing a GitHub release builds and attaches Linux
  packages — a Debian `.deb` and an RPM `.rpm` (via CPack, reusing the install
  rules) plus a single-file Flatpak bundle (`com.qvxlabs.xsd_tools`). Build them
  locally with `pkg/build-packages.sh` and `pkg/flatpak/build-flatpak.sh`.
- (planned) Prebuilt binary releases (Linux/macOS), Homebrew formula, Docker image.

### Changed
- Build migrated from SCons to CMake with multi-platform CI
  (Linux/macOS, x86_64/arm64; Windows experimental). Dependencies resolve via
  `find_package` → FetchContent (lua 5.1.5, tinyxml 2.6.2) or git submodule
  (boost, expat, googletest).
- Parser tree traversal (`ParseChildren`) refactored to functional sibling
  recursion via `Node::_eachChild`; behavior-preserving.
- Test scripts migrated to Python 3 (the round-trip harness and its driver/
  scaffold scripts now run under `python3`; generated python-sax/python-json
  emit Python 3). Closes #33.

### Fixed
- Same-named child elements of different types no longer collapse into a
  single generated struct/class. Structurally-distinct types that share a
  local name are disambiguated (`id`, `id1`, …) while identically-structured
  types still share one definition. The disambiguated type's element/key
  serializes under the suffixed name (e.g. `<id1>`), since the generated
  unmarshallers dispatch on element name without parent context — consistent
  with how non-identifier-safe names are already remapped. Closes #11.

## [0.1.2] - 2023-10-16

### Fixed
- Security vulnerabilities in test-case dependencies.
- A gcc compile error.

## [0.1.1] - 2021-11-22

### Added
- Conan support in the build system.
- Overriding the C/C++ compiler and compile flags via environment variables.

### Changed
- The embedded Lua script engine is inlined into the executable via the
  external `incbin` library.
- Cleaned up the `c-xml-expat` template — tests pass again and the gcc/clang
  compile warnings are fixed.

## [0.1.0] - 2021-09-11

### Added
- First public release: generates marshalling/unmarshalling code from XSD
  schemas via on-disk Lua templates (C, Python and Java output targets).
- `XSDTOOLS_DATA` environment variable to locate the template/data directory.

[Unreleased]: https://github.com/QVXLabs/xsd-tools/compare/0.1.2...HEAD
[0.1.2]: https://github.com/QVXLabs/xsd-tools/compare/v0.1.1...0.1.2
[0.1.1]: https://github.com/QVXLabs/xsd-tools/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/QVXLabs/xsd-tools/releases/tag/v0.1.0
