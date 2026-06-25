cmake_minimum_required(VERSION 3.16)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/GitVersion.cmake)
project(xsd-tools VERSION ${XSD_VERSION} LANGUAGES CXX C)

# Some vendored deps (googletest 1.11, expat R_2_4_1) declare a pre-3.5
# cmake_minimum_required, which CMake 4.x rejects. Treat such floors as 3.5.
if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

# Use extraction-time timestamps for FetchContent URL downloads, so a changed
# URL/hash triggers a rebuild (CMP0135, CMake >= 3.24).
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

# Keep module-mode find_package(Boost): the Conan cmake_find_package generator
# ships a FindBoost module, removed under CMP0167 NEW (CMake >= 3.30).
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 OLD)
endif()

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(XSD_WERROR "Treat warnings as errors (GCC/Clang only)" ON)
include(CTest)  # defines BUILD_TESTING (default ON)

# ---------------------------------------------------------------------------
# Language standards (C++ sources are C++11; the bundled lua + the generated
# C output build as C11). Pin both so the build is captured, not host-default.
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# C extensions stay ON (gnu11): the bundled lua needs POSIX (dlopen/popen).
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Build configurations. Single-config generators bake one type per build dir;
# prefer a multi-config generator (Ninja Multi-Config / VS / Xcode) so one
# configure yields both Debug and Release.
# ---------------------------------------------------------------------------
get_property(_isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_isMultiConfig)
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
  if(NOT CMAKE_DEFAULT_BUILD_TYPE)
    set(CMAKE_DEFAULT_BUILD_TYPE Release)
  endif()
elseif(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release)
endif()

# ---------------------------------------------------------------------------
# Let Conan's cmake_find_package modules (if present) satisfy find_package().
# `conan install` drops Find*.cmake / *-config files into the build tree.
# ---------------------------------------------------------------------------
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}")
list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")

# ---------------------------------------------------------------------------
# Warning / optimization flags, per-config so multi-config trees get them
# right.
# ---------------------------------------------------------------------------
# Strict warnings/opt flags apply ONLY to our own targets (link xsd_flags),
# never to vendored third-party/ code. CMake's per-config defaults already
# supply -O3 -DNDEBUG (Release) / -g (Debug) to everything.
add_library(xsd_flags INTERFACE)
if(MSVC)
  target_compile_options(xsd_flags INTERFACE /W3)
else()
  target_compile_options(xsd_flags INTERFACE
    -Wall -Winit-self -Wformat -Wno-format-nonliteral
    $<$<CONFIG:Release>:-fomit-frame-pointer>)
  if(XSD_WERROR)
    target_compile_options(xsd_flags INTERFACE -Werror)
  endif()
  # GCC/Clang version-specific suppressions present in the old build.
  include(CheckCXXCompilerFlag)
  foreach(_f -Wno-unused-local-typedefs -Wno-potentially-evaluated-expression)
    string(MAKE_C_IDENTIFIER "HAS${_f}" _flagvar)
    check_cxx_compiler_flag("${_f}" ${_flagvar})
    if(${_flagvar})
      target_compile_options(xsd_flags INTERFACE "${_f}")
    endif()
  endforeach()
  if(NOT APPLE)
    target_compile_options(xsd_flags INTERFACE
      $<$<CONFIG:Release>:-ffunction-sections>
      $<$<CONFIG:Release>:-fdata-sections>)
    add_link_options($<$<CONFIG:Release>:-Wl,--gc-sections>
                     $<$<CONFIG:Release>:-Wl,--discard-all>)
  endif()
endif()

# ---------------------------------------------------------------------------
# Dependencies: find_package first, FetchContent fallback. The fetched deps
# mirror the Conan recipes — upstream tarball (pinned by URL_HASH) with our
# build overlay from cmake/ copied in (PATCH_COMMAND), since neither lua.org
# nor the TinyXML release ships a CMakeLists. Each resolves into the
# normalized interface target xsd_<name>.
# ---------------------------------------------------------------------------

include(FetchContent)
# Fetched sources/build live under <build>/third-party, not the default _deps.
set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/third-party"
    CACHE PATH "Location for FetchContent sources" FORCE)

# Fetch a release tarball that ships no CMakeLists, optionally pre-applying
# source patches, then overlay our own build file and expose the result through
# an INTERFACE target.
#   fetch_third_party(<name> <url> <sha256> <overlay> <provided> <iface>
#                      [PATCH <file>...])
# Each PATCH is applied with `patch -p1`, in order, before the overlay is copied
# over the upstream tree's missing CMakeLists.txt.
function(fetch_third_party name url sha256 overlay provided iface)
  cmake_parse_arguments(ARG "" "" "PATCH" ${ARGN})
  set(patch_cmd "")
  foreach(p IN LISTS ARG_PATCH)
    list(APPEND patch_cmd patch -p1 -i "${p}" COMMAND)
  endforeach()
  list(APPEND patch_cmd ${CMAKE_COMMAND} -E copy_if_different
       "${overlay}" "<SOURCE_DIR>/CMakeLists.txt")
  FetchContent_Declare(${name}
    URL      "${url}"
    URL_HASH SHA256=${sha256}
    PATCH_COMMAND ${patch_cmd})
  FetchContent_MakeAvailable(${name})
  target_link_libraries(${iface} INTERFACE ${provided})
endfunction()

# -- Lua 5.1 (EXACT: code uses loadstring/setfenv, gone after 5.1) --
# Conan's cmake_find_package provides target Lua::Lua; CMake's builtin FindLua
# provides LUA_INCLUDE_DIR / LUA_LIBRARIES. Else fetch lua 5.1.5 and overlay
# our build files (which also yield the matching luac, below).
find_package(Lua 5.1 EXACT QUIET)
add_library(xsd_lua INTERFACE)
if(TARGET Lua::Lua)
  target_link_libraries(xsd_lua INTERFACE Lua::Lua)
elseif(LUA_FOUND)
  target_include_directories(xsd_lua INTERFACE ${LUA_INCLUDE_DIR})
  target_link_libraries(xsd_lua INTERFACE ${LUA_LIBRARIES})
else()
  # Apply the recipe's LNUM source patch + the unsigned-char lundump fix, then
  # overlay our build files. (The recipe's other patch is build-system only;
  # cmake/lua-CMakeLists.txt replaces it and reproduces its LNUM config.)
  fetch_third_party(lua
    https://www.lua.org/ftp/lua-5.1.5.tar.gz
    2640fc56a795f29d28ef15e13c34a47e223960b0240e8cb0a82d9b0738695333
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/lua-CMakeLists.txt"
    Lua::Lua xsd_lua
    PATCH
      "${CMAKE_CURRENT_SOURCE_DIR}/cmake/patches/luanum-5.1.5.patch"
      "${CMAKE_CURRENT_SOURCE_DIR}/cmake/patches/lundump-signedchar-5.1.5.patch")
  set(XSD_BUNDLED_LUA TRUE)
endif()

# -- TinyXML (the original TiXmlDocument API) --
find_package(TinyXML QUIET)
add_library(xsd_tinyxml INTERFACE)
if(TARGET TinyXML::TinyXML)
  target_link_libraries(xsd_tinyxml INTERFACE TinyXML::TinyXML)
elseif(TinyXML_FOUND OR TINYXML_FOUND)
  target_include_directories(xsd_tinyxml INTERFACE ${TinyXML_INCLUDE_DIRS})
  target_link_libraries(xsd_tinyxml INTERFACE ${TinyXML_LIBRARIES})
else()
  fetch_third_party(tinyxml
    https://downloads.sourceforge.net/project/tinyxml/tinyxml/2.6.2/tinyxml_2_6_2.tar.gz
    15bdfdcec58a7da30adc87ac2b078e4417dbe5392f3afb719f9ba6d062645593
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/tinyxml-CMakeLists.txt"
    TinyXML::TinyXML xsd_tinyxml)
endif()

# -- Boost (system + filesystem; foreach + algorithm/string are header-only) --
# find_package, else the third-party/boost superproject submodule via Boost's
# own CMake (needs `git submodule update --init --recursive third-party/boost`).
find_package(Boost QUIET COMPONENTS system filesystem)
add_library(xsd_boost INTERFACE)
if(Boost_FOUND)
  target_link_libraries(xsd_boost INTERFACE
    Boost::system Boost::filesystem)
elseif(TARGET boost::boost)
  target_link_libraries(xsd_boost INTERFACE boost::boost)
else()
  # The boost headers xsd-tools includes: filesystem, system, algorithm,
  # foreach, tokenizer (+ their transitive deps, resolved by boost's CMake).
  set(BOOST_INCLUDE_LIBRARIES filesystem system algorithm foreach tokenizer
      CACHE STRING "" FORCE)
  # The superproject add_subdirectory()s every lib; a few (e.g. local_function,
  # log, mpi) ship no CMakeLists at this tag and would error. Exclude any
  # lib dir lacking one.
  set(_boost_libs_dir "${CMAKE_CURRENT_SOURCE_DIR}/third-party/boost/libs")
  file(GLOB _boost_entries RELATIVE "${_boost_libs_dir}" "${_boost_libs_dir}/*")
  set(_boost_exclude "")
  foreach(_l ${_boost_entries})
    if(IS_DIRECTORY "${_boost_libs_dir}/${_l}"
       AND NOT EXISTS "${_boost_libs_dir}/${_l}/CMakeLists.txt")
      list(APPEND _boost_exclude "${_l}")
    endif()
  endforeach()
  set(BOOST_EXCLUDE_LIBRARIES "${_boost_exclude}" CACHE STRING "" FORCE)
  # Boost's superproject registers its own CTest suites for any lib in
  # BOOST_INCLUDE_LIBRARIES when BUILD_TESTING is on; we never build those exes
  # (EXCLUDE_FROM_ALL), so they'd show up in our ctest run as "Not Run". Must
  # override the *cache* entry: BoostRoot.cmake unset()s any normal BUILD_TESTING
  # var mid-loop, which would re-expose our cached ON. Restore it after, so our
  # own test/ subdir still registers.
  set(_xsd_build_testing "${BUILD_TESTING}")
  set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
  add_subdirectory(third-party/boost EXCLUDE_FROM_ALL)
  set(BUILD_TESTING "${_xsd_build_testing}" CACHE BOOL "" FORCE)
  target_link_libraries(xsd_boost INTERFACE Boost::filesystem Boost::system
    Boost::algorithm Boost::foreach Boost::tokenizer)
endif()

# -- incbin: header-only, vendored --
add_library(xsd_incbin INTERFACE)
target_include_directories(xsd_incbin INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/third-party/incbin")

# ---------------------------------------------------------------------------
# Lua engine -> luascript.luac (luac-compiled bytecode, embedded via incbin).
# MUST use a Lua 5.1 luac; the system one may be newer. Override with -DLUAC=.
# ---------------------------------------------------------------------------
# When Lua is vendored, use the luac built from the same sources (guaranteed
# bytecode-compatible with the linked liblua). Otherwise discover a 5.1 luac.
set(LUASCRIPT_LUAC_DEP "")
if(XSD_BUNDLED_LUA)
  set(LUAC "$<TARGET_FILE:luac>")
  set(LUASCRIPT_LUAC_DEP luac)
else()
  if(NOT LUAC)
    find_program(LUAC NAMES luac5.1 luac)
  endif()
  if(NOT LUAC)
    message(FATAL_ERROR "luac not found; set -DLUAC=/path/to/luac (Lua 5.1).")
  endif()
  execute_process(COMMAND "${LUAC}" -v
    OUTPUT_VARIABLE _luac_ver ERROR_VARIABLE _luac_ver)
  if(NOT _luac_ver MATCHES "5\\.1")
    message(WARNING
      "luac at '${LUAC}' reports: ${_luac_ver}\n"
      "xsd-tools needs Lua 5.1 bytecode. Activate the Conan env or pass "
      "-DLUAC=<lua5.1 luac>, or the embedded engine will fail to load at "
      "runtime.")
  endif()
endif()

set(LUA_ENGINE_SOURCES
  src/TemplateEngine/table.lua
  src/TemplateEngine/string.lua
  src/TemplateEngine/parser.lua
  src/TemplateEngine/safeEnv.lua
  src/TemplateEngine/stringbuffer.lua
  src/TemplateEngine/templateEngine.lua)
list(TRANSFORM LUA_ENGINE_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")

set(LUASCRIPT_LUAC "${CMAKE_BINARY_DIR}/luascript.luac")
# Strip debug info only in Release. COMMAND_EXPAND_LISTS drops the empty list
# element in other configs, so no stray "" argument reaches luac.
set(_luac_strip $<$<CONFIG:Release>:-s>)
add_custom_command(
  OUTPUT "${LUASCRIPT_LUAC}"
  COMMAND "${LUAC}" -o "${LUASCRIPT_LUAC}" ${_luac_strip} ${LUA_ENGINE_SOURCES}
  DEPENDS ${LUA_ENGINE_SOURCES} ${LUASCRIPT_LUAC_DEP}
  COMMENT "Compiling Lua template engine -> luascript.luac"
  VERBATIM COMMAND_EXPAND_LISTS)
add_custom_target(luascript DEPENDS "${LUASCRIPT_LUAC}")

# ---------------------------------------------------------------------------
# Version + prefix-aware runtime template path -> resource_config.hpp
# ---------------------------------------------------------------------------
include(GNUInstallDirs)
set(XSDTOOLS_VERSION "${XSD_VERSION_FULL}")
set(XSDTOOLS_GLOBAL_TEMPLATE_PATH
  "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/xsdtools/templates")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/resource_config.hpp.in"
  "${CMAKE_BINARY_DIR}/generated/resource_config.hpp" @ONLY)

# ---------------------------------------------------------------------------
# libxsdb: every src/*.cpp except main.cpp
# ---------------------------------------------------------------------------
set(LIBXSDB_SOURCES
  src/util.cpp
  src/luaScriptAdapter.cpp
  src/resource.cpp
  src/xsdtools.cpp
  src/XSDParser/Exception.cpp
  src/XSDParser/Parser.cpp
  src/XSDParser/Types.cpp
  src/XSDParser/TypesDB.cpp
  src/XSDParser/Elements/Attribute.cpp
  src/XSDParser/Elements/Choice.cpp
  src/XSDParser/Elements/ComplexType.cpp
  src/XSDParser/Elements/Element.cpp
  src/XSDParser/Elements/List.cpp
  src/XSDParser/Elements/Node.cpp
  src/XSDParser/Elements/Restriction.cpp
  src/XSDParser/Elements/Schema.cpp
  src/XSDParser/Elements/Sequence.cpp
  src/XSDParser/Elements/SimpleType.cpp
  src/XSDParser/Elements/Union.cpp
  src/XSDParser/Elements/Group.cpp
  src/XSDParser/Elements/Any.cpp
  src/XSDParser/Elements/ComplexContent.cpp
  src/XSDParser/Elements/Extension.cpp
  src/XSDParser/Elements/SimpleContent.cpp
  src/XSDParser/Elements/MinExclusive.cpp
  src/XSDParser/Elements/MaxExclusive.cpp
  src/XSDParser/Elements/MinInclusive.cpp
  src/XSDParser/Elements/MaxInclusive.cpp
  src/XSDParser/Elements/MinLength.cpp
  src/XSDParser/Elements/MaxLength.cpp
  src/XSDParser/Elements/Length.cpp
  src/XSDParser/Elements/FractionDigits.cpp
  src/XSDParser/Elements/Pattern.cpp
  src/XSDParser/Elements/TotalDigits.cpp
  src/XSDParser/Elements/WhiteSpace.cpp
  src/XSDParser/Elements/Enumeration.cpp
  src/XSDParser/Elements/AttributeGroup.cpp
  src/XSDParser/Elements/Include.cpp
  src/XSDParser/Elements/Annotation.cpp
  src/XSDParser/Elements/Documentation.cpp
  src/XSDParser/Elements/All.cpp
  src/XSDParser/Elements/AppInfo.cpp
  src/Processors/ArrayType.cpp
  src/Processors/LuaProcessorBase.cpp
  src/Processors/LuaAdapter.cpp
  src/Processors/LuaProcessor.cpp
  src/Processors/SimpleTypeExtracter.cpp
  src/Processors/ElementExtracter.cpp
  src/Processors/RestrictionVerify.cpp)

add_library(xsdb-lib STATIC ${LIBXSDB_SOURCES})
add_dependencies(xsdb-lib luascript)
# Build-tree include roots resolve our "./src/..." includes and the generated
# resource_config.hpp. Wrapped in BUILD_INTERFACE so they don't leak into the
# installed export; consumers get the install-tree include dir added below.
target_include_directories(xsdb-lib PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/generated>")
# Deps are needed to build the archive but xsdb-lib is static; consumers
# relink their own deps. BUILD_INTERFACE keeps the export self-contained.
target_link_libraries(xsdb-lib PUBLIC
  "$<BUILD_INTERFACE:xsd_lua>"
  "$<BUILD_INTERFACE:xsd_tinyxml>"
  "$<BUILD_INTERFACE:xsd_boost>"
  PRIVATE
  "$<BUILD_INTERFACE:xsd_incbin>"
  "$<BUILD_INTERFACE:xsd_flags>")

# -- Embed luascript.luac into resource.cpp --
if(MSVC)
  # MSVC lacks the assembler `.incbin` directive. Build incbin's bundled
  # generator and expand the blob into a compilable C source. (Validated in
  # Windows CI; see plan.)
  add_executable(incbin-tool "${CMAKE_CURRENT_SOURCE_DIR}/third-party/incbin/incbin.c")
  set(_incbin_data "${CMAKE_BINARY_DIR}/generated/luascript_data.c")
  add_custom_command(
    OUTPUT "${_incbin_data}"
    COMMAND incbin-tool "${CMAKE_CURRENT_SOURCE_DIR}/src/resource.cpp"
            -I "${CMAKE_CURRENT_SOURCE_DIR}/third-party/incbin"
            -o "${_incbin_data}"
    DEPENDS "${LUASCRIPT_LUAC}" incbin-tool
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "incbin: expanding luascript.luac for MSVC" VERBATIM)
  target_sources(xsdb-lib PRIVATE "${_incbin_data}")
  target_compile_definitions(xsdb-lib PRIVATE INCBIN_PREFIX=_binary_)
else()
  # GCC/Clang: the `.incbin` is resolved by the assembler. Point it at the
  # binary dir (where luascript.luac is generated); the compile working dir is
  # also the binary dir, so a relative lookup succeeds too.
  set_source_files_properties(src/resource.cpp PROPERTIES
    OBJECT_DEPENDS "${LUASCRIPT_LUAC}"
    COMPILE_OPTIONS "-Wa,-I${CMAKE_BINARY_DIR}")
endif()

# ---------------------------------------------------------------------------
# xsdb executable (thin main)
# ---------------------------------------------------------------------------
add_executable(xsdb src/main.cpp)
target_link_libraries(xsdb PRIVATE xsdb-lib xsd_flags)

# Consumers link the public Generate()/SplitMarkedFiles() API via this alias.
add_library(xsdtools::xsdtools ALIAS xsdb-lib)

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
install(TARGETS xsdb RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/templates/"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/xsdtools/templates")

# ---------------------------------------------------------------------------
# Library export: downstream C++ links xsdtools::xsdtools for Generate() /
# SplitMarkedFiles() and includes <xsdtools/xsdtools.hpp>.
# ---------------------------------------------------------------------------
target_include_directories(xsdb-lib PUBLIC
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/xsdtools.hpp"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/xsdtools")
install(TARGETS xsdb-lib EXPORT xsdtools-targets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(EXPORT xsdtools-targets
  FILE xsdtools-targets.cmake
  NAMESPACE xsdtools::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/xsdtools")

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(BUILD_TESTING)
  add_subdirectory(test)
endif()
