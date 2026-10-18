"""Quit nested animations through actual game functions, without UI or storage."""
from pathlib import Path
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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef bool Boolean;
typedef struct { unsigned short red, green, blue; } RGBColor;
typedef struct { short top, left, bottom, right; } Rect;
#define TRUE true
#define FALSE false
#define nil 0
enum { BASERES=400, U3PreferenceClassicAppearance, U3PreferenceAutoSave,
       U3SoundEffectSink, U3SoundEffectBigDeath, redColor, blackColor, blend, srcOr };
static Boolean gDone, gResurrect, sDefeatScenarioCheck;
static unsigned char Party[65], Player[21][65];
static short zp[255], gSongCurrent, gSongNext, gUpdateWhere=3, gMouseState;
static short WhirlX, WhirlY, gCurMapSize=64, gTimeNegate, blkSiz=16;
static int xpos=42, ypos=20, xs, ys, ox, oy, wx, wy, gamePort, mainPort;
static long ticks;
static int cancelMode, polls, writes, writesAfterQuit, pushes, pulls, loads, mapEdits, resets, alerts, flashes, postQuitPolls;
static void check(bool ok, const char *message) { if(!ok) { fprintf(stderr,"FAIL: %s\n",message);exit(1); } }
static bool U3PlatformGetBooleanPreference(int key) { (void)key; return true; }
static long U3PlatformTickCount(void) { return ++ticks; }
static int U3PlatformGetKeyMouse(int mode) {
    if (gDone) {
        check(++postQuitPolls < 4, "map wait must not poll forever after native quit returns false");
        return 0;
    }
    (void)mode; ++polls;
    if ((cancelMode==1 && gUpdateWhere==8) || (cancelMode==2 && gUpdateWhere==0) ||
        (cancelMode==3 && gUpdateWhere==3) || cancelMode==5) gDone=true;
    return 0;
}
static int U3PlatformWaitKeyMouse(void) { if(cancelMode==4)gDone=true; return 0; }
static void U3PlatformWaitTicks(long count) { ticks+=count; }
static void IdleUntil(long end) { ticks=end; }
static int U3PlatformRandom(int min,int max) { (void)max; return min; }
static void DrawMap(int x,int y) { (void)x;(void)y; }
static void U3RenderPrintMessage(int message) { (void)message; }
static void U3RenderPrintMessageRewrapped(int message) { (void)message; }
static void U3RenderPrintPascalString(const void *text) { (void)text; }
static void U3RenderDrawPrompt(void) {}
static void U3RenderClearTiles(void) {}
static void U3AudioUpdateMusic(void) { ++ticks; }
static void U3AudioPlaySound(int sound,bool async) { (void)sound;(void)async; }
static void CursorUpdate(void) {}
static void AnimateTiles(void) {}
static void PutXYVal(int value,int x,int y) { (void)value;(void)x;(void)y;++mapEdits; }
static void PushSosaria(void) { ++pushes; }
static void PullSosaria(void) { ++pulls; }
static void LoadUltimaMap(int id) { (void)id;++loads; }
static int GetXYVal(int x,int y) { (void)x;(void)y;return 0; }
static Boolean WriteResource(void) { ++writes; if(gDone)++writesAfterQuit;return true; }
static Boolean PutRoster(void) { return WriteResource(); }
static Boolean PutParty(void) { return WriteResource(); }
static Boolean PutSosaria(void) { return WriteResource(); }
static Boolean PutMiscStuff(void) { return WriteResource(); }
static Boolean ResetSosaria(void) { ++resets;return WriteResource(); }
static void GetMiscStuff(int id) { (void)id; }
static void GetSosaria(void) { ++loads; }
static Boolean CheckAlive(short member) { (void)member;return false; }
static void U3PlatformFlushAllEvents(void) {}
static int Alert(int id,void *unused) { (void)id;(void)unused;++alerts;return 1; }
static void ForceUpdateMain(void) {}
static void DrawGamePortToMain(int transition) { (void)transition; }
static void SetGWorld(int port,int device) { (void)port;(void)device; }
static void OpColor(RGBColor *color) { (void)color; }
static void ForeColor(int color) { (void)color; }
static void SetRect(Rect *rect,int l,int t,int r,int b) { (void)rect;(void)l;(void)t;(void)r;(void)b; }
static void PenMode(int mode) { (void)mode; }
static void PaintRect(Rect *rect) { (void)rect; }
static void ShowChars(Boolean force) { (void)force; }
static void RGBForeColor(RGBColor *color) { (void)color;++flashes; }
static void DngPiece(int shape) { (void)shape; }
static Boolean Button(void) { return false; }
/* IMPLEMENTATION */
static void mapWait(void) {
    const unsigned short fadeJump=4096, minBrightness=8192, maxBrightness=61439;
    Boolean fadeToWhite=true;
    RGBColor color={61439,61439,61439}; Rect myRect={0};
    /* MAP_WAIT */
}
static void dungeonMapWait(void) {
    unsigned char done=0, under=0;
    /* DUNGEON_WAIT */
}
static void reset(int mode) {
    gDone=gResurrect=false; ticks=polls=writes=writesAfterQuit=pushes=pulls=loads=mapEdits=resets=alerts=flashes=postQuitPolls=0;
    cancelMode=mode; gUpdateWhere=3; xpos=42;ypos=20;gSongCurrent=gSongNext=1;
    memset(Party,0,sizeof(Party));memset(Player,0,sizeof(Player));
    Party[1]=0x16;Party[2]=1;Party[7]=1;Player[1][17]='D';Player[1][36]=37;
}
int main(void) {
    reset(1);GoWhirlPool();
    check(gDone && polls==1 && writes==0 && pushes==0 && loads==0 && mapEdits==0,
          "quit in first whirlpool animation must return before mutations and autosave");
    check(gUpdateWhere==3 && xpos==42 && ypos==20 && Party[3]==0,"first whirlpool cancellation restores UI mode and skips transition");
    reset(2);GoWhirlPool();
    check(gDone && writes==3 && writesAfterQuit==0 && loads==1,"second animation keeps only saves made before quit");
    check(gUpdateWhere==3 && xpos==42 && ypos==20 && Party[3]==0,"second whirlpool cancellation skips final teleport");
    reset(3);CheckAllDead();
    check(gDone && writes==3 && writesAfterQuit==0 && resets==0 && loads==0,"quit in defeat delay cannot perform resurrection saves");
    check(!gResurrect && Player[1][17]=='D' && Player[1][36]==37,"quit in defeat delay leaves characters unmodified");
    reset(4);CheckAllDead();
    check(gDone && alerts==0 && writes==3 && resets==0 && !gResurrect,"quit during defeat input cannot open resurrection dialog or save");
    reset(0);gDone=true;GoWhirlPool();CheckAllDead();
    check(!polls && !writes && !pushes && !loads && !mapEdits,"already quitting game performs no animation or save work");
    reset(5);mapWait();
    check(gDone && polls==1 && !flashes && !postQuitPolls,"surface map quits when native poll sets done and returns false");
    reset(5);dungeonMapWait();
    check(gDone && polls==1 && !postQuitPolls,"dungeon map quits when native poll sets done and returns false");
    puts("Whirlpool and defeat cancellation: no mutations or autosaves after quit");
    return 0;
}
'''


DEMO_HARNESS = r'''
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
typedef bool Boolean;
#define TRUE true
#define FALSE false
typedef struct { const uint8_t *bytes; size_t size; void *owner; } U3DataBuffer;
static Boolean gDone, pending;
static short gSongNext, gSongCurrent, gDemoSong;
static unsigned char TileArray[128];
static U3DataBuffer gDemoData;
static int draws, peeks, inject;
static void check(bool ok, const char *message) {
    if (!ok) { fprintf(stderr,"FAIL: %s\n",message);exit(1); }
}
static void DrawDemo(void) {
    ++draws;
    if (draws == 1 && inject == 1) pending=true;
    if (draws == 1 && inject == 2) gDone=true;
}
static Boolean U3CocoaHasPendingGameInput(void) { ++peeks;return pending; }
/* DEMO_IMPLEMENTATION */
int main(void) {
    /* The actual DEMO400 opening instruction is repeat4: four repeats of
       four animation frames. A frame's native wait may queue a click. */
    uint8_t demo[1280]={0};demo[0]=255;demo[512]=4;
    gDemoData=(U3DataBuffer){demo,sizeof(demo),NULL};
    DemoUpdate(0);
    check(draws==16 && !pending && !gDone,"demo retains all16 frames when no input arrives");
    draws=peeks=0;inject=1;
    DemoUpdate(0);
    check(draws==1 && peeks==1,"queued click interrupts repeat4 after its first frame");
    check(pending && !gDone,"demo leaves the click queued for its outer input owner");
    draws=peeks=0;pending=false;inject=2;
    DemoUpdate(0);
    check(draws==1 && gDone && !peeks,"native quit stops demo immediately after the current frame");
    draws=0;DemoUpdate(0);
    check(!draws,"already quitting game cannot begin another demo frame");
    puts("Demo repeat4 responsiveness: one frame, queued click preserved");
    return 0;
}
'''


@unittest.skipUnless(shutil.which("clang"), "Cancellation harness requires clang")
class QuitCancellationTests(unittest.TestCase):
    def test_demo_yields_after_one_frame_without_consuming_queued_input(self):
        graphics = (ROOT / "Sources/UltimaGraphics.c").read_text()
        demo = function_source(graphics, "void DemoUpdate(short ptr) {")
        with tempfile.TemporaryDirectory(prefix="u3-demo-responsiveness-") as temporary:
            directory = Path(temporary)
            harness, binary = directory / "demo.c", directory / "demo"
            harness.write_text(DEMO_HARNESS.replace("/* DEMO_IMPLEMENTATION */", demo))
            compiled = subprocess.run(["clang", "-fsanitize=address,undefined", "-g", str(harness), "-o", str(binary)],
                                      capture_output=True, text=True, timeout=30)
            self.assertEqual(compiled.returncode, 0, compiled.stderr)
            checked = subprocess.run([str(binary)], capture_output=True, text=True, timeout=5)
            self.assertEqual(checked.returncode, 0, checked.stdout + checked.stderr)
            self.assertIn("one frame, queued click preserved", checked.stdout)

    def test_nested_animation_quit_cannot_write_or_resurrect(self):
        source = (ROOT / "Sources/UltimaMain.c").read_text()
        implementation = "\n".join(function_source(source, signature) for signature in
                                    ("void GoWhirlPool(void) {", "void CheckAllDead(void) {"))
        graphics = (ROOT / "Sources/UltimaGraphics.c").read_text()
        surface_map = function_source(graphics, "void DrawMiniMap(void) {")
        dungeon_map = function_source(graphics, "void DrawMiniDng(unsigned char mode) {")
        map_wait = function_source(surface_map, "while (")
        dungeon_wait = function_source(dungeon_map, "while (")
        with tempfile.TemporaryDirectory(prefix="u3-quit-cancellation-") as temporary:
            directory = Path(temporary)
            harness, binary = directory / "harness.c", directory / "harness"
            harness.write_text(HARNESS.replace("/* IMPLEMENTATION */", implementation)
                               .replace("/* MAP_WAIT */", map_wait)
                               .replace("/* DUNGEON_WAIT */", dungeon_wait))
            compiled = subprocess.run(["clang", "-fsanitize=address,undefined", "-g", "-Wno-unknown-escape-sequence",
                                       str(harness), "-o", str(binary)], capture_output=True, text=True, timeout=30)
            self.assertEqual(compiled.returncode, 0, compiled.stderr)
            checked = subprocess.run([str(binary)], capture_output=True, text=True, timeout=5)
            self.assertEqual(checked.returncode, 0, checked.stdout + checked.stderr)
            self.assertIn("no mutations or autosaves after quit", checked.stdout)


if __name__ == "__main__":
    unittest.main()
