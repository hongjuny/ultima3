#!/bin/sh
set -eu

status=0

check_absent() {
    description=$1
    pattern=$2
    shift 2

    if rg -n "$pattern" "$@"; then
        echo "error: $description"
        status=1
    fi
}

runtime_files="
Sources/UltimaMain.c
Sources/UltimaMisc.c
Sources/UltimaSpellCombat.c
Sources/UltimaAutocombat.c
Sources/UltimaNew.c
Sources/UltimaText.c
Sources/UltimaGraphics.c
Sources/UltimaMacIF.c
Sources/UltimaSound.m
"

check_absent \
    "runtime files should not access preference storage directly" \
    "CFPreferences|NSUserDefaults|NumberForPrefsKey" \
    $runtime_files

check_absent \
    "core-like files should not call legacy input/timing APIs directly" \
    "\\b(WaitKeyMouse|GetKeyMouse|CursorKey|ThreadSleepTicks|RandNum)[[:space:]]*\\(" \
    Sources/UltimaAutocombat.c \
    Sources/UltimaNew.c \
    Sources/UltimaSpellCombat.c

check_absent \
    "UltimaMisc should not open or update resource files directly" \
    "\\b(FSpOpenResFile|FSpCreateResFile|FSReadFork|FSWriteFork|FSClose|GetEOF|UseResFile|UpdateResFile|FindFolder|FSMakeFSSpec)[[:space:]]*\\(" \
    Sources/UltimaMisc.c

check_absent \
    "UltimaMisc should not depend on the legacy roster resource refnum" \
    "\\bgRosterRefNum\\b" \
    Sources/UltimaMisc.c

check_absent \
    "UltimaGraphics should not access resource storage directly" \
    "\\b(GetResource|LoadResource|ChangedResource|WriteResource|ReleaseResource|DetachResource|AddResource|NewHandleClear|HLock|HUnlock)[[:space:]]*\\(" \
    Sources/UltimaGraphics.c

check_absent \
    "UltimaGraphics should not use Resource Manager memory copy calls" \
    "\\bBlockMoveData[[:space:]]*\\(" \
    Sources/UltimaGraphics.c

check_absent \
    "UltimaGraphics should route timing, random, and low-level sound through abstractions" \
    "\\b(ThreadSleepTicks|TickCount|Random|SndDoImmediate|SndCommand|GetKeyMouse|WaitKeyMouse|RandNum)[[:space:]]*\\(" \
    Sources/UltimaGraphics.c

check_absent \
    "UltimaGraphics should not reach into legacy sound globals directly" \
    "\\b(gSampChan|gToneChan|gCurChan|gMaxChan)\\b" \
    Sources/UltimaGraphics.c

check_ultima_misc_region() {
    description=$1
    start_pattern=$2
    end_pattern=$3

    if awk -v start="$start_pattern" -v end="$end_pattern" '
        $0 ~ start { in_region = 1 }
        $0 ~ end { in_region = 0 }
        in_region && /(^|[^A-Za-z0-9_])(GetResource|LoadResource|ChangedResource|WriteResource|ReleaseResource|DetachResource|AddResource|NewHandleClear)[[:space:]]*\(/ {
            print FILENAME ":" FNR ":" $0
            found = 1
        }
        END { exit found ? 0 : 1 }
    ' Sources/UltimaMisc.c; then
        echo "error: $description"
        status=1
    fi
}

check_ultima_misc_region \
    "MISC table load/save should route through U3IO" \
    "^void GetMiscStuff" \
    "^unsigned char ValidMonsterDir"

check_ultima_misc_region \
    "demo data should route through U3IO" \
    "^void GetDemoRsrc" \
    "^void OpenRstr"

check_ultima_misc_region \
    "roster resource templates should route through U3IO" \
    "^void OpenRstr" \
    "^void GetRoster"

check_ultima_misc_region \
    "roster and party load/save should route through U3IO" \
    "^void GetRoster" \
    "^void ResetSosaria"

check_ultima_misc_region \
    "Sosaria reset should route through U3IO" \
    "^void ResetSosaria" \
    "^void GetSosaria"

check_ultima_misc_region \
    "Sosaria save should route through U3IO" \
    "^void PutSosaria" \
    "^void LoadUltimaMap"

check_ultima_misc_region \
    "map load should route storage resources through U3IO" \
    "^void LoadUltimaMap" \
    "^void PushSosaria"

check_absent \
    "UltimaSpellCombat should not access combat screen resources directly" \
    "\\b(GetResource|LoadResource|ReleaseResource|HLock|HUnlock)[[:space:]]*\\(" \
    Sources/UltimaSpellCombat.c

check_absent \
    "UltimaSpellCombat should route legacy UI, audio, random, and memory helpers through abstractions" \
    "\\b(UPrintMessage|UPrintWin|UPrintNumPad|DrawPrompt|PlaySoundFile|ErrorTone|GetDirection|RandNum|ThreadSleepTicks|GetKeyMouse|FlushEvents|NumToString|BlockMove|ObscureCursor)[[:space:]]*\\(" \
    Sources/UltimaSpellCombat.c

check_absent \
    "UltimaNewMap should route random, timing, and resource release through abstractions" \
    "\\b(Random|TickCount|ThreadSleepTicks|ReleaseResource)[[:space:]]*\\(" \
    Sources/UltimaNewMap.c

check_absent \
    "UltimaDngn should route active timing through U3Platform" \
    "\\b(TickCount|ThreadSleepTicks|GetKeyMouse|WaitKeyMouse|RandNum|Random)[[:space:]]*\\(" \
    Sources/UltimaDngn.c

check_absent \
    "UltimaDngn should not access legacy picture resources directly" \
    "\\b(GetPicture|GetResource|LoadResource|ReleaseResource|HLock|HUnlock)[[:space:]]*\\(" \
    Sources/UltimaDngn.c

check_absent \
    "UltimaMain should route active timing, event flushing, and cursor hiding through U3Platform" \
    "\\b(TickCount|ThreadSleepTicks|FlushEvents|ObscureCursor)[[:space:]]*\\(" \
    Sources/UltimaMain.c

check_absent \
    "UltimaMisc should route active timing and raw random through U3Platform" \
    "\\b(TickCount|ThreadSleepTicks|Random)[[:space:]]*\\(" \
    Sources/UltimaMisc.c

check_absent \
    "UltimaNew should route timing and resource release through abstractions" \
    "\\b(TickCount|ThreadSleepTicks|ReleaseResource)[[:space:]]*\\(" \
    Sources/UltimaNew.c

check_absent \
    "UltimaMacIF should route active timing, event flushing, and resource release through abstractions" \
    "\\b(TickCount|ThreadSleepTicks|FlushEvents|ReleaseResource)[[:space:]]*\\(" \
    Sources/UltimaMacIF.c

check_absent \
    "UltimaText should route input and timing through abstractions" \
    "\\b(CursorKey|GetKeyMouse|WaitKeyMouse|ThreadSleepTicks|TickCount)[[:space:]]*\\(" \
    Sources/UltimaText.c

exit "$status"
