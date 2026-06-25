# Changelog

All notable changes to xsd-tools are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/), and this project adheres to
[Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added
- Output targets completing the C/Python/Java × XML/JSON grid: **Python→JSON**
  (stdlib `json`), **C→JSON** (json-c), **Java→XML** (StAX). Each constructs
  types through overridable factory methods so consumers can inject subtypes.
- CLI: `--out-dir` (split multi-file `/* FILE: */` output into real files),
  `--list` (enumerate templates), `--version`, `-h/--help`, and distinct
  error/exit codes for missing XSD vs template vs parse error.
- Public library API: `XsdTools::Generate()` / `XsdTools::SplitMarkedFiles()`,
  installed header + exported `xsdtools::xsdtools` CMake target.
- Version derived from the git tag (`cmake/GitVersion.cmake`), reported by
  `xsdb --version`.
- (planned) Namespace awareness and `xs:import` support. Closes #10.
- (planned) Runtime enforcement of XSD restriction facets in generated code.
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
