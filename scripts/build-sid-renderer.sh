#!/bin/sh
# Build the GPL offline renderer in a caller-selected scratch directory.
# No installation, GUI, hardware audio, or app-bundle changes are performed.
set -eu

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 EMPTY_OR_NEW_BUILD_DIRECTORY" >&2
    exit 2
fi
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
mkdir -p -- "$1"
build_dir=$(CDPATH= cd -- "$1" && pwd)
if [ -n "$(ls -A "$build_dir")" ]; then
    echo "Build directory must be empty: $build_dir" >&2
    exit 1
fi
archive="$build_dir/libsidplayfp-2.12.0.tar.gz"
url='https://github.com/libsidplayfp/libsidplayfp/releases/download/v2.12.0/libsidplayfp-2.12.0.tar.gz'
expected='bc4f4fa203dcf0736fe48c23dce9aa0db825370e5941e7595e4851efe6937cdc'
curl --fail --location --proto '=https' --tlsv1.2 "$url" --output "$archive"
actual=$(shasum -a 256 "$archive" | awk '{print $1}')
if [ "$actual" != "$expected" ]; then
    echo 'libsidplayfp source archive hash mismatch' >&2
    exit 1
fi
tar -xzf "$archive" -C "$build_dir"
source_dir="$build_dir/libsidplayfp-2.12.0"
(cd "$source_dir" && patch -p1 < "$script_dir/sid-deterministic-lut.patch")
(cd "$source_dir" && ./configure --disable-shared --enable-static --disable-hardsid --without-exsid --prefix="$build_dir/local" > "$build_dir/configure.log" 2>&1)
(cd "$source_dir" && make -j4 > "$build_dir/make.log" 2>&1)
"${CXX:-c++}" -std=c++20 -O2 -I "$source_dir/src" "$script_dir/render-sid.cpp" \
    "$source_dir/src/.libs/libsidplayfp.a" -o "$build_dir/render-sid"
echo "Renderer: $build_dir/render-sid"
echo "Source, GPL license, applied patch and build logs are retained in $build_dir"
