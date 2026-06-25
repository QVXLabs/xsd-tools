![ubuntu latest x86_64 workflow](https://github.com/Ardy123/xsd-tools/actions/workflows/build-and-test.yml/badge.svg)

# xsd-tools
### Overview ###
xsd-tools is a set of tools for generating code from xml xsd schema documents, mainly around generating marshalling & unmarshalling code. It is designed such that it can be easily extended by any user to enable code generation for any language.

It ships output targets covering the C / C++ / Python / Java / TypeScript languages, all round-trip tested:

| Target template      | Language   | Format   | Library          | Guide |
|----------------------|------------|----------|------------------|-------|
| `c-xml-expat`        | C          | XML      | expat            | [docs](docs/templates/c-xml-expat.md) |
| `c-xml-expat-dom`    | C          | XML (DOM)| expat            | [docs](docs/templates/c-xml-expat-dom.md) |
| `c-json-jsonc`       | C          | JSON     | json-c           | [docs](docs/templates/c-json-jsonc.md) |
| `cpp-xml-expat`      | C++11      | XML      | expat            | [docs](docs/templates/cpp-xml-expat.md) |
| `cpp-json-jsonc`     | C++11      | JSON     | json-c           | [docs](docs/templates/cpp-json-jsonc.md) |
| `python-sax`         | Python     | XML      | stdlib `xml.sax` | [docs](docs/templates/python-sax.md) |
| `python-json.tmpl`   | Python     | JSON     | stdlib `json`    | [docs](docs/templates/python-json.md) |
| `java-json.org.tmpl` | Java       | JSON     | org.json         | [docs](docs/templates/java-json.md) |
| `java-xml-stax.tmpl` | Java       | XML      | JDK StAX         | [docs](docs/templates/java-xml-stax.md) |
| `ts-xml.tmpl`        | TypeScript | XML      | fast-xml-parser  | [docs](docs/templates/ts-xml.md) |

Per-target guides (generate command, toolchain, generated API, examples) live
under [`docs/templates/`](docs/templates/).

Most targets construct schema types through overridable factory methods, so consumers can subclass and inject custom types (the `java-json.org` target constructs nested types directly) — see each target's guide for the exact mechanism.

It processes XSD schema documents and invokes a template file which outputs code. The templates files use Lua for scripting within the template file. Custom user templates can be easily be created to extend the tool to generate different output code.

### Features ###
  * XSD schema parsing
  * Ten built-in output targets across C/C++/Python/Java/TypeScript,
    easily extendable
  * Generated unmarshallers enforce XSD restriction facets at parse time
    (range, length, enumeration on elements and attributes) — invalid documents
    are rejected.
  * Facet-bounded integer fields narrow to the smallest fitting type
    (e.g. `0..255` → `uint8_t` in C, `Short` in Java).
  * Simple Lua based templates for customizing code generation.
  * Usable as a C++ library (`XsdTools::Generate()`) as well as a CLI.
  * Open Source!

Namespaces are supported: schemas with a `targetNamespace` resolve and the XML
targets emit the right `xmlns`/prefixes, and `xs:import` brings in types from
another namespace (`xs:include` continues to merge same-namespace documents).

### Sample Output ###
Each [per-target guide](docs/templates/) shows real generated output and a
marshal/unmarshal example for that target. For a quick look without reading a
guide, just run the tool — e.g. `xsdb python-sax test/xsd-positive/testA002.xsd`
prints the generated Python to stdout.

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

#### Installing / Uninstalling ####
```
   sudo cmake --install build --config Release            # uses CMAKE_INSTALL_PREFIX
   xargs rm < build/install_manifest.txt                  # to uninstall
```
Set the install location at configure time
(`-DCMAKE_INSTALL_PREFIX=/usr/local`); it is baked into the binary's template
search path.
