// A lot of miscellaneous Ultima routines

#include <string.h>

#import "UltimaMisc.h"

#import "UltimaIncludes.h"
#import "CarbonShunts.h"
#import "CocoaBridge.h"
#import "U3Audio.h"
#import "U3IO.h"
#import "U3Renderer.h"
#import "U3Platform.h"
#import "UltimaDngn.h"
#import "UltimaGraphics.h"
#import "UltimaMacIF.h"
#import "UltimaMain.h"
#import "UltimaSpellCombat.h"
#import "UltimaText.h"

extern Boolean          gDone, gResurrect;
extern unsigned char    Monsters[256], Talk[256], Player[21][65];
extern char             gKeyPress;
extern unsigned char    TileArray[128], Dungeon[2048], gBallTileBackground;
extern unsigned char    MoonXTable[8], MoonYTable[8], careerTable[12], wpnUseTable[12];
extern unsigned char    armUseTable[12], LocationX[20], LocationY[20], Experience[17];
extern U3DataBuffer     gDemoData;
extern char             WhirlDX, WhirlDY;
extern char             YellStat;
extern long             gTime[2], gMapOffset;
extern int              xpos, ypos, xs, ys, tx, ty, wx, wy, dx, dy;
extern short            WhirlX, WhirlY, gUpdateWhere, gMoon[2], gMoonDisp[2];
extern short            gTimeNegate, zp[255], lastCard;
extern short            gMouseState, gCurMapID;
extern short            blkSiz, gCurMapSize;
extern short            gSongCurrent, gSongNext, gSongPlaying;
extern Str255           gString;
extern CGrafPtr         framePort, mainPort;

Handle                  Map, pushMap;
const char              MonTypes[14] = {24,23,25,20,26,27,13,28,22,14,15,29,30,24};
const char              MonBegin[13] = {4,4,4,4,4,4,0,4,4,0,0,4,4};

// ----------------------------------------------------------------------
// Local prototypes

short Clerical(short rosNum, short cost);
void EatFood(short member, short amount);
void GetMonsterDir(short monNum);
void MoonGateUpdate(void);
void HandleMoonStep(void);
short ShrineRace(short race);
void SpellNoize(short opnum);
void WeaponList(short lastitem);
void ArmourList(short lastitem);
Boolean GuildPay(short rosNum, short cost);
void GuildGive(short rosNum, short item, short amount);

// ----------------------------------------------------------------------

void GetDirection(short mode) {   // $7D73? mode=1 means accept space (combat)
    short dirgot, direct, oldMouseState;
    dirgot = 0;
    oldMouseState = gMouseState;
    gMouseState = 3;
    U3CocoaTextCheckpoint("direction");
    while (dirgot == 0) {
        if (gDone) break;
        direct = U3PlatformWaitKeyMouse();
        if (direct == '4')
            direct = 28;
        if (direct == '6')
            direct = 29;
        if (direct == '8')
            direct = 30;
        if (direct == '2')
            direct = 31;
        Boolean allowDiagonal = (!U3PlatformGetBooleanPreference(U3PreferenceNoDiagonals));
        switch (direct) {
            case '1':
                if (allowDiagonal) {
                    dirgot = 1;
                    xs = xpos - 1;
                    ys = ypos + 1;
                    dx = -1;
                    dy = 1;
                    U3RenderPrintMessage(250);
                }
                break;
            case '3':
                if (allowDiagonal) {
                    dirgot = 1;
                    xs = xpos + 1;
                    ys = ypos + 1;
                    dx = 1;
                    dy = 1;
                    U3RenderPrintMessage(251);
                }
                break;
            case '7':
                if (allowDiagonal) {
                    dirgot = 1;
                    xs = xpos - 1;
                    ys = ypos - 1;
                    dx = -1;
                    dy = -1;
                    U3RenderPrintMessage(252);
                }
                break;
            case '9':
                if (allowDiagonal) {
                    dirgot = 1;
                    xs = xpos + 1;
                    ys = ypos - 1;
                    dx = 1;
                    dy = -1;
                    U3RenderPrintMessage(253);
                }
                break;
            case 28:
                dirgot = 1;
                xs = xpos - 1;
                ys = ypos;
                dx = -1;
                dy = 0;
                U3RenderPrintMessage(27);
                break;
            case 29:
                dirgot = 1;
                xs = xpos + 1;
                ys = ypos;
                dx = 1;
                dy = 0;
                U3RenderPrintMessage(26);
                break;
            case 30:
                dirgot = 1;
                xs = xpos;
                ys = ypos - 1;
                dx = 0;
                dy = -1;
                U3RenderPrintMessage(24);
                break;
            case 31:
                dirgot = 1;
                xs = xpos;
                ys = ypos + 1;
                dx = 0;
                dy = 1;
                U3RenderPrintMessage(25);
                break;
            case ' ':
                if (mode == 1) {
                    dirgot = 1;
                    xs = xpos;
                    ys = ypos;
                    dx = 0;
                    dy = 0;
                    U3RenderPrintMessage(173);
                    break;
                }
            default: break;
        }
    }
    gMouseState = oldMouseState;
}

static void ReadByteTable(short resourceID, unsigned char *table, size_t capacity) {
    U3DataBuffer buffer;
    if (!U3IOLoadResource(U3ResourceKindMisc, resourceID, &buffer)) return;
    size_t count = buffer.size < capacity ? buffer.size : capacity;
    memset(table, 0, capacity);
    memcpy(table, buffer.bytes, count);
    U3IOReleaseResource(&buffer);
}

static void WriteByteTable(short resourceID, const unsigned char *table, size_t capacity) {
    U3MutableDataBuffer buffer;
    if (!U3IOOpenMutableResource(U3ResourceKindMisc, resourceID, &buffer)) return;
    size_t count = buffer.size < capacity ? buffer.size : capacity;
    memcpy(buffer.bytes, table, count);
    U3IOCloseMutableResource(&buffer, true);
}

void GetMiscStuff(short id) {
    U3DataBuffer tempBuffer;
    unsigned short byte;

    if (U3IOLoadResource(U3ResourceKindMisc, BASERES + id, &tempBuffer)) {
        for (byte = 0; byte < 8; byte++) {
            MoonXTable[byte] = tempBuffer.bytes[byte];
            MoonYTable[byte] = tempBuffer.bytes[byte + 8];
        }
        U3IOReleaseResource(&tempBuffer);
    }

    ReadByteTable(BASERES + id + 1, careerTable, sizeof(careerTable));
    ReadByteTable(BASERES + id + 2, wpnUseTable, sizeof(wpnUseTable));
    ReadByteTable(BASERES + id + 3, armUseTable, sizeof(armUseTable));

    if (U3IOLoadResource(U3ResourceKindMisc, BASERES + id + 4, &tempBuffer)) {
        for (byte = 0; byte < 20; byte++) {
            LocationX[byte] = tempBuffer.bytes[byte];
            LocationY[byte] = tempBuffer.bytes[byte + 32];
        }
        U3IOReleaseResource(&tempBuffer);
    }

    ReadByteTable(BASERES + id + 5, Experience, sizeof(Experience));
}

void PutMiscStuff(void) {
    U3MutableDataBuffer tempBuffer;
    unsigned short byte;

    if (U3IOOpenMutableResource(U3ResourceKindMisc, BASERES + 100, &tempBuffer)) {
        for (byte = 0; byte < 8; byte++) {
            tempBuffer.bytes[byte] = MoonXTable[byte];
            tempBuffer.bytes[byte + 8] = MoonYTable[byte];
        }
        U3IOCloseMutableResource(&tempBuffer, true);
    }

    WriteByteTable(BASERES + 101, careerTable, sizeof(careerTable));
    WriteByteTable(BASERES + 102, wpnUseTable, sizeof(wpnUseTable));
    WriteByteTable(BASERES + 103, armUseTable, sizeof(armUseTable));

    if (U3IOOpenMutableResource(U3ResourceKindMisc, BASERES + 104, &tempBuffer)) {
        for (byte = 0; byte < 20; byte++) {
            tempBuffer.bytes[byte] = LocationX[byte];
            tempBuffer.bytes[byte + 32] = LocationY[byte];
        }
        U3IOCloseMutableResource(&tempBuffer, true);
    }

    WriteByteTable(BASERES + 105, Experience, sizeof(Experience));
}

unsigned char ValidMonsterDir(short tile, short montype) { /* $7C0C */
    if (montype > 0x28 && montype < 0x40) {   // pirate/sea monster
        return (tile == 0) ? 0 : 255;
    } else {
        if (tile == 4 || tile == 8 || tile == 12 || tile == 32)
            return 0;
        else
            return 255;
    }
}

void GetMonsterDir(short monNum) { /* $7C37 */
    if (Party[3] != 0) {
        zp[0xF5] = xpos - Monsters[monNum + XMON];
        dx = GetHeading(zp[0xF5]);
        xs = MapConstrain(dx + Monsters[monNum + XMON]);
        zp[0xF6] = ypos - Monsters[monNum + YMON];
        dy = GetHeading(zp[0xF6]);
        ys = MapConstrain(dy + Monsters[monNum + YMON]);
    } else {
        zp[0xF5] = xpos - Monsters[monNum + XMON];
        dx = GetHeading(zp[0xF5] * 4);
        xs = MapConstrain(dx + Monsters[monNum + XMON]);
        zp[0xF6] = ypos - Monsters[monNum + YMON];
        dy = GetHeading(zp[0xF6] * 4);
        ys = MapConstrain(dy + Monsters[monNum + YMON]);
    }
    zp[0xFB] = Absolute(zp[0xF5]);
    zp[0xFB] += Absolute(zp[0xF6]);
}

Rect DebugMapRect(int x, int y) {
    int scale = (blkSiz / 12);
    if (scale < 1)
        scale = 1;
    int px = ((blkSiz * 22.5) - (gCurMapSize * scale)) + (x * scale);
    int py = y * scale + (blkSiz * 1.5);
    Rect r;
    r.left = px;
    r.top = py;
    r.right = r.left + scale;
    r.bottom = r.top + scale;
    return r;
}

void ShowMonsterList(void) {
    short monNum, x, y, value, value2;
    Rect ToRect;
    Str255 tempStr;
    RGBColor color;

    SetRect(&ToRect, blkSiz, blkSiz, blkSiz * 23, blkSiz * 23);
    ForeColor(whiteColor);
    PaintRect(&ToRect);
    TextMode(srcOr);
    TextFont(3);
    TextSize(blkSiz * 0.5625);
    PenSize(1, 1);
    value2 = 256;
    /*  for (x=0; x<32; x++) // Force locations where location entry is fucked up
        {
        value = GetXYVal(LocationX[x],LocationY[x]);
        value2=0x14; // dungeon
        if (x<12) value2=0x18; // or towne
        if (x<2) value2=0x1C; // or castle
        if (value<0x14) PutXYVal(value2,LocationX[x],LocationY[x]);
        } */
    for (y = 0; y < gCurMapSize; y++) {
        for (x = 0; x < gCurMapSize; x++) {
            value = GetXYVal(x, y) / 4;
            if (value2 != value) {
                color.red = color.blue = color.green = 0;
                if (value == 0) {
                    color.blue = 65535;
                    color.green = 16384;
                }
                if (value == 1) {
                    color.green = 32768;
                }
                if (value == 2) {
                    color.green = 40960;
                }
                if (value == 3) {
                    color.green = 49152;
                }
                if (value == 8) {
                    color.red = color.green = color.blue = 32768;
                }
                if (value == 33) {
                    color.red = 65535;
                    color.green = 32768;
                }
                if (value == 35) {
                    color.red = color.green = color.blue = 65535;
                }
                if (value > 36) {
                    color.red = color.green = 24576;
                }
                RGBForeColor(&color);
            }
            value2 = value;
            Rect r = DebugMapRect(x, y);
            PaintRect(&r);
        }
    }
    for (monNum = 0; monNum < 32; monNum++) {
        y = monNum * (blkSiz * .65) + (blkSiz * 2);
        MoveTo((blkSiz * 1.5), y);
        ForeColor(blackColor);
        NumToString(monNum, tempStr);
        DrawString(tempStr);
        if (Monsters[monNum] != 0) {
            MoveTo((blkSiz * 2.5), y);
            GetPascalStringFromArrayByIndex(tempStr, CFSTR("Tiles"), Monsters[monNum] / 4);
            color.red = ((Monsters[monNum] / 4) % 7) * 10000;
            color.green = ((Monsters[monNum] / 4) % 5) * 16384;
            color.blue = ((Monsters[monNum] / 4) % 2) * 65535;
            RGBForeColor(&color);
            DrawString(tempStr);
            DrawString("\p (");
            NumToString(Monsters[monNum] / 4, tempStr);
            DrawString(tempStr);
            DrawString("\p)");
            ForeColor(cyanColor);
            MoveTo((blkSiz * 8.5), y);
            value = ((Monsters[monNum] / 4) & 0x0F);
            NumToString(Experience[value], tempStr);
            DrawString(tempStr);
            ForeColor(redColor);
            MoveTo((blkSiz * 9.75), y);
            NumToString(Monsters[monNum + XMON], tempStr);
            DrawString(tempStr);
            DrawString("\p, ");
            NumToString(Monsters[monNum + YMON], tempStr);
            DrawString(tempStr);
            color.red = 65535;
            color.green = 65535;
            color.blue = 0;
            RGBForeColor(&color);
            Rect r = DebugMapRect(Monsters[monNum + XMON], Monsters[monNum + YMON]);
            PaintRect(&r);
            ForeColor(greenColor);
            MoveTo((blkSiz * 12), y);
            GetPascalStringFromArrayByIndex(tempStr, CFSTR("Tiles"), Monsters[monNum + TILEON] / 4);
            DrawString(tempStr);
            ForeColor(magentaColor);
            MoveTo((blkSiz * 14.5), y);
            value = Monsters[monNum + HPMON] & 0xC0;
            value2 = 4;
            if (value == 0x00)
                value2 = 1;
            if (value == 0x40)
                value2 = 2;
            if (value == 0x80)
                value2 = 3;
            GetIndString(tempStr, BASERES + 10, value2);
            DrawString(tempStr);
            ForeColor(blueColor);
            MoveTo((blkSiz * 17), y);
            NumToString(Monsters[monNum + HPMON] & 0x0F, tempStr);
            DrawString(tempStr);
        }
    }
    SetUpFont();
}

short MonsterHere(short x, short y) { /* $7CA4 */
    short count;
    count = 32;
monhere:
    count--;
    if (count < 0) {
        return 255;
    }
    if (Monsters[count] == 0)
        goto monhere;
    if (Monsters[count + XMON] == x && Monsters[count + YMON] == y)
        return count;
    goto monhere;
}

char GetHeading(short value) { /* $7DFC */
    if (value == 0)
        return 0;
    if (value < 0)
        return -1;
    if (value > 127)
        return -1;
    return 1;
}

short Absolute(short value) { /* $7E0D */
    if (value > 127)
        value = (255 - value) + 1;
    if (value < 0)
        value = (-value);
    return value;
}

short GetXY(short x, short y) { /* $7E18 */
    return TileArray[y * 11 + x];
}

void PutXY(short a, short x, short y) {
    TileArray[y * 11 + x] = a;
}

void AddExp(short chnum, short amount) { /* $7091 */
    short rosNum, experience;
    rosNum = Party[6 + chnum];
    experience = (Player[rosNum][30] * 100) + Player[rosNum][31];
    int oldLvl = (experience / 100);
    experience += amount;
    if (experience > 9899)
        experience = 9899;
    int newLvl = (experience / 100);
    if (oldLvl < newLvl)
        U3AudioPlaySound(U3SoundEffectLevelUp, true);
    Player[rosNum][30] = experience / 100;
    Player[rosNum][31] = experience - (Player[rosNum][30] * 100);
    ShowChars(false);
}

Boolean AddGold(short rosNum, short gold, Boolean overflow) { /* $70BB */
    short presentGold;
    Boolean retVal;

    retVal = TRUE;
    presentGold = ((Player[rosNum][35]) * 256) + Player[rosNum][36];
    gold += presentGold;
    if (gold > 9999) {
        if (overflow == FALSE)
            return FALSE;
        retVal = FALSE;
        gold = 9999;
    }
    Player[rosNum][35] = gold / 256;
    Player[rosNum][36] = gold - (Player[rosNum][35] * 256);
    return retVal;
}

void AddItem(short rosNum, short item, short amount) { /* $7145 */
    Player[rosNum][item] += amount;
    if (Player[rosNum][item] > 99)
        Player[rosNum][item] = 99;
}

Boolean StealDisarmFail(short rosNum) { /* $75CF - result TRUE = failed */
    short classType, factor;
    Boolean result;
    factor = Player[rosNum][19];
    classType = Player[rosNum][23];
    if (classType == careerTable[3])
        factor += 0x80;    // thief
    // Barbarian, Illusionist, Ranger
    if (classType == careerTable[5] || classType == careerTable[7] || classType == careerTable[10])
        factor += 0x40;

    // Alchemists now get steal&disarm bonus also. Apple II version does not.
    if (classType == careerTable[9])
        factor += 0x40;

    result = FALSE;
    if (U3PlatformRandom(0, 255) > factor)
        result = TRUE;
    return result;
}

void IncMoves(void) { /* $3AF */
    Party[11] += Party[2];
    if (Party[11] > 99) {
        Party[11] -= 100;
        Party[12]++;
        if (Party[12] > 99) {
            Party[12] -= 100;
            Party[13]++;
            if (Party[13] > 99) {
                Party[13] -= 100;
                Party[14]++;
                if (Party[14] > 99) {
                    Party[11] = Party[12] = Party[13] = Party[14] = 99;
                }
            }
        }
    }
}

void GetDemoRsrc(void) {
    short ptr;
    if (gDemoData.owner)
        U3IOReleaseResource(&gDemoData);
    if (!U3IOLoadResource(U3ResourceKindDemo, BASERES, &gDemoData))
        return;
    if (gDemoData.size < 1151)
        return;
    for (ptr = 0; ptr < 127; ptr++) {
        TileArray[ptr] = gDemoData.bytes[ptr + 1024];
    }
}

void OpenRstr(void) {
    U3MutableDataBuffer resourceBuffer;
    short byte;
    U3SaveContainerOpenResult openResult = U3IOOpenSaveContainer();

    if (openResult == U3SaveContainerOpenResultFailed) {
        HandleError(U3IOLastError(), 38, 1);
        return;
    }

    if (openResult == U3SaveContainerOpenResultCreatedEmpty) {
        // No existing Ultima III Roster in Preferences, and no Roster next to
        // the app. Create the new roster from scratch.
        if (!U3IOCreateMutableResource(U3ResourceKindMonster, BASERES + 19, 256, (const uint8_t *)"\pSosaria Monsters", &resourceBuffer)) {
            HandleError(U3IOLastError(), 46, 0);
        } else {
            U3IOCloseMutableResource(&resourceBuffer, true);
        }
        if (!U3IOCreateMutableResource(U3ResourceKindPreferences, BASERES, 32, (const uint8_t *)"\pPreferences", &resourceBuffer)) {
            HandleError(U3IOLastError(), 47, 0);
        } else {
            for (byte = 0; byte < 8; byte++) {
                resourceBuffer.bytes[byte] = 1;
            }
            U3IOCloseMutableResource(&resourceBuffer, true);
        }

        // the new way, copy MAPS 420->419 and PRTY ROST 500->400
        if (!U3IOCopyResource(U3ResourceKindMap, BASERES + 20, BASERES + 19, (const uint8_t *)"\pSosaria Current"))
            HandleError(U3IOLastError(), 45, BASERES + 19);

        if (!U3IOCopyResource(U3ResourceKindParty, BASERES + 100, BASERES, (const uint8_t *)"\pParty"))
            HandleError(U3IOLastError(), 43, BASERES);

        if (!U3IOCopyResource(U3ResourceKindRoster, BASERES + 100, BASERES, (const uint8_t *)"\pRoster"))
            HandleError(U3IOLastError(), 44, BASERES);

        if (!U3IOCopyResource(U3ResourceKindMisc, BASERES, BASERES + 100, (const uint8_t *)"\pMoongate Locations"))
            HandleError(U3IOLastError(), 48, BASERES + 100);
        if (!U3IOCopyResource(U3ResourceKindMisc, BASERES + 1, BASERES + 101, (const uint8_t *)"\pType Initial Table"))
            HandleError(U3IOLastError(), 48, BASERES + 101);
        if (!U3IOCopyResource(U3ResourceKindMisc, BASERES + 2, BASERES + 102, (const uint8_t *)"\pWeapon Use By class"))
            HandleError(U3IOLastError(), 48, BASERES + 102);
        if (!U3IOCopyResource(U3ResourceKindMisc, BASERES + 3, BASERES + 103, (const uint8_t *)"\pArmour Use By class"))
            HandleError(U3IOLastError(), 48, BASERES + 103);
        if (!U3IOCopyResource(U3ResourceKindMisc, BASERES + 4, BASERES + 104, (const uint8_t *)"\pLocation Table"))
            HandleError(U3IOLastError(), 48, BASERES + 104);
        if (!U3IOCopyResource(U3ResourceKindMisc, BASERES + 5, BASERES + 105, (const uint8_t *)"\pExperience Table"))
            HandleError(U3IOLastError(), 48, BASERES + 105);
        U3IOFlushSaveContainer();
        if (U3IOLastError())
            HandleError(U3IOLastError(), 38, 1);
    }    // new Roster creation finished
}

void GetRoster(void) {
    short player, byte;
    U3DataBuffer rosterBuffer;

    if (!U3IOLoadResource(U3ResourceKindRoster, BASERES, &rosterBuffer))
        return;
    for (player = 0; player < 20; player++) {
        for (byte = 0; byte < 64; byte++) {
            Player[player + 1][byte] = rosterBuffer.bytes[((player)*64) + byte];
        }
    }
    U3IOReleaseResource(&rosterBuffer);
}

void PutRoster(void) {
    short player, byte;
    U3MutableDataBuffer rosterBuffer;

    if (!U3IOOpenMutableResource(U3ResourceKindRoster, BASERES, &rosterBuffer))
        return;
    for (player = 0; player < 20; player++) {
        for (byte = 0; byte < 64; byte++) {
            rosterBuffer.bytes[((player)*64) + byte] = Player[player + 1][byte];
        }
    }
    U3IOCloseMutableResource(&rosterBuffer, true);
}

void GetParty(void) {
    U3DataBuffer partyBuffer;

    if (!U3IOLoadResource(U3ResourceKindParty, BASERES, &partyBuffer))
        return;
    if (partyBuffer.size < U3PartyResourceSize) {
        U3IOReleaseResource(&partyBuffer);
        return;
    }
    Party[0] = 0;
    memcpy(Party + 1, partyBuffer.bytes, U3PartyResourceSize);
    U3IOReleaseResource(&partyBuffer);
    xpos = Party[4];
    ypos = Party[5];
}

void PutParty(void) {
    U3MutableDataBuffer partyBuffer;

    if (Party[3] == 0) {
        if (!U3IOOpenMutableResource(U3ResourceKindParty, BASERES, &partyBuffer))
            return;
        if (partyBuffer.size < U3PartyResourceSize) {
            U3IOCloseMutableResource(&partyBuffer, false);
            return;
        }
        memcpy(partyBuffer.bytes, Party + 1, U3PartyResourceSize);
        U3IOCloseMutableResource(&partyBuffer, true);
    }
}

Boolean U3PartyResourceSelfTest(void) {
    unsigned char savedParty[U3LegacyPartySize], original[U3PartyResourceSize];
    unsigned char expected[U3PartyResourceSize];
    int savedX = xpos, savedY = ypos;
    U3DataBuffer read = {0};
    U3MutableDataBuffer edit = {0};
    Boolean passed = FALSE;
    memcpy(savedParty, Party, sizeof(savedParty));
    if (!U3IOLoadResource(U3ResourceKindParty, BASERES, &read)) return FALSE;
    if (read.size != sizeof(original)) {
        U3IOReleaseResource(&read);
        return FALSE;
    }
    memcpy(original, read.bytes, sizeof(original));
    U3IOReleaseResource(&read);
    for (unsigned int i = 0; i < sizeof(expected); ++i) expected[i] = (i * 37 + 5) & 255;
    expected[2] = 0;
    expected[3] = 42;
    expected[4] = 20;
    for (unsigned int i = 0; i < 4; ++i) expected[6 + i] = i + 1;
    do {
        if (!U3IOOpenMutableResource(U3ResourceKindParty, BASERES, &edit)) break;
        memcpy(edit.bytes, expected, sizeof(expected));
        U3IOCloseMutableResource(&edit, true);
        if (U3IOLastError()) break;
        memset(Party, 0xA5, sizeof(Party));
        GetParty();
        if (U3IOLastError() || Party[0] != 0 || xpos != 42 || ypos != 20 ||
            memcmp(Party + 1, expected, sizeof(expected))) break;
        expected[U3PartyResourceSize - 1] ^= 255;
        Party[U3PartyResourceSize] = expected[U3PartyResourceSize - 1];
        PutParty();
        if (U3IOLastError() || U3IOOpenSaveContainer() != U3SaveContainerOpenResultOpened) break;
        memset(Party, 0xA5, sizeof(Party));
        GetParty();
        if (U3IOLastError() || Party[0] != 0 || memcmp(Party + 1, expected, sizeof(expected))) break;
        passed = TRUE;
    } while (0);
    U3IOCloseMutableResource(&edit, false);
    if (U3IOOpenMutableResource(U3ResourceKindParty, BASERES, &edit)) {
        memcpy(edit.bytes, original, sizeof(original));
        U3IOCloseMutableResource(&edit, true);
        if (U3IOLastError()) passed = FALSE;
    } else passed = FALSE;
    memcpy(Party, savedParty, sizeof(savedParty));
    xpos = savedX;
    ypos = savedY;
    fprintf(stderr, "Party resource: 64-byte load/save/reopen %s\n", passed ? "passed" : "FAILED");
    return passed;
}

void ResetSosaria(void) {
    short byte;
    long mapLength;
    U3DataBuffer originalMapBuffer, originalMonsterBuffer, originalMiscBuffer;
    U3MutableDataBuffer currentMapBuffer, currentMonsterBuffer, currentMiscBuffer;

    if (U3IOLoadResource(U3ResourceKindMap, BASERES + 20, &originalMapBuffer)) {
        if (originalMapBuffer.size >= 1) {
            gCurMapSize = originalMapBuffer.bytes[0];
            if (gCurMapSize == 0)
                gCurMapSize = 256;
            mapLength = (gCurMapSize * gCurMapSize) + 5;
            if (originalMapBuffer.size >= (size_t)mapLength &&
                U3IOOpenMutableResource(U3ResourceKindMap, BASERES + 19, &currentMapBuffer)) {
                if (U3IOResizeMutableResource(&currentMapBuffer, (size_t)mapLength)) {
                    memcpy(currentMapBuffer.bytes, originalMapBuffer.bytes, (size_t)mapLength);
                    U3IOCloseMutableResource(&currentMapBuffer, true);
                } else {
                    U3IOCloseMutableResource(&currentMapBuffer, false);
                }
            }
        }
        U3IOReleaseResource(&originalMapBuffer);
    }

    if (U3IOLoadResource(U3ResourceKindMonster, BASERES + 20, &originalMonsterBuffer)) {
        if (originalMonsterBuffer.size >= 256 &&
            U3IOOpenMutableResource(U3ResourceKindMonster, BASERES + 19, &currentMonsterBuffer)) {
            if (currentMonsterBuffer.size >= 256) {
                for (byte = 0; byte < 256; byte++) {
                    currentMonsterBuffer.bytes[byte] = originalMonsterBuffer.bytes[byte];
                }
                U3IOCloseMutableResource(&currentMonsterBuffer, true);
            } else {
                U3IOCloseMutableResource(&currentMonsterBuffer, false);
            }
        }
        U3IOReleaseResource(&originalMonsterBuffer);
    }

    if (U3IOLoadResource(U3ResourceKindMisc, BASERES, &originalMiscBuffer)) {
        if (originalMiscBuffer.size >= 16 &&
            U3IOOpenMutableResource(U3ResourceKindMisc, BASERES + 100, &currentMiscBuffer)) {
            if (currentMiscBuffer.size >= 16) {
                for (byte = 0; byte < 16; byte++) {
                    currentMiscBuffer.bytes[byte] = originalMiscBuffer.bytes[byte];
                }
                U3IOCloseMutableResource(&currentMiscBuffer, true);
            } else {
                U3IOCloseMutableResource(&currentMiscBuffer, false);
            }
        }
        U3IOReleaseResource(&originalMiscBuffer);
    }

    if (U3IOLoadResource(U3ResourceKindMisc, BASERES + 4, &originalMiscBuffer)) {
        if (originalMiscBuffer.size >= 64 &&
            U3IOOpenMutableResource(U3ResourceKindMisc, BASERES + 104, &currentMiscBuffer)) {
            if (currentMiscBuffer.size >= 64) {
                for (byte = 0; byte < 64; byte++) {
                    currentMiscBuffer.bytes[byte] = originalMiscBuffer.bytes[byte];
                }
                U3IOCloseMutableResource(&currentMiscBuffer, true);
            } else {
                U3IOCloseMutableResource(&currentMiscBuffer, false);
            }
        }
        U3IOReleaseResource(&originalMiscBuffer);
    }
}

void GetSosaria(void) {
    LoadUltimaMap(BASERES + 19);
    BlockExodus();
}

void BlockExodus(void) {
    if (Party[3] != 0 || gUpdateWhere != 3)
        return;    // if not Sosaria
    if (GetXYVal(0x0A, 0x35) == 0x1C && GetXYVal(0x0B, 0x36) == 0x00 && GetXYVal(0x0C, 0x35) == 0x10) {
        if (U3PlatformGetBooleanPreference(U3PreferenceNoDiagonals)) {
            PutXYVal(0x84, 0x09, 0x35);
            PutXYVal(0x84, 0x0B, 0x35);
        } else {
            PutXYVal(0x10, 0x09, 0x35);
            PutXYVal(0x10, 0x0B, 0x35);
        }
    }
}

void PutSosaria(void) {
    long mark;
    long mapLength;
    U3MutableDataBuffer currentMapBuffer, currentMonsterBuffer;

    if (Party[3] != 0)
        return;    // I shouldn't need this, dammit!
    mapLength = gCurMapSize * gCurMapSize;
    if (U3IOOpenMutableResource(U3ResourceKindMap, BASERES + 19, &currentMapBuffer)) {
        if (currentMapBuffer.size >= (size_t)(mapLength + 5)) {
            currentMapBuffer.bytes[0] = (unsigned char)(gCurMapSize & 0xFF);
            for (mark = 0; mark < mapLength; mark++) {
                currentMapBuffer.bytes[mark + 1] = (*Map)[mark];
            }
            currentMapBuffer.bytes[mapLength + 1] = (unsigned char)(WhirlX & 0xFF);
            currentMapBuffer.bytes[mapLength + 2] = (unsigned char)(WhirlY & 0xFF);
            currentMapBuffer.bytes[mapLength + 3] = (unsigned char)(WhirlDX & 0xFF);
            currentMapBuffer.bytes[mapLength + 4] = (unsigned char)(WhirlDY & 0xFF);
            U3IOCloseMutableResource(&currentMapBuffer, true);
        } else {
            U3IOCloseMutableResource(&currentMapBuffer, false);
        }
    }

    if (U3IOOpenMutableResource(U3ResourceKindMonster, BASERES + 19, &currentMonsterBuffer)) {
        if (currentMonsterBuffer.size >= 256) {
            for (mark = 0; mark < 256; mark++) {
                currentMonsterBuffer.bytes[mark] = Monsters[mark];
            }
            U3IOCloseMutableResource(&currentMonsterBuffer, true);
        } else {
            U3IOCloseMutableResource(&currentMonsterBuffer, false);
        }
    }
    PutMiscStuff();
}

void LoadUltimaMap(short resid) {
    long mark;
    long mapLength;
    Boolean commitMap = false;
    U3MutableDataBuffer currentMapBuffer;
    U3DataBuffer currentMonsterBuffer, currentTalkBuffer;

    gCurMapID = resid;
    if (!U3IOOpenMutableResource(U3ResourceKindMap, resid, &currentMapBuffer))
        return;
    if (resid > 411 && resid < 419) {
        if (currentMapBuffer.size >= 2048) {
            for (mark = 0; mark < 2048; mark++) {
                Dungeon[mark] = currentMapBuffer.bytes[mark];
            }
        }
    } else {
        if (resid == 419 && currentMapBuffer.size == 4100) {
            if (U3IOResizeMutableResource(&currentMapBuffer, 4101)) {
                memmove(currentMapBuffer.bytes + 1, currentMapBuffer.bytes, 4100);
                currentMapBuffer.bytes[0] = 64;
                commitMap = true;
            } else {
                U3IOCloseMutableResource(&currentMapBuffer, false);
                HandleError(MemError(), 61, 3);
                return;
            }
        }
        gCurMapSize = currentMapBuffer.bytes[0];
        if (gCurMapSize == 0)
            gCurMapSize = 256;
        mapLength = gCurMapSize * gCurMapSize;
        if (currentMapBuffer.size < (size_t)(mapLength + 1)) {
            U3IOCloseMutableResource(&currentMapBuffer, commitMap);
            return;
        }
        if (GetHandleSize(Map) > 0)
            DisposeHandle(Map);
        Map = NewHandle(mapLength);
        if (MemError()) {
            U3IOCloseMutableResource(&currentMapBuffer, commitMap);
            HandleError(MemError(), 61, 1);
            return;
        }
        for (mark = 0; mark < mapLength; mark++) {
            (*Map)[mark] = currentMapBuffer.bytes[mark + 1];
        }
        if (U3IOLoadResource(U3ResourceKindMonster, resid, &currentMonsterBuffer)) {
            if (currentMonsterBuffer.size >= 256) {
                for (mark = 0; mark < 256; mark++) {
                    Monsters[mark] = currentMonsterBuffer.bytes[mark];
                }
            }
            U3IOReleaseResource(&currentMonsterBuffer);
        }
    }
    if (resid < 419 || resid == 421) {   // "<419" *was* "<420", big mistake.
        if (U3IOLoadResource(U3ResourceKindTalk, resid, &currentTalkBuffer)) {
            if (currentTalkBuffer.size >= 256) {
                for (mark = 0; mark < 256; mark++) {
                    Talk[mark] = currentTalkBuffer.bytes[mark];
                }
            }
            U3IOReleaseResource(&currentTalkBuffer);
        }
    }
    if (resid == (BASERES + 19)) {   // Sosaria
        if (currentMapBuffer.size < (size_t)(mapLength + 5)) {
            U3IOCloseMutableResource(&currentMapBuffer, commitMap);
            return;
        }
        WhirlX = currentMapBuffer.bytes[mapLength + 1];
        WhirlY = currentMapBuffer.bytes[mapLength + 2];
        WhirlDX = currentMapBuffer.bytes[mapLength + 3];
        WhirlDY = currentMapBuffer.bytes[mapLength + 4];
    }
    U3IOCloseMutableResource(&currentMapBuffer, commitMap);
}

void PushSosaria(void) {
    long mapLength, pushSize, mark;

    mapLength = gCurMapSize * gCurMapSize;
    pushSize = 1 + mapLength + 256 + 4 + 2;    // size + map + mon + whirl + posn
    pushMap = NewHandle(pushSize);
    if (MemError())
        HandleError(MemError(), 61, 2);
    *(*pushMap) = (unsigned char)gCurMapSize;
    for (mark = 0; mark < mapLength; mark++) {   // back up sosaria
        *(*pushMap + mark + 1) = (*Map)[mark];
    }
    for (mark = 0; mark < 256; mark++) {   // and monster list
        *(*pushMap + mapLength + mark + 1) = Monsters[mark];
    }
    *(*pushMap + mapLength + 256) = (unsigned char)(WhirlX & 0xFF);
    *(*pushMap + mapLength + 257) = (unsigned char)(WhirlY & 0xFF);
    *(*pushMap + mapLength + 258) = (unsigned char)(WhirlDX & 0xFF);
    *(*pushMap + mapLength + 259) = (unsigned char)(WhirlDY & 0xFF);
    //  *(*pushMap+mapLength+260) = (unsigned char)xpos;
    //  *(*pushMap+mapLength+261) = (unsigned char)xpos;
}

void PullSosaria(void) {
    long mapLength, mark;

    if (GetHandleSize(pushMap) < 10) {   // was never pushed!
        GetSosaria();
        PutRoster();
        PutParty();
        PutSosaria();
    } else {   // oh, yes it was!  Don't tease!
        gCurMapSize = *(*pushMap);
        mapLength = gCurMapSize * gCurMapSize;
        for (mark = 0; mark < mapLength; mark++) {   // sosaria map
            (*Map)[mark] = *(*pushMap + mark + 1);
        }
        for (mark = 0; mark < 256; mark++) {   // and monster list
            Monsters[mark] = *(*pushMap + mapLength + mark + 1);
        }
        WhirlX = (unsigned char)*(*pushMap + mapLength + 256);
        WhirlY = (unsigned char)*(*pushMap + mapLength + 257);
        WhirlDX = (unsigned char)*(*pushMap + mapLength + 258);
        WhirlDY = (unsigned char)*(*pushMap + mapLength + 259);
        //      xpos=(unsigned char)*(*pushMap+mapLength+260);
        //      ypos=(unsigned char)*(*pushMap+mapLength+261);
        DisposeHandle(pushMap);
    }

    // If Exodus has been defeated, make land creatures placid.
    if (Party[16] == 1) {
        int i;
        for (i = 0; i < 32; i++) {
            if (Monsters[i] >= 0x40)
                Monsters[i + HPMON] = 0x40;
        }
    }
}

unsigned char GetXYVal(int x, int y) {
    unsigned char value;
    gMapOffset = (MapConstrain(y) * gCurMapSize);
    gMapOffset += MapConstrain(x);
    value = (*Map)[gMapOffset];
    if (Party[3] != 0) {
        if (x < 0 || x >= gCurMapSize || y < 0 || y >= gCurMapSize)
            value = 0x04;
    }
    return value;
}

void PutXYVal(unsigned char value, unsigned char x, unsigned char y) {
    gMapOffset = (MapConstrain(y) * gCurMapSize);
    gMapOffset += MapConstrain(x);
    (*Map)[gMapOffset] = value;
}

void Shrine(short chnum) { /* $9400 */
    short shtype = xpos & 0x03;
    short voiceTile=0;    // correspond to class which ability is most apropos for
    short rosNum, maxval, statnum=0, race, key=0, gold;
    short shMax[22] = {75, 75, 99, 75, 25, 75, 99, 75, 50, 99, 75, 50, 75, 99, 75, 75, 75, 50, 75, 99};
    rosNum = Party[6 + chnum];
    Str255 attributeName;

    ImageDisplay(7, TRUE);

    GetPascalStringFromArrayByIndex(attributeName, CFSTR("Messages"), 173 + shtype);
    //GetIndString(attributeName, BASERES+12, 174+shtype);
    Boolean classic = U3PlatformGetBooleanPreference(U3PreferenceClassicAppearance);
    if (classic) {
        U3RenderPrintMessage(235);
        for (key = 0; key < ((16 - attributeName[0]) / 2); key++) {
            U3RenderPrintPascalString("\p ");
        }
        U3RenderPrintPascalString(attributeName);
        U3RenderPrintPascalString("\p\n\n");
    }

    Str255 msg;
    GetPascalStringFromArrayByIndex(gString, CFSTR("Messages"), 258);
    //GetIndString(gString, BASERES+12, 259);
    BlockMove(gString, msg, gString[0] + 1);
    BlockMove(attributeName + 1, gString + gString[0] + 1, attributeName[0] + 1);
    gString[0] += attributeName[0];
    RewrapString(gString, false);
    gString[++gString[0]] = '\n';
    gString[++gString[0]] = '\n';
    if (!classic)
        U3RenderPrintPascalString(gString);

    race = Player[rosNum][22];
    switch (shtype) {
        case 0:
            maxval = shMax[ShrineRace(race)];
            statnum = 18;
            voiceTile = 20;
            break;
        case 1:
            maxval = shMax[ShrineRace(race) + 5];
            statnum = 19;
            voiceTile = 23;
            break;
        case 2:
            maxval = shMax[ShrineRace(race) + 15];
            statnum = 20;
            voiceTile = 22;
            break;
        case 3:
            maxval = shMax[ShrineRace(race) + 10];
            statnum = 21;
            voiceTile = 21;
            break;
    }

    U3AudioSpeakPascalString(gString, voiceTile);
    U3RenderPrintMessage(178);
    while ((key < '0' || key > '9') && (gDone == FALSE)) {
        U3PlatformGetKeyMouse(0);
        key = gKeyPress;
    }
    U3RenderPrintCharAt(key, tx, ty);
    key -= '0';
    if (key == 0) {
        U3RenderPrintMessage(179);
        ImageGoAway();
        return;
    }
    gold = ((Player[rosNum][35]) * 256) + Player[rosNum][36];
    if (gold - (key * 100) < 0) {
        U3RenderPrintMessage(180);
        U3AudioPlaySound(U3SoundEffectError1, false);
        ImageGoAway();
        return;
    }
    gold -= (key * 100);
    Player[rosNum][35] = gold / 256;
    Player[rosNum][36] = gold - (Player[rosNum][35] * 256);
    U3RenderPrintMessage(181);
    InverseChar(chnum - 1);
    InverseTiles();
    U3AudioPlaySound(U3SoundEffectShrine, false);    // was 0xF0
    InverseTiles();
    InverseChar(chnum - 1);
    if (statnum>0) {
        Player[rosNum][statnum] += key;
        if (Player[rosNum][statnum] > 99)
            Player[rosNum][statnum] = 99;
        if ((Player[rosNum][statnum] > maxval) && (Party[16] == 0))
            Player[rosNum][statnum] = maxval;
    }
    ShowChars(TRUE);
    ImageGoAway();
    return;
}

short ShrineRace(short race) {
    short byte, result;
    Str255 tempStr;

    result = 0;
    for (byte = 0; byte < 5; byte++) {
        GetPascalStringFromArrayByIndex(tempStr, CFSTR("Races"), byte);
        if (race == tempStr[1])
            result = byte;
    }
    return result;
}

void FinishAll(void) { /* $79DD */
    tx = 0x18;
    ty = 0x17;
    if (Party[1] == 0x14 || Party[1] == 0x16) {
        zp[0xCD] = 255 - zp[0xCD];
        if (zp[0xCD] < 128)
            return;
    }
    if (gTimeNegate != 0) {
        gTimeNegate--;
        return;
    }
    SpawnMonster();
    MoveMonsters();
}

void SpawnUntilFull(void) {
    Boolean foundEmptySpot = true;
    while (foundEmptySpot) {
        foundEmptySpot = false;
        int offset = 31;
        while (!foundEmptySpot && offset >= 0) {
            foundEmptySpot = (Monsters[offset--] == 0);
        }
        if (foundEmptySpot)
            SpawnMonster();
    }
}

void SpawnMonster(void) { /* $7A0C */
    Boolean allFirst;
    long hpmax;
    short offset, type, chnum;
    if (Party[3] != 0)
        return;
    if (U3PlatformRandom(0, 134) < 128)
        return;
    offset = 32;
spawn:
    offset--;
    if (offset < 0)
        return;
    if (Monsters[offset] != 0)
        goto spawn;
    allFirst = TRUE;
    for (chnum = 0; chnum < 4; chnum++) {   // is everyone hpmax = 150?
        hpmax = Player[Party[chnum + 7]][28] * 256 + Player[Party[chnum + 7]][29];
        if (hpmax > 150)
            allFirst = FALSE;
    }
    type = U3PlatformRandom(0, 12) & U3PlatformRandom(0, 12);
    if (allFirst)
        type = U3PlatformRandom(0, 2);    // then only thiefs/orx/skeletons
    Monsters[offset] = MonTypes[type] * 4;
    Monsters[offset + TILEON] = MonBegin[type];
    Monsters[offset + XMON] = U3PlatformRandom(0, gCurMapSize - 1);
    if (Monsters[offset + XMON] == xpos) {
        Monsters[offset] = 0;
        goto spawn;
    }
    Monsters[offset + YMON] = U3PlatformRandom(0, gCurMapSize - 1);
    if (Monsters[offset + YMON] == ypos) {
        Monsters[offset] = 0;
        goto spawn;
    }
    if (GetXYVal(Monsters[offset + XMON], Monsters[offset + YMON]) != Monsters[offset + TILEON]) {
        Monsters[offset] = 0;
        goto spawn;
    }
    Monsters[offset + HPMON] = 0xC0;
    char var = 0;
    if (U3PlatformRandom(0, 1))
        var = U3PlatformRandom(1, 2);
    Monsters[offset + VARMON] = var;
    if (Party[16] != 0 && Monsters[offset] != 0x3C)
        Monsters[offset + HPMON] = 0x40;
    PutXYVal(Monsters[offset], Monsters[offset + XMON], Monsters[offset + YMON]);
}

void MoveMonsters(void) { /* $7A81 */
    short offset, value, expnum1, expnum2;
    offset = 32;
movemon: /* $7A85 */
    offset--;
    if (offset < 0)
        return;
    if (Monsters[offset] == 0)
        goto movemon;
    if (Party[3] == 0 && Party[16] == 0) {    // Player hasn't beaten Exodus.
    moveoutside:
        GetMonsterDir(offset);
        if (xpos == xs && ypos == ys) {
            AttackCode(offset);
            return;
        }
    move7AAA:
        // check if this is a valid place for the monster to walk on.
        value = ValidMonsterDir(GetXYVal(xs, ys), Monsters[offset]);
        if (value == 0 && MonsterHere(xs, ys) != 255)
            value = 255;
        if (value != 0) {
            xs = Monsters[offset + XMON];
            value = ValidMonsterDir(GetXYVal(xs, ys), Monsters[offset]);
            if (value == 0 && MonsterHere(xs, ys) != 255)
                value = 255;
            if (value != 0) {
                xs = MapConstrain(Monsters[offset + XMON] + dx);
                ys = MapConstrain(Monsters[offset + YMON]);    // no +dy!?
                value = ValidMonsterDir(GetXYVal(xs, ys), Monsters[offset]);
                if (value == 0 && MonsterHere(xs, ys) != 255)
                    value = 255;
                if (value != 0) {
                    if (Monsters[offset] == 0x3C || Monsters[offset] == 0x74)
                        goto moveshoot;
                    goto movemon;
                }
            }
        }
        if (xpos == xs && ypos == ys)
            goto movemon;
        PutXYVal(Monsters[offset + TILEON], Monsters[offset + XMON], Monsters[offset + YMON]);
        Monsters[offset + XMON] = xs;
        Monsters[offset + YMON] = ys;
        Monsters[offset + TILEON] = GetXYVal(Monsters[offset + XMON], Monsters[offset + YMON]);
        unsigned char monsterTile = Monsters[offset];
        if (Monsters[offset + VARMON]) {
            monsterTile += Monsters[offset + VARMON];
        }
        PutXYVal(monsterTile, Monsters[offset + XMON], Monsters[offset + YMON]);
        //PutXYVal(Monsters[offset],Monsters[offset+XMON],Monsters[offset+YMON]);
        if (Monsters[offset] == 0x3C || Monsters[offset] == 0x74)
            goto moveshoot;
        goto movemon;
    moveshoot: /* $7B36 */
        if (U3PlatformRandom(0, 255) > 127)
            goto movemon;
        GetMonsterDir(offset);
        xs = 5 - zp[0xF5];
        if (xs > 10 || xs < 0)
            goto movemon;
        ys = 5 - zp[0xF6];
        if (ys > 10 || ys < 0)
            goto movemon;
        DrawMap(xpos, ypos);
        U3AudioPlaySound(U3SoundEffectShoot, true);    // was 0xEA
        zp[0xFB] = 3;
    moveshoot2: /* $7B60 */
        xs += dx;
        if (xs > 10 || xs < 0)
            goto movemon;
        ys += dy;
        if (ys > 10 || ys < 0)
            goto movemon;
        value = GetXY(xs, ys);
        if (value == 0x08 || value == 0x46 || value == 0x48)
            goto movemon;
        gBallTileBackground = value;
        PutXY(0x7A, xs, ys);
        DrawTiles();
        DrawMapPause();
        PutXY(value, xs, ys);
        if (xs == 5 && ys == 5) {
            BombTrap();
            goto movemon;
        }
        zp[0xFB]--;
        if (zp[0xFB] > 0)
            goto moveshoot2;
        goto movemon;
    } else {
        value = Monsters[offset + 128];
        value = value & 0xC0;
        if (value == 0)
            goto movemon;
        if (value == 0x40) {
            if (U3PlatformRandom(0, 255) < 128)
                goto movemon;
            xs = MapConstrain(Monsters[offset + XMON] + GetHeading(U3PlatformRandom(0, 255)));
            if (xs == 0)
                goto movemon;
            ys = MapConstrain(Monsters[offset + YMON] + GetHeading(U3PlatformRandom(0, 255)));
            if (ys == 0)
                goto movemon;

            // Sosaria, and user has already defeated Exodus.  Handle monsters running into one another.
            if (Party[16] != 0 && Party[3] == 0) {
                value = GetXYVal(xs, ys);
                if (value > 0x27) {
                    value = MonsterHere(xs, ys);
                    if (value != 255 && Monsters[offset] != Monsters[value]) {
                        if (xs > (xpos - 6) && ys > (ypos - 6) && xs < (xpos + 6) && ys < (ypos + 6)) {
                            gBallTileBackground = Monsters[value + TILEON] / 2;
                            PutXYVal(0xF4, xs, ys);
                            DrawMap(xpos, ypos);
                            U3AudioPlaySound(U3SoundEffectHit, false);    // was 0xF7
                            PutXYVal(Monsters[value], xs, ys);
                            DrawMap(xpos, ypos);
                        }
                        expnum1 = (Experience[Monsters[offset] / 4 & 0x0F]);
                        expnum2 = (Experience[Monsters[value] / 4 & 0x0F]);
                        if (expnum1 >= expnum2) {
                            Monsters[value] = 0;
                            PutXYVal(Monsters[value + TILEON], xs, ys);
                            DrawMap(xpos, ypos);
                        }
                    }
                }
            }

            goto move7AAA;
        }
        if (value == 0x80) {
            GetMonsterDir(offset);
            goto move7AAA;
        }
        if (value != 0x40 && value != 0x80)
            goto moveoutside;
    }
}

void Routine6E6B(void) {
    if (!gDone) {
        xpos = zp[0xE3];
        ypos = zp[0xE4];
        Party[3] = 0;    // back to surface
        gSongCurrent = gSongNext = 0;
        if (!gResurrect)
            U3RenderPrintMessage(182);
        //      U3RenderPrintPascalString("\pPLEASE WAIT...\n");
        Party[4] = xpos;
        Party[5] = ypos;
        if (U3PlatformGetBooleanPreference(U3PreferenceAutoSave)) {
            GetSosaria();
            PutRoster();
            PutParty();
            PutSosaria();
        } else
            PullSosaria();

        gUpdateWhere = 3;
        gSongNext = 1;
        ShowWind();
    }
}

void Routine6E35(void) {
    short temp;
    IncMoves();
    MoonGateUpdate();
    if (Party[3] == 1) {
        DrawDungeon();
        DungeonStart(1);
        return;
    }
    /* if $E2=#$80 (combat?), see $6E5C.  Apparently unneccessary
       code, since the combat routines never touch this area while in effect. */
    if (Party[3] > 1) { /* Town or castle, IOW */
        if (xpos == 0 || ypos == 0) {
            Routine6E6B();
        }
    }
    AgeChars();
    ShowChars(false);
    temp = GetXYVal(xpos, ypos);
    if (temp == 136)
        HandleMoonStep();
    if (temp == 48)
        GoWhirlPool();
    if (ExodusCastle() == 0) {
        gTimeNegate = 0;
        xs = U3PlatformRandom(0, 11);
        ys = U3PlatformRandom(0, 11);
        gBallTileBackground = GetXYTile(xs, ys);
        if (xs == 5 && ys == 5) {
            PutXYTile(0x7A, xs, ys);
            DrawTiles();
            DrawMapPause();
            BombTrap();
            DrawMap(xpos, ypos);
        } else {
            if (gBallTileBackground == 0x10) {
                PutXYTile(0x7A, xs, ys);
                DrawTiles();
                U3AudioPlaySound(U3SoundEffectHit, false);    // was 0xF7
                DrawMap(xpos, ypos);
            }
        }
    }
    FinishAll();
}

void AgeChars(void) { /* $7470 */
    int x, rosNum, temp;
    char cType;
    if (Party[3] > 0) {
        gTime[0]--;
        if (gTime[0] > 0)
            return;
        gTime[0] = 4;
    }
    gTime[1]--;
    if (gTime[1] < 0)
        gTime[1] = 9;
    for (x = 3; x >= 0; x--) {
        rosNum = Party[x + 7];
        cType = Player[rosNum][23];

        // Wizard, full Int
        if (cType == careerTable[2]) {
            if (Player[rosNum][25] < Player[rosNum][20])
                Player[rosNum][25]++;
        }

        // Cleric, full Wis
        if (cType == careerTable[1]) {
            if (Player[rosNum][25] < Player[rosNum][21])
                Player[rosNum][25]++;
        }

        // Lark Druid Alchemist, half Int
        if ((cType == careerTable[6]) || (cType == careerTable[8]) || (cType == careerTable[9])) {
            if (Player[rosNum][25] < (Player[rosNum][20] / 2))
                Player[rosNum][25]++;
        }

        // Paladin Illusionist Druid, half Wis
        if ((cType == careerTable[4]) || (cType == careerTable[7]) || (cType == careerTable[8])) {
            if (Player[rosNum][25] < (Player[rosNum][21] / 2))
                Player[rosNum][25]++;
        }

        // Ranger lesser of half Wis or half Int
        if (cType == careerTable[10]) {
            temp = Player[rosNum][25];
            if ((temp < (Player[rosNum][20] / 2)) && (temp < (Player[rosNum][21] / 2))) {
                Player[rosNum][25]++;
            }
        }
        temp = Player[rosNum][17];
        if ((temp == 'D') || (temp == 'A') || (temp == 0)) {
        } else {
            EatFood(x, 10);
            if (Player[rosNum][17] == 'P') {
                HPSubtract(rosNum, 1);
                InverseChar(x);
                if (!gDone)
                    U3RenderPrintMessage(183);
                InverseChar(x);
            }
            if (gTime[1] == 0)
                HPAdd(rosNum, 1);
        }
    }
    ShowChars(false);
}

void EatFood(short member, short amount) { /* member = 0-3 $761D */
    short rosNum;
    rosNum = Party[7 + member];
    Player[rosNum][34] -= amount;
    if (Player[rosNum][34] > 127) {
        Player[rosNum][34] -= 157;
        Player[rosNum][33] -= 1;
        if (Player[rosNum][33] > 127) {
            Player[rosNum][33] -= 157;
            Player[rosNum][32] -= 1;
            if (Player[rosNum][32] > 127) {
                Player[rosNum][32] = Player[rosNum][33] = Player[rosNum][34] = 0;
                U3RenderPrintMessage(184);
                InverseChar(member);
                U3AudioPlaySound(U3SoundEffectHit, false);    // was 0xF7
                InverseChar(member);
                HPSubtract(rosNum, 5);
            }
        }
    }
}

void MoonGateUpdate(void) { /* $6F5D */
    if (Party[3] != 0)
        return; /* only if on surface! */
    gMoon[0]--;
    if (gMoon[0] < 1) {
        gMoon[0] = 12;
        gMoonDisp[0]++;
        if (gMoonDisp[0] > '7')
            gMoonDisp[0] = '0';
    }
    gMoon[1]--;
    if (gMoon[1] < 1) {
        gMoon[1] = 4;
        gMoonDisp[1]++;
        if (gMoonDisp[1] > '7')
            gMoonDisp[1] = '0';
    }
    DrawMoonGateStuff();
}

void DrawMoonGateStuff(void) {
    if (U3PlatformGetBooleanPreference(U3PreferenceClassicAppearance)) {
        U3RenderPrintPascalStringAt("\p(", 9, 0);
        U3RenderPrintCharAt(gMoonDisp[0], tx, ty);
        U3RenderPrintPascalStringAt("\p)(", 11, 0);
        U3RenderPrintCharAt(gMoonDisp[1], tx, ty);
        U3RenderPrintPascalStringAt("\p)", 14, 0);
        DrawFramePiece(12, 8, 0);
        DrawFramePiece(13, 15, 0);
    } else {
        Str32 str;
        short txstor, tystor;
        Rect fromRect, toRect;

        ForeColor(blackColor);
        BackColor(whiteColor);
        SetRect(&toRect, blkSiz * 8, 0, blkSiz * 9, blkSiz);    // was 9-10
        txstor = (gMoonDisp[0] - '0') * blkSiz;
        SetRect(&fromRect, txstor, blkSiz * 2, txstor + blkSiz, blkSiz * 3);
        CopyBits(LWPortCopyBits(framePort), LWPortCopyBits(mainPort),    // Trammel
                 &fromRect, &toRect, srcCopy, nil);
        SetRect(&toRect, blkSiz * 12, 0, blkSiz * 13, blkSiz);    // was 13-14
        txstor = (gMoonDisp[1] - '0' + 8) * blkSiz;
        SetRect(&fromRect, txstor, blkSiz * 2, txstor + blkSiz, blkSiz * 3);
        CopyBits(LWPortCopyBits(framePort), LWPortCopyBits(mainPort),    // Felucca
                 &fromRect, &toRect, srcCopy, nil);
        txstor = tx;
        tystor = ty;
        str[0] = 3;
        str[1] = '(';
        str[2] = gMoonDisp[0];
        str[3] = ')';
        UCenterAt(str, 9, 0);
        str[0] = 3;
        str[1] = '(';
        str[2] = gMoonDisp[1];
        str[3] = ')';
        UCenterAt(str, 13, 0);
        tx = txstor;
        ty = tystor;
        DrawFramePiece(12, 7, 0);
        DrawFramePiece(13, 16, 0);
    }
    if (Party[3] != 0)
        return;
    if ((gMoonDisp[0] == '0') && (gMoonDisp[1] == '0')) {
        PutXYVal(24, LocationX[8], LocationY[8]);
    }    // Towne (Dawn)
    else if (GetXYVal(LocationX[8], LocationY[8]) == 24) {
        PutXYVal(12, LocationX[8], LocationY[8]);
    }    // or forest
    short x;
    for (x = 0; x < 8; x++) {
        PutXYVal(4, MoonXTable[x], MoonYTable[x]);
    }
    x = gMoonDisp[0] - '0';
    PutXYVal(136, MoonXTable[x], MoonYTable[x]);    // put Moongate
}

void HandleMoonStep(void) { /* $7961 */
    short value;
    int oldx, oldy;

    oldx = xpos;
    oldy = ypos;
    if (Party[3] == 0) {
        value = gMoonDisp[1] - '0';
        xpos = MoonXTable[value];
        ypos = MoonYTable[value];
    } else {
        value = 0;
        while (value != 0x04) {
            xpos = U3PlatformRandom(0, gCurMapSize - 1);
            ypos = U3PlatformRandom(0, gCurMapSize - 1);
            value = GetXYVal(xpos, ypos);
        }
    }
    DrawMap(oldx, oldy);
    InverseTiles();
    ForceUpdateMain();
    U3AudioPlaySound(U3SoundEffectMoongate, false);    // was 0xF1
    InverseTiles();
    DrawMap(xpos, ypos);
    InverseTiles();
    ForceUpdateMain();
    U3AudioPlaySound(U3SoundEffectMoongate, false);    // was 0xF1
    InverseTiles();
}

void Shop(short shopNum, short chnum) {
    short rosNum, input, gold, opnum;
    Str255 message, spmessage;
    long amount;

    rosNum = Party[6 + chnum];
    switch (shopNum) {
        case 0:
            U3RenderPrintMessageRewrapped(185);
        shop0:
            U3RenderPrintMessage(186);
            U3AudioSpeakMessages(186, 0, 20);
            //Speech(GetLocalizedPascalString("\pHere friend, have a drink! It costs 7 gold."),20);
            input = UInputNum(tx, ty);
            U3RenderPrintPascalString("\p\n\n");
            if (input < 7) {
                U3RenderPrintMessageRewrapped(187);
                U3AudioSpeakMessages(187, 0, 20);
                //Speech(GetLocalizedPascalString("\pLeave my shop! You scum!"),20);
                U3AudioPlaySound(U3SoundEffectError1, true);
                return;
            }
            gold = (Player[rosNum][35] * 256) + Player[rosNum][36];
            if (gold < input) {
                U3RenderPrintMessageRewrapped(188);
                U3AudioSpeakMessages(188, 0, 20);
                //Speech(GetLocalizedPascalString("\pWhat? Can't pay? Out, you scum!"),20);
                U3AudioPlaySound(U3SoundEffectError1, true);
                return;
            }
            gold -= input;
            Player[rosNum][35] = gold / 256;
            Player[rosNum][36] = gold - (Player[rosNum][35] * 256);
            GetPascalStringFromArrayByIndex(message, CFSTR("Pub"), (input / 10));
            if (!U3PlatformGetBooleanPreference(U3PreferenceClassicAppearance))
                RewrapString(message, false);
            U3RenderPrintPascalString(message);
            for (opnum = 0; opnum <= message[0]; opnum++) {
                spmessage[opnum] = message[opnum];
                if (message[opnum] > 'z')
                    spmessage[opnum] = ' ';
            }
            U3AudioSpeakPascalString(message, 20);
            U3RenderPrintMessage(189);
            //Speech("\pAnother drink?",20);
            input = U3PlatformCursorKey(false);
            if (input > 95)
                input -= 32;
            if (input != 'Y') {
                U3RenderPrintPascalString("\pN");
                U3RenderPrintMessageRewrapped(190);
                U3AudioSpeakMessages(190, 0, 20);
                //Speech(GetLocalizedPascalString("\pIts been a pleasure!"),20);
                return;
            }
            U3RenderPrintPascalString("\pY");
            goto shop0;
            break;
        case 1:
            U3RenderPrintMessage(191);
        shop1:
            U3RenderPrintMessage(192);
            U3AudioSpeakMessages(192, 0, 16);
            //Speech(GetLocalizedPascalString("\pRations 1 gold each, how many would you like?"),16);
            input = UInputBigNum(tx, ty);
            if (input == 0) {
                U3RenderPrintPascalString("\p\n\n");
                return;
            }
            int existingFood = (Player[rosNum][32] * 100) + Player[rosNum][33];
            if (input > (9999 - existingFood)) {
                U3RenderPrintMessageRewrapped(260);
                U3RenderPrintPascalString("\p\n\n");
                U3AudioPlaySound(U3SoundEffectError1, true);
                goto shop1;
            }
            gold = (Player[rosNum][35] * 256) + Player[rosNum][36];
            if (gold < input) {
                U3RenderPrintMessageRewrapped(193);
                U3AudioSpeakMessages(193, 0, 16);
                //Speech(GetLocalizedPascalString("\pWhat? Can't pay? Out, you scum!"),16);
                U3AudioPlaySound(U3SoundEffectError1, true);
                return;
            }
            gold -= input;
            Player[rosNum][35] = gold / 256;
            Player[rosNum][36] = gold - (Player[rosNum][35] * 256);
            existingFood += input;
            Player[rosNum][32] = existingFood / 100;
            Player[rosNum][33] = existingFood - (Player[rosNum][32] * 100);
            ShowChars(false);
            U3RenderPrintMessageRewrapped(194);
            U3AudioSpeakMessages(194, 0, 16);
            //Speech(GetLocalizedPascalString("\pThank you, anything else?"),16);
            input = U3PlatformCursorKey(false);
            if (input > 95)
                input -= 32;
            if (input != 'Y') {
                U3RenderPrintPascalString("\pN\n\n");
                U3RenderPrintMessageRewrapped(195);
                U3AudioSpeakMessages(195, 0, 16);
                //Speech(GetLocalizedPascalString("\pVery well, come again!"),16);
                return;
            }
            U3RenderPrintPascalString("\pY\n\n");
            goto shop1;
            break;
        case 2:
            U3RenderPrintMessage(196);
            U3RenderPrintMessage(197);
            U3AudioSpeakMessages(197, 0, 9);
            //Speech(GetLocalizedPascalString("\pYour needs?"),9);
            input = 0;
            while ((input < '0' || input > '4') && input != ' ') {
                input = U3PlatformCursorKey(false);
            }
            if (input == ' ') {
                input = 0;
            } else {
                input -= '0';
            }
            switch (input) {
                case 0: U3RenderPrintPascalString("\p0\n"); break;
                case 1:
                    U3RenderPrintMessage(198);
                    U3AudioSpeakMessages(198, 0, 9);
                    //Speech(GetLocalizedPascalString("\pA curing will cost 100 gold.  Wilt thou pay?"),9);
                    if (Clerical(rosNum, 100) == 0) {
                        U3RenderPrintMessage(199);
                        opnum = GetChar();
                        U3RenderPrintPascalString("\p\n");
                        if (opnum >= 1 && opnum <= 4) {
                            SpellNoize(opnum - 1);
                            if (Player[Party[6 + opnum]][17] == 'P')
                                Player[Party[6 + opnum]][17] = 'G';
                        } else {
                            U3AudioPlaySound(U3SoundEffectBump, true);
                        }    // was 0xE7
                    }
                    break;
                case 2:
                    U3RenderPrintMessage(200);
                    U3AudioSpeakMessages(200, 0, 9);
                    //Speech(GetLocalizedPascalString("\pHealings cost 200 gold.  Wilt thou pay?"),9);
                    if (Clerical(rosNum, 200) == 0) {
                        U3RenderPrintMessage(201);
                        opnum = GetChar();
                        U3RenderPrintPascalString("\p\n");
                        if (opnum >= 1 && opnum <= 4) {
                            SpellNoize(opnum - 1);
                            Player[Party[6 + opnum]][26] = Player[Party[6 + opnum]][28];
                            Player[Party[6 + opnum]][27] = Player[Party[6 + opnum]][29];
                        } else {
                            U3AudioPlaySound(U3SoundEffectBump, true);
                        }    // was 0xE7
                    }
                    break;
                case 3:
                    U3RenderPrintMessage(202);
                    U3AudioSpeakMessages(202, 0, 9);
                    //Speech(GetLocalizedPascalString("\pResurrections cost 500 gold.  Wilt thou pay?"),9);
                    if (Clerical(rosNum, 500) == 0) {
                        U3RenderPrintMessage(203);
                        opnum = GetChar();
                        U3RenderPrintPascalString("\p\n");
                        if (opnum >= 1 && opnum <= 4) {
                            SpellNoize(opnum - 1);
                            if (Player[Party[6 + opnum]][17] == 'D')
                                Player[Party[6 + opnum]][17] = 'G';
                        } else {
                            U3AudioPlaySound(U3SoundEffectBump, true);
                        }    // was 0xE7
                    }
                    break;
                case 4:
                    U3RenderPrintMessage(204);
                    U3AudioSpeakMessages(204, 0, 9);
                    //Speech(GetLocalizedPascalString("\pRecallings cost 900 gold.  Wilt thou pay?"),9);
                    if (Clerical(rosNum, 900) == 0) {
                        U3RenderPrintMessage(205);
                        opnum = GetChar();
                        U3RenderPrintPascalString("\p\n");
                        if (opnum >= 1 && opnum <= 4) {
                            SpellNoize(opnum - 1);
                            if (Player[Party[6 + opnum]][17] == 'A')
                                Player[Party[6 + opnum]][17] = 'G';
                        } else {
                            U3AudioPlaySound(U3SoundEffectBump, true);
                        }    // was 0xE7
                    }
                    break;
            }
            return;
            break;
        case 3:
            opnum = 'I';
            if (Party[4] == 37)
                opnum = 'P';    // x location of party on Sosaria
            U3RenderPrintMessage(206);
            U3AudioSpeakMessages(206, 0, 16);
            //Speech(GetLocalizedPascalString("\pWelcome to the weapons shop!"),16);
            input = U3PlatformCursorKey(false);
            while (input > 95) {
                input -= 32;
            }
            U3RenderPrintCharAt(input, tx, ty);
            if (input == 'Y')
                WeaponList(opnum);
            U3RenderPrintMessage(207);
            U3AudioSpeakMessages(207, 0, 16);
            //Speech(GetLocalizedPascalString("\pBuy or sell?"),16);
            input = U3PlatformCursorKey(false);
            while (input > 95) {
                input -= 32;
            }
            U3RenderPrintCharAt(input, tx, ty);
        shop3:
            if (input == 'B') {
                U3RenderPrintMessage(208);
                input = U3PlatformCursorKey(false);
                while (input > 95) {
                    input -= 32;
                }
                if (input < 'B' || input >= opnum)
                    goto shop3done;
                U3RenderPrintCharAt(input, tx, ty);
                input -= 'B';
                gold = (Player[rosNum][35] * 256) + Player[rosNum][36];
                GetPascalStringFromArrayByIndex(gString, CFSTR("WeaponsArmour"), input + 25);
                StringToNum(gString, &amount);
                if (amount > gold) {
                    U3RenderPrintMessageRewrapped(209);
                    U3AudioSpeakMessages(209, 0, 16);
                    //Speech(GetLocalizedPascalString("\pI'm very sorry, but you haven't the gold!"),16);
                    return;
                }
                if (Player[rosNum][49 + input] > 98) {
                    U3RenderPrintMessageRewrapped(260);
                    U3RenderPrintPascalString("\p\n\n");
                    U3AudioPlaySound(U3SoundEffectError1, true);
                    input += 'B';
                    goto shop3;
                }
                gold -= amount;
                Player[rosNum][35] = gold / 256;
                Player[rosNum][36] = gold - (Player[rosNum][35] * 256);
                Player[rosNum][49 + input]++;
                if (Player[rosNum][49 + input] > 99)
                    Player[rosNum][49 + input] = 99;
                U3RenderPrintMessageRewrapped(210);
                U3AudioSpeakMessages(210, 0, 16);
                //Speech(GetLocalizedPascalString("\pHere you are, may it serve you well!"),16);
                input = 'B';
                goto shop3;
            } else {
                U3RenderPrintMessage(211);
                input = U3PlatformCursorKey(false);
                while (input > 95) {
                    input -= 32;
                }
                if (input < 'A' || input >= opnum)
                    goto shop3done;
                if (input == 'A') {
                    DoStandardAlert(kAlertNoteAlert, 9);
                    goto shop3done;
                }
                U3RenderPrintCharAt(input, tx, ty);
                input -= 'B';
                if (Player[rosNum][49 + input] < 1) {
                    U3RenderPrintMessageRewrapped(212);
                    U3AudioSpeakMessages(212, 0, 16);
                    //Speech(GetLocalizedPascalString("\pYou don't own one of those!"),16);
                    return;
                }
                GetPascalStringFromArrayByIndex(gString, CFSTR("WeaponsArmour"), input + 25);
                StringToNum(gString, &amount);
                if (AddGold(rosNum, amount, FALSE)) {
                    Player[rosNum][49 + input]--;
                    if (Player[rosNum][49 + input] < 1 && Player[rosNum][48] == input + 1)
                        Player[rosNum][48] = 0;
                    U3RenderPrintMessage(213);
                    U3AudioSpeakMessages(213, 0, 16);
                    //Speech(GetLocalizedPascalString("\pThank you!"),16);
                } else {
                    U3RenderPrintMessageRewrapped(214);
                    U3AudioPlaySound(U3SoundEffectError1, true);
                    return;
                }
                input = 'S';
                goto shop3;
            }
        shop3done:
            U3RenderPrintMessageRewrapped(215);
            U3AudioSpeakMessages(215, 0, 16);
            //Speech(GetLocalizedPascalString("\pOh well, maybe next time!"),16);
            return;
            break;
        case 4:
            opnum = 'F';
            if (Party[4] == 37)
                opnum = 'H';
            U3RenderPrintMessage(216);
            U3AudioSpeakMessages(216, 0, 18);
            //Speech(GetLocalizedPascalString("\pWelcome to the armor shop!"),18);
            input = U3PlatformCursorKey(false);
            while (input > 95) {
                input -= 32;
            }
            U3RenderPrintCharAt(input, tx, ty);
            if (input == 'Y')
                ArmourList(opnum);
            U3RenderPrintMessage(207);
            U3AudioSpeakMessages(207, 0, 18);
            //Speech(GetLocalizedPascalString("\pBuy or sell?"),18);
            input = U3PlatformCursorKey(false);
            while (input > 95) {
                input -= 32;
            }
            U3RenderPrintCharAt(input, tx, ty);
        shop4:
            if (input == 'B') {
                U3RenderPrintMessage(208);
                input = U3PlatformCursorKey(false);
                while (input > 95) {
                    input -= 32;
                }
                if (input < 'B' || input >= opnum)
                    goto shop4done;
                U3RenderPrintCharAt(input, tx, ty);
                input -= 'B';
                gold = (Player[rosNum][35] * 256) + Player[rosNum][36];
                GetPascalStringFromArrayByIndex(gString, CFSTR("WeaponsArmour"), input + 41);
                StringToNum(gString, &amount);
                if (amount > gold) {
                    U3RenderPrintMessage(209);
                    U3AudioSpeakMessages(209, 0, 18);
                    //Speech(GetLocalizedPascalString("\pI'm very sorry, but you haven't the gold!"),18);
                    return;
                }
                if (Player[rosNum][41 + input] > 98) {
                    U3RenderPrintMessageRewrapped(260);
                    U3RenderPrintPascalString("\p\n\n");
                    U3AudioPlaySound(U3SoundEffectError1, true);
                    input = 'B';
                    goto shop4;
                }
                gold -= amount;
                Player[rosNum][35] = gold / 256;
                Player[rosNum][36] = gold - (Player[rosNum][35] * 256);
                Player[rosNum][41 + input]++;
                if (Player[rosNum][41 + input] > 99)
                    Player[rosNum][41 + input] = 99;
                U3RenderPrintMessage(210);
                U3AudioSpeakMessages(210, 0, 18);
                //Speech(GetLocalizedPascalString("\pHere you are, may it serve you well!"),18);
                input = 'B';
                goto shop4;
            } else {
                U3RenderPrintMessage(211);
                input = U3PlatformCursorKey(false);
                while (input > 95) {
                    input -= 32;
                }
                if (input < 'B' || input >= opnum)
                    goto shop4done;
                U3RenderPrintCharAt(input, tx, ty);
                input -= 'B';
                if (Player[rosNum][41 + input] < 1) {
                    U3RenderPrintMessage(212);
                    U3AudioSpeakMessages(212, 0, 18);
                    //Speech(GetLocalizedPascalString("\pYou don't own one of those!"),18);
                    return;
                }
                GetPascalStringFromArrayByIndex(gString, CFSTR("WeaponsArmour"), input + 41);
                StringToNum(gString, &amount);
                if (AddGold(rosNum, amount, FALSE)) {
                    Player[rosNum][41 + input]--;
                    if (Player[rosNum][41 + input] < 1 && Player[rosNum][40] == input + 1)
                        Player[rosNum][40] = 0;
                } else {
                    U3RenderPrintMessage(214);
                    U3AudioPlaySound(U3SoundEffectError1, true);
                    return;
                }
                U3RenderPrintMessage(213);
                U3AudioSpeakMessages(213, 0, 18);
                //Speech(GetLocalizedPascalString("\pThank you!"),18);
                input = 'S';
                goto shop4;
            }
        shop4done:
            U3RenderPrintMessageRewrapped(215);
            U3AudioSpeakMessages(215, 0, 18);
            //Speech(GetLocalizedPascalString("\pOh well, maybe next time."),18);
            return;
            break;
        case 5:
            opnum = 0;
        shop5:
            U3RenderPrintMessage(217);
            if (opnum == 0) {
                U3AudioSpeakMessages(217, 261, 23);
                //Speech(GetLocalizedPascalString("\pWe have keys, torches, powders, and gems."),23);
                opnum = 1;
            }
        shop5a:
            U3RenderPrintMessage(261);
            U3RenderPrintMessage(218);
            input = U3PlatformCursorKey(false);
            if (input > 95)
                input -= 32;
            short cost = 0, attrib, qty = 1;
            if (input == 'T') {
                cost = 30;
                attrib = 15;
                qty = 5;
            } else if (input == 'K') {
                cost = 50;
                attrib = 38;
            } else if (input == 'P') {
                cost = 90;
                attrib = 39;
            } else if (input == 'G') {
                cost = 75;
                attrib = 37;
            }
            if (cost == 0) {
                U3RenderPrintPascalString("\pN\n\n");
                U3RenderPrintMessageRewrapped(219);
                U3AudioSpeakMessages(219, 0, 23);
                //Speech(GetLocalizedPascalString("\pThank you, come again!"),23);
                return;
            }
            gString[0] = 1;
            gString[1] = input;
            U3RenderPrintPascalString(gString);
            U3RenderPrintPascalString("\p\n");
            if (Player[rosNum][attrib] + qty > 99) {
                U3RenderPrintMessageRewrapped(260);
                U3AudioPlaySound(U3SoundEffectError1, true);
                goto shop5a;
            }
            if (GuildPay(rosNum, cost) == 0) {
                GuildGive(rosNum, attrib, qty);
            } else {
                return;
            }
            U3RenderPrintMessage(220);
            U3AudioSpeakMessages(220, 0, 23);
            //Speech(GetLocalizedPascalString("\pAnything else?"),23);
            input = U3PlatformCursorKey(false);
            if (input > 95)
                input -= 32;
            if (input == 'Y')
                goto shop5;
            U3RenderPrintPascalString("\pN\n\n");
            U3RenderPrintMessage(219);
            U3AudioSpeakMessages(219, 0, 23);
            //Speech(GetLocalizedPascalString("\pThank you, come again!"),23);
            break;
        case 6:
            U3RenderPrintMessage(221);
        shop6:
            U3RenderPrintMessageRewrapped(222);
            U3AudioSpeakMessages(222, 0, 16);
            //Speech(GetLocalizedPascalString("\pHow many 100 gold is your offering?"),16);
            input = -1;
            while (gDone != TRUE && (input < 0 || input > 9)) {
                input = U3PlatformCursorKey(false) - '0';
            }
            U3RenderPrintCharAt(input + '0', wx, wy);
            U3RenderPrintPascalString("\p\n\n");
            input *= 100;
            gold = (Player[rosNum][35] * 256) + Player[rosNum][36];
            if (input > gold) {
                U3RenderPrintMessage(188);
                U3AudioSpeakMessages(188, 0, 16);
                //Speech(GetLocalizedPascalString("\pWhat? Can't pay? Out, you scum!"),16);
                U3AudioPlaySound(U3SoundEffectError1, true);
                return;
            }
            gold -= input;
            Player[rosNum][35] = gold / 256;
            Player[rosNum][36] = gold - (Player[rosNum][35] * 256);
            GetPascalStringFromArrayByIndex(message, CFSTR("Radrion"), (input / 100));
            if (!U3PlatformGetBooleanPreference(U3PreferenceClassicAppearance))
                RewrapString(message, false);
            U3RenderPrintPascalString(message);
            for (opnum = 0; opnum <= message[0]; opnum++) {
                spmessage[opnum] = message[opnum];
                if (message[opnum] > 'z')
                    spmessage[opnum] = ' ';
            }
            U3AudioSpeakPascalString(spmessage, 16);
            U3RenderPrintMessage(223);
            input = U3PlatformCursorKey(false);
            if (input > 95)
                input -= 32;
            if (input != 'Y') {
                U3RenderPrintPascalString("\pN\n");
                U3RenderPrintMessageRewrapped(224);
                U3AudioSpeakMessages(224, 0, 16);
                //Speech(GetLocalizedPascalString("\pFare thee well, and good luck!"),16);
                return;
            }
            U3RenderPrintPascalString("\pY\n");
            goto shop6;
            break;
        case 7:
            U3RenderPrintMessage(225);
            U3RenderPrintNumberAt(Party[2], wx, wy);
            wx++;
            U3RenderPrintMessage(226);
            opnum = Party[2] * 200;
            U3RenderPrintNumberAt(opnum, wx, wy);
            wx += 3;
            U3RenderPrintMessage(227);

            // Build up a string to speak the whole thing
            Str255 theString = "\p";
            theString[++theString[0]] = '0' + Party[2];
            GetPascalStringFromArrayByIndex(gString, CFSTR("Messages"), 226 - 1);
            AddString(theString, gString);
            NumToString(opnum, gString);
            AddString(theString, gString);
            GetPascalStringFromArrayByIndex(gString, CFSTR("Messages"), 227 - 1);
            AddString(theString, gString);
            SearchReplace(theString, "\pgp", "\p gold");
            U3AudioSpeakPascalString(theString, 63);

            input = U3PlatformCursorKey(false);
            if (input > 95)
                input -= 32;
            if (input != 'Y') {
                U3RenderPrintPascalString("\pN\n\n");
                U3RenderPrintMessageRewrapped(228);
                U3AudioSpeakMessages(228, 0, 63);
                //Speech(GetLocalizedPascalString("\pAh, too bad. These are the best in town!"),63);
                return;
            }
            gold = (Player[rosNum][35] * 256) + Player[rosNum][36];
            if (gold < opnum) {
                U3RenderPrintPascalString("\pY\n\n");
                U3RenderPrintMessageRewrapped(229);
                U3AudioSpeakMessages(229, 0, 63);
                //Speech(GetLocalizedPascalString("\pI'm sorry, but you haven't the gold!"),63);
                U3AudioPlaySound(U3SoundEffectError1, true);
                return;
            }
            gold -= opnum;
            Player[rosNum][35] = gold / 256;
            Player[rosNum][36] = gold - (Player[rosNum][35] * 256);
            U3RenderPrintPascalString("\pY\n\n");
            U3RenderPrintMessageRewrapped(230);
            U3AudioSpeakMessages(230, 0, 63);
            //Speech(GetLocalizedPascalString("\pMay you ride fast and true, friend!"),63);
            Party[1] = 0x14;
            DrawMap(xpos, ypos);
            return;
            break;
    }
}

Boolean GuildPay(short rosNum, short cost) {
    short gold;
    gold = (Player[rosNum][35] * 256) + Player[rosNum][36];
    if (gold < cost) {
        U3RenderPrintMessageRewrapped(231);
        U3AudioSpeakMessages(231, 0, 23);
        //Speech(GetLocalizedPascalString("\pI'm sorry, but you have not the funds!"),23);
        return TRUE;
    }
    gold -= cost;
    Player[rosNum][35] = gold / 256;
    Player[rosNum][36] = gold - (Player[rosNum][35] * 256);
    return FALSE;
}

void GuildGive(short rosNum, short item, short amount) {
    Player[rosNum][item] += amount;
    if (Player[rosNum][item] > 99)
        Player[rosNum][item] = 99;
}

short Clerical(short rosNum, short cost) {
    short input, result, gold;
    result = 0;
    input = U3PlatformCursorKey(false);
    if (input > 95)
        input -= 32;
    if (input != 'Y') {
        U3RenderPrintPascalString("\pN\n\n");
        U3RenderPrintMessageRewrapped(232);
        U3AudioSpeakMessages(232, 0, 9);
        //Speech(GetLocalizedPascalString("\pWithout proper offerings, I can not help!"),9);
        result = 1;
        return result;
    }
    gold = (Player[rosNum][35] * 256) + Player[rosNum][36];
    if (cost > gold) {
        U3RenderPrintPascalString("\pY\n\n");
        U3RenderPrintMessageRewrapped(233);
        U3AudioSpeakMessages(233, 0, 9);
        //Speech(GetLocalizedPascalString("\pI'm sorry, but thou hast not gold enough."),9);
        U3AudioPlaySound(U3SoundEffectError1, true);
        result = 1;
        return result;
    }
    gold -= cost;
    Player[rosNum][35] = gold / 256;
    Player[rosNum][36] = gold - (Player[rosNum][35] * 256);
    U3RenderPrintPascalString("\pY\n");
    return result;
}

void SpellNoize(short opnum) {
    InverseTiles();
    InverseChar(opnum);
    U3AudioPlaySound(U3SoundEffectHeal, false);    // was 0xF3, was Whine(0xC0,0x80);
    InverseChar(opnum);
    InverseTiles();
    U3RenderPrintMessageRewrapped(234);
    U3AudioSpeakMessages(234, 0, 9);
    //Speech(GetLocalizedPascalString("\pFare thee well, my children."),9);
}

void WeaponList(short lastitem) {
    U3RenderPrintMessage(237);
    PrintWeaponList(1);
    PrintWeaponList(2);
    PrintWeaponList(3);
    PrintWeaponList(4);
    PrintWeaponList(5);
    U3PlatformWaitKeyMouse();
    PrintWeaponList(6);
    PrintWeaponList(7);
    if (lastitem == 'P') {
        PrintWeaponList(8);
        PrintWeaponList(9);
        PrintWeaponList(10);
        PrintWeaponList(11);
        U3PlatformWaitKeyMouse();
        PrintWeaponList(12);
        PrintWeaponList(13);
        PrintWeaponList(14);
    }
    U3PlatformWaitKeyMouse();
}

void ArmourList(short lastitem) {
    U3RenderPrintMessage(237);
    PrintArmourList(1);
    PrintArmourList(2);
    PrintArmourList(3);
    PrintArmourList(4);
    U3PlatformWaitKeyMouse();
    if (lastitem == 'H') {
        PrintArmourList(5);
        PrintArmourList(6);
        U3PlatformWaitKeyMouse();
    }
}

void OtherCommand(short yell) {
    short chnum, rosNum, bytes, input, object, gold;
    Rect screct;
    RGBColor color;
    long time;
    unsigned char bits[8] = {1, 2, 4, 8, 16, 32, 64, 128};
    Str255 str;

    if (yell == 0) {
        YellStat = 0xFF;
        U3RenderPrintMessage(236);
    }
    chnum = GetChar();
    if (chnum < 1 || chnum > 4)
        return;
    if (CheckAlive(chnum - 1) == FALSE) {
        U3RenderPrintMessage(126);
        U3AudioPlaySound(U3SoundEffectError1, true);
        return;
    }
    rosNum = Party[6 + chnum];
    U3RenderPrintMessage(238);
    UInputText(tx, ty, str, 8, false);
    U3RenderPrintPascalString("\p\n");
    if (EqualString(str, "\pPAXUM", 0, 0) && Party[3] != 0) {
        U3RenderPrintMessage(254);
        for (chnum = 0; chnum < 32; chnum++) {
            if (Monsters[chnum]) {
                if (Monsters[chnum + XMON] > xpos - 6 && Monsters[chnum + XMON] < xpos + 6 && Monsters[chnum + YMON] > ypos - 6 &&
                    Monsters[chnum + YMON] < ypos + 6 && Monsters[chnum + HPMON] == 0xC0)
                    Monsters[chnum + HPMON] = 0x40;
            }
        }
        return;
    }
    if (EqualString(str, "\pSCREAM", 0, 0)) {
        U3RenderPrintPascalString("\p\nAIEEEEE!\n\n");
        if (Player[rosNum][24] == 'F')
            U3AudioPlaySound(U3SoundEffectDeathFemale, true);
        else
            U3AudioPlaySound(U3SoundEffectDeathMale, true);
        return;
    }
    if (EqualString(str, "\pINSERT", 0, 0)) {
        U3RenderPrintMessage(239);
        U3PlatformGetDirection(0);

        if (GetXYVal(xs, ys) != 0x7C) {
            NotHere();
            return;
        }
        U3RenderPrintMessage(240);
        input = GetKey();
        object = 0;
        if (input == 'L')
            object = 0x1E;
        if (input == 'S')
            object = 0x1F;
        if (input == 'M')
            object = 0x20;
        if (input == 'D')
            object = 0x21;
        if (object == 0) {
            What2();
            return;
        }
        bytes = bits[object - 0x1E];
        if ((Player[rosNum][14] & bytes) == 0) {
            U3RenderPrintMessage(67);
            return;
        }
        if (xs != object || object != lastCard) {
            InverseChar(chnum - 1);
            U3AudioPlaySound(U3SoundEffectHit, true);
            U3PlatformWaitTicks(15);
            InverseChar(chnum - 1);
            Player[rosNum][26] = 0;
            Player[rosNum][27] = 1;
            HPSubtract(rosNum, 255);
            return;
        }
        lastCard++;
        for (input = 4; input >= 0; input--) {
            gBallTileBackground = 0x3E;    // Exodus
            PutXYVal(0xF0, xs, ys);
            DrawMap(xpos, ypos);
            U3AudioPlaySound(U3SoundEffectHit, true);
            U3PlatformWaitTicks(15);
            PutXYVal(0x7C, xs, ys);
            DrawMap(xpos, ypos);
            U3AudioPlaySound(U3SoundEffectHit, true);
            U3PlatformWaitTicks(15);
        }
        PutXYVal(0x20, xs, ys);
        DrawMap(xpos, ypos);
        if (lastCard != 0x22)
            return;
        Party[16] = 1;

        Boolean classic = U3PlatformGetBooleanPreference(U3PreferenceClassicAppearance);
        gSongCurrent = gSongNext = 10;
        U3AudioUpdateMusic();
        //      U3RenderPrintMessage(241);
        if (classic)
            GetPascalStringFromArrayByIndex(str, CFSTR("Messages"), 240);
        //GetIndString(str, BASERES+12, 241);
        else {
            GetPascalStringFromArrayByIndex(str, CFSTR("Messages"), 254);
            // GetIndString(str, BASERES+12, 255);
            RewrapString(str, false);
        }
        U3RenderPrintPascalString(str);
        time = Party[11] + Party[12] * 100 + Party[13] * 10000 + Party[14] * 1000000;
        NumToString(time, str);
        if (!classic)
            U3RenderPrintPascalString("\p\n");
        U3RenderPrintPascalString(str);
        U3RenderPrintMessage(242);

        // Build up a string to speak the whole thing
        Str255 theString = "\p";
        GetPascalStringFromArrayByIndex(theString, CFSTR("Messages"), 254);
        AddString(theString, str);
        GetPascalStringFromArrayByIndex(gString, CFSTR("Messages"), 242 - 1);
        AddString(theString, gString);
        SearchReplace(theString, "\p:", "\p,");    // Exodus: Ultima III
        U3AudioSpeakPascalString(theString, 31);

        object = gUpdateWhere;
        gUpdateWhere = 0;
        for (input = 0; input <= 20; input++) {
            SetRect(&screct, blkSiz, blkSiz, blkSiz * 23, blkSiz * 23);
            color.red = Absolute(U3PlatformRandomRaw() * 2);
            color.green = Absolute(U3PlatformRandomRaw() * 2);
            color.blue = Absolute(U3PlatformRandomRaw() * 2);
            RGBForeColor(&color);
            BackColor(whiteColor);
            PenMode(addOver);
            PaintRect(&screct);
            ForceUpdateMain();
            U3AudioPlaySound(U3SoundEffectHit, false);    // was 0xF7
            U3PlatformWaitTicks(10);
        }
        PenMode(srcCopy);
        U3RenderClearTiles();
        for (input = 0; input < 256; input += 16) {
            WinText(input);
            U3PlatformWaitTicks(1);
            time = U3PlatformTickCount();
            while (time == U3PlatformTickCount()) {
                U3PlatformGetKeyMouse(0);
            }
        }
        U3PlatformWaitKeyMouse();
        for (input = 255; input >= 0; input -= 16) {
            WinText(input);
            time = U3PlatformTickCount();
            while (time == U3PlatformTickCount()) {
                U3PlatformGetKeyMouse(0);
            }
        }
        gUpdateWhere = object;
        SafeExodus();
        DrawMap(xpos, ypos);
        U3RenderPrintPascalString("\p\n");
        return;
    }
    if (EqualString(str, "\pDIG", 0, 0)) {
        if (Party[3] != 0) {
            NotHere();
            return;
        }
        if ((xpos == 0x21 && ypos == 0x03) || (xpos == 0x13 && ypos == 0x2C)) {
            if (xpos == 0x21)
                Player[rosNum][63] = 1;
            if (xpos == 0x13)
                Player[rosNum][47] = 1;
            U3RenderPrintMessage(243);
            return;
        }
        NotHere();
        return;
    }
    if (EqualString(str, "\pSearch", 0, 0)) {
        if (GetXYVal(xpos, ypos) != 0xF8) {
            NotHere();
            return;
        }
        Player[rosNum][14] = Player[rosNum][14] | bits[xpos & 0x03];
        U3RenderPrintMessageRewrapped(244);
        return;
    }
    if (str[0] == 5 && str[str[0]] == 'E') { /* Bribe */
        U3RenderPrintMessage(239);
        U3PlatformGetDirection(0);
        object = MonsterHere(xs, ys);
        if (object > 127) {
            NotHere();
            return;
        }
        gold = (Player[rosNum][35] * 256) + Player[rosNum][36];
        if (gold < 100) {
            U3RenderPrintMessage(245);
            U3AudioPlaySound(U3SoundEffectError1, true);
            return;
        }
        gold -= 100;
        Player[rosNum][35] = gold / 256;
        Player[rosNum][36] = gold - (Player[rosNum][35] * 256);
        if (Monsters[object] != 0x48) { /* Guard */
            NoEffect();
            return;
        }
        PutXYVal(Monsters[object + TILEON], Monsters[object + XMON], Monsters[object + YMON]);
        Monsters[object] = 0;
        return;
    }
    if (str[0] == 4 && str[str[0]] == 'Y') { /* Pray */
        if (Party[3] != 2) {
            NoEffect();
            return;
        }
        if (Party[4] != LocationX[4]) {
            NoEffect();
            return;
        }
        if (xpos != 0x30 || ypos != 0x30) {
            NoEffect();
            return;
        }
        U3RenderPrintMessage(246);
        U3AudioSpeakMessages(246, 0, 31);
        //Speech(GetLocalizedPascalString("\pYell ee voh care!"),31);
        return;
    }
    if (str[0] == 7 && str[str[0]] == 'E') { /* Evocare */
        Yell(1 + chnum);
        return;
    }
    if (EqualString(str, "\pPISSOFF", 0, 0)) {
        U3RenderPrintMessage(247);
        for (chnum = 0; chnum < 32; chnum++) {
            Monsters[chnum + HPMON] = 0xC0;
        }
        return;
    }
    NoEffect();
    return;
}

void SafeExodus(void) {
    short x, y;

    for (x = 0; x < 0x20; x++) {
        if (Monsters[x + HPMON] > 0) {
            PutXYVal(Monsters[x + TILEON], Monsters[x + XMON], Monsters[x + YMON]);
            Monsters[x + HPMON] = 0;
            Monsters[x] = 0;
        }
    }
    for (y = 1; y < 12; y++) {
        for (x = 0x1E; x < 0x22; x++) {
            PutXYVal(0x20, x, y);
        }
    }
}

void NoEffect(void) {
    U3RenderPrintMessage(248);
}

unsigned char GetXYTile(short x, short y) {
    if (x < 0 || x > 10 || y < 0 || y > 10)
        return 0;
    return TileArray[y * 11 + x];
}

void PutXYTile(short value, short x, short y) {
    if (x < 0 || x > 10 || y < 0 || y > 10)
        return;
    TileArray[y * 11 + x] = value;
}

short MapConstrain(short value) {
    while (value < 0) {
        value += gCurMapSize;
    }
    while (value >= gCurMapSize) {
        value -= gCurMapSize;
    }
    return value;
}
