# Derive the project version. The base version is the single source of truth in
# the top-level VERSION.txt file; git only decorates non-release dev builds.
#
# Priority:
#   1. -DXSD_VERSION=<x>   a published release sets this to the release tag
#                          (leading 'v' stripped below); used verbatim, clean.
#   2. VERSION.txt         base M.M.P, plus a git dev-suffix when the tree
#                          isn't a clean release of that base.
#   3. fallback 0.0.0      no override and no VERSION.txt.
#
# Sets XSD_VERSION       — numeric MAJOR.MINOR.PATCH, for project(VERSION ...)
#      XSD_VERSION_FULL  — descriptive string for display / --version

# VERSION.txt, not VERSION: the repo root is on the compiler include path (for
# the "./src/..." includes), and a root file named VERSION shadows the C++
# standard <version> header on case-insensitive filesystems (Windows/macOS),
# breaking the build.
set(_xsd_version_file "${CMAKE_CURRENT_LIST_DIR}/../VERSION.txt")

# Re-run CMake when the authority file changes, so version.h regenerates on the
# next build instead of going stale.
if(EXISTS "${_xsd_version_file}")
  set_property(DIRECTORY APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS "${_xsd_version_file}")
endif()

if(DEFINED XSD_VERSION AND XSD_VERSION)
  # Explicit override (release/tarball) — use verbatim, no git decoration.
  set(XSD_VERSION_FULL "${XSD_VERSION}")
elseif(EXISTS "${_xsd_version_file}")
  file(STRINGS "${_xsd_version_file}" _xsd_base LIMIT_COUNT 1)
  string(STRIP "${_xsd_base}" _xsd_base)
  set(XSD_VERSION "${_xsd_base}")
  set(XSD_VERSION_FULL "${_xsd_base}")
  # Decorate dev builds: commits past the v<base> tag, and/or a dirty tree.
  find_package(Git QUIET)
  if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/../.git")
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
      WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/.."
      OUTPUT_VARIABLE _xsd_sha OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    # Commit count since the matching v<base> tag; empty when no such tag.
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" rev-list "v${_xsd_base}..HEAD" --count
      WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/.."
      OUTPUT_VARIABLE _xsd_count OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" status --porcelain
      WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/.."
      OUTPUT_VARIABLE _xsd_dirty OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    set(_xsd_suffix "")
    if(_xsd_count AND NOT _xsd_count STREQUAL "0")
      # commits since v<base>
      set(_xsd_suffix "+${_xsd_count}.g${_xsd_sha}")
    elseif(NOT _xsd_count AND _xsd_sha)
      # no v<base> tag yet — still a dev build, mark by sha
      set(_xsd_suffix "+g${_xsd_sha}")
    endif()
    if(_xsd_dirty)
      if(_xsd_suffix STREQUAL "")
        set(_xsd_suffix "+g${_xsd_sha}.dirty")
      else()
        set(_xsd_suffix "${_xsd_suffix}.dirty")
      endif()
    endif()
    set(XSD_VERSION_FULL "${_xsd_base}${_xsd_suffix}")
  endif()
else()
  set(XSD_VERSION "0.0.0")
  set(XSD_VERSION_FULL "0.0.0")
endif()

# Normalize XSD_VERSION to a numeric MAJOR.MINOR.PATCH for project(VERSION ...).
string(REGEX REPLACE "^v" "" XSD_VERSION "${XSD_VERSION}")
if(XSD_VERSION MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)")
  set(XSD_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
else()
  # e.g. a bare sha or non-numeric override — project() needs numerics.
  set(XSD_VERSION "0.0.0")
endif()
