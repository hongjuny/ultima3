#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
APP=${1:-$ROOT_DIR/Build/Products/Release/Ultima III.app}
CHECK_DIR=$(mktemp -d /tmp/u3-gameplay.XXXXXX)
env U3_BASIC_PLAY=1 U3_GAMEPLAY_SCENARIO_CHECK=1 U3_DUNGEON_TEXT_CHECK="$CHECK_DIR/inscription" \
    U3_WORLD_RENDER_CHECK="$CHECK_DIR/world.png" U3_SAVE_DIRECTORY="$CHECK_DIR/save" \
    "$APP/Contents/MacOS/Ultima III"
env U3_BASIC_PLAY=1 U3_RESUME_CHECK=1 \
    U3_WORLD_INPUT_CHECK="$CHECK_DIR/resume.png" U3_SAVE_DIRECTORY="$CHECK_DIR/save" \
    "$APP/Contents/MacOS/Ultima III"
printf 'Gameplay scenario artifacts: %s\n' "$CHECK_DIR"
