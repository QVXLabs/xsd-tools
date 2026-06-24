# Changelog

All notable changes to xsd-tools are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/), and this project adheres to
[Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added
- (planned) Output targets to complete the C/Python/Java × XML/JSON grid:
  Python→JSON, C→JSON (json-c), Java→XML (StAX).
- (planned) Namespace awareness and `xs:import` support. Closes #10.
- (planned) Runtime enforcement of XSD restriction facets in generated code.
- (planned) Overridable type factory methods in all generated targets.
- (planned) CLI: `--out-dir` (split multi-file output), `--list`, `--version`.
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
