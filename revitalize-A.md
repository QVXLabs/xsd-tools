# Revitalize Workstream A — Distribution & discoverability

Branch: `revitalize`. Planning only (read-only on source/CI/CMake).

## Status

- **A0 — Portable template discovery: DONE.** `src/resource.cpp` `_exeDir`
  (`/proc/self/exe` / `_NSGetExecutablePath` / `GetModuleFileNameA`) +
  exe-relative fallback in `GetTemplatePath` (`<exe>/../share/xsdtools/templates`
  then `<exe>/templates`); `<unistd.h>/<pwd.h>/<dirent.h>` guarded behind
  `#if defined(_WIN32)`. A `bin/`+`share/` tarball finds templates with no env.
- **A1 — Versioning: DONE.** `cmake/GitVersion.cmake` (git describe → numeric
  `XSD_VERSION` for `project(VERSION)`, descriptive `XSD_VERSION_FULL`, override
  `-DXSD_VERSION=`); `XSDTOOLS_VERSION` flows into `resource_config.hpp.in`;
  `xsdb --version` works.
- **A1b — C/C++ standards: DONE.** `CMAKE_CXX_STANDARD 11` +
  `CMAKE_C_STANDARD 11` pinned (`CMakeLists.txt:33-38`).
- **A2/A3/A4 — remaining** (below).

### Two findings that change the plan

1. **CI is `.github/workflows/build-and-test.yml`** (not `cmake.yml`). It
   **already** carries the fixes the composite action must preserve: Java 21
   (temurin), `~/.m2` cache, the stray-lua removal (`apt-get remove 'liblua5.*'
   'lua5.*'`), and `-DCMAKE_DISABLE_FIND_PACKAGE_Lua=ON`. **Tests run via
   `python3 test/run_parallel.py --binary build/test/Release/xsdb-test`, not
   ctest** — the composite refactor must keep that invocation.
2. **Windows-in-release stays NO.** `resource.cpp` `TemplatesDir`/`ListTemplates`
   (185-222) call `getpwuid`/`opendir`/`readdir` **unconditionally** — library
   code linked into `xsdb`. Until those get a `_WIN32` branch
   (FindFirstFile / `std::filesystem`), ship Unix-first; track the guard as a
   prerequisite.

---

## A2 — Release workflow

Tag `v*` → relocatable tarball per OS/arch → extract-and-run validate on a clean
runner → GitHub Release with tarballs + `SHA256SUMS`.

### A2.1 Composite action `.github/actions/build-xsdb/action.yml`

Factor the shared build out of `build-and-test.yml` so CI and release run one
incantation (risk H3). Inputs: `version` (→ `-DXSD_VERSION=` only when set),
`build-testing` (default `ON`; release passes `OFF` so json-c — fetched only in
`test/CMakeLists.txt` under `BUILD_TESTING` — is skipped), `config`
(default `Release`), `extra-cmake-args` (release passes A2.3 flags).
Steps, carrying forward CI fixes verbatim:
1. `setup-java@v4` temurin 21.
2. Per-`$RUNNER_OS` deps: Linux `apt-get install -y ninja-build maven` + print
   then `apt-get remove -y 'liblua5.*' 'lua5.*' || true`; macOS
   `brew install ninja maven || true`.
3. `cmake -S . -B build -G "Ninja Multi-Config"
   -DCMAKE_DISABLE_FIND_PACKAGE_Lua=ON -DBUILD_TESTING=<input>` (+version
   +extra-args).
4. `cmake --build build --config <config> -j`.
`checkout` (`submodules: recursive`), `setup-python`, and the `~/.m2` cache stay
in the **callers** (checkout must precede the action; python/maven are test-only).
**Edit `build-and-test.yml`**: replace "Install build tools" + "Configure +
build + test" in the `unix` job with `- uses: ./.github/actions/build-xsdb`
then a `Test` step `python3 test/run_parallel.py --binary
build/test/Release/xsdb-test`. Leave `windows` job as-is (out of release scope,
can't share the apt/brew branch).

### A2.2 `.github/workflows/release.yml` (`on: push: tags: ['v*']`)

`permissions: contents: write`. Matrix (Unix-first, mirror CI runners):
`ubuntu-latest`/x86_64, `ubuntu-24.04-arm`/arm64, `macos-15`/arm64,
`macos-15-large`/x86_64. **Recommend NOT `continue-on-error`** — a missing arch
tarball must fail loudly.

`build` job: checkout (recursive) → derive `VER=${GITHUB_REF_NAME#v}` →
`uses: ./.github/actions/build-xsdb` (`version: ${VER}`, `build-testing: OFF`,
`extra-cmake-args:` per-OS map A2.3) → `cmake --install build --config Release
--prefix "$PWD/stage"` (yields `bin/xsdb` + `share/xsdtools/templates/`, plus
`lib/`+`include/` SDK export — keep, small) → **in-place validate from a
foreign cwd** (`cd /tmp; "$GITHUB_WORKSPACE/stage/bin/xsdb" --version` and a
generate against `test/xsd-positive/testA001.xsd`, assert non-empty — proves
exe-relative discovery, risk H1) → package `TAR=xsdb-${VER}-<os>-<arch>.tar.gz`
(`<os>`=`linux`/`macos` from `$RUNNER_OS`), `tar czf "$TAR" -C stage .`,
`shasum -a 256 "$TAR" > "$TAR.sha256"` (present on both OSes) →
`upload-artifact@v4` name `dist-<os>-<arch>`.

`release` job (`needs: build`, once): `download-artifact` `pattern: dist-*`
`merge-multiple: true` → **clean-runner extract-validate**: on the linux runner,
`tar xzf` the `linux-x86_64` tarball into a temp dir, `cd /tmp`, run
`bin/xsdb --version` + a generate, fail on error (macOS/arm legs validated
in-place in `build`) → `cat dist/*.sha256 > dist/SHA256SUMS` →
`softprops/action-gh-release@v2` (`files: dist/xsdb-*.tar.gz` + `SHA256SUMS`,
`generate_release_notes: true`). **First run `draft: true`** for a human to
eyeball assets.

`<ver>` in the artifact name is the tag minus `v` (`XSD_VERSION_FULL` via the
`-DXSD_VERSION=${VER}` the action forwards), so `--version` and the filename
match.

### A2.3 Portability flags (per-OS `extra-cmake-args`)

- Linux: `-DCMAKE_EXE_LINKER_FLAGS="-static-libstdc++ -static-libgcc"` (runs on
  older glibc; full `-static` impossible — lua needs dlopen). Root CMake already
  adds `-Wl,--gc-sections`/`--discard-all` on non-Apple Release.
- macOS: `-DCMAKE_OSX_DEPLOYMENT_TARGET=12.0`. Each runner is single-arch; build
  native per leg, no fat binary.

`~/.m2` cache already exists in CI (no A2.4 work) — release sets
`BUILD_TESTING=OFF` so needs no Maven.

---

## A3 — Packages (order: Homebrew → Docker/GHCR → RPM)

### A3.1 Homebrew (first; post-release — needs published sha + tap repo)

Tap repo `Ardy123/homebrew-xsd-tools` (`brew tap Ardy123/xsd-tools`). Formula
`Formula/xsdb.rb`: per-arch `url`/`sha256` via `on_macos`+`on_arm`/`on_intel`
pointing at the A2 tarballs, `version "<ver>"`; `def install` =
`prefix.install Dir["*"]` (drops the relocatable `bin/`+`share/` tree —
exe-relative discovery finds `../share/xsdtools/templates` under the cellar).
`test do`: `system bin/"xsdb", "--version"` + a generate. Manual sha bump for
the first release; auto-bump PR job is a follow-up.

### A3.2 Docker / GHCR (distribution only — never in build/test path)

`Dockerfile` (root), multi-stage. Build: `FROM ubuntu:24.04`, install
`cmake ninja-build g++ git python3 patch ca-certificates`, copy repo with
submodules (or `git submodule update --init --recursive`), configure/build/
install as A2 into `/opt/xsdtools` (`-DBUILD_TESTING=OFF
-DCMAKE_DISABLE_FIND_PACKAGE_Lua=ON`). Runtime: `debian:bookworm-slim`, copy
`/opt/xsdtools`, `ENV XSDTOOLS_DATA=/opt/xsdtools/share/xsdtools/templates`
(belt-and-suspenders per risk H1 — `GetTemplatePath` checks `XSDTOOLS_DATA`
before exe-relative), `ENV PATH=/opt/xsdtools/bin:$PATH`, `ENTRYPOINT
["xsdb"]`. Standalone `.github/workflows/docker.yml` on `v*` (keep it out of
`release.yml` to avoid file-ownership conflict if fanned out):
`setup-qemu` + `setup-buildx` + `login` to `ghcr.io` (`GITHUB_TOKEN`) +
`build-push` tags `ghcr.io/ardy123/xsdb:<ver>`+`:latest`, `platforms:
linux/amd64,linux/arm64`, `permissions: packages: write`.

### A3.3 RPM (last) — update `pkg/rpm/` for the CMake build

`pkg/rpm/SPECS/xsd-tools.spec` already calls `cmake -S . -B build
-DCMAKE_INSTALL_PREFIX=/usr -DBUILD_TESTING=OFF` and `cmake --install`, and the
`%files` list (`/usr/bin/xsdb`, `/usr/share/xsdtools/templates/`) matches.
Changes: (1) drop hardcoded `%define version 0.1.0` — pass `--define "version
<ver>"` from `pkg/rpm/build-rpm.sh` (already on-branch). (2) **Dep strategy: drop
system lua.** Spec lists `lua-devel`/`lua` Requires, but the build defaults to
bundled lua and CI *removes* system lua because of the luac/bytecode mismatch —
a system-lua RPM reintroduces that bug. Add `-DCMAKE_DISABLE_FIND_PACKAGE_Lua=ON`
to `%build`, drop lua/tinyxml from BuildRequires/Requires, keep boost/expat as
system; note FetchContent needs network at build time. Optional RPM job in
`docker.yml`/`release.yml` on a Rocky/Fedora container attaching the `.rpm`.

---

## A4 — Docs

`README.md`: (1) **Install quickstart** near top — tarball (download +
`SHA256SUMS` verify + `tar xzf` + run `bin/xsdb`, no env), `brew tap
Ardy123/xsd-tools && brew install xsdb`, `docker run --rm -v "$PWD:/work"
-w /work ghcr.io/ardy123/xsdb c-xml-expat schema.xsd`. (2) **Replace stale
build section** (lines ~164-221): drops the Conan + `source build/activate.sh` +
`ctest` flow; the real flow is `git submodule update --init --recursive`,
`cmake -S . -B build -G "Ninja Multi-Config" -DCMAKE_DISABLE_FIND_PACKAGE_Lua=ON`,
`cmake --build`, **test via `python3 test/run_parallel.py --binary
build/test/Release/xsdb-test`** (not ctest). Keep Conan as a legacy footnote
(`build-conan.sh` still exists). (3) **Fix dead Wiki link** (line 34) → point at
`examples/` + the Usage Guide anchor. (4) **Badges**: keep build-and-test; add
release (`shields.io/github/v/release/Ardy123/xsd-tools`), GPL-3.0 license,
optional GHCR. Usage already documents `--out-dir`/`--list`/`--version` and the
six targets — no change there.
`examples/`: `examples/address.xsd` (copy `test/xsd-positive/testA001.xsd`,
small complexType/sequence/restriction, no namespaces) + `examples/README.md`
walkthrough (`--list`, `c-xml-expat` to stdout, `--out-dir out c-json-jsonc`,
`python-sax`, note on trailing `k v` → `__CMD_ARGS__`).

---

## Risks

- **H1 — A0 is a single point of failure for releases.** The whole distribution
  depends on exe-relative discovery. Mitigate: extract-and-run from a foreign
  cwd both in-place (`build` job) and on a clean runner (`release` job) **before
  publish**; `XSDTOOLS_DATA` set in the Docker image as a fallback.
- **H3 — composite action must preserve CI fixes** (Java 21, disable-Lua flag,
  stray-lua removal, the `run_parallel.py` test invocation). Verify CI stays
  green after the refactor before building release on top.
- **Homebrew is post-release** — formula sha/url require the published tarballs;
  draft the formula against expected asset names, finalize sha after the RC.
- **Windows release deferred** until `ListTemplates`/`TemplatesDir` are
  `_WIN32`-guarded.

## Verification

Push `v0.2.0-rc` → per-OS `xsdb-<ver>-<os>-<arch>.tar.gz` + `SHA256SUMS` as
draft-release assets; extract on a clean machine and run `bin/xsdb --version` +
a generate with no env; `brew install` from the tap; `docker run
ghcr.io/ardy123/xsdb --version`; walk `examples/README.md`. Flip draft→published.

## Sequencing

A2 (release workflow) is independent of the parser workstreams and can run in
parallel with B/I. Within A: A2.1 composite + CI refactor (verify green) →
A2.2/A2.3 release matrix + RC dry-run → **A3/A4 last** (Homebrew/Docker/RPM
consume the tarball; docs cite real install commands). Decisions to confirm: tap
name `Ardy123/homebrew-xsd-tools`, image `ghcr.io/ardy123/xsdb`, macOS floor
12.0, first release draft, RPM bundled-lua, Conan-as-footnote.
