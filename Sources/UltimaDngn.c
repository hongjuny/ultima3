// Dungeon routines

#import "UltimaDngn.h"

#import "CarbonShunts.h"
#import "CocoaBridge.h"
#import "UltimaGraphics.h"
#import "UltimaMacIF.h"
#import "UltimaMain.h"
#import "UltimaMisc.h"
#import "UltimaNew.h"
#import "UltimaSound.h"
#import "UltimaSpellCombat.h"
#import "UltimaText.h"

extern Boolean          gDone, gResurrect;
extern short            zp[255], gUpdateWhere, gTorch, gDepth, gCurMapID;
extern char             dungeonLevel, gKeyPress;
extern unsigned char    Dungeon[2048], Player[21][65], Party[64];
extern int              xpos, ypos, tx, ty, xs, ys, wx, wy;
extern CGrafPtr         mainPort, tilesPort, gamePort;
extern GDHandle         mainDevice;
extern char             HeadX[4], HeadY[4];
extern short            heading, gMonType, gExitDungeon, gMouseState;
extern OSErr            gError;
extern Str255           gString;
extern short            gSongCurrent, gSongNext, gSongPlaying;
extern Boolean          gUnusualSize;
extern short            blkSiz;

BitMap          dungPortMap;
CGrafPtr        dungPort;
PixMapHandle    dungPixMap;

CGrafPtr        nDngShapes, nDngMasks;
PixMapHandle    nDngShapesPixMap, nDngMasksPixMap;

/*                           0    1    2    3    4    5    6    7    8    9   10   11   12   13   14   15   16   17   18   19   20   21   22   23   24   25   26   27   28   29   30   31 */
const short     dOfX[37]={ 600,   0,   0,1200,1200,1200, 300,   0,   0, 300,1200,1200,1200,1200,1200, 300, 300,   0,   0, 300, 300,1200,1200,1200,1200,1200,1200,1200, 300,   0,   0, 300};
const short     dOfY[37]={   0,   0,   0, -64, -64, -64, -64,   0,   0, -64,  34,  34,  34,  34,  34,  34, -64,   0,   0, -64,  34,  84,  84,  84,  84,  84,  84,  84, -64,   0,   0, -64};
const Boolean   dMask[37]={  0,   1,   1,   0,   0,   0,   1,   1,   1,   1,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1};

/*                          0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32  33  34  35 */
const short     dngL[37]={  0,  0,225,  0, 75,225,  0, 75,187,225,  0, 75,113,187,225,  0, 75,113,170,188,226,  0, 75,112,130,170,188,225,112,130,152,168,  0,300,375,377};
const short     dngT[37]={  0,  0,  0, 64, 64, 64, 64, 65, 65, 64, 96, 96, 96, 96, 96, 96, 96, 97, 97, 96, 96,112,112,112,112,112,112,112,112,112,112,112,  0,208,208,208};
const short     dngR[37]={300, 75,300, 75,225,300, 75,113,225,300, 75,113,187,225,300, 74,112,130,187,225,300, 75,112,130,170,188,225,300,132,148,170,188,300,375,377,381};
const short     dngB[37]={256,256,256,192,192,192,192,191,191,192,160,160,160,160,160,160,160,159,159,160,160,144,144,144,144,144,144,144,144,144,144,144,256,240,212,256};
/*                 0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20 */
short   chL[21]={112,  0,264, 56,131,226,  0, 75,207,272, 66,103,141,188,226, 28, 75,113,179,216,254};
short   chT[21]={208,208,208,168,168,168,168,168,168,168,148,148,148,148,148,148,148,148,148,148,148};
short   chR[21]={186, 36,300, 74,169,244, 18, 93,225,300, 74,112,159,197,234, 46, 84,121,187,225,272};
short   chB[21]={240,240,240,184,184,184,184,184,184,184,156,156,156,156,156,156,156,156,156,156,156};
short   chSide[21]={0,2,1,1,0,2,2,2,1,1,1,1,0,2,2,0,2,2,1,1,0};

#define PASSTIME 360          // 6 seconds

// ____________________________________________________________________
// Local prototypes

void dPass(void);
void Forward(void);
void Retreat(void);
void Right(void);
void Left(void);
void dDescend(void);
void dKlimb(void);
void dPeer(void);
void NotDngCmd(void);
void InvalCmd(void);
void DrawDungeonBackGround(void);
short DungeonBlock(short location);
void DrawSecretMessage(void);
Boolean ShowSecret(short which);
void DimDungeon(void);
short DrawWall(short location);
void DrawLadder(short location, short direction);
void DrawChest(short location);
void DrawDoor(short location);

// ____________________________________________________________________

void DungeonStart(short mode) { /* $8CE9 */
    short value, chnum, rosNum;
    char bits[8] = {1, 2, 4, 8, 16, 32, 64, 128};
    long nextPassTime;

    gUpdateWhere = 4;
    if (mode == 1)
        goto dungeonmech;
    gSongCurrent = gSongNext = 0;
    gTorch = 0;
    DrawDungeon();
    gSongCurrent = gSongNext = 4;
    gExitDungeon = 0;
dungstart: /* $8D13 */
    if (gExitDungeon == 1)
        return;
    CheckAllDead();
    DngInfo();
    wx = 0x18;
    wy = 0x17;
    if (gTorch == 0) {
        UPrintMessage(150);
        ClearTiles();
    }
    DrawDungeon();
    UPrintWin("\p ");
    DrawPrompt();
    gMouseState = 2;
    nextPassTime = TickCount() + PASSTIME;
    zp[0xD1] = zp[0xD2] = 0;
    zp[0x12] = 5;
dungkeystart: /* $8D4A */
    if (zp[0xD1] == -2000)
        DoAutoHeal();
    zp[0xD1]--;
    if (TickCount() < nextPassTime)
        goto dungkey;
    //if (zp[0xD1]>-5000) goto dungkey;
    gKeyPress = ' ';
    goto dungkeygot;
dungkey: /* $8D59 */
    if (gResurrect)
        return;
    if (!GetKeyMouse(0))
        goto dungkeystart;
dungkeygot:
    if (gDone)
        return;
    if (gKeyPress > 95)
        gKeyPress -= 32;
    if (gKeyPress >= 'A' && gKeyPress <= 'Z') {
        switch (gKeyPress) {
            case 'A': NotDngCmd(); break;
            case 'B': NotDngCmd(); break;
            case 'C': Cast(0, 0); break;
            case 'D': dDescend(); break;
            case 'E': NotDngCmd(); break;
            case 'F': NotDngCmd(); break;
            case 'G': GetChest(0, 0); break;
            case 'H': HandEquip(0, 0, 0, 0, 0); break;
            case 'I': Ignite(); break;
            case 'J': JoinGold(0); break;
            case 'K': dKlimb(); break;
            case 'L': NotDngCmd(); break;
            case 'M': ModifyOrder(); break;
            case 'N': NegateTime(0, 0); break;
            case 'O': OtherCommand(0); break;
            case 'P': dPeer(); break;
            case 'Q': NotDngCmd(); break;
            case 'R': ReadyWeapon(0, 0); break;
            case 'S': NotDngCmd(); break;
            case 'T': NotDngCmd(); break;
            case 'U': NotDngCmd(); break;
            case 'V': Volume(); break;
            case 'W': WearArmour(0, 0); break;
            case 'X': NotDngCmd(); break;
            case 'Y': Yell(0); break;
            case 'Z': Stats(0, 0); break;
        }
        goto dungeonmech;
        return;
    }
    if (gKeyPress == '4')
        gKeyPress = 28;
    if (gKeyPress == '6')
        gKeyPress = 29;
    if (gKeyPress == '8')
        gKeyPress = 30;
    if (gKeyPress == '2')
        gKeyPress = 31;
    switch (gKeyPress) {
        case ' ': dPass(); break;
        case 28: Left(); break;
        case 29: Right(); break;
        case 30: Forward(); break;
        case 31: Retreat(); break;
        default:
            if (!gDone)
                What2();
            break;
    }
dungeonmech: /* $8FC2 */
    if (dungeonLevel < 0) {
        gExitDungeon = true;
        goto dungstart;
    }
    AgeChars();
    ShowChars(false);
    wx = 0x18;
    wy = 0x17;
    if (gTorch > 0)
        gTorch--;
    xs = xpos;
    ys = ypos;
    value = GetXYDng(xs, ys);
    if (value != 0)
        goto dngnotcombat;
    if (RandNum(0, 0x82 + dungeonLevel) < 128)
        goto dungstart;
    value = RandNum(0, dungeonLevel + 2);
    if (value > 6)
        value = 6;
    value += 0x18;
    gMonType = value * 2;
    PutXYDng(0x40, xpos, ypos);
    Combat();
    ClearTiles();
    goto dungstart;
    return;
dngnotcombat: /* $9014 */
    switch (value) {
        case 1: /* $9076 time lord */
            ImageDisplay(8, TRUE);
            gSongCurrent = gSongNext = 10;
            UPrintMessage(151);
        timelord:
            WaitKeyMouse();
            ImageGoAway();
            UPrintWin("\p\n");
            DrawDungeon();
            gSongNext = 4;
            break;
        case 2: /* $9174 fountain */
            ImageDisplay(5, TRUE);
            gSongCurrent = gSongNext = 10;
        fountain:
            wx = 0x18;
            wy = 0x17;
            UPrintMessage(152);
            chnum = GetChar();
            if (chnum < 1 || chnum > 4) {
                gSongCurrent = gSongNext = 4;
                ImageGoAway();
                goto dungstart;
            }
            if (CheckAlive(chnum - 1) == FALSE) {
                UPrintMessage(153);
                ErrorTone();
                goto fountain;
            }
            rosNum = Party[6 + chnum];
            switch (xpos & 0x03) {
                case 0:    // Poison fountain
                    Player[rosNum][17] = 'P';
                    UPrintMessage(154);
                    InverseChar(chnum - 1);
                    PlaySoundFile(CFSTR("Hit"), FALSE);    // was 0xF7
                    InverseChar(chnum - 1);
                    ShowChars(false);
                    goto fountain;
                    break;
                case 1:    // Heal fountain
                    Player[rosNum][26] = Player[rosNum][28];
                    Player[rosNum][27] = Player[rosNum][29];
                    UPrintMessage(155);
                    ShowChars(false);
                    goto fountain;
                    break;
                case 2:    // Damage fountain
                    UPrintMessage(156);
                    HPSubtract(rosNum, 25);
                    InverseTiles();
                    InverseChar(chnum - 1);
                    PlaySoundFile(CFSTR("Hit"), FALSE);    // was 0xF7
                    InverseTiles();
                    InverseChar(chnum - 1);
                    ShowChars(false);
                    goto fountain;
                    break;
                case 3:    // Cure poison fountain
                    UPrintMessage(157);
                    if (Player[rosNum][17] == 'P')
                        Player[rosNum][17] = 'G';
                    ShowChars(false);
                    goto fountain;
                    break;
            }
            break;
        case 3: /* $92C1 strange wind */
            UPrintMessage(158);
            gTorch = 0;
            goto dungstart;
            break;
        case 4: /* $9135 trap */
            PutXYDng(0, xs, ys);
            UPrintMessage(159);
            PlaySoundFile(CFSTR("Step"), TRUE);    // was 0xF6, TRUE
            if (StealDisarmFail(Party[7])) {
                UPrintMessage(160);
                goto dungstart;
            }
            BombTrap();
            break;
        case 5: /* $931C brand */
            ImageDisplay(6, TRUE);
            gSongCurrent = gSongNext = 0;
            gSongCurrent = gSongNext = 10;
            wx = 0x18;
            wy = 0x17;
            UPrintMessage(161);
            chnum = GetChar();
            if (chnum < 1 || chnum > 4) {
            mark:
                ImageGoAway();
                ShowChars(false);
                gSongCurrent = gSongNext = 4;
                goto dungstart;
            }
            rosNum = Party[6 + chnum];
            Player[rosNum][14] = Player[rosNum][14] | bits[(xpos & 3) + 4];
            InverseChar(chnum - 1);
            PlaySoundFile(CFSTR("Hit"), FALSE);    // was 0xF7
            InverseChar(chnum - 1);
            HPSubtract(rosNum, 50);
            UPrintMessage(162);
            goto mark;
            break;
        case 6: /* $92DA gremlins */
            PutXYDng(0, xs, ys);
            chnum = RandNum(0, Party[2] - 1);
            if (CheckAlive(chnum) == FALSE)
                goto dungstart;
            rosNum = Party[7 + chnum];
            if (Player[rosNum][32] < 1) {
                Player[rosNum][32] = 0;
            } else {
                Player[rosNum][32]--;
            }
            PlaySoundFile(CFSTR("Ouch"), FALSE);    // was 0xFA
            UPrintMessage(163);
            ShowChars(false);
            break;
        case 8: /* $9053 writing */
            UPrintMessage(164);
            Speak(dungeonLevel + 1, 23);
            UPrintWin("\p\n");
            break;
    }
    if (gExitDungeon == 0)
        goto dungstart;
}

void dPass(void) { /* $8DE6 */
    UPrintMessage(23);
}

void Forward(void) { /* $8DF2 */
    UPrintMessage(165);
    xs = HeadX[heading] + xpos & 0x0F;
    if (xs < 0)
        xs += 16;
    ys = HeadY[heading] + ypos & 0x0F;
    if (ys < 0)
        ys += 16;
    if (GetXYDng(xs, ys) == 0x80) {
        NoGo();
        return;
    }
    xpos = xs;
    ypos = ys;
    DrawDungeon();
}

void Retreat(void) { /* $8E2C */
    UPrintMessage(166);
    xs = HeadX[(heading + 2) & 3] + xpos & 0x0F;
    if (xs < 0)
        xs += 16;
    ys = HeadY[(heading + 2) & 3] + ypos & 0x0F;
    if (ys < 0)
        ys += 16;
    if (GetXYDng(xs, ys) == 0x80) {
        NoGo();
        return;
    }
    xpos = xs;
    ypos = ys;
    DrawDungeon();
}

void Right(void) { /* $8E67 */
    UPrintMessage(167);
    xs = xpos;
    ys = ypos;
    if (GetXYDng(xs, ys) >= 0xA0) {
        NoGo();
        return;
    }
    heading++;
    if (heading > 3)
        heading -= 4;
    DrawDungeon();
}

void Left(void) { /* $8E93 */
    UPrintMessage(168);
    xs = xpos;
    ys = ypos;
    if (GetXYDng(xs, ys) >= 0xA0) {
        NoGo();
        return;
    }
    heading--;
    if (heading < 0)
        heading += 4;
    DrawDungeon();
}

void dDescend(void) { /* $8F0C */
    UPrintMessage(169);
    if (GetXYDng(xpos, ypos) > 127) {
        InvalCmd();
        return;
    }
    if ((GetXYDng(xpos, ypos) & 0x20) == 0) {
        InvalCmd();
        return;
    }
    dungeonLevel++;
    DrawDungeon();
}

void dKlimb(void) { /* $8F37 */
    UPrintMessage(170);
    if ((GetXYDng(xpos, ypos) & 0x10) == 0) {
        InvalCmd();
        return;
    }
    dungeonLevel--;
    if (dungeonLevel >= 0 && dungeonLevel < 8) {
        DrawDungeon();
        return;
    }
    dungeonLevel = 0;
    gExitDungeon = 1;
    return;
}

void dPeer(void) { /* $8F60 */
    short chnum, rosNum;
    UPrintMessage(75);
    chnum = GetChar();
    if (chnum < 1 || chnum > 4)
        return;
    rosNum = Party[6 + chnum];
    if (Player[rosNum][37] < 1) {
        UPrintMessage(67);
        return;
    }
    Player[rosNum][37]--;
    DrawMiniDng(0);
    ClearTiles();
}

void DngInfo(void) { /* $7CC6 */
    Str32 str;

    DrawFramePiece(12, 8, 0);
    DrawFramePiece(13, 15, 0);
    DrawFramePiece(12, 6, 23);
    DrawFramePiece(13, 17, 23);
    DrawFramePiece(10, 5, 23);
    DrawFramePiece(10, 7, 0);
    DrawFramePiece(10, 16, 0);
    //  Message(48,9,0);
    //  UPrintNumPad(dungeonLevel+1,1);
    GetPascalStringFromArrayByIndex(str, CFSTR("MoreMessages"), 47);    //GetIndString(str, BASERES+14, 48);
    str[++str[0]] = '1' + dungeonLevel;
    UCenterAt(str, 9, 0);
    //  Message(49,7,23);
    GetPascalStringFromArrayByIndex(str, CFSTR("MoreMessages"), 48);    //GetIndString(str, BASERES+14, 49);
    UCenterAt(str, 7, 23);

    if (heading == 0)
        GetPascalStringFromArrayByIndex(str, CFSTR("MoreMessages"), 49);    //GetIndString(str, BASERES+14, 50);
    if (heading == 1)
        GetPascalStringFromArrayByIndex(str, CFSTR("MoreMessages"), 50);    //GetIndString(str, BASERES+14, 51);
    if (heading == 2)
        GetPascalStringFromArrayByIndex(str, CFSTR("MoreMessages"), 51);    //GetIndString(str, BASERES+14, 52);
    if (heading == 3)
        GetPascalStringFromArrayByIndex(str, CFSTR("MoreMessages"), 52);    //GetIndString(str, BASERES+14, 53);
    UCenterAt(str, 12, 23);
    return;
}

void InvalCmd(void) { /* $8ED8 */
    UPrintMessage(171);
    ErrorTone();
}

void NotDngCmd(void) { /* $8EF1 */
    UPrintMessage(172);
    ErrorTone();
}

void DrawDungeon(void) { /* $1800 */
    short value;
    Rect FromRect, ToRect;
    if (gTorch < 1) {
        ClearTiles();
        return;
    }
    DrawDungeonBackGround();
    value = DungeonBlock(0);
    if (value < 0 || value > 127)
        goto dungeondrawdone;
    if (value != 0)
        goto chunk3;
    if (DungeonBlock(1) > 127)
        goto chunk2;
    if (DungeonBlock(3) > 127)
        goto chunk2;
    if (DungeonBlock(6) > 127)
        goto chunk2;
    if (DungeonBlock(10) > 127)
        goto chunk2;
    if (DungeonBlock(15) > 127)
        goto chunk2;
    DungeonBlock(21);
chunk2: /* $183A */
    if (DungeonBlock(2) > 127)
        goto chunk3;
    if (DungeonBlock(5) > 127)
        goto chunk3;
    if (DungeonBlock(9) > 127)
        goto chunk3;
    if (DungeonBlock(14) > 127)
        goto chunk3;
    if (DungeonBlock(20) > 127)
        goto chunk3;
    DungeonBlock(27);
chunk3: /* $1862 */
    if (DungeonBlock(4) != 0)
        goto dungeondrawdone;
    if (DungeonBlock(7) > 127)
        goto chunk4;
    if (DungeonBlock(11) > 127)
        goto chunk4;
    if (DungeonBlock(16) > 127)
        goto chunk4;
    DungeonBlock(22);
chunk4: /* $1884 */
    if (DungeonBlock(8) > 127)
        goto chunk5;
    if (DungeonBlock(13) > 127)
        goto chunk5;
    if (DungeonBlock(19) > 127)
        goto chunk5;
    DungeonBlock(26);
chunk5: /* $189E */
    if (DungeonBlock(12) != 0)
        goto dungeondrawdone;
    if (DungeonBlock(17) > 127)
        goto chunk6;
    if (DungeonBlock(23) > 127)
        goto chunk6;
    DungeonBlock(28);
chunk6: /* $18B9 */
    if (DungeonBlock(18) > 127)
        goto chunk7;
    if (DungeonBlock(25) > 127)
        goto chunk7;
chunk7: /* $18CC */
    if (DungeonBlock(24) > 127)
        goto dungeondrawdone;
    DungeonBlock(29);
    DungeonBlock(30);
dungeondrawdone:
    DrawSecretMessage();
    if (gTorch < 3)
        DimDungeon();
    SetRect(&FromRect, 0, 0, 600, 512);
    /*if (gUnusualSize)
        {
        ToRect.left = blkSiz; ToRect.top = blkSiz;
        ToRect.right = blkSiz*23; ToRect.bottom = blkSiz*23;
        CopyBits(LWPortCopyBits(dungPort),
                 LWPortCopyBits(mainPort),
                 &FromRect, &ToRect, ditherCopy, nil);
        }
    else*/
    {
        float mult = (float)blkSiz / 32.0;    // offscreen is x2 normal size
        ToRect = FromRect;
        ToRect.right *= mult;
        ToRect.bottom *= mult;
        OffsetRect(&ToRect, 84 * mult, 128 * mult);
        /*ToRect.left=FromRect.left+42;
        ToRect.top=FromRect.top+64;
        ToRect.right=FromRect.right+42;
        ToRect.bottom=FromRect.bottom+64;*/
        CopyBits(LWPortCopyBits(dungPort), LWPortCopyBits(mainPort), &FromRect, &ToRect, srcCopy, nil);
    }
    return;
}

void DrawSecretMessage(void) {
    if (dungeonLevel == 6 && xpos == 1 && ypos == 1 && heading == 3)
        ShowSecret((gCurMapID - 412) * 3 + 1);
    if (dungeonLevel == 6 && xpos == 1 && ypos == 15 && heading == 2)
        ShowSecret((gCurMapID - 412) * 3 + 2);
    if (dungeonLevel == 7 && xpos == 15 && ypos == 15 && heading == 1)
        ShowSecret((gCurMapID - 412) * 3 + 3);
}

void DimDungeon(void) {
    Rect myRect;
    RGBColor blendCol;

    SetRect(&myRect, 0, 0, 600, 512);
    SetGWorld(dungPort, nil);
    blendCol.red = blendCol.green = blendCol.blue = 28000;
    OpColor(&blendCol);
    PenMode(blend);
    ForeColor(blackColor);
    PaintRect(&myRect);
    PenMode(srcCopy);
    SetGWorld(mainPort, nil); /*was mainDevice*/
}

Boolean ShowSecret(short which) {
    Rect FromRect;
    Str255 message;
    RGBColor blendCol;
    short textX, textY;    //, byte, low, high;

    GetPascalStringFromArrayByIndex(message, CFSTR("Messages"), which - 1);

    //GetIndString(message,BASERES+12,which);
    if (message[0] == 0)
        return FALSE;
    /*
    byte=1;
    while (byte<=message[0])
        {
        low = message[byte] & 0x0F;
        high = (message[byte]/16) & 0x0F;
        message[byte]=(low*16)+high;
        byte++;
        }
    */
    SetRect(&FromRect, 0, 0, 600, 512);
    CopyBits(LWPortCopyBits(dungPort), LWPortCopyBits(gamePort), &FromRect, &FromRect, srcCopy, nil);
    SetGWorld(gamePort, nil);
    TextFont(3);
    TextFace(1);
    TextSize(24);
    TextMode(srcOr);
    textX = 300 - (StringWidth(message) / 2);
    textY = 282;
    ForeColor(whiteColor);
    MoveTo(textX + 1, textY);
    DrawString(message);
    MoveTo(textX + 1, textY + 1);
    DrawString(message);
    MoveTo(textX, textY + 1);
    DrawString(message);
    ForeColor(blackColor);
    MoveTo(textX, textY - 1);
    DrawString(message);
    MoveTo(textX - 1, textY - 1);
    DrawString(message);
    MoveTo(textX - 1, textY);
    DrawString(message);
    blendCol.red = blendCol.green = blendCol.blue = 32768;
    RGBForeColor(&blendCol);
    MoveTo(textX, textY);
    DrawString(message);
    SetGWorld(mainPort, nil); /*was mainDevice*/
    ForeColor(blackColor);
    BackColor(whiteColor);
    blendCol.red = blendCol.green = blendCol.blue = 28000;
    OpColor(&blendCol);
    CopyBits(LWPortCopyBits(gamePort), LWPortCopyBits(dungPort), &FromRect, &FromRect, blend, nil);
    return TRUE;
}

short DungeonBlock(short location) { /* $18DE */
    short dx, dy, misc, swap, tile;
    char offsetX[32] = {0,  -1, 1, -1, 0, 1,  -2, -1, 1, 2, -2, -1, 0,  1,  2, -3,
                        -2, -1, 1, 2,  3, -3, -2, -1, 0, 1, 2,  3,  -2, -1, 1, 2};
    char offsetY[32] = {0,  0,  0,  -1, -1, -1, -1, -1, -1, -1, -2, -2, -2, -2, -2, -2,
                        -2, -2, -2, -2, -2, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3};
    dx = offsetX[location];
    dy = offsetY[location];
    misc = heading;
    while (misc > 0) {
        misc--;
        swap = dx;
        dx = (-dy);
        dy = swap;
    }
    xs = (xpos + dx) & 0x0F;
    ys = (ypos + dy) & 0x0F;
    if (xs < 0)
        xs += 16;
    if (ys < 0)
        ys += 16;
    tile = GetXYDng(xs, ys);
    if (tile < 128) {
        if ((tile & 0x10) != 0)
            DrawLadder(location, 0);
        if ((tile & 0x20) != 0)
            DrawLadder(location, 1);
        if ((tile & 0x40) != 0)
            DrawChest(location);
        return 0;
    }
    if (tile < 0xC0) {
        DrawWall(location);
        return 0xFF;
    }
    misc = DrawWall(location);
    misc = DrawWall(misc + 32);
    if (misc == 0x20)
        return 1;
    return 0xFF;
}

short DrawWall(short location) { /* $19B4 */
    Rect FromRect, ToRect;
    if (location > 31) {
        DrawDoor(location - 32);
        return location;
    }
    ToRect.left = (dngL[location] * 2);
    ToRect.top = (dngT[location] * 2);
    ToRect.right = (dngR[location] * 2);
    ToRect.bottom = (dngB[location] * 2);
    FromRect.left = ToRect.left + (dOfX[location] * 2);
    FromRect.top = ToRect.top + (dOfY[location] * 2);
    FromRect.right = ToRect.right + (dOfX[location] * 2);
    FromRect.bottom = ToRect.bottom + (dOfY[location] * 2);
    ForeColor(blackColor);
    BackColor(whiteColor);
    if (dMask[location]) {
        CopyMask(LWPortCopyBits(nDngShapes), LWPortCopyBits(nDngMasks), LWPortCopyBits(dungPort), &FromRect, &FromRect, &ToRect);
    } else {
        CopyBits(LWPortCopyBits(nDngShapes), LWPortCopyBits(dungPort), &FromRect, &ToRect, srcCopy, nil);
    }
    return location;
}

void DrawDoor(short location) {
    PolyHandle poly;
    // 0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31 */
    short dx1[32] = {74, 0,   300, 0,   114, 300, 20, 88,  212, 280, 36,  79,  135, 221, 264, 24,
                     87, 118, 182, 213, 276, 40,  87, 114, 143, 186, 213, 260, 118, 136, 164, 182};
    short dx2[32] = {226, 40,  260, 37,  186, 263, 48,  102, 198, 252, 64,  108, 164, 192, 236, 48,
                     101, 124, 176, 199, 252, 54,  100, 127, 156, 173, 200, 246, 126, 140, 160, 174};

    short dy1[32] = {256, 256, 256, 191, 191, 191, 182, 179, 179, 182, 159, 159, 159, 159, 159, 154,
                     153, 153, 153, 153, 154, 143, 143, 143, 143, 143, 143, 143, 140, 138, 138, 140};
    short dy2[32] = {64,  75,  75,  95,  95,  95,  96,  100, 100, 96,  112, 112, 112, 112, 112, 112,
                     110, 110, 110, 110, 112, 120, 120, 120, 120, 120, 120, 120, 122, 124, 124, 122};
    short dy3[32] = {64,  92,  92,  95,  95,  95,  102, 106, 106, 102, 112, 112, 112, 112, 112, 116,
                     114, 115, 115, 114, 116, 120, 120, 120, 120, 120, 120, 120, 124, 126, 126, 124};
    short dy4[32] = {256, 220, 220, 191, 191, 191, 170, 168, 168, 170, 159, 159, 159, 159, 159, 149,
                     147, 148, 148, 147, 149, 143, 143, 143, 143, 143, 143, 143, 137, 134, 134, 137};

    SetGWorld(dungPort, nil);
    poly = OpenPoly();
    MoveTo((dx1[location] * 2), (dy1[location] * 2));
    LineTo((dx1[location] * 2), (dy2[location] * 2));
    LineTo((dx2[location] * 2), (dy3[location] * 2));
    LineTo((dx2[location] * 2), (dy4[location] * 2));
    LineTo((dx1[location] * 2), (dy1[location] * 2));
    ClosePoly();
    ForeColor(blackColor);
    PaintPoly(poly);
    KillPoly(poly);
    ForeColor(blackColor);
    SetGWorld(mainPort, nil);
}

void DrawChest(short location) { /* $1A5A */
    Rect FromRect, ToRect;
    short shape, width;
    if (location > 20)
        return;
    shape = 33;
    SetRect(&FromRect, (dngL[shape] * 2), (dngT[shape] * 2), (dngR[shape] * 2), (dngB[shape] * 2));
    width = (FromRect.right - FromRect.left) / 2;
    if (chSide[location] == 1)
        FromRect.right -= width;
    if (chSide[location] == 2)
        FromRect.left += (width - 1);
    SetRect(&ToRect, (chL[location] * 2), (chT[location] * 2), (chR[location] + 1) * 2, (chB[location] + 1) * 2);
    CopyBits(LWPortCopyBits(nDngShapes), LWPortCopyBits(dungPort), &FromRect, &ToRect, srcCopy, nil);
}

void DrawLadder(short location, short direction) { /* $1AAB */
/* direction=$1F */
    Rect FromRect, ToRect;
    short lleft, ltop, lright, lbottom, swap, side;
    short width, height, base, rung, half, shape;

    if (location > 20)
        return;
    lleft = (chL[location] * 2);
    ltop = (chT[location] * 2);
    lright = (chR[location] * 2);
    lbottom = (chB[location] * 2);
    ltop = lbottom - ((lbottom - ltop) * 1.5);
    SetGWorld(dungPort, nil);
    base = lbottom;
    if (direction == 0) {
        swap = 512 - lbottom;
        lbottom = 512 - ltop;
        ltop = swap;
        base = ltop;
    }
    side = chSide[location];
    ForeColor(blackColor);
    height = (((lbottom - ltop) * 1.25) - (lbottom - ltop)) / 2;
    width = (lbottom - ltop) / 12;
    rung = (((lbottom - ltop) / 2) + ltop) - (width / 2);
    half = lleft + ((lright - lleft) / 2);
    if (side == 1)
        half = lright;
    if (side == 2)
        half = lleft;
    SetRect(&FromRect, lleft - width - 2, base - height, lright + width + 2, base + height);
    PaintRect(&FromRect);
    SetGWorld(mainPort, nil); /*was mainDevice*/
    if (side == 0 || side == 1) {
        shape = 35;
        SetRect(&FromRect, (dngL[shape] * 2), (dngT[shape] * 2), (dngR[shape] * 2), (dngB[shape] * 2));
        SetRect(&ToRect, lleft, ltop, lleft + width, lbottom);
        CopyBits(LWPortCopyBits(nDngShapes), LWPortCopyBits(dungPort), &FromRect, &ToRect, srcCopy, nil);
        shape = 34;
        SetRect(&FromRect, (dngL[shape] * 2), (dngT[shape] * 2), (dngR[shape] * 2), (dngB[shape] * 2));
        SetRect(&ToRect, lleft, rung, half + 1, rung + width);
        CopyBits(LWPortCopyBits(nDngShapes), LWPortCopyBits(dungPort), &FromRect, &ToRect, srcCopy, nil);
    }
    if (side == 0 || side == 2) {
        shape = 35;
        SetRect(&FromRect, (dngL[shape] * 2), (dngT[shape] * 2), (dngR[shape] * 2), (dngB[shape] * 2));
        SetRect(&ToRect, lright - width, ltop, lright, lbottom);
        CopyBits(LWPortCopyBits(nDngShapes), LWPortCopyBits(dungPort), &FromRect, &ToRect, srcCopy, nil);
        shape = 34;
        SetRect(&FromRect, (dngL[shape] * 2), (dngT[shape] * 2), (dngR[shape] * 2), (dngB[shape] * 2));
        SetRect(&ToRect, half, rung, lright, rung + width);
        CopyBits(LWPortCopyBits(nDngShapes), LWPortCopyBits(dungPort), &FromRect, &ToRect, srcCopy, nil);
    }
}

void DrawDungeonBackGround(void) {
    Rect FromRect, ToRect;

    SetRect(&FromRect, 1800, 0, 2400, 512);
    SetRect(&ToRect, 0, 0, 600, 512);
    CopyBits(LWPortCopyBits(nDngShapes), LWPortCopyBits(dungPort), &FromRect, &ToRect, srcCopy, nil);
}

void GetDungeonGraphics(void) {
    Rect theRect;
    SetRect(&theRect, 0, 0, 3000, 512);
    if (noErr == NewGWorld(&nDngShapes, 32, &theRect, nil, nil, 0)) {
        if (!DrawNamedImage(CFSTR("DungeonShapes.jpg"), nDngShapes, &theRect))
            printf("Dungeon shapes could not load.\n");
    }

    theRect.right = 1200;
    if (noErr == NewGWorld(&nDngMasks, 1, &theRect, nil, nil, 0)) {
        if (!DrawNamedImage(CFSTR("DungeonMasks.png"), nDngMasks, &theRect))
            printf("Dungeon masks could not load.\n");
    }

    SetRect(&theRect, 0, 0, 600, 512);
    gError = NewGWorld(&dungPort, 32, &theRect, nil, nil, 0);
    if (gError)
        gError = NewGWorld(&dungPort, 32, &theRect, nil, nil, 0);

    /*
    PicHandle       picture;
    Rect            FromRect;
    
    picture = GetPicture(10000);
    if (!picture)
        { HandleError(ResError(), 3, 10000); return; }
    
    SetRect (&FromRect, 0, 0, 1500, 256);
    gError = NewGWorld(&nDngShapes, 0, &FromRect, nil, nil, 0);
    if (gError) gError = NewGWorld(&nDngShapes, 8, &FromRect, nil, nil, 0);
    if (gError) { HandleError(gError, 2, 0); return; }
    nDngShapesPixMap = GetGWorldPixMap(nDngShapes);
    //( *( ( *nDngShapesPixMap )->pmTable ) )->ctSeed = ( *( ( *( ( *( GetGDevice() ) )->gdPMap ) )->pmTable ) )->ctSeed;
    LockPixels(nDngShapesPixMap);
    SetGWorld(nDngShapes, nil);
    DrawPicture(picture, &FromRect);
    ReleaseResource((Handle)picture);
    SetRect(&FromRect, 0, 0, 600, 256);
    gError = NewGWorld(&nDngMasks, 1, &FromRect, nil, nil, 0);
    if (gError!=0)
        {
        HandleError(gError, 4, 0);
        return;
        }
    nDngMasksPixMap = GetGWorldPixMap(nDngMasks);
    //( *( ( *nDngMasksPixMap )->pmTable ) )->ctSeed = ( *( ( *( ( *( GetGDevice() ) )->gdPMap ) )->pmTable ) )->ctSeed;
    LockPixels(nDngMasksPixMap);
    SetGWorld(nDngMasks, nil);
    picture = GetPicture(10001);
    if (picture==0)
        {
        HandleError(ResError(), 5, 10001);
        return;
        }
    DrawPicture(picture, &FromRect);
    ReleaseResource((Handle)picture);
    SetGWorld(mainPort,nil);
    SetRect(&FromRect, 0, 0, 300, 256);
    gError = NewGWorld(&dungPort, 0, &FromRect, nil, nil, 0);
    if (gError) gError = NewGWorld(&dungPort, 8, &FromRect, nil, nil, 0);
    if (gError!=0) { HandleError(gError, 4, 1); return; }
    dungPixMap = GetGWorldPixMap(dungPort);
    //( *( ( *dungPixMap )->pmTable ) )->ctSeed = ( *( ( *( ( *( GetGDevice() ) )->gdPMap ) )->pmTable ) )->ctSeed;
    LockPixels(dungPixMap);
*/
}

void DisposeDungeonGraphics(void) {
    UnlockPixels(nDngShapesPixMap);
    DisposeGWorld(nDngShapes);
    nDngShapes = nil;
    UnlockPixels(nDngMasksPixMap);
    DisposeGWorld(nDngMasks);
    nDngMasks = nil;
    UnlockPixels(dungPixMap);
    DisposeGWorld(dungPort);
    dungPort = nil;
}
