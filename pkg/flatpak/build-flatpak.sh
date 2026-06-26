#!/usr/bin/env bash
# Build a single-file xsd-tools Flatpak bundle.
#
# Usage: pkg/flatpak/build-flatpak.sh [VERSION]
#   VERSION  optional MAJOR.MINOR.PATCH; defaults to `git describe`.
#
# Output: dist/xsd-tools-<version>.flatpak
#
# Requires: flatpak and flatpak-builder. The freedesktop Platform/Sdk runtimes
# are installed from flathub if missing. Run after a `git submodule update
# --init --recursive` (the build uses the in-tree boost/expat submodules).
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "$here/../.." && pwd)"
app_id="com.qvxlabs.xsd_tools"
runtime_ver="24.08"

version="${1:-}"
if [ -z "$version" ]; then
  version="$(git -C "$repo_root" describe --tags --match 'v*' --always \
    2>/dev/null | sed 's/^v//' || true)"
fi
version="${version:-0.0.0}"

dist_dir="$repo_root/dist"
mkdir -p "$dist_dir"

# Ensure flathub + the runtime/sdk are available (user installation).
flatpak --user remote-add --if-not-exists flathub \
  https://flathub.org/repo/flathub.flatpakrepo
flatpak --user install -y flathub \
  "org.freedesktop.Platform//$runtime_ver" \
  "org.freedesktop.Sdk//$runtime_ver" || true

# Generate a manifest beside the original (so the relative `dir` source still
# resolves to the repo root) with the version substituted in.
gen_manifest="$here/.${app_id}.generated.yaml"
sed "s/@XSD_VERSION@/${version}/g" "$here/${app_id}.yaml" > "$gen_manifest"
trap 'rm -f "$gen_manifest"' EXIT

# Keep flatpak-builder's state outside the repo so the `dir` source can't copy
# its own output back into the build.
work="$(mktemp -d)"
# --disable-rofiles-fuse: CI runners often lack FUSE, which rofiles-fuse needs.
flatpak-builder --user --force-clean --disable-rofiles-fuse \
  --install-deps-from=flathub \
  --state-dir "$work/state" --repo "$work/repo" \
  "$work/build" "$gen_manifest"

# Single-file bundle (default app branch is 'master').
flatpak build-bundle "$work/repo" \
  "$dist_dir/xsd-tools-${version}.flatpak" "$app_id"

rm -rf "$work"
echo ">> built $dist_dir/xsd-tools-${version}.flatpak"
