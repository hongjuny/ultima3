#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
APP=${1:-$ROOT_DIR/Build/Products/Release/Ultima III.app}
CHECK_DIR=$(mktemp -d /tmp/u3-command-text.XXXXXX)

env U3_BASIC_PLAY=1 U3_COMMAND_TEXT_CHECK="$CHECK_DIR/classic" \
    U3_WORLD_RENDER_CHECK="$CHECK_DIR/world.png" U3_SAVE_DIRECTORY="$CHECK_DIR/save-classic" \
    "$APP/Contents/MacOS/Ultima III"
env U3_MODERN_TEXT_CHECK="$CHECK_DIR/roster.png" U3_COMMAND_TEXT_CHECK="$CHECK_DIR/modern" \
    U3_WORLD_RENDER_CHECK="$CHECK_DIR/world.png" U3_SAVE_DIRECTORY="$CHECK_DIR/save-modern" \
    "$APP/Contents/MacOS/Ultima III"
printf 'Command/text screenshots: %s\n' "$CHECK_DIR"
