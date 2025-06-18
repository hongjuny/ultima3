// Spell & Combat routines

#import "CarbonShunts.h"
#import "UltimaSpellCombat.h"

#import "UltimaIncludes.h"
#import "CarbonShunts.h"
#import "CocoaBridge.h"
#import "UltimaAutocombat.h"
#import "UltimaDngn.h"
#import "UltimaGraphics.h"
#import "UltimaMain.h"
#import "UltimaMisc.h"
#import "UltimaSound.h"
#import "UltimaText.h"

char                    g835D, g835E, g835F;
short                   spellnum;

extern char             g5521, g56E7;
extern Boolean          gDone, gResurrect, gAutoCombat;
extern char             gKeyPress, dungeonLevel;
extern int              xpos, ypos, xs, ys, wx, wy, dx, dy;
extern unsigned char    Monsters[256], TileArray[128], Player[21][65], Macro[32];
extern char             MonsterVariants[];
extern unsigned char    Party[64], Dungeon[2048], m5BDC, cHide;
extern unsigned char    CharX[4], CharY[4], CharTile[4], CharShape[4];
extern unsigned char    MonsterX[8], MonsterY[8], MonsterTile[8], MonsterHP[8];
extern short            gTorch, gTimeNegate, zp[255], gExitDungeon, gMonType;
extern short            gUpdateWhere, gChnum, gMouseState;
extern unsigned char    Experience[17], LocationX[20], careerTable[12], gBallTileBackground;
extern short            gCurMapSize, gSongCurrent, gSongNext, gSongPlaying;
extern char             gMonVarType;

// ----------------------------------------------------------------------
// Local prototypes

void Flashriek(void);
void Pilfer(short chnum);
void RelocateDungeon(void);

// ----------------------------------------------------------------------

Boolean Cast(short mode, short chnum) { /* $535C. if mode<>0, chnum is set (1-4) */
    short rosNum;
    char classType;
    if (mode == 0) {
        zp[0x1F] = 0x78;
        zp[0xD6] = gChnum;
        UPrintMessage(119);
        chnum = GetChar();
        if (chnum < 1 || chnum > 4)
            return FALSE;
        if (CheckAlive(chnum - 1) == FALSE) {
            Incap();
            return FALSE;
        }
        zp[0xD6] = gChnum;
    }
    rosNum = Party[6 + chnum];
    classType = Player[rosNum][23];
    spellnum = -1;
    if (classType == careerTable[1])
        spellnum = ClericChoose();
    if (classType == careerTable[4])
        spellnum = ClericChoose();
    if (classType == careerTable[7])
        spellnum = ClericChoose();
    if (classType == careerTable[2])
        spellnum = WizardChoose();
    if (classType == careerTable[6])
        spellnum = WizardChoose();
    if (classType == careerTable[9])
        spellnum = WizardChoose();
    if (classType == careerTable[8])
        spellnum = EitherChoose();
    if (classType == careerTable[10])
        spellnum = EitherChoose();
    if (spellnum == -2) {
        UPrintMessage(120);
        return FALSE;
    }
    if (spellnum == -1) {
        UPrintMessage(121);
        return FALSE;
    }
    ProcessMagic(chnum);
    return TRUE;
}

void ProcessMagic(short chnum) {
    short rosNum, magicreq;
    rosNum = Party[6 + chnum];
    magicreq = (spellnum & 0x0F) * 5;
    if (spellnum == 32)
        magicreq = 10;
    if (spellnum == 33)
        magicreq = 85;
    if (spellnum == 34)
        magicreq = 90;
    if (magicreq > Player[rosNum][25]) {
        UPrintMessage(122);
        return;
    }
    Player[rosNum][25] -= magicreq;
    ShowChars(false);
    UPrintWin("\p\n");
    if (spellnum < 32) {
        PrintSpell(spellnum);
    } else {
        if (spellnum == 32)
            UPrintWin("\pTERRAFORM");
        if (spellnum == 33)
            UPrintWin("\pARMAGEDDON");
        if (spellnum == 34)
            UPrintWin("\pFLOTELLUM");
    }
    UPrintWin("\p\n\n");
    Spell(chnum);
    return;
}

short EitherChoose(void) {
    short value, result;
    result = -1;
either0:
    UPrintMessage(123);
    value = GetKey();
    if (value == 27)
        return -2;
    if (gDone)
        return -2;
    if (value == 'W')
        result = WizardChoose();
    if (value == 'C')
        result = ClericChoose();
    if (result == -1)
        goto either0;
    return result;
}

short ClericChoose(void) {
    short value;
cleric0:
    UPrintMessage(124);
    value = GetKey();
    if (value == 27)
        return -2;
    if (gDone)
        return -2;
    if (value < 'A' || value > 'P')
        goto cleric0;
    return value - ('A' - 16);
}

short WizardChoose(void) {
    short value;
wizard0:
    UPrintMessage(125);
    value = GetKey();
    if (value == 27)
        return -2;
    if (gDone)
        return -2;
    if (value < 'A' || value > 'S')
        goto wizard0;
    if (value > 'P' && Party[16] == 0)
        goto wizard0;
    if (value == 'Q')
        return 32;
    if (value == 'R')
        return 33;
    if (value == 'S')
        return 34;
    return value - 'A';
}

void Incap(void) {
    UPrintMessage(126);
    ErrorTone();
}

/* ---------------------------------------------------------------- */

void Spell(short chnum) {
    short rosNum, value, mon, openSpot;

    if (gDone)
        return;
    rosNum = Party[6 + chnum];
    switch (spellnum) {
        case 0: /* Repond */
            if ((Party[3] != 0x80) || (gMonType != 0x30) || (g5521 != 0)) {
                Failed();
                return;
            }
            g5521 = 0xFF;
            if (RandNum(0, 255) < 128) {
                Failed();
                return;
            }
            BigDeath(255, chnum);
            break;
        case 1: /* Mittar */ Projectile(chnum, RandNum(0, 40) | 0x10); break;
        case 2: /* Lorum */
            gTorch = 10;
            Flashriek();
            break;
        case 3: /* Dor Acron */ DownLevel(); break;
        case 4: /* Sur Acron */ UpLevel(); break;
        case 5: /* Fulgar */ Projectile(chnum, 75); break;
        case 6: /* Dag Acron */
            Flashriek();
            if ((Party[3] != 0) || (Party[1] == 0x16 && Party[16] == 0)) {
                Failed();
                return;
            }
            int matchValue = (Party[1] == 0x16) ? 0 : 4;
            value = -1;
            while (value != matchValue) {
                xs = RandNum(0, gCurMapSize - 1);
                ys = RandNum(0, gCurMapSize - 1);
                value = GetXYVal(xs, ys);
            }
            xpos = xs;
            ypos = ys;
            break;
        case 7: /* Mentar */ Projectile(chnum, Player[rosNum][20]); break;
        case 8: /* Dag Lorum */
            gTorch = 250;
            Flashriek();
            break;
        case 9: /* Fal Divi */
            Flashriek();
            spellnum = ClericChoose();
            ProcessMagic(chnum);
            break;
        case 10: /* Noxum */ BigDeath(75, chnum); break;
        case 11: /* Decorp */ Projectile(chnum, 255); break;
        case 12: /* Altair */ gTimeNegate = 20; break;
        case 13: /* Dag Mentar */ BigDeath(Player[rosNum][20] * 2, chnum); break;
        case 14: /* Necorp */
            if (Party[3] != 0x80) {
                Failed();
                return;
            }
            Necorp();
            break;
        case 15: /* nameless */ BigDeath(255, chnum); break;
        case 16: /* Pontori */
            if (Party[3] != 0x80) {
                Failed();
                return;
            }
            if (gMonType != 0x32) {
                Failed();
                return;
            }
            if (g56E7 != 0) {
                Failed();
                return;
            }
            g56E7 = 0xFF;
            if (RandNum(0, 255) < 128) {
                Failed();
                return;
            }
            BigDeath(255, chnum);
            break;
        case 17: /* Appar Unem */
            Flashriek();
            if ((RandNum(0, 255) & 0x03) == 0) {
                Failed();
                return;
            }
            m5BDC = 0;
            GetChest(1, chnum);
            break;
        case 18: /* Sanctu */ Heal(RandNum(0, 20) + 10); break;
        case 19: /* Luminae */
            gTorch = 10;
            Flashriek();
            break;
        case 20: /* Rec Su */ UpLevel(); break;
        case 21: /* Rec Du */ DownLevel(); break;
        case 22: /* Lib Rec */
            Flashriek();
            if (Party[3] != 1) {
                Failed();
                return;
            }
            RelocateDungeon();
            break;
        case 23: /* Alcort */
            UPrintMessage(127);
            value = GetChar();
            if (value < 1 || value > 4) {
                Failed();
                return;
            }
            InverseChar(value - 1);
            Flashriek();
            InverseChar(value - 1);
            if (Player[Party[6 + value]][17] != 'P') {
                Failed();
                return;
            }
            Player[Party[6 + value]][17] = 'G';
            break;
        case 24: /* Sequitu */
            if (Party[3] != 1) {
                Failed();
                return;
            }
            Flashriek();
            dungeonLevel = 0;
            gExitDungeon = 1;
            break;
        case 25: /* Sominae */
            gTorch = 250;
            Flashriek();
            break;
        case 26: /* Sanctu Mani */ Heal(RandNum(0, 80) + 20); break;
        case 27: /* Vieda */
            Flashriek();
            if (Party[3] == 1)
                DrawMiniDng(0);
            else
                DrawMiniMap();
            break;
        case 28: /* Excuun */ Projectile(chnum, 255); break;
        case 29: /* Surmandum */
            if (Party[3] == 0x80) {
                Failed();
                return;
            }
            UPrintMessage(128);
            value = GetChar();
            if (value < 1 || value > 4) {
                Failed();
                return;
            }
            InverseChar(value - 1);
            Flashriek();
            InverseChar(value - 1);
            if (Player[Party[6 + value]][17] != 'D') {
                Failed();
                return;
            }
            if ((RandNum(0, 255) & 0x03) == 0) {
                Player[Party[6 + value]][17] = 'A';
                Failed();
                return;
            }
            Player[Party[6 + value]][17] = 'G';
            break;
        case 30: /* ZXKUQYB */ BigDeath(255, chnum); break;
        case 31: /* Anju Sermani */
            if (Party[3] == 0x80) {
                Failed();
                return;
            }
            UPrintMessage(129);
            value = GetChar();
            if (value < 1 || value > 4) {
                Failed();
                return;
            }
            InverseChar(value - 1);
            Flashriek();
            InverseChar(value - 1);
            if (Player[Party[6 + value]][17] != 'A') {
                Failed();
                return;
            }
            Player[Party[6 + value]][17] = 'G';
            Player[Party[6 + chnum]][21] -= 5; /* Caster's wisdom-5 */
            break;
        case 32: /* Terramorph */
            if (Party[3] == 0x80) {
                Failed();
                return;
            }
            UPrintMessage(85);
            GetDirection(0);
            UPrintWin("\pTypeNum-");
            value = UInputNum(wx, wy);
            Flashriek();
            if (value > 12 && value < 31) {   // a creature
                openSpot = -1;
                for (mon = 0; mon < 32; mon++) {
                    if (Monsters[mon] == 0)
                        openSpot = mon;
                }
                if (openSpot == -1) {
                    PutXYVal(value * 4, xpos + dx, ypos + dy);
                } else {
                    Monsters[openSpot] = value * 4;
                    Monsters[openSpot + TILEON] = GetXYVal(xpos + dx, ypos + dy);
                    Monsters[openSpot + XMON] = xpos + dx;
                    Monsters[openSpot + YMON] = ypos + dy;
                    Monsters[openSpot + HPMON] = 0x40;    // wander
                    PutXYVal(value * 4, xpos + dx, ypos + dy);
                }
            } else {
                PutXYVal(value * 4, xpos + dx, ypos + dy);
            }
            UPrintWin("\p\n");
            break;
        case 33: /* Armageddon */
            if (Party[3] == 0x80) {
                Failed();
                return;
            }
            Flashriek();
            for (mon = 0; mon < 32; mon++) {
                if (Monsters[mon] != 0) {
                    // put a chest where the monster was
                    short tileon = Monsters[mon + TILEON];
                    if (tileon != 0)
                        tileon = ((tileon / 4) & 0x03) + 0x24;
                    PutXYVal(tileon, Monsters[mon + XMON], Monsters[mon + YMON]);
                    Monsters[mon] = 0;
                }
            }
            break;
        case 34:    // Flotellum
            if (Party[3] == 0x80) {
                Failed();
                return;
            }
            int closestDist = 99;
            int destX = -1, destY = -1;
            int testY;
            for (testY = -5; testY <= 5; testY++) {
                int testX;
                for (testX = -5; testX <= 5; testX++) {
                    if (!(testX == 0 && testY == 0) && GetXYVal(xpos + testX, ypos + testY) == 0) {
                        int thisDist = Absolute(testX) + Absolute(testY);
                        if (thisDist < closestDist) {
                            closestDist = thisDist;
                            destX = xpos + testX;
                            destY = ypos + testY;
                        }
                    }
                }
            }
            if (destX < 0) {
                Failed();
                return;
            }
            Flashriek();
            PutXYVal(11 * 4, destX, destY);
            break;
    }
}

void Projectile(short chnum, short damage) { /* $552B */
    short hit;
    if (Party[3] != 0x80) {
        Failed();
        return;
    }
    UPrintMessage(85);
    GetDirection(0);
    Flashriek();
    hit = Shoot(CharX[chnum - 1], CharY[chnum - 1]);
    if (hit > 127) {
        Failed();
        return;
    }
    ShowHit(MonsterX[hit], MonsterY[hit], 0x78, MonsterTile[hit]);
    PlaySoundFile(CFSTR("Hit"), FALSE);    // was 0xF7
    DamageMonster(hit, damage, chnum);
}

void BigDeath(short damage, short chnum) { /* $5600 */
    short mon;
    if (Party[3] != 0x80) {
        Failed();
        return;
    }
    Flashriek();
    for (mon = 7; mon >= 0; mon--) {
        if ((RandNum(0, 255) & 0x03) != 0) {
            if (MonsterHP[mon] != 0) {
                unsigned char orgTile = GetXYTile(MonsterX[mon], MonsterY[mon]);
                gBallTileBackground = MonsterTile[mon];
                PutXYTile(0x78, MonsterX[mon], MonsterY[mon]);
                DrawTiles();
                ShowHit(MonsterX[mon], MonsterY[mon], 0x78, MonsterTile[mon]);
                PlaySoundFile(CFSTR("Hit"), FALSE);    // was 0xF7
                DrawMapPause();
                PutXYTile(orgTile, MonsterX[mon], MonsterY[mon]);
                DrawTiles();
                DamageMonster(mon, damage, chnum);
            }
        }
    }
}

void Necorp(void) { /* $5677 */
    short mon;
    if (Party[3] != 0x80) {
        Failed();
        return;
    }
    Flashriek();
    for (mon = 7; mon >= 0; mon--) {
        if (MonsterHP[mon] > 0) {
            MonsterHP[mon] = 5;
            unsigned char orgTile = GetXYTile(MonsterX[mon], MonsterY[mon]);
            gBallTileBackground = MonsterTile[mon];
            PutXYTile(0x78, MonsterX[mon], MonsterY[mon]);
            DrawTiles();
            ShowHit(MonsterX[mon], MonsterY[mon], 0x78, MonsterTile[mon]);
            PlaySoundFile(CFSTR("Hit"), FALSE);    // was 0xF7
            DrawMapPause();
            PutXYTile(orgTile, MonsterX[mon], MonsterY[mon]);
            DrawTiles();
        }
    }
}

void Heal(short damage) { /* $56F7 */
    short chnum;
    UPrintMessage(130);
    chnum = GetChar();
    if (chnum < 1 || chnum > 4) {
        Failed();
        return;
    }
    HPAdd(Party[6 + chnum], damage);
    InverseChar(chnum - 1);
    Flashriek();
    InverseChar(chnum - 1);
    return;
}

void Failed(void) { /* $586E */
    UPrintMessage(86);
    PlaySoundFile(CFSTR("FailedSpell"), TRUE);    // was 0xDA
}

void Flashriek(void) { /* $5885 */
    short SpellSound[34] = {0, 1, 2, 3, 4, 1, 5, 1, 2, 7, 0, 1, 7, 0, 0, 0, 0, 7, 6, 2, 4, 3, 5, 6, 5, 2, 6, 7, 1, 6, 0, 6, 7, 0};

    InverseTiles();
    switch (SpellSound[spellnum]) {
        case 0:
            PlaySoundFile(CFSTR("BigDeath"), FALSE);
            break;    // was 0xE0
        case 1:
            PlaySoundFile(CFSTR("Immolate"), FALSE);
            break;    // was 0xE9
        case 2:
            PlaySoundFile(CFSTR("TorchIgnite"), FALSE);
            break;    // was 0xDF
        case 3:
            PlaySoundFile(CFSTR("Downwards"), FALSE);
            break;    // was 0xDE
        case 4:
            PlaySoundFile(CFSTR("Upwards"), FALSE);
            break;    // was 0xDD
        case 5:
            PlaySoundFile(CFSTR("Invocation"), FALSE);
            break;    // was 0xF4
        case 6:
            PlaySoundFile(CFSTR("Heal"), FALSE);
            break;    // was 0xF3
        default:
            PlaySoundFile(CFSTR("MiscSpell"), FALSE);
            break;    // was 0xDC
    }
    InverseTiles();
}

void RelocateDungeon(void) { /* $572B */
    short value;
    value = -1;
    while (value != 0) {
        xs = RandNum(0, 16);
        ys = RandNum(0, 16);
        value = GetXYDng(xs, ys);
    }
    xpos = xs;
    ypos = ys;
}

void DownLevel(void) {
    if (Party[3] != 1) {
        Failed();
        return;
    }
    Flashriek();
    if (dungeonLevel > 6) {
        Failed();
        return;
    }
    dungeonLevel++;
    RelocateDungeon();
}

void UpLevel(void) {
    if (Party[3] != 1) {
        Failed();
        return;
    }
    Flashriek();
    dungeonLevel--;
    if (dungeonLevel < 0) {
        gExitDungeon = 1;
        return;
    }
    RelocateDungeon();
}

unsigned char CombatValidMove(short value) { /* $7E31 */
    if (gMonType < 0x16 || gMonType >= 0x20) {
        if (value == 2)
            return 0;
        if (value == 4)
            return 0;
        if (value == 6)
            return 0;
        if (value == 0x10)
            return 0;
        return 0xFF;
    } else {
        if (value == 0)
            return 0;
        return 0xFF;
    }
}

unsigned char ValidMove(short value) { /* $7E5B */
    if (value == 2)
        goto good;
    if (value == 4)
        goto good;
    if (value == 6)
        goto good;
    if (value == 0x10)
        goto good;
    return 0xFF;
good:
    PlaySoundFile(CFSTR("Step"), TRUE);    // was 0xF6, TRUE
    return 0;
}

void GetScreen(short resid) {
    short ptr;
    Handle screen;
    screen = GetResource('CONS', resid);
    LoadResource(screen);
    for (ptr = 0; ptr < 121; ptr++) {
        TileArray[ptr] = *(*screen + ptr);
    }
    for (ptr = 0; ptr < 8; ptr++) {
        MonsterX[ptr] = *(*screen + ptr + 0x80);
        MonsterY[ptr] = *(*screen + ptr + 0x88);
        MonsterTile[ptr] = *(*screen + ptr + 0x90);
        MonsterHP[ptr] = *(*screen + ptr + 0x98);
    }
    for (ptr = 0; ptr < 4; ptr++) {
        CharX[ptr] = *(*screen + ptr + 0xA0);
        CharY[ptr] = *(*screen + ptr + 0xA4);
        CharTile[ptr] = *(*screen + ptr + 0xA8);
        CharShape[ptr] = *(*screen + ptr + 0xAC);
    }
    ReleaseResource(screen);
}

unsigned char ExodusCastle(void) { /* $6F43 */
    short value;
    value = Party[3];
    if (Party[16] == 1)
        return 0xFF;
    if (value == 0x80)
        value = g835E;
    if (value != 3)
        return 0xFF;
    if (Party[4] != LocationX[1])
        return 0xFF;
    return 0;    // In Exodus Castle!
}

unsigned char CombatMonsterHere(short x, short y) { /* $7D24 */
    short mon, result;
    result = 255;
    for (mon = 7; mon >= 0; mon--) {
        if (MonsterHP[mon] != 0) {
            if (MonsterX[mon] == x && MonsterY[mon] == y) {
                result = mon;
            }
        }
    }
    return result;
}

unsigned char CombatCharacterHere(short x, short y) {
    char i;
    for (i = 3; i >= 0; i--) {
        if (CharX[i] == x && CharY[i] == y)
            return i;
    }
    return 255;
}

short Shoot(short x, short y) { /* $7D41 */
shoot0:
    x += dx;
    y += dy;
    if (x < 0 || x > 10 || y < 0 || y > 10) {
        DrawTiles();
        return 255;
    }
    unsigned char monster = CombatMonsterHere(x, y);
    unsigned char character = CombatCharacterHere(x, y);
    short orgval = GetXYTile(x, y);
    gBallTileBackground = orgval;
    if (monster != 255)
        gBallTileBackground = MonsterTile[monster];
    else if (character != 255)
        gBallTileBackground = CharTile[character];
    PutXYTile(zp[0x1F], x, y);
    DrawTiles();
    DrawMapPause();
    PutXYTile(orgval, x, y);
    orgval = monster;
    if (orgval > 127)
        goto shoot0;
    return orgval;
}

void DamageMonster(short which, short damage, short chnum) { /* $84C7 */
    Str32 str, numStr;
    short expnum;

    if (gMonType != 0x26) { /* not Lord British, har har */
        if ((MonsterHP[which] - damage) < 1) {
            GetPascalStringFromArrayByIndex(str, CFSTR("Messages"), 130);
            //GetIndString(str, BASERES+12, 131);
            expnum = ((gMonType / 2) & 0x0F);
            NumToString(Experience[expnum], numStr);
            BlockMove(numStr + 1, str + str[0] + 1, numStr[0]);
            str[0] += numStr[0];
            str[++str[0]] = '\n';
            UPrintWin(str);
            AddExp(chnum, Experience[expnum]);
            PutXYTile(MonsterTile[which], MonsterX[which], MonsterY[which]);
            MonsterHP[which] = 0;
            DrawTiles();
        } else {
            MonsterHP[which] -= damage;
        }
    }
}

unsigned char GetXYDng(short x, short y) { /* $93DE */
    if (y < 0)
        y += 16;
    if (x < 0)
        x += 16;
    if (y > 15)
        y -= 16;
    if (x > 15)
        x -= 16;
    return Dungeon[(dungeonLevel * 256) + (y * 16) + x];
}

void PutXYDng(unsigned char value, short x, short y) { /* ?? */
    Dungeon[(dungeonLevel * 256) + (y * 16) + x] = value;
}

short BackGround(short montype) { /* A, +1=B, +2=C, +3=F, +4=G, +5=M, +6=Q, +7=R, +8=S */
    short tile;
    if (Party[3] == 1)
        return BASERES + 2;
    if (montype == 0x10)
        return BASERES + 2;    // fighting floor on floor
    if (montype == 0x02)
        return BASERES + 4;    // fighting grass on grass [pass the bong!]
    if (montype == 0x1E)       // Frigate filled with
    {
        gMonType = 0x2E;    // Thieves
        if (Party[1] == 0x16)
            return BASERES + 8;
        return BASERES;
    }
    if (Party[1] == 0x16) {
        if (montype < 0x20)
            return BASERES + 6;
        return BASERES + 7;
    }
    if ((montype < 0x20) && (montype > 0x14))
        return BASERES + 5;
    tile = GetXYVal(xpos, ypos) / 2;
    if (tile == 0x02)
        return BASERES + 4;
    if (tile == 0x04)
        return BASERES + 1;
    if (tile == 0x06)
        return BASERES + 3;
    if (tile == 0x10)
        return BASERES + 2;
    if (tile == 0x40)
        return BASERES + 2;
    if (tile == 0x42)
        return BASERES + 2;
    return BASERES + 4;
}

unsigned char HowMany(void) { /* $80DE */
    if (gMonType == 0x26)
        return 0;
    if (gMonType == 0x24)
        return 7;
    if (ExodusCastle() == 0)
        return 7;
    if (Party[3] < 2)
        return RandNum(0, 7);    // Party[3]'s were g835E, which wasn't set yet!
    if (Party[3] > 3)
        return RandNum(0, 7);
    return 0;
}

void HandleMove(short chnum) { /* $828D */
    short value;
    xs = CharX[chnum] + dx;
    if (xs < 0 || xs > 10)
        goto bad;
    ys = CharY[chnum] + dy;
    if (ys < 0 || ys > 10)
        goto bad;
    value = ValidMove(GetXYTile(xs, ys));
    if (value != 0)
        goto bad;
    PutXYTile(CharTile[chnum], CharX[chnum], CharY[chnum]);
    CharX[chnum] = xs;
    CharY[chnum] = ys;
    CharTile[chnum] = GetXYTile(xs, ys);
    PutXYTile(CharShape[chnum], xs, ys);
    return;
bad:
    UPrintMessage(116);
    PlaySoundFile(CFSTR("Bump"), TRUE);    // was 0xE7
    return;
}

void Victory(void) { /* $8535 */
    gTimeNegate = gSongCurrent = gSongNext = 0;
    UPrintMessage(132);
    PlaySoundFile(CFSTR("CombatVictory"), TRUE);    // was 0xED
    gSongCurrent = g835F;
    gSongNext = g835F;
    Party[3] = g835E;
    ShowWind();
    if (Party[3] == 1) {
        DrawDungeon();
        return;
    }
    DrawMap(xpos, ypos);
}

void Combat(void) { /* $7FB0 */
    short mon, numMon, chnum, health, value, temp, updateStore;
    unsigned char count, count2;
    unsigned char monhpstart[16] = {0x20, 0x20, 0xF0, 0xF0, 0xC0, 0x60, 0xA0, 0x80, 0x30, 0x50, 0x70, 0xA0, 0xC0, 0xE0, 0xF0, 0xF0};
    //Str255            str;

    g835F = gSongCurrent;
    g5521 = g56E7 = 0;
    gSongCurrent = gSongNext = 0;
    for (mon = 31; mon >= 0; mon--) {
        if (Monsters[mon] == 0x4C || Monsters[mon] == 0x48)
            Monsters[mon + HPMON] = 0xC0;
    }
    UPrintMessage(133);
    numMon = HowMany();
    PrintMonster(gMonType, (numMon > 0), gMonVarType);
    UPrintWin("\p\n\n");
    GetScreen(BackGround(gMonType));
    g835E = Party[3];
    Party[3] = 0x80;
    ShowWind();
    updateStore = gUpdateWhere;
    gUpdateWhere = 3; /* a la outside */
    for (chnum = Party[2] - 1; chnum >= 0; chnum--) {
        health = Player[Party[7 + chnum]][17];
        if (health == 'G' || health == 'P') {
            CharShape[chnum] = DetermineShape(Player[Party[7 + chnum]][23]);
            CharTile[chnum] = GetXYTile(CharX[chnum], CharY[chnum]);
            PutXYTile(CharShape[chnum], CharX[chnum], CharY[chnum]);
        } else {
            CharX[chnum] = 0xFF;
            CharY[chnum] = 0xFF;
        }
    }
    for (mon = 7; mon >= 0; mon--) {
        MonsterHP[mon] = 0;
    }
    while (numMon >= 0) {
        mon = RandNum(0, 7);
        if (MonsterHP[mon] == 0) {
            health = (gMonType / 2) & 0x0F;
            MonsterHP[mon] = RandNum(0, monhpstart[health]) | 0x0F;
            MonsterTile[mon] = GetXYTile(MonsterX[mon], MonsterY[mon]);
            unsigned char tileValue = gMonType;
            if (gMonVarType && gMonType >= 46 && gMonType <= 63)
                tileValue = (((gMonType / 2) - 23) * 2 + 79 + gMonVarType) * 2;
            PutXYTile(tileValue, MonsterX[mon], MonsterY[mon]);
            numMon--;
        }
    }
    DrawTiles();
    PlaySoundFile(CFSTR("CombatStart"), FALSE);    // was 0xEE
    FlushEvents(keyDownMask | keyUpMask, 0);
    FlushEvents(mDownMask | mUpMask, 0);
    gSongNext = 5;
    gAutoCombat = !CFPreferencesGetAppBooleanValue(U3PrefManualCombat, kCFPreferencesCurrentApplication, NULL);
combatstart:
    AgeChars();
    ShowChars(false);
    g835D = 0;
combatloop: /* $8164 */
    if (gDone)
        return;
    if (gAutoCombat)
        ObscureCursor();
    chnum = g835D;
    gChnum = chnum;
    if (gResurrect) {
        gUpdateWhere = updateStore;
        return;
    }
    InverseChnum(chnum);
    if (CheckAlive(chnum) == FALSE)
        goto plrdone;
    count = 0x2F;
    UPrintMessage(134);
    UPrintNumPad(chnum + 1, 1);
    wx++;
    UPrintMessage(135);
    DrawPrompt();
    gMouseState = 5;
    dx = dy = 0;
    xs = CharX[chnum];
    ys = CharY[chnum];
    count2 = 0xC0;
keyloop:
    count2 += 0x40;
    if (count2 == 0) {
        PutXYTile(CharTile[chnum], xs, ys);
        cHide = chnum + 1;
    } else {
        PutXYTile(CharShape[chnum], xs, ys);
        cHide = 0;
    }
    ScrollThings();
    AnimateTiles();
    DrawTiles();
    count--;
    //  gAutoCombat &= (!NearlyDead(0));
    if (gAutoCombat && Macro[0] == 0)
        AutoCombat(chnum);
    if (count == 0) {
        gKeyPress = ' ';
    } else {
        if (GetKeyMouse(0) == FALSE)
            goto keyloop;
    }
    PutXYTile(CharShape[chnum], xs, ys);
    cHide = 0;
    DrawTiles();
    while (gKeyPress > 'Z') {
        gKeyPress -= 32;
    }
    if (gKeyPress < 'A' || gKeyPress > 'Z') {
        if (gKeyPress == '4')
            gKeyPress = 28;
        if (gKeyPress == '6')
            gKeyPress = 29;
        if (gKeyPress == '8')
            gKeyPress = 30;
        if (gKeyPress == '2')
            gKeyPress = 31;
        Boolean allowDiagonal = !(CFPreferencesGetAppBooleanValue(U3PrefNoDiagonals, kCFPreferencesCurrentApplication, NULL));
        switch (gKeyPress) {
            case '1':
                if (allowDiagonal) {
                    UPrintMessage(250);
                    dx = -1;
                    dy = 1;
                    HandleMove(chnum);
                }
                break;
            case '3':
                if (allowDiagonal) {
                    UPrintMessage(251);
                    dx = 1;
                    dy = 1;
                    HandleMove(chnum);
                }
                break;
            case '7':
                if (allowDiagonal) {
                    UPrintMessage(252);
                    dx = -1;
                    dy = -1;
                    HandleMove(chnum);
                }
                break;
            case '9':
                if (allowDiagonal) {
                    UPrintMessage(253);
                    dx = 1;
                    dy = -1;
                    HandleMove(chnum);
                }
                break;
            case 28:
                UPrintMessage(27);
                dx = -1;
                HandleMove(chnum);
                break;
            case 29:
                UPrintMessage(26);
                dx = 1;
                HandleMove(chnum);
                break;
            case 30:
                UPrintMessage(24);
                dy = -1;
                HandleMove(chnum);
                break;
            case 31:
                UPrintMessage(25);
                dy = 1;
                HandleMove(chnum);
                break;
            case ' ': UPrintMessage(23); break;
            default: What2(); break;
        }
        goto plrdone;
    }
    switch (gKeyPress) {
        case 'A': CombatAttack(chnum); break;
        case 'C':
            zp[0x1F] = 0x78;
            UPrintMessage(136);
            if (Cast(1, chnum + 1) == FALSE) {
                UPrintWin("\p ");
                DrawFramePiece(8, 23, 23);
                DrawFramePiece(12, 24, 23);
                goto keyloop;
            }
            break;
        case 'N':
            UPrintMessage(137);
            NegateTime(1, chnum + 1);
            break;
        case 'R':
            UPrintMessage(138);
            ReadyWeapon(chnum + 1, 0);
            break;
        case 'V': Volume(); break;
        case 'Z':
            UPrintMessage(139);
            Stats(1, chnum + 1);
            break;
        default:
            UPrintMessage(140);
            PlaySoundFile(CFSTR("Error2"), TRUE);    // was 0xFE
            DrawFramePiece(8, 23, 23);
            DrawFramePiece(12, 24, 23);
            goto keyloop;
            break;
    }
    goto plrdone;
    return;
plrdone: /* $85A6 */
    if (g835F == 7)
        gTimeNegate = 0;
    chnum = g835D;
    DrawTiles();
    UnInverseChnum(chnum);
    value = 1;
    for (mon = 0; mon < 8; mon++) {
        if (MonsterHP[mon] != 0)
            value = 0;
    }
    ShowChars(false);
    if (value == 1) {
        Victory();
        gUpdateWhere = updateStore;
        return;
    }
    wy = 0x17;
    wx = 0x18;
    g835D++;
    if (g835D < Party[2])
        goto combatloop;
    if (gTimeNegate != 0) {
        gTimeNegate--;
        goto combatstart;
    }
    mon = -1;
nextmon:
    mon++;
    if (gResurrect)
        goto combatstart;
    if (mon > 7)
        goto combatstart;
    if (MonsterHP[mon] == 0)
        goto nextmon;
    gChnum = FigureNewMonPosition(mon);
    zp[0x1F] = 0x7A;
    if (zp[0xD0] == 0)
        goto afternext;
    if (gChnum >= 0) {
        if (RandNum(0, 255) < 128) {
            if (gMonType == 0x3A)
                goto monshoot;
        }
    }
    if (RandNum(0, 0xC0) > 127) {
        if (gMonType == 0x1A)
            goto monmagic;
        if (gMonType == 0x1C)
            goto monmagic;
        if (gMonType == 0x2C)
            goto monmagic;
        if (gMonType == 0x36)
            goto monmagic;
        if (gMonType == 0x3A)
            goto monmagic;
        if (gMonType == 0x3C)
            goto monmagic;
        if (gMonType == 0x26)
            goto monlb;
    }
nextplr:
    if (zp[0xD0] >= 0 && zp[0xD0] < 0x80)
        goto monlb;
    goto nextmon;
monmagic: /* $864A */
    gChnum = RandNum(0, 255) & 3;
    if (Player[Party[7 + gChnum]][17] != 'G')
        goto nextplr;
    InverseTiles();
    PlaySoundFile(CFSTR("MonsterSpell"), FALSE);    // was 0xF2, was Whine(0x40,0x40);
    InverseTiles();
    zp[0x1F] = 0x78;
    goto afternext;
monlb: /* $8672 */
    PutXYTile(MonsterTile[mon], MonsterX[mon], MonsterY[mon]);
    MonsterX[mon] = zp[0xF7];
    MonsterY[mon] = zp[0xF8];
    MonsterTile[mon] = GetXYTile(MonsterX[mon], MonsterY[mon]);
    unsigned char tileValue = gMonType;
    if (gMonVarType && gMonType >= 46 && gMonType <= 63)
        tileValue = (((gMonType / 2) - 23) * 2 + 79 + gMonVarType) * 2;
    PutXYTile(tileValue, MonsterX[mon], MonsterY[mon]);
    DrawTiles();
    goto nextmon;
monshoot:                                   /* $86A4 */
    PlaySoundFile(CFSTR("Shoot"), TRUE);    // was 0xEA
    xs = MonsterX[mon];
    ys = MonsterY[mon];
monshoot2: /* $86B5 */
    xs += zp[0xF5];
    if (xs > 10 || xs < 0)
        goto drawdone;
    ys += zp[0xF6];
    if (ys > 10 || ys < 0)
        goto drawdone;
    chnum = Party[2] - 1;
felch: /* $86CE */
    if (xs == CharX[chnum] && ys == CharY[chnum]) {
        gChnum = chnum;
        goto c8777;
    }
    chnum--;
    if (chnum >= 0)
        goto felch;
    temp = GetXYTile(xs, ys);
    gBallTileBackground = temp;
    PutXYTile(0x7A, xs, ys);
    DrawTiles();
    DrawMapPause();
    PutXYTile(temp, xs, ys);
    goto monshoot2;
drawdone: /* $86F7 */
    DrawTiles();
    DrawMapPause();
    goto nextmon;
afternext: /* $86FD */
    if (gMonType == 0x1C || gMonType == 0x3C || gMonType == 0x38) {
        Poison(gChnum);
    } else {
        if (gMonType == 0x2E)
            Pilfer(gChnum);
    }
    UPrintMessage(141);
    UPrintNumPad(gChnum + 1, 1);
    wx++;
    PlaySoundFile(CFSTR("Attack"), FALSE);    // was 0xF8

    // If in Exodus Castle and the character is not wearing Exotic, it's an automatic hit.
    if (g835E == 3 && Party[4] == LocationX[1] && Player[Party[7 + gChnum]][40] != 7)
        goto plrhit;

    // Random from 0 to armour+16 -- less than 8 is a hit.
    temp = RandNum(0, Player[Party[7 + gChnum]][40] + 0x10);
    if (temp < 8)
        goto plrhit;

    UPrintMessage(142);    // Missed
    goto nextmon;
plrhit:                    /* $876D */
    UPrintMessage(143);    // Hit
c8777:
    temp = ((Player[Party[7 + gChnum]][28] * 256) + Player[Party[7 + gChnum]][29]) / 100;
    temp = ((monhpstart[(gMonType / 2) & 0x0F] / 8) + temp) | 1;
    temp = RandNum(0, temp) + 1;
    HPSubtract(Party[7 + gChnum], temp);
    HPSubtract(Party[7 + gChnum], (g835E & 3) * 16);
    InverseChar(gChnum);
    gBallTileBackground = CharTile[gChnum];
    PutXYTile(zp[0x1F], CharX[gChnum], CharY[gChnum]);
    DrawTiles();
    ShowHit(CharX[gChnum], CharY[gChnum], 0x7A, CharTile[gChnum]);
    PlaySoundFile(CFSTR("Hit"), FALSE);    // was 0xF7
    DrawMapPause();
    PutXYTile(CharShape[gChnum], CharX[gChnum], CharY[gChnum]);
    DrawTiles();
    InverseChar(gChnum); /* <--- I had to add this fucker! */
    if (Player[Party[7 + gChnum]][17] == 'D') {
        UPrintMessage(144);
        PutXYTile(CharTile[gChnum], CharX[gChnum], CharY[gChnum]);
        CharX[gChnum] = 0xFF;
        CharY[gChnum] = 0xFF;
        DrawTiles();
        ShowChars(false);
        CheckAllDead();
    }
    ShowChars(false);
    wx = 0x18;
    wy = 0x17;
    goto nextmon;
}

unsigned char DetermineShape(short type) { /* $7F5D */
    if (type == careerTable[0])
        return 0x80;
    if (type == careerTable[1])
        return 0x82;
    if (type == careerTable[2])
        return 0x84;
    if (type == careerTable[3])
        return 0x86;
    if (type == careerTable[4])
        return 0x80;
    if (type == careerTable[5])
        return 0x80;
    if (type == careerTable[6])
        return 0x22;
    if (type == careerTable[7])
        return 0x84;
    if (type == careerTable[8])
        return 0x82;
    if (type == careerTable[9])
        return 0x84;
    return 0x7E;
}

void CombatAttack(short chnum) { /* $8360 */
    short wpn, rosNum, mon, damage;
    Str255 str1, str2;

    zp[0x1F] = 0x7A; /* Red ball */
    rosNum = Party[7 + chnum];
    wpn = Player[rosNum][48];
    GetPascalStringFromArrayByIndex(str1, CFSTR("WeaponsArmour"), wpn);
    GetPascalStringFromArrayByIndex(str2, CFSTR("Messages"), 144);
    //GetIndString(str2, BASERES+12, 145);
    AddString(str1, str2);
    UPrintWin(str1);

    GetDirection(1);
    if (dx == 0 && dy == 0)
        return;
    /* was Whine(chnum*8+0xE0, 6); */
    if (chnum == 0)
        PlaySoundFile(CFSTR("Swish1"), FALSE);
    else if (chnum == 1)
        PlaySoundFile(CFSTR("Swish2"), FALSE);
    else if (chnum == 2)
        PlaySoundFile(CFSTR("Swish3"), FALSE);
    else
        PlaySoundFile(CFSTR("Swish4"), FALSE);

    if (wpn == 3 || wpn == 5 || wpn == 9 || wpn == 13) {
    projectile:
        mon = Shoot(CharX[chnum], CharY[chnum]);
        if (mon > 127) {
            Missed();
            return;
        }
    } else {
        mon = CombatMonsterHere(CharX[chnum] + dx, CharY[chnum] + dy);
        if (mon > 127 && wpn != 1) {
            Missed();
            return;
        }
        if (mon > 127 && wpn == 1) {   // Thrown a dagger
            Player[rosNum][49]--;
            if (Player[rosNum][49] < 1 || Player[rosNum][49] > 250) {
                Player[rosNum][48] = 0;
                Player[rosNum][49] = 0;
            }
            mon = Shoot(CharX[chnum], CharY[chnum]);
            if (mon > 127) {
                Missed();
                return;
            }
        }
    }
    if (ExodusCastle() == 0 && wpn != 15) {
        Missed();
        return;
    }
    if (RandNum(0, 255) < 128) {
        if (Player[rosNum][19] < RandNum(0, 99)) {
            Missed();
            return;
        }
    }
    PrintMonster(gMonType, false, gMonVarType);
    UPrintMessage(146);
    gBallTileBackground = MonsterTile[mon];
    PutXYTile(zp[0x1F], MonsterX[mon], MonsterY[mon]);
    DrawTiles();
    ShowHit(MonsterX[mon], MonsterY[mon], 0x7A, MonsterTile[mon]);
    PlaySoundFile(CFSTR("Hit"), FALSE);    // was 0xF7
    DrawMapPause();
    PutXYTile(gMonType, MonsterX[mon], MonsterY[mon]);
    damage = RandNum(0, (Player[rosNum][18] | 1));
    damage += Player[rosNum][18] / 2;
    damage += Player[rosNum][48] * 3;
    damage += 4;
    DamageMonster(mon, damage, chnum + 1);
}

void Missed(void) { /* $8414 */
    UPrintMessage(147);
    DrawTiles();
}

short FigureNewMonPosition(short mon) { /* $7E85 */
    short rosNum, health, plrNum, plrTarget, newx, newy;
    zp[0xD0] = 0xFF;
    plrTarget = 0xFF;
    plrNum = -1;
nextPlrTarget:
    plrNum++;
    if (plrNum >= Party[2])
        goto markdone;
    rosNum = Party[7 + plrNum];
    health = Player[rosNum][17];
    if (health == 'D' || health == 'A')
        goto nextPlrTarget;
    dx = CharX[plrNum] - MonsterX[mon];
    zp[0xD1] = Absolute(dx);
    dy = CharY[plrNum] - MonsterY[mon];
    zp[0xD2] = Absolute(dy);
    if ((zp[0xD2] < 2) && (zp[0xD1] < 2))
        goto markclose;
    if ((zp[0xD1] + zp[0xD2]) >= zp[0xD0])
        goto nextPlrTarget;
    zp[0xD1] += zp[0xD2];
    dx = GetHeading(dx);
    newx = MonsterX[mon] + dx;
    dy = GetHeading(dy);
    newy = MonsterY[mon] + dy;
    if (CombatValidMove(GetXYTile(newx, newy)) != 0) {
        newx = MonsterX[mon];
        newy = MonsterY[mon] + dy;
        if (CombatValidMove(GetXYTile(newx, newy)) != 0) {
            newx = MonsterX[mon] + dx;
            newy = MonsterY[mon];
            if (CombatValidMove(GetXYTile(newx, newy)) != 0)
                goto nextPlrTarget;
        }
    }
mark2: /* $7F29 */
    plrTarget = plrNum;
    zp[0xD0] = zp[0xD1];
    zp[0xF7] = newx;
    zp[0xF8] = newy;
    zp[0xF5] = dx;
    zp[0xF6] = dy;
    goto nextPlrTarget;
markclose: /* $7F44 */
    zp[0xD0] = 0;
    plrTarget = plrNum;
    if (zp[0xD1] + zp[0xD2] < 2)
        goto markdone;
    goto nextPlrTarget;
markdone:
    plrNum = plrTarget;
    /*  ForeColor(redColor);
    MoveTo(MonsterX[mon]*32+32,MonsterY[mon]*32+32);
    LineTo(CharX[plrTarget]*32+32,CharY[plrTarget]*32+32);
    ForeColor(blackColor); */
    return plrNum;
}

void Pilfer(short chnum) { /* $881F */
    short rosNum, item;
    rosNum = Party[7 + chnum];
    if (RandNum(0, 255) < 128) {
        item = RandNum(0, 15);
        if (item == 0)
            return;
        if (Player[rosNum][48] == item)
            return;
        if (Player[rosNum][48 + item] == 0)
            return;
        Player[rosNum][48 + item] = 0;
    } else {
        item = RandNum(0, 7);
        if (item == 0)
            return;
        if (Player[rosNum][40] == item)
            return;
        if (Player[rosNum][40 + item] == 0)
            return;
        Player[rosNum][40 + item] = 0;
    }
    UPrintMessage(141);
    UPrintNumPad(chnum + 1, 1);
    wx++;
    UPrintMessage(148);
    PlaySoundFile(CFSTR("Ouch"), FALSE);    // was 0xFA
}

void Poison(short chnum) { /* $8881 */
    short rosNum;
    rosNum = Party[7 + chnum];
    if ((RandNum(0, 255) & 0x03) != 0)
        return;
    if (Player[rosNum][17] != 'G')
        return;
    Player[rosNum][17] = 'P';
    UPrintMessage(141);
    UPrintNumPad(chnum + 1, 1);
    wx++;
    UPrintMessage(149);
    PlaySoundFile(CFSTR("Ouch"), FALSE);    // was 0xFA
}

void ShowHit(short x, short y, unsigned char hitType, unsigned char tileUnder) {
    short orgVal;
    //long      time;
    if (!CFPreferencesGetAppBooleanValue(U3PrefSoundInactive, kCFPreferencesCurrentApplication, NULL))
        return;
    //if (prefs.soundActive) return;
    orgVal = GetXYTile(x, y);
    gBallTileBackground = tileUnder;
    PutXYTile(hitType, x, y);
    SwapShape(hitType);
    DrawTiles();
    ThreadSleepTicks(10);
    SwapShape(hitType);
    PutXYTile(orgVal, x, y);
    DrawTiles();
}
