# Derive the project version. Priority:
#   1. -DXSD_VERSION=<x>  (release/tarball builds pass the tag explicitly)
#   2. `git describe --tags --match v*`  (working-tree dev builds)
#   3. fallback 0.0.0
# Sets XSD_VERSION       — numeric MAJOR.MINOR.PATCH, for project(VERSION ...)
#      XSD_VERSION_FULL  — descriptive (e.g. 0.2.0-5-gabcdef-dirty), for display
if(NOT DEFINED XSD_VERSION OR NOT XSD_VERSION)
  find_package(Git QUIET)
  if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/../.git")
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" describe --tags --match "v*" --always --dirty
      WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/.."
      OUTPUT_VARIABLE XSD_VERSION
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET)
  endif()
endif()
if(NOT XSD_VERSION)
  set(XSD_VERSION "0.0.0")
endif()

string(REGEX REPLACE "^v" "" XSD_VERSION "${XSD_VERSION}")
set(XSD_VERSION_FULL "${XSD_VERSION}")
if(XSD_VERSION_FULL MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)")
  set(XSD_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
else()
  # No usable tag (e.g. a bare commit SHA) — project() needs numerics.
  set(XSD_VERSION "0.0.0")
endif()
