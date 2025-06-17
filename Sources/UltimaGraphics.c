// Low-level graphic routines

#import "UltimaGraphics.h"

#import "UltimaIncludes.h"
#import "CarbonShunts.h"
#import "CocoaBridge.h"
#import "UltimaDngn.h"
#import "UltimaMacIF.h"
#import "UltimaMain.h"
#import "UltimaMisc.h"
#import "UltimaNew.h"
#import "UltimaSound.h"
#import "UltimaSpellCombat.h"
#import "UltimaText.h"


extern OSErr            gError;
extern WindowPtr        gMainWindow, gShroudWindow;
extern char             gKeyPress;
extern unsigned char    gCurFrame, Player[21][65], Party[64], TileArray[128];
extern unsigned char    Monsters[256], cHide, gBallTileBackground;
extern unsigned char    careerTable[12], CharX[4], CharY[4], CharTile[4], CharShape[4];
extern unsigned char    MonsterX[8], MonsterY[8], MonsterTile[8], MonsterHP[8];
extern Boolean          gDone, gInterrupt, gInBackground, gHasOddTextPort;
extern GrafPtr          mainWMPort;
extern CGrafPtr         portraitPort, framePort, mainPort, tilesPort, tilesMaskPort;
extern CGrafPtr         minitilesPort, gamePort, demoPort, textPort, textOddPort, updatePort;
extern CGrafPtr         exodusFromPort, exodusToPort, logoFromPort, logoToPort, gWidePort;
extern PixMapHandle     exodusFromPixMap, exodusToPixMap, logoFromPixMap, logoToPixMap;
extern PixMapHandle     gWidePixMap, demoPixMap, tilesPixMap, gamePixMap, mainPixMap;
extern PixMapHandle     framePixMap, directPixMap;
extern GDHandle         mainDevice;
extern int              tx, ty, xpos, ypos, xs, ys;
extern EventRecord      gTheEvent;
extern short            gCurMapSize, gUpdateWhere, gMouseState, stx, sty;
extern Handle           gDemoData;
extern short            zp[255], animFlag[4], gMonType, gDepth, gTorch, gCurChan, gMaxChan;
extern SndChannelPtr    gSampChan[5], gToneChan;
extern Str255           gString;
extern short            gDemoSong, gSongCurrent, gSongNext, gSongPlaying;
extern Boolean          gShapeSwapped[], gHorseFacingEast;
extern short            blkSiz;
extern char             gMonVarType;

Ptr                     gFromPixMapBase, gToPixMapBase;
long                    gFromRowByteCount, gToRowByteCount;
unsigned char           maskRestoreArray[128], maskArray[128];
short                   directOffsetX, directOffsetY;
Rect                    mouseRect;

void SetUpMouseRect(void) {
    Point mouse;
    GetMouse(&mouse);
    LocalToGlobal(&mouse);
    mouseRect.left = mouse.h - blkSiz;
    mouseRect.right = mouse.h + blkSiz;
    mouseRect.top = mouse.v - blkSiz;
    mouseRect.bottom = mouse.v + blkSiz;
}

void ClearBottom(void) {
    Rect myRect;
    SetRect(&myRect, blkSiz, (blkSiz * 10) - 1, blkSiz * 39, blkSiz * 23);
    ForeColor(blackColor);
    PaintRect(&myRect);
    ForeColor(whiteColor);
}

void ClearScreen(void) {
    Rect myRect;
    LWGetWindowBounds(gMainWindow, &myRect);
    ForeColor(blackColor);
    PaintRect(&myRect);
    ForeColor(whiteColor);
}

void DrawFrame(short which)
/* 1=Game frame, 2=Intro frame, 3=Plain square */
{
    Str32 str;
    short x;
    ForeColor(blackColor);
    BackColor(whiteColor);
    gCurFrame = which;
    if (!gDone) {
        if (which == 1) {
            for (x = 1; x < 39; x++) {
                DrawFramePiece(10, x, 0);
            }
            for (x = 1; x < 23; x++) {
                DrawFramePiece(10, x, 23);
            }
            for (x = 24; x < 39; x++) {
                DrawFramePiece(10, x, 4);
                DrawFramePiece(10, x, 8);
                DrawFramePiece(10, x, 12);
                DrawFramePiece(10, x, 16);
            }
            for (x = 1; x < 23; x++) {
                DrawFramePiece(11, 0, x);
                DrawFramePiece(11, 23, x);
            }
            for (x = 1; x < 16; x++) {
                DrawFramePiece(11, 39, x);
            }
            DrawFramePiece(1, 0, 0);
            DrawFramePiece(3, 39, 0);
            DrawFramePiece(7, 0, 23);
            DrawFramePiece(2, 23, 0);
            DrawFramePiece(9, 39, 16);
            DrawFramePiece(9, 23, 23);
            DrawFramePiece(4, 23, 4);
            DrawFramePiece(6, 39, 4);
            DrawFramePiece(4, 23, 8);
            DrawFramePiece(6, 39, 8);
            DrawFramePiece(4, 23, 12);
            DrawFramePiece(6, 39, 12);
            DrawFramePiece(4, 23, 16);
            for (x = 0; x < 4; x++) {
                DrawFramePiece(12, 30, x * 4);
                DrawFramePiece(13, 32, x * 4);
                str[0] = 1;
                str[1] = '1' + x;
                UCenterAt(str, 31, x * 4);
            }
            if (CFPreferencesGetAppBooleanValue(U3PrefIncludeWind, kCFPreferencesCurrentApplication, NULL)) {
                UCenterAt("\p          ", 7, 23);
                DrawFramePiece(12, 6, 23);
                DrawFramePiece(13, 17, 23);
            }
            DrawMoonGateStuff();
        }
        if ((which == 2) || (which == 3)) {
            DrawFramePiece(1, 0, 0);
            DrawFramePiece(7, 0, 23);
            DrawFramePiece(3, 39, 0);
            DrawFramePiece(9, 39, 23);
            for (x = 1; x < 39; x++) {
                DrawFramePiece(10, x, 0);
            }
            for (x = 1; x < 39; x++) {
                DrawFramePiece(10, x, 23);
            }
            if (which == 2) {
                for (x = 1; x < 39; x++) {
                    DrawFramePiece(10, x, 10);
                }
            }
            for (x = 1; x < 23; x++) {
                DrawFramePiece(11, 0, x);
            }
            for (x = 1; x < 23; x++) {
                DrawFramePiece(11, 39, x);
            }
            if (which == 2) {
                DrawFramePiece(4, 0, 10);
                DrawFramePiece(6, 39, 10);
            }
        }
    }
}

void DrawFramePiece(short which, short x, short y) {
    Rect frameRect, myRect;
    if (!gDone) {
        which--;
        ForeColor(blackColor);
        BackColor(whiteColor);
        myRect.left = x * blkSiz;
        myRect.top = y * blkSiz;
        myRect.right = myRect.left + blkSiz;
        myRect.bottom = myRect.top + blkSiz;
        frameRect.top = 0;
        frameRect.bottom = blkSiz;
        frameRect.left = which * blkSiz;
        frameRect.right = frameRect.left + blkSiz;
        CopyBits(LWPortCopyBits(framePort), LWPortCopyBits(mainPort), &frameRect, &myRect, srcCopy, nil);
    }
}

void DrawFramePieceScroll(short which, short x, short y) {
    Rect frameRect, myRect;
    if (!gDone) {
        which--;
        ForeColor(blackColor);
        BackColor(whiteColor);
        myRect.left = x * blkSiz;
        myRect.top = y * blkSiz;
        myRect.right = myRect.left + blkSiz;
        myRect.bottom = myRect.top + blkSiz;
        frameRect.top = 0;
        frameRect.bottom = blkSiz;
        frameRect.left = which * blkSiz;
        frameRect.right = frameRect.left + blkSiz;
        CopyBits(LWPortCopyBits(framePort), LWPortCopyBits(updatePort), &frameRect, &myRect, srcCopy, nil);
    }
}

void GetFont(void) {
    SetUpFont();
    short chr;
    for (chr = 0; chr < 128; chr++) { /* logistically bad place to put this but oh well */
        maskArray[chr] = 255;
    }
}

void ClearTiles(void) {
    Rect ClearRect;

    ClearRect.left = blkSiz;
    ClearRect.top = blkSiz;
    ClearRect.bottom = blkSiz * 23;
    ClearRect.right = blkSiz * 23;
    ForeColor(blackColor);
    PaintRect(&ClearRect);
}

Boolean GetGraphicTiledFile(CFURLRef fileURLRef, CGrafPtr destWorld, int tileWidth, int tileHeight, int tilesWide, int tilesHigh) {
    Boolean success = FALSE;
    FSRef fsr;
    if (CFURLGetFSRef(fileURLRef, &fsr)) {
        FSSpec fss;
        OSErr err = FSGetCatalogInfo(&fsr, kFSCatInfoNone, nil, nil, &fss, nil);
        if (err == noErr) {
            ComponentInstance gi;
            err = GetGraphicsImporterForFile(&fss, &gi);
            Rect finalRect;
            finalRect.left = finalRect.top = 0;
            finalRect.right = tilesWide * tileWidth;
            finalRect.bottom = tilesHigh * tileHeight;

            // Find out the size of the source image. If it's a PDF, we'll ignore its size.
            Rect importRect;
            if (fss.name[fss.name[0] - 2] == 'p' && fss.name[fss.name[0] - 1] == 'd' && fss.name[fss.name[0]] == 'f')
                importRect = finalRect;
            else if (err == noErr)
                err = GraphicsImportGetBoundsRect(gi, &importRect);

            Boolean straightDump = TRUE;
            // if the tile set is not the dimensions we will need, resize each tile separately.
            if (err == noErr && (importRect.right != finalRect.right || importRect.bottom != finalRect.bottom)) {
                CGrafPtr tempWorld = nil;
                err = NewGWorld(&tempWorld, 32, &importRect, nil, nil, 0);
                CGrafPtr resizeTileWorld = nil;
                Rect resizeTileRect;
                // if our final size is *bigger* than the source, we'll have an interim
                // resize world to make them bigger than the final size so we'll get smoothing.
                if (importRect.right < finalRect.right && finalRect.right % importRect.right != 0) {
                    SetRect(&resizeTileRect, 0, 0, tileWidth * 4, tileHeight * 4);
                    err = NewGWorld(&resizeTileWorld, 32, &resizeTileRect, nil, nil, 0);
                }
                if (err == noErr)
                    err = GraphicsImportSetGWorld(gi, tempWorld, nil);
                if (err == noErr)
                    err = GraphicsImportSetBoundsRect(gi, &importRect);
                if (err == noErr)
                    err = GraphicsImportDraw(gi);
                if (err == noErr) {
                    straightDump = FALSE;
                    success = TRUE;
                    int importTileVertSize = importRect.bottom / tilesHigh;
                    int importTileHorizSize = importRect.right / tilesWide;
                    Rect importTileRect, finalTileRect;
                    importTileRect.left = importTileRect.top = finalTileRect.left = finalTileRect.top = 0;
                    importTileRect.right = importTileHorizSize;
                    importTileRect.bottom = importTileVertSize;
                    finalTileRect.right = tileWidth;
                    finalTileRect.bottom = tileHeight;
                    ForeColor(blackColor);
                    BackColor(whiteColor);
                    int x, y;
                    for (y = 0; y < tilesHigh; y++) {
                        importTileRect.top = y * importTileVertSize;
                        importTileRect.bottom = importTileRect.top + importTileVertSize;
                        importTileRect.left = 0;
                        importTileRect.right = importTileHorizSize;
                        finalTileRect.top = y * tileHeight;
                        finalTileRect.bottom = finalTileRect.top + tileHeight;
                        finalTileRect.left = 0;
                        finalTileRect.right = tileWidth;
                        for (x = 0; x < tilesWide; x++) {
                            if (resizeTileWorld) {
                                CopyBits(LWPortCopyBits(tempWorld), LWPortCopyBits(resizeTileWorld), &importTileRect,
                                         &resizeTileRect, ditherCopy, nil);
                                CopyBits(LWPortCopyBits(resizeTileWorld), LWPortCopyBits(destWorld), &resizeTileRect,
                                         &finalTileRect, ditherCopy, nil);
                            } else
                                CopyBits(LWPortCopyBits(tempWorld), LWPortCopyBits(destWorld), &importTileRect, &finalTileRect,
                                         srcCopy, nil);
                            importTileRect.left += importTileHorizSize;
                            importTileRect.right += importTileHorizSize;
                            finalTileRect.left += tileWidth;
                            finalTileRect.right += tileWidth;
                        }
                    }
                }
                if (resizeTileWorld)
                    DisposeGWorld(resizeTileWorld);
                if (tempWorld)
                    DisposeGWorld(tempWorld);
            }

            if (straightDump) {   // just draw it straight in.
                err = GraphicsImportSetGWorld(gi, destWorld, nil);
                if (err == noErr)
                    err = GraphicsImportSetBoundsRect(gi, &finalRect);
                if (err == noErr)
                    err = GraphicsImportDraw(gi);
                success = (err == noErr);
            }
            CloseComponent(gi);
        }
    }
    return success;
}

// performs functions of GetTiles(), GetFrames() etc.
void GetGraphics(void) {
    int i;
    Boolean success;

    CFStringRef defaultTilesRef = CFSTR("Standard");
    CFURLRef tilesBaseURL = (CFURLRef)GraphicsDirectoryURL();
    CFArrayRef graphicsArrayRef = (CFArrayRef)CopyGraphicsDirectoryItems();
    CFStringRef tilesNameRef = CFPreferencesCopyAppValue(U3PrefTileSet, kCFPreferencesCurrentApplication);
    if (!tilesNameRef)
        tilesNameRef = defaultTilesRef;

tilesAgain:
    // Set up Tiles
    success = FALSE;
    CFStringRef tilesFilenamePrefixRef = (CFStringRef)CopyCatStrings(tilesNameRef, CFSTR("-Tiles"));
    i = CFArrayGetCount(graphicsArrayRef) - 1;
    while (!success && i >= 0) {
        CFStringRef anItemRef = CFArrayGetValueAtIndex(graphicsArrayRef, i--);
        if (CFStringHasPrefix(anItemRef, tilesFilenamePrefixRef)) {
            CFURLRef fullTilesURLRef = CFURLCreateCopyAppendingPathComponent(nil, tilesBaseURL, anItemRef, false);
            success = (GetGraphicTiledFile(fullTilesURLRef, tilesPort, blkSiz * 2, blkSiz * 2, 12, 16));
            CFRelease(fullTilesURLRef);
        }
    }
    CFRelease(tilesFilenamePrefixRef);
    // If there was a problem and this wasn't the default, go up and try again.
    if (!success && CFStringCompare(tilesNameRef, defaultTilesRef, 0) != kCFCompareEqualTo) {
        CFRelease(tilesNameRef);
        tilesNameRef = defaultTilesRef;
        goto tilesAgain;
    }

    // Set up Mini tiles
    success = FALSE;
    int miniTileSize = (blkSiz * 22) / 64;
    CFStringRef miniFilenamePrefixRef = (CFStringRef)CopyCatStrings(tilesNameRef, CFSTR("-Mini"));
    i = CFArrayGetCount(graphicsArrayRef) - 1;
    while (i >= 0) {
        CFStringRef anItemRef = CFArrayGetValueAtIndex(graphicsArrayRef, i--);
        if (CFStringHasPrefix(anItemRef, miniFilenamePrefixRef)) {
            CFURLRef fullTilesURLRef = CFURLCreateCopyAppendingPathComponent(nil, tilesBaseURL, anItemRef, false);
            success = (GetGraphicTiledFile(fullTilesURLRef, minitilesPort, miniTileSize, miniTileSize, 64, 1));
            CFRelease(fullTilesURLRef);
            i = -1;
        }
    }
    CFRelease(miniFilenamePrefixRef);
    if (!success) {   // if no mini file is available, generate mini tiles from full sized ones.
        Rect toRect;
        toRect.top = 0;
        toRect.bottom = miniTileSize;
        ForeColor(blackColor);
        BackColor(whiteColor);
        for (i = 0; i < 64; i++) {
            Rect fromRect = GetTileRectForIndex(i);
            toRect.left = i * miniTileSize;
            toRect.right = toRect.left + miniTileSize;
            //SetGWorld(minitilesPort, nil);
            CopyBits(LWPortCopyBits(tilesPort), LWPortCopyBits(minitilesPort), &fromRect, &toRect, srcCopy, nil);
            //SetGWorld(mainPort, nil);
        }
    }

    // Set up Mask
    success = FALSE;
    CFStringRef maskFilenamePrefixRef = (CFStringRef)CopyCatStrings(tilesNameRef, CFSTR("-Mask"));
    i = CFArrayGetCount(graphicsArrayRef) - 1;
    while (i >= 0) {
        CFStringRef anItemRef = CFArrayGetValueAtIndex(graphicsArrayRef, i--);
        if (CFStringHasPrefix(anItemRef, maskFilenamePrefixRef)) {
            CFURLRef fullTilesURLRef = CFURLCreateCopyAppendingPathComponent(nil, tilesBaseURL, anItemRef, false);
            success = (GetGraphicTiledFile(fullTilesURLRef, tilesMaskPort, blkSiz * 2, blkSiz * 2, 12, 16));
            CFRelease(fullTilesURLRef);
            i = -1;
        }
    }
    CFRelease(maskFilenamePrefixRef);
    if (!success) {   // if no mask file is available, nothing will get masked.
        SetGWorld(tilesMaskPort, nil);
        ForeColor(blackColor);
        Rect maskRect;
        SetRect(&maskRect, 0, 0, (blkSiz * 2) * 12, (blkSiz * 2) * 16);
        PaintRect(&maskRect);
        SetGWorld(mainPort, nil);
    }

    for (i = 0; i < 256; i++)
        gShapeSwapped[i] = false;

    // Set up Font
    CFStringRef fontNameRef = tilesNameRef;
FontAgain:
    gHasOddTextPort = FALSE;
    success = FALSE;
    CFStringRef FontFilenamePrefixRef = (CFStringRef)CopyCatStrings(fontNameRef, CFSTR("-Font"));
    CFStringRef FontFilenameOddPrefixRef = (CFStringRef)CopyCatStrings(fontNameRef, CFSTR("-OddFont"));
    i = CFArrayGetCount(graphicsArrayRef) - 1;
    while (i >= 0) {
        CFStringRef anItemRef = CFArrayGetValueAtIndex(graphicsArrayRef, i--);
        if (CFStringHasPrefix(anItemRef, FontFilenameOddPrefixRef)) {
            CFURLRef fullTilesURLRef = CFURLCreateCopyAppendingPathComponent(nil, tilesBaseURL, anItemRef, false);
            gHasOddTextPort = (GetGraphicTiledFile(fullTilesURLRef, textOddPort, blkSiz, blkSiz, 96, 1));
            CFRelease(fullTilesURLRef);
        }
        if (CFStringHasPrefix(anItemRef, FontFilenamePrefixRef)) {
            CFURLRef fullTilesURLRef = CFURLCreateCopyAppendingPathComponent(nil, tilesBaseURL, anItemRef, false);
            success = (GetGraphicTiledFile(fullTilesURLRef, textPort, blkSiz, blkSiz, 96, 1));
            CFRelease(fullTilesURLRef);
            i = -1;
        }
    }
    CFRelease(FontFilenamePrefixRef);
    CFRelease(FontFilenameOddPrefixRef);
    // If there was a problem and this wasn't the default, go up and try again.
    if (!success && CFStringCompare(fontNameRef, defaultTilesRef, 0) != kCFCompareEqualTo) {
        fontNameRef = defaultTilesRef;
        goto FontAgain;
    }

// Set up UI (Frames)
UIAgain:
    success = FALSE;
    CFStringRef UIFilenamePrefixRef = (CFStringRef)CopyCatStrings(tilesNameRef, CFSTR("-UI"));
    i = CFArrayGetCount(graphicsArrayRef) - 1;
    while (i >= 0) {
        CFStringRef anItemRef = CFArrayGetValueAtIndex(graphicsArrayRef, i--);
        if (CFStringHasPrefix(anItemRef, UIFilenamePrefixRef)) {
            CFURLRef fullTilesURLRef = CFURLCreateCopyAppendingPathComponent(nil, tilesBaseURL, anItemRef, false);
            success = (GetGraphicTiledFile(fullTilesURLRef, framePort, blkSiz, blkSiz, 16, 3));
            CFRelease(fullTilesURLRef);
            i = -1;
        }
    }
    CFRelease(UIFilenamePrefixRef);
    // If there was a problem and this wasn't the default, go up and try again.
    if (!success && CFStringCompare(tilesNameRef, defaultTilesRef, 0) != kCFCompareEqualTo) {
        CFRelease(tilesNameRef);
        tilesNameRef = defaultTilesRef;
        goto UIAgain;
    }

    CFRelease(tilesNameRef);
    CFRelease(graphicsArrayRef);
}

Rect GetTileRectForIndex(short index) {
    Rect theRect;

    theRect.top = (index % 16) * (blkSiz * 2);
    theRect.bottom = theRect.top + (blkSiz * 2);
    theRect.left = (index / 16) * (blkSiz * 4);
    if (gShapeSwapped[index])
        theRect.left += (blkSiz * 2);
    theRect.right = theRect.left + (blkSiz * 2);
    return theRect;
}

void GetPortraits(void) {
    CFURLRef imagesBaseURL = (CFURLRef)ResourcesDirectoryURL();
    CFURLRef portraitsURLRef = CFURLCreateCopyAppendingPathComponent(nil, imagesBaseURL, CFSTR("Portraits.png"), false);
    GetGraphicTiledFile(portraitsURLRef, portraitPort, blkSiz * 2, blkSiz * 3, 40, 1);
    CFRelease(portraitsURLRef);
}

void ScrollShape(short shape, short amount) { /* $48D9 */
    // uses black square 'below' final exodus frame as scratch (5,4) * (blksiz*2)
    Rect DestRect, myRect;

    SetGWorld(tilesPort, nil);
    myRect = GetTileRectForIndex(shape / 2);
    DestRect.top = 4 * (blkSiz * 2);
    DestRect.bottom = DestRect.top + (blkSiz * 2);
    DestRect.left = 5 * (blkSiz * 2);
    DestRect.right = DestRect.left + (blkSiz * 2);
    CopyBits(LWPortCopyBits(tilesPort), LWPortCopyBits(tilesPort), &myRect, &DestRect, srcCopy, nil);
    ScrollRect(&myRect, 0, amount, nil);
    myRect.bottom = myRect.top + amount;
    DestRect.top = DestRect.bottom - amount;
    CopyBits(LWPortCopyBits(tilesPort), LWPortCopyBits(tilesPort), &DestRect, &myRect, srcCopy, nil);
    SetGWorld(mainPort, nil); /*was mainDevice*/
}

void ExodusLights(void) {
    static unsigned char exoduslitez = 0;
    Rect FromRect, ToRect;

    //if (Party[3]!=3) return;
    animFlag[2]++;
    if (animFlag[2] > 1) {
        animFlag[2] = 0;
        return;
    }
    exoduslitez--;
    exoduslitez = exoduslitez & 0x03;

    FromRect.left = 5 * (blkSiz * 2);
    FromRect.top = exoduslitez * (blkSiz * 2);
    FromRect.right = FromRect.left + (blkSiz * 2);
    FromRect.bottom = FromRect.top + (blkSiz * 2);

    ToRect.left = 2 * (blkSiz * 2);
    ToRect.top = 15 * (blkSiz * 2);
    ToRect.right = ToRect.left + (blkSiz * 2);
    ToRect.bottom = ToRect.top + (blkSiz * 2);
    SetGWorld(tilesPort, nil);
    CopyBits(LWPortCopyBits(tilesPort), LWPortCopyBits(tilesPort), &FromRect, &ToRect, srcCopy, nil);
    SetGWorld(mainPort, mainDevice);
}

void SwapShape(unsigned short shape) {
    gShapeSwapped[shape / 2] = !gShapeSwapped[shape / 2];
}

// returns true if everything worked properly.
Boolean DrawNamedImage(CFStringRef imageName, CGrafPtr destWorld, const Rect *destRect) {
    Boolean result = FALSE;
    CFURLRef imagesBaseURL = (CFURLRef)ResourcesDirectoryURL();
    CFURLRef fullImageURLRef = CFURLCreateCopyAppendingPathComponent(nil, imagesBaseURL, imageName, false);
    FSRef fsr;
    if (CFURLGetFSRef(fullImageURLRef, &fsr)) {
        FSSpec fss;
        OSErr err = FSGetCatalogInfo(&fsr, kFSCatInfoNone, nil, nil, &fss, nil);
        if (err == noErr) {
            Rect destOffRect = *destRect;
            OffsetRect(&destOffRect, -destRect->left, -destRect->top);
            CGrafPtr offWorld;
            err = NewGWorld(&offWorld, 32, &destOffRect, nil, nil, 0);
            if (err == noErr) {
                ComponentInstance gi;
                err = GetGraphicsImporterForFile(&fss, &gi);
                if (err == noErr)
                    err = GraphicsImportSetBoundsRect(gi, &destOffRect);
                if (err == noErr)
                    err = GraphicsImportSetGWorld(gi, offWorld, nil);
                if (err == noErr)
                    err = GraphicsImportDraw(gi);
                result = (err == noErr);
                ForeColor(blackColor);
                BackColor(whiteColor);
                CopyBits(LWPortCopyBits(offWorld), LWPortCopyBits(destWorld), &destOffRect, destRect, srcCopy, nil);
                DisposeGWorld(offWorld);
            }
        }
    }
    CFRelease(fullImageURLRef);
    return result;
}

void DrawMap(unsigned char x, unsigned char y) {
    const char xhide[11] = {1, 1, 1, 1, 1, 0, -1, -1, -1, -1, -1};
    const char yhide[11] = {11, 11, 11, 11, 11, 0, -11, -11, -11, -11, -11};

    unsigned char xt, yt, xm, ym, offset2, value;

    unsigned char offset = 0;
    unsigned char numy = 11;
    for (ym = y - 5; numy > 0; ym++) {
        unsigned char numx = 11;
        for (xm = x - 5; numx > 0; xm++) {
            unsigned char val = GetXYVal(xm, ym);
            char variant = (val & 0x03);
            if (variant && val >= 92 && val <= 126)
                TileArray[offset++] = (((val / 4) - 23) * 2 + 79 + variant) * 2;
            else
                TileArray[offset++] = (val & 0xFC) >> 1;
            // if it's an I and not up against the edge of the map,
            if (val == 0xB8 && xm > 0 && xm < gCurMapSize - 1 && ym > 0) {   // letter I or Door
                // and the item left of it is not a letter
                unsigned char left = GetXYVal(xm - 1, ym);
                if (left < 0x98 || left > 0xE4 || left == 0xB8) {
                    // and the item right of it is not a letter
                    unsigned char right = GetXYVal(xm + 1, ym);
                    if (right < 0x98 || right > 0xE4 || right == 0xB8) {
                        // and the item above it is not a letter, turn it into a door.
                        unsigned char above = GetXYVal(xm, ym - 1);
                        if (above < 0x98 || above > 0xE4)
                            TileArray[offset - 1] = 0x5D;    // 0x5C normally for I
                    }
                }
            }
            numx--;
        }
        numy--;
    }
    Boolean flag1 = FALSE;
    offset = 120;
    xm = 10;
    ym = 10;
    while (ym < 11) {
        if (!flag1) {
            xt = xm;
            yt = ym;
            offset2 = offset;
        }
        value = offset2 + xhide[xt] + yhide[yt];
        if (value != 0x3C) {
            offset2 = value;
            value = TileArray[offset2];
            flag1 = FALSE;
            if ((value == 6) || (value == 8) || (value == 0x46) || (value == 0x48)) {
                TileArray[offset] = 0x48; /* Void */
            } else {
                xt = xt + xhide[xt];
                yt = yt + xhide[yt];
                flag1 = TRUE;
            }
        } else {
            flag1 = FALSE;
        }
        if (!flag1) {
            offset--;
            xm--;
            if (xm > 10) {
                xm = 10;
                ym--;
            }
        }
    }
    stx = x - 5;
    sty = y - 5;
    DrawTiles();
}

void DrawTiles(void) {
    unsigned char offset;
    Rect myRect, shapeRect, offRect;
    short lastTile, final, shapSize;

    if (gDone)
        return;
    MusicUpdate();

    SetGWorld(gamePort, nil);
    HideMonsters();

    offset = 0;
    lastTile = 255;
    final = blkSiz * 22;
    shapSize = blkSiz * 2;
    ForeColor(blackColor);
    BackColor(whiteColor);
    for (shapeRect.top = 0; shapeRect.top < final; shapeRect.top += shapSize) {
        shapeRect.bottom = shapeRect.top + shapSize;
        for (shapeRect.left = 0; shapeRect.left < final; shapeRect.left += shapSize) {
            shapeRect.right = shapeRect.left + shapSize;
            unsigned char tile = TileArray[offset];
            if (lastTile != tile) {
                myRect = GetTileRectForIndex(tile / 2);
                if (tile == 0x5D) {   // Door
                    myRect.left += shapSize;
                    myRect.right += shapSize;
                }
                lastTile = tile;
            }
            offRect = shapeRect;
            CopyBits(LWPortCopyBits(tilesPort), LWPortCopyBits(gamePort), &myRect, &offRect, srcCopy, nil);
            offset++;
        }
    }

    ShowMonsters();
    SetGWorld(mainPort, nil);

    LWGetPortBounds(gamePort, &myRect);
    shapeRect.left = myRect.left + blkSiz;
    shapeRect.right = myRect.right + blkSiz;
    shapeRect.top = myRect.top + blkSiz;
    shapeRect.bottom = myRect.bottom + blkSiz;
    CopyBits(LWPortCopyBits(gamePort), LWPortCopyBits(mainPort), &myRect, &shapeRect, srcCopy, nil);

    CursorUpdate();
    ForceUpdateMain();
}

void DrawMasked(unsigned short shape, unsigned short x, unsigned short y) {
    Rect FromRect, ToRect;
    short shapSize;

    shapSize = blkSiz * 2;
    FromRect = GetTileRectForIndex(shape / 2);
    if (shape == 0x5D) {   // door
        FromRect.left += shapSize;
        FromRect.right += shapSize;
    }
    ToRect.left = x * shapSize;
    ToRect.top = y * shapSize;
    ToRect.right = ToRect.left + shapSize;
    ToRect.bottom = ToRect.top + shapSize;
    CopyMask(LWPortCopyBits(tilesPort), LWPortCopyBits(tilesMaskPort), LWPortCopyBits(gamePort), &FromRect, &FromRect, &ToRect);
}

void DrawDemo(void) {
    static unsigned char *sDemoBgndTiles = nil;
    if (!sDemoBgndTiles) {
        sDemoBgndTiles = malloc(19 * 6);
        BlockMove(TileArray, sDemoBgndTiles, 19 * 6);
        sDemoBgndTiles[62] = 0;    // ship should be water.
    }

    Rect shapeRect, myRect;
    short demoffset, lastTile, shapSize;
    long gTime, storeUpdate;

    if (gDone)
        return;
    gTime = TickCount();
    ScrollThings();
    AnimateTiles();
    TwiddleFlags();
    demoffset = 0;
    ForeColor(blackColor);
    BackColor(whiteColor);
    shapSize = blkSiz * 2;
    lastTile = 255;
    for (shapeRect.top = blkSiz * 11; shapeRect.top < (blkSiz * 23); shapeRect.top += shapSize) {
        shapeRect.bottom = shapeRect.top + shapSize;
        for (shapeRect.left = blkSiz; shapeRect.left < (blkSiz * 39); shapeRect.left += shapSize) {
            shapeRect.right = shapeRect.left + shapSize;
            unsigned char thisTile = TileArray[demoffset];
            if (lastTile != thisTile) {
                if (thisTile >= 0x28 && thisTile <= 0x2e) {   // use "player" tiles for fighter/cleric/wiz/thief
                    thisTile += 88;
                }
                myRect = GetTileRectForIndex(thisTile / 2);
                lastTile = thisTile;
            }
            unsigned char bgndTile = sDemoBgndTiles[demoffset];
            if (bgndTile == thisTile)
                CopyBits(LWPortCopyBits(tilesPort), LWPortCopyBits(mainPort), &myRect, &shapeRect, srcCopy, nil);
            else {
                Rect bgndTileRect = GetTileRectForIndex(bgndTile / 2);
                CopyBits(LWPortCopyBits(tilesPort), LWPortCopyBits(mainPort), &bgndTileRect, &shapeRect, srcCopy, nil);
                CopyMask(LWPortCopyBits(tilesPort), LWPortCopyBits(tilesMaskPort), LWPortCopyBits(mainPort), &myRect, &myRect,
                         &shapeRect);
            }
            demoffset++;
        }
    }
    ForceUpdateMain();
    storeUpdate = gUpdateWhere;
    gUpdateWhere = 0;
    IdleUntil(gTime + 4);
    gUpdateWhere = storeUpdate;
}

char CursorKey(Boolean usePenLoc) {
    if (gDone)
        return 0;

    char theChar=0, shape;
    long time;
    short myX, myY, keepX, keepY;
    Point point;
    Rect CursorRect, myRect;
    Boolean GotKey;

    GotKey = FALSE;
    shape = 0;
    keepX = tx;
    keepY = ty;
    if (usePenLoc) {
        LWGetPortPenLocation(mainPort, &point);
        myX = point.h;
        myY = point.v;
    } else {
        myX = tx * blkSiz;
        myY = ty * blkSiz;
    }
    if (gUpdateWhere == 7)
        SaveWideArea();
    CursorUpdate();
    while (GotKey != TRUE && gDone != TRUE) {
        time = TickCount();
        shape++;
        if (shape > 6)
            shape = 0;
        SetRect(&myRect, myX, myY + 1, myX + blkSiz, myY + blkSiz + 1);
        SetRect(&CursorRect, shape * blkSiz, blkSiz, shape * blkSiz + blkSiz, blkSiz * 2);
        ForeColor(blackColor);
        BackColor(whiteColor);
        CopyBits(LWPortCopyBits(framePort), LWPortCopyBits(mainPort), &CursorRect, &myRect, srcCopy, nil);
        GotKey = GetKeyMouse(1);
        while (TickCount() <= time + 2) {
        }
        if (GotKey) {
            theChar = gKeyPress;
        }
        if (gKeyPress == 0) {
            GotKey = FALSE; /* click not xlated to char */
        }
    }
    tx = keepX;
    ty = keepY;
    ForeColor(blackColor);
    PaintRect(&myRect);
    ForeColor(whiteColor);
    return theChar;
}

void HandleUpdate(void) {
    SetGWorld(mainPort, nil); /*was mainDevice*/
    if (CFPreferencesGetAppBooleanValue(U3PrefFullScreen, kCFPreferencesCurrentApplication, NULL) && gShroudWindow != 0) {
        BeginUpdate(gShroudWindow);
        ClearShroud();
        EndUpdate(gShroudWindow);
    }
    BeginUpdate(gMainWindow);
    DrawFrame(gCurFrame);
    switch (gUpdateWhere) {
        case 0: break;
        case 1:
            CenterMessage(54, 23);
            DrawFramePiece(12, 12, 23);
            DrawFramePiece(13, 27, 23);
            DrawIntroPix();
            DrawIntro(5, 22);
            break;
        case 2: DrawDemoScreen(); break;
        case 3:
            TextScrollAreaUpdate();
            ShowChars(true);
            break;
        case 4:
            TextScrollAreaUpdate();
            DngInfo();
            ShowChars(true);
            DrawDungeon();
            break;
        case 5: DrawMenu(); break;
        case 6:
            DrawOrganizeMenu();
            DrawExodusPict();
            break;
        case 7:
            DrawExodusPict();
            WideAreaUpdate();
            break;
        case 8:
            TextScrollAreaUpdate();
            break;
        /*      case 9: // Ztats, now do same as default
        //  DrawFancyRecord(FALSE);
            DrawGamePortToMain(0);
            TextScrollAreaUpdate();
            ShowChars(false);
            break; */
        /*      case 10: // Pause, now do same as default too
            TextScrollAreaUpdate();
            DrawGamePortToMain(0); // instead of DrawPause();
            ShowChars(false);
            break; */
        default:                      // Whatever's in gamePort
            DrawGamePortToMain(0);    // instead of ImageDisplay(gUpdateWhere-19, FALSE);
            TextScrollAreaUpdate();
            ShowChars(true);
            break;
    }
    EndUpdate(gMainWindow);
}

void WideAreaUpdate(void) {
    Rect myRect;

    ForeColor(blackColor);
    BackColor(whiteColor);
    SetRect(&myRect, blkSiz, blkSiz * 11, blkSiz * 39, blkSiz * 23);
    CopyBits(LWPortCopyBits(gWidePort), LWPortCopyBits(mainPort), &myRect, &myRect, srcCopy, nil);
}

void SaveWideArea(void) {
    Rect myRect;

    ForeColor(blackColor);
    BackColor(whiteColor);
    SetRect(&myRect, blkSiz, blkSiz * 11, blkSiz * 39, blkSiz * 23);
    CopyBits(LWPortCopyBits(mainPort), LWPortCopyBits(gWidePort), &myRect, &myRect, srcCopy, nil);
}

void TextScrollAreaUpdate(void) {
    Rect myRect;

    ForeColor(blackColor);
    BackColor(whiteColor);
    SetRect(&myRect, blkSiz * 24, blkSiz * 17, blkSiz * 40, blkSiz * 24);
    CopyBits(LWPortCopyBits(updatePort), LWPortCopyBits(mainPort), &myRect, &myRect, srcCopy, nil);
}

void SaveScrollArea(void) {
    Rect myRect;

    ForeColor(blackColor);
    BackColor(whiteColor);
    SetRect(&myRect, blkSiz * 24, blkSiz * 17, blkSiz * 40, blkSiz * 24);
    CopyBits(LWPortCopyBits(mainPort), LWPortCopyBits(updatePort), &myRect, &myRect, srcCopy, nil);
}

void DemoUpdate(short ptr) {
    Boolean doUpdate = TRUE;
    char songnum[17] = {1, 2, 3, 4, 5, 6, 7, 8, 10, 11};

    if (gDone)
        return;
    if (gSongNext == gSongCurrent) {
        if (gDemoSong > 9)
            gDemoSong = 0;
        gSongNext = songnum[gDemoSong];
        gDemoSong++;
    }
    unsigned char where = *(*gDemoData + ptr);
    unsigned char what = *(*gDemoData + ptr + 512);
    unsigned char repet;

    if (where != 255) {
        TileArray[where] = what;
        repet = 1;
        if (what < 16)
            doUpdate = FALSE;
    } else
        repet = what;

    unsigned char i, j;
    if (doUpdate) {
        for (i = repet; i > 0; i--) {
            for (j = 1; j < 5; j++) {
                DrawDemo();
            }
        }
    }
}

void Stalagtites(void) {
    Rect ToRect;
    SetRect(&ToRect, blkSiz, blkSiz * 19, blkSiz * 39, blkSiz * 21.125);
    DrawNamedImage(CFSTR("Stalagtites.png"), mainPort, &ToRect);
}

void FadeOnExodusUltima(void) {
    register unsigned long offset;
    unsigned long lastByte, lastByte2;
    long error, pass, time, delayTime, frameTime;
    register int byte;
    Rect FromRect, ToRect;
    short chunk;
    Handle Sound;
    SndCommand soundCommand;

    gInterrupt = FALSE;
    Stalagtites();
    DrawIntro(2, -25);
    ForceUpdateMain();
    if (!gInterrupt) {
        delayTime = TickCount() + 80;
    }
    SetRect(&FromRect, 0, 0, blkSiz * 29.5, blkSiz * 8.375);
    ToRect = FromRect;
    OffsetRect(&ToRect, blkSiz * 5.25, blkSiz * 1.25);
    error = NewGWorld(&exodusToPort, 0, &FromRect, nil, nil, 0);
    if (error != 0)
        HandleError(error, 15, 0);
    exodusToPixMap = GetGWorldPixMap(exodusToPort);
    LockPixels(exodusToPixMap);
    DrawNamedImage(CFSTR("Exodus.png"), exodusFromPort, &FromRect);
    gFromPixMapBase = GetPixBaseAddr(exodusFromPixMap);
    gToPixMapBase = GetPixBaseAddr(exodusToPixMap);
    gFromRowByteCount = (0x7FFF & (**exodusFromPixMap).rowBytes);
    gToRowByteCount = (0x7FFF & (**exodusToPixMap).rowBytes);
    pass = -32767;
    lastByte = (FromRect.bottom * gFromRowByteCount);
    time = TickCount();
    for (offset = 0; offset < lastByte; offset++) {
        Random();
        byte = gToPixMapBase[offset];
    }
    time = TickCount() - time;
    if (time > 100)
        time = 100;
    if (time < 10)
        time = 10;
    time -= 10; /* result 0-90 */
    chunk = (time * (gFromRowByteCount / 500.0));
    if (chunk < 1)
        chunk = 1;
    if (gDepth == 32)
        chunk = (chunk / 4) + 4;
    if (!gInterrupt) {
        while (TickCount() < delayTime) {
            CheckInterrupted();
        }
    }
    Sound = GetResource('snd ', BASERES);
    HLock(Sound);
    lastByte2 = lastByte - chunk;
    soundCommand.cmd = initCmd;
    soundCommand.param1 = 0;
    soundCommand.param2 = initMono;
    for (byte = 1; byte <= gMaxChan; byte++) {
        SndDoImmediate(gSampChan[byte], &soundCommand);
        soundCommand.cmd = soundCmd;
        soundCommand.param1 = 0;
        soundCommand.param2 = (long)(*Sound + 0x14);
        SndDoImmediate(gSampChan[byte], &soundCommand);
    }
    Boolean soundInactive = CFPreferencesGetAppBooleanValue(U3PrefSoundInactive, kCFPreferencesCurrentApplication, NULL);
    while (pass < 32768 && !gInterrupt) {
        frameTime = TickCount() + 5;
        if (!soundInactive) {
            soundCommand.cmd = freqDurationCmd;
            soundCommand.param1 = 60;
            soundCommand.param2 = (0xFF00E0A0 | ((32768 - pass) / 2048));
            gCurChan++;
            if (gCurChan > gMaxChan)
                gCurChan = 1;
            SndDoImmediate(gSampChan[gCurChan], &soundCommand);
        }
        (gDepth > 8) ? (byte = 0) : (byte = 255);
        for (offset = 0; offset < lastByte; offset++)
            gToPixMapBase[offset] = byte;
        for (offset = 0; offset < lastByte2; offset += chunk) {
            if (Random() < pass)
                BlockMoveData(gFromPixMapBase + offset, gToPixMapBase + offset, chunk);
        }
        CopyBits(LWPortCopyBits(exodusToPort), LWPortCopyBits(mainPort), &FromRect, &ToRect, srcCopy, nil);
        ForceUpdateMain();
        CheckInterrupted();
        while (TickCount() < frameTime) {
            CheckInterrupted();
        }
        pass += 1536;
    }
    SetGWorld(mainPort, nil); /*was mainDevice*/
    CopyBits(LWPortCopyBits(exodusFromPort), LWPortCopyBits(mainPort), &FromRect, &ToRect, srcCopy, nil);
    ForceUpdateMain();
    UnlockPixels(exodusToPixMap);
    DisposeGWorld(exodusToPort);
    if (!gInterrupt) {
        delayTime = TickCount() + 60;
        while (TickCount() < delayTime) {
            CheckInterrupted();
        }
    }
    SetRect(&FromRect, 0, 0, blkSiz * 25.5, blkSiz * 5.625);
    ToRect = FromRect;
    OffsetRect(&ToRect, blkSiz * 7.25, blkSiz * 9.9375);
    error = NewGWorld(&logoToPort, 0, &FromRect, nil, nil, 0);
    if (error != 0)
        HandleError(error, 17, 0);
    logoToPixMap = GetGWorldPixMap(logoToPort);
    LockPixels(logoToPixMap);
    DrawNamedImage(CFSTR("UltimaLogo.png"), logoFromPort, &FromRect);
    SetGWorld(mainPort, nil);
    gFromPixMapBase = GetPixBaseAddr(logoFromPixMap);
    gToPixMapBase = GetPixBaseAddr(logoToPixMap);
    gFromRowByteCount = (0x7FFF & (**logoFromPixMap).rowBytes);
    gToRowByteCount = (0x7FFF & (**logoToPixMap).rowBytes);
    pass = -32767;
    lastByte = (FromRect.bottom * gFromRowByteCount);
    chunk = chunk / 2;
    if (chunk < 1)
        chunk = 1;
    if (gDepth == 32)
        chunk = (chunk / 4) + 4;
    lastByte2 = lastByte - chunk;
    while (pass < 32768 && !gInterrupt) {
        frameTime = TickCount() + 4;
        if (!soundInactive) {
            soundCommand.cmd = freqDurationCmd;
            soundCommand.param1 = 60;
            soundCommand.param2 = (0xFF00E0A0 | ((32768 - pass) / 2048));
            gCurChan++;
            if (gCurChan > gMaxChan)
                gCurChan = 1;
            SndDoImmediate(gSampChan[gCurChan], &soundCommand);
        }
        (gDepth > 8) ? (byte = 0) : (byte = 255);
        for (offset = 0; offset < lastByte; offset++) {
            gToPixMapBase[offset] = byte;
        }
        for (offset = 0; offset < lastByte2; offset += chunk) {
            if (Random() < pass)
                BlockMoveData(gFromPixMapBase + offset, gToPixMapBase + offset, chunk);
        }
        CopyBits(LWPortCopyBits(logoToPort), LWPortCopyBits(mainPort), &FromRect, &ToRect, srcCopy, nil);
        ForceUpdateMain();
        CheckInterrupted();
        while (TickCount() < frameTime) {
            CheckInterrupted();
        }
        pass += 1536;
    }
    SetGWorld(mainPort, nil);
    CopyBits(LWPortCopyBits(logoFromPort), LWPortCopyBits(mainPort), &FromRect, &ToRect, srcCopy, nil);
    ForceUpdateMain();
    UnlockPixels(logoToPixMap);
    DisposeGWorld(logoToPort);
    if (!gInterrupt) {
        delayTime = TickCount() + 120;
        while (TickCount() < delayTime) {
            CheckInterrupted();
        }
    }
    HUnlock(Sound);
}

void WriteLordBritish(void) {
    Handle sigdata;
    Rect rectum;
    long time;
    Boolean done, near;
    short value, cx, cy, offset;

    sigdata = GetResource('SGNT', BASERES);
    LoadResource(sigdata);
    SetRect(&rectum, blkSiz * 11.25, blkSiz * 15.75, blkSiz * 12.875, blkSiz * 17.1875);
    DrawNamedImage(CFSTR("By.png"), mainPort, &rectum);
    if (!gInterrupt) {
        time = TickCount() + 10;
        while (TickCount() < time) {
            CheckInterrupted();
        }
    }
    offset = 0;
    cx = 0;
    cy = 0;
    ForeColor(whiteColor);
    done = near = FALSE;
    while (!done) {
        PlotSig(cx * 2, cy * 2);
        time = TickCount() + 1;
        if (!gInterrupt && (offset % 2)) {
            ForceUpdateMain();
            while (TickCount() < time) {
                CheckInterrupted();
            }
        }
        value = (unsigned char)*(*sigdata + offset);
        offset++;
        if (value == 0) {
            PlotSig((cx * 2 - 1), (cy * 2 - 1));
            cx--;
            cy--;
        }
        if (value == 1) {
            PlotSig(cx * 2, (cy * 2 - 1));
            cy--;
        }
        if (value == 2) {
            PlotSig((cx * 2 + 1), (cy * 2 - 1));
            cx++;
            cy--;
        }
        if (value == 3) {
            PlotSig((cx * 2 - 1), cy * 2);
            cx--;
        }
        if (value == 4) {
            PlotSig((cx * 2 + 1), cy * 2);
            cx++;
        }
        if (value == 5) {
            PlotSig((cx * 2 - 1), (cy * 2 + 1));
            cx--;
            cy++;
        }
        if (value == 6) {
            PlotSig(cx * 2, (cy * 2 + 1));
            cy++;
        }
        if (value == 7) {
            PlotSig((cx * 2 + 1), (cy * 2 + 1));
            cx++;
            cy++;
        }
        if (value == 8) {
            cx = (signed char)*(*sigdata + offset);
            cy = (signed char)*(*sigdata + offset + 1);
            offset += 2;
        }
        if (value == 0xFF)
            done = TRUE;
    }
    // Draw credits
    SetRect(&rectum, blkSiz, blkSiz * 17.5625, blkSiz * 39, blkSiz * 19);
    DrawNamedImage(CFSTR("Credits.png"), mainPort, &rectum);
    ForceUpdateMain();
}

void PlotSig(short x, short y) {
    static float sScaler = -1.0;
    if (sScaler == -1.0)
        sScaler = (float)blkSiz / 16.0;
    Rect rectum;

    x += 244;
    y += 253;
    x = (int)truncf((float)x * sScaler);
    y = (int)truncf((float)y * sScaler);
    SetRect(&rectum, x, y, x + (blkSiz >> 2), y + (blkSiz >> 3));
    PaintRect(&rectum);
}

void FightScene(void) {
    short anim, x;
    const short animorder[5] = {0, 1, 2, 3, 2};
    //long          time;

    ForeColor(blackColor);
    BackColor(whiteColor);
    anim = 1;
    for (x = -25; x < 25; x += 2) {
        DrawIntro(animorder[anim], x);
        anim++;
        if (anim > 4)
            anim = 1;
    }
    for (x = 25; x > -25; x -= 2) {
        DrawIntro(animorder[anim], x);
        anim--;
        if (anim < 1)
            anim = 4;
    }
    for (x = -25; x < 10; x += 2) {
        DrawIntro(animorder[anim], x);
        anim++;
        if (anim > 4)
            anim = 1;
    }
    DrawIntro(animorder[anim], 10);
    DrawIntro(animorder[anim], 10);
    DrawIntro(animorder[anim], 10);
    DrawIntro(animorder[anim], 10);
    if (!gInterrupt) {
        PlaySoundFile(CFSTR("Immolate"), TRUE);    // was 0xE9
        DrawIntro(4, 4);
        ForceUpdateMain();
        ThreadSleepTicks(30);
    }
    DrawIntro(5, 22);
}

void DrawIntro(unsigned char shape, short offset) {
    long time;
    Rect myRect;
    if (!gInterrupt) {
        time = TickCount() + 8;
        float scaler = (float)blkSiz / 16.0;
        myRect.left = (192 + offset) * scaler;
        myRect.right = myRect.left + (255 * scaler);
        myRect.top = 335 * scaler;
        myRect.bottom = myRect.top + (32 * scaler);
        switch (shape) {
            case 1: DrawNamedImage(CFSTR("Opening_Anim_1.png"), mainPort, &myRect); break;
            case 2: DrawNamedImage(CFSTR("Opening_Anim_2.png"), mainPort, &myRect); break;
            case 3: DrawNamedImage(CFSTR("Opening_Anim_3.png"), mainPort, &myRect); break;
            case 4: DrawNamedImage(CFSTR("Opening_Anim_4.png"), mainPort, &myRect); break;
            case 5: DrawNamedImage(CFSTR("Opening_Anim_5.png"), mainPort, &myRect); break;
        }

        ForceUpdateMain();
        while (TickCount() < time) {
            CheckInterrupted();
            ThreadSleepTicks(1);
        }
    }
}

void CheckInterrupted(void) {
    EventRecord theEvent;
    WaitNextEvent(everyEvent, &theEvent, 1L, nil);
    switch (theEvent.what) {
        case mouseDown:
        case keyDown:
            gInterrupt = TRUE;
            Rect myRect;
            SetRect(&myRect, blkSiz, blkSiz * 20.9375, blkSiz * 39, blkSiz * 22.9375);
            ForeColor(blackColor);
            PaintRect(&myRect);
            break;
    }
}

void CreateIntroData(void) {
    Rect myRect;

    SetRect(&myRect, blkSiz * 1, blkSiz * 11, blkSiz * 36, blkSiz * 23);
    gError = NewGWorld(&demoPort, 0, &myRect, nil, nil, 0);
    if (gError)
        HandleError(gError, 1, 0);
    demoPixMap = GetGWorldPixMap(demoPort);
    LockPixels(demoPixMap);

    SetRect(&myRect, 0, 0, blkSiz * 29.5, blkSiz * 8.375);
    gError = NewGWorld(&exodusFromPort, 0, &myRect, nil, nil, 0);
    if (gError != 0)
        HandleError(gError, 14, 0);
    exodusFromPixMap = GetGWorldPixMap(exodusFromPort);
    LockPixels(exodusFromPixMap);

    SetRect(&myRect, 0, 0, blkSiz * 25.5, blkSiz * 5.625);
    gError = NewGWorld(&logoFromPort, 0, &myRect, nil, nil, 0);
    if (gError != 0)
        HandleError(gError, 16, 0);
    logoFromPixMap = GetGWorldPixMap(logoFromPort);
    LockPixels(logoFromPixMap);

    SetRect(&myRect, blkSiz, blkSiz * 11, blkSiz * 39, blkSiz * 23);
    gError = NewGWorld(&gWidePort, 1, &myRect, nil, nil, 0);
    if (gError != 0)
        HandleError(gError, 25, 0);
    gWidePixMap = GetGWorldPixMap(gWidePort);
    gError = LockPixels(gWidePixMap);
    if (gError == FALSE)
        HandleError(gError, 25, 0);
}

void DisposeIntroData(void) {
    UnlockPixels(demoPixMap);
    DisposeGWorld(demoPort);
    UnlockPixels(exodusFromPixMap);
    DisposeGWorld(exodusFromPort);
    UnlockPixels(logoFromPixMap);
    DisposeGWorld(logoFromPort);
    UnlockPixels(gWidePixMap);
    DisposeGWorld(gWidePort);
}

void DrawIntroPix(void) {
    Rect FromRect, ToRect;

    gInterrupt = TRUE;
    Stalagtites();
    SetRect(&FromRect, 0, 0, blkSiz * 29.5, blkSiz * 8.375);
    ToRect = FromRect;
    OffsetRect(&ToRect, blkSiz * 5.25, blkSiz * 1.25);
    CopyBits(LWPortCopyBits(exodusFromPort), LWPortCopyBits(mainPort), &FromRect, &ToRect, srcCopy, nil);
    SetRect(&FromRect, 0, 0, blkSiz * 25.5, blkSiz * 5.625);
    ToRect = FromRect;
    OffsetRect(&ToRect, blkSiz * 7.25, (blkSiz * 10) - 1);
    CopyBits(LWPortCopyBits(logoFromPort), LWPortCopyBits(mainPort), &FromRect, &ToRect, srcCopy, nil);
    WriteLordBritish();
    gInterrupt = FALSE;
    DrawIntro(5, 22);
}

void DrawExodusPict(void) {
    Rect myRect;

    SetRect(&myRect, 0, 0, blkSiz * 29.5, blkSiz * 8.375);
    OffsetRect(&myRect, blkSiz * 5.25, blkSiz * 1.25);
    DrawNamedImage(CFSTR("Exodus.png"), mainPort, &myRect);
}

void DrawMenu(void) {
    Rect myRect;
    short centre;
    RGBColor color;
    Boolean classic = CFPreferencesGetAppBooleanValue(U3PrefClassicAppearance, kCFPreferencesCurrentApplication, NULL);

    if (gDone)
        return;
    myRect.left = blkSiz;
    myRect.right = blkSiz * 36;
    myRect.top = blkSiz * 10;
    myRect.bottom = blkSiz * 23;
    ForeColor(blackColor);
    PaintRect(&myRect);
    ForeColor(whiteColor);
    DrawExodusPict();
    if (classic) {
        CenterMessage(27, 11);    // From the depths of hell
        CenterMessage(28, 12);    // he comes for vengeance!@#
    } else {
        CenterMessage(97, 11);    // From the depths of hell
        CenterMessage(98, 12);    // he comes for vengeance!@#
    }

    /*  CenterMessage(32,15); // old menu commands
    CenterMessage(29,17);
    CenterMessage(30,18);
    CenterMessage(31,19); */
    DrawButton(0, FALSE, FALSE);    // Return to View
    DrawButton(1, FALSE, FALSE);    // Organize Party
    DrawButton(2, FALSE, FALSE);    // Journey Onward
    DrawButton(8, FALSE, FALSE);    // Adjust Options

    // The copyright message is displayed in "modern" appearance no matter what.
    if (classic) {
        CFPreferencesSetAppValue(U3PrefClassicAppearance, kCFBooleanFalse, kCFPreferencesCurrentApplication);
        CenterMessage(26, 22);
        CFPreferencesSetAppValue(U3PrefClassicAppearance, kCFBooleanTrue, kCFPreferencesCurrentApplication);
    } else
        CenterMessage(26, 22);    // � LB

    LWGetScreenRect(&myRect);
    LWValidWindowRect(gMainWindow, &myRect);
}

void DrawDemoScreen(void) {
    Rect myRect;
    if (gDone)
        return;
    DrawExodusPict();
    CenterMessage(55, 10);
    DrawFramePiece(12, 14, 10);
    DrawFramePiece(13, 25, 10);
    //CenterMessage(54,23);
    //DrawFramePiece(12,12,23);
    //DrawFramePiece(13,27,23);
    DrawDemo();
    LWGetScreenRect(&myRect);
    LWValidWindowRect(gMainWindow, &myRect);
}

void DrawMiniMap(void) {
    unsigned char value;
    short x, y, mouseStore, UpdateStore, minSize;
    CursHandle watchCurs;
    Rect miniRect, myRect, mapRect;
    RGBColor color;
    RgnHandle saveRgn;

    saveRgn = NewRgn();
    mouseStore = gMouseState;
    gMouseState = 0;
    GetClip(saveRgn);
    SetRect(&myRect, blkSiz, blkSiz, blkSiz * 23, blkSiz * 23);
    ClipRect(&myRect);
    minSize = (blkSiz * 22) / gCurMapSize;
    if (minSize == 0)
        minSize++;
    gSongCurrent = 10;
    MusicUpdate();
    watchCurs = GetCursor(watchCursor);
    SetCursor(*watchCurs);
    ForeColor(blackColor);
    mapRect.left = (blkSiz * 12) - (minSize * (gCurMapSize / 2));
    mapRect.right = mapRect.left + (minSize * gCurMapSize);
    mapRect.top = mapRect.left;
    mapRect.bottom = mapRect.right;
    PaintRect(&mapRect);
    color.blue = color.green = color.red = 24576;
    OpColor(&color);
    PenMode(blend);
    SetRect(&myRect, mapRect.right, mapRect.top + blkSiz, mapRect.right + blkSiz, mapRect.bottom + blkSiz);
    PaintRect(&myRect);
    SetRect(&myRect, mapRect.left + blkSiz, mapRect.bottom, mapRect.right, mapRect.bottom + blkSiz);
    PaintRect(&myRect);
    SetRect(&myRect, mapRect.right, mapRect.top + (blkSiz / 2), mapRect.right + (blkSiz / 2), mapRect.bottom + (blkSiz / 2));
    PaintRect(&myRect);
    SetRect(&myRect, mapRect.left + (blkSiz / 2), mapRect.bottom, mapRect.right, mapRect.bottom + (blkSiz / 2));
    PaintRect(&myRect);
    ForeColor(whiteColor);
    PenMode(srcCopy);
    myRect = mapRect;
    InsetRect(&myRect, -1, -1);
    FrameRect(&myRect);
    InsetRect(&myRect, -1, -1);
    ForeColor(blackColor);
    FrameRect(&myRect);
    miniRect.top = 0;
    miniRect.bottom = minSize;
    for (y = 0; y < gCurMapSize; y++) {
        myRect.top = y * minSize + mapRect.top;
        myRect.bottom = myRect.top + minSize;
        for (x = 0; x < gCurMapSize; x++) {
            value = GetXYVal(x, y) / 4;
            myRect.left = x * minSize + mapRect.left;
            myRect.right = myRect.left + minSize;
            miniRect.left = value * minSize;
            miniRect.right = miniRect.left + minSize;
            CopyBits(LWPortCopyBits(minitilesPort), LWPortCopyBits(mainPort), &miniRect, &myRect, srcCopy, nil);
        }
    }
    DrawGamePortToMain(1);
    SetClip(saveRgn);
    DisposeRgn(saveRgn);
    UpdateStore = gUpdateWhere;
    gUpdateWhere = 20;
    myRect.left = xpos * minSize + mapRect.left;
    myRect.right = myRect.left + minSize;
    myRect.top = ypos * minSize + mapRect.top;
    myRect.bottom = myRect.top + minSize;
    InitCursor();
    const unsigned short fadeJump = 4096;
    const unsigned short minBrightness = fadeJump * 2;
    const unsigned short maxBrightness = 65535 - fadeJump;
    Boolean fadeToWhite = TRUE;
    color.red = color.green = color.blue = maxBrightness;
    while (!GetKeyMouse(2)) {
        unsigned short value = color.red;
        if (value <= minBrightness || value >= maxBrightness)
            fadeToWhite = !fadeToWhite;
        if (fadeToWhite)
            value += fadeJump;
        else
            value -= fadeJump;
        color.red = color.green = color.blue = value;
        RGBForeColor(&color);
        PaintRect(&myRect);
        ThreadSleepTicks(2);
    }
    gUpdateWhere = UpdateStore;
    gMouseState = mouseStore;
    CursorUpdate();
}

void DrawMiniDng(unsigned char mode) { /* $8F9A */
    unsigned char value, done, chr, under=0;
    short updateStore, mouseStore;

    mouseStore = gMouseState;
    gMouseState = 0;
    CursorUpdate();
    gSongCurrent = gSongNext = 0;
    gSongCurrent = 10;
    gSongNext = 4;
    ClearTiles();
    for (ys = 0; ys < 16; ys++) {
        for (xs = 0; xs < 16; xs++) {
            value = GetXYDng(xs, ys);
            chr = 7;
            if (value == 0xC0)
                chr = 0;
            if (value == 0xA0)
                chr = 1;
            if (value == 0x80)
                chr = 2;
            if (value == 0x30)
                chr = 3;
            if (value == 0x20)
                chr = 5;
            if (value == 0x10)
                chr = 4;
            if (value == 0x00)
                chr = 6;
            DngPiece(chr);
            if (xs == xpos && ys == ypos)
                under = chr;
        }
    }
    updateStore = gUpdateWhere;
    gUpdateWhere = 20;
    DrawGamePortToMain(1);
    done = 0;
    xs = xpos;
    ys = ypos;
    if (mode == 1) {
        DngPiece(8);
        return;
    }
    while (done == 0) {
        DngPiece(8);
        ForceUpdateMain();
        ThreadSleepTicks(10);
        DngPiece(under);
        ForceUpdateMain();
        ThreadSleepTicks(10);
        done = GetKeyMouse(0) | Button();
    }
    gUpdateWhere = updateStore;
    gMouseState = mouseStore;
    CursorUpdate();
    UPrintWin("\p\n");
    DrawDungeon();
}

void DngPiece(unsigned char shape) {
    Rect myRect, DngTileRect;
    SetRect(&myRect, (xs + 4) * blkSiz, (ys + 4) * blkSiz, (xs + 4) * blkSiz + blkSiz, (ys + 4) * blkSiz + blkSiz);
    SetRect(&DngTileRect, (shape + 7) * blkSiz, blkSiz, (shape + 7) * blkSiz + blkSiz, blkSiz * 2);
    ForeColor(blackColor);
    BackColor(whiteColor);
    CopyBits(LWPortCopyBits(framePort), LWPortCopyBits(mainPort), &DngTileRect, &myRect, srcCopy, nil);
}

void SetupTileToGame(void) {
    gFromPixMapBase = GetPixBaseAddr(tilesPixMap);
    gToPixMapBase = GetPixBaseAddr(gamePixMap);
    gFromRowByteCount = (0x7FFF & (**tilesPixMap).rowBytes);
    gToRowByteCount = (0x7FFF & (**gamePixMap).rowBytes);
    directOffsetX = 0;
    directOffsetY = 0;
}

Boolean SetupGameToDisplay(void) {
    gFromPixMapBase = GetPixBaseAddr(gamePixMap);
    directPixMap = (**mainDevice).gdPMap;
    gToPixMapBase = GetPixBaseAddr(directPixMap);
    gFromRowByteCount = (0x7FFF & (**gamePixMap).rowBytes);
    gToRowByteCount = (0x7FFF & (**directPixMap).rowBytes);
    SetupOffset();
    if (directOffsetY < 32) {
        gInBackground = TRUE;
        return TRUE;
    }
    return FALSE;
}

Boolean SetupTileToDisplay(void) {
    gFromPixMapBase = GetPixBaseAddr(tilesPixMap);
    directPixMap = (**mainDevice).gdPMap;
    gToPixMapBase = GetPixBaseAddr(directPixMap);
    gFromRowByteCount = (0x7FFF & (**tilesPixMap).rowBytes);
    gToRowByteCount = (0x7FFF & (**directPixMap).rowBytes);
    SetupOffset();
    if (directOffsetY < 32) {
        gInBackground = TRUE;
        return TRUE;
    }
    return FALSE;
}

Boolean SetupFrameToDisplay(void) {
    gFromPixMapBase = GetPixBaseAddr(framePixMap);
    directPixMap = (**mainDevice).gdPMap;
    gToPixMapBase = GetPixBaseAddr(directPixMap);
    gFromRowByteCount = (0x7FFF & (**framePixMap).rowBytes);
    gToRowByteCount = (0x7FFF & (**directPixMap).rowBytes);
    SetupOffset();
    if (directOffsetY < 32) {
        gInBackground = TRUE;
        return TRUE;
    }
    return FALSE;
}

void SetupOffset(void) {
    Rect tPix;
    LWGetWindowBounds(gMainWindow, &tPix);
    directOffsetX = tPix.left;
    directOffsetY = tPix.top;
}

void RectCopy(Rect tSourceRect, Rect tDestRect, Boolean hideCurs) {
    Boolean cursorOverlap = false;
    Rect intersectRect;
    register long fromOffset, toOffset;
    register short yRect, width;

    tDestRect.top += directOffsetY;
    tDestRect.bottom += directOffsetY;
    tDestRect.left += directOffsetX;
    tDestRect.right += directOffsetX;
    if (hideCurs) {
        if (SectRect(&tDestRect, &mouseRect, &intersectRect)) {
            cursorOverlap = TRUE;
            HideCursor();
        } else {
            cursorOverlap = FALSE;
        }
    }
    fromOffset = (tSourceRect.top * gFromRowByteCount) + tSourceRect.left;
    toOffset = (tDestRect.top * gToRowByteCount) + tDestRect.left;
    yRect = tSourceRect.top;
    width = tSourceRect.right - tSourceRect.left;
    while (yRect < tSourceRect.bottom) {
        yRect++;
        BlockMoveData(gFromPixMapBase + fromOffset, gToPixMapBase + toOffset, width);
        fromOffset += gFromRowByteCount;
        toOffset += gToRowByteCount;
    }
    if (cursorOverlap)
        ShowCursor();
}

void HideMonsters(void) {
    short mon, xm, ym, offset, value;
    if (Party[3] == 1)
        return;
    if (Party[3] != 0x80) {
        // first hide the actual monster tiles.
        for (mon = 0; mon < 32; mon++) {
            if (Monsters[mon] != 0) {
                xm = (Monsters[mon + XMON] - stx);
                if (Party[3] == 0)
                    xm = MapConstrain(xm);
                ym = (Monsters[mon + YMON] - sty);
                if (Party[3] == 0)
                    ym = MapConstrain(ym);
                if (xm > -1 && xm < 11 && ym > -1 && ym < 11) {
                    offset = ym * 11 + xm;
                    if (TileArray[offset] != 72 && (TileArray[offset] < 120 || TileArray[offset] > 122)) {
                        unsigned char tileChar = TileArray[offset];
                        maskRestoreArray[offset] = TileArray[offset];
                        TileArray[offset] = Monsters[TILEON + mon] / 2;
                        maskArray[offset] = tileChar;    //Monsters[mon];
                    }
                }
            }
        }
        // now hide anything else we'd like to show through.
        offset = 0;
        for (ym = 0; ym < 11; ym++) {
            for (xm = 0; xm < 11; xm++) {
                value = TileArray[offset];
                switch (value) {
                    case 0x22:    // Jester in castle on water (breaks the Talk cursor!)
                        maskRestoreArray[offset] = 0x22;
                        maskArray[offset] = 0x22;
                        TileArray[offset] = 0;
                        break;
                    case 0x74:    // Snake Bottom
                        maskRestoreArray[offset] = value;
                        maskArray[offset] = value;
                        TileArray[offset] = 0;
                        break;
                    case 0x76:    // Snake Top
                        maskRestoreArray[offset] = value;
                        maskArray[offset] = value;
                        TileArray[offset] = 0;
                        break;
                    case 0x7C:    // Shrine
                        maskRestoreArray[offset] = value;
                        maskArray[offset] = value;
                        TileArray[offset] = GetXYVal(xpos + xm - 6, ypos + ym - 5) / 2;
                        //TileArray[offset] = TileArray[offset-1];
                        break;
                    case 0x16:    // Frigate
                        maskRestoreArray[offset] = value;
                        maskArray[offset] = value;
                        TileArray[offset] = 0;
                        break;
                    case 0x12:    // Chest 1 or
                    case 0x13:    // Chest 2
                        maskRestoreArray[offset] = value;
                        maskArray[offset] = 0x12;
                        value = (GetXYVal(xpos + xm - 5, ypos + ym - 5) & 0x3) * 2;
                        if (value == 0)
                            value = 0x10;
                        TileArray[offset] = value;
                        break;
                    case 0x14:    // Horse
                        maskRestoreArray[offset] = value;
                        maskArray[offset] = value;
                        TileArray[offset] = 2;
                        break;
                    case 0x18:    // Whirlpool
                        maskRestoreArray[offset] = value;
                        maskArray[offset] = value;
                        TileArray[offset] = 0;
                        break;
                    case 0x5D:    // Door
                        maskRestoreArray[offset] = value;
                        maskArray[offset] = value;
                        int mon = MonsterHere(xpos + xm - 6, ypos + ym - 5);
                        short neighbor = (mon < 255) ? Monsters[mon + TILEON] / 2 : GetXYVal(xpos + xm - 6, ypos + ym - 5) / 2;
                        if (neighbor > 0x10)
                            neighbor = 0x02;
                        TileArray[offset] = neighbor;
                        break;
                    case 0x78:    // Magic ball
                    case 0x7A:    // Fire ball
                        maskRestoreArray[offset] = value;
                        maskArray[offset] = value;
                        TileArray[offset] = gBallTileBackground;
                }
                offset++;
            }
        }

    } else {   // it's combat, hide the monsters and the players.
        for (mon = 0; mon < 8; mon++) {
            if (MonsterHP[mon] > 0) {
                xm = MonsterX[mon];
                ym = MonsterY[mon];
                if (GetXYTile(xm, ym) < 120 || GetXYTile(xm, ym) > 122)
                    PutXYTile(MonsterTile[mon], xm, ym);
            }
        }
        for (mon = 0; mon < 4; mon++) {
            if (CheckAlive(mon)) {
                xm = CharX[mon];
                ym = CharY[mon];
                value = GetXYTile(xm, ym);
                if (value != 0x78 && value != 0x7A)
                    PutXYTile(CharTile[mon], xm, ym);
            }
        }
        // other things to hide in combat
        offset = 0;
        for (ym = 0; ym < 11; ym++) {
            for (xm = 0; xm < 11; xm++) {
                value = TileArray[offset];
                switch (value) {
                    case 0x78:    // Magic ball
                    case 0x7A:    // Fire ball
                        maskRestoreArray[offset] = value;
                        maskArray[offset] = value;
                        TileArray[offset] = gBallTileBackground;
                        break;
                }
                offset++;
            }
        }
    }
}

void ShowMonsters(void) {
    short mon, xm, ym, value, offset;
    if (Party[3] == 1)
        return;

    // Handle globally masked items.
    offset = 0;
    for (ym = 0; ym < 11; ym++) {
        for (xm = 0; xm < 11; xm++) {
            if (maskArray[offset] != 255) {
                TileArray[offset] = maskRestoreArray[offset];
                DrawMasked(maskArray[offset], xm, ym);
                maskArray[offset] = 255;
            }
            offset++;
        }
    }

    // Then either the outdoor party symbol ...
    if (Party[3] != 0x80) {
        if (TileArray[0x3C] < 120 || TileArray[0x3C] > 122) {
            if (Party[1] == 0x14 && gHorseFacingEast)
                gShapeSwapped[10] = true;
            DrawMasked(Party[1], 5, 5);
            gShapeSwapped[10] = false;
        }
    } else {   // ... or in combat, draw masked mons & chars, restore tiles.
        for (mon = 0; mon < 8; mon++) {
            if (MonsterHP[mon] > 0) {
                xm = MonsterX[mon];
                ym = MonsterY[mon];
                unsigned char tileValue = gMonType;    // tile * 2
                if (gMonVarType && gMonType >= 46 && gMonType <= 63)
                    tileValue = (((gMonType / 2) - 23) * 2 + 79 + gMonVarType) * 2;
                if (GetXYTile(xm, ym) < 120 || GetXYTile(xm, ym) > 122)
                    DrawMasked(tileValue, xm, ym);
                PutXYTile(gMonType, xm, ym);
            }
        }
        for (mon = 0; mon < 4; mon++) {
            if (CheckAlive(mon) && (mon + 1) != cHide) {
                xm = CharX[mon];
                ym = CharY[mon];
                value = GetXYTile(xm, ym);
                if (value != 0x78 && value != 0x7A) {
                    DrawMasked(CharShape[mon], xm, ym);
                    PutXYTile(CharShape[mon], xm, ym);
                }
            }
        }
    }
}

void DrawGamePortToMain(Boolean which) {
    Rect fromRect, toRect;

    fromRect.left = 0;
    fromRect.top = 0;
    fromRect.right = blkSiz * 22;
    fromRect.bottom = blkSiz * 22;
    toRect = fromRect;
    OffsetRect(&toRect, blkSiz, blkSiz);
    if (which) {
        CopyBits(LWPortCopyBits(mainPort), LWPortCopyBits(gamePort), &toRect, &fromRect, srcCopy, nil);
    } else {
        CopyBits(LWPortCopyBits(gamePort), LWPortCopyBits(mainPort), &fromRect, &toRect, srcCopy, nil);
    }
}

void DrawPortrait(char charNum, CGrafPtr destWorld) {   // 0-3
    short error, rosNum, value, clss, rce, sx;
    char charRaces[5] = {'H', 'E', 'D', 'B', 'F'};
    char usePortrait[12] = {0, 1, 2, 3, 0, 3, 2, 2, 1, 2, 0, 0};
    Rect fromRect, toRect, offRect;
    CGrafPtr offPort;
    GrafPtr curPort;
    RGBColor color;

    GetPort(&curPort);
    rosNum = Party[7 + charNum];
    for (value = 0; value < 11; value++) {
        if (Player[rosNum][23] == careerTable[value])
            clss = usePortrait[value];
    }
    for (value = 0; value < 5; value++) {
        if (Player[rosNum][22] == charRaces[value])
            rce = value;
    }
    sx = 0;
    if (Player[rosNum][24] == 'F')
        sx = 1;
    fromRect.left = ((clss * 10) + (sx * 5) + rce) * (blkSiz * 2);
    fromRect.right = fromRect.left + (blkSiz * 2);
    fromRect.top = 0;
    fromRect.bottom = blkSiz * 3;

    SetRect(&toRect, 0, 0, blkSiz * 2, blkSiz * 3);
    SetRect(&offRect, 0, 0, blkSiz * 2, blkSiz * 3);
    error = 1;
    BackColor(whiteColor);
    if (Player[rosNum][17] != 'G')
        error = NewGWorld(&offPort, 32, &offRect, nil, nil, 0);
    if (!error) {
        SetGWorld(offPort, nil);
        if (Player[rosNum][17] == 'P')
            ForeColor(greenColor);
        if (Player[rosNum][17] == 'D')
            ForeColor(redColor);
        if (Player[rosNum][17] == 'A')
            ForeColor(whiteColor);
        PaintRect(&offRect);
        ForeColor(blackColor);
        color.red = color.green = color.blue = 32768;
        OpColor(&color);
        CopyBits(LWPortCopyBits(portraitPort), LWPortCopyBits(offPort), &fromRect, &offRect, blend, nil);
        SetGWorld(mainPort, nil);
        CopyBits(LWPortCopyBits(offPort), LWPortCopyBits(destWorld), &offRect, &toRect, ditherCopy, nil);
        DisposeGWorld(offPort);
    } else {    // just do it the old way
        ForeColor(blackColor);
        if (Player[rosNum][17] == 'P') {
            color.red = 0;
            color.blue = 8192;
            color.green = 24576;
            RGBForeColor(&color);
        }    // bl was 0 gr was 24576
        if (Player[rosNum][17] == 'D')
            ForeColor(redColor);
        if (Player[rosNum][17] == 'A') {
            color.red = color.green = color.blue = 24576;
            RGBForeColor(&color);
        }
        CopyBits(LWPortCopyBits(portraitPort), LWPortCopyBits(destWorld), &fromRect, &toRect, srcCopy, nil);
    }
    SetPort(curPort);
}
