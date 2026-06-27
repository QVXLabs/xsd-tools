# xsd-tools

**One XSD schema in, idiomatic marshalling code out — for C, C++, Python,
Java, and TypeScript.** Stop hand-writing parsers; generate them, validation
and all.

Build status (one badge per CI build, all on push/PR to `master`):

| Platform | x86_64 | arm64 |
|----------|--------|-------|
| Linux   | ![linux-x86_64](https://github.com/QVXLabs/xsd-tools/actions/workflows/ci-linux-x86_64.yml/badge.svg?branch=master) | ![linux-arm64](https://github.com/QVXLabs/xsd-tools/actions/workflows/ci-linux-arm64.yml/badge.svg?branch=master) |
| macOS   | ![macos-x86_64](https://github.com/QVXLabs/xsd-tools/actions/workflows/ci-macos-x86_64.yml/badge.svg?branch=master) | ![macos-arm64](https://github.com/QVXLabs/xsd-tools/actions/workflows/ci-macos-arm64.yml/badge.svg?branch=master) |
| Windows | ![windows-x86_64](https://github.com/QVXLabs/xsd-tools/actions/workflows/ci-windows-x86_64.yml/badge.svg?branch=master) | ![windows-arm64](https://github.com/QVXLabs/xsd-tools/actions/workflows/ci-windows-arm64.yml/badge.svg?branch=master) |

### Why xsd-tools ###

Hand-writing the code to parse and emit XML/JSON against a schema is tedious,
repetitive, and easy to get subtly wrong. And the existing generators are each
welded to a single language — JAXB for Java, `xsd.exe` for .NET, generateDS for
Python — so a team that shares one schema across services ends up maintaining N
separate, slowly-drifting hand-rolled implementations.

xsd-tools takes that schema and generates the marshalling & unmarshalling code
for **ten targets across five languages from a single XSD**, every one
round-trip tested in CI. The generated code isn't a lowest-common-denominator
blob either — it's idiomatic per target, enforces the schema's own validation
rules, and carries your `xs:documentation` through as comments.

Need a target it doesn't ship? Adding one is **a Lua template — no C++ to
write, no rebuild.** Try it in ten seconds against the bundled example:

```
xsdb python-sax examples/library.xsd     # generated Python, straight to stdout
```

It ships ten output targets covering C / C++ / Python / Java / TypeScript, all
round-trip tested:

| Target template      | Language   | Format   | Library          | Guide |
|----------------------|------------|----------|------------------|-------|
| `c-xml-expat`        | C          | XML      | expat            | [docs](docs/templates/c-xml-expat.md) |
| `c-xml-expat-dom`    | C          | XML (DOM)| expat            | [docs](docs/templates/c-xml-expat-dom.md) |
| `c-json-jsonc`       | C          | JSON     | json-c           | [docs](docs/templates/c-json-jsonc.md) |
| `cpp-xml-expat`      | C++11      | XML      | expat            | [docs](docs/templates/cpp-xml-expat.md) |
| `cpp-json-jsonc`     | C++11      | JSON     | json-c           | [docs](docs/templates/cpp-json-jsonc.md) |
| `python-sax`         | Python     | XML      | stdlib `xml.sax` | [docs](docs/templates/python-sax.md) |
| `python-json`         | Python     | JSON     | stdlib `json`    | [docs](docs/templates/python-json.md) |
| `java-json.org`      | Java       | JSON     | org.json         | [docs](docs/templates/java-json.md) |
| `java-xml-stax`      | Java       | XML      | JDK StAX         | [docs](docs/templates/java-xml-stax.md) |
| `ts-xml`                | TypeScript | XML      | fast-xml-parser  | [docs](docs/templates/ts-xml.md) |

Per-target guides (generate command, toolchain, generated API, examples) live
under [`docs/templates/`](docs/templates/).

Most targets construct schema types through overridable factory methods, so consumers can subclass and inject custom types (the `java-json.org` target constructs nested types directly) — see each target's guide for the exact mechanism.

### What makes it different ###

  * **Five languages, one schema.** Ten round-trip-tested targets across
    C / C++ / Python / Java / TypeScript — change the schema once, regenerate
    every binding, and they stay in sync by construction.
  * **Extend without touching C++.** Output is driven by simple Lua
    [templates](templates/); a new target is a template file, not a fork and a
    rebuild. Copy the closest existing one and adapt the format-specific bits.
  * **The schema's rules are enforced, not just documented.** Generated
    unmarshallers validate XSD restriction facets at parse time — range,
    length, and enumeration on both elements and attributes — so invalid
    documents are rejected instead of silently accepted.
  * **Tighter types for free.** A facet-bounded integer narrows to the smallest
    type that fits (`0..255` → `uint8_t` in C, `Short` in Java), so the
    generated structs cost what they should.
  * **Your docs survive the trip.** `xs:documentation` is carried into the
    generated code as a comment at the matching type, field, or attribute.
  * **Handles real-world schemas.** Namespaces / `targetNamespace`,
    `xs:import` / `xs:include`, substitution groups, abstract types, and
    recursive (self- and mutually-referential) types are all supported — see
    the [capability map](docs/limitations.md) for the exact boundaries.
  * **CLI or library.** Use the `xsdb` command, or embed the generator in your
    own C++ via `XsdTools::Generate()`.
  * **Open source.**

### See it work ###
[`examples/`](examples/) has a guided, end-to-end walkthrough — a small library
catalog schema and the real generated output across several targets (showing
annotations-as-comments, facet validation, and type narrowing). Each
[per-target guide](docs/templates/) then shows that target's generated API and
a marshal/unmarshal example. For a quick look, just run the tool — e.g.
`xsdb python-sax examples/library.xsd` prints the generated Python to stdout.

For **cross-language interop**, [`examples/quickstart/`](examples/quickstart/)
relays one XML message through five programs in Python, C++, and Java — every
binding generated by `xsdb` from one schema. Build and run it with
`cmake -DQUICKSTART=ON … && cmake --build build --target run-quickstart`.

### Usage Guide ###

To use xsd-tools, invoke xsdb using the following syntax from the console

`# ./xsdb [options] <template> <input.xsd> [k v ...]`

Generated code is printed to stdout. Trailing `k v` pairs are passed to the
template as `__CMD_ARGS__`.

Options:
  * `--out-dir <dir>` — split multi-file output (the `/* FILE: name */` markers
    some targets emit) into real files under `<dir>`.
  * `--list` — list the available templates.
  * `--version` — print the version.
  * `-h`, `--help` — show usage.

#### examples ####
```
# xsdb python-sax testA022.xsd                       # print Python to stdout
# xsdb --out-dir out c-json-jsonc testA022.xsd        # write the C/JSON files
# xsdb --list                                         # show templates
```

### Install Instructions ###

#### Packages ####

Each published GitHub release attaches prebuilt Linux packages — a Debian
`.deb`, an RPM `.rpm`, and a single-file Flatpak `.flatpak` bundle:

```
   sudo apt install ./xsd-tools_<ver>_amd64.deb        # Debian/Ubuntu
   sudo dnf install ./xsd-tools-<ver>.x86_64.rpm       # Fedora/RHEL
   flatpak install ./xsd-tools-<ver>.flatpak           # then: flatpak run com.qvxlabs.xsd_tools
```

Build the packages yourself (Linux): `pkg/build-packages.sh` produces the
`.deb` + `.rpm` (via CPack) into `dist/`, and `pkg/flatpak/build-flatpak.sh`
produces the `.flatpak`. boost/expat/lua/tinyxml are statically linked, so the
packages depend only on the C/C++ runtime.

#### From source ####

The build uses **CMake**. For each dependency CMake tries `find_package`
first (satisfied by **Conan** — which also supplies the Lua 5.1 `luac` the
embedded template engine needs — or by system packages); whatever is not found
falls back automatically to a bundled copy, all matching the Conan recipe
versions. googletest, expat, and the boost superproject are git submodules;
fetch them first:
```
   git submodule update --init --recursive
```
(The boost superproject is large; its recursive init pulls boost's component
libraries.) lua 5.1.5 and tinyxml 2.6.2 are downloaded automatically by CMake
(FetchContent) at configure time into `<build>/third-party/` — no submodule
needed, but the first configure needs network access. The lua fallback applies
the recipe's LNUM patch, so `patch` must be on the host.

#### Requirements ####
  * cmake (>= 3.16) and a generator (Ninja recommended)
  * `patch` (only for the lua FetchContent fallback)
  * a C++11 compiler (gcc/clang/MSVC)
  * Lua **5.1** (library + `luac`) — newer Lua will not work
  * tinyxml, expat, boost (system + filesystem)
  * python3 (build-time test generation; runtime for the python round-trips)
  * conan (`pip install 'conan<2'`) for the dependency path below
  * json-c (the C/JSON target; fetched automatically if not found)
  * optional test toolchains — the round-trip tests skip when these are
    absent: a JDK + Maven (Java targets), node + npm (the `ts-xml` target)

On ubuntu the system packages are:
```
   sudo apt-get install cmake ninja-build lua5.1 liblua5.1-0-dev \
       libtinyxml-dev libexpat1-dev libboost-system-dev \
       libboost-filesystem-dev python3
```

#### Building (Conan) ####
The simplest path — fetches deps and builds both configurations:
```
   ./build-conan.sh Release      # or: Debug
```
Equivalently, by hand:
```
   conan install . -if build --build=missing
   source build/activate.sh                 # puts the Lua 5.1 luac on PATH
   cmake -S . -B build -G "Ninja Multi-Config"
   cmake --build build --config Release
```
`Ninja Multi-Config` keeps both Debug and Release in one tree; pick the
config at build time with `--config`.

#### Testing ####
Run the gtest binary directly (project convention — not ctest):
```
   build/test/xsdb-test                                # all tests
   build/test/xsdb-test --gtest_filter='Parser.*'      # a subset
   python3 test/run_parallel.py --binary build/test/xsdb-test  # sharded, faster
```
The round-trip tests shell out to each target's toolchain (cc, python3, mvn/java,
node/tsc) and **skip** cleanly when a toolchain is absent.

`test/stress/` is a separate **discovery** tool, not a CI gate. It runs
`python3 test/stress/run_stress.py` over a corpus of real-world schemas
(xhtml, wsdl, soap, kml, …) and reports only whether each parses across the
targets — it does **not** compile or run the generated code, and a failure
there does not fail the build.

#### Installing / Uninstalling ####
```
   sudo cmake --install build --config Release            # uses CMAKE_INSTALL_PREFIX
   xargs rm < build/install_manifest.txt                  # to uninstall
```
Set the install location at configure time
(`-DCMAKE_INSTALL_PREFIX=/usr/local`); it is baked into the binary's template
search path.
