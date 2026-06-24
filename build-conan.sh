#!/bin/bash
#
# Conan + CMake build driver for xsd-tools.
#
# Resolves dependencies with Conan (generating cmake_find_package modules +
# a virtualenv that puts the Lua 5.1 luac on PATH), then configures a
# multi-config CMake tree and builds the requested configuration.

set -euo pipefail

HOST_PROFILE_DBG="${HOST_PROFILE:-$(uname -s)-$(uname -m)-Debug}"
HOST_PROFILE_REL="${HOST_PROFILE:-$(uname -s)-$(uname -m)-Release}"
BUILD_PROFILE="${BUILD_PROFILE:-$(uname -s)-$(uname -m)-Release}"
BUILD_DIR="${BUILD_DIR:-build}"

function num_cpus() {
    if [[ "Darwin" == "$(uname -s)" ]]; then
        sysctl -n hw.ncpu
    else
        nproc
    fi
}

function usage() {
    cat <<'EOF'
xsd-tools conan + cmake build script

    Usage
    build-conan.sh [Debug|Release|clean]
         Resolves deps via Conan and builds the chosen configuration
         (multi-config tree, so both configs share one build dir), or
         cleans the build tree.

    Environment Variables
    HOST_PROFILE   Conan profile defining the host environment.
    BUILD_PROFILE  Conan profile defining the build environment.
    BUILD_DIR      CMake build directory (default: build).

    Examples
        ./build-conan.sh Release
        ./build-conan.sh Debug
        ./build-conan.sh clean
EOF
}

# Run conan install for one build_type, then configure+build that config.
function build_config() {
    local config="$1" host_profile="$2"
    echo "Compiling ${config}"
    echo "    host profile : ${host_profile}"
    echo "    build profile: ${BUILD_PROFILE}"
    echo "    ncpus        : $(num_cpus)"
    mkdir -p "${BUILD_DIR}"
    conan install . \
        -pr:h "${host_profile}" \
        -pr:b "${BUILD_PROFILE}" \
        --build=outdated \
        --build=missing \
        -if "${BUILD_DIR}"
    # The virtualenv puts the Lua 5.1 luac (and tool bins) on PATH.
    source "${BUILD_DIR}/activate.sh"
    cmake -S . -B "${BUILD_DIR}" -G "Ninja Multi-Config"
    cmake --build "${BUILD_DIR}" --config "${config}" -j "$(num_cpus)"
    ctest --test-dir "${BUILD_DIR}" -C "${config}" --output-on-failure || true
    source "${BUILD_DIR}/deactivate.sh"
}

if [[ $# -eq 0 ]]; then
    usage
    exit 0
fi

case "$1" in
    Release) build_config Release "${HOST_PROFILE_REL}" ;;
    Debug)   build_config Debug   "${HOST_PROFILE_DBG}" ;;
    clean)
        rm -rf "${BUILD_DIR}"
        git clean -d -f
        ;;
    *) usage ;;
esac
