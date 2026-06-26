# ---------------------------------------------------------------------------
# CPack configuration — produces the .deb and .rpm release artifacts directly
# from the install() rules, so the packaged file list and version can never
# drift from the build. Invoke from the build tree with:
#     cpack -G DEB      ->  xsd-tools_<ver>_<arch>.deb
#     cpack -G RPM      ->  xsd-tools-<ver>-<rel>.<arch>.rpm
# (pkg/build-packages.sh wraps configure + build + both generators.)
#
# Runtime dependencies are kept minimal on purpose: boost, lua, tinyxml and
# expat are statically linked from the in-tree build, so the only shared-object
# dependencies are the C/C++ runtime, which DEB shlibdeps / RPM autoreq detect
# automatically. (The old hand-written spec's tinyxml/boost/lua Requires were
# already stale for this reason.)
# ---------------------------------------------------------------------------

set(CPACK_PACKAGE_NAME "xsd-tools")
set(CPACK_PACKAGE_VENDOR "QVXLabs")
set(CPACK_PACKAGE_CONTACT "QVXLabs <agentic@qvxlabs.com>")
set(CPACK_PACKAGE_VERSION "${XSD_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/QVXLabs/xsd-tools")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
  "Generate marshalling code in many languages from XSD schema documents")
set(CPACK_PACKAGE_DESCRIPTION
  "xsd-tools generates marshalling/unmarshalling code from XML XSD schemas for\n\
C, C++, Python, Java and TypeScript (XML and JSON). A C++ core parses the\n\
schema and on-disk Lua templates emit the target-language code, so new output\n\
targets are template-only.")

# Install everything under /usr in the package (matches the legacy layout).
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
# Strip debug symbols from packaged binaries.
set(CPACK_STRIP_FILES ON)
# Only the host arch / single package; no source package.
set(CPACK_SOURCE_GENERATOR "")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
  set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
endif()

# --- Debian (.deb) -------------------------------------------------------
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "QVXLabs <agentic@qvxlabs.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${CPACK_PACKAGE_HOMEPAGE_URL}")
# auto-detect the libc/libstdc++ shared-lib dependencies
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
# xsd-tools_<ver>_<arch>.deb
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

# --- RPM (.rpm) ----------------------------------------------------------
set(CPACK_RPM_PACKAGE_LICENSE "GPLv3")
set(CPACK_RPM_PACKAGE_GROUP "Development/Tools")
set(CPACK_RPM_PACKAGE_URL "${CPACK_PACKAGE_HOMEPAGE_URL}")
# auto-detect runtime deps; don't hard-code stale Requires
set(CPACK_RPM_PACKAGE_AUTOREQ ON)
# xsd-tools-<ver>-<rel>.<arch>.rpm
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)

include(CPack)
