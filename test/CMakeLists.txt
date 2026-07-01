# ---------------------------------------------------------------------------
# xsdb-test : GoogleTest suite — hand-written internals tests + auto-generated
# per-XSD round-trip cases. Follows libqmg's find-or-vendor GTest pattern.
# ---------------------------------------------------------------------------

find_package(GTest QUIET)
if(NOT TARGET GTest::gtest_main)
  message(STATUS "GTest: building third-party/googletest submodule")
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  add_subdirectory("${CMAKE_SOURCE_DIR}/third-party/googletest"
                   "${CMAKE_BINARY_DIR}/googletest" EXCLUDE_FROM_ALL)
  if(NOT TARGET GTest::gtest_main)
    add_library(GTest::gtest_main ALIAS gtest_main)
  endif()
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Expat — only the generated C round-trip needs it. find_package, else the
# third-party/expat submodule (built static).
find_package(Expat QUIET)
set(EXPAT_BUNDLED FALSE)
if(NOT Expat_FOUND AND NOT TARGET Expat::Expat)
  message(STATUS "Expat: building third-party/expat submodule")
  set(EXPAT_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
  set(EXPAT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(EXPAT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(EXPAT_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(EXPAT_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  add_subdirectory("${CMAKE_SOURCE_DIR}/third-party/expat/expat"
                   "${CMAKE_BINARY_DIR}/expat-build" EXCLUDE_FROM_ALL)
  set(EXPAT_BUNDLED TRUE)
endif()

# json-c — only the generated C→JSON round-trip needs it. find_package first
# (our Conan recipe / system provides json-c::json-c), else FetchContent the
# upstream 0.17 tarball (ships its own CMake; no submodule).
find_package(json-c QUIET)
set(JSONC_BUNDLED FALSE)
if(NOT TARGET json-c::json-c AND NOT json-c_FOUND)
  message(STATUS "json-c: fetching json-c 0.17")
  include(FetchContent)
  # Save/restore BUILD_TESTING: json-c's CMake reads it, but it's also the
  # top-level gate that add_subdirectory()s this dir on reconfigure.
  set(_xsd_save_build_testing "${BUILD_TESTING}")
  set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  set(DISABLE_WERROR ON CACHE BOOL "" FORCE)
  FetchContent_Declare(jsonc
    URL      "https://s3.amazonaws.com/json-c_releases/releases/json-c-0.17.tar.gz"
    URL_HASH SHA256=7550914d58fb63b2c3546f3ccfbe11f1c094147bd31a69dcd23714d7956159e6)
  FetchContent_MakeAvailable(jsonc)
  set(BUILD_TESTING "${_xsd_save_build_testing}" CACHE BOOL "" FORCE)
  set(JSONC_BUNDLED TRUE)
endif()

# -- json-c include/link flags for the popen-driven C compile. Bundled headers
#    are flat in the source tree (umbrella json.h) with json_config.h/config.h
#    generated into the binary tree; pass both. roundtrip_util splits the dir
#    list on '|' (a ';' would split the -D, being a CMake list separator). --
if(JSONC_BUNDLED)
  set(JSONC_INCLUDE_DIR_VAL "${jsonc_SOURCE_DIR}|${jsonc_BINARY_DIR}")
  set(JSONC_LINK_VAL "$<TARGET_FILE:json-c>")
  set(JSONC_EXTRA_DEP json-c)
else()
  set(JSONC_EXTRA_DEP "")
  if(json-c_INCLUDE_DIRS)
    set(JSONC_INCLUDE_DIR_VAL "${json-c_INCLUDE_DIRS}")
  elseif(TARGET json-c::json-c)
    get_target_property(JSONC_INCLUDE_DIR_VAL json-c::json-c
                        INTERFACE_INCLUDE_DIRECTORIES)
  endif()
  if(JSONC_INCLUDE_DIR_VAL MATCHES "-NOTFOUND$")
    set(JSONC_INCLUDE_DIR_VAL "")
  endif()
  string(REPLACE ";" "|" JSONC_INCLUDE_DIR_VAL "${JSONC_INCLUDE_DIR_VAL}")
  if(TARGET json-c::json-c)
    set(JSONC_LINK_VAL "$<TARGET_FILE:json-c::json-c>")
  elseif(json-c_LIBRARIES)
    string(REPLACE ";" " " JSONC_LINK_VAL "${json-c_LIBRARIES}")
  else()
    set(JSONC_LINK_VAL "-ljson-c")
  endif()
endif()

set(CORPUS_DIR   "${CMAKE_CURRENT_SOURCE_DIR}/xsd-positive")
set(NEGATIVE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/xsd-negative")
set(XSDPARSE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/xsdparse")
set(TEMPLATES_DIR "${CMAKE_SOURCE_DIR}/templates")

# -- libb64 (used by the C round-trip's generated code) --
# Build via libb64's own CMake: 2.1.0 split encode/decode across SIMD TUs and
# needs the generated b64/version.h, so the old two-file build no longer links.
# EXCLUDE_FROM_ALL keeps libb64's CLI/examples/install rules out of this build.
# The b64 target's PUBLIC includes cover both the source headers and the
# generated version.h; join them with '|' for includeFlags() in
# roundtrip_util.cpp, which compiles the generated C against libb64.
add_subdirectory(libb64 EXCLUDE_FROM_ALL)
get_target_property(_libb64_incs b64 INTERFACE_INCLUDE_DIRECTORIES)
string(REPLACE ";" "|" LIBB64_INCLUDE_DIR_VAL "${_libb64_incs}")

# -- Expat include/link flags for the popen-driven C compile --
set(EXPAT_EXTRA_DEP "")
if(EXPAT_BUNDLED)
  set(EXPAT_INCLUDE_DIR_VAL "${CMAKE_SOURCE_DIR}/third-party/expat/expat/lib")
  set(EXPAT_LINK_VAL "$<TARGET_FILE:expat>")
  set(EXPAT_EXTRA_DEP expat)
else()
  # CMake's builtin FindEXPAT sets EXPAT_* and target EXPAT::EXPAT; Conan's
  # cmake_find_package generator sets Expat_* and target Expat::Expat.
  if(EXPAT_INCLUDE_DIR)
    set(EXPAT_INCLUDE_DIR_VAL "${EXPAT_INCLUDE_DIR}")
  else()
    set(EXPAT_INCLUDE_DIR_VAL "${Expat_INCLUDE_DIR}")
  endif()
  foreach(_expat_tgt EXPAT::EXPAT Expat::Expat)
    if(NOT EXPAT_INCLUDE_DIR_VAL AND TARGET ${_expat_tgt})
      get_target_property(EXPAT_INCLUDE_DIR_VAL ${_expat_tgt}
                          INTERFACE_INCLUDE_DIRECTORIES)
    endif()
  endforeach()
  # An unset property yields <var>-NOTFOUND; expat on a standard path needs no
  # -I, so treat that as empty (roundtrip_util.cpp omits the flag).
  if(NOT EXPAT_INCLUDE_DIR_VAL OR EXPAT_INCLUDE_DIR_VAL MATCHES "-NOTFOUND$")
    set(EXPAT_INCLUDE_DIR_VAL "")
  endif()
  if(EXPAT_LIBRARIES)
    string(REPLACE ";" " " EXPAT_LINK_VAL "${EXPAT_LIBRARIES}")
  else()
    string(REPLACE ";" " " EXPAT_LINK_VAL "${Expat_LIBRARIES}")
  endif()
  if(NOT EXPAT_LINK_VAL)
    set(EXPAT_LINK_VAL "-lexpat")
  endif()
endif()

# The round-trip harness (roundtrip_util.cpp) is POSIX-only (popen/mkdtemp/
# system/shell quoting) and the round-trips shell out to cc/mvn/python/node;
# generateTests lists goldens via dirent. On Windows, build only the
# parser/codegen/resource tests (no POSIX) — they exercise the library and
# every target's code generation; the round-trips run on the POSIX runners.
if(WIN32)
  add_executable(xsdb-test
    facetTests.cpp
    parserTests.cpp
    resourceTests.cpp)
else()
  # -- Auto-generate the round-trip .cpp at configure time. Re-run configure
  #    when the corpus changes. --
  file(GLOB _corpus_xsds CONFIGURE_DEPENDS "${CORPUS_DIR}/*.xsd")
  set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated-tests")
  execute_process(
    COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/gen_roundtrip_tests.py"
            "${CORPUS_DIR}" "${GEN_DIR}"
    RESULT_VARIABLE _gen_rc)
  if(NOT _gen_rc EQUAL 0)
    message(FATAL_ERROR "gen_roundtrip_tests.py failed (${_gen_rc})")
  endif()
  set(GENERATED_ROUNDTRIP_SOURCES
    "${GEN_DIR}/roundtrip_python_sax.cpp"
    "${GEN_DIR}/roundtrip_python_json.cpp"
    "${GEN_DIR}/roundtrip_c_xml_expat.cpp"
    "${GEN_DIR}/roundtrip_c_xml_expat_dom.cpp"
    "${GEN_DIR}/roundtrip_c_json.cpp"
    "${GEN_DIR}/roundtrip_java_json.cpp"
    "${GEN_DIR}/roundtrip_java_xml.cpp"
    "${GEN_DIR}/roundtrip_cpp_xml.cpp"
    "${GEN_DIR}/roundtrip_cpp_json.cpp"
    "${GEN_DIR}/roundtrip_ts_xml.cpp")

  # node/tsc are probed at run time (programAvailable) so cases SKIP cleanly
  # when absent; detect at configure too for an informative status line.
  find_program(NODE_EXECUTABLE node)
  find_program(NPM_EXECUTABLE npm)
  if(NOT NODE_EXECUTABLE OR NOT NPM_EXECUTABLE)
    message(STATUS "ts-xml round-trip: node/npm not found; cases will SKIP")
  endif()

  add_executable(xsdb-test
    roundtrip_util.cpp
    generateTests.cpp
    facetTests.cpp
    parserTests.cpp
    resourceTests.cpp
    ${GENERATED_ROUNDTRIP_SOURCES})
  add_dependencies(xsdb-test b64 ${EXPAT_EXTRA_DEP} ${JSONC_EXTRA_DEP})
endif()
target_include_directories(xsdb-test PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}")
target_link_libraries(xsdb-test PRIVATE xsdb-lib GTest::gtest_main xsd_flags)
target_compile_definitions(xsdb-test PRIVATE
  TEMPLATES_DIR="${TEMPLATES_DIR}"
  XSD_CORPUS_DIR="${CORPUS_DIR}"
  XSD_NEGATIVE_DIR="${NEGATIVE_DIR}"
  QUICKSTART_DIR="${CMAKE_SOURCE_DIR}/examples/quickstart"
  XSDPARSE_DIR="${XSDPARSE_DIR}"
  C_COMPILER="${CMAKE_C_COMPILER}"
  CXX_COMPILER="${CMAKE_CXX_COMPILER}"
  LIBB64_INCLUDE_DIR="${LIBB64_INCLUDE_DIR_VAL}"
  LIBB64_ARCHIVE="$<TARGET_FILE:b64>"
  EXPAT_INCLUDE_DIR="${EXPAT_INCLUDE_DIR_VAL}"
  EXPAT_LINK="${EXPAT_LINK_VAL}"
  JSONC_INCLUDE_DIR="${JSONC_INCLUDE_DIR_VAL}"
  JSONC_LINK="${JSONC_LINK_VAL}"
  JAVA_POM="${CMAKE_CURRENT_SOURCE_DIR}/java-json.org/pom.xml"
  JAVA_XML_POM="${CMAKE_CURRENT_SOURCE_DIR}/java-xml-stax/pom.xml"
  TS_PACKAGE_JSON="${CMAKE_CURRENT_SOURCE_DIR}/ts-xml/package.json"
  TS_TSCONFIG="${CMAKE_CURRENT_SOURCE_DIR}/ts-xml/tsconfig.json")

include(GoogleTest)
gtest_discover_tests(xsdb-test)
