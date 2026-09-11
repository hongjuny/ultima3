#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
DERIVED_DATA=${DERIVED_DATA:-$ROOT_DIR/Build/DerivedData}
APP_PATH="$ROOT_DIR/Build/Products/Release/Ultima III.app"

cd "$ROOT_DIR"
"$ROOT_DIR/scripts/fetch-midi-soundbank.sh"
xcodebuild -project Ultima3.xcodeproj \
    -scheme Ultima3 \
    -configuration Release \
    -derivedDataPath "$DERIVED_DATA" \
    -arch arm64 \
    CODE_SIGNING_ALLOWED=NO \
    GCC_PREPROCESSOR_DEFINITIONS=APPSTORE \
    build

file "$APP_PATH/Contents/MacOS/Ultima III"
printf '%s\n' "Release app: $APP_PATH"
