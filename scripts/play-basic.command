#!/bin/sh
set -eu

APP_PATH="/tmp/ultima3-release-derived/Build/Products/Release/Ultima III.app"
if [ ! -x "$APP_PATH/Contents/MacOS/Ultima III" ]; then
    printf '%s\n' 'Run sh scripts/build-release-app.sh first.' >&2
    exit 1
fi
export U3_BASIC_PLAY=1
export U3_SAVE_DIRECTORY=/tmp/u3-basic-play-save
exec "$APP_PATH/Contents/MacOS/Ultima III"
