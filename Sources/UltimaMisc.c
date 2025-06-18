// A lot of miscellaneous Ultima routines

#import "CarbonShunts.h"
#import "UltimaMisc.h"

#import "UltimaIncludes.h"
#import "CocoaBridge.h"
#import "UltimaDngn.h"
#import "UltimaGraphics.h"
#import "UltimaMacIF.h"
#import "UltimaMain.h"
#import "UltimaSound.h"
#import "UltimaSpellCombat.h"
#import "UltimaText.h"

extern Boolean          gDone, gResurrect;
extern unsigned char    Monsters[256], Talk[256], Player[21][65], Party[64];
extern char             gKeyPress;
extern unsigned char    TileArray[128], Dungeon[2048], gBallTileBackground;
extern unsigned char    MoonXTable[8], MoonYTable[8], careerTable[12], wpnUseTable[12];
extern unsigned char    armUseTable[12], LocationX[20], LocationY[20], Experience[17];
extern Handle           gParty, gRoster, gCurrentTlk, gDemoData;
extern char             WhirlDX, WhirlDY;
extern char             YellStat;
extern long             gTime[2], gMapOffset;
extern int              xpos, ypos, xs, ys, tx, ty, wx, wy, dx, dy;
extern short            WhirlX, WhirlY, gUpdateWhere, gMoon[2], gMoonDisp[2];
extern short            gTimeNegate, zp[255], lastCard;
extern short            gRosterRefNum, gMouseState, gCurMapID;
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
    while (dirgot == 0) {
        if (gDone == 1)
            dirgot = 1;
        direct = WaitKeyMouse();
        if (direct == '4')
            direct = 28;
        if (direct == '6')
            direct = 29;
        if (direct == '8')
            direct = 30;
        if (direct == '2')
            direct = 31;
        Boolean allowDiagonal = (!CFPreferencesGetAppBooleanValue(U3PrefNoDiagonals, kCFPreferencesCurrentApplication, NULL));
        switch (direct) {
            case '1':
                if (allowDiagonal) {
                    dirgot = 1;
                    xs = xpos - 1;
                    ys = ypos + 1;
                    dx = -1;
                    dy = 1;
                    UPrintMessage(250);
                }
                break;
            case '3':
                if (allowDiagonal) {
                    dirgot = 1;
                    xs = xpos + 1;
                    ys = ypos + 1;
                    dx = 1;
                    dy = 1;
                    UPrintMessage(251);
                }
                break;
            case '7':
                if (allowDiagonal) {
                    dirgot = 1;
                    xs = xpos - 1;
                    ys = ypos - 1;
                    dx = -1;
                    dy = -1;
                    UPrintMessage(252);
                }
                break;
            case '9':
                if (allowDiagonal) {
                    dirgot = 1;
                    xs = xpos + 1;
                    ys = ypos - 1;
                    dx = 1;
                    dy = -1;
                    UPrintMessage(253);
                }
                break;
            case 28:
                dirgot = 1;
                xs = xpos - 1;
                ys = ypos;
                dx = -1;
                dy = 0;
                UPrintMessage(27);
                break;
            case 29:
                dirgot = 1;
                xs = xpos + 1;
                ys = ypos;
                dx = 1;
                dy = 0;
                UPrintMessage(26);
                break;
            case 30:
                dirgot = 1;
                xs = xpos;
                ys = ypos - 1;
                dx = 0;
                dy = -1;
                UPrintMessage(24);
                break;
            case 31:
                dirgot = 1;
                xs = xpos;
                ys = ypos + 1;
                dx = 0;
                dy = 1;
                UPrintMessage(25);
                break;
            case ' ':
                if (mode == 1) {
                    dirgot = 1;
                    xs = xpos;
                    ys = ypos;
                    dx = 0;
                    dy = 0;
                    UPrintMessage(173);
                    break;
                }
            default: break;
        }
    }
    gMouseState = oldMouseState;
}

void GetMiscStuff(short id) {
    Handle tempHandle;
    unsigned short byte;

    tempHandle = GetResource('MISC', BASERES + id);
    LoadResource(tempHandle);
    for (byte = 0; byte < 8; byte++) {
        MoonXTable[byte] = *(*tempHandle + byte);
        MoonYTable[byte] = *(*tempHandle + byte + 8);
    }
    ReleaseResource(tempHandle);
    tempHandle = GetResource('MISC', BASERES + id + 1);
    LoadResource(tempHandle);
    for (byte = 0; byte < 12; byte++) {
        careerTable[byte] = *(*tempHandle + byte);
    }
    ReleaseResource(tempHandle);
    tempHandle = GetResource('MISC', BASERES + id + 2);
    LoadResource(tempHandle);
    for (byte = 0; byte < 12; byte++) {
        wpnUseTable[byte] = *(*tempHandle + byte);
    }
    ReleaseResource(tempHandle);
    tempHandle = GetResource('MISC', BASERES + id + 3);
    LoadResource(tempHandle);
    for (byte = 0; byte < 12; byte++) {
        armUseTable[byte] = *(*tempHandle + byte);
    }
    ReleaseResource(tempHandle);
    tempHandle = GetResource('MISC', BASERES + id + 4);
    LoadResource(tempHandle);
    for (byte = 0; byte < 20; byte++) {
        LocationX[byte] = *(*tempHandle + byte);
        LocationY[byte] = *(*tempHandle + byte + 32);
    }
    ReleaseResource(tempHandle);
    tempHandle = GetResource('MISC', BASERES + id + 5);
    LoadResource(tempHandle);
    for (byte = 0; byte < 17; byte++) {
        Experience[byte] = *(*tempHandle + byte);
    }
    ReleaseResource(tempHandle);
}

void PutMiscStuff(void) {
    Handle tempHandle;
    unsigned short byte;

    tempHandle = GetResource('MISC', BASERES + 100);
    LoadResource(tempHandle);
    for (byte = 0; byte < 8; byte++) {
        *(*tempHandle + byte) = MoonXTable[byte];
        *(*tempHandle + byte + 8) = MoonYTable[byte];
    }
    ChangedResource(tempHandle);
    WriteResource(tempHandle);
    ReleaseResource(tempHandle);
    tempHandle = GetResource('MISC', BASERES + 101);
    LoadResource(tempHandle);
    for (byte = 0; byte < 12; byte++) {
        *(*tempHandle + byte) = careerTable[byte];
    }
    ChangedResource(tempHandle);
    WriteResource(tempHandle);
    ReleaseResource(tempHandle);
    tempHandle = GetResource('MISC', BASERES + 102);
    LoadResource(tempHandle);
    for (byte = 0; byte < 12; byte++) {
        *(*tempHandle + byte) = wpnUseTable[byte];
    }
    ChangedResource(tempHandle);
    WriteResource(tempHandle);
    ReleaseResource(tempHandle);
    tempHandle = GetResource('MISC', BASERES + 103);
    LoadResource(tempHandle);
    for (byte = 0; byte < 12; byte++) {
        *(*tempHandle + byte) = armUseTable[byte];
    }
    ChangedResource(tempHandle);
    WriteResource(tempHandle);
    ReleaseResource(tempHandle);
    tempHandle = GetResource('MISC', BASERES + 104);
    LoadResource(tempHandle);
    for (byte = 0; byte < 20; byte++) {
        *(*tempHandle + byte) = LocationX[byte];
        *(*tempHandle + byte + 32) = LocationY[byte];
    }
    ChangedResource(tempHandle);
    WriteResource(tempHandle);
    ReleaseResource(tempHandle);
    tempHandle = GetResource('MISC', BASERES + 105);
    LoadResource(tempHandle);
    for (byte = 0; byte < 17; byte++) {
        *(*tempHandle + byte) = Experience[byte];
    }
    ChangedResource(tempHandle);
    WriteResource(tempHandle);
    ReleaseResource(tempHandle);
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
        PlaySoundFile(CFSTR("ExpLevelUp"), TRUE);
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
    if (RandNum(0, 255) > factor)
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
    gDemoData = GetResource('DEMO', BASERES);
    LoadResource(gDemoData);
    for (ptr = 0; ptr < 127; ptr++) {
        TileArray[ptr] = *(*gDemoData + ptr + 1024);
    }
}

void OpenRstr(void) {
    Handle dummyHandle;
    Str255 pathStr;
    long error;
    short byte;
    Boolean gotOpenEvent;
    FSSpec fss;

    gotOpenEvent = FALSE;

    if (!gotOpenEvent) {
        short prefVRefNum;
        long prefDirID;
        error = FindFolder(kOnSystemDisk, kPreferencesFolderType, kDontCreateFolder, &prefVRefNum, &prefDirID);
        if (error)
            HandleError(error, 38, 0);
        GetIndString(pathStr, BASERES + 11, 2);    // Ultima III Roster
        error = FSMakeFSSpec(prefVRefNum, prefDirID, pathStr, &fss);
    }
    if (error != noErr && error != fnfErr)
        HandleError(error, 38, 1);

    if (error == noErr) {
        Boolean isAlias, isFolder;
        OSErr err = IsAliasFile(&fss, &isAlias, &isFolder);
        if (err == noErr && isAlias) {
            ResolveAliasFile(&fss, true, &isFolder, &isAlias);
        }
        gRosterRefNum = FSpOpenResFile(&fss, fsRdWrPerm);
        if (gRosterRefNum == -1)
            HandleError(ResError(), 39, 0);
    }

    if (error == fnfErr) {   // file not found, so create "Ultima III Roster"
        // this only works if Roster is next to the executable within the bundle!
        FSpDelete(&fss);
        FSpCreateResFile(&fss, 'Ult3', 'RSTR', smSystemScript);
        if (ResError()) {
            HandleError(ResError(), 41, 0);
        }
        gRosterRefNum = FSpOpenResFile(&fss, fsRdWrPerm);
        if (gRosterRefNum == -1) {
            HandleError(ResError(), 42, gRosterRefNum);
        }
        // If they have an old "Roster" next to the app, just copy it.
        GetIndString(pathStr, BASERES + 11, 15);    // old ":Roster"
        FSSpec oldRosterFSS;
        error = FSMakeFSSpec(0, 0, pathStr, &oldRosterFSS);
        if (error == noErr) {    // copy the old :Roster into the prefs.
            error = permErr;
            int oldRosterRefNum = FSpOpenResFile(&oldRosterFSS, fsRdPerm);
            if (oldRosterRefNum != -1) {
                long oldNumBytes;
                error = GetEOF(oldRosterRefNum, &oldNumBytes);
                if (error == noErr && oldNumBytes > 0) {
                    Ptr buffer = NewPtr(oldNumBytes);
                    if (buffer) {
                        error = FSReadFork(oldRosterRefNum, fsFromStart, 0, oldNumBytes, buffer, NULL);
                        if (error == noErr) {
                            FSWriteFork(gRosterRefNum, fsFromStart, 0, oldNumBytes, buffer, NULL);
                        }
                        DisposePtr(buffer);
                    }
                }
                error = FSClose(oldRosterRefNum);
            }
            FSClose(gRosterRefNum);
            gRosterRefNum = FSpOpenResFile(&fss, fsRdWrPerm);
        }
        if (error != noErr) {
            // No existing Ultima III Roster in Preferences, and no
            // Roster next to the app.  Create the new roster from scratch.
            dummyHandle = NewHandleClear(256);
            AddResource(dummyHandle, (ResType)'MONS', BASERES + 19, "\pSosaria Monsters");
            if (ResError())
                HandleError(ResError(), 46, 0);
            ChangedResource(dummyHandle);
            dummyHandle = NewHandleClear(32);
            AddResource(dummyHandle, (ResType)'PREF', BASERES, "\pPreferences");
            if (ResError())
                HandleError(ResError(), 47, 0);
            for (byte = 0; byte < 8; byte++) {
                (*dummyHandle)[byte] = 1;
            }
            ChangedResource(dummyHandle);
            UseResFile(gRosterRefNum);

            // the new way, copy MAPS 420->419 and PRTY ROST 500->400
            dummyHandle = GetResource((ResType)'MAPS', BASERES + 20);
            DetachResource(dummyHandle);
            AddResource(dummyHandle, (ResType)'MAPS', BASERES + 19, "\pSosaria Current");
            if (ResError())
                HandleError(ResError(), 45, BASERES + 19);
            ChangedResource(dummyHandle);

            dummyHandle = GetResource((ResType)'PRTY', BASERES + 100);
            DetachResource(dummyHandle);
            AddResource(dummyHandle, (ResType)'PRTY', BASERES, "\pParty");
            if (ResError())
                HandleError(ResError(), 43, BASERES);
            ChangedResource(dummyHandle);

            dummyHandle = GetResource((ResType)'ROST', BASERES + 100);
            DetachResource(dummyHandle);
            AddResource(dummyHandle, (ResType)'ROST', BASERES, "\pRoster");
            if (ResError())
                HandleError(ResError(), 44, BASERES);
            ChangedResource(dummyHandle);

            dummyHandle = GetResource((ResType)'MISC', BASERES);
            DetachResource(dummyHandle);
            AddResource(dummyHandle, (ResType)'MISC', BASERES + 100, "\pMoongate Locations");
            if (ResError())
                HandleError(ResError(), 48, BASERES + 100);
            ChangedResource(dummyHandle);
            dummyHandle = GetResource((ResType)'MISC', BASERES + 1);
            DetachResource(dummyHandle);
            AddResource(dummyHandle, (ResType)'MISC', BASERES + 101, "\pType Initial Table");
            if (ResError())
                HandleError(ResError(), 48, BASERES + 101);
            ChangedResource(dummyHandle);
            dummyHandle = GetResource((ResType)'MISC', BASERES + 2);
            DetachResource(dummyHandle);
            AddResource(dummyHandle, (ResType)'MISC', BASERES + 102, "\pWeapon Use By class");
            if (ResError())
                HandleError(ResError(), 48, BASERES + 102);
            ChangedResource(dummyHandle);
            dummyHandle = GetResource((ResType)'MISC', BASERES + 3);
            DetachResource(dummyHandle);
            AddResource(dummyHandle, (ResType)'MISC', BASERES + 103, "\pArmour Use By class");
            if (ResError())
                HandleError(ResError(), 48, BASERES + 103);
            ChangedResource(dummyHandle);
            dummyHandle = GetResource((ResType)'MISC', BASERES + 4);
            DetachResource(dummyHandle);
            AddResource(dummyHandle, (ResType)'MISC', BASERES + 104, "\pLocation Table");
            if (ResError())
                HandleError(ResError(), 48, BASERES + 104);
            ChangedResource(dummyHandle);
            dummyHandle = GetResource((ResType)'MISC', BASERES + 5);
            DetachResource(dummyHandle);
            AddResource(dummyHandle, (ResType)'MISC', BASERES + 105, "\pExperience Table");
            if (ResError())
                HandleError(ResError(), 48, BASERES + 105);
            ChangedResource(dummyHandle);
        }
        UpdateResFile(gRosterRefNum);
    }    // new Roster creation finished
}

void GetRoster(void) {
    short player, byte;

    gRoster = GetResource('ROST', BASERES);
    LoadResource(gRoster);
    for (player = 0; player < 20; player++) {
        for (byte = 0; byte < 64; byte++) {
            Player[player + 1][byte] = (unsigned char)*(*gRoster + ((player)*64) + byte);
        }
    }
}

void PutRoster(void) {
    short player, byte;

    for (player = 0; player < 20; player++) {
        for (byte = 0; byte < 64; byte++) {
            *(*gRoster + ((player)*64) + byte) = Player[player + 1][byte];
        }
    }
    ChangedResource(gRoster);
    WriteResource(gRoster);
}

void GetParty(void) {
    short byte;

    gParty = GetResource('PRTY', BASERES);
    LoadResource(gParty);
    for (byte = 0; byte < 64; byte++) {
        Party[byte + 1] = *(*gParty + byte);
    }
    xpos = Party[4];
    ypos = Party[5];
}

void PutParty(void) {
    short byte;

    if (Party[3] == 0) {
        for (byte = 0; byte < 64; byte++) {
            *(*gParty + byte) = Party[byte + 1];
        }
        ChangedResource(gParty);
        WriteResource(gParty);
    }
}

void ResetSosaria(void) {
    short byte;
    Handle OrgSos, CurSos;
    long mapLength;

    OrgSos = GetResource('MAPS', BASERES + 20);
    CurSos = GetResource('MAPS', BASERES + 19);
    gCurMapSize = *(*OrgSos);
    if (gCurMapSize == 0)
        gCurMapSize = 256;
    mapLength = (gCurMapSize * gCurMapSize) + 5;
    SetHandleSize(CurSos, (Size)mapLength);
    BlockMoveData(*OrgSos, *CurSos, (Size)mapLength);
    //  for (byte=0; byte<mapLength; byte++)
    //      {
    //      *(*CurSos+byte)=*(*OrgSos+byte);
    //      }
    ChangedResource(CurSos);
    WriteResource(CurSos);
    ReleaseResource(OrgSos);
    ReleaseResource(CurSos);

    OrgSos = GetResource('MONS', BASERES + 20);
    CurSos = GetResource('MONS', BASERES + 19);
    for (byte = 0; byte < 256; byte++) {
        *(*CurSos + byte) = *(*OrgSos + byte);
    }
    ChangedResource(CurSos);
    WriteResource(CurSos);
    ReleaseResource(OrgSos);
    ReleaseResource(CurSos);

    OrgSos = GetResource((ResType)'MISC', BASERES);
    CurSos = GetResource((ResType)'MISC', BASERES + 100);
    for (byte = 0; byte < 16; byte++) {
        *(*CurSos + byte) = *(*OrgSos + byte);
    }
    ChangedResource(CurSos);
    WriteResource(CurSos);
    ReleaseResource(OrgSos);
    ReleaseResource(CurSos);
    DetachResource(OrgSos);

    OrgSos = GetResource((ResType)'MISC', BASERES + 4);
    CurSos = GetResource((ResType)'MISC', BASERES + 104);
    for (byte = 0; byte < 64; byte++) {
        *(*CurSos + byte) = *(*OrgSos + byte);
    }
    ChangedResource(CurSos);
    WriteResource(CurSos);
    ReleaseResource(OrgSos);
    ReleaseResource(CurSos);
    DetachResource(OrgSos);
}

void GetSosaria(void) {
    LoadUltimaMap(BASERES + 19);
    BlockExodus();
}

void BlockExodus(void) {
    if (Party[3] != 0 || gUpdateWhere != 3)
        return;    // if not Sosaria
    if (GetXYVal(0x0A, 0x35) == 0x1C && GetXYVal(0x0B, 0x36) == 0x00 && GetXYVal(0x0C, 0x35) == 0x10) {
        if (CFPreferencesGetAppBooleanValue(U3PrefNoDiagonals, kCFPreferencesCurrentApplication, NULL)) {
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
    Handle gCurrentMap, gCurrentMon;

    if (Party[3] != 0)
        return;    // I shouldn't need this, dammit!
    mapLength = gCurMapSize * gCurMapSize;
    gCurrentMap = GetResource('MAPS', BASERES + 19);
    *(*gCurrentMap) = (unsigned char)(gCurMapSize & 0xFF);
    for (mark = 0; mark < mapLength; mark++) {
        *(*gCurrentMap + mark + 1) = *(*Map + mark);
    }
    *(*gCurrentMap + mapLength + 1) = (unsigned char)(WhirlX & 0xFF);
    *(*gCurrentMap + mapLength + 2) = (unsigned char)(WhirlY & 0xFF);
    *(*gCurrentMap + mapLength + 3) = (unsigned char)(WhirlDX & 0xFF);
    *(*gCurrentMap + mapLength + 4) = (unsigned char)(WhirlDY & 0xFF);
    ChangedResource(gCurrentMap);
    WriteResource(gCurrentMap);
    ReleaseResource(gCurrentMap);
    gCurrentMon = GetResource('MONS', BASERES + 19);
    for (mark = 0; mark < 256; mark++) {
        *(*gCurrentMon + mark) = Monsters[mark];
    }
    ChangedResource(gCurrentMon);
    WriteResource(gCurrentMon);
    ReleaseResource(gCurrentMon);
    PutMiscStuff();
}

void LoadUltimaMap(short resid) {
    long mark;
    long mapLength;
    Handle gCurrentMap, gCurrentMon, resizeMap;

    gCurMapID = resid;
    gCurrentMap = GetResource('MAPS', resid);
    LoadResource(gCurrentMap);
    if (resid > 411 && resid < 419) {
        for (mark = 0; mark < 2048; mark++) {
            Dungeon[mark] = *(*gCurrentMap + mark);
        }
        ReleaseResource(gCurrentMap);
    } else {
        if (resid == 419 && GetHandleSize(gCurrentMap) == 4100) {
            resizeMap = NewHandle(4100);
            if (MemError())
                HandleError(MemError(), 61, 3);
            BlockMoveData(*gCurrentMap, *resizeMap, 4100);
            SetHandleSize(gCurrentMap, 4101);
            *(*gCurrentMap) = 64;
            BlockMoveData(*resizeMap, (*gCurrentMap) + 1, 4100);
            DisposeHandle(resizeMap);
            ChangedResource(gCurrentMap);
            WriteResource(gCurrentMap);
        }
        gCurMapSize = (unsigned char)*(*gCurrentMap);
        if (gCurMapSize == 0)
            gCurMapSize = 256;
        mapLength = gCurMapSize * gCurMapSize;
        if (GetHandleSize(Map) > 0)
            DisposeHandle(Map);
        Map = NewHandle(mapLength);
        if (MemError())
            HandleError(MemError(), 61, 1);
        for (mark = 0; mark < mapLength; mark++) {
            (*Map)[mark] = *(*gCurrentMap + mark + 1);
        }
        gCurrentMon = GetResource('MONS', resid);
        LoadResource(gCurrentMon);
        for (mark = 0; mark < 256; mark++) {
            Monsters[mark] = *(*gCurrentMon + mark);
        }
    }
    if (resid < 419 || resid == 421) {   // "<419" *was* "<420", big mistake.
        gCurrentTlk = GetResource('TLKS', resid);
        LoadResource(gCurrentTlk);
        for (mark = 0; mark < 256; mark++) {
            Talk[mark] = *(*gCurrentTlk + mark);
        }
        ReleaseResource(gCurrentTlk);
    }
    if (resid == (BASERES + 19)) {   // Sosaria
        WhirlX = *(*gCurrentMap + mapLength + 1);
        WhirlY = *(*gCurrentMap + mapLength + 2);
        WhirlDX = *(*gCurrentMap + mapLength + 3);
        WhirlDY = *(*gCurrentMap + mapLength + 4);
    }
    ReleaseResource(gCurrentMap);
    ReleaseResource(gCurrentMon);
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
    Boolean classic = CFPreferencesGetAppBooleanValue(U3PrefClassicAppearance, kCFPreferencesCurrentApplication, NULL);
    if (classic) {
        UPrintMessage(235);
        for (key = 0; key < ((16 - attributeName[0]) / 2); key++) {
            UPrintWin("\p ");
        }
        UPrintWin(attributeName);
        UPrintWin("\p\n\n");
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
        UPrintWin(gString);

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

    Speech(gString, voiceTile);
    UPrintMessage(178);
    while ((key < '0' || key > '9') && (gDone == FALSE)) {
        GetKeyMouse(0);
        key = gKeyPress;
    }
    UPrintChar(key, tx, ty);
    key -= '0';
    if (key == 0) {
        UPrintMessage(179);
        ImageGoAway();
        return;
    }
    gold = ((Player[rosNum][35]) * 256) + Player[rosNum][36];
    if (gold - (key * 100) < 0) {
        UPrintMessage(180);
        PlaySoundFile(CFSTR("Error1"), FALSE);
        ImageGoAway();
        return;
    }
    gold -= (key * 100);
    Player[rosNum][35] = gold / 256;
    Player[rosNum][36] = gold - (Player[rosNum][35] * 256);
    UPrintMessage(181);
    InverseChar(chnum - 1);
    InverseTiles();
    PlaySoundFile(CFSTR("Shrine"), FALSE);    // was 0xF0
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
    if (RandNum(0, 134) < 128)
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
    type = RandNum(0, 12) & RandNum(0, 12);
    if (allFirst)
        type = RandNum(0, 2);    // then only thiefs/orx/skeletons
    Monsters[offset] = MonTypes[type] * 4;
    Monsters[offset + TILEON] = MonBegin[type];
    Monsters[offset + XMON] = RandNum(0, gCurMapSize - 1);
    if (Monsters[offset + XMON] == xpos) {
        Monsters[offset] = 0;
        goto spawn;
    }
    Monsters[offset + YMON] = RandNum(0, gCurMapSize - 1);
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
    if (RandNum(0, 1))
        var = RandNum(1, 2);
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
        if (RandNum(0, 255) > 127)
            goto movemon;
        GetMonsterDir(offset);
        xs = 5 - zp[0xF5];
        if (xs > 10 || xs < 0)
            goto movemon;
        ys = 5 - zp[0xF6];
        if (ys > 10 || ys < 0)
            goto movemon;
        DrawMap(xpos, ypos);
        PlaySoundFile(CFSTR("Shoot"), TRUE);    // was 0xEA
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
            if (RandNum(0, 255) < 128)
                goto movemon;
            xs = MapConstrain(Monsters[offset + XMON] + GetHeading(RandNum(0, 255)));
            if (xs == 0)
                goto movemon;
            ys = MapConstrain(Monsters[offset + YMON] + GetHeading(RandNum(0, 255)));
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
                            PlaySoundFile(CFSTR("Hit"), FALSE);    // was 0xF7
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
            UPrintMessage(182);
        //      UPrintWin("\pPLEASE WAIT...\n");
        Party[4] = xpos;
        Party[5] = ypos;
        if (CFPreferencesGetAppBooleanValue(U3PrefAutoSave, kCFPreferencesCurrentApplication, NULL)) {
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
        xs = RandNum(0, 11);
        ys = RandNum(0, 11);
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
                PlaySoundFile(CFSTR("Hit"), FALSE);    // was 0xF7
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
                    UPrintMessage(183);
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
                UPrintMessage(184);
                InverseChar(member);
                PlaySoundFile(CFSTR("Hit"), FALSE);    // was 0xF7
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
    if (CFPreferencesGetAppBooleanValue(U3PrefClassicAppearance, kCFPreferencesCurrentApplication, NULL)) {
        UPrint("\p(", 9, 0);
        UPrintChar(gMoonDisp[0], tx, ty);
        UPrint("\p)(", 11, 0);
        UPrintChar(gMoonDisp[1], tx, ty);
        UPrint("\p)", 14, 0);
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
            xpos = RandNum(0, gCurMapSize - 1);
            ypos = RandNum(0, gCurMapSize - 1);
            value = GetXYVal(xpos, ypos);
        }
    }
    DrawMap(oldx, oldy);
    InverseTiles();
    ForceUpdateMain();
    PlaySoundFile(CFSTR("Moongate"), FALSE);    // was 0xF1
    InverseTiles();
    DrawMap(xpos, ypos);
    InverseTiles();
    ForceUpdateMain();
    PlaySoundFile(CFSTR("Moongate"), FALSE);    // was 0xF1
    InverseTiles();
}

void Shop(short shopNum, short chnum) {
    short rosNum, input, gold, opnum;
    Str255 message, spmessage;
    long amount;

    rosNum = Party[6 + chnum];
    switch (shopNum) {
        case 0:
            UPrintMessageRewrapped(185);
        shop0:
            UPrintMessage(186);
            SpeakMessages(186, 0, 20);
            //Speech(GetLocalizedPascalString("\pHere friend, have a drink! It costs 7 gold."),20);
            input = UInputNum(tx, ty);
            UPrintWin("\p\n\n");
            if (input < 7) {
                UPrintMessageRewrapped(187);
                SpeakMessages(187, 0, 20);
                //Speech(GetLocalizedPascalString("\pLeave my shop! You scum!"),20);
                ErrorTone();
                return;
            }
            gold = (Player[rosNum][35] * 256) + Player[rosNum][36];
            if (gold < input) {
                UPrintMessageRewrapped(188);
                SpeakMessages(188, 0, 20);
                //Speech(GetLocalizedPascalString("\pWhat? Can't pay? Out, you scum!"),20);
                ErrorTone();
                return;
            }
            gold -= input;
            Player[rosNum][35] = gold / 256;
            Player[rosNum][36] = gold - (Player[rosNum][35] * 256);
            GetPascalStringFromArrayByIndex(message, CFSTR("Pub"), (input / 10));
            if (!CFPreferencesGetAppBooleanValue(U3PrefClassicAppearance, kCFPreferencesCurrentApplication, NULL))
                RewrapString(message, false);
            UPrintWin(message);
            for (opnum = 0; opnum <= message[0]; opnum++) {
                spmessage[opnum] = message[opnum];
                if (message[opnum] > 'z')
                    spmessage[opnum] = ' ';
            }
            Speech(message, 20);
            UPrintMessage(189);
            //Speech("\pAnother drink?",20);
            input = CursorKey(false);
            if (input > 95)
                input -= 32;
            if (input != 'Y') {
                UPrintWin("\pN");
                UPrintMessageRewrapped(190);
                SpeakMessages(190, 0, 20);
                //Speech(GetLocalizedPascalString("\pIts been a pleasure!"),20);
                return;
            }
            UPrintWin("\pY");
            goto shop0;
            break;
        case 1:
            UPrintMessage(191);
        shop1:
            UPrintMessage(192);
            SpeakMessages(192, 0, 16);
            //Speech(GetLocalizedPascalString("\pRations 1 gold each, how many would you like?"),16);
            input = UInputBigNum(tx, ty);
            if (input == 0) {
                UPrintWin("\p\n\n");
                return;
            }
            int existingFood = (Player[rosNum][32] * 100) + Player[rosNum][33];
            if (input > (9999 - existingFood)) {
                UPrintMessageRewrapped(260);
                UPrintWin("\p\n\n");
                ErrorTone();
                goto shop1;
            }
            gold = (Player[rosNum][35] * 256) + Player[rosNum][36];
            if (gold < input) {
                UPrintMessageRewrapped(193);
                SpeakMessages(193, 0, 16);
                //Speech(GetLocalizedPascalString("\pWhat? Can't pay? Out, you scum!"),16);
                ErrorTone();
                return;
            }
            gold -= input;
            Player[rosNum][35] = gold / 256;
            Player[rosNum][36] = gold - (Player[rosNum][35] * 256);
            existingFood += input;
            Player[rosNum][32] = existingFood / 100;
            Player[rosNum][33] = existingFood - (Player[rosNum][32] * 100);
            ShowChars(false);
            UPrintMessageRewrapped(194);
            SpeakMessages(194, 0, 16);
            //Speech(GetLocalizedPascalString("\pThank you, anything else?"),16);
            input = CursorKey(false);
            if (input > 95)
                input -= 32;
            if (input != 'Y') {
                UPrintWin("\pN\n\n");
                UPrintMessageRewrapped(195);
                SpeakMessages(195, 0, 16);
                //Speech(GetLocalizedPascalString("\pVery well, come again!"),16);
                return;
            }
            UPrintWin("\pY\n\n");
            goto shop1;
            break;
        case 2:
            UPrintMessage(196);
            UPrintMessage(197);
            SpeakMessages(197, 0, 9);
            //Speech(GetLocalizedPascalString("\pYour needs?"),9);
            input = 0;
            while ((input < '0' || input > '4') && input != ' ') {
                input = CursorKey(false);
            }
            if (input == ' ') {
                input = 0;
            } else {
                input -= '0';
            }
            switch (input) {
                case 0: UPrintWin("\p0\n"); break;
                case 1:
                    UPrintMessage(198);
                    SpeakMessages(198, 0, 9);
                    //Speech(GetLocalizedPascalString("\pA curing will cost 100 gold.  Wilt thou pay?"),9);
                    if (Clerical(rosNum, 100) == 0) {
                        UPrintMessage(199);
                        opnum = GetChar();
                        UPrintWin("\p\n");
                        if (opnum >= 1 && opnum <= 4) {
                            SpellNoize(opnum - 1);
                            if (Player[Party[6 + opnum]][17] == 'P')
                                Player[Party[6 + opnum]][17] = 'G';
                        } else {
                            PlaySoundFile(CFSTR("Bump"), TRUE);
                        }    // was 0xE7
                    }
                    break;
                case 2:
                    UPrintMessage(200);
                    SpeakMessages(200, 0, 9);
                    //Speech(GetLocalizedPascalString("\pHealings cost 200 gold.  Wilt thou pay?"),9);
                    if (Clerical(rosNum, 200) == 0) {
                        UPrintMessage(201);
                        opnum = GetChar();
                        UPrintWin("\p\n");
                        if (opnum >= 1 && opnum <= 4) {
                            SpellNoize(opnum - 1);
                            Player[Party[6 + opnum]][26] = Player[Party[6 + opnum]][28];
                            Player[Party[6 + opnum]][27] = Player[Party[6 + opnum]][29];
                        } else {
                            PlaySoundFile(CFSTR("Bump"), TRUE);
                        }    // was 0xE7
                    }
                    break;
                case 3:
                    UPrintMessage(202);
                    SpeakMessages(202, 0, 9);
                    //Speech(GetLocalizedPascalString("\pResurrections cost 500 gold.  Wilt thou pay?"),9);
                    if (Clerical(rosNum, 500) == 0) {
                        UPrintMessage(203);
                        opnum = GetChar();
                        UPrintWin("\p\n");
                        if (opnum >= 1 && opnum <= 4) {
                            SpellNoize(opnum - 1);
                            if (Player[Party[6 + opnum]][17] == 'D')
                                Player[Party[6 + opnum]][17] = 'G';
                        } else {
                            PlaySoundFile(CFSTR("Bump"), TRUE);
                        }    // was 0xE7
                    }
                    break;
                case 4:
                    UPrintMessage(204);
                    SpeakMessages(204, 0, 9);
                    //Speech(GetLocalizedPascalString("\pRecallings cost 900 gold.  Wilt thou pay?"),9);
                    if (Clerical(rosNum, 900) == 0) {
                        UPrintMessage(205);
                        opnum = GetChar();
                        UPrintWin("\p\n");
                        if (opnum >= 1 && opnum <= 4) {
                            SpellNoize(opnum - 1);
                            if (Player[Party[6 + opnum]][17] == 'A')
                                Player[Party[6 + opnum]][17] = 'G';
                        } else {
                            PlaySoundFile(CFSTR("Bump"), TRUE);
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
            UPrintMessage(206);
            SpeakMessages(206, 0, 16);
            //Speech(GetLocalizedPascalString("\pWelcome to the weapons shop!"),16);
            input = CursorKey(false);
            while (input > 95) {
                input -= 32;
            }
            UPrintChar(input, tx, ty);
            if (input == 'Y')
                WeaponList(opnum);
            UPrintMessage(207);
            SpeakMessages(207, 0, 16);
            //Speech(GetLocalizedPascalString("\pBuy or sell?"),16);
            input = CursorKey(false);
            while (input > 95) {
                input -= 32;
            }
            UPrintChar(input, tx, ty);
        shop3:
            if (input == 'B') {
                UPrintMessage(208);
                input = CursorKey(false);
                while (input > 95) {
                    input -= 32;
                }
                if (input < 'B' || input >= opnum)
                    goto shop3done;
                UPrintChar(input, tx, ty);
                input -= 'B';
                gold = (Player[rosNum][35] * 256) + Player[rosNum][36];
                GetPascalStringFromArrayByIndex(gString, CFSTR("WeaponsArmour"), input + 25);
                StringToNum(gString, &amount);
                if (amount > gold) {
                    UPrintMessageRewrapped(209);
                    SpeakMessages(209, 0, 16);
                    //Speech(GetLocalizedPascalString("\pI'm very sorry, but you haven't the gold!"),16);
                    return;
                }
                if (Player[rosNum][49 + input] > 98) {
                    UPrintMessageRewrapped(260);
                    UPrintWin("\p\n\n");
                    ErrorTone();
                    input += 'B';
                    goto shop3;
                }
                gold -= amount;
                Player[rosNum][35] = gold / 256;
                Player[rosNum][36] = gold - (Player[rosNum][35] * 256);
                Player[rosNum][49 + input]++;
                if (Player[rosNum][49 + input] > 99)
                    Player[rosNum][49 + input] = 99;
                UPrintMessageRewrapped(210);
                SpeakMessages(210, 0, 16);
                //Speech(GetLocalizedPascalString("\pHere you are, may it serve you well!"),16);
                input = 'B';
                goto shop3;
            } else {
                UPrintMessage(211);
                input = CursorKey(false);
                while (input > 95) {
                    input -= 32;
                }
                if (input < 'A' || input >= opnum)
                    goto shop3done;
                if (input == 'A') {
                    DoStandardAlert(kAlertNoteAlert, 9);
                    goto shop3done;
                }
                UPrintChar(input, tx, ty);
                input -= 'B';
                if (Player[rosNum][49 + input] < 1) {
                    UPrintMessageRewrapped(212);
                    SpeakMessages(212, 0, 16);
                    //Speech(GetLocalizedPascalString("\pYou don't own one of those!"),16);
                    return;
                }
                GetPascalStringFromArrayByIndex(gString, CFSTR("WeaponsArmour"), input + 25);
                StringToNum(gString, &amount);
                if (AddGold(rosNum, amount, FALSE)) {
                    Player[rosNum][49 + input]--;
                    if (Player[rosNum][49 + input] < 1 && Player[rosNum][48] == input + 1)
                        Player[rosNum][48] = 0;
                    UPrintMessage(213);
                    SpeakMessages(213, 0, 16);
                    //Speech(GetLocalizedPascalString("\pThank you!"),16);
                } else {
                    UPrintMessageRewrapped(214);
                    ErrorTone();
                    return;
                }
                input = 'S';
                goto shop3;
            }
        shop3done:
            UPrintMessageRewrapped(215);
            SpeakMessages(215, 0, 16);
            //Speech(GetLocalizedPascalString("\pOh well, maybe next time!"),16);
            return;
            break;
        case 4:
            opnum = 'F';
            if (Party[4] == 37)
                opnum = 'H';
            UPrintMessage(216);
            SpeakMessages(216, 0, 18);
            //Speech(GetLocalizedPascalString("\pWelcome to the armor shop!"),18);
            input = CursorKey(false);
            while (input > 95) {
                input -= 32;
            }
            UPrintChar(input, tx, ty);
            if (input == 'Y')
                ArmourList(opnum);
            UPrintMessage(207);
            SpeakMessages(207, 0, 18);
            //Speech(GetLocalizedPascalString("\pBuy or sell?"),18);
            input = CursorKey(false);
            while (input > 95) {
                input -= 32;
            }
            UPrintChar(input, tx, ty);
        shop4:
            if (input == 'B') {
                UPrintMessage(208);
                input = CursorKey(false);
                while (input > 95) {
                    input -= 32;
                }
                if (input < 'B' || input >= opnum)
                    goto shop4done;
                UPrintChar(input, tx, ty);
                input -= 'B';
                gold = (Player[rosNum][35] * 256) + Player[rosNum][36];
                GetPascalStringFromArrayByIndex(gString, CFSTR("WeaponsArmour"), input + 41);
                StringToNum(gString, &amount);
                if (amount > gold) {
                    UPrintMessage(209);
                    SpeakMessages(209, 0, 18);
                    //Speech(GetLocalizedPascalString("\pI'm very sorry, but you haven't the gold!"),18);
                    return;
                }
                if (Player[rosNum][41 + input] > 98) {
                    UPrintMessageRewrapped(260);
                    UPrintWin("\p\n\n");
                    ErrorTone();
                    input = 'B';
                    goto shop4;
                }
                gold -= amount;
                Player[rosNum][35] = gold / 256;
                Player[rosNum][36] = gold - (Player[rosNum][35] * 256);
                Player[rosNum][41 + input]++;
                if (Player[rosNum][41 + input] > 99)
                    Player[rosNum][41 + input] = 99;
                UPrintMessage(210);
                SpeakMessages(210, 0, 18);
                //Speech(GetLocalizedPascalString("\pHere you are, may it serve you well!"),18);
                input = 'B';
                goto shop4;
            } else {
                UPrintMessage(211);
                input = CursorKey(false);
                while (input > 95) {
                    input -= 32;
                }
                if (input < 'B' || input >= opnum)
                    goto shop4done;
                UPrintChar(input, tx, ty);
                input -= 'B';
                if (Player[rosNum][41 + input] < 1) {
                    UPrintMessage(212);
                    SpeakMessages(212, 0, 18);
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
                    UPrintMessage(214);
                    ErrorTone();
                    return;
                }
                UPrintMessage(213);
                SpeakMessages(213, 0, 18);
                //Speech(GetLocalizedPascalString("\pThank you!"),18);
                input = 'S';
                goto shop4;
            }
        shop4done:
            UPrintMessageRewrapped(215);
            SpeakMessages(215, 0, 18);
            //Speech(GetLocalizedPascalString("\pOh well, maybe next time."),18);
            return;
            break;
        case 5:
            opnum = 0;
        shop5:
            UPrintMessage(217);
            if (opnum == 0) {
                SpeakMessages(217, 261, 23);
                //Speech(GetLocalizedPascalString("\pWe have keys, torches, powders, and gems."),23);
                opnum = 1;
            }
        shop5a:
            UPrintMessage(261);
            UPrintMessage(218);
            input = CursorKey(false);
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
                UPrintWin("\pN\n\n");
                UPrintMessageRewrapped(219);
                SpeakMessages(219, 0, 23);
                //Speech(GetLocalizedPascalString("\pThank you, come again!"),23);
                return;
            }
            gString[0] = 1;
            gString[1] = input;
            UPrintWin(gString);
            UPrintWin("\p\n");
            if (Player[rosNum][attrib] + qty > 99) {
                UPrintMessageRewrapped(260);
                ErrorTone();
                goto shop5a;
            }
            if (GuildPay(rosNum, cost) == 0) {
                GuildGive(rosNum, attrib, qty);
            } else {
                return;
            }
            UPrintMessage(220);
            SpeakMessages(220, 0, 23);
            //Speech(GetLocalizedPascalString("\pAnything else?"),23);
            input = CursorKey(false);
            if (input > 95)
                input -= 32;
            if (input == 'Y')
                goto shop5;
            UPrintWin("\pN\n\n");
            UPrintMessage(219);
            SpeakMessages(219, 0, 23);
            //Speech(GetLocalizedPascalString("\pThank you, come again!"),23);
            break;
        case 6:
            UPrintMessage(221);
        shop6:
            UPrintMessageRewrapped(222);
            SpeakMessages(222, 0, 16);
            //Speech(GetLocalizedPascalString("\pHow many 100 gold is your offering?"),16);
            input = -1;
            while (gDone != TRUE && (input < 0 || input > 9)) {
                input = CursorKey(false) - '0';
            }
            UPrintChar(input + '0', wx, wy);
            UPrintWin("\p\n\n");
            input *= 100;
            gold = (Player[rosNum][35] * 256) + Player[rosNum][36];
            if (input > gold) {
                UPrintMessage(188);
                SpeakMessages(188, 0, 16);
                //Speech(GetLocalizedPascalString("\pWhat? Can't pay? Out, you scum!"),16);
                ErrorTone();
                return;
            }
            gold -= input;
            Player[rosNum][35] = gold / 256;
            Player[rosNum][36] = gold - (Player[rosNum][35] * 256);
            GetPascalStringFromArrayByIndex(message, CFSTR("Radrion"), (input / 100));
            if (!CFPreferencesGetAppBooleanValue(U3PrefClassicAppearance, kCFPreferencesCurrentApplication, NULL))
                RewrapString(message, false);
            UPrintWin(message);
            for (opnum = 0; opnum <= message[0]; opnum++) {
                spmessage[opnum] = message[opnum];
                if (message[opnum] > 'z')
                    spmessage[opnum] = ' ';
            }
            Speech(spmessage, 16);
            UPrintMessage(223);
            input = CursorKey(false);
            if (input > 95)
                input -= 32;
            if (input != 'Y') {
                UPrintWin("\pN\n");
                UPrintMessageRewrapped(224);
                SpeakMessages(224, 0, 16);
                //Speech(GetLocalizedPascalString("\pFare thee well, and good luck!"),16);
                return;
            }
            UPrintWin("\pY\n");
            goto shop6;
            break;
        case 7:
            UPrintMessage(225);
            UPrintNum(Party[2], wx, wy);
            wx++;
            UPrintMessage(226);
            opnum = Party[2] * 200;
            UPrintNum(opnum, wx, wy);
            wx += 3;
            UPrintMessage(227);

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
            Speech(theString, 63);

            input = CursorKey(false);
            if (input > 95)
                input -= 32;
            if (input != 'Y') {
                UPrintWin("\pN\n\n");
                UPrintMessageRewrapped(228);
                SpeakMessages(228, 0, 63);
                //Speech(GetLocalizedPascalString("\pAh, too bad. These are the best in town!"),63);
                return;
            }
            gold = (Player[rosNum][35] * 256) + Player[rosNum][36];
            if (gold < opnum) {
                UPrintWin("\pY\n\n");
                UPrintMessageRewrapped(229);
                SpeakMessages(229, 0, 63);
                //Speech(GetLocalizedPascalString("\pI'm sorry, but you haven't the gold!"),63);
                ErrorTone();
                return;
            }
            gold -= opnum;
            Player[rosNum][35] = gold / 256;
            Player[rosNum][36] = gold - (Player[rosNum][35] * 256);
            UPrintWin("\pY\n\n");
            UPrintMessageRewrapped(230);
            SpeakMessages(230, 0, 63);
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
        UPrintMessageRewrapped(231);
        SpeakMessages(231, 0, 23);
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
    input = CursorKey(false);
    if (input > 95)
        input -= 32;
    if (input != 'Y') {
        UPrintWin("\pN\n\n");
        UPrintMessageRewrapped(232);
        SpeakMessages(232, 0, 9);
        //Speech(GetLocalizedPascalString("\pWithout proper offerings, I can not help!"),9);
        result = 1;
        return result;
    }
    gold = (Player[rosNum][35] * 256) + Player[rosNum][36];
    if (cost > gold) {
        UPrintWin("\pY\n\n");
        UPrintMessageRewrapped(233);
        SpeakMessages(233, 0, 9);
        //Speech(GetLocalizedPascalString("\pI'm sorry, but thou hast not gold enough."),9);
        ErrorTone();
        result = 1;
        return result;
    }
    gold -= cost;
    Player[rosNum][35] = gold / 256;
    Player[rosNum][36] = gold - (Player[rosNum][35] * 256);
    UPrintWin("\pY\n");
    return result;
}

void SpellNoize(short opnum) {
    InverseTiles();
    InverseChar(opnum);
    PlaySoundFile(CFSTR("Heal"), FALSE);    // was 0xF3, was Whine(0xC0,0x80);
    InverseChar(opnum);
    InverseTiles();
    UPrintMessageRewrapped(234);
    SpeakMessages(234, 0, 9);
    //Speech(GetLocalizedPascalString("\pFare thee well, my children."),9);
}

void WeaponList(short lastitem) {
    UPrintMessage(237);
    PrintWeaponList(1);
    PrintWeaponList(2);
    PrintWeaponList(3);
    PrintWeaponList(4);
    PrintWeaponList(5);
    WaitKeyMouse();
    PrintWeaponList(6);
    PrintWeaponList(7);
    if (lastitem == 'P') {
        PrintWeaponList(8);
        PrintWeaponList(9);
        PrintWeaponList(10);
        PrintWeaponList(11);
        WaitKeyMouse();
        PrintWeaponList(12);
        PrintWeaponList(13);
        PrintWeaponList(14);
    }
    WaitKeyMouse();
}

void ArmourList(short lastitem) {
    UPrintMessage(237);
    PrintArmourList(1);
    PrintArmourList(2);
    PrintArmourList(3);
    PrintArmourList(4);
    WaitKeyMouse();
    if (lastitem == 'H') {
        PrintArmourList(5);
        PrintArmourList(6);
        WaitKeyMouse();
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
        UPrintMessage(236);
    }
    chnum = GetChar();
    if (chnum < 1 || chnum > 4)
        return;
    if (CheckAlive(chnum - 1) == FALSE) {
        UPrintMessage(126);
        ErrorTone();
        return;
    }
    rosNum = Party[6 + chnum];
    UPrintMessage(238);
    UInputText(tx, ty, str, 8, false);
    UPrintWin("\p\n");
    if (EqualString(str, "\pPAXUM", 0, 0) && Party[3] != 0) {
        UPrintMessage(254);
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
        UPrintWin("\p\nAIEEEEE!\n\n");
        if (Player[rosNum][24] == 'F')
            PlaySoundFile(CFSTR("DeathFemale"), TRUE);
        else
            PlaySoundFile(CFSTR("DeathMale"), TRUE);
        return;
    }
    if (EqualString(str, "\pINSERT", 0, 0)) {
        UPrintMessage(239);
        GetDirection(0);

        if (GetXYVal(xs, ys) != 0x7C) {
            NotHere();
            return;
        }
        UPrintMessage(240);
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
            UPrintMessage(67);
            return;
        }
        if (xs != object || object != lastCard) {
            InverseChar(chnum - 1);
            PlaySoundFile(CFSTR("Hit"), true);
            ThreadSleepTicks(15);
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
            PlaySoundFile(CFSTR("Hit"), true);
            ThreadSleepTicks(15);
            PutXYVal(0x7C, xs, ys);
            DrawMap(xpos, ypos);
            PlaySoundFile(CFSTR("Hit"), true);
            ThreadSleepTicks(15);
        }
        PutXYVal(0x20, xs, ys);
        DrawMap(xpos, ypos);
        if (lastCard != 0x22)
            return;
        Party[16] = 1;

        Boolean classic = CFPreferencesGetAppBooleanValue(U3PrefClassicAppearance, kCFPreferencesCurrentApplication, NULL);
        gSongCurrent = gSongNext = 10;
        MusicUpdate();
        //      UPrintMessage(241);
        if (classic)
            GetPascalStringFromArrayByIndex(str, CFSTR("Messages"), 240);
        //GetIndString(str, BASERES+12, 241);
        else {
            GetPascalStringFromArrayByIndex(str, CFSTR("Messages"), 254);
            // GetIndString(str, BASERES+12, 255);
            RewrapString(str, false);
        }
        UPrintWin(str);
        time = Party[11] + Party[12] * 100 + Party[13] * 10000 + Party[14] * 1000000;
        NumToString(time, str);
        if (!classic)
            UPrintWin("\p\n");
        UPrintWin(str);
        UPrintMessage(242);

        // Build up a string to speak the whole thing
        Str255 theString = "\p";
        GetPascalStringFromArrayByIndex(theString, CFSTR("Messages"), 254);
        AddString(theString, str);
        GetPascalStringFromArrayByIndex(gString, CFSTR("Messages"), 242 - 1);
        AddString(theString, gString);
        SearchReplace(theString, "\p:", "\p,");    // Exodus: Ultima III
        Speech(theString, 31);

        object = gUpdateWhere;
        gUpdateWhere = 0;
        for (input = 0; input <= 20; input++) {
            SetRect(&screct, blkSiz, blkSiz, blkSiz * 23, blkSiz * 23);
            color.red = Absolute(Random() * 2);
            color.green = Absolute(Random() * 2);
            color.blue = Absolute(Random() * 2);
            RGBForeColor(&color);
            BackColor(whiteColor);
            PenMode(addOver);
            PaintRect(&screct);
            ForceUpdateMain();
            PlaySoundFile(CFSTR("Hit"), FALSE);    // was 0xF7
            ThreadSleepTicks(10);
        }
        PenMode(srcCopy);
        ClearTiles();
        for (input = 0; input < 256; input += 16) {
            WinText(input);
            ThreadSleepTicks(1);
            time = TickCount();
            while (time == TickCount()) {
                GetKeyMouse(0);
            }
        }
        WaitKeyMouse();
        for (input = 255; input >= 0; input -= 16) {
            WinText(input);
            time = TickCount();
            while (time == TickCount()) {
                GetKeyMouse(0);
            }
        }
        gUpdateWhere = object;
        SafeExodus();
        DrawMap(xpos, ypos);
        UPrintWin("\p\n");
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
            UPrintMessage(243);
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
        UPrintMessageRewrapped(244);
        return;
    }
    if (str[0] == 5 && str[str[0]] == 'E') { /* Bribe */
        UPrintMessage(239);
        GetDirection(0);
        object = MonsterHere(xs, ys);
        if (object > 127) {
            NotHere();
            return;
        }
        gold = (Player[rosNum][35] * 256) + Player[rosNum][36];
        if (gold < 100) {
            UPrintMessage(245);
            ErrorTone();
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
        UPrintMessage(246);
        SpeakMessages(246, 0, 31);
        //Speech(GetLocalizedPascalString("\pYell ee voh care!"),31);
        return;
    }
    if (str[0] == 7 && str[str[0]] == 'E') { /* Evocare */
        Yell(1 + chnum);
        return;
    }
    if (EqualString(str, "\pPISSOFF", 0, 0)) {
        UPrintMessage(247);
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
    UPrintMessage(248);
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
