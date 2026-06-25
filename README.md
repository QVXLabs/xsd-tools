![ubuntu latest x86_64 workflow](https://github.com/Ardy123/xsd-tools/actions/workflows/build-and-test.yml/badge.svg)

# xsd-tools
### Overview ###
xsd-tools is a set of tools for generating code from xml xsd schema documents, mainly around generating marshalling & unmarshalling code. It is designed such that it can be easily extended by any user to enable code generation for any language.

It ships output targets covering the C / Python / Java languages in both XML and JSON, all round-trip tested:

| Target template      | Language | Format | Library          |
|----------------------|----------|--------|------------------|
| `c-xml-expat`        | C        | XML    | expat            |
| `c-json-jsonc`       | C        | JSON   | json-c           |
| `python-sax`         | Python   | XML    | stdlib `xml.sax` |
| `python-json.tmpl`   | Python   | JSON   | stdlib `json`    |
| `java-json.org.tmpl` | Java     | JSON   | org.json         |
| `java-xml-stax.tmpl` | Java     | XML    | JDK StAX         |

Generated code constructs schema types through overridable factory methods, so consumers can subclass and inject custom types.

It processes XSD schema documents and invokes a template file which outputs code. The templates files use Lua for scripting withing the template file. Custom user templates can be easily be created to extend the tool to generate different output code.

### Features ###
  * XSD schema parsing
  * Six built-in output targets (C/Python/Java × XML/JSON), easily extendable
  * Generated unmarshallers enforce XSD restriction facets at parse time
    (range, length, enumeration on elements and attributes) — invalid documents
    are rejected.
  * Facet-bounded integer fields narrow to the smallest fitting type
    (e.g. `0..255` → `uint8_t` in C, `Short` in Java).
  * Simple Lua based templates for customizing code generation.
  * Usable as a C++ library (`XsdTools::Generate()`) as well as a CLI.
  * Open Source!

_Look at the Wiki section for more information._

NOTE: Currently the tools are not namespace aware.

### Sample Output ###
```
# xsdb python-sax test/xsd-positive/testA002.xsd


import cStringIO
import xml.sax

class _handler(xml.sax.ContentHandler):
        _elemTbl = {
                'myElem':lambda: xml_myElem(),
                'foo':lambda: xml_foo(),
        }
        def __init__(self):
                xml.sax.ContentHandler.__init__(self)
                return
        # methods inherited from xml.sax.ContentHandler
        def startDocument(self):
                self._elemStk = [("testA021", {}, [])]
                return
        def startElementNS(self, name, qname, SAXAttributes):
                element = _handler._elemTbl[name[1]]()
                elementRecord = (element, self._convertToDictionary(SAXAttributes),  [])
                self._elemStk[-1][2].append(element)    
                self._elemStk.append(elementRecord)        
                return
        def characters(self, content): 
                if self._elemStk[-1][2] and isinstance(self._elemStk[-1][2][-1], basestring):
                        self._elemStk[-1][2][-1] = self._elemStk[-1][2][-1] + content;
                else:
                        self._elemStk[-1][2].append(content)
                return
        def endElementNS(self, name, qname):
                elementRecord = self._elemStk.pop()
                elementRecord[0].unmarshall(elementRecord[1], elementRecord[2])
                return
        @staticmethod
        def _convertToDictionary(SAXAttributes):
                return {k: SAXAttributes.getValueByQName(k) for k in SAXAttributes.getQNames() }

class _xmlelement:
        def __init__(self):
                self._content = []
                return
        def getContent(self):
                return self._content

class xml_myElem(_xmlelement):
        def __init__(self):
                _xmlelement.__init__(self)
                return
        def marshall(self, stream):
                stream.write('<myElem')
                stream.write('>')
                for node in self.getContent():
                        if isinstance(node, _xmlelement):
                                node.marshall(stream)
                        else:
                                stream.write(str(node))
                stream.write('</myElem>')
                return
        def unmarshall(self, attributes, content):
                self._content = content
                return

class xml_foo(_xmlelement):
        def __init__(self):
                _xmlelement.__init__(self)
                return
        def marshall(self, stream):
                stream.write('<foo')
                stream.write('>')
                for node in self.getContent():
                        if isinstance(node, _xmlelement):
                                node.marshall(stream)
                        else:
                                stream.write(str(node))
                stream.write('</foo>')
                return
        def unmarshall(self, attributes, content):
                self._content = content
                return



class xml_testA002(_handler, _xmlelement):
        def __init__(self):
                _handler.__init__(self)
                _xmlelement.__init__(self)
                return
        def marshall(self, stream):
                stream.write("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n")
                for elm in self.getContent():
                        elm.marshall(stream)
                return
        def unmarshall(self, stream):
                parser = xml.sax.make_parser()
                parser.setFeature(xml.sax.handler.feature_namespaces, 1)
                parser.setContentHandler(self)
                parser.parse(stream)
                self._content = self._elemStk[-1][2]
                return
```
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
  * python3 (build-time test generation; runtime for the python round-trip)
  * conan (`pip install 'conan<2'`) for the dependency path below

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
```
   ctest --test-dir build -C Release --output-on-failure
```

#### Installing / Uninstalling ####
```
   sudo cmake --install build --config Release            # uses CMAKE_INSTALL_PREFIX
   xargs rm < build/install_manifest.txt                  # to uninstall
```
Set the install location at configure time
(`-DCMAKE_INSTALL_PREFIX=/usr/local`); it is baked into the binary's template
search path.
