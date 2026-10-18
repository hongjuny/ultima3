"""Failure injection through the actual save implementation and game save caller.

The Foundation harness uses synthetic resources and a temporary save directory.
It never opens the application, user saves, preferences, or an audio device.
"""
from pathlib import Path
import platform
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


def function_source(source, signature):
    start = source.index(signature)
    body = source.index("{", start)
    depth, end = 1, body + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


HARNESS = r'''
#import <Foundation/Foundation.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "U3IO.h"
#include "U3PartyRecord.h"
typedef uint32_t ResType;
typedef unsigned char Boolean;
typedef unsigned char Str32[33];
typedef char **Handle;
enum { BASERES = 400, U3SoundEffectError1 = 1 };
static NSMutableDictionary *bundled;
static unsigned char Player[21][65], Monsters[256];
unsigned char Party[U3LegacyPartySize];
static unsigned char MoonXTable[8], MoonYTable[8], careerTable[12], wpnUseTable[12], armUseTable[12];
static unsigned char LocationX[20], LocationY[20], Experience[17];
static short gCurMapSize = 64, WhirlX, WhirlY;
static char WhirlDX, WhirlDY;
static int xpos = 42, ypos = 20;
static long lastSaveNumberOfMoves;
static Handle Map;
static size_t mapCapacity;
static int successes, errors, dispersed;
static Boolean gDone;
static int32_t reportedError;
#define ReleaseResource U3TestReleaseResource
#define GetHandleSize U3TestGetHandleSize
static void ReleaseResource(Handle handle) { (void)handle; }
static long GetHandleSize(Handle handle) { return handle == Map ? mapCapacity : 0; }
static void U3RenderPrintMessage(short id) { (void)id; }
static void U3RenderClearBottom(void) {}
static void CenterMessage(short id, short row) { (void)row; if (id == 15) ++dispersed; }
static void ShowClickMessage(void) {}
static short U3PlatformWaitKeyMouse(void) { return 0; }
static void U3AudioPlaySound(short sound, Boolean async) { (void)sound; (void)async; }
static void U3RenderPrintPascalString(const unsigned char *text) { (void)text; ++successes; }
static void GetPascalStringFromArrayByIndex(unsigned char *text, CFStringRef table, int index) {
    (void)table; (void)index; text[0] = 0;
}
static void NumToString(long number, unsigned char *text) { (void)number; text[0] = 0; }
static void AddString(unsigned char *first, const unsigned char *second) { (void)first; (void)second; }
static void U3CocoaReportSaveError(int32_t error) { ++errors; reportedError = error; }
static void GetMiscStuff(short id) { (void)id; }
static void check(BOOL condition, const char *message) {
    if (!condition) { fprintf(stderr, "FAIL: %s\n", message); exit(1); }
}
/* IO_IMPLEMENTATION */
/* GAME_IMPLEMENTATION */
static NSURL *changeDestination(NSURL *url) {
    NSURL *previous = [[sSaveURL retain] autorelease];
    [sSaveURL release]; sSaveURL = [url retain]; return previous;
}
static NSData *disk(void) { return [NSData dataWithContentsOfURL:sSaveURL]; }
int main(int argc, char **argv) {
 @autoreleasepool {
    check(argc == 2, "temporary directory argument");
    setenv("U3_SAVE_DIRECTORY", argv[1], 1);
    bundled = [NSMutableDictionary dictionary];
    for (int i = 0; i < 6; ++i)
        bundled[U3ResourceKey(U3ResourceKindMisc, 400 + i)] = [NSMutableData dataWithLength:64];
    NSMutableData *originalMap = [NSMutableData dataWithLength:4101];
    ((uint8_t *)originalMap.mutableBytes)[0] = 64;
    bundled[U3ResourceKey(U3ResourceKindMap, 420)] = originalMap;
    bundled[U3ResourceKey(U3ResourceKindMonster, 420)] = [NSMutableData dataWithLength:256];
    check(U3IOOpenSaveContainer() == U3SaveContainerOpenResultCreatedEmpty, "new isolated container");
    sResources[U3ResourceKey(U3ResourceKindRoster, 400)] = [NSMutableData dataWithLength:1280];
    sResources[U3ResourceKey(U3ResourceKindParty, 400)] = [NSMutableData dataWithLength:64];
    sResources[U3ResourceKey(U3ResourceKindPreferences, 400)] = [NSMutableData dataWithLength:32];
    sResources[U3ResourceKey(U3ResourceKindMap, 419)] = [NSMutableData dataWithLength:4101];
    sResources[U3ResourceKey(U3ResourceKindMonster, 419)] = [NSMutableData dataWithLength:256];
    for (int i = 0; i < 6; ++i)
        sResources[U3ResourceKey(U3ResourceKindMisc, 500 + i)] = bundled[U3ResourceKey(U3ResourceKindMisc, 400 + i)];
    check(U3IOFlushSaveContainer(), "initial resources written");
    char *mapBytes = calloc(4096, 1); Map = &mapBytes; mapCapacity = 4096;
    Party[11] = 5; Party[4] = 1; Party[5] = 2; Player[1][0] = 'A';
    check(QuitSave(0), "initial user save succeeds");
    check(successes == 1 && errors == 0 && lastSaveNumberOfMoves == 5, "successful save publishes success and move count");
    NSDictionary *baseline = [[sResources copy] autorelease];
    NSURL *realURL = [[sSaveURL retain] autorelease];
    NSData *lastGood = [[disk() retain] autorelease];
    NSURL *badURL = [[realURL URLByDeletingLastPathComponent] URLByAppendingPathComponent:@"missing-parent/save.plist"];
    Player[1][35] = 17; Party[11] = 6; ++xpos; mapBytes[0] = 7; Monsters[0] = 9;
    changeDestination(badURL);
    check(!QuitSave(0), "failed disk write propagates to user save");
    check(errors == 1 && reportedError != 0 && successes == 1, "failure reports an error without a success message");
    check(lastSaveNumberOfMoves == 5 && Party[4] == 42 && Party[5] == 20, "failure preserves dirty move marker and prior party coordinates");
    check([sResources isEqual:baseline] && [[NSData dataWithContentsOfURL:realURL] isEqual:lastGood], "whole multi-resource save rolls back cache and disk");
    check(U3IOHasUnsavedChanges(), "failed transaction remains pending");
    changeDestination(realURL);
    U3DataBuffer read = {0};
    check(U3IOLoadResource(U3ResourceKindRoster, 400, &read), "read following failure");
    U3IOReleaseResource(&read);
    check(U3IOLastError() == 0 && U3IOHasUnsavedChanges(), "successful reads do not hide pending failure");
    U3MutableDataBuffer edit = {0};
    check(U3IOOpenMutableResource(U3ResourceKindPreferences, 400, &edit), "unrelated write opens");
    edit.bytes[0] = 1; check(U3IOCloseMutableResource(&edit, true), "unrelated write succeeds");
    check(U3IOHasUnsavedChanges(), "unrelated write does not clear failed game save");
    check(QuitSave(0), "retry persists all resources");
    check(lastSaveNumberOfMoves == 6 && successes == 2 && !U3IOHasUnsavedChanges(), "successful retry alone clears pending save and updates success marker");
    check(U3IOLoadResource(U3ResourceKindRoster, 400, &read), "read retried roster");
    check(read.bytes[35] == 17, "retry persisted changed roster"); U3IOReleaseResource(&read);
    baseline = [[sResources copy] autorelease]; lastGood = [[disk() retain] autorelease];
    Party[11] = 7; Player[1][35] = 19; mapCapacity = 0;
    check(!QuitSave(1), "preparation failure aborts after earlier resources were staged");
    check([sResources isEqual:baseline] && [disk() isEqual:lastGood] && lastSaveNumberOfMoves == 6, "preparation failure is transactional too");
    check(U3IOHasUnsavedChanges(), "preparation failure remains pending");
    mapCapacity = 4096;
    check(QuitSave(1) && successes == 2 && !U3IOHasUnsavedChanges(), "silent retry clears failure without verbose success text");
    changeDestination(badURL); Player[1][35] = 20;
    check(!PutRoster() && U3IOHasUnsavedChanges(), "direct roster failure propagates and remains pending outside world");
    changeDestination(realURL);
    check(U3IOOpenMutableResource(U3ResourceKindPreferences, 400, &edit), "open unrelated resource again");
    check(U3IOCloseMutableResource(&edit, true) && U3IOHasUnsavedChanges(), "unrelated write cannot clear failed roster");
    check(PutRoster() && !U3IOHasUnsavedChanges(), "retry of failed resource clears its pending flag");
    U3GameState expected = {0}, restored = {0}; expected.x = 123;
    check(U3IOSaveGame(&expected), "portable snapshot saved");
    baseline = [[sResources copy] autorelease]; lastGood = [[disk() retain] autorelease];
    expected.x = 456; changeDestination(badURL);
    check(!U3IOSaveGame(&expected), "portable snapshot failure propagates");
    check([sResources isEqual:baseline] && [[NSData dataWithContentsOfURL:realURL] isEqual:lastGood], "portable snapshot failure preserves previous state");
    check(U3IOLoadGame(&restored) && restored.x == 123 && U3IOHasUnsavedChanges(), "portable load still sees last successful snapshot");
    changeDestination(realURL);
    check(U3IOSaveGame(&expected) && !U3IOHasUnsavedChanges(), "portable snapshot retry clears its failure");
    check(U3IOOpenSaveContainer() == U3SaveContainerOpenResultOpened && U3IOLoadGame(&restored) && restored.x == 456, "successful retry survives container reopen");
    baseline = [[sResources copy] autorelease]; lastGood = [[disk() retain] autorelease];
    unsigned char previousPlayers[sizeof(Player)], previousParty[sizeof(Party)];
    memcpy(previousPlayers, Player, sizeof(Player)); memcpy(previousParty, Party, sizeof(Party));
    int previousX = xpos, previousY = ypos;
    changeDestination(badURL);
    check(!U3StorePartySelection((short[4]){1, 0, 0, 0}), "new party reports failed transaction");
    check(!memcmp(previousPlayers, Player, sizeof(Player)) && !memcmp(previousParty, Party, sizeof(Party)) && xpos == previousX && ypos == previousY,
          "failed party creation restores roster selection and live party state");
    check([sResources isEqual:baseline] && [[NSData dataWithContentsOfURL:realURL] isEqual:lastGood], "failed party creation rolls back all resource updates");
    changeDestination(realURL);
    check(U3StorePartySelection((short[4]){1, 0, 0, 0}) && Party[7] == 1 && Player[1][16] == 255 && !U3IOHasUnsavedChanges(),
          "new party can be retried and succeeds only after persistence");
    baseline = [[sResources copy] autorelease]; lastGood = [[disk() retain] autorelease];
    memcpy(previousPlayers, Player, sizeof(Player)); memcpy(previousParty, Party, sizeof(Party));
    changeDestination(badURL); DisperseParty();
    check(!dispersed && !memcmp(previousPlayers, Player, sizeof(Player)) && !memcmp(previousParty, Party, sizeof(Party)),
          "failed party dispersal preserves party and does not claim success");
    check([sResources isEqual:baseline] && [[NSData dataWithContentsOfURL:realURL] isEqual:lastGood], "failed dispersal rolls back both party and roster writes");
    changeDestination(realURL); DisperseParty();
    check(dispersed == 1 && !Party[7] && !Player[1][16] && !U3IOHasUnsavedChanges(), "dispersal retry commits party and roster together before success");
    check(U3IOLoadResource(U3ResourceKindRoster, 400, &read) && read.bytes[16] == 0, "dispersal persists cleared roster party flags");
    U3IOReleaseResource(&read);
    baseline = [[sResources copy] autorelease]; lastGood = [[disk() retain] autorelease];
    gDone = TRUE;
    check(!PutRoster() && !PutParty() && !PutSosaria() && !PutMiscStuff() && !ResetSosaria() && !QuitSave(0),
          "quitting game refuses every gameplay save entry point");
    check([sResources isEqual:baseline] && [disk() isEqual:lastGood], "late post-quit callers cannot change cache or save file");
    free(mapBytes);
    puts("Save failure propagation, atomic rollback, dirty tracking, UI success and retry tests passed");
 }
 return 0;
}
'''


@unittest.skipUnless(platform.system() == "Darwin" and shutil.which("clang"),
                     "Foundation save harness requires macOS developer tools")
class SaveFailureTests(unittest.TestCase):
    def test_failed_writes_never_publish_success_or_discard_pending_state(self):
        io = (ROOT / "Sources/U3IOLegacy.m").read_text()
        io = io[:io.index("bool U3IOSelfTest(void)")]
        io = io.replace('#import "UltimaIncludes.h"', "")
        bundled = function_source(io, "static NSData *U3BundledResource(")
        io = io.replace(bundled, r'''static NSData *U3BundledResource(U3ResourceKind kind, int16_t resourceID) {
    NSData *data = bundled[U3ResourceKey(kind, resourceID)];
    if (!data) sLastError = resNotFound;
    return data;
}''')
        misc = (ROOT / "Sources/UltimaMisc.c").read_text()
        functions = [function_source(misc, signature) for signature in
                     ("static Boolean WriteByteTable(", "Boolean PutMiscStuff(", "Boolean PutRoster(",
                      "Boolean PutParty(", "Boolean PutSosaria(", "static Boolean ResetResourcePrefix(",
                      "Boolean ResetSosaria(")]
        main = (ROOT / "Sources/UltimaMain.c").read_text()
        functions.extend(function_source(main, signature) for signature in
                         ("Boolean QuitSave(", "void DisperseParty(void) {"))
        party = (ROOT / "Sources/UltimaNew.c").read_text()
        functions.extend(function_source(party, signature) for signature in
                         ("static Boolean PartySelectionIsValid(", "Boolean U3ApplyPartySelection(", "Boolean U3StorePartySelection("))
        with tempfile.TemporaryDirectory(prefix="u3-save-failures-") as temporary:
            directory = Path(temporary)
            source, executable = directory / "save-harness.m", directory / "save-harness"
            source.write_text(HARNESS.replace("/* IO_IMPLEMENTATION */", io)
                              .replace("/* GAME_IMPLEMENTATION */", "\n".join(functions)))
            compiled = subprocess.run(["clang", "-fsanitize=address,undefined", "-g", "-Wno-deprecated-declarations",
                                       "-Wno-multichar", "-I", str(ROOT / "Sources"), str(source),
                                       "-framework", "Foundation", "-o", str(executable)],
                                      capture_output=True, text=True, timeout=45)
            self.assertEqual(compiled.returncode, 0, compiled.stderr)
            checked = subprocess.run([str(executable), str(directory / "saves")],
                                     capture_output=True, text=True, timeout=15)
            self.assertEqual(checked.returncode, 0, checked.stdout + checked.stderr)
            self.assertIn("tests passed", checked.stdout)


if __name__ == "__main__":
    unittest.main()
