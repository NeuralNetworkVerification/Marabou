#!/usr/bin/env bash
set -euo pipefail

# cibuildwheel sets MACOSX_DEPLOYMENT_TARGET before invoking this script.
# Keep GMP shared so delocate can bundle it with the macOS wheel.
prefix="${1:?Usage: build_gmp_for_wheels.sh absolute-install-prefix}"
case "$prefix" in
    /*) ;;
    *) echo "The installation prefix must be absolute" >&2; exit 1 ;;
esac

build_dir=$(mktemp -d)
trap 'rm -rf "$build_dir"' EXIT
cd "$build_dir"

version=6.3.0
archive="gmp-$version.tar.xz"
curl --fail --location --retry 3 "https://ftp.gnu.org/gnu/gmp/$archive" -o "$archive"
echo "a3c2b80201b89e68616f4ad30bc66aee4927c3ce50e33929ca819d5c43538898  $archive" | shasum -a 256 -c -
tar -xf "$archive"
cd "gmp-$version"
./configure --prefix="$prefix" --enable-cxx --enable-shared --disable-static
make -j2
make install
