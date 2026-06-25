# Facet test fixtures

Schemas that exercise XSD restriction facets, used by the facet-enforcement
tests (Workstream D). Kept in a separate directory from `xsd-positive/` so the
generic round-trip suites don't auto-glob them (their generated test values must
respect the facets, and they also drive negative/invalid-instance cases).

Each schema drives two checks:

- **Smallest-type selection** — the generated field uses the narrowest type the
  bounds allow (e.g. `facet_uint8` → `uint8_t`, `facet_int16` → `int16_t`,
  `facet_range` 1..5 → `uint8_t`).
- **Runtime validation** — generated unmarshal rejects out-of-bounds / non-member
  / pattern-mismatched values (`facet_enum`, `facet_strlen`, `facet_range`,
  `facet_pattern`).

| Schema              | Facets                         | Checks                |
|---------------------|--------------------------------|-----------------------|
| `facet_uint8.xsd`   | integer 0..255                 | type `uint8_t`        |
| `facet_int16.xsd`   | integer -32768..32767          | type `int16_t`        |
| `facet_range.xsd`   | integer 1..5                   | type narrowing + range|
| `facet_enum.xsd`    | string enumeration             | membership            |
| `facet_strlen.xsd`  | string min/maxLength 2..4      | length                |
| `facet_pattern.xsd` | string pattern `[0-9]{5}`      | regex (P3)            |
