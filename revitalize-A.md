# Revitalize Workstream A — Release Automation, Packaging & Distribution Docs

Branch: `revitalize`. Scope: A2 (release workflow), A3 (packages), A4 (docs).
Planning only. Each item lists exact files, sequencing, effort, and the
decisions to confirm before execution.

## Current state (verified by reading the tree)

- **CI** `.github/workflows/build-and-test.yml`: matrix `ubuntu-latest`
  (x86_64), `ubuntu-24.04-arm` (arm64, experimental), `macos-15` (arm64,
  experimental), `macos-15-large` (x86_64) + experimental Windows
  (`windows-latest`, `windows-11-arm`). Configure line:
  `cmake -S . -B build -G "Ninja Multi-Config"
  -DCMAKE_DISABLE_FIND_PACKAGE_Lua=ON`. Java 21 (temurin). Linux installs
  `ninja-build maven` and removes stray `liblua5.*`/`lua5.*` so the bundled
  Lua 5.1 + matching `luac` is used. macOS `brew install ninja maven`.
  **Produces no artifacts** — only `ctest`.
- **Build/install** root `CMakeLists.txt`: `project(... VERSION ${XSD_VERSION})`
  from `cmake/GitVersion.cmake` (git `describe --tags --match v*`, override
  `-DXSD_VERSION=`). `install(TARGETS xsdb RUNTIME DESTINATION bin)` +
  `install(DIRECTORY templates/ DESTINATION share/xsdtools/templates)`. A
  library export already exists: installs `src/xsdtools.hpp`, exports
  `xsdtools-targets`, alias `xsdtools::xsdtools`. `GNUInstallDirs` in use, so
  `bin/`+`share/`+`lib/` land at the prefix.
- **Relocatable templates**: `src/resource.cpp` `GetTemplatePath` already has an
  exe-relative fallback (`<exe>/../share/xsdtools/templates/<name>` then
  `<exe>/templates/<name>`). A tarball with `bin/xsdb` + `share/xsdtools/
  templates/` finds templates with **no env**. `xsdb --version` prints
  `XSDTOOLS_VERSION` (= `XSD_VERSION_FULL`).
- **json-c** is fetched in `test/CMakeLists.txt` (FetchContent, json-c 0.17),
  needed **only** by the C→JSON round-trip test compile, **not** by the runtime
  `xsdb` binary. So the release build (`-DBUILD_TESTING=OFF`) does not need it.
- **Packaging**: `pkg/rpm/SPECS/xsd-tools.spec` hardcodes `version 0.1.0`, uses
  **system** lib deps (tinyxml/boost/lua-devel). `build-conan.sh` is the stale
  Conan driver. `README.md` install section references Conan + `ctest` (stale),
  has a dead "Wiki" link (line 29: `_Look at the Wiki section..._`), one badge
  (build-and-test). No `examples/` dir. No `.github/actions/`.

### Blocking finding for Windows-in-release

`src/resource.cpp` guards `dirent.h` behind `#if !defined(_WIN32)` (lines 27-35)
but `Resource::ListTemplates()` (lines 199-219) calls `opendir`/`DIR`/`readdir`/
`closedir` **unconditionally**. This is library code linked into `xsdb`, so the
Windows build of `xsdb` itself (not just `--list`) will fail to compile/link
once exercised for a real artifact. The experimental Windows CI leg may be
passing only because of MSVC quirks or because it is `continue-on-error`.
**Windows release is therefore gated on first guarding `ListTemplates` /
`TemplatesDir` for `_WIN32`** (FindFirstFile/FindNextFile or `std::filesystem`).
=> **Recommendation: Windows-in-release = NO for the first release.** Ship
Unix-first; track the Windows guard as a separate prerequisite task.

---

## A2 — Release workflow

### Goal
Tag push `v*` -> build relocatable tarballs per OS/arch -> validate by
extracting on a clean runner and running `bin/xsdb --version` + a generate ->
publish a GitHub Release with tarballs + `SHA256SUMS`.

### A2.1 — Composite build action (shared by CI and release)

**Create** `.github/actions/build-xsdb/action.yml` — a composite action that
factors out everything the two workflows share: deps setup, the stray-lua
handling, configure, and build. Inputs let release pass a version and disable
tests.

Inputs:
- `version` (default empty) -> forwarded as `-DXSD_VERSION=${{ inputs.version }}`
  only when non-empty (release passes the tag minus `v`).
- `build-testing` (default `ON`) -> `-DBUILD_TESTING=${{ inputs.build-testing }}`
  (release passes `OFF` so json-c is never fetched and tests don't build).
- `config` (default `Release`).
- `extra-cmake-args` (default empty) -> appended verbatim (release passes the
  static-link / deployment-target flags, see A2.3).

Steps (carry forward **all** current CI fixes):
1. `actions/setup-java@v4` temurin 21. (Kept here so both callers get it; if a
   caller needs Java only for the test leg, that's fine — it's cheap.)
2. Install build tools, branching on `$RUNNER_OS` exactly as today:
   - Linux: `apt-get update`; `apt-get install -y ninja-build maven`; print
     then `apt-get remove -y 'liblua5.*' 'lua5.*' || true` (the stray-lua fix).
   - macOS: `brew install ninja maven || true`.
3. Configure: `cmake -S . -B build -G "Ninja Multi-Config"
   -DCMAKE_DISABLE_FIND_PACKAGE_Lua=ON -DBUILD_TESTING=${{ inputs.build-testing }}`
   `${{ inputs.version && format('-DXSD_VERSION={0}', inputs.version) }}`
   `${{ inputs.extra-cmake-args }}`.
4. Build: `cmake --build build --config ${{ inputs.config }} -j`.

Notes:
- `actions/checkout` with `submodules: recursive` and `setup-python` stay in the
  **callers**, not the action (checkout must run before the action so the action
  files exist; python is test-only). Document this in the action's README block.
- json-c is *not* referenced anywhere in the action — it lives in
  `test/CMakeLists.txt` and is gated by `BUILD_TESTING`. The action carries it
  forward implicitly: with `build-testing: ON` (CI) it is fetched, with `OFF`
  (release) it is skipped. Call this out in a comment so it isn't "lost".

**Edit** `.github/workflows/build-and-test.yml`: replace the per-job "Install
build tools" + "Configure + build + test" steps in the `unix` job with:
`- uses: ./.github/actions/build-xsdb` (after checkout/python/java) then a
  separate `- name: Test` step running `ctest --test-dir build -C Release
  --output-on-failure`. Keep Java in the caller or let the action own it (pick
one; recommend action owns it, remove the caller's setup-java to avoid double
setup). Leave the experimental `windows` job as-is for now (it can't use the
composite action's apt/brew branch cleanly, and Windows is out of release
scope).

### A2.2 — Release workflow

**Create** `.github/workflows/release.yml`.

```
on:
  push:
    tags: ['v*']
permissions:
  contents: write          # needed to create the Release
```

Job `build` (matrix, Unix-first):
```
matrix:
  include:
    - { os: ubuntu-latest,    arch: x86_64 }
    - { os: ubuntu-24.04-arm, arch: arm64  }
    - { os: macos-15,         arch: arm64  }
    - { os: macos-15-large,   arch: x86_64 }
```
(Decision: keep arm64/macos as non-experimental for release, or mark
`continue-on-error` so one flaky arch doesn't block the whole release. Default
recommendation: NOT continue-on-error for release — a missing arch tarball
should fail loudly.)

Steps:
1. `actions/checkout@v4` `submodules: recursive`.
2. `actions/setup-python@v5` 3.11 (needed only if a generate-validation step
   touches python; the validate step below uses C/Java targets, so python is
   optional — include it for safety, it's cheap).
3. Derive version: `VER=${GITHUB_REF_NAME#v}` into `$GITHUB_ENV`.
4. `- uses: ./.github/actions/build-xsdb` with
   `version: ${{ env.VER }}`, `build-testing: OFF`,
   `extra-cmake-args:` per-OS static/deployment flags (A2.3).
5. Install to a staging prefix:
   `cmake --install build --config Release --prefix "$PWD/stage"`.
   Yields `stage/bin/xsdb` + `stage/share/xsdtools/templates/` (+ `lib/`,
   `include/` — keep them; they make the tarball usable as an SDK, small cost).
6. **Validate the relocatable layout on the build runner before packaging**:
   from a *different* working dir (e.g. `cd /tmp`), run
   `"$PWD/stage/bin/xsdb" --version` and a generate against a repo XSD copied
   into /tmp, e.g.
   `"$GITHUB_WORKSPACE/stage/bin/xsdb" c-xml-expat
   "$GITHUB_WORKSPACE/test/xsd-positive/testA001.xsd" > /tmp/out.c` and assert
   the output is non-empty. Running from `/tmp` proves templates are found via
   the exe-relative fallback, not the source-tree `./templates` convenience.
7. Package: `TAR=xsdb-${VER}-<os>-<arch>.tar.gz` where `<os>` is `linux`/`macos`
   from `$RUNNER_OS` and `<arch>` from the matrix. `tar czf "$TAR" -C stage .`.
   Then `shasum -a 256 "$TAR" > "$TAR.sha256"` (macOS) / `sha256sum`
   (Linux) — use `shasum -a 256` on both (present on Linux too) for uniformity.
8. `actions/upload-artifact@v4` with `name: dist-<os>-<arch>`, path `xsdb-*`.

Job `release` (`needs: build`, runs once):
1. `actions/download-artifact@v4` with `pattern: dist-*`, `merge-multiple: true`
   into `dist/`.
2. **Clean-runner extraction validation** (the explicit ask): on a fresh
   `ubuntu-latest`, pick the `linux-x86_64` tarball, `mkdir t && tar xzf
   dist/xsdb-${VER}-linux-x86_64.tar.gz -C t`, then `cd /tmp` and run
   `"$GITHUB_WORKSPACE/t/bin/xsdb" --version` and a generate against a checked-in
   XSD; fail the job if either fails. This is the gate that proves the published
   tarball is self-contained on a machine that never ran the build.
   (Note: only the linux-x86_64 leg can be validated on the linux release
   runner; the macOS/arm tarballs are validated in-place in step A2.2/6 on their
   own runners. Document this split.)
3. Assemble `SHA256SUMS`: `cat dist/*.sha256 > dist/SHA256SUMS` (or recompute
   `shasum -a 256 dist/xsdb-*.tar.gz`).
4. Publish: `softprops/action-gh-release@v2` with
   `files: dist/xsdb-*.tar.gz` + `dist/SHA256SUMS`, `generate_release_notes:
   true`, `draft: false`. (Decision: draft vs auto-publish — recommend
   `draft: true` for the first run so a human eyeballs assets, flip to false
   later.)

### A2.3 — Static-ish linking / portability flags (passed via `extra-cmake-args`)

- **Linux**: link libstdc++/libgcc statically so the tarball runs on older
  glibc userspaces:
  `-DCMAKE_EXE_LINKER_FLAGS="-static-libstdc++ -static-libgcc"`.
  (Cannot fully `-static` because of glibc/dlopen in lua; static libstdc++ is
  the pragmatic choice — matches the prompt.) The root CMake already adds
  `-Wl,--gc-sections`/`--discard-all` on non-Apple Release, which keeps size
  down.
- **macOS**: set a floor for portability:
  `-DCMAKE_OSX_DEPLOYMENT_TARGET=12.0` (or 11.0). Each runner is single-arch
  (arm64 on macos-15, x86_64 on macos-15-large), so build native per leg; no
  `-DCMAKE_OSX_ARCHITECTURES` fat binary needed. (Decision: pick the deployment
  floor; 12.0 is safe for current Homebrew/runner baselines.)
- These flags go into the matrix as a per-OS map, e.g. a `static-flags` matrix
  field referenced in the `extra-cmake-args` input.

### A2.4 — Maven cache for the test legs

The release build sets `BUILD_TESTING=OFF`, so **release itself needs no Maven**.
The ask ("cache `~/.m2` for the test Maven legs") applies to
`build-and-test.yml`: add `actions/cache@v4` keyed on `~/.m2` + a hash of the
`test/**/pom.xml` files, before the build step, in the `unix` job. Net effect:
faster Java round-trip test legs in CI; no change to release. Note this scoping
explicitly in the plan PR.

### A2 sequencing & effort
1. Composite action + refactor CI to use it (verify CI still green). ~0.5 day.
2. `release.yml` build matrix + tarball + in-place validate. ~0.5 day.
3. `release` job: download, clean-runner extract validate, gh-release. ~0.5 day.
4. Static/deployment flags + first tagged dry-run (`v0.2.0-rc1` on a throwaway
   tag, draft release). ~0.5 day incl. debugging arch-specific link issues.
5. `~/.m2` cache (CI only). ~15 min.
**Total ~2 days.** Critical path: the static-libstdc++ link on Linux arm64 and
the macOS deployment-target floor are the most likely to need iteration.

### A2 decisions to confirm
- Windows in release: **NO** (gated on the `ListTemplates` `_WIN32` guard).
- Draft vs auto-publish release: recommend **draft** first run.
- arm64/macOS legs `continue-on-error` in release: recommend **no**.
- Java owned by action vs caller: recommend **action**.

---

## A3 — Packages

Recommended order: **Homebrew first, Docker second, RPM last** (Homebrew/Docker
consume the A2 tarball directly and have the highest payoff-to-effort; RPM needs
a dep-strategy decision and rebuilds from source).

### A3.1 — Homebrew formula (do first)

**Create** `Formula/xsdb.rb` in a **tap repo** (not this repo's main tree).
- **Tap repo name recommendation**: `Ardy123/homebrew-xsd-tools` (Homebrew
  convention: tap repos must be named `homebrew-<name>`; `brew tap
  Ardy123/xsd-tools` then resolves it). Decision needed: confirm the GitHub
  org/user (`Ardy123`) and tap name.
- Formula shape: a `Formula` subclass with per-arch `url`/`sha256` selected via
  `on_macos`+`on_arm`/`on_intel` (and optionally `on_linux`) pointing at the A2
  release tarballs `xsdb-<ver>-macos-arm64.tar.gz` etc., `version "<ver>"`.
  `def install` = `bin.install "bin/xsdb"` + `(share).install
  "share/xsdtools"` (or `prefix.install Dir["*"]` to drop the whole relocatable
  tree). **No wrapper script needed** — the exe-relative discovery in
  `resource.cpp` finds `../share/xsdtools/templates` under the Homebrew cellar
  layout (`bin/` and `share/` are siblings). Add a `test do` block running
  `system bin/"xsdb", "--version"` and a generate on a small inline XSD.
- The release workflow can optionally auto-bump the formula (a job that opens a
  PR to the tap with new url/sha). Defer to a follow-up; manual bump for first
  release.
- **Effort**: ~0.5 day incl. creating the tap repo. **Payoff**: high (one-line
  install for macOS/Linux users).

### A3.2 — Docker / GHCR (do second)

**Create** `Dockerfile` (repo root) — multi-stage:
- Stage `build`: `FROM ubuntu:24.04`, install `cmake ninja-build g++ git
  python3 patch ca-certificates`, copy the repo (with submodules — so the build
  context must include them; either build from a checkout with
  `submodules: recursive` or `git submodule update --init` inside), then the
  same configure/build/install as A2 into `/opt/xsdtools` (`-DBUILD_TESTING=OFF
  -DCMAKE_DISABLE_FIND_PACKAGE_Lua=ON`).
- Stage `runtime`: small base (`ubuntu:24.04` slim or `debian:bookworm-slim`),
  copy `/opt/xsdtools` from build, `ENV XSDTOOLS_DATA=/opt/xsdtools/share/
  xsdtools/templates`, `ENV PATH=/opt/xsdtools/bin:$PATH`,
  `ENTRYPOINT ["xsdb"]`. (`XSDTOOLS_DATA` makes template discovery explicit and
  independent of exe-relative logic — belt and suspenders.)

**Create** `.github/workflows/docker.yml` (or fold a push job into
`release.yml`) on the same `v*` tag:
- `docker/setup-qemu-action` + `docker/setup-buildx-action` for multi-arch
  (`linux/amd64,linux/arm64`), `docker/login-action` to `ghcr.io` using
  `GITHUB_TOKEN`, `docker/build-push-action` with tags
  `ghcr.io/ardy123/xsdb:<ver>` + `:latest`, `platforms:
  linux/amd64,linux/arm64`. `permissions: packages: write`.
- **Decision**: separate `docker.yml` vs a job in `release.yml`. Recommend a
  **job in `release.yml`** (`needs: build` not required since Docker rebuilds
  from source) so one tag drives everything; or a standalone `docker.yml` to
  keep release.yml focused. Either works; pick one.
- **Effort**: ~0.5 day (multi-arch buildx can be slow/finicky). **Payoff**:
  high for CI-pipeline consumers.

### A3.3 — RPM (do last)

**Edit** `pkg/rpm/SPECS/xsd-tools.spec`:
- Remove the hardcoded `%define version 0.1.0`; drive version from the tag.
  Two options: (a) pass `--define "version <ver>"` from `pkg/rpm/build-rpm.sh`
  (which already exists and is modified on this branch — read it before
  editing); (b) keep a `%define version` but have CI `sed` it. Recommend (a).
- **Bundled vs system deps decision**: the current spec uses **system**
  tinyxml/boost/lua-devel. But the CMake build defaults to the *bundled*
  Lua 5.1 (+ matching luac) and fetches tinyxml; system lua is actively
  *removed* in CI because the luac mismatch breaks bytecode. A system-lua RPM
  reintroduces exactly that risk. **Recommendation: build the RPM self-contained
  (bundled lua/tinyxml via the same FetchContent path,
  `-DCMAKE_DISABLE_FIND_PACKAGE_Lua=ON`), keep only boost + expat as system
  `BuildRequires`** (boost is large to vendor; expat is only a test/runtime-gen
  concern). Update `BuildRequires`/`Requires` accordingly and add
  `-DBUILD_TESTING=OFF -DCMAKE_DISABLE_FIND_PACKAGE_Lua=ON` to the `%build`
  cmake invocation. Requires network at build time for FetchContent (note in
  the spec; or pre-seed sources).
- The `%files` list (`/usr/bin/xsdb`, `/usr/share/xsdtools/templates/`) already
  matches the install layout — keep.
- **Optional**: an RPM build job in `release.yml` on a `rockylinux`/`fedora`
  container that runs `pkg/rpm/build-rpm.sh` and attaches the `.rpm` to the
  release. Defer to follow-up.
- **Effort**: ~0.5–1 day (the dep-strategy switch + a clean container build).
  **Payoff**: lower (narrower audience); hence last.

---

## A4 — Docs

### A4.1 — README install quickstart + fix stale/dead content

**Edit** `README.md`:
- **Add an "Install" quickstart** near the top (before the source-build
  section), three paths:
  - **Tarball**: download `xsdb-<ver>-<os>-<arch>.tar.gz` from Releases, verify
    against `SHA256SUMS`, `tar xzf`, run `bin/xsdb` (templates auto-found via
    exe-relative discovery — no env needed). Show the `--version` + a generate.
  - **Homebrew**: `brew tap Ardy123/xsd-tools && brew install xsdb`.
  - **Docker**: `docker run --rm -v "$PWD:/work" -w /work
    ghcr.io/ardy123/xsdb c-xml-expat schema.xsd`.
- **Replace the stale build section** (lines ~159-219): the current text
  describes Conan + `source build/activate.sh` + `ctest`. Replace with the
  **current CMake + gtest-binary flow**:
  `git submodule update --init --recursive` (boost/expat/googletest), then
  `cmake -S . -B build -G "Ninja Multi-Config"
  -DCMAKE_DISABLE_FIND_PACKAGE_Lua=ON`, `cmake --build build --config Release`,
  test via `ctest --test-dir build -C Release --output-on-failure`. Mention lua
  5.1.5 + tinyxml + json-c are auto-fetched, `patch` is needed for the lua
  fallback, and the `-DCMAKE_DISABLE_FIND_PACKAGE_Lua=ON` flag prevents the
  system-lua/luac mismatch. Drop the Conan-specific `build-conan.sh` /
  `activate.sh` instructions (or move to an "advanced/Conan (legacy)" footnote
  since `build-conan.sh` still exists on this branch).
- **Fix the dead Wiki link** (line 29 `_Look at the Wiki section for more
  information._`): replace with a link to the in-repo docs / the new `examples/`
  dir, or remove it. Recommend pointing at `examples/` + the Usage Guide
  section anchor.
- **Badges** (line 1 has one): add a release badge
  (`img.shields.io/github/v/release/Ardy123/xsd-tools`), a license badge
  (GPL-3.0), optionally a GHCR/Docker badge. Keep the existing build-and-test
  badge.

### A4.2 — `examples/` directory

**Create** `examples/` with:
- `examples/address.xsd` — a trimmed real schema. Use a copy of
  `test/xsd-positive/testA001.xsd` (an address `complexType` with a `sequence`
  of street/city/zipcode/country + a `USAddress` restriction + an
  `AddressElm` element) — it's small, self-contained, and exercises
  complexType/sequence/restriction without namespaces.
- `examples/README.md` — a `xsdb <target> address.xsd` walkthrough:
  - `xsdb --list` to show targets.
  - `xsdb c-xml-expat examples/address.xsd > address.c` (print to stdout).
  - `xsdb --out-dir out c-json-jsonc examples/address.xsd` (multi-file split).
  - `xsdb python-sax examples/address.xsd` and a Java target, showing the
    six built-in targets from the README table.
  - A note that trailing `k v` pairs become `__CMD_ARGS__` in the template.
- Keep the example XSD < ~25 lines; the goal is a copy-pasteable first run.

### A4 sequencing & effort
1. `examples/` (XSD + walkthrough README). ~0.5 day.
2. README install quickstart + replace stale Conan/ctest section + wiki-link
   fix + badges. ~0.5 day. (Do after A2/A3 so the `brew`/`docker`/tarball
   commands are real, or write them ahead and verify against the first
   release.)
**Total ~1 day.**

### A4 decisions to confirm
- Keep `build-conan.sh` / Conan as a "legacy/advanced" footnote, or delete the
  Conan path from docs entirely (the script still exists on-branch).
- Example schema choice: recommend `testA001.xsd` (address) over `testA002`
  (substitutionGroup, less intuitive).

---

## Cross-cutting sequencing (recommended overall order)

1. **A2.1** composite action + CI refactor (unblocks everything, keeps CI green).
2. **A2.2/A2.3** release build matrix + tarball + validate + first draft release
   on an RC tag.
3. **A3.1** Homebrew tap + formula (consumes the tarball).
4. **A3.2** Docker/GHCR.
5. **A4** docs (now the install commands are real).
6. **A3.3** RPM (dep-strategy switch).
7. (Deferred) Windows-in-release — only after `ListTemplates`/`TemplatesDir`
   get a `_WIN32` branch (FindFirstFile or `std::filesystem`).

## Parallelization plan (sub-agent fan-out)

The work splits cleanly once one piece of scaffolding is frozen. Do the
scaffolding **synchronously first**, then fan out the independent legs.

### Phase 0 — scaffolding (sync, single owner, no fan-out)

This is the frozen API contract every parallel leg depends on. Must land and CI
must be green before dispatching agents.

- **Build the composite action** `.github/actions/build-xsdb/action.yml`
  (A2.1) and refactor `build-and-test.yml` to consume it. Verify CI green.
- **Confirm the decisions checklist** (tap name, GHCR image name, version flow,
  RPM dep strategy). The agents need these as constants.

Why sync: A2.2 (release.yml), A3.2 (Dockerfile), and A3.3 (RPM) all need to
invoke the *same* configure/build incantation. If three agents each invent it,
they drift and merge conflicts surface as build failures. Freeze it once.

Deliver to every agent: the path `.github/actions/build-xsdb/action.yml`, its
input names (`version`, `build-testing`, `config`, `extra-cmake-args`), and the
canonical configure line + install layout (`bin/`, `share/xsdtools/templates/`,
exe-relative discovery).

### Phase 1 — parallel batch (4 independent agents, one dispatch message)

No shared files between these legs; each owns a disjoint file set.

| Agent | Owns (creates/edits) | Must NOT touch | Report |
|-------|----------------------|----------------|--------|
| **A — Release workflow** | `.github/workflows/release.yml`; the `~/.m2` cache edit in `build-and-test.yml` | `.github/actions/build-xsdb/action.yml` (consume only); any `src/` | tag-push dry-run on an RC tag produces tarballs + draft release; paste the run URL + word cap 80 |
| **B — Homebrew** | new tap repo `Ardy123/homebrew-xsd-tools` + `Formula/xsdb.rb` (separate repo — agent works in a worktree/clone, not this tree) | this repo's tree entirely | `brew install --build-from-source` against a real RC tarball succeeds; word cap 60 |
| **C — Docker/GHCR** | `Dockerfile` (root); `.github/workflows/docker.yml` (or the docker job spec) | `release.yml` (owned by A), `src/` | `docker build` multi-arch succeeds locally + `xsdb --version` in container; word cap 60 |
| **D — Docs + examples** | `README.md`; `examples/address.xsd`; `examples/README.md` | all workflow/action/spec files | README renders; `examples/` walkthrough commands run against a built `xsdb`; word cap 80 |

Notes on the batch:
- Agents A and C both *reference* the composite action but only A edits a
  workflow that lives in this repo's `.github/workflows/` besides its own; C's
  `docker.yml` is a new file, no overlap. If you instead fold Docker into
  `release.yml`, then A and C **conflict** — in that case keep Docker as a
  standalone `docker.yml` so the fan-out stays conflict-free.
- Agent B operates in a *different repository* (the tap), so it is fully
  isolated — ideal parallel candidate. It only needs the RC tarball URL/sha
  from Agent A's dry-run, so either run B slightly behind A, or hand B a
  placeholder url/sha to fill once A's RC release exists. Recommend: B drafts
  the formula against the expected asset names immediately, finalizes sha after
  A's RC.
- Agent D writes install commands (`brew tap ...`, `docker run ghcr.io/...`)
  that depend on B's tap name and C's image name — both are frozen constants
  from Phase 0's decision checklist, so D does not need to wait on B/C.

### Phase 2 — RPM (sequential, after Phase 1 or in parallel as a 5th agent)

RPM (A3.3) edits `pkg/rpm/SPECS/xsd-tools.spec` + `pkg/rpm/build-rpm.sh` — a
disjoint file set, so it *can* join Phase 1 as a fifth agent. But it carries the
bundled-vs-system dep decision and a containerized build that's slower to
iterate, and it's the lowest-payoff item. Recommend running it **after** the
Phase 1 merge so a flaky RPM container build doesn't gate the high-value legs.
If wall-clock matters more than risk, dispatch it in the Phase 1 batch (it
touches only `pkg/rpm/`, zero conflict with A–D).

### Phase 3 — integration (sync, single owner)

After agents report: merge all branches, then run end-to-end on one real RC tag
push:
1. Confirm `release.yml` produces all four tarballs + `SHA256SUMS`.
2. Confirm the clean-runner extract-validate job passes.
3. `brew install` from the tap against the published assets.
4. `docker run ghcr.io/ardy123/xsdb --version`.
5. Walk through `examples/README.md` against a freshly installed `xsdb`.
6. Fix any contract-drift surfaced as a failure (the expected failure mode of
   fan-out), then flip the release from draft to published.

### Cost/benefit of fanning out

- 4 agents on ~3–4 days of serial work -> ~1–1.5 days wall-clock. Worth it.
- Each agent pays cold-context cost (~4x tokens), justified by the leg sizes
  (each is >0.5 day of real work, well above the "wasteful for small tasks"
  threshold).
- Coordination cost is the Phase 0 contract design (~half a day building the
  composite action) — which is required work regardless, so it's not pure
  overhead.
- Main drift risk: the configure/build line. Eliminated by Phase 0 freezing it
  in the composite action; B (Homebrew, prebuilt tarball) and C (Docker,
  in-container build) both consume artifacts/incantations rather than
  re-deriving them.

## Decisions checklist (collect before starting)
- [ ] Tap repo name + owner: proposed `Ardy123/homebrew-xsd-tools`.
- [ ] GHCR image name: proposed `ghcr.io/ardy123/xsdb`.
- [ ] Windows in first release: proposed NO.
- [ ] RPM deps: proposed bundled lua/tinyxml + system boost/expat.
- [ ] First release: draft vs publish (proposed draft).
- [ ] macOS deployment floor: proposed 12.0.
- [ ] Conan docs: legacy footnote vs removed.
