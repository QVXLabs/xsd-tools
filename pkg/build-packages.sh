#!/usr/bin/env bash
# Build the xsd-tools .deb and .rpm packages via CPack.
#
# Usage: pkg/build-packages.sh [VERSION]
#   VERSION  optional MAJOR.MINOR.PATCH (e.g. 0.2.0). Defaults to the value
#            CMake derives from `git describe`.
#
# Output: the packages are written to ./dist/ at the repo root.
#
# Requirements (Debian/Ubuntu): cmake (>=3.16), ninja or make, a C++11
# compiler, dpkg-dev (DEB shlibdeps) and rpm (the RPM generator's rpmbuild).
# boost/expat/lua/tinyxml are built in-tree, so no -dev packages are needed.
set -euo pipefail

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$repo_root"

version="${1:-}"
build_dir="$repo_root/build-pkg"
dist_dir="$repo_root/dist"

cfg_args=(
  -S . -B "$build_dir"
  -DCMAKE_BUILD_TYPE=Release
  -DBUILD_TESTING=OFF
  -DCMAKE_DISABLE_FIND_PACKAGE_Lua=ON
)
if command -v ninja >/dev/null 2>&1; then
  cfg_args+=(-G Ninja)
fi
if [ -n "$version" ]; then
  cfg_args+=(-DXSD_VERSION="$version")
fi

echo ">> configuring"
cmake "${cfg_args[@]}"
echo ">> building"
cmake --build "$build_dir" --config Release -j

echo ">> packaging (.deb, .rpm)"
mkdir -p "$dist_dir"
(
  cd "$build_dir"
  cpack -G DEB
  cpack -G RPM
)
find "$build_dir" -maxdepth 1 \( -name '*.deb' -o -name '*.rpm' \) \
  -exec mv -f {} "$dist_dir/" \;

echo ">> done. artifacts in $dist_dir:"
ls -1 "$dist_dir"/*.deb "$dist_dir"/*.rpm 2>/dev/null || true
