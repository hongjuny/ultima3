"""Run production command/input code with native-style cancellation, without UI or saves."""
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
typedef unsigned char Str255[256];
typedef struct { short top, left, bottom, right; } Rect;
#define TRUE true
#define FALSE false
#define CFSTR(s) s
enum { PASSTIME=360, HPMON=32, TILEON=64, XMON=96, YMON=128, VARMON=160,
       U3PreferenceNoDiagonals, U3SoundEffectError1, U3SoundEffectShoot,
       U3SoundEffectHit, U3SoundEffectOuch, U3SoundEffectAlarm,
       U3SoundEffectCreak, U3SoundEffectLBLevelRise, U3SoundEffectSwish1,
       U3SoundEffectSwish2, U3SoundEffectSwish3, U3SoundEffectSwish4,
       blackColor, whiteColor };
static bool gDone, gResurrect, gAutoCombat, gMouseKey, noDiagonals;
static bool sDungeonEventCheck, sTraversalCheck, sTraversalPassed;
static unsigned int sTraversalStep;
static const unsigned char sTraversalKeys[]=" ";
static unsigned char Party[65], Player[21][65], Monsters[256], Macro[32];
static unsigned char CharX[4], CharY[4], CharShape[4], CharTile[4];
static unsigned char MonsterX[8], MonsterY[8], MonsterTile[8];
static short zp[255], gUpdateWhere, gSongCurrent, gSongNext, gTorch, gExitDungeon;
static short gMouseState, gCurMouseDir, gMonType, gMonVarType, gBallTileBackground;
static short heading, spellnum, cHide, blkSiz=16;
static int xpos, ypos, xs, ys, dx, dy, tx, ty, wx, wy, value;
static char gKeyPress, dungeonLevel;
static int polls, postQuitCalls, turns, ages, heals, draws, mapWrites, attacks, damageCalls;
static int cancelMode, inputIndex, inputCount, inputs[12];
static long ticks;
static void check(bool ok,const char *message) {
    if(!ok) {fprintf(stderr,"FAIL: %s\n",message);exit(1);}
}
static void work(void) { if(gDone) ++postQuitCalls; }
static void U3RenderPrintMessage(int n) {(void)n;work();}
static void U3RenderPrintMessageRewrapped(int n) {(void)n;work();}
static void U3RenderPrintPascalString(const void *s) {(void)s;work();}
static void U3RenderPrintNumberPadded(int n,int w) {(void)n;(void)w;work();}
static void U3RenderDrawPrompt(void) {work();}
static void U3RenderClearTiles(void) {work();}
static void U3AudioPlaySound(int n,bool b) {(void)n;(void)b;work();}
static void U3AudioSpeakMessages(int a,int b,int c) {(void)a;(void)b;(void)c;work();}
static void U3AudioUpdateMusic(void) {work();}
static void DrawMap(int x,int y) {(void)x;(void)y;work();}
static void DrawMapPause(void) {work();}
static void DrawDungeon(void) {work();}
static void DrawTiles(void) {++draws;work();}
static void DrawFrame(int n) {(void)n;work();}
static void DngInfo(void) {work();}
static void ScrollThings(void) {work();}
static void AnimateTiles(void) {work();}
static void CheckAllDead(void) {if(cancelMode==4)gDone=true;}
static void DoAutoHeal(void) {++heals;work();}
static void AgeChars(void) {++ages;work();}
static void ShowChars(bool b) {(void)b;work();}
static void InverseTiles(void) {work();}
static void InverseChnum(int n) {(void)n;work();}
static void UnInverseChnum(int n) {(void)n;work();}
static void GetPascalStringFromArrayByIndex(unsigned char *s,const char *name,int n) {
    (void)name;(void)n;s[0]=1;s[1]='X';work();
}
static void AddString(unsigned char *a,const unsigned char *b) {(void)a;(void)b;work();}
static void SetRect(Rect *r,int l,int t,int rr,int b) {(void)r;(void)l;(void)t;(void)rr;(void)b;work();}
static void ForeColor(int c) {(void)c;work();}
static void PaintRect(Rect *r) {(void)r;work();}
static void NotHere(void) {work();}
static void What2(void) {work();}
static void Incap(void) {work();}
static void Failed(void) {work();}
static void Missed(void) {work();}
static void Speak(int p,int n) {(void)p;(void)n;work();}
static void Shop(int n,int c) {(void)n;(void)c;work();}
static void GetChest(int n,int c) {(void)n;(void)c;work();}
static void PrintTile(int n,bool b) {(void)n;(void)b;work();}
static void PrintMonster(int n,bool b,int v) {(void)n;(void)b;(void)v;work();}
static void ShowHit(int x,int y,int n,int tile) {(void)x;(void)y;(void)n;(void)tile;work();}
static void Flashriek(void) {if(cancelMode==3)gDone=true;else work();}
static bool CheckAlive(int n) {(void)n;return true;}
static bool StealDisarmFail(int n) {(void)n;work();return false;}
static int U3PlatformRandom(int a,int b) {(void)b;work();return a;}
static bool U3PlatformGetBooleanPreference(int n) {(void)n;return noDiagonals;}
static void U3PlatformSetBooleanPreference(int n,bool b) {(void)n;noDiagonals=b;}
static long U3PlatformTickCount(void) {return ++ticks;}
static void U3CocoaQueueDiagnosticKey(int k) {(void)k;}
static bool U3PlatformGetKeyMouse(int mode) {
    (void)mode;++polls;
    if(cancelMode==1)gDone=true;
    gKeyPress=0;
    return false; /* Native input returns false once quit is accepted. */
}
static void AutoCombat(int c) {(void)c;if(cancelMode==2)gDone=true;}
static void U3PlatformGetDirection(int mode) {
    (void)mode;++polls;
    if(cancelMode==1)gDone=true;
    /* Leave the previous direction and coordinates intact, as GetDirection does. */
}
static int input(void) {
    check(inputIndex<inputCount,"unexpected extra input after quit");
    return inputs[inputIndex++];
}
static int GetChar(void) {return input();}
static int GetKey(void) {return input();}
static short UInputNum(int x,int y) {
    (void)x;(void)y;if(cancelMode==2)gDone=true;return input();
}
static long UInputBigNum(int x,int y) {return UInputNum(x,y);}
static void AddMacro(int c) {(void)c;work();}
static int MonsterHere(int x,int y) {(void)x;(void)y;work();return 0;}
static int GetXYVal(int x,int y) {(void)x;(void)y;work();return 184;}
static int GetXYTile(int x,int y) {(void)x;(void)y;work();return 2;}
static int MapConstrain(int n) {return n & 63;}
static void PutXYVal(int n,int x,int y) {(void)n;(void)x;(void)y;++mapWrites;work();}
static void PutXYTile(int n,int x,int y) {(void)n;(void)x;(void)y;work();}
static void AttackCode(int n) {(void)n;++attacks;work();}
static int Shoot(int x,int y) {(void)x;(void)y;work();return 0;}
static int CombatMonsterHere(int x,int y) {(void)x;(void)y;work();return 0;}
static int ExodusCastle(void) {work();return 1;}
static void DamageMonster(int n,int d,int c) {(void)n;(void)d;(void)c;++damageCalls;work();}
/* IMPLEMENTATION */
static void reset(void) {
    gDone=gResurrect=gAutoCombat=gMouseKey=noDiagonals=false;
    sDungeonEventCheck=sTraversalCheck=false;sTraversalPassed=true;sTraversalStep=0;
    polls=postQuitCalls=turns=ages=heals=draws=mapWrites=attacks=damageCalls=0;
    cancelMode=inputIndex=inputCount=0;ticks=0;
    memset(Party,0,sizeof(Party));memset(Player,0,sizeof(Player));
    memset(Monsters,0,sizeof(Monsters));memset(Macro,0,sizeof(Macro));
    memset(zp,0,sizeof(zp));
    Party[2]=2;Party[7]=1;Party[8]=2;
    Player[1][17]=Player[2][17]='G';Player[1][32]=2;Player[2][32]=1;
    Player[1][35]=1;Player[1][36]=44;Player[2][36]=100;
    Player[1][37]=5;Player[2][37]=2;
    gTorch=10;gExitDungeon=0;gUpdateWhere=8;gCurMouseDir=1;
    xpos=ypos=4;xs=5;ys=4;dx=1;dy=0;
}
static void samePlayers(const unsigned char before[21][65]) {
    check(!memcmp(before,Player,sizeof(Player)),"cancelled quantity must not transfer inventory");
    check(!postQuitCalls,"cancelled prompt must return before continuing command work");
}
static void transfer(char item,bool cancel) {
    unsigned char before[21][65];reset();
    inputs[0]=1;inputs[1]=2;inputs[2]=item;
    inputCount=4;inputs[3]=7;
    if(item=='E') {inputs[3]='G';inputs[4]=2;inputCount=5;}
    memcpy(before,Player,sizeof(Player));cancelMode=cancel?2:0;
    HandEquip(0,0,0,0,0);
    if(cancel) {check(gDone,"quantity input delivered quit");samePlayers(before);}
    else if(item=='F') check(Player[1][32]*100+Player[1][33]==193 && Player[2][32]*100+Player[2][33]==107,"normal food transfer");
    else if(item=='G') check(Player[1][35]*256+Player[1][36]==293 && Player[2][35]*256+Player[2][36]==107,"normal gold transfer");
    else check(Player[1][37]==3 && Player[2][37]==4,"normal equipment transfer");
}
int main(int argc,char **argv) {
    check(argc==2,"scenario required");
    if(!strcmp(argv[1],"dungeon")) {
        reset();cancelMode=1;dungeonWait();
        check(gDone && polls==1 && !heals && !postQuitCalls,"dungeon quit must stop after one poll, without waiting six seconds");
        reset();cancelMode=4;dungeonWait();
        check(gDone && !polls && !postQuitCalls,"quit in defeat handling must not resume the dungeon prompt");
        reset();gDone=true;dungeonMechanics();
        check(!ages && !postQuitCalls && gTorch==10,"cancelled command must not age characters or consume torch");
        reset();dungeonWait();
        check(!gDone && polls>0 && gKeyPress==' ',"normal dungeon wait still times out to pass");
    } else if(!strcmp(argv[1],"combat")) {
        reset();cancelMode=1;combatWait();
        check(gDone && polls==1 && !turns && !postQuitCalls && gUpdateWhere==4,"combat quit must stop immediately and restore display mode");
        reset();cancelMode=2;gAutoCombat=true;combatWait();
        check(gDone && !polls && !turns && !postQuitCalls && gUpdateWhere==4,"auto-combat input quit must not finish a player turn");
        reset();combatWait();
        check(!gDone && turns==1 && gKeyPress==' ',"normal combat timeout still completes a pass");
    } else if(!strcmp(argv[1],"quantity")) {
        for(int n=0;n<3;++n) {char item="FGE"[n];transfer(item,true);transfer(item,false);}
    } else if(!strcmp(argv[1],"direction")) {
        void (*commands[])(void)={Attack,Fire,Look,Steal,Transact,Unlock};
        for(unsigned n=0;n<sizeof(commands)/sizeof(commands[0]);++n) {
            unsigned char before[21][65];reset();cancelMode=1;Party[1]=0x16;
            inputCount=1;inputs[0]=1;memcpy(before,Player,sizeof(Player));
            commands[n]();
            check(gDone && polls==1 && !postQuitCalls && !mapWrites && !attacks,"direction cancellation must stop map/combat commands");
            check(!noDiagonals,"Unlock must restore its temporary direction preference");
            samePlayers(before);
        }
        reset();Attack();check(attacks==1,"normal attack still dispatches the selected target");
        reset();Party[3]=0x80;cancelMode=1;Projectile(1,20);
        check(gDone && !damageCalls && !postQuitCalls,"projectile direction cancellation must not damage a monster");
        reset();Party[3]=0x80;cancelMode=1;CombatAttack(0);
        check(gDone && !damageCalls && !postQuitCalls,"combat attack direction cancellation must not attack");
    } else if(!strcmp(argv[1],"terrain")) {
        reset();cancelMode=1;terramorph();
        check(gDone && !mapWrites && !postQuitCalls,"terrain direction cancellation must not ask for a tile or write one");
        reset();cancelMode=2;inputCount=1;inputs[0]=14;terramorph();
        check(gDone && !mapWrites && !postQuitCalls,"terrain numeric cancellation must not create a creature or tile");
        reset();inputCount=1;inputs[0]=4;terramorph();
        check(!gDone && mapWrites==1,"normal terrain spell still changes its tile");
    } else check(false,"unknown scenario");
    puts("Command cancellation: bounded exit and no post-quit mutations");
    return 0;
}
'''


@unittest.skipUnless(shutil.which("clang"), "Command harness requires clang")
class CommandQuitTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        main = (ROOT / "Sources/UltimaMain.c").read_text()
        spell = (ROOT / "Sources/UltimaSpellCombat.c").read_text()
        dungeon = (ROOT / "Sources/UltimaDngn.c").read_text()
        commands = [function_source(main, signature) for signature in (
            "void Attack(void) {", "void Fire(void) {", "void Look(void) {",
            "void Steal(void) {", "void Transact(void) {", "void Unlock(void) {",
            "void HandEquip(short chnum1, short chnum2, short item, char what2, short value) {")]
        commands.extend(function_source(spell, signature) for signature in (
            "void Projectile(short chnum, short damage) {", "void CombatAttack(short chnum) {"))
        # Keep the real polling labels, count-downs, rendering and native input
        # calls. Stop at command dispatch so this harness never needs an engine,
        # window, party save or full dungeon/combat setup.
        wait = dungeon[dungeon.index("dungstart:"):dungeon.index("    if (gKeyPress > 95)", dungeon.index("dungstart:"))]
        commands.append("static void dungeonWait(void) { long nextPassTime;\n" + wait + "\n}")
        mechanics = dungeon[dungeon.index("dungeonmech:"):dungeon.index("    xs = xpos;", dungeon.index("dungeonmech:"))]
        mechanics = mechanics.replace("goto dungstart;", "return;")
        commands.append("static void dungeonMechanics(void) {\n" + mechanics + "\n}")
        combat = spell[spell.index("keyloop:"):spell.index("    while (gKeyPress > 'Z')", spell.index("keyloop:"))]
        commands.append("static void combatWait(void) { short updateStore=4, chnum=0; unsigned char count=0x2F,count2=0xC0;\n" + combat + "\n++turns; }")
        terrain = spell[spell.index("        case 32: /* Terramorph */"):spell.index("        case 33: /* Armageddon */")]
        commands.append("static void terramorph(void) { short value,mon,openSpot; switch(32) {\n" + terrain + "\n} }")
        cls.temporary = tempfile.TemporaryDirectory(prefix="u3-command-quit-")
        directory = Path(cls.temporary.name)
        source, cls.binary = directory / "commands.c", directory / "commands"
        source.write_text(HARNESS.replace("/* IMPLEMENTATION */", "\n\n".join(commands)))
        compiled = subprocess.run(["clang", "-fsanitize=address,undefined", "-g", str(source), "-o", str(cls.binary)],
                                  capture_output=True, text=True, timeout=30)
        if compiled.returncode:
            raise AssertionError(compiled.stderr)

    @classmethod
    def tearDownClass(cls):
        cls.temporary.cleanup()

    def run_scenario(self, name):
        checked = subprocess.run([str(self.binary), name], capture_output=True, text=True, timeout=5)
        self.assertEqual(checked.returncode, 0, checked.stdout + checked.stderr)

    def test_dungeon_wait_and_cancelled_turn(self):
        self.run_scenario("dungeon")

    def test_combat_wait_and_auto_combat(self):
        self.run_scenario("combat")

    def test_quantity_prompt_preserves_inventory(self):
        self.run_scenario("quantity")

    def test_direction_prompts_preserve_map_and_combat_state(self):
        self.run_scenario("direction")

    def test_terrain_prompt_preserves_map(self):
        self.run_scenario("terrain")


if __name__ == "__main__":
    unittest.main()
