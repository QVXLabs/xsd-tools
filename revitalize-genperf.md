# Generated-code performance & leanness — synthesized findings

Read-only audit of the code emitted by all 7 targets (schemas: elemT039 facets/
nesting, testA022 list, one attribute-bearing). Ranked by real impact. Nothing
changed yet.

## Tier 1 — High impact

### T1.1 O(n²) marshal in the C-XML targets (`_cstr_append`)
**c-xml-expat, c-xml-expat-dom.** Output is rebuilt fragment-by-fragment; each
`_cstr_append` does `strlen(whole-buffer)` + realloc-to-exact-length per call,
and each element emits 3–5 appends. Quadratic in output size on wide/deep docs.
**Fix:** growable buffer with amortized doubling + tracked length; append without
re-`strlen`; bake compile-time-known literal lengths (`_buffer_append("<test>",
6)`) and coalesce open-tag+`>`. (c-json uses json-c, python/java use native
builders — not affected.)

### T1.2 D2b width narrowing not applied — two C targets
**c-xml-expat** (`int32_t` for a 0..255 field), **c-xml-expat-dom** (shared
`xml_int` singleton + a separate heap malloc per int). **Fix:** `c_narrowTypes`
(task #12) — rewrite `content.type`/`attr.type` to the facet-narrowed sized type
before `uniqueTypes()`, so the narrowed type/singleton + helpers emit
consistently. c-json/java-json/java-xml already narrow.

### T1.3 java-json adapter de-bloat (YOUR priority: no unnecessary members)
**java-json.** `JSONObjectAdapter`/`JSONArrayAdapter` are fixed template text
emitting **every** type accessor (getBoolean/Int/Long/Double/Base64/Hex/Object/
List + put-overloads), all dead for schemas that don't use those types. **Fix:**
collect the leaf types actually referenced by the transformed schema and emit
only matching accessor blocks (mirror python-json, which dispatches type at
generation time and ships zero dead accessors).

### T1.4 Allocation-free / numeric enum validation (cross-cutting)
Enum guards do per-call allocation + stringify + linear scan:
- **java-xml** — `java.util.Arrays.asList("1",…).contains(String.valueOf(_v))`
  allocates a list every call (worst). 
- **java-json** — `String.valueOf(_v).equals("1")||…` allocates a String/call.
- **python (sax/json)** — `str(_v) in ("1",…)`: tuple const-folded (no alloc)
  but still stringifies a numeric field.
- **C (expat/dom)** — `strcmp` chain / `atoll` on an already-parsed int.
**Fix:** when the field is numeric, compare numerically (`_v == 1 || …` or
`switch`), dropping the stringify; for string enums in Java, a `static final
Set`. All become allocation-free / O(1).

### T1.5 python-sax O(N²) attribute unmarshal
**python-sax.** `for attribute in attributes:` then an unchained `if "x" ==
attribute:` per declared attr → N×M. **Fix:** `attributes` is already a dict —
emit `if "x" in attributes: self._x = conv(attributes["x"])`, dropping the loop
(also stops silently ignoring unknown attrs).

## Tier 2 — Medium

- **c-xml-expat-dom: value parsed 2–3×** (`atoi` in unmarshal, then `atoll`
  twice in the range guard). Validate the already-parsed int once.
- **c-xml-expat-dom: per-scalar heap malloc** (every int/string its own
  allocation). Store small scalars inline in the pointer-width `m_pData`, or
  arena-allocate.
- **c-xml-expat-dom: `xml_typeFactory` carries 3 dead fn-ptrs/type** (NULL-checked
  on the hot alloc path). Emit slots only for overridable types, or one
  "has-custom-allocator" flag.
- **c-json: marshal parent registry is O(n) linear scan + last-writer-wins**
  (also mis-attaches repeated names across subtrees). Thread the parent
  `json_object*` down the marshal chain (unmarshal already threads `parentId`).
- **Unconditional shared/import emission:** python `import base64` (never used) /
  `binascii` (only for hex); java `Hex.java` always emitted; the C runtime
  boilerplate (~110 lines) duplicated per schema with dead `_safeCStr`/`alloca`.
  Gate each on `uniqueTypes()` / actual use.

## Tier 3 — Low
- Empty `validate()` method + call emitted for facet-less elements (python-
  json/sax). Gate on presence of facets.
- java-xml: every `Element` allocates `_buf`+`_children` even for leaves; c-json:
  `_eid` written-not-read on leaves. Minor per-instance overhead.

## Correctness asides (NOT perf — verify & fix separately)
These are latent defects surfaced during the audit; suites pass only because
test values don't exercise them.
- **A. c-xml-expat `templates/c-xml-expat-sax/hex`** — malformed `hexTbl[256]`
  (a missing comma fuses two rows) → wrong hex decode for some bytes.
- **B. python-sax** — required attribute inits to `None` but marshal does
  unguarded `' x="' + self._x + '"'` → `TypeError` if unset.
- **C. java-json** — attribute `default="0"` silently dropped (no init).
- **D. c-json** — validation guard `if (0==_validate(..))` has no braces; the
  destroy/realloc after it always run (memory-safe today by luck).
- **E. c-xml-expat-dom** — `atoi` (unmarshal) vs `atoll` (validate) on the same
  data: inconsistent parse width.

## Already lean (negative findings)
python-sax/json, c-json, java-xml, c-xml-expat structs/classes carry only
schema-used members (no java-json-style accessor explosion). c-json + java-json +
java-xml narrowing is correct and single-parse. C integer validation (expat/json)
is allocation-free. List/hex paths are generally single-pass.
