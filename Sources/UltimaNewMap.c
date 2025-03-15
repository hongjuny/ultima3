// New random map generator [K]reate after game solved

#import "UltimaNewMap.h"

#import "UltimaIncludes.h"
#import "CarbonShunts.h"
#import "UltimaGraphics.h"
#import "UltimaMain.h"
#import "UltimaMisc.h"
#import "UltimaSound.h"

extern CGrafPtr         mainPort, minitilesPort;
extern int              xpos, ypos;
extern short            WhirlX, WhirlY;
extern unsigned char    LocationX[20], LocationY[20], MoonXTable[8], MoonYTable[8];
extern unsigned char    Monsters[256], Party[64];
extern short            gCurMapSize, gUpdateWhere, zp[255];
extern short            blkSiz;
extern short            gSongCurrent, gSongNext, gSongPlaying;

Boolean                 showCreation=TRUE;

// ----------------------------------------------------------------------
// Local prototypes

void MapSplat(unsigned char startX, unsigned char startY, unsigned char size, unsigned char onlyOn, unsigned char type);
void CleanUpDiags(short what);
void CleanSurround(void);
void CleanUpSingle(void);
Boolean CheckIfHasLava(void);
void ShowDot(short x, short y);
void AllWater(void);
void Progress(short percent);

// ----------------------------------------------------------------------

void CreateNewMap(void) {
    Rect myRect;
    Boolean hasLava;
    short x, y, saveCursor;
    unsigned char storeIcons[19], range, halfrange;
    unsigned short value, byte, byte2, target, counter, proximityLimit;

    saveCursor = gUpdateWhere;
    gUpdateWhere = 0;
    LWSetArrowCursor();
    Progress(0);
    /*
    showCreation=FALSE;
    long keyz[4];
    GetKeys(keyz);
    if (keyz[1] & 0x00000001) showCreation=TRUE; // shift key down
    */
    if (showCreation) {
        SetRect(&myRect, blkSiz, blkSiz, blkSiz * 23, blkSiz * 23);
        ForeColor(blackColor);
        BackColor(whiteColor);
        PaintRect(&myRect);
    }
    Progress(1);
    for (byte = 0; byte < 32; byte++) {
        Monsters[byte] = 0;
    }
    for (byte = 0; byte < 19; byte++) {
        storeIcons[byte] = GetXYVal(LocationX[byte], LocationY[byte]);
        if (storeIcons[byte] == 0x0C)
            storeIcons[byte] = 0x18; /* dawn */
    }
    AllWater();
    PlaySoundFile(CFSTR("BigDeath"), TRUE);    // was 0xE0
    value = RandNum(8, 72);
    for (byte = 0; byte < value; byte++) {
        range = RandNum(4, (gCurMapSize / 2));
        halfrange = range / 2;
        MapSplat(RandNum(halfrange, gCurMapSize - halfrange), RandNum(halfrange, gCurMapSize - halfrange), range, 0, 4);
        Progress((50.0 / value) * byte + 5);
    }
    CleanUpSingle();
    CleanSurround();
    CleanUpDiags(0x04);
    for (byte = 0; byte < 64; byte++) {
        MapSplat(RandNum(2, gCurMapSize - 2), RandNum(2, gCurMapSize - 2), RandNum(2, 8), 0x04, 0x10);
    }
    Progress(60);
    for (byte = 0; byte < 16; byte++) {
        MapSplat(RandNum(2, gCurMapSize - 2), RandNum(2, gCurMapSize - 2), RandNum(4, 16), 0x04, 0x0C);
    }
    Progress(65);
    for (byte = 0; byte < 20; byte++) {
        MapSplat(RandNum(2, gCurMapSize - 2), RandNum(2, gCurMapSize - 2), RandNum(4, 16), 0x04, 0x08);
    }
    Progress(70);
    CleanUpSingle();
    CleanSurround();
    CleanUpSingle();
    CleanSurround();
    CleanUpDiags(0x10);
    CleanUpDiags(0x04);
    for (byte = 0; byte < 3; byte++) {
        MapSplat(RandNum(2, gCurMapSize - 2), RandNum(2, gCurMapSize - 2), RandNum(2, 12), 0x10, 0x84);
    }
    CleanUpDiags(0x84);
    CleanUpDiags(0x10);
    CleanUpSingle();
    hasLava = CheckIfHasLava();
    for (byte = 0; byte < 19; byte++) {
        Progress(70 + byte);
        target = 0x04;
        if (storeIcons[byte] == 0x14)
            target = 0x10; /* mountain for dungeon */
        if (byte == 8 || byte == 4)
            target = 0x0C; /* forest for dawn & yew */
        if (byte == 11)
            target = 0x10; /* mountain for death gulch */
        value = 0xFF;
        counter = 0;
        proximityLimit = 18;
        while (value != target) {
            x = RandNum(0, gCurMapSize - 1);
            y = RandNum(0, gCurMapSize - 1);
            value = GetXYVal(x, y);
            if (target == 0x10 && value == target) {
                if (GetXYVal(x, y + 1) > 0x0C && GetXYVal(x, y - 1) > 0x0C) {
                    if (GetXYVal(x + 1, y) > 0x0C && GetXYVal(x - 1, y) > 0x0C) {
                        value = 0xFF;
                    }
                }
                if (GetXYVal(x, y - 1) != 0x10)
                    value = 0xFF;
                if (GetXYVal(x - 1, y) != 0x10 && GetXYVal(x + 1, y) != 0x10)
                    value = 0xFF;
            }
            if (target != 0x10 && value == target && byte != 1) {
                if (GetXYVal(x, y + 1) != target || GetXYVal(x, y - 1) != target || GetXYVal(x - 1, y) != target) {
                    counter++;
                    if (counter < 100)
                        value = 0xFF;
                }
            }
            if (value == target && (byte == 9 || byte == 10)) { /* devil guard & fawn by water */
                if (GetXYVal(x + 1, y) != 0) {
                    counter++;
                    if (counter < 100)
                        value = 0xFF;
                }
            }
            if (value == target && byte == 11) { /* death gulch by mountains */
                if (GetXYVal(x + 1, y) != 0x10) {
                    counter++;
                    if (counter < 100)
                        value = 0xFF;
                }
            }
            if (value == target && byte == 1 && hasLava) { /* Exodus by lava */
                if (GetXYVal(x, y - 1) != 0x84 && GetXYVal(x - 1, y) != 0x84) {
                    counter++;
                    if (counter < 2000)
                        value = 0xFF;
                }
            }
            if (value == target) { /* after above rigorous testing! */
                if (byte > 0) {
                    for (byte2 = 0; byte2 < byte; byte2++) {
                        if (Absolute(x - LocationX[byte2]) + Absolute(y - LocationY[byte2]) < proximityLimit) {
                            value = 0xFF;
                            counter++;
                            if (counter > 20) {
                                counter = 0;
                                proximityLimit--;
                            }
                        }
                    }
                }
            }
        }
        LocationX[byte] = x;
        LocationY[byte] = y;
        PutXYVal(storeIcons[byte], x, y);
        ShowDot(x, y);
    }
    counter = 0;
    for (byte = 0; byte < 8; byte++) {
        Progress(90 + byte);
        proximityLimit = 20;
        value = 0;
        while (value != 0x04) {
            x = RandNum(0, gCurMapSize - 1);
            y = RandNum(0, gCurMapSize - 1);
            value = GetXYVal(x, y);
            if ((byte % 2) && (GetXYVal(x, y - 1) != 0x10) && (GetXYVal(x, y + 1) != 0x10))
                value = 0;
            if (byte > 0 && value == 0x04) {
                for (byte2 = 0; byte2 < byte; byte2++) {
                    if ((Absolute(x - MoonXTable[byte2]) + Absolute(y - MoonYTable[byte2]) < proximityLimit)) {
                        value = 0xFF;
                        counter++;
                        if (counter > 20) {
                            counter = 0;
                            proximityLimit--;
                        }
                    }
                }
            }
        }
        MoonXTable[byte] = x;
        MoonYTable[byte] = y;
        PutXYVal(0x88, x, y);
        ShowDot(x, y);
    }
    value = 0xFF;
    target = 0x04;
    if (Party[1] == 0x16)
        target = 0;
    while (value != target) {
        x = RandNum(0, gCurMapSize - 1);
        y = RandNum(0, gCurMapSize - 1);
        value = GetXYVal(x, y);
    }
    Progress(100);
    xpos = x;
    ypos = y;
    WhirlX = 0;
    WhirlY = 0;
    /*  if (showCreation)
        {
        value = gUpdateWhere; gUpdateWhere=0;
        WaitKeyMouse();
        gUpdateWhere=value;
        }*/
    QuitSave(0);
    PutMiscStuff();
    PlaySoundFile(CFSTR("LBLevelRise"), TRUE);
    gUpdateWhere = saveCursor;
    DrawMiniMap();
}

void MapSplat(unsigned char startX, unsigned char startY, unsigned char size, unsigned char onlyOn, unsigned char type) {
    short leftLimit, rightLimit, topLimit, bottomLimit;
    short x, y, oldx, oldy, counter, amount;
    short dir[4] = {0, -1, 0, 1};

    x = startX;
    y = startY;
    oldx = x;
    oldy = y;
    amount = size * size;
    leftLimit = startX - (size / 2);
    if (leftLimit < 1)
        leftLimit = 1;
    rightLimit = startX + (size / 2);
    if (rightLimit > gCurMapSize - 2)
        rightLimit = gCurMapSize - 2;
    topLimit = startY - (size / 2);
    if (topLimit < 1)
        topLimit = 1;
    bottomLimit = startY + (size / 2);
    if (bottomLimit > gCurMapSize - 2)
        bottomLimit = gCurMapSize - 2;
    for (counter = 0; counter < amount; counter++) {
        if (GetXYVal(x, y) == onlyOn) {
            PutXYVal(type, x, y);
            ShowDot(x, y);
        }
        while (oldx == x && oldy == y) {
            x += (dir[Random() & 0x03]);
            y += (dir[Random() & 0x03]);
            x = MapConstrain(x);
            y = MapConstrain(y);
            if (x != oldx && y != oldy) {
                x = oldx;
                y = oldy;
            }
        }
        oldx = x;
        oldy = y;
        if (x < leftLimit || x > rightLimit || y < topLimit || y > bottomLimit) {
            x = startX;
            y = startY;
        }
    }
}

void CleanUpDiags(short what) {
    short x, y;
    for (y = 1; y < gCurMapSize - 2; y++) {
        for (x = 1; x < gCurMapSize - 2; x++) {
            if (GetXYVal(x, y) == what) {
                if (GetXYVal(x - 1, y - 1) == what && GetXYVal(x, y - 1) != what && GetXYVal(x - 1, y) != what) {
                    if (RandNum(0, 255) > 127) {
                        PutXYVal(what, x - 1, y);
                        ShowDot(x - 1, y);
                    } else {
                        PutXYVal(what, x, y - 1);
                        ShowDot(x, y - 1);
                    }
                }
                if (GetXYVal(x - 1, y + 1) == what && GetXYVal(x - 1, y) != what && GetXYVal(x, y + 1) != what) {
                    if (RandNum(0, 255) > 127) {
                        PutXYVal(what, x - 1, y);
                        ShowDot(x - 1, y);
                    } else {
                        PutXYVal(what, x, y + 1);
                        ShowDot(x, y + 1);
                    }
                }
            }
        }
    }
}

void CleanSurround(void) {
    short x, y, value;

    for (y = 1; y < gCurMapSize - 1; y++) {
        for (x = 1; x < gCurMapSize - 1; x++) {
            value = GetXYVal(x, y);
            if (value != GetXYVal(x - 1, y - 1) && value != GetXYVal(x + 1, y - 1)) {
                if (value != GetXYVal(x + 1, y - 1) && value != GetXYVal(x - 1, y + 1)) {
                    PutXYVal(GetXYVal(x - 1, y), x, y);
                    ShowDot(x, y);
                }
            }
        }
    }
}

void CleanUpSingle(void) {
    short x, y, value;

    for (y = 1; y < gCurMapSize - 1; y++) {
        for (x = 1; x < gCurMapSize - 1; x++) {
            value = GetXYVal(x, y);
            if (value != GetXYVal(x - 1, y) && value != GetXYVal(x + 1, y)) {
                if (value != GetXYVal(x, y - 1) && value != GetXYVal(x, y + 1)) {
                    PutXYVal(GetXYVal(x - 1, y), x, y);
                    ShowDot(x, y);
                }
            }
        }
    }
}

Boolean CheckIfHasLava(void) {
    short x, y;
    for (y = 1; y < gCurMapSize - 1; y++) {
        for (x = 1; x < gCurMapSize - 1; x++) {
            if ((GetXYVal(x, y) == 0x84) && (GetXYVal(x, y + 1) == 0x04))
                return TRUE;
            if ((GetXYVal(x, y) == 0x84) && (GetXYVal(x + 1, y) == 0x04))
                return TRUE;
        }
    }
    return FALSE;
}

void ShowDot(short x, short y) {

    Rect myRect, miniRect;
    short value;

    if (!showCreation)
        return;
    short minSize = (short)truncf((float)(blkSiz * 22) / (float)gCurMapSize);
    int offset = (blkSiz == 16) ? blkSiz * 2 : blkSiz;
    myRect.left = x * minSize + offset;
    myRect.top = y * minSize + offset;
    myRect.right = myRect.left + minSize;
    myRect.bottom = myRect.top + minSize;
    value = (GetXYVal(x, y) / 4);
    miniRect.left = value * minSize;
    miniRect.right = miniRect.left + minSize;
    miniRect.top = 0;
    miniRect.bottom = minSize;
    ForeColor(blackColor);
    BackColor(whiteColor);
    CopyBits(LWPortCopyBits(minitilesPort), LWPortCopyBits(mainPort), &miniRect, &myRect, srcCopy, nil);
}

void AllWater(void) {
    short x, y;
    Rect myRect, miniRect;
    short minSize;

    int offset = (blkSiz == 16) ? blkSiz * 2 : blkSiz;
    minSize = (blkSiz * 22) / gCurMapSize;
    miniRect.top = 0;
    miniRect.bottom = minSize;
    miniRect.left = 0;
    miniRect.right = minSize;
    ForeColor(blackColor);
    BackColor(whiteColor);
    for (y = 0; y < gCurMapSize; y++) {
        for (x = 0; x < gCurMapSize; x++) {
            PutXYVal(0, x, y);
        }
    }
    if (!showCreation)
        return;
    for (y = 0; y < gCurMapSize; y++) {
        for (x = 0; x < gCurMapSize; x++) {
            myRect.left = x * minSize + offset;
            myRect.top = y * minSize + offset;
            myRect.right = myRect.left + minSize;
            myRect.bottom = myRect.top + minSize;
            CopyBits(LWPortCopyBits(minitilesPort), LWPortCopyBits(mainPort), &miniRect, &myRect, srcCopy, nil);
        }
    }
}

void Progress(short percent) {
    ForceUpdateMain();
}

void DrawDioramaMap(void) {
    Boolean drawOcean;
    //  RgnHandle       saveRegion;
    short value, x, y, scrx, scry, top, updateStore, minSize;
    Rect fromRect, toRect, mapRect;
    RGBColor color;
    CursHandle watchCurs;
    long time;
    PicHandle ocean;
    RgnHandle saveRgn;
    GrafPtr savePort;

    //                     00010203040506070809101112131415161718192021222324252627282930313233343536373839404142434445464748495051525354555657585960616263
    char ht[64] = {0, 1, 2, 4, 6, 4, 4, 4, 1, 4, 4, 4, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
                   6, 0, 4, 6, 0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 4, 6, 6, 4, 4};

    GetPort(&savePort);
    saveRgn = NewRgn();
    GetClip(saveRgn);
    SetRect(&toRect, blkSiz, blkSiz, blkSiz * 23, blkSiz * 23);
    ClipRect(&toRect);
    minSize = (blkSiz * 22) / gCurMapSize;
    ocean = GetPicture(BASERES + 3);
    drawOcean = (ocean != 0);
    gSongCurrent = 10;
    MusicUpdate();
    watchCurs = GetCursor(watchCursor);
    SetCursor(*watchCurs);
    ForeColor(blackColor);
    mapRect.left = (blkSiz * 12) - (minSize * 32);
    mapRect.right = mapRect.left + (minSize * gCurMapSize);
    mapRect.top = mapRect.left;
    mapRect.bottom = mapRect.right;
    if (drawOcean) {
        DrawPicture(ocean, &mapRect);
        ReleaseResource((Handle)ocean);
    } else {
        PaintRect(&mapRect);
    }
    color.blue = color.green = color.red = 24576;
    OpColor(&color);
    SetRect(&toRect, mapRect.right, mapRect.top + blkSiz, mapRect.right + blkSiz, mapRect.bottom + blkSiz);
    PenMode(blend);
    PaintRect(&toRect);
    SetRect(&toRect, mapRect.left + blkSiz, mapRect.bottom, mapRect.right, mapRect.bottom + blkSiz);
    PaintRect(&toRect);
    ForeColor(whiteColor);
    PenMode(srcCopy);
    toRect = mapRect;
    InsetRect(&toRect, -1, -1);
    FrameRect(&toRect);
    InsetRect(&toRect, -1, -1);
    ForeColor(blackColor);
    FrameRect(&toRect);
    fromRect.top = 0;
    fromRect.bottom = minSize;
    color.blue = color.green = color.red = 32768;
    OpColor(&color);
    for (y = 0; y < gCurMapSize; y++) {
        for (x = 0; x < gCurMapSize; x++) {
            value = GetXYVal(x, y) / 4;
            top = ht[value];
            scrx = (x * minSize + mapRect.left) - top;
            scry = (y * minSize + mapRect.top) - top;
            toRect.left = scrx;
            toRect.right = scrx + minSize;
            toRect.top = scry;
            toRect.bottom = scry + minSize;
            fromRect.left = value * minSize;
            fromRect.right = fromRect.left + minSize;
            ForeColor(blackColor);
            if (value != 0 || !drawOcean)
                CopyBits(LWPortCopyBits(minitilesPort), LWPortCopyBits(mainPort), &fromRect, &toRect, srcCopy, nil);
            if (top > 0) {
                GetCPixel(scrx, scry, &color);
                color.red = color.red / 2;
                color.green = color.green / 2;
                color.blue = color.blue / 2;
                RGBForeColor(&color);
                for (value = 0; value <= top; value++) {
                    MoveTo(scrx + value, scry + value + minSize);
                    LineTo(scrx + value + minSize, scry + value + minSize);
                }
                color.red = color.red / 2;
                color.green = color.green / 2;
                color.blue = color.blue / 2;
                RGBForeColor(&color);
                for (value = 0; value <= top; value++) {
                    MoveTo(scrx + value + minSize, scry + value);
                    LineTo(scrx + value + minSize, scry + value + minSize);
                }
            }
            if (x > 0 && y > 0 && top < 3) {
                if ((ht[GetXYVal(x - 1, y) / 4] > 3) || (ht[GetXYVal(x, y - 1) / 4] > 3) || (ht[GetXYVal(x - 1, y - 1) / 4] > 3)) {
                    PenMode(blend);
                    PaintRect(&toRect);
                    PenMode(srcCopy);
                }
            }
        }
    }
    updateStore = gUpdateWhere;
    gUpdateWhere = 0;
    SetClip(saveRgn);
    DisposeRgn(saveRgn);
    value = ht[GetXYVal(xpos, ypos) / 4];
    toRect.left = xpos * minSize + (mapRect.left) - value;
    toRect.top = ypos * minSize + (mapRect.top) - value;
    toRect.right = toRect.left + minSize;
    toRect.bottom = toRect.top + minSize;
    InitCursor();
    SetPort(savePort);
    while (!GetKeyMouse(1)) {
        time = TickCount();
        InvertRect(&toRect);
        while ((time + 10) > TickCount()) {
        }
    }
    gUpdateWhere = updateStore;
}
