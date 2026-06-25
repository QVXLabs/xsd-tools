# Workstreams G & H — Implementation Plan

Tracker confirmed live via `gh` against **`Ardy123/xsd-tools`** (and the
`QVXLabs/xsd-tools` fork — identical issue set). Open: **#33**, **#11**, **#10**,
**#4**. `#10` (namespaces) is closed by Workstream E — no extra work here; it
gets a `Closes #10` line in E's PR. The qvxlabs blog repo is accessible
(`~/code/qvxlabs/web/QVXLabs`); `zopfli.html` and `blog.html` were read and are
mirrored below.

---

## G — Close open issues

Each fix lands a corpus fixture/test plus a `Closes #N` line in the PR body and
the `CHANGELOG.md` `Unreleased` entry.

### G1 · #33 — Migrate test scripts to Python 3 (land early, with Step 0/F)

**Root cause.** Legacy Py2-era driver scripts predate the gtest +
`gen_roundtrip_tests.py` (Py3) harness and are dead weight. Confirmed by grep:
**nothing** in `test/CMakeLists.txt`, `test/gen_roundtrip_tests.py`, or
`test/roundtrip_util.{hpp,cpp}` references them.

**Files to remove** (orphaned, Py2 idioms): `test/test_framework.py`,
`test/conio.py`, `test/xsdparse-test.py`, `test/c-xml-expat-test.py`,
`test/c-xml-expat-dom-test.py`, `test/python-sax-test.py`,
`test/java-json.org-test.py`. The live Py3 harness — `test/gen_roundtrip_tests.py`,
`test/run_parallel.py`, plus the C++ drivers (`generateTests.cpp`,
`parserTests.cpp`, `resourceTests.cpp`, `roundtrip_util.*`) — already supersedes
them: per-target round-trips are generated from the corpus glob and run through
the gtest binary, and `xsdparse` goldens cover the parser.

**Action.** Delete the seven scripts; grep the tree once more to confirm no
`pkg/`, CI workflow, or README reference survives (README "Run tests" already
points at the gtest binary per the cross-cutting doc rule). No port needed — the
behavior already exists in Py3/gtest. CHANGELOG `Changed`: "Removed dead Py2 test
scripts; the test harness is gtest + Python 3."

**Test.** No new fixture; the gate is the full `xsdb-test` run staying green
after removal. `Closes #33`.

### G2 · #11 — Same-named children of different types → duplicate structs (with B/D)

**Root cause (codegen naming collision, confirmed in the bridge).** The Lua model
keys a type's children by **element/type name string only**. `LuaType`
(`src/Processors/LuaAdapter.cpp:126`) does `lua_setfield(... rTypeName)`, and
`schema.types` / `node.dependents` / `schema.uniqueTypes`
(`templates/shared/schemaEx:67`) are tables keyed by that name. Two child
elements named the same but with **different types** therefore land on one table
key — one silently overwrites the other, and the C/struct emitters
(`*_type_info` dispatch) generate a single struct name for two distinct shapes.
`c_safeNames`/`renameNames` (`schemaEx:21-61`) only de-collides
NCName-illegal/duplicate *spellings*, not same-name-different-type identity.

**Fix depth — bridge first, then names.** The name layer cannot disambiguate
what the model collapsed: the Lua object carries no per-child type identity to
key on. Add identity in the bridge: in `LuaAdapter.cpp` carry the resolved
**type name** (and, post-E, namespace) onto each content/dependent entry so two
same-named children remain distinct table entries (e.g. compose a stable key from
`name`+typeident, or store children as a list with `{name, type}` rather than a
name-keyed map). Then teach `schemaEx`'s `uniqueTypes`/`renameNames` to emit one
generated struct **per (name,type)** pair, and have each target's `*_type_info`
naming consume the disambiguated identity. Keep local-name keys where templates
depend on them to contain churn (mirrors E3's principle).

**Repro XSD + test.** Add `test/xsd-positive/testG011_dupname.xsd`: a complex type
with two child elements both named `value`, one `xs:string` and one a local
complex type (or `xs:int`). Pre-fix this generates two identically-named structs
(C won't compile / Java/Python collide). The corpus glob auto-generates
round-trip cases across all targets; assert a clean round-trip. Update
`test/xsdparse/*.out` goldens for the new file. `Closes #11`.

### G3 · #4 — Emit XSD documentation/annotation in output (with B)

**Root cause (parsed but not bridged).** `Documentation::DocumentationStr()`
(`src/XSDParser/Elements/Documentation.cpp:65`) already extracts the text, and
`Annotation`/`Documentation` are walked — but `LuaProcessorBase::Process{Annotation,
Documentation}` (`LuaProcessorBase.cpp:224-232`) only recurse; `LuaProcessor`
does **not** override them, so the text never reaches the Lua `schema` table.

**Fix (bridge + template-only emit).** Override `ProcessAnnotation`/
`ProcessDocumentation` in `LuaProcessor` to capture `DocumentationStr()` and, via
`LuaAdapter`, attach an optional `documentation` field to the current type/element
Lua object (omit when empty — avoid golden churn, same discipline as D's
`facets`). Then each target emits language-appropriate comments/docstrings
(template-only, per-target): C/C++ `/* */` above the struct/class, Python a
`"""docstring"""`, Java a `/** */` Javadoc, TypeScript a `/** */` JSDoc. Add a
shared emitter in `templates/shared/` so all targets (incl. B and I) share one
escaping/wrapping path.

**Test.** Add `test/xsd-positive/testG004_annotated.xsd` with an
`xs:annotation/xs:documentation` on a type; update `xsdparse/*.out` goldens to
show the captured text; round-trip still passes (comments are inert). `Closes #4`.

---

## H — Announcement blog post (qvxlabs repo, LAST)

**Repo:** `~/code/qvxlabs/web/QVXLabs` (static HTML, GAE). **Commit there, never
in xsd-tools.**

**File to create:** `static/posts/xsd-tools.html`, mirroring `zopfli.html`
**exactly**: `<!DOCTYPE html>`/`<html lang="en">`; `<head>` with `<meta charset>`,
viewport, `<title>… — QVXLabs</title>`, `<meta name="description">`, and
`<link rel="canonical" href="https://www.qvxlabs.com/blog/xsd-tools-<slug>">`
plus the `/css/qvxlabs.css` stylesheet; the shared `<header class="site-header">`
nav block verbatim; `<main><article class="container article">` with `<h1>`, a
`<p class="meta">… 2026 · QVXLabs · <a href="https://github.com/QVXLabs/xsd-tools">
github.com/QVXLabs/xsd-tools</a></p>`, `<h2>` sections, an "Honest caveats"
`<div class="note warn">`, a "Getting it" `<a class="btn">`, and the shared
`<footer class="site-footer">`.

**Index entry:** add one `<li>` to the `<ul class="post-list">` in
`static/blog.html` (mirror the existing zopfli `<li>`: `<span class="date">`,
`<h3><a href="/blog/xsd-tools-<slug>">…</a></h3>`, `<p class="muted">` blurb).

**Tone = zopfli post.** Honest "revived & maintained", NOT an over-claimed
"modern" splash. Lead with what xsd-tools is and why it lapsed; then what shipped
this release.

**Claim only what shipped:** installable (release binaries + Homebrew + Docker/
GHCR); namespace + `xs:import` aware (E); validates output via facet enforcement
(D); the **full C / Python / Java × XML/JSON grid plus C++11 and TypeScript
targets** (B + I); **all open issues closed (#4/#10/#11/#33)**.

**Candid caveats** (the "note warn" block): still on **EOL Lua 5.1 + unmaintained
TinyXML**; existing Python/Java target idioms are dated; no Go/Rust targets; the
C `pattern` facet is deferred; identity constraints / `xsi:type` / `nillable` /
`redefine` unsupported. State these plainly rather than hide them — the zopfli
post's credibility comes from the caveats.

**Guardrails.** Claim only what actually shipped; cross-check the post against the
final CHANGELOG before publish. **Publish/deploy ONLY after the release is live
and with explicit user sign-off** (outward-facing). Commit to the qvxlabs repo.

---

## Sequencing

1. **G1 (#33)** early — bundled with Step 0 / Workstream F baseline cleanup
   (pure deletion, green suite is the gate).
2. **G2 (#11)** and **G3 (#4)** ride along with template work: #11 with B and D's
   bridge changes (it needs a `LuaAdapter` identity change); #4 with B (small
   bridge + per-target template emit). Both inherit by I's new targets.
3. **H absolute last** — drafted after Workstream I (so it can claim the full
   nine-target grid), published only after the release is live **and** explicit
   user sign-off.
