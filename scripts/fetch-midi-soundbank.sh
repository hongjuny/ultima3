#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
DESTINATION="$ROOT_DIR/Resources/MusicMIDI/FluidR3_GM.sf2"
URL='https://raw.githubusercontent.com/urish/cinto/master/media/FluidR3%20GM.sf2'
EXPECTED_SHA256='eebdac4b1f95625bf2902b77838769111cb478fb9d2c66aa8e9a8ae80fe7d0c0'

if [ "${U3_SKIP_FLUIDR3_DOWNLOAD:-0}" = "1" ]; then
    printf '%s\n' 'FluidR3 download skipped; using the bundled fallback bank.'
    exit 0
fi

if [ -f "$DESTINATION" ] && [ "$(shasum -a 256 "$DESTINATION" | awk '{print $1}')" = "$EXPECTED_SHA256" ]; then
    printf '%s\n' "FluidR3 GM already available: $DESTINATION"
    exit 0
fi

temporary=$(mktemp "${TMPDIR:-/tmp}/u3-fluidr3.XXXXXX")
trap 'rm -f "$temporary"' EXIT INT TERM
printf '%s\n' 'Downloading FluidR3 GM SoundFont...'
curl --fail --location --retry 3 --connect-timeout 15 --output "$temporary" "$URL"
actual=$(shasum -a 256 "$temporary" | awk '{print $1}')
if [ "$actual" != "$EXPECTED_SHA256" ]; then
    printf '%s\n' "FluidR3 checksum mismatch: $actual" >&2
    exit 1
fi
mv "$temporary" "$DESTINATION"
printf '%s\n' "Installed FluidR3 GM: $DESTINATION"
