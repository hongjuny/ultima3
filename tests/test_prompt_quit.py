"""Exercise actual shop and shrine prompts with silent input and no game storage."""
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef bool Boolean;
typedef unsigned char Str255[256];
typedef struct { short top,left,bottom,right; } Rect;
typedef struct { unsigned short red,green,blue; } RGBColor;
#define TRUE true
#define FALSE false
#define CFSTR(s) s
#define BlockMove(a,b,c) memmove(b,a,c)
enum { U3PreferenceClassicAppearance, U3SoundEffectError1, U3SoundEffectBump,
       U3SoundEffectShrine, kAlertNoteAlert, U3SoundEffectDeathFemale,
       U3SoundEffectDeathMale, U3SoundEffectHit, whiteColor, addOver, srcCopy,
       XMON=32, YMON=64, HPMON=96, TILEON=128 };
static bool gDone;
static unsigned char Player[21][65], Party[65], gString[256];
static unsigned char Monsters[256], LocationX[20], gBallTileBackground;
static char gKeyPress;
static char YellStat;
static short lastCard, gUpdateWhere, gSongCurrent, gSongNext, blkSiz=16;
static int xpos, ypos, tx, ty, wx, wy, xs, ys;
static int polls, cancelAt, quitValue, postQuitOutput, imageCloses;
static int mapWrites, mapWritesAfterQuit, waits, cancelWait;
static const char *typedCommand;
static int inputs[16], inputCount, inputIndex;
static void check(bool ok,const char *message) {
    if(!ok) {fprintf(stderr,"FAIL: %s\n",message);exit(1);}
}
static int readInput(void) {
    if(gDone) return 0;
    ++polls;
    if(cancelAt && polls==cancelAt) {gDone=true;return quitValue;}
    return inputIndex<inputCount ? inputs[inputIndex++] : ' ';
}
static char U3PlatformCursorKey(bool usePen) {(void)usePen;return readInput();}
static short U3PlatformWaitKeyMouse(void) {return readInput();}
static bool U3PlatformGetKeyMouse(int mode) {(void)mode;gKeyPress=readInput();return !gDone;}
static short UInputNum(int x,int y) {(void)x;(void)y;return readInput();}
static long UInputBigNum(int x,int y) {(void)x;(void)y;return readInput();}
static short GetChar(void) {return readInput();}
static short GetKey(void) {return readInput();}
static void UInputText(int x,int y,unsigned char *out,int max,bool num) {
    (void)x;(void)y;(void)max;(void)num;
    out[0]=strlen(typedCommand);memcpy(out+1,typedCommand,out[0]);(void)readInput();
}
static void U3PlatformGetDirection(int mode) {(void)mode;(void)readInput();}
static void U3PlatformWaitTicks(long count) {
    (void)count;if(cancelWait && ++waits==cancelWait)gDone=true;
}
static long U3PlatformTickCount(void) {static long clock;return ++clock;}
static int U3PlatformRandomRaw(void) {return 1;}
static bool U3PlatformGetBooleanPreference(int key) {(void)key;return false;}
static void output(void) {if(gDone)++postQuitOutput;}
static void U3RenderPrintMessage(int n) {(void)n;output();}
static void U3RenderPrintMessageRewrapped(int n) {(void)n;output();}
static void U3RenderPrintPascalString(const unsigned char *s) {(void)s;output();}
static void U3RenderPrintCharAt(int n,int x,int y) {(void)n;(void)x;(void)y;output();}
static void U3RenderPrintNumberAt(int n,int x,int y) {(void)n;(void)x;(void)y;output();}
static void U3AudioSpeakMessages(int a,int b,int c) {(void)a;(void)b;(void)c;output();}
static void U3AudioSpeakPascalString(const unsigned char *s,int voice) {(void)s;(void)voice;output();}
static void U3AudioPlaySound(int n,bool async) {(void)n;(void)async;output();}
static void GetPascalStringFromArrayByIndex(unsigned char *s,const char *kind,int n) {
    s[0]=1;s[1]=!strcmp(kind,"Races")?(n==0?'H':'E'):'1';
}
static void RewrapString(unsigned char *s,bool b) {(void)s;(void)b;}
static void StringToNum(unsigned char *s,long *n) {(void)s;*n=10;}
static void NumToString(long n,unsigned char *s) {(void)n;s[0]=1;s[1]='1';}
static void AddString(unsigned char *a,const unsigned char *b) {
    memcpy(a+a[0]+1,b+1,b[0]);a[0]+=b[0];
}
static void SearchReplace(unsigned char *s,const unsigned char *a,const unsigned char *b) {(void)s;(void)a;(void)b;}
static void ImageDisplay(int n,bool b) {(void)n;(void)b;}
static void ImageGoAway(void) {++imageCloses;}
static void InverseChar(int n) {(void)n;}
static void InverseTiles(void) {}
static void ShowChars(bool b) {(void)b;}
static void DrawMap(int x,int y) {(void)x;(void)y;output();}
static bool AddGold(int ros,int n,bool b) {(void)b;Player[ros][36]+=n;return true;}
static void DoStandardAlert(int a,int b) {(void)a;(void)b;output();}
static void SpellNoize(int n) {(void)n;output();}
static void PrintWeaponList(int n) {(void)n;output();}
static void PrintArmourList(int n) {(void)n;output();}
static bool CheckAlive(int n) {(void)n;return true;}
static bool EqualString(const unsigned char *a,const unsigned char *b,int c,int d) {
    (void)c;(void)d;return a[0]==b[0] && !memcmp(a+1,b+1,a[0]);
}
static int GetXYVal(int x,int y) {(void)x;(void)y;return 0x7c;}
static void PutXYVal(int v,int x,int y) {
    (void)v;(void)x;(void)y;++mapWrites;if(gDone)++mapWritesAfterQuit;
}
static int MonsterHere(int x,int y) {(void)x;(void)y;return 0;}
static void HPSubtract(int ros,int amount) {(void)amount;--Player[ros][27];}
static void NotHere(void) {output();}
static void What2(void) {output();}
static void NoEffect(void) {output();}
static void Yell(int n) {(void)n;output();}
static void SafeExodus(void) {++mapWrites;if(gDone)++mapWritesAfterQuit;}
static void U3AudioUpdateMusic(void) {}
static void U3RenderClearTiles(void) {}
static void SetRect(Rect *r,int l,int t,int rr,int b) {(void)r;(void)l;(void)t;(void)rr;(void)b;}
static int Absolute(int n) {return abs(n);}
static void RGBForeColor(RGBColor *c) {(void)c;}
static void BackColor(int n) {(void)n;}
static void PenMode(int n) {(void)n;}
static void PaintRect(Rect *r) {(void)r;}
static void ForceUpdateMain(void) {}
static void WinText(int n) {(void)n;}
static short ShrineRace(short race);
static short Clerical(short ros,short cost);
static Boolean GuildPay(short ros,short cost);
static void GuildGive(short ros,short item,short qty);
static void WeaponList(short lastitem);
static void ArmourList(short lastitem);
/* IMPLEMENTATION */
static void reset(void) {
    gDone=false;polls=cancelAt=quitValue=postQuitOutput=imageCloses=inputCount=inputIndex=0;
    mapWrites=mapWritesAfterQuit=waits=cancelWait=0;typedCommand="";
    memset(Player,0,sizeof(Player));memset(Party,0,sizeof(Party));memset(inputs,0,sizeof(inputs));
    memset(Monsters,0,sizeof(Monsters));Monsters[0]=0x48;
    Party[2]=1;Party[7]=1;Player[1][17]='P';Player[1][18]=50;Player[1][22]='H';
    Player[1][35]=3;Player[1][36]=232;
    xpos=ypos=tx=ty=wx=wy=0;
    xs=lastCard=0x1e;ys=1;gUpdateWhere=3;
}
static int gold(void) {return Player[1][35]*256+Player[1][36];}
static void unchanged(const unsigned char before[21][65]) {
    check(gDone,"input callback accepted native quit");
    check(!memcmp(before,Player,sizeof(Player)),"quit must not charge gold or alter character data");
    check(!postQuitOutput,"quit must not continue prompting or rendering choices");
    check(!mapWritesAfterQuit,"quit must not continue mutating the map");
}
int main(int argc,char **argv) {
    unsigned char before[21][65];
    if(argc>1 && !strcmp(argv[1],"healer")) {
        reset();cancelAt=1;memcpy(before,Player,sizeof(Player));Shop(2,1);unchanged(before);return 0;
    }
    if(argc>1 && !strcmp(argv[1],"offering")) {
        reset();cancelAt=1;memcpy(before,Player,sizeof(Player));Shop(6,1);unchanged(before);return 0;
    }
    if(argc>1 && !strcmp(argv[1],"shrine")) {
        reset();cancelAt=1;memcpy(before,Player,sizeof(Player));Shrine(1);unchanged(before);
        check(imageCloses==1,"cancelled shrine restores its image state");return 0;
    }
    if(argc>1 && !strcmp(argv[1],"commands")) {
        reset();typedCommand="DIG";xpos=0x21;ypos=3;inputs[0]=1;inputCount=1;cancelAt=2;
        memcpy(before,Player,sizeof(Player));OtherCommand(1);unchanged(before);
        check(!Player[1][63],"partially typed DIG must not execute after quit");
        reset();typedCommand="BRIBE";inputs[0]=1;inputs[1]=0;inputCount=2;cancelAt=3;
        memcpy(before,Player,sizeof(Player));OtherCommand(1);unchanged(before);
        check(Monsters[0]==0x48 && !mapWrites,"cancelled direction must not spend gold or remove guard");
        reset();typedCommand="INSERT";inputs[0]=1;inputs[1]=0;inputCount=2;cancelAt=3;
        memcpy(before,Player,sizeof(Player));OtherCommand(1);unchanged(before);
        check(polls==3,"cancelled INSERT direction must not prompt for a card");
        reset();typedCommand="INSERT";Player[1][14]=15;
        inputs[0]=1;inputs[1]=inputs[2]=0;inputCount=3;cancelAt=4;quitValue='L';
        memcpy(before,Player,sizeof(Player));OtherCommand(1);unchanged(before);
        check(lastCard==0x1e && !mapWrites,"cancelled card selection must not alter Exodus");
        reset();typedCommand="INSERT";Player[1][14]=15;
        inputs[0]=1;inputs[1]=inputs[2]=0;inputs[3]='S';inputCount=4;cancelWait=1;
        memcpy(before,Player,sizeof(Player));OtherCommand(1);unchanged(before);
        reset();typedCommand="INSERT";Player[1][14]=15;xs=lastCard=0x21;
        inputs[0]=1;inputs[1]=inputs[2]=0;inputs[3]='D';inputCount=4;cancelAt=5;
        memcpy(before,Player,sizeof(Player));OtherCommand(1);unchanged(before);
        check(Party[16]==1 && gUpdateWhere==3,"quit at ending input restores UI mode and skips final map cleanup");
        reset();typedCommand="DIG";xpos=0x21;ypos=3;inputs[0]=1;inputCount=1;
        OtherCommand(1);check(!gDone && Player[1][63],"normal typed DIG still executes");
        reset();typedCommand="BRIBE";inputs[0]=1;inputCount=1;
        OtherCommand(1);check(!gDone && gold()==900 && !Monsters[0] && mapWrites==1,
                              "normal bribe still spends gold and removes guard");
        return 0;
    }
    for(int shop=0;shop<8;++shop) {
        reset();cancelAt=1;quitValue=(shop==0 || shop==1)?7:0;
        memcpy(before,Player,sizeof(Player));Shop(shop,1);unchanged(before);
    }
    // Quitting after listing equipment must not start another trade prompt.
    for(int shop=3;shop<=4;++shop) {
        reset();inputs[0]='Y';inputCount=1;cancelAt=2;
        memcpy(before,Player,sizeof(Player));Shop(shop,1);unchanged(before);
    }
    reset();cancelAt=1;memcpy(before,Player,sizeof(Player));
    check(Clerical(1,100)!=0,"clerical confirmation cancels after quit");unchanged(before);
    reset();gDone=true;memcpy(before,Player,sizeof(Player));Shop(2,1);Shrine(1);unchanged(before);
    check(!polls,"already quitting prompts must not wait for another input");
    // Accepted input keeps its original gameplay behavior.
    reset();inputs[0]='0';inputCount=1;Shop(2,1);check(gold()==1000,"normal healer cancellation is free");
    reset();inputs[0]='1';inputs[1]='Y';inputs[2]=1;inputCount=3;Shop(2,1);
    check(gold()==900 && Player[1][17]=='G',"normal healing still charges and cures");
    reset();inputs[0]='2';inputCount=1;Shrine(1);
    check(gold()==800 && Player[1][18]==52,"normal shrine donation still raises its attribute");
    reset();inputs[0]='1';inputs[1]='N';inputCount=2;Shop(6,1);
    check(gold()==900,"normal offering still costs its accepted amount");
    puts("Prompt cancellation returns without loops, transactions or further UI; normal input preserved");
    return 0;
}
'''


@unittest.skipUnless(shutil.which("clang"), "C harness requires clang")
class PromptQuitTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="u3-prompt-quit-")
        base = Path(cls.temporary.name)
        source = (ROOT / "Sources/UltimaMisc.c").read_text()
        functions = ("short ShrineRace(", "void Shrine(", "Boolean GuildPay(",
                     "void GuildGive(", "short Clerical(", "void WeaponList(",
                     "void ArmourList(", "void Shop(", "void OtherCommand(")
        # Select definitions after the local prototype section.
        source = source[source.index("void GetDirection(short mode) {"):]
        harness = base / "harness.c"
        harness.write_text(HARNESS.replace("/* IMPLEMENTATION */", "\n\n".join(
            function_source(source, signature) for signature in functions)))
        cls.executable = base / "prompts"
        result = subprocess.run(["clang", "-fpascal-strings", "-fsanitize=address,undefined", "-g",
                                 str(harness), "-o", str(cls.executable)],
                                capture_output=True, text=True)
        if result.returncode:
            cls.temporary.cleanup()
            raise AssertionError(result.stderr)

    @classmethod
    def tearDownClass(cls):
        cls.temporary.cleanup()

    def run_prompt(self, name):
        result = subprocess.run([str(self.executable), name], capture_output=True,
                                text=True, timeout=2)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_quit_at_healer_prompt_returns(self):
        self.run_prompt("healer")

    def test_quit_at_offering_prompt_keeps_gold(self):
        self.run_prompt("offering")

    def test_quit_at_shrine_prompt_keeps_gold_and_stats(self):
        self.run_prompt("shrine")

    def test_shop_cancellation_boundaries_and_normal_transactions(self):
        self.run_prompt("transactions")

    def test_other_commands_stop_before_interpreting_quit_input(self):
        self.run_prompt("commands")


if __name__ == "__main__":
    unittest.main()
