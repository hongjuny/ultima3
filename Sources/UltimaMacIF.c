// Mac interface related routines

#import "UltimaMacIF.h"

#import "UltimaIncludes.h"
#import "CarbonShunts.h"
#import "CocoaBridge.h"
#import "LWAboutWindow.h"
#import "UltimaGraphics.h"
#import "UltimaMain.h"
#import "UltimaMisc.h"
#import "UltimaNew.h"
#import "UltimaSound.h"
#import "UltimaSpellCombat.h"
#import "UltimaText.h"

extern EventRecord      gTheEvent;
extern Rect             gDragRect;
extern Boolean          gPaused, gDone, gMenuDone, gSoundIncapable, gMusicIncapable;
extern Boolean          gHideBackground, gInBackground, gAbort, gAutoCombat, gInterrupt;
extern short            gDepth, mouseX, mouseY, zp[255], gRefChange, gPauseChange;
extern short            gUpdateWhere, gUpdateStore, gCurImage, gOrgDepth;
extern MenuHandle       gAppleMenu, gFileMenu, gEditMenu, gSpecialMenu, gRefMenu;
extern WindowPtr        gMainWindow, gShroudWindow;
extern RgnHandle        UpdateRgn;
extern CGrafPtr         portraitPort, tilesPort, demoPort, framePort, tilesMaskPort;
extern CGrafPtr         gamePort, mainPort, minitilesPort, textPort, textOddPort, updatePort, gWidePort;
extern PixMapHandle     portraitPixMap, gWidePixMap, updatePixMap, tilesMaskPixMap, tilesPixMap;
extern PixMapHandle     gamePixMap, framePixMap, minitilesPixMap, textPixMap, mainPixMap;
extern PixMapHandle     directPixMap;
extern GDHandle         mainDevice;
extern unsigned char    Party[64], Player[21][65], stx, sty, gCurFrame, gStoreDirect;
extern char             g835E, gKeyPress;
extern unsigned char    careerTable[12], CharX[4], CharY[4], Macro[32], TileArray[128];
extern short            gCurCursor, gChnum, gMouseState, gCurMouseDir, gTorch;
extern int              xpos, ypos, xs, ys;
extern short            blkSiz;
extern short            gSongCurrent, gSongNext, gSongPlaying;
extern UniversalProcPtr DialogFilterProc;
extern long             lastSaveNumberOfMoves;

Boolean                 gStatsActive=false, gWasFullScreen=false, gIgnoreNextWakeHibernate=false;
Boolean                 gUnusualSize;
RgnHandle               gGrayRgn=nil;
short                   chStatsCur, gOrgctSeed, gBarHeight=0;
short                   gCurNumWeapons, gCurNumArmours;
char                    gCurWeapons[32], gCurArmours[32];
MCTableHandle           orgMenuColors;
CFMutableArrayRef       gTilePaths = nil;

// ----------------------------------------------------------------------
// Local prototypes

void UpdateGameOptionsDisplay(DialogPtr theDialog);
void SetUpTilesMenu(ControlHandle ctrl);
void ForceGameWindowUpdate(void);
void ForceAllOnScreen(void);
void HandleAppleChoice(int theItem);
void HandleFileChoice(int theItem);
void HandleSpecialChoice(int theItem);
void HandleReferenceChoice(int theItem);
void AboutUltima3(void);
void DoPause(void);
void DrawPause(void);
void ReflectNewCursor(short newCursor);

// ----------------------------------------------------------------------

void ReflectPrefs(void) {
    if (CFPreferencesGetAppBooleanValue(U3PrefFullScreen, kCFPreferencesCurrentApplication, NULL))
        MyShowMenuBar();
    CheckMenuItem(gSpecialMenu, SOUNDID,
                  !CFPreferencesGetAppBooleanValue(U3PrefSoundInactive, kCFPreferencesCurrentApplication, NULL));
    CheckMenuItem(gSpecialMenu, MUSICID,
                  !CFPreferencesGetAppBooleanValue(U3PrefMusicInactive, kCFPreferencesCurrentApplication, NULL));
    CheckMenuItem(gSpecialMenu, SPEECHID,
                  !CFPreferencesGetAppBooleanValue(U3PrefSpeechInactive, kCFPreferencesCurrentApplication, NULL));
    CheckMenuItem(gSpecialMenu, CONSTRAINID,
                  !CFPreferencesGetAppBooleanValue(U3PrefSpeedUnconstrain, kCFPreferencesCurrentApplication, NULL));
    CheckMenuItem(gSpecialMenu, AUTOCOMBATID,
                  !CFPreferencesGetAppBooleanValue(U3PrefManualCombat, kCFPreferencesCurrentApplication, NULL));
    CheckMenuItem(gSpecialMenu, DOUBLESIZEID,
                  !CFPreferencesGetAppBooleanValue(U3PrefOriginalSize, kCFPreferencesCurrentApplication, NULL));
    Boolean isFullScreen = CFPreferencesGetAppBooleanValue(U3PrefFullScreen, kCFPreferencesCurrentApplication, NULL);
    CheckMenuItem(gSpecialMenu, FULLSCREENID, isFullScreen);
    if (isFullScreen)
        LWDisableMenuItem(gSpecialMenu, DOUBLESIZEID);
    else
        LWEnableMenuItem(gSpecialMenu, DOUBLESIZEID);
    ForceAllOnScreen();
    DrawMenuBar();
}

void DisableMenus(void) {
    LWDisableMenuItem(gAppleMenu, 0);
    LWDisableMenuItem(gFileMenu, 0);
    LWDisableMenuItem(gSpecialMenu, 0);
    DrawMenuBar();
}

void EnableMenus(void) {
    LWEnableMenuItem(gAppleMenu, 0);
    LWEnableMenuItem(gFileMenu, 0);
    LWEnableMenuItem(gSpecialMenu, 0);
    DrawMenuBar();
}

Boolean QuitDialog(void) {
    long curNumberOfMoves = Party[14] * 1000000 + Party[13] * 10000 + Party[12] * 100 + Party[11];
    if (curNumberOfMoves <= (lastSaveNumberOfMoves + 4))
        return true;

    Str255 where;
    where[0] = 0;
    short button, mouseStateStore;
    if (Party[3] == 0 && gCurFrame == 1) {
        mouseStateStore = gMouseState;
        gMouseState = 0;
        CursorUpdate();
        button = Alert(BASERES + 4, NIL_PTR);
        gMouseState = mouseStateStore;
        if (button == 1) {
            QuitSave(1);
            return TRUE;
        }
        if (button == 2) {
            return FALSE;
        }
        if (button == 3) {
            return TRUE;
        }
    }
    if (gCurFrame != 1)
        return TRUE;

    if (CFPreferencesGetAppBooleanValue(U3PrefAutoSave, kCFPreferencesCurrentApplication, NULL)) {
        GetPascalStringFromArrayByIndex(where, CFSTR("MoreMessages"), 55);    //GetIndString(where, BASERES+14, 56);
        if (Party[3] == 1)
            GetPascalStringFromArrayByIndex(where, CFSTR("MoreMessages"), 56);    //GetIndString(where, BASERES+14, 57);
        if (Party[3] == 2)
            GetPascalStringFromArrayByIndex(where, CFSTR("MoreMessages"), 57);    //GetIndString(where, BASERES+14, 58);
        if (Party[3] == 3)
            GetPascalStringFromArrayByIndex(where, CFSTR("MoreMessages"), 58);    //GetIndString(where, BASERES+14, 59);
        if (Party[3] == 0xFF)
            GetPascalStringFromArrayByIndex(where, CFSTR("MoreMessages"), 59);    //GetIndString(where, BASERES+14, 60);
        if (Party[3] == 0x80 && g835E == 0)
            GetPascalStringFromArrayByIndex(where, CFSTR("MoreMessages"), 60);    //GetIndString(where, BASERES+14, 61);
    } else
        GetPascalStringFromArrayByIndex(where, CFSTR("MoreMessages"), 95);    //GetIndString(where, BASERES+14, 96);

    ParamText(where, nil, nil, nil);
    mouseStateStore = gMouseState;
    gMouseState = 0;
    CursorUpdate();
    button = Alert(BASERES + 5, NIL_PTR);
    gMouseState = mouseStateStore;
    if (button == 1)
        return FALSE;
    return TRUE;
}

void HandleMouseDown(void) {
    WindowPtr whichWindow;
    Boolean formed;
    short i, numChars, thePart, num, left, right, cleric;
    long menuChoice;
    Point mouse;
    Rect charRect;

    gInBackground = FALSE;
    mouse = gTheEvent.where;
    if (CFPreferencesGetAppBooleanValue(U3PrefFullScreen, kCFPreferencesCurrentApplication, NULL) && mouse.v < gBarHeight)
        MyShowMenuBar();
    GlobalToLocal(&mouse);
    thePart = FindWindow(gTheEvent.where, &whichWindow);
    switch (thePart) {
        case inMenuBar:
            menuChoice = MenuSelect(gTheEvent.where);
            HandleMenuChoice(menuChoice);
            //          if (prefs.fullScreen) MyHideMenuBar();
            gKeyPress = 0;
            break;
        case inSysWindow:
#if !TARGET_CARBON
            SystemClick(&gTheEvent, whichWindow);
            gKeyPress = 0;
#endif
            break;
        case inDrag:
            SetUpDragRect();
            DragWindow(whichWindow, gTheEvent.where, &gDragRect);
            ForceAllOnScreen();
            gKeyPress = 0;
            break;
        case inGoAway:
            if (TrackGoAway(whichWindow, gTheEvent.where)) {
                if (QuitDialog())
                    gDone = TRUE;
            }
            gKeyPress = 0;
            break;
        default:
            if (whichWindow == gShroudWindow) {
                break;
            }
            right = blkSiz * 39 - 1;
            if (gUpdateWhere == 3 || gUpdateWhere == 4 || gUpdateWhere == 8 || gUpdateWhere == 9 || gMouseState == 4) {
                num = 0;
                SetRect(&charRect, blkSiz * 24 + 1, blkSiz + 1, right, blkSiz * 4 - 1);
                if (PtInRect(mouse, &charRect) && WidgetClick(charRect, TRUE, FALSE))
                    num = 1;
                SetRect(&charRect, blkSiz * 24 + 1, blkSiz * 5 + 1, right, blkSiz * 8 - 1);
                if (PtInRect(mouse, &charRect) && WidgetClick(charRect, TRUE, FALSE))
                    num = 2;
                SetRect(&charRect, blkSiz * 24 + 1, blkSiz * 9 + 1, right, blkSiz * 12 - 1);
                if (PtInRect(mouse, &charRect) && WidgetClick(charRect, TRUE, FALSE))
                    num = 3;
                SetRect(&charRect, blkSiz * 24 + 1, blkSiz * 13 + 1, right, blkSiz * 16 - 1);
                if (PtInRect(mouse, &charRect) && WidgetClick(charRect, TRUE, FALSE))
                    num = 4;
                if (num) {
                    if (gMouseState == 4) {
                        AddMacro('0' + num);
                    } else {
                        DoStats(num);
                    }
                }
            }
            if (gUpdateWhere == 5) {   // main menu
                num = 0;
                if (HandleButtonClick(mouse, 0))
                    num = 'R';
                if (HandleButtonClick(mouse, 1))
                    num = 'O';
                if (HandleButtonClick(mouse, 2))
                    num = 'J';
                if (HandleButtonClick(mouse, 8))
                    num = 'A';
                if (num)
                    AddMacro(num);
            }
            if (gUpdateWhere == 6) {   // Organize Party
                numChars = 0;
                formed = FALSE;
                for (i = 1; i < 21; i++) {
                    if (Player[i][0])
                        numChars++;
                    if (Player[i][16])
                        formed = TRUE;
                }
                num = 0;
                if (numChars < 20) {
                    if (HandleButtonClick(mouse, 3))
                        num = 'C';
                }
                if (numChars > 0) {
                    if (HandleButtonClick(mouse, 4))
                        num = 'T';
                }
                if (numChars > 0) {
                    if (HandleButtonClick(mouse, 5 + formed)) {
                        num = 'F';
                        if (formed)
                            num = 'D';
                    }
                }
                if (HandleButtonClick(mouse, 7))
                    num = 27;    // esc
                if (num != 0)
                    AddMacro(num);
            }
            if (gUpdateWhere == 9)
                HandleStatsClick(mouse);
            if (gCurCursor > -1) {
                switch (gCurCursor) {
                    case 0: AddMacro(' '); break;
                    case 1: AddMacro(30); break;
                    case 2: AddMacro(31); break;
                    case 3: AddMacro(28); break;
                    case 4: AddMacro(29); break;
                    case 5:
                        AddMacro(gCurMouseDir);
                        AddMacro('A');
                        break;
                    case 6: AddMacro('T'); break;
                    case 7:    // open chest
                        cleric = 0;
                        num = 0;
                        while (!cleric && num < 4) {
                            left = Party[num + 7];
                            if ((Player[left][17] == 'G') || (Player[left][17] == 'P')) {
                                if (Player[left][25] > 4) {
                                    if (Player[left][23] == careerTable[8] || Player[left][23] == careerTable[10]) {
                                        AddMacro('B');
                                        AddMacro('C');
                                        AddMacro('1' + num);
                                        AddMacro('C');
                                        cleric = 1;
                                    }
                                    if (Player[left][23] == careerTable[1] || Player[left][23] == careerTable[4] ||
                                        Player[left][23] == careerTable[7]) {
                                        AddMacro('B');
                                        AddMacro('1' + num);
                                        AddMacro('C');
                                        cleric = 1;
                                    }
                                }
                            }
                            num++;
                        }
                        if (!cleric)
                            AddMacro('G');
                        break;
                    case 8: AddMacro('E'); break;
                    case 9: AddMacro('B'); break;
                    case 10:
                        AddMacro(gCurMouseDir);
                        AddMacro('F');
                        break;
                    case 11:
                        AddMacro(gCurMouseDir);
                        AddMacro('U');
                        break;
                    case 12: AddMacro('X'); break;
                    case 13: break;
                    case 14: AddMacro('K'); break;
                    case 15: AddMacro('D'); break;
                    case 16: AddMacro(28); break;
                    case 17: AddMacro(29); break;
                    case 18: AddMacro(30); break;
                    case 19: AddMacro(31); break;
                    case 20:    // Ignite (cleric means who has a torch)
                        cleric = -1;
                        num = 0;
                        while (cleric < 0 && num < 4) {
                            left = Party[num + 7];
                            if ((Player[left][17] == 'G') || (Player[left][17] == 'P')) {
                                if (Player[left][15] > 0)
                                    cleric = num;
                            }
                            num++;
                        }
                        if (cleric > -1) {
                            AddMacro('1' + cleric);
                            AddMacro('I');
                        } else {   // no one had a torch, just press I and let 'em figure it out.
                            AddMacro('I');
                        }
                        break;
                    case 22: AddMacro('7'); break;
                    case 23: AddMacro('9'); break;
                    case 24: AddMacro('1'); break;
                    case 25: AddMacro('3'); break;
                }
            }
            break;
    }
}

void HandleMenuChoice(long menuChoice) {
    int theMenu;
    int theItem;

    if (menuChoice != 0) {
        theMenu = HiWord(menuChoice);
        theItem = LoWord(menuChoice);
        switch (theMenu) {
            case APPLEMENU: HandleAppleChoice(theItem); break;
            case FILEMENU: HandleFileChoice(theItem); break;
            case SPECIALMENU: HandleSpecialChoice(theItem); break;
            case REFERENCEMENU: HandleReferenceChoice(theItem); break;
        }
        HiliteMenu(0);
    }
}

void HandleAppleChoice(int theItem) {
    short mouseStateStore;

    switch (theItem) {
        case ABOUTID: AboutUltima3(); break;
        default: break;
    }
}

void AboutUltima3(void) {
    // Use a Cocoa window for the About box.
    ShowAboutWindow();
    /*
    OSErr           error;
    short           bytesWide, i, numblurs, mouseStateStore, xOff, yOff;
    short           x, y;
    PicHandle       pict;
    Rect            fromRect, toRect;
    CGrafPtr        aboutOffWorld1, aboutOffWorld2, aboutBackStoreWorld;
    PixMapHandle    aboutOffPixMap1, aboutOffPixMap2;
    long            endTime, offset;
    Boolean         done=FALSE;
    Ptr             base2;
    
    mouseStateStore = gMouseState;
    gMouseState = 0;
    CursorUpdate();
    pict = GetPicture(10);
    if (pict==0) { HandleError(ResError(), 73, 10); return; }
    fromRect = (*pict)->picFrame;
    toRect = fromRect;
    xOff = ((blkSiz*40)/2)-(fromRect.right/2);
    yOff = ((blkSiz*24)/2)-(fromRect.bottom/2);
    OffsetRect(&toRect,xOff,yOff);
    gInterrupt=FALSE;
    error = NewGWorld(&aboutOffWorld1, 32, &fromRect, nil, nil, 0);
    if (!error)
        {
        SetGWorld(aboutOffWorld1, nil);
        ForeColor(blackColor); BackColor(whiteColor);
        DrawPicture(pict, &fromRect);
        ReleaseResource((Handle)pict); pict=0;
        error = NewGWorld(&aboutOffWorld2, 32, &fromRect, nil, nil, 0);
        if (error) { SetGWorld(mainPort, nil); DisposeGWorld(aboutOffWorld1); }
        }
    if (!error)
        {
        error = NewGWorld(&aboutBackStoreWorld, 0, &fromRect, nil, nil, 0);
        if (error) { SetGWorld(mainPort, nil);
                     DisposeGWorld(aboutOffWorld1); DisposeGWorld(aboutOffWorld2); }
        }
    if (!error)
        {
        CopyBits(LWPortCopyBits(mainPort),
                 LWPortCopyBits(aboutBackStoreWorld), &toRect, &fromRect, srcCopy, nil);
        aboutOffPixMap1 = GetGWorldPixMap(aboutOffWorld1);
        LockPixels(aboutOffPixMap1);
        aboutOffPixMap2 = GetGWorldPixMap(aboutOffWorld2);
        LockPixels(aboutOffPixMap2);
        base2 = GetPixBaseAddr(aboutOffPixMap2);
        bytesWide = (0x7FFF & (**aboutOffPixMap2).rowBytes);
        SetGWorld(mainPort, nil);
        ForeColor(blackColor); BackColor(whiteColor);
        endTime = TickCount()+360;
        while (!gInterrupt)
            {
            if (!done)
                {
                CopyBits(LWPortCopyBits(aboutOffWorld1),
                         LWPortCopyBits(aboutOffWorld2), &fromRect, &fromRect, srcCopy, nil);
                numblurs = (endTime-TickCount())/12;
                if (TickCount()>endTime) numblurs=0;
                for (i=0; i<numblurs; i++)
                    {
                    offset = 0;
                    for (y=0; y<fromRect.bottom; y++)
                        {
                        for (x=8; x<bytesWide; x++)
                            {
                            base2[x+offset] = ((unsigned char)base2[x+offset]*64 +
                                          (unsigned char)base2[x+offset-4]*64 +
                                          (unsigned char)base2[x+offset+4]*64 +
                                          (unsigned char)base2[x+offset-8]*32 +
                                          (unsigned char)base2[x+offset+8]*32)>>8; }
                        offset += bytesWide;
                        }
                    }
                MoveTo(toRect.left-1, toRect.top-1); LineTo(toRect.right, toRect.top-1);
                LineTo(toRect.right, toRect.bottom); LineTo(toRect.left-1, toRect.bottom);
                LineTo(toRect.left-1, toRect.top-1);
                ForeColor(blackColor); BackColor(whiteColor);
                CopyBits(LWPortCopyBits(aboutOffWorld2),
                         LWPortCopyBits(mainPort), &fromRect, &toRect, ditherCopy, nil);
                done=(numblurs==0);
                }
            CheckInterrupted();
            }
        UnlockPixels(aboutOffPixMap1);
        DisposeGWorld(aboutOffWorld1);
        UnlockPixels(aboutOffPixMap2);
        DisposeGWorld(aboutOffWorld2);
        SetPortWindowPort(gMainWindow);
        CopyBits(LWPortCopyBits(aboutBackStoreWorld),
                 LWPortCopyBits(mainPort), &fromRect, &toRect, srcCopy, nil);
        DisposeGWorld(aboutBackStoreWorld);
        }
    else // something went wrong
        {
        SetGWorld(mainPort, nil);
        ForeColor(blackColor); BackColor(whiteColor);
        if (!pict) pict = GetPicture(10);
        DrawPicture(pict, &toRect);
        ReleaseResource((Handle)pict);
        while (!gInterrupt) { CheckInterrupted(); }
        }
    PaintRect(&toRect);
    LWGetWindowBounds(gMainWindow, &toRect);
    LWInvalWindowRect(gMainWindow, &toRect);
    gMouseState = mouseStateStore;
    CursorUpdate();
    */
}

void HandleFileChoice(int theItem) {
    short mouseStateStore;

    switch (theItem) {
        case SAVEID:
            if (Party[3] == 0 && gUpdateWhere == 3) {
                QuitSave(0); /* mode 1 is silent, 0 is verbose */
                UPrintWin("\p ");
                DrawPrompt();
            } else {
                mouseStateStore = gMouseState;
                gMouseState = 0;
                CursorUpdate();
                if (Party[3] != 0)
                    DoStandardAlert(kAlertStopAlert, 2);
                else if (gUpdateWhere != 3)
                    DoStandardAlert(kAlertStopAlert, 1);
                //Alert(BASERES+3, NIL_PTR);
                gMouseState = mouseStateStore;
            }
            break;
        case PAUSEID: DoPause(); break;
        case ABORTID:
            if (QuitDialog()) {
                gDone = TRUE;
                gAbort = TRUE;
            }
            break;
        case QUITID:
            if (QuitDialog()) {
                gDone = TRUE;
            }
            break;
    }
}

void HandleSpecialChoice(int theItem) {
    CFStringRef key = nil;
    Boolean newValue = false;
    switch (theItem) {
        case SOUNDID: key = U3PrefSoundInactive; break;
        case MUSICID: key = U3PrefMusicInactive; break;
        case SPEECHID: key = U3PrefSpeechInactive; break;
        case CONSTRAINID: key = U3PrefSpeedUnconstrain; break;
        case AUTOCOMBATID: key = U3PrefManualCombat; break;
        case DOUBLESIZEID: key = U3PrefOriginalSize; break;
        case FULLSCREENID: key = U3PrefFullScreen; break;
    }
    if (key) {
        newValue = !CFPreferencesGetAppBooleanValue(key, kCFPreferencesCurrentApplication, NULL);
        CFPreferencesSetAppValue(key, (newValue) ? kCFBooleanTrue : kCFBooleanFalse, kCFPreferencesCurrentApplication);
    }

    switch (theItem) {
        case AUTOCOMBATID:
            gAutoCombat = !(CFPreferencesGetAppBooleanValue(U3PrefManualCombat, kCFPreferencesCurrentApplication, NULL));
            break;
        case DOUBLESIZEID:
            AdaptToWindow(true);
            ResetCursor();
            break;
        case FULLSCREENID:
            SetGWorld(mainPort, mainDevice);
            if (newValue) {
                SetUpDisplay();
                ShowHideBackground();
            } else {   // going windowed
                ShowHideBackground();
                RestoreDisplay();
            }
            ResetCursor();
            SetMusicPortAndDevice(mainPort, mainDevice);
            break;
            /*case CUSTOMID:
            GameOptionsDialog();
            BlockExodus();
            break;*/
    }
    ApplyVolumePreferences();
    ReflectPrefs();
}

void HandleReferenceChoice(int theItem) {
    short mouseStateStore;

    if (gUpdateWhere > 19)
        return;
    if (theItem < 5) {
        ImageDisplay(theItem, TRUE);
        HiliteMenu(0);
        mouseStateStore = gMouseState;
        gMouseState = 0;
        CursorUpdate();
        WaitKeyMouse();
        ImageGoAway();
        gMouseState = mouseStateStore;
    } else {
        if (theItem == 6)
            LWOpenURL(CFSTR("http://lairware.com/ultima3/manual.html"));
        else if (theItem == 7)
            LWOpenURL(CFSTR("http://lairware.com/ultima3/cleric.html"));
        else if (theItem == 8)
            LWOpenURL(CFSTR("http://lairware.com/ultima3/wizard.html"));
    }
}

void ShowHideReference(void) {
    if (gCurFrame != gRefChange) {
        gRefChange = gCurFrame;
        if (gCurFrame == 1) {
            LWEnableMenuItem(gRefMenu, 0);
            LWEnableMenuItem(gRefMenu, 1);
            LWEnableMenuItem(gRefMenu, 2);
            LWEnableMenuItem(gRefMenu, 3);
            LWEnableMenuItem(gRefMenu, 4);
            LWEnableMenuItem(gFileMenu, PAUSEID);
        } else {
            LWDisableMenuItem(gRefMenu, 1);
            LWDisableMenuItem(gRefMenu, 2);
            LWDisableMenuItem(gRefMenu, 3);
            LWDisableMenuItem(gRefMenu, 4);
            LWDisableMenuItem(gFileMenu, PAUSEID);
        }
        DrawMenuBar();
    }
}

Boolean ShouldSuppressMenuBarHiding(void) {
    Rect mainRect;
    LWGetScreenRect(&mainRect);
    int margin = 16;
    mainRect.left += margin;
    mainRect.top += gBarHeight;
    mainRect.right -= margin;
    mainRect.bottom -= margin;
    Point mouse;
    GetMouse(&mouse);
    LocalToGlobal(&mouse);
    Boolean outside = !PtInRect(mouse, &mainRect);
    return outside;
}

void ShowMenuBarIfNecessary(void) {
    if (!IsMenuBarVisible() && ShouldSuppressMenuBarHiding())
        MyShowMenuBar();
}

void MenuBarInit(void) {
    Handle myMenuBar;

    myMenuBar = GetNewMBar(128);
    SetMenuBar(myMenuBar);
    gAppleMenu = GetMenuHandle(APPLEMENU);
    gFileMenu = GetMenuHandle(FILEMENU);
    gSpecialMenu = GetMenuHandle(SPECIALMENU);
    gRefMenu = GetMenuHandle(REFERENCEMENU);
    SetRefMenuIcons(gRefMenu);
    if (gAppleMenu)
        AppendResMenu(gAppleMenu, 'DRVR');
    DeleteMenuItem(gFileMenu, QUITID);
    DrawMenuBar();
    orgMenuColors = GetMCInfo();
    if (orgMenuColors == 0)
        HandleError(ResError(), 21, 0);
}

void ToolBoxInit(void) {
    CursHandle watchCurs;

#if !TARGET_CARBON
    InitGraf(&qd.thePort);
    GetDateTime((unsigned long *)&qd.randSeed);
    InitFonts();
    FlushEvents(everyEvent, 0);
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(0L);
    MaxApplZone();
#endif
    InitCursor();
    watchCurs = GetCursor(watchCursor);
    SetCursor(*watchCurs);
}

void SetUpDragRect(void) {
    Rect curRect, scrRect;

    LWGetScreenRect(&scrRect);
    LWGetWindowBounds(gMainWindow, &curRect);
    gDragRect.left = -16384;
    gDragRect.right = 16384;
    gDragRect.top = scrRect.top;
    gDragRect.bottom = 16384;
}

void SetCurWindowPrefPosn(short x, short y) {
    CFNumberRef numRef = CFNumberCreate(NULL, kCFNumberShortType, &x);
    CFPreferencesSetAppValue(U3PrefCurWindowX, numRef, kCFPreferencesCurrentApplication);
    CFRelease(numRef);
    numRef = CFNumberCreate(NULL, kCFNumberShortType, &y);
    CFPreferencesSetAppValue(U3PrefCurWindowY, numRef, kCFPreferencesCurrentApplication);
    CFRelease(numRef);
}

void SetSaveWindowPrefPosn(short x, short y) {
    CFNumberRef numRef = CFNumberCreate(NULL, kCFNumberShortType, &x);
    CFPreferencesSetAppValue(U3PrefSaveWindowX, numRef, kCFPreferencesCurrentApplication);
    CFRelease(numRef);
    numRef = CFNumberCreate(NULL, kCFNumberShortType, &y);
    CFPreferencesSetAppValue(U3PrefSaveWindowY, numRef, kCFPreferencesCurrentApplication);
    CFRelease(numRef);
}

void ForceAllOnScreen(void) {
    Rect curRect, scrRect;
    ForceOnScreen(gMainWindow);
    GetGlobalWindowRect(gMainWindow, &curRect);
    SetCurWindowPrefPosn(curRect.left, curRect.top);
    LWGetScreenRect(&scrRect);
    MoveWindow(gMainWindow, curRect.left, curRect.top, TRUE);
}

void PlaceWindow(void) {
    Rect curRect, scrRect;
    short width, height, newx, newy;

    newx = CFPreferencesGetAppIntegerValue(U3PrefCurWindowX, kCFPreferencesCurrentApplication, NULL);
    newy = CFPreferencesGetAppIntegerValue(U3PrefCurWindowY, kCFPreferencesCurrentApplication, NULL);
    if (CFPreferencesGetAppBooleanValue(U3PrefFullScreen, kCFPreferencesCurrentApplication, NULL) || (newx == 0 && newy == 0)) {
        width = blkSiz * 40;
        height = blkSiz * 24;
        LWGetWindowBounds(gMainWindow, &curRect);
        LWGetScreenRect(&scrRect);
        newx = (scrRect.right - scrRect.left - width) / 2;
        newy = (scrRect.bottom - scrRect.top - height) / 2;
        if (scrRect.bottom == 400)
            newy = (scrRect.bottom - scrRect.top - height);
        SetCurWindowPrefPosn(newx, newy);
    }
    MoveWindow(gMainWindow, newx, newy, TRUE);
}

void WindowInit(short which) {
    short width, height;
    Str255 tempStr;
    long tileSizeIn;
    Rect scrRect, winRect;
    RGBColor color;

    Boolean keyExists = false;
    Boolean smallSize = CFPreferencesGetAppBooleanValue(U3PrefOriginalSize, kCFPreferencesCurrentApplication, &keyExists);
    // Establish a default based on the size of their main screen.
    if (!keyExists) {
        CFDictionaryRef mainDict = CGDisplayCurrentMode(kCGDirectMainDisplay);
        CFNumberRef number;
        int screenWidth, screenHeight;
        number = CFDictionaryGetValue(mainDict, kCGDisplayWidth);
        CFNumberGetValue(number, kCFNumberIntType, &screenWidth);
        number = CFDictionaryGetValue(mainDict, kCGDisplayHeight);
        CFNumberGetValue(number, kCFNumberIntType, &screenHeight);
        smallSize = (screenWidth < 1280 || screenHeight < (800 + GetMBarHeight() + 8));
        CFPreferencesSetAppValue(U3PrefOriginalSize, (smallSize) ? kCFBooleanTrue : kCFBooleanFalse,
                                 kCFPreferencesCurrentApplication);
    }

    LWGetScreenRect(&scrRect);
    width = scrRect.right - scrRect.left;
    height = scrRect.bottom - scrRect.top;
    if (width < 640 || height < 400) {
        width = (width / 40);
        height = (height / 24);
        if (height < width)
            width = height;
        blkSiz = width;
    } else {
        GetIndString(tempStr, BASERES + 11, 5);
        StringToNum(tempStr, &tileSizeIn);
        if (tileSizeIn < 1 || tileSizeIn > 48)
            tileSizeIn = 16;
        if (!smallSize)
            tileSizeIn *= 2;
        blkSiz = tileSizeIn;
    }
    gUnusualSize = (blkSiz != 16);
#if TARGET_CARBON
    SetRect(&winRect, 0, 0, blkSiz * 40, blkSiz * 24);
    gMainWindow = NewCWindow(nil, &winRect, "\pUltima III", false, kWindowFullZoomDocumentProc, nil, true, 0);
    if (gMainWindow) {
        color.red = color.green = color.blue = 0;
        SetWindowContentColor(gMainWindow, &color);
    }
#else
    gMainWindow = GetNewCWindow(BASERES + which, nil, (WindowPtr)-1L);
#endif
    if (!gMainWindow)
        HandleError(ResError(), 22, BASERES + which);
    if (gUnusualSize)
        SizeWindow(gMainWindow, blkSiz * 40, blkSiz * 24, FALSE);
    PlaceWindow();
    ForceAllOnScreen();
    // put this boolean YES into info.plist: NSHighResolutionCapable
    Rect windowRect;
    if (noErr == GetWindowBounds(gMainWindow, kWindowContentRgn, &windowRect)) {
        WrapCarbonWindowInCocoa(gMainWindow, windowRect.left, windowRect.top, windowRect.right-windowRect.left, windowRect.bottom-windowRect.top);
        void *mtk = LWCreateMetalView(gMainWindow);
        if (mtk)
            U3InitMetalRenderer((MTKView *)mtk);
    }
    ShowWindow(gMainWindow);
    SetPortWindowPort(gMainWindow);
    UpdateRgn = NewRgn();
    ClearScreen();
    ForeColor(whiteColor);
    BackColor(blackColor);
}

void ShowHideBackground(void) {
    Rect screenRect, winRect;
    RGBColor color;

    if (CFPreferencesGetAppBooleanValue(U3PrefFullScreen, kCFPreferencesCurrentApplication, NULL)) {
        MyHideMenuBar();
#if TARGET_CARBON
        GetGlobalWindowRect(gMainWindow, &winRect);
        DisposeWindow(gMainWindow);
        gMainWindow = NewCWindow(nil, &winRect, "\p ", false, kWindowPlainDialogProc, nil, false, 0);
        color.red = color.green = color.blue = 0;
        SetWindowContentColor(gMainWindow, &color);
        SetPortWindowPort(gMainWindow);
        ForeColor(blackColor);
        BackColor(whiteColor);
        GetGWorld(&mainPort, &mainDevice);
#else
        HideWindow(gMainWindow);
        ((WindowPeek)gMainWindow)->goAwayFlag = FALSE;
        SetWTitle(gMainWindow, "\p        ");
#endif
        PlaceWindow();
        if (gShroudWindow)
            DisposeWindow(gShroudWindow);
        LWGetScreenRect(&screenRect);
        gShroudWindow = NewWindow(nil, &screenRect, "\p ", TRUE, plainDBox, nil, FALSE, 0);
        if (!gShroudWindow)
            HandleError(ResError(), 23, 0);
        BringToFront(gMainWindow);
        ShowWindow(gMainWindow);
        MyHideMenuBar();
        ClearShroud();
    } else {
#if TARGET_CARBON
        GetGlobalWindowRect(gMainWindow, &winRect);
        DisposeWindow(gMainWindow);
        gMainWindow = NewCWindow(nil, &winRect, "\pUltima III", false, kWindowFullZoomDocumentProc, nil, true, 0);
        color.red = color.green = color.blue = 0;
        SetWindowContentColor(gMainWindow, &color);
        SetPortWindowPort(gMainWindow);
        ForeColor(blackColor);
        BackColor(whiteColor);
        GetGWorld(&mainPort, &mainDevice);
#else
        HideWindow(gMainWindow);
        ((WindowPeek)gMainWindow)->goAwayFlag = TRUE;
        SetWTitle(gMainWindow, "\pUltima III");
#endif
        ShowWindow(gMainWindow);
        if (gShroudWindow)
            DisposeWindow(gShroudWindow);
        gShroudWindow = 0;
    }
}

Boolean ClearShroud(void) {
    Rect myRect;

    if (gShroudWindow == 0)
        return FALSE;
    SetPortWindowPort(gShroudWindow);
    ForeColor(blackColor);
    //  BackColor(blackColor);
    LWGetWindowBounds(gShroudWindow, &myRect);
    PaintRect(&myRect);
    SetPortWindowPort(gMainWindow);
    return TRUE;
}

void SetUpGWorlds(void) {
    short useDepth, minSize;
    Rect myRect;
    long error;
    CTabHandle greyColors = GetCTable(BASERES);

    useDepth = 0; /* or gDepth, optionally */

    SetRect(&myRect, 0, 0, blkSiz * 80, blkSiz * 3);
    error = NewGWorld(&portraitPort, useDepth, &myRect, nil, nil, 0);
    if (error != 0)
        HandleError(error, 24, 0);
    portraitPixMap = GetGWorldPixMap(portraitPort);
    error = LockPixels(portraitPixMap);
    if (error == FALSE)
        HandleError(error, 24, 1);

    SetRect(&myRect, blkSiz * 24, blkSiz * 17, blkSiz * 40, blkSiz * 24);
    error = NewGWorld(&updatePort, gDepth, &myRect, nil, nil, 0);
    if (error != 0)
        HandleError(error, 26, 0);
    updatePixMap = GetGWorldPixMap(updatePort);
    error = LockPixels(updatePixMap);
    if (error == FALSE)
        HandleError(error, 26, 1);

    SetGWorld(updatePort, nil);
    ForeColor(blackColor);
    PaintRect(&myRect);
    SetGWorld(mainPort, nil); /*was mainDevice*/

    SetRect(&myRect, 0, 0, blkSiz * 24, blkSiz * 32);
    //SetRect(&myRect, 0, 0, (short)blkSiz*4, blkSiz*136);

    error = NewGWorld(&tilesPort, 0, &myRect, nil, nil, 0);
    if (error != 0)
        HandleError(error, 27, 0);
    tilesPixMap = GetGWorldPixMap(tilesPort);
    error = LockPixels(tilesPixMap);
    if (error == FALSE)
        HandleError(error, 27, 1);

    error = NewGWorld(&tilesMaskPort, 4, &myRect, greyColors, nil, 0);
    if (error != 0)
        HandleError(error, 28, 0);
    tilesMaskPixMap = GetGWorldPixMap(tilesMaskPort);
    error = LockPixels(tilesMaskPixMap);
    if (error == FALSE)
        HandleError(error, 28, 1);

    SetRect(&myRect, 0, 0, blkSiz * 22, blkSiz * 22);
    error = NewGWorld(&gamePort, useDepth, &myRect, nil, nil, 0);
    if (error != 0)
        HandleError(error, 29, 0);
    gamePixMap = GetGWorldPixMap(gamePort);
    error = LockPixels(gamePixMap);
    if (error == FALSE)
        HandleError(error, 29, 1);

    SetRect(&myRect, 0, 0, blkSiz * 16, blkSiz * 3);
    error = NewGWorld(&framePort, useDepth, &myRect, nil, nil, 0);
    if (error != 0)
        HandleError(error, 30, 0);
    framePixMap = GetGWorldPixMap(framePort);
    error = LockPixels(framePixMap);
    if (error == FALSE)
        HandleError(error, 30, 1);

    minSize = (blkSiz * 22) / 64;
    SetRect(&myRect, 0, 0, minSize * 64, minSize);
    error = NewGWorld(&minitilesPort, useDepth, &myRect, nil, nil, 0);
    if (error != 0)
        HandleError(error, 31, 0);
    minitilesPixMap = GetGWorldPixMap(minitilesPort);
    error = LockPixels(minitilesPixMap);
    if (error == FALSE)
        HandleError(error, 31, 1);

    SetRect(&myRect, 0, 0, blkSiz * 224, blkSiz);
    error = NewGWorld(&textPort, useDepth, &myRect, nil, nil, 0);
    if (error != 0)
        HandleError(error, 32, 0);
    textPixMap = GetGWorldPixMap(textPort);
    error = LockPixels(textPixMap);
    if (error == FALSE)
        HandleError(error, 32, 1);

    NewGWorld(&textOddPort, useDepth, &myRect, nil, nil, 0);
}

void TearDownGWorlds(void) {
    DisposeGWorld(portraitPort);
    portraitPixMap = nil;
    DisposeGWorld(updatePort);
    updatePixMap = nil;
    DisposeGWorld(tilesPort);
    tilesPixMap = nil;
    DisposeGWorld(tilesMaskPort);
    tilesMaskPixMap = nil;
    DisposeGWorld(gamePort);
    gamePixMap = nil;
    DisposeGWorld(framePort);
    framePixMap = nil;
    DisposeGWorld(minitilesPort);
    minitilesPixMap = nil;
    DisposeGWorld(textPort);
    textPixMap = nil;
}

void SetUpFont(void) {
    Str255 fontName;
    static short fontNum = -1;

    if (fontNum == -1) {
        GetIndString(fontName, BASERES + 11, 1);
        GetFNum(fontName, &fontNum);
        if (fontNum < 1) {
            GetIndString(fontName, BASERES + 11, 7);    // Skia
            GetFNum(fontName, &fontNum);
            if (fontNum < 1) {
                GetIndString(fontName, BASERES + 11, 8);    // Optima ExtraBlack
                GetFNum(fontName, &fontNum);
            }
        }
    }
    TextFace(0);
    TextFont(fontNum);
    TextSize(blkSiz);
    ForeColor(whiteColor);
    BackColor(blackColor);
    TextMode(srcCopy);
}

void ResetCursor(void) {
    ReflectNewCursor(-2);
}

void CursorUpdate(void) { /* figure what cursor to show where */
    Boolean aimedAt;
    Rect gameRect, topOfRect;
    Point mous;
    short newCursor, mouseTileX, mouseTileY, offVal, tileSiz, tilesWidth;
    short cX, cY, cXBig, cYBig;
    unsigned char value, wpn, keyEquiv[26] = {' ', 30,  31,  28, 29, 'A', 'T', 'G', 'E', 'B', 'F', 'U', 'X',
                                              'Z', 'K', 'D', 28, 29, 30,  31,  'I', 0,   '7', '9', '1', '3'};

    if (gInBackground)
        return;
    GetMouse(&mous);
    tileSiz = blkSiz * 2;
    tilesWidth = blkSiz * 24;

    if (gUpdateWhere == 9) {   // Stats screen
        float scaler = (float)blkSiz / 16.0;
        newCursor = -1;
        // Other stats screens
        SetRect(&topOfRect, tilesWidth, blkSiz, (blkSiz * 39) - 1, (blkSiz * 4) - 1);
        if (PtInRect(mous, &topOfRect))
            newCursor = 13;
        SetRect(&topOfRect, tilesWidth, blkSiz * 5, (blkSiz * 39) - 1, (blkSiz * 8) - 1);
        if (PtInRect(mous, &topOfRect))
            newCursor = 13;
        SetRect(&topOfRect, tilesWidth, blkSiz * 9, (blkSiz * 39) - 1, (blkSiz * 12) - 1);
        if (PtInRect(mous, &topOfRect))
            newCursor = 13;
        SetRect(&topOfRect, tilesWidth, blkSiz * 13, (blkSiz * 39) - 1, (blkSiz * 16) - 1);
        if (PtInRect(mous, &topOfRect))
            newCursor = 13;
        if (Party[3] != 0x80) {   // not in combat
            // Distribute Food & Gather Gold
            SetRect(&topOfRect, 252 * scaler, 321 * scaler, 348 * scaler, 340 * scaler);
            if (PtInRect(mous, &topOfRect))
                newCursor = 26;
            SetRect(&topOfRect, 252 * scaler, 344 * scaler, 348 * scaler, 363 * scaler);
            if (PtInRect(mous, &topOfRect))
                newCursor = 26;
            // "Thieves" tools
            SetRect(&topOfRect, 287 * scaler, 129 * scaler, 337 * scaler, 139 * scaler);
            if (PtInRect(mous, &topOfRect))
                newCursor = 26;
            SetRect(&topOfRect, 287 * scaler, 143 * scaler, 337 * scaler, 153 * scaler);
            if (PtInRect(mous, &topOfRect))
                newCursor = 26;
            SetRect(&topOfRect, 287 * scaler, 157 * scaler, 337 * scaler, 167 * scaler);
            if (PtInRect(mous, &topOfRect))
                newCursor = 26;
            SetRect(&topOfRect, 287 * scaler, 171 * scaler, 337 * scaler, 181 * scaler);
            if (PtInRect(mous, &topOfRect))
                newCursor = 26;
            // Gold
            SetRect(&topOfRect, 131 * scaler, 183 * scaler, 230 * scaler, 195 * scaler);
            if (PtInRect(mous, &topOfRect))
                newCursor = 26;
            // Weapon
            if (gCurNumWeapons > 8) {
                SetRect(&topOfRect, 41 * scaler, 223 * scaler, 122 * scaler, 311 * scaler);
                if (PtInRect(mous, &topOfRect))
                    newCursor = 26;
                SetRect(&topOfRect, 141 * scaler, 223 * scaler, 216 * scaler, (223 + ((gCurNumWeapons - 8) * 11)) * scaler);
                if (PtInRect(mous, &topOfRect))
                    newCursor = 26;
            } else {
                SetRect(&topOfRect, 41 * scaler, 223 * scaler, 122 * scaler, (223 + (gCurNumWeapons * 11)) * scaler);
                if (PtInRect(mous, &topOfRect))
                    newCursor = 26;
            }
            // Armour
            SetRect(&topOfRect, 257 * scaler, 223 * scaler, 335 * scaler, (223 + (gCurNumArmours * 11)) * scaler);
            if (PtInRect(mous, &topOfRect))
                newCursor = 26;
        }
        ReflectNewCursor(newCursor);
        return;
    }
    if (gUpdateWhere != 3 && gUpdateWhere != 4 && gUpdateWhere < 20)
        return;
    LWGetPortBounds(gamePort, &gameRect);
    OffsetRect(&gameRect, blkSiz, blkSiz);
    if (PtInRect(mous, &gameRect)) {
        if (Party[3] == 1) {   // is in dungeon
            if (gTorch == 0 && GetXYDng(xs, ys) != 3) {   // no light but not on strange wind
                newCursor = 20;    // ignite torch
            } else {
                gCurMouseDir = 0;
                value = GetXYDng(xpos, ypos);
                if (mous.h > mous.v) {
                    if ((tilesWidth - mous.h) > mous.v) {
                        newCursor = 18;    // forward
                        if ((value & 0x10) && mous.v < (blkSiz * 8))
                            newCursor = 14;    // klimb
                    } else {
                        newCursor = 17;    // right
                    }
                } else {
                    if ((tilesWidth - mous.h) > mous.v) {
                        newCursor = 16;    // left
                    } else {
                        newCursor = 19;    // backward
                        if ((value < 128) && (value & 0x20) && mous.v > (blkSiz * 16))
                            newCursor = 15;    // descend
                        if ((value == 0x40) && mous.v > (blkSiz * 16))
                            newCursor = 7;    // get chest
                    }
                }
            }
        } else {   // not in dungeon
            mouseTileX = (mous.h - blkSiz) / tileSiz;
            mouseTileY = (mous.v - blkSiz) / tileSiz;
            cX = cY = 5;
            if (Party[3] == 0x80) {   // in combat
                cX = CharX[gChnum];
                cY = CharY[gChnum];
            }

            if (cX == mouseTileX && cY == mouseTileY && Party[3] != 0x80)
                value = GetXYVal(xpos, ypos) / 2;
            else
                value = GetXYTile(mouseTileX, mouseTileY);

            topOfRect.left = cX * tileSiz + blkSiz;
            topOfRect.right = topOfRect.left + tileSiz;
            topOfRect.top = cY * tileSiz + blkSiz;
            topOfRect.bottom = topOfRect.top + tileSiz;
            cXBig = (cX - 5) * tileSiz;
            cYBig = (cY - 5) * tileSiz;
            if (mous.h - cXBig > mous.v - cYBig) {
                if ((tilesWidth - mous.h) + cXBig > mous.v - cYBig) {
                    newCursor = 1;    // North
                } else {
                    newCursor = 4;    // East
                }
            } else {
                if ((tilesWidth - mous.h) + cXBig > mous.v - cYBig) {
                    newCursor = 3;    // West
                } else {
                    newCursor = 2;    // South
                }
            }
            Boolean allowDiagonal = (!CFPreferencesGetAppBooleanValue(U3PrefNoDiagonals, kCFPreferencesCurrentApplication, NULL));
            if (allowDiagonal) {
                offVal = (blkSiz * 6);
                if (((mous.v - cYBig) / blkSiz) > 12) {   // bottom half
                    if ((((mous.h - cXBig) - offVal) * 2 < tilesWidth - (mous.v - cYBig)) &&
                        ((tilesWidth - (mous.v - cYBig)) - offVal) * 2 < mous.h - cXBig)
                        newCursor = 24;    // Southeast
                    if ((((tilesWidth - (mous.h - cXBig)) - offVal) * 2 < tilesWidth - (mous.v - cYBig)) &&
                        ((tilesWidth - (mous.v - cYBig)) - offVal) * 2 < tilesWidth - (mous.h - cXBig))
                        newCursor = 25;    // Southwest
                } else                     // top half
                {
                    if ((((mous.h - cXBig) - offVal) * 2 < mous.v - cYBig) && ((mous.v - cYBig) - offVal) * 2 < mous.h - cXBig)
                        newCursor = 22;    // Northwest
                    if ((((tilesWidth - (mous.h - cXBig)) - offVal) * 2 < mous.v - cYBig) &&
                        ((mous.v - cYBig) - offVal) * 2 < tilesWidth - (mous.h - cXBig))
                        newCursor = 23;    // Northeast
                }
            }
            if (Party[1] != 1 && value >= 0x5C && value < 0x5E) {
                aimedAt = (cY == mouseTileY && (mouseTileX > cX - 2 && mouseTileX < cX + 2));    // only west or east!
                if (aimedAt) {
                    gCurMouseDir = keyEquiv[newCursor];
                    newCursor = 11;    // Key (unlock)
                }
            }
            if ((value >= 0x1A && value < 0x3E) || value == 0x7E || value >= 0xA0) {   // a being; standard range or a variant tile.
                gCurMouseDir = keyEquiv[newCursor];
                if (Party[3] == 0x80 && gMouseState != 3) { /* in combat & not GetDirection */
                    aimedAt = (cX == mouseTileX || cY == mouseTileY);
                    if (allowDiagonal)
                        aimedAt |= (Absolute(cX - mouseTileX) == Absolute(cY - mouseTileY));
                    if (aimedAt) {
                        wpn = Player[Party[gChnum + 7]][48];
                        if (wpn == 1 || wpn == 3 || wpn == 5 || wpn == 9 || wpn == 13) {
                            newCursor = 5;
                        } else {
                            aimedAt = (cX == mouseTileX && (mouseTileY == cY - 1 || mouseTileY == cY + 1));
                            if (aimedAt)
                                newCursor = 5;    // Attack
                            aimedAt = (cY == mouseTileY && (mouseTileX == cX - 1 || mouseTileX == cX + 1));
                            if (aimedAt)
                                newCursor = 5;    // Attack
                            if (allowDiagonal) {
                                if (mouseTileY > cY - 2 && mouseTileY < cY + 2 && mouseTileX > cX - 2 && mouseTileX < cX + 2)
                                    newCursor = 5;    // Attack all 'round!
                            }
                        }
                    }
                }
                if (Party[3] > 1 && Party[3] < 4 && gMouseState != 3) { /* indoors & not GetDir */
                    if (cX == mouseTileX && (mouseTileY == cY - 1 || mouseTileY == cY + 1)) {
                        newCursor = 6;    // Transact
                    }
                    if (cY == mouseTileY && (mouseTileX == cX - 1 || mouseTileX == cX + 1)) {
                        newCursor = 6;    // Transact
                    }
                    if (allowDiagonal) {
                        if (mouseTileY > cY - 2 && mouseTileY < cY + 2 && mouseTileX > cX - 2 && mouseTileX < cX + 2)
                            newCursor = 6;    // Transact all 'round!
                    }
                    if (cX == mouseTileX && mouseTileY == cY - 2 && GetXYTile(cX, cY - 1) > 0x48) {
                        newCursor = 6;    // Transact
                    }
                    if (cX == mouseTileX && mouseTileY == cY + 2 && GetXYTile(cX, cY + 1) > 0x48) {
                        newCursor = 6;    // Transact
                    }
                    if (cY == mouseTileY && mouseTileX == cX - 2 && GetXYTile(cX - 1, cY) > 0x48) {
                        newCursor = 6;    // Transact
                    }
                    if (cY == mouseTileY && mouseTileX == cX + 2 && GetXYTile(cX + 1, cY) > 0x48) {
                        newCursor = 6;    // Transact
                    }
                }
                if (Party[1] == 0x16 && (Party[3] == 0 || Party[3] == 0xFF)) { /* Sosaria/Ambrosia Ship*/
                    aimedAt = ((cX == mouseTileX && (mouseTileY > cY - 4 && mouseTileY < cY + 4)) ||
                               (cY == mouseTileY && (mouseTileX > cX - 4 && mouseTileX < cX + 4)));
                    if (!aimedAt && allowDiagonal)
                        aimedAt = ((Absolute(cX - mouseTileX) == Absolute(cY - mouseTileY)) &&
                                   (mouseTileX > cX - 4 && mouseTileX < cX + 4 && mouseTileY > cY - 4 && mouseTileY < cY + 4));
                    if (aimedAt)
                        newCursor = 10;    // Fire Cannon
                }
                if (Party[1] != 0x16 && (Party[3] == 0 || Party[3] == 0xFF)) { /* Sosaria/Ambrosia non-ship*/
                    aimedAt = (cX == mouseTileX && (mouseTileY > cY - 2 && mouseTileY < cY + 2));
                    aimedAt |= (cY == mouseTileY && (mouseTileX > cX - 2 && mouseTileX < cX + 2));
                    if (allowDiagonal)
                        aimedAt |= (mouseTileY > cY - 2 && mouseTileY < cY + 2 && mouseTileX > cX - 2 && mouseTileX < cX + 2);
                    if (aimedAt)
                        newCursor = 5;
                }
            }
            if (PtInRect(mous, &topOfRect)) {
                newCursor = 0; /* nothing */
                if (Party[1] == 0x14 || Party[1] == 0x16)
                    newCursor = 12; /* X-it */
                if (value > 0x11 && value < 0x14)
                    newCursor = 7; /* Get Chest */
                if ((value > 0x09 && value < 0x10) || value == 0x7C)
                    newCursor = 8; /* Enter */
                if (value == 0x14 || value == 0x16)
                    newCursor = 9; /* Board/Mount */
                if (Party[3] == 0x80)
                    newCursor = 0;
            }
        }
    } else {
        newCursor = -1; /* just arrow */
        if (gUpdateWhere == 3 || gUpdateWhere == 4 || gUpdateWhere == 8 || gUpdateWhere == 9 || gUpdateWhere > 19) {
            if (gMouseState != 4) {
                SetRect(&topOfRect, tilesWidth, blkSiz, (blkSiz * 39) - 1, (blkSiz * 4) - 1);
                if (PtInRect(mous, &topOfRect))
                    newCursor = 13;
                SetRect(&topOfRect, tilesWidth, blkSiz * 5, (blkSiz * 39) - 1, (blkSiz * 8) - 1);
                if (PtInRect(mous, &topOfRect))
                    newCursor = 13;
                SetRect(&topOfRect, tilesWidth, blkSiz * 9, (blkSiz * 39) - 1, (blkSiz * 12) - 1);
                if (PtInRect(mous, &topOfRect))
                    newCursor = 13;
                SetRect(&topOfRect, tilesWidth, blkSiz * 13, (blkSiz * 39) - 1, (blkSiz * 16) - 1);
                if (PtInRect(mous, &topOfRect))
                    newCursor = 13;
            }
        }
    }
    if (gMouseState == 0)
        newCursor = -1;
    if (gMouseState == 666 && PtInRect(mous, &gameRect))
        newCursor = 21;
    ReflectNewCursor(newCursor);
}

void ReflectNewCursor(short newCursor) {
    if (newCursor != gCurCursor) {
        gCurCursor = newCursor;
        CFStringRef cursorName = NULL;
        switch (gCurCursor) {
            case 0: cursorName = CFSTR("CursorPass"); break;
            case 1: cursorName = CFSTR("CursorNorth"); break;
            case 2: cursorName = CFSTR("CursorSouth"); break;
            case 3: cursorName = CFSTR("CursorWest"); break;
            case 4: cursorName = CFSTR("CursorEast"); break;
            case 5: cursorName = CFSTR("CursorAttack"); break;
            case 6: cursorName = CFSTR("CursorTalk"); break;
            case 7: cursorName = CFSTR("CursorChest"); break;
            case 8: cursorName = CFSTR("CursorEnter"); break;
            case 9: cursorName = CFSTR("CursorBoard"); break;
            case 10: cursorName = CFSTR("CursorCannon"); break;
            case 11: cursorName = CFSTR("CursorUnlock"); break;
            case 12: cursorName = CFSTR("CursorExit"); break;
            case 13: cursorName = CFSTR("CursorLook"); break;
            case 14: cursorName = CFSTR("CursorUp"); break;
            case 15: cursorName = CFSTR("CursorDown"); break;
            case 16: cursorName = CFSTR("CursorLeft"); break;
            case 17: cursorName = CFSTR("CursorRight"); break;
            case 18: cursorName = CFSTR("CursorForward"); break;
            case 19: cursorName = CFSTR("CursorBackward"); break;
            case 20: cursorName = CFSTR("CursorTorch"); break;
            case 21: cursorName = CFSTR("CursorDead"); break;
            case 22: cursorName = CFSTR("CursorNorthWest"); break;
            case 23: cursorName = CFSTR("CursorNorthEast"); break;
            case 24: cursorName = CFSTR("CursorSouthWest"); break;
            case 25: cursorName = CFSTR("CursorSouthEast"); break;
            case 26: cursorName = CFSTR("CursorUse"); break;
        }
        if (cursorName == NULL)
            LWSetArrowCursor();
        else
            SetCursorNamed(cursorName, blkSiz / 16);
    }
}

void DoStats(short chnum) {
    short updateStore = gUpdateWhere;
    short mouseStateStore;

    if (Party[6 + chnum] == 0 || Player[Party[6 + chnum]][0] == 0) {
        ErrorTone();
        return;
    }
    mouseStateStore = gMouseState;
    gMouseState = 0;
    CursorUpdate();
    LWDisableMenuItem(gRefMenu, 1);
    LWDisableMenuItem(gRefMenu, 2);
    LWDisableMenuItem(gRefMenu, 3);
    LWDisableMenuItem(gRefMenu, 4);
    LWDisableMenuItem(gFileMenu, PAUSEID);
    DrawMenuBar();
    if (gUpdateWhere != 9) {
        gUpdateWhere = 9;
    }
    chStatsCur = chnum;
    DrawFancyRecord(FALSE);
    gStatsActive = TRUE;
    while (gStatsActive && !gDone) {
        GetKeyMouse(1);
        if (gKeyPress)
            gStatsActive = FALSE;
    }
    LWEnableMenuItem(gRefMenu, 1);
    LWEnableMenuItem(gRefMenu, 2);
    LWEnableMenuItem(gRefMenu, 3);
    LWEnableMenuItem(gRefMenu, 4);
    LWEnableMenuItem(gFileMenu, PAUSEID);
    DrawMenuBar();
    gMouseState = mouseStateStore;
    gUpdateWhere = updateStore;
    ClearTiles();
    ForceGameWindowUpdate();
}

void DrawFancyRecord(Boolean centered) {
    Rect myRect, FromRect, ToRect;
    short value, rosNum, hOff, vOff, cx, cy;
    long tempNum;
    short rectList[28] = {30,  37,  72, 79,  247, 37,  340, 82,  30,  99,  110, 177, 118, 99,
                          264, 177, 30, 195, 218, 293, 245, 195, 340, 293, 272, 99,  340, 177};
    float scaler = (float)blkSiz / 16.0;
    char charRaces[5] = {'H', 'E', 'D', 'B', 'F'};
    RGBColor color;
    Str255 tempStr;
    static short fontNumber = -1;

    if (fontNumber == -1)
        GetFNum("\pHelvetica", &fontNumber);

    hOff = (blkSiz / 2);
    vOff = (blkSiz * 1.5);
    /*
    hOff-=blkSiz; vOff-=blkSiz; // account for offscreen coordinates
    if (gUnusualSize)
        {
        SetRect(&myRect, 0, 0, 352, 352);
        UpdateGWorld(&gamePort, 0, &myRect, nil, nil, (GWorldFlags)nil);
        }*/
    //SetGWorld(gamePort,0);
    rosNum = Party[6 + chStatsCur];
    color.red = color.green = color.blue = 179 * 256;
    RGBForeColor(&color);
    if (gDepth < 4)
        ForeColor(whiteColor);
    SetRect(&myRect, blkSiz, blkSiz, blkSiz * 23, blkSiz * 23);
    PaintRect(&myRect);
    if (1) {   // was gStatsHeader
        SetRect(&myRect, 0, 0, 352, 37);
        myRect.left *= scaler;
        myRect.right *= scaler;
        myRect.top *= scaler;
        myRect.bottom *= scaler;
        OffsetRect(&myRect, blkSiz, blkSiz);
        DrawNamedImage(CFSTR("CharacterRecord.jpg"), mainPort, &myRect);
        //DrawPicture(gStatsHeader, &myRect);
    }
    if (scaler > 1.0)
        PenSize(truncf(scaler), truncf(scaler));
    for (value = 0; value < 7; value++) {
        myRect.left = (rectList[value * 4] * scaler) + hOff;
        myRect.top = (rectList[value * 4 + 1] * scaler) + vOff;
        myRect.right = (rectList[value * 4 + 2] * scaler) + hOff;
        myRect.bottom = (rectList[value * 4 + 3] * scaler) + vOff;
        color.red = color.green = color.blue = 230 * 256;
        RGBForeColor(&color);
        FrameRect(&myRect);
        myRect.left -= scaler;
        myRect.top -= scaler;
        myRect.right -= scaler;
        myRect.bottom -= scaler;
        color.red = color.green = color.blue = 127 * 256;
        RGBForeColor(&color);
        FrameRect(&myRect);
    }
    ForeColor(blackColor);
    myRect.left = (rectList[0] * scaler) + hOff;
    myRect.top = (rectList[1] * scaler) + vOff;
    myRect.right = (rectList[2] * scaler) + hOff - 2;
    myRect.bottom = (rectList[3] * scaler) + vOff - 2;
    PaintRect(&myRect);
    value = DetermineShape(Player[rosNum][23]);
    //FromRect.left=0; FromRect.right=blkSiz*2;
    //FromRect.top=(value*blkSiz)-1; FromRect.bottom=FromRect.top+(blkSiz*2);
    FromRect = GetTileRectForIndex(value / 2);
    ToRect.left = (34 * scaler) + hOff;
    ToRect.top = (41 * scaler) + vOff;
    ToRect.right = ToRect.left + (32 * scaler);
    ToRect.bottom = ToRect.top + (32 * scaler);
    ForeColor(blackColor);
    CopyBits(LWPortCopyBits(tilesPort), LWPortCopyBits(mainPort), &FromRect, &ToRect, srcCopy, nil);
    TextMode(srcOr);
    TextFont(fontNumber);    // was 3
    TextSize(18 * scaler);
    TextFace(1);
    tempStr[0] = 0;
    value = 0;
    while (tempStr[0] == 0) {
        if (Player[rosNum][value] != 0)
            tempStr[value + 1] = Player[rosNum][value];
        else
            tempStr[0] = value;
        value++;
    }
    color.red = color.green = color.blue = 127 * 256;
    RGBForeColor(&color);

    MoveTo((83 * scaler) + hOff, (55 * scaler) + vOff);
    UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
    ForeColor(blackColor);
    MoveTo((82 * scaler) + hOff, (54 * scaler) + vOff);
    UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
    color.red = 60000;
    color.green = 65535;
    color.blue = 65535;
    RGBForeColor(&color);
    MoveTo((80 * scaler) + hOff, (52 * scaler) + vOff);
    UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
    color.red = 0;
    color.green = 49152;
    color.blue = 49152;
    RGBForeColor(&color);
    MoveTo((81 * scaler) + hOff, (53 * scaler) + vOff);
    UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
    ForeColor(blackColor);
    TextSize(10 * scaler);
    TextFace(0);
    Str255 outStr;

    // Strength
    tempNum = Player[rosNum][18];
    NumToString(tempNum, outStr);
    outStr[++outStr[0]] = ' ';
    GetPascalStringFromArrayByIndex(tempStr, CFSTR("Messages"), 173);
    // GetIndString(tempStr, BASERES+12, 174);
    AddString(outStr, tempStr);
    MoveTo((36 * scaler) + hOff, (113 * scaler) + vOff);
    UDrawThemePascalString(outStr, kThemeCurrentPortFont);

    // Dexterity
    tempNum = Player[rosNum][19];
    NumToString(tempNum, outStr);
    outStr[++outStr[0]] = ' ';
    GetPascalStringFromArrayByIndex(tempStr, CFSTR("Messages"), 174);
    // GetIndString(tempStr, BASERES+12, 175);
    AddString(outStr, tempStr);
    MoveTo((36 * scaler) + hOff, (131 * scaler) + vOff);
    UDrawThemePascalString(outStr, kThemeCurrentPortFont);

    // Intelligence
    tempNum = Player[rosNum][20];
    NumToString(tempNum, outStr);
    outStr[++outStr[0]] = ' ';
    GetPascalStringFromArrayByIndex(tempStr, CFSTR("Messages"), 175);
    // GetIndString(tempStr, BASERES+12, 176);
    AddString(outStr, tempStr);
    MoveTo((36 * scaler) + hOff, (149 * scaler) + vOff);
    UDrawThemePascalString(outStr, kThemeCurrentPortFont);

    // Wisdom
    tempNum = Player[rosNum][21];
    NumToString(tempNum, outStr);
    outStr[++outStr[0]] = ' ';
    GetPascalStringFromArrayByIndex(tempStr, CFSTR("Messages"), 176);
    // GetIndString(tempStr, BASERES+12, 177);
    AddString(outStr, tempStr);
    MoveTo((36 * scaler) + hOff, (167 * scaler) + vOff);
    UDrawThemePascalString(outStr, kThemeCurrentPortFont);

    // Magic, HP, Exp, Food, Gold labels
    MoveTo((125 * scaler) + hOff, (112 * scaler) + vOff);
    GetPascalStringFromArrayByIndex(tempStr, CFSTR("MoreMessages"), 69);    //GetIndString(tempStr, BASERES+14, 70);
    UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
    MoveTo((125 * scaler) + hOff, (126 * scaler) + vOff);
    GetPascalStringFromArrayByIndex(tempStr, CFSTR("MoreMessages"), 70);    //GetIndString(tempStr, BASERES+14, 71);
    UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
    MoveTo((125 * scaler) + hOff, (140 * scaler) + vOff);
    GetPascalStringFromArrayByIndex(tempStr, CFSTR("MoreMessages"), 71);    //GetIndString(tempStr, BASERES+14, 72);
    UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
    MoveTo((125 * scaler) + hOff, (154 * scaler) + vOff);
    GetPascalStringFromArrayByIndex(tempStr, CFSTR("MoreMessages"), 72);    //GetIndString(tempStr, BASERES+14, 73);
    UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
    MoveTo((125 * scaler) + hOff, (168 * scaler) + vOff);
    GetPascalStringFromArrayByIndex(tempStr, CFSTR("MoreMessages"), 73);    //GetIndString(tempStr, BASERES+14, 74);
    UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
    // Display current magic value / max value
    tempNum = Player[rosNum][25];
    NumToString(tempNum, outStr);
    outStr[++outStr[0]] = '/';
    tempNum = MaxMana(rosNum);
    if (tempNum != 0) {
        NumToString(tempNum, tempStr);
        AddString(outStr, tempStr);
    } else
        GetPascalStringFromArrayByIndex(outStr, CFSTR("MoreMessages"), 74);    //GetIndString(outStr, BASERES+14, 75);
    MoveTo((196 * scaler) + hOff, (112 * scaler) + vOff);
    UDrawThemePascalString(outStr, kThemeCurrentPortFont);
    // Display current hit points / max value
    tempNum = (Player[rosNum][26] * 256) + Player[rosNum][27];
    NumToString(tempNum, outStr);
    outStr[++outStr[0]] = '/';
    tempNum = (Player[rosNum][28] * 256) + Player[rosNum][29];
    NumToString(tempNum, tempStr);
    AddString(outStr, tempStr);
    MoveTo((196 * scaler) + hOff, (126 * scaler) + vOff);
    UDrawThemePascalString(outStr, kThemeCurrentPortFont);
    // experience
    tempNum = (Player[rosNum][30] * 100) + Player[rosNum][31];
    NumToString(tempNum, outStr);
    MoveTo((196 * scaler) + hOff, (140 * scaler) + vOff);
    UDrawThemePascalString(outStr, kThemeCurrentPortFont);
    // food
    tempNum = (Player[rosNum][32] * 100) + Player[rosNum][33];
    NumToString(tempNum, outStr);
    MoveTo((196 * scaler) + hOff, (154 * scaler) + vOff);
    UDrawThemePascalString(outStr, kThemeCurrentPortFont);
    // gold
    tempNum = (Player[rosNum][35] * 256) + Player[rosNum][36];
    NumToString(tempNum, outStr);
    MoveTo((196 * scaler) + hOff, (168 * scaler) + vOff);
    UDrawThemePascalString(outStr, kThemeCurrentPortFont);
    // draw weapons list
    cx = 36;
    cy = 208;
    gCurNumWeapons = 0;
    for (value = 0; value < 16; value++) {
        tempNum = Player[rosNum][48 + value];    // tempnum = quantity
        if (value == 0)
            tempNum = 2;    // always 2 hands
        if (tempNum) {
            gCurWeapons[gCurNumWeapons++] = 'A' + value;
            MoveTo((cx * scaler) + hOff, (cy * scaler) + vOff);

            NumToString(tempNum, tempStr);
            UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
            if (value == Player[rosNum][48])
                ForeColor(redColor);
            MoveTo((cx + 16) * scaler + hOff, (cy * scaler) + vOff);
            outStr[0] = 4;
            outStr[1] = '(';
            outStr[3] = ')';
            outStr[4] = ' ';
            outStr[2] = 'A' + value;
            UDrawThemePascalString(outStr, kThemeCurrentPortFont);
            MoveTo((cx + 34) * scaler + hOff, (cy * scaler) + vOff);
            GetPascalStringFromArrayByIndex(tempStr, CFSTR("WeaponsArmour"), value);
            if (tempStr[tempStr[0]] != 's' && tempNum > 1)
                tempStr[++tempStr[0]] = 's';
            UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
            ForeColor(blackColor);
            cy += 11;
            if (cy > 293) {
                cy = 208;
                cx += 100;
            }
        }
    }
    // draw armour list
    cx = 252;
    cy = 208;
    gCurNumArmours = 0;
    for (value = 0; value < 8; value++) {
        tempNum = Player[rosNum][40 + value];
        if (value == 0)
            tempNum = 1;    // Always 1 skin
        if (tempNum) {
            gCurArmours[gCurNumArmours++] = 'A' + value;
            MoveTo((cx * scaler) + hOff, (cy * scaler) + vOff);
            NumToString(tempNum, tempStr);
            UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
            if (value == Player[rosNum][40])
                ForeColor(redColor);
            MoveTo((cx + 16) * scaler + hOff, (cy * scaler) + vOff);
            outStr[0] = 4;
            outStr[1] = '(';
            outStr[3] = ')';
            outStr[4] = ' ';
            outStr[2] = 'A' + value;
            UDrawThemePascalString(outStr, kThemeCurrentPortFont);
            MoveTo((cx + 34) * scaler + hOff, (cy * scaler) + vOff);
            GetPascalStringFromArrayByIndex(tempStr, CFSTR("WeaponsArmour"), value + 16);
            if (tempStr[tempStr[0]] != 's' && tempNum > 1)
                tempStr[++tempStr[0]] = 's';
            UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
            ForeColor(blackColor);
            cy += 11;
        }
    }
    // cards
    cx = 36;
    cy = 304;
    if (Player[rosNum][14] & 0x08) {
        MoveTo((cx * scaler) + hOff, (cy * scaler) + vOff);
        cy += 12;
        GetPascalStringFromArrayByIndex(tempStr, CFSTR("MoreMessages"), 75);    //GetIndString(tempStr,BASERES+14,76);
        UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
    }
    if (Player[rosNum][14] & 0x02) {
        MoveTo((cx * scaler) + hOff, (cy * scaler) + vOff);
        cy += 12;
        GetPascalStringFromArrayByIndex(tempStr, CFSTR("MoreMessages"), 76);    //GetIndString(tempStr,BASERES+14,77);
        UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
    }
    if (Player[rosNum][14] & 0x01) {
        MoveTo((cx * scaler) + hOff, (cy * scaler) + vOff);
        cy += 12;
        GetPascalStringFromArrayByIndex(tempStr, CFSTR("MoreMessages"), 77);    //GetIndString(tempStr,BASERES+14,78);
        UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
    }
    if (Player[rosNum][14] & 0x04) {
        MoveTo((cx * scaler) + hOff, (cy * scaler) + vOff);
        GetPascalStringFromArrayByIndex(tempStr, CFSTR("MoreMessages"), 78);    //GetIndString(tempStr,BASERES+14,79);
        UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
    }
    // marks
    cx = 130;
    cy = 304;
    if (Player[rosNum][14] & 0x10) {
        MoveTo((cx * scaler) + hOff, (cy * scaler) + vOff);
        cy += 12;
        GetPascalStringFromArrayByIndex(tempStr, CFSTR("MoreMessages"), 79);    //GetIndString(tempStr,BASERES+14,80);
        UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
    }
    if (Player[rosNum][14] & 0x20) {
        MoveTo((cx * scaler) + hOff, (cy * scaler) + vOff);
        cy += 12;
        GetPascalStringFromArrayByIndex(tempStr, CFSTR("MoreMessages"), 80);    //GetIndString(tempStr,BASERES+14,81);
        UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
    }
    if (Player[rosNum][14] & 0x40) {
        MoveTo((cx * scaler) + hOff, (cy * scaler) + vOff);
        cy += 12;
        GetPascalStringFromArrayByIndex(tempStr, CFSTR("MoreMessages"), 81);    //GetIndString(tempStr,BASERES+14,82);
        UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
    }
    if (Player[rosNum][14] & 0x80) {
        MoveTo((cx * scaler) + hOff, (cy * scaler) + vOff);
        GetPascalStringFromArrayByIndex(tempStr, CFSTR("MoreMessages"), 82);    //GetIndString(tempStr,BASERES+14,83);
        UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
    }
    // Torches
    tempNum = Player[rosNum][15];
    NumToString(tempNum, outStr);
    GetPascalStringFromArrayByIndex(tempStr, CFSTR("MoreMessages"), 83);    //GetIndString(tempStr,BASERES+14,84);
    AddString(outStr, tempStr);
    MoveTo((280 * scaler) + hOff, (112 * scaler) + vOff);
    UDrawThemePascalString(outStr, kThemeCurrentPortFont);
    // Powders
    tempNum = Player[rosNum][37];
    NumToString(tempNum, outStr);
    GetPascalStringFromArrayByIndex(tempStr, CFSTR("MoreMessages"), 84);    //GetIndString(tempStr,BASERES+14,85);
    AddString(outStr, tempStr);
    MoveTo((280 * scaler) + hOff, (126 * scaler) + vOff);
    UDrawThemePascalString(outStr, kThemeCurrentPortFont);
    // Gems
    tempNum = Player[rosNum][38];
    NumToString(tempNum, outStr);
    GetPascalStringFromArrayByIndex(tempStr, CFSTR("MoreMessages"), 85);    //GetIndString(tempStr,BASERES+14,86);
    AddString(outStr, tempStr);
    MoveTo((280 * scaler) + hOff, (140 * scaler) + vOff);
    UDrawThemePascalString(outStr, kThemeCurrentPortFont);
    // Keys
    tempNum = Player[rosNum][39];
    NumToString(tempNum, outStr);
    GetPascalStringFromArrayByIndex(tempStr, CFSTR("MoreMessages"), 86);    //GetIndString(tempStr,BASERES+14,87);
    AddString(outStr, tempStr);
    MoveTo((280 * scaler) + hOff, (154 * scaler) + vOff);
    UDrawThemePascalString(outStr, kThemeCurrentPortFont);

    // Small text
    TextSize(10 * scaler);
    MoveTo((82 * scaler) + hOff, (72 * scaler) + vOff);
    GetPascalStringFromArrayByIndex(outStr, CFSTR("MoreMessages"), 61);    //GetIndString(outStr, BASERES+14, 62);
    // add the user's level
    tempNum = Player[rosNum][30] + 1;
    NumToString(tempNum, tempStr);
    AddString(outStr, tempStr);
    // space
    outStr[++outStr[0]] = ' ';
    tempStr[0] = 0;
    for (value = 0; value < 11; value++) {
        if (Player[rosNum][23] == careerTable[value])
            GetPascalStringFromArrayByIndex(tempStr, CFSTR("Classes"), value);
    }
    // add class name then print
    if (tempStr[0])
        AddString(outStr, tempStr);
    UDrawThemePascalString(outStr, kThemeCurrentPortFont);

    // Draw boxed overall health, race, and gender.
    MoveTo((252 * scaler) + hOff, (50 * scaler) + vOff);
    if (Player[rosNum][17] == 'G')
        value = 0;
    if (Player[rosNum][17] == 'P') {
        ForeColor(greenColor);
        value = 1;
    }
    if (Player[rosNum][17] == 'D') {
        ForeColor(redColor);
        value = 2;
    }
    if (Player[rosNum][17] == 'A') {
        ForeColor(whiteColor);
        value = 3;
    }
    GetPascalStringFromArrayByIndex(tempStr, CFSTR("MoreMessages"), 62 + value);    //GetIndString(tempStr, BASERES+14, 63+value);
    UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
    ForeColor(blackColor);
    MoveTo((252 * scaler) + hOff, (63 * scaler) + vOff);
    for (value = 0; value < 5; value++) {
        if (Player[rosNum][22] == charRaces[value])
            GetPascalStringFromArrayByIndex(tempStr, CFSTR("Races"), value);
    }
    UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
    MoveTo((252 * scaler) + hOff, (76 * scaler) + vOff);
    if (Player[rosNum][24] == 'F')
        value = 0;
    if (Player[rosNum][24] == 'M')
        value = 1;
    if (Player[rosNum][24] == 'O')
        value = 2;
    GetPascalStringFromArrayByIndex(tempStr, CFSTR("MoreMessages"), 66 + value);    //GetIndString(tempStr, BASERES+14, 67+value);
    UDrawThemePascalString(tempStr, kThemeCurrentPortFont);

    // Buttons
    if (Party[3] != 0x80) {   // not in combat
        SetRect(&ToRect, 0, 0, 96 * scaler, 19 * scaler);
        OffsetRect(&ToRect, 244 * scaler + hOff, 298 * scaler + vOff);
        DrawNamedImage(CFSTR("DistributeFood.png"), mainPort, &ToRect);
        SetRect(&ToRect, 0, 0, 96 * scaler, 19 * scaler);
        OffsetRect(&ToRect, 244 * scaler + hOff, 321 * scaler + vOff);
        DrawNamedImage(CFSTR("GatherGold.png"), mainPort, &ToRect);
    }

    // headers of boxed text (were kThemeSmallEmphasizedSystemFont)
    ForeColor(blueColor);
    TextFace(1);
    TextSize(9 * scaler);
    MoveTo((36 * scaler) + hOff, (95 * scaler) + vOff);
    GetPascalStringFromArrayByIndex(tempStr, CFSTR("MoreMessages"), 87);    //GetIndString(tempStr,BASERES+14,88);
    UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
    MoveTo((125 * scaler) + hOff, (95 * scaler) + vOff);
    GetPascalStringFromArrayByIndex(tempStr, CFSTR("MoreMessages"), 88);    //GetIndString(tempStr,BASERES+14,89);
    UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
    MoveTo((36 * scaler) + hOff, (191 * scaler) + vOff);
    GetPascalStringFromArrayByIndex(tempStr, CFSTR("MoreMessages"), 89);    //GetIndString(tempStr,BASERES+14,90);
    UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
    MoveTo((252 * scaler) + hOff, (191 * scaler) + vOff);
    GetPascalStringFromArrayByIndex(tempStr, CFSTR("MoreMessages"), 90);    //GetIndString(tempStr,BASERES+14,91);
    UDrawThemePascalString(tempStr, kThemeCurrentPortFont);
    MoveTo((280 * scaler) + hOff, (95 * scaler) + vOff);
    GetPascalStringFromArrayByIndex(tempStr, CFSTR("MoreMessages"), 91);    //GetIndString(tempStr,BASERES+14,92);
    UDrawThemePascalString(tempStr, kThemeCurrentPortFont);

    LWValidWindowRect(gMainWindow, &ToRect);
    SetPortWindowPort(gMainWindow);
    SetUpFont();
    ForeColor(blackColor);
    BackColor(whiteColor);
}

// 0123456789A
// FCWTPBLIDAR
short MaxMana(char rosNum) {
    short value, tempNum;

    value = Player[rosNum][23];
    tempNum = 0;
    if (value == careerTable[2])
        tempNum = Player[rosNum][20];    // Wizard
    else if (value == careerTable[1])
        tempNum = Player[rosNum][21];                                   // Cleric
    else if ((value == careerTable[6]) || (value == careerTable[9])) {  // Lark Alchemist
        tempNum = Player[rosNum][20] / 2;
    } else if ((value == careerTable[4]) || (value == careerTable[7])) {  // Paladin Illus
        tempNum = Player[rosNum][21] / 2;
    } else if (value == careerTable[8]) {  // Druid (greater of both halves)
        tempNum = Player[rosNum][20] / 2;
        if ((Player[rosNum][21] / 2) > tempNum)
            tempNum = Player[rosNum][21] / 2;
    } else if (value == careerTable[10]) {  // Ranger (lesser of both halves)
        tempNum = Player[rosNum][20] / 2;
        if ((Player[rosNum][21] / 2) < tempNum)
            tempNum = Player[rosNum][21] / 2;
    }
    return tempNum;
}

void DisableSound(void) {
    //prefs.soundActive=0;
    CFPreferencesSetAppValue(U3PrefSoundInactive, kCFBooleanTrue, kCFPreferencesCurrentApplication);
    ReflectPrefs();
    LWDisableMenuItem(gSpecialMenu, SOUNDID);
    gSoundIncapable = TRUE;
}

void DisableMusic(void) {
    //prefs.musicActive=0;
    CFPreferencesSetAppValue(U3PrefMusicInactive, kCFBooleanTrue, kCFPreferencesCurrentApplication);
    ReflectPrefs();
    LWDisableMenuItem(gSpecialMenu, MUSICID);
    gMusicIncapable = TRUE;
}

void DisableDirectGraphics(void) {
}

void CheckSystemRequirements(void) {
    long response;
    //OSErr     error;
    Str255 errorStr;    //, pathStr="\p:Music:Song 1.mov";
    NumVersion versionNum;
    short button;
    //FSSpec        fss;

    Gestalt(gestaltProcessorType, &response);
    if (response == gestalt68000) {
        //FadeWindowsGDev(gMainWindow, 1, eFade_FadeInCommand);
        GetIndString(errorStr, BASERES + 9, 1);
        ParamText(errorStr, nil, nil, nil);
        button = Alert(BASERES + 7, NIL_PTR);
        if (button == 1)
            ExitToShell();
    }
    Gestalt(gestaltQuickdrawVersion, &response);
    if (response < gestalt32BitQD) {
        //FadeWindowsGDev(gMainWindow, 1, eFade_FadeInCommand );
        GetIndString(errorStr, BASERES + 9, 2);
        ParamText(errorStr, nil, nil, nil);
        button = Alert(BASERES + 7, NIL_PTR);
        if (button == 1)
            ExitToShell();
    }
    GetGWorld(&mainPort, &mainDevice);
    gDepth = (*(*mainDevice)->gdPMap)->pixelSize;
    if (gDepth != 8) {
        DisableDirectGraphics();
    } else {
        DisableDirectGraphics();
    }    // don't allow direct drawing anymore at all.
    if (true /*was DIRECTOFF*/ == 0) {
        Gestalt(gestaltAddressingModeAttr, &response);
        if (response < 7) {
            //FadeWindowsGDev(gMainWindow, 1, eFade_FadeInCommand );
            GetIndString(errorStr, BASERES + 9, 4);
            ParamText(errorStr, nil, nil, nil);
            button = Alert(BASERES + 6, NIL_PTR);
            if (button == 1)
                ExitToShell();
            DisableDirectGraphics();
        }
    }
    gSoundIncapable = FALSE;
    versionNum = SndSoundManagerVersion();
    if (versionNum.majorRev < 3) {
        //FadeWindowsGDev(gMainWindow, 1, eFade_FadeInCommand );
        GetIndString(errorStr, BASERES + 9, 3);
        ParamText(errorStr, nil, nil, nil);
        button = Alert(BASERES + 6, NIL_PTR);
        if (button == 1)
            ExitToShell();
        DisableSound();
    }
    gMusicIncapable = FALSE;
    /*
    error = FSMakeFSSpec(nil, nil, pathStr, &fss);
    if (error)
        {
        //FadeWindowsGDev(gMainWindow, 1, eFade_FadeInCommand );
        GetIndString(errorStr, BASERES+9,7);
        ParamText(errorStr,nil,nil,nil);
        button = Alert(BASERES+6, NIL_PTR);
        if (button == 1) ExitToShell();
        DisableMusic();
        }
    */
            ExitToShell();
    }
}

void ImageDisplay(short which, Boolean hidePause) {
    CFStringRef imageName = nil;
    switch (which) {
        case 1: imageName = CFSTR("Commands.pdf"); break;
        case 2: imageName = CFSTR("SpellList.pdf"); break;
        case 3: imageName = CFSTR("MiscTables.pdf"); break;
        case 4: imageName = CFSTR("SosariaMap.jpg"); break;
        case 5: imageName = CFSTR("Fountain.jpg"); break;
        case 6: imageName = CFSTR("Rod.jpg"); break;
        case 7: imageName = CFSTR("Shrine.jpg"); break;
        case 8: imageName = CFSTR("TimeLord.jpg"); break;
    }

    Rect toRect;
    SetRect(&toRect, blkSiz, blkSiz, blkSiz * 23, blkSiz * 23);
    if (!DrawNamedImage(imageName, mainPort, &toRect))
        printf("could not display image #%d\n", which);

    if (gUpdateWhere < 20 && gUpdateWhere != 10) {
        gUpdateStore = gUpdateWhere;
        gUpdateWhere = 19 + which;
    }
    return;
}

void ImageGoAway(void) {
    LWEnableMenuItem(gRefMenu, 1);
    LWEnableMenuItem(gRefMenu, 2);
    LWEnableMenuItem(gRefMenu, 3);
    LWEnableMenuItem(gRefMenu, 4);
    LWEnableMenuItem(gFileMenu, PAUSEID);
    DrawMenuBar();
    gUpdateWhere = gUpdateStore;
    ClearTiles();
    ForceGameWindowUpdate();
    gCurImage = -1;
    return;
}

void DoPause(void) {
    short mouseStateStore;

    gPaused = (!gPaused);
    CheckMenuItem(gFileMenu, PAUSEID, gPaused);
    HiliteMenu(0);
    if (gPaused) {
        mouseStateStore = gMouseState;
        gMouseState = 0;
        CursorUpdate();
        DrawMenuBar();
        DrawPause();
        while (gPaused && gDone == FALSE) {
            GetKeyMouse(1);
        }
        gMouseState = mouseStateStore;
    } else {
        DrawMenuBar();
        ImageGoAway();
    }
}

void ForceGameWindowUpdate(void) {
    Rect myRect;
    SetRect(&myRect, blkSiz, blkSiz, blkSiz * 23, blkSiz * 23);
    LWInvalWindowRect(gMainWindow, &myRect);
}

void DrawPause(void) {
    short offset, myStore;
    Rect myRect;
    RGBColor color;
    Str255 waitString;

    GetPascalStringFromArrayByIndex(waitString, CFSTR("MoreMessages"), 92);    //GetIndString(waitString,BASERES+14,93);
    if (gUpdateWhere != 10) {
        myStore = gUpdateWhere;
        ImageDisplay(4, FALSE);
        gUpdateWhere = 10;
        gUpdateStore = myStore;
    } else {
        ImageDisplay(4, FALSE);
    }
    color.blue = color.green = color.red = 24576;
    OpColor(&color);
    SetRect(&myRect, blkSiz * 6.3, blkSiz * 10.9, blkSiz * 17.7, blkSiz * 13);
    PenMode(blend);
    PaintRect(&myRect);
    PenMode(srcCopy);
    TextFont(3);
    TextSize(blkSiz * 1.5);
    TextMode(srcOr);
    ForeColor(yellowColor);
    if (gDepth < 4)
        ForeColor(whiteColor);
    offset = blkSiz * 12 - (StringWidth(waitString) / 2);
    MoveTo(offset, blkSiz * 12.5);
    DrawString(waitString);
    ForeColor(blackColor);
    DrawGamePortToMain(1);
    SetUpFont();
}

void Hibernate(void) {
    gInBackground = TRUE;
}

void WakeUp(void) {
    gInBackground = FALSE;
}

void InitMacro(void) {
    char byte;
    for (byte = 0; byte < 32; byte++) {
        Macro[byte] = 0;
    }
}

void DecMacro(void) {
    char byte;
    for (byte = 0; byte < 31; byte++) {
        Macro[byte] = Macro[byte + 1];
    }
    Macro[31] = 0;
}

void AddMacro(char whut) {
    char byte;
    for (byte = 30; byte >= 0; byte--) {
        Macro[byte + 1] = Macro[byte];
    }
    Macro[0] = whut;
}

void SetUpTilesMenu(ControlHandle ctrl) {
    MenuItemIndex selectedMenuIndex = 666;
    MenuItemIndex standardMenuIndex = 666;
    CFArrayRef graphicsArrayRef = (CFArrayRef)CopyGraphicsDirectoryItems();
    MenuRef theMenu = GetControlPopupMenuHandle(ctrl);
    CFStringRef defaultTilesRef = CFSTR("Standard");
    CFStringRef userTilesNameRef = CFPreferencesCopyAppValue(U3PrefTileSet, kCFPreferencesCurrentApplication);
    CFStringRef tilesNameRef;
    if (userTilesNameRef) {
        tilesNameRef = userTilesNameRef;
    } else {
        tilesNameRef = defaultTilesRef;
    }
    int i;
    for (i = 0; i < CFArrayGetCount(graphicsArrayRef); i++) {
        CFStringRef anItemRef = CFArrayGetValueAtIndex(graphicsArrayRef, i);
        CFRange rng = CFStringFind(anItemRef, CFSTR("-Tiles."), 0);
        if (rng.location != kCFNotFound) {
            CFStringRef nameRef = CFStringCreateWithSubstring(nil, anItemRef, CFRangeMake(0, rng.location));
            MenuItemIndex thisMenuIndex;
            AppendMenuItemTextWithCFString(theMenu, nameRef, 0, 0, &thisMenuIndex);
            if (CFStringCompare(nameRef, tilesNameRef, 0) == kCFCompareEqualTo)
                selectedMenuIndex = thisMenuIndex;
            else if (standardMenuIndex == 666 && CFStringCompare(nameRef, defaultTilesRef, 0) == kCFCompareEqualTo)
                standardMenuIndex = thisMenuIndex;
            CFRelease(nameRef);
        }
    }
    CFRelease(graphicsArrayRef);
    if (userTilesNameRef) {
        CFRelease(userTilesNameRef);
    }
    SetControlMaximum(ctrl, 32767);
    if (selectedMenuIndex != 666)
        SetControlValue(ctrl, selectedMenuIndex);
    else if (standardMenuIndex != 666)
        SetControlValue(ctrl, standardMenuIndex);
}

short RosterSelect(void) {
    char ros, mark, playerNum;
    GrafPtr curPort;
    Rect myRect;
    Boolean dLogDone, firstFound, nameSet;
    DialogPtr rosDlog;
    short clss, itemHit, temp;
    Handle myHandle;
    Str255 itemStr, tempStr;

    if (CFPreferencesGetAppBooleanValue(U3PrefFullScreen, kCFPreferencesCurrentApplication, NULL))
        MyShowMenuBar();
    rosDlog = GetNewDialog(BASERES + 10, nil, (WindowPtr)-1);
    GetPort(&curPort);
    LWSetDialogPort(rosDlog);
    //TextFont(3); TextSize(9);
    firstFound = TRUE;
    for (ros = 1; ros < 21; ros++) {
        GetDialogItem(rosDlog, ros + 3, &temp, &myHandle, &myRect);
        GetControlTitle((ControlHandle)myHandle, itemStr);
        if (Player[ros][0] > 22) {
            nameSet = FALSE;
            mark = 0;
            while (!nameSet) {
                itemStr[0]++;
                itemStr[itemStr[0]] = Player[ros][mark];
                mark++;
                nameSet = (Player[ros][mark] == 0);
            }
            SetControlTitle((ControlHandle)myHandle, itemStr);
            HiliteControl((ControlHandle)myHandle, 255);
        } else {
            GetPascalStringFromArrayByIndex(tempStr, CFSTR("MoreMessages"), 93);    //GetIndString(tempStr, BASERES+14, 94);
            AddString(itemStr, tempStr);
            SetControlTitle((ControlHandle)myHandle, itemStr);
            if (firstFound) {
                firstFound = FALSE;
                GetDialogItem(rosDlog, ros + 3, &temp, &myHandle, &myRect);
                SetControlValue((ControlHandle)myHandle, 1);
                playerNum = ros;
            }
        }
    }
    ShowWindow(GetDialogWindow(rosDlog));

    // draw nicer character record than the old PICT
    GetDialogItem(rosDlog, 3, &temp, &myHandle, &myRect);
    GrafPtr dialogPort = NULL;
    GetPort(&dialogPort);
    DrawNamedImage(CFSTR("CharacterRecord.jpg"), dialogPort, &myRect);

    GetDialogItem(rosDlog, 24, &temp, &myHandle, &myRect);
    SetControlValue((ControlHandle)myHandle, 1);    // Default to Fighter
    clss = 1;
    GetDialogItem(rosDlog, 26, &temp, &myHandle, &myRect);
    DrawClassHelp(1, myRect.left, myRect.top);    // Default to Fighter
    DefineDefaultItem(rosDlog, 1);
    //GetDialogItem(rosDlog, 1, &temp, &myHandle, &myRect);
    //PenSize(3,3);
    //InsetRect(&myRect, -4, -4);
    //FrameRoundRect(&myRect, 16, 16);
    ConfigureFilter(1, 2);    // OK, Cancel
    dLogDone = FALSE;
    while (dLogDone == FALSE) {
        ModalDialog((ModalFilterUPP)DialogFilterProc, &itemHit);
        switch (itemHit) {
            case 1:    // OK
                Player[playerNum][0] = clss;
                dLogDone = TRUE;
                break;
            case 2:    // Cancel
                dLogDone = TRUE;
                Player[playerNum][0] = 0;
                playerNum = 0;
                break;
            default:
                if (itemHit == 24) {   // pop-up class menu
                    GetDialogItem(rosDlog, 24, &temp, &myHandle, &myRect);
                    clss = GetControlValue((ControlHandle)myHandle);
                    GetDialogItem(rosDlog, 26, &temp, &myHandle, &myRect);
                    DrawClassHelp(clss, myRect.left, myRect.top);
                }
                if (itemHit > 3 && itemHit < 24) {
                    playerNum = itemHit - 3;
                    for (mark = 4; mark < 24; mark++) {
                        GetDialogItem(rosDlog, mark, &temp, &myHandle, &myRect);
                        SetControlValue((ControlHandle)myHandle, (mark == itemHit));
                    }
                }
                break;
        }
    }
    //  myRect = rosDlog->portRect;
    DisposeDialog(rosDlog);
    SetPort(curPort);
    //  InvalRect(&myRect);
    //  if (prefs.fullScreen) MyHideMenuBar();
    return playerNum;
}

#define IDCCD_CREATE 1
#define IDCCD_CANCEL 2
#define IDCCD_NAME 3
#define IDCCD_RANDOMNAME 4
#define IDCCD_STR 5
#define IDCCD_DEX 6
#define IDCCD_INT 7
#define IDCCD_WIS 8
#define IDCCD_REMAIN 9
#define IDCCD_ROSNUM 18
#define IDCCD_SEX 19
#define IDCCD_RACE 20
#define IDCCD_TYPE 21
#define IDCCD_POSRACEHELP 22
#define IDCCD_POSCLASSHELP 23
#define IDCCD_GRAPHICHEADER 24

Boolean CharacterCreateDialog(short rosNum) {
    GrafPtr curPort;
    Rect myRect;
    Boolean dLogDone, retVal;
    DialogPtr charDlog = nil;
    long number;
    short itemHit, nameLen, temp, entry, value, pointsLeft, preset;
    Handle myHandle;
    Str255 itemStr, tempStr;
    ControlHandle ctrl;

    retVal = FALSE;
    if (CFPreferencesGetAppBooleanValue(U3PrefFullScreen, kCFPreferencesCurrentApplication, NULL))
        MyShowMenuBar();

    DrawOrganizeMenu();
    charDlog = GetNewDialog(BASERES + 8, nil, (WindowPtr)-1);
    GetPort(&curPort);
    LWSetDialogPort(charDlog);

    // Sex
    GetDialogItem(charDlog, IDCCD_SEX, &temp, &myHandle, &myRect);
    value = RandNum(1, 2);    // No "Other", it's stupid.  They can select it later.
    SetControlValue((ControlHandle)myHandle, value);

    // Randomly pick a name
    CFStringRef identifier;
    if (value == 1) {
        identifier = CFSTR("Female");
    } else {
        identifier = CFSTR("Male");
    }
    int numNames = CFArrayGetCount((CFArrayRef)StringsArray(identifier));
    GetPascalStringFromArrayByIndex(tempStr, identifier, RandNum(0, numNames - 1));
    GetDialogItemAsControl(charDlog, IDCCD_NAME, &ctrl);
    SetDialogItemText((Handle)ctrl, tempStr);

    GetDialogItem(charDlog, IDCCD_TYPE, &temp, &myHandle, &myRect);    // Use Existing Type
    preset = Player[rosNum][0];
    Player[rosNum][0] = 0;
    SetControlValue((ControlHandle)myHandle, preset);
    GetIndString(tempStr, BASERES + 15, preset);    // tempStr contains all preset settings for given class
    if (!tempStr[0])
        HandleError(ResError(), 76, preset);
    GetDialogItem(charDlog, IDCCD_RACE, &temp, &myHandle, &myRect);    // Suggested Race
    SetControlValue((ControlHandle)myHandle, tempStr[2] - '0' + 1);

    // Preset Strength
    if (tempStr[4] == '0') {
        itemStr[0] = 1;
        itemStr[1] = tempStr[5];
    } else {
        itemStr[0] = 2;
        itemStr[1] = tempStr[4];
        itemStr[2] = tempStr[5];
    }
    GetDialogItemAsControl(charDlog, IDCCD_STR, &ctrl);
    SetDialogItemText((Handle)ctrl, itemStr);
    StringToNum(itemStr, &number);
    Player[rosNum][18] = number;

    // Preset Dexterity
    if (tempStr[7] == '0') {
        itemStr[0] = 1;
        itemStr[1] = tempStr[8];
    } else {
        itemStr[0] = 2;
        itemStr[1] = tempStr[7];
        itemStr[2] = tempStr[8];
    }
    GetDialogItemAsControl(charDlog, IDCCD_DEX, &ctrl);
    SetDialogItemText((Handle)ctrl, itemStr);
    StringToNum(itemStr, &number);
    Player[rosNum][19] = number;

    // Preset Intelligence
    if (tempStr[10] == '0') {
        itemStr[0] = 1;
        itemStr[1] = tempStr[11];
    } else {
        itemStr[0] = 2;
        itemStr[1] = tempStr[10];
        itemStr[2] = tempStr[11];
    }
    GetDialogItemAsControl(charDlog, IDCCD_INT, &ctrl);
    SetDialogItemText((Handle)ctrl, itemStr);
    StringToNum(itemStr, &number);
    Player[rosNum][20] = number;

    // Preset Wisdom
    if (tempStr[13] == '0') {
        itemStr[0] = 1;
        itemStr[1] = tempStr[14];
    } else {
        itemStr[0] = 2;
        itemStr[1] = tempStr[13];
        itemStr[2] = tempStr[14];
    }
    GetDialogItemAsControl(charDlog, IDCCD_WIS, &ctrl);
    SetDialogItemText((Handle)ctrl, itemStr);
    StringToNum(itemStr, &number);
    Player[rosNum][21] = number;

    // Should always come out to 0, but just in case ...
    pointsLeft = 50 - (Player[rosNum][18] + Player[rosNum][19] + Player[rosNum][20] + Player[rosNum][21]);
    NumToString((long)pointsLeft, itemStr);
    GetDialogItemAsControl(charDlog, IDCCD_REMAIN, &ctrl);
    SetDialogItemText((Handle)ctrl, itemStr);
    ShowWindow(GetDialogWindow(charDlog));
    ReleaseResource(myHandle);

    SelectDialogItemText(charDlog, IDCCD_NAME, 0, 32767);    // Select all of name field

    // draw nicer character record than the old PICT
    GetDialogItem(charDlog, IDCCD_GRAPHICHEADER, &temp, &myHandle, &myRect);
    GrafPtr dialogPort = NULL;
    GetPort(&dialogPort);
    DrawNamedImage(CFSTR("CharacterRecord.jpg"), dialogPort, &myRect);

    // Show beginning class help
    GetDialogItem(charDlog, IDCCD_TYPE, &temp, &myHandle, &myRect);
    preset = GetControlValue((ControlHandle)myHandle);
    GetDialogItem(charDlog, IDCCD_POSCLASSHELP, &temp, &myHandle, &myRect);
    DrawClassHelp(preset, myRect.left, myRect.top);

    // Show beginning race help
    GetDialogItem(charDlog, IDCCD_RACE, &temp, &myHandle, &myRect);
    preset = GetControlValue((ControlHandle)myHandle);
    GetDialogItem(charDlog, IDCCD_POSRACEHELP, &temp, &myHandle, &myRect);
    DrawRaceHelp(preset, myRect.left, myRect.top);

    DefineDefaultItem(charDlog, 1);

    ConfigureFilter(IDCCD_CREATE, IDCCD_CANCEL);    // OK, Cancel
    dLogDone = FALSE;
    while (dLogDone == FALSE) {
        GetDialogItem(charDlog, IDCCD_NAME, &temp, &myHandle, &myRect);
        GetDialogItemText(myHandle, tempStr);
        nameLen = tempStr[0];
        if (nameLen < 1) {
            GetDialogItem(charDlog, 1, &temp, &myHandle, &myRect);
            HiliteControl((ControlHandle)myHandle, 255);
        } else {
            GetDialogItem(charDlog, 1, &temp, &myHandle, &myRect);
            HiliteControl((ControlHandle)myHandle, 0);
        }
        ModalDialog((ModalFilterUPP)DialogFilterProc, &itemHit);
        switch (itemHit) {
            case IDCCD_CREATE:
                if (nameLen > 0) {
                    retVal = TRUE;
                    dLogDone = TRUE;
                }
                break;
            case IDCCD_CANCEL:
                retVal = FALSE;
                dLogDone = TRUE;
                break;
            case IDCCD_NAME:
                GetDialogItem(charDlog, IDCCD_NAME, &temp, &myHandle, &myRect);
                GetDialogItemText(myHandle, tempStr);
                if (tempStr[0] > 12) {
                    tempStr[0] = 12;
                    SetDialogItemText(myHandle, tempStr);
                }
                break;
            case IDCCD_RANDOMNAME:                                                // Random Name
                GetDialogItem(charDlog, IDCCD_SEX, &temp, &myHandle, &myRect);    // get sex
                value = GetControlValue((ControlHandle)myHandle);
                CFStringRef identifier;
                if (value == 1) {
                    identifier = CFSTR("Female");
                } else if (value == 2) {
                    identifier = CFSTR("Male");
                } else {
                    identifier = CFSTR("Intersex");
                }
                int numNames = CFArrayGetCount((CFArrayRef)StringsArray(identifier));
                GetPascalStringFromArrayByIndex(tempStr, identifier, RandNum(0, numNames - 1));
                GetDialogItemAsControl(charDlog, IDCCD_NAME, &ctrl);
                SetDialogItemText((Handle)ctrl, tempStr);
                SelectDialogItemText(charDlog, IDCCD_NAME, 0, 32767);    // Select all of name field
                break;
            case IDCCD_TYPE:
                GetDialogItem(charDlog, IDCCD_TYPE, &temp, &myHandle, &myRect);
                preset = GetControlValue((ControlHandle)myHandle);
                GetDialogItem(charDlog, IDCCD_POSCLASSHELP, &temp, &myHandle, &myRect);
                DrawClassHelp(preset, myRect.left, myRect.top);
                break;
            case IDCCD_RACE:
                GetDialogItem(charDlog, IDCCD_RACE, &temp, &myHandle, &myRect);
                preset = GetControlValue((ControlHandle)myHandle);
                GetDialogItem(charDlog, IDCCD_POSRACEHELP, &temp, &myHandle, &myRect);
                DrawRaceHelp(preset, myRect.left, myRect.top);
                break;
            default:
                if (itemHit > IDCCD_REMAIN && itemHit < IDCCD_ROSNUM) {
                    entry = ((itemHit - 10) / 2) + 18;
                    value = Player[rosNum][entry];
                    temp = 1;
                    if (itemHit % 2)
                        temp = -1;
                    if ((value + temp) < 5 || (value + temp) > 25)
                        break;
                    if ((pointsLeft - temp) > -1) {
                        Player[rosNum][entry] += temp;
                        pointsLeft -= temp;
                    }

                    NumToString((long)Player[rosNum][entry], tempStr);
                    GetDialogItemAsControl(charDlog, entry - 13, &ctrl);
                    SetDialogItemText((Handle)ctrl, tempStr);

                    NumToString((long)pointsLeft, itemStr);
                    GetDialogItemAsControl(charDlog, IDCCD_REMAIN, &ctrl);
                    SetDialogItemText((Handle)ctrl, itemStr);
                }
                break;
        }
    }
    if (retVal) {
        GetDialogItemAsControl(charDlog, IDCCD_NAME, &ctrl);
        GetDialogItemText((Handle)ctrl, tempStr);
        for (temp = 1; temp < 13; temp++) {
            Player[rosNum][temp - 1] = 0;
            if (tempStr[0] >= temp)
                Player[rosNum][temp - 1] = tempStr[temp];
        }
        GetDialogItem(charDlog, IDCCD_SEX, &temp, &myHandle, &myRect);
        temp = GetControlValue((ControlHandle)myHandle);
        value = 'O';
        if (temp == 1)
            value = 'F';
        if (temp == 2)
            value = 'M';
        Player[rosNum][24] = value;
        GetDialogItem(charDlog, IDCCD_RACE, &temp, &myHandle, &myRect);
        temp = GetControlValue((ControlHandle)myHandle);
        GetPascalStringFromArrayByIndex(tempStr, CFSTR("Races"), temp - 1);
        Player[rosNum][22] = tempStr[1];
        GetDialogItem(charDlog, IDCCD_TYPE, &temp, &myHandle, &myRect);
        temp = GetControlValue((ControlHandle)myHandle);
        GetPascalStringFromArrayByIndex(tempStr, CFSTR("Classes"), temp - 1);
        Player[rosNum][23] = tempStr[1];
    }
    DisposeDialog(charDlog);
    SetPort(curPort);
    return retVal;
}

// returns:   0=nothing
//          1-4=character dragged to
//          255=click
short WidgetClick(Rect cRect, Boolean click, Boolean drag) {
    Point curPoint, lastPoint;
    Boolean firstTime = TRUE, nowOver, lastOver;
    short i, charOver, offx, offy;
    Rect lastRect, dragRect, sectRect;
    float fx, fy;

    lastRect.left = -999;
    GetMouse(&curPoint);
    offx = curPoint.h - cRect.left;
    offy = curPoint.v - cRect.top;
    lastOver = nowOver = FALSE;
    InsetRect(&cRect, -1, -1);
    while (StillDown() || firstTime) {
        firstTime = FALSE;
        GetMouse(&curPoint);
        if (click) {    // handle mouseover hilighting
            nowOver = PtInRect(curPoint, &cRect);
            if (lastOver != nowOver) {
                lastOver = nowOver;
                MyInvertRect(&cRect);
            }
        }
        if (drag) {
            if (click && nowOver && lastRect.left != -999) {
                MyInvertFrame(&lastRect);
                lastRect.left = -999;
            }
            if (!click || (click && !nowOver)) {
                curPoint.h -= offx;
                curPoint.v -= offy;
                if (!EqualPt(curPoint, lastPoint)) {
                    // Invert last position
                    if (lastRect.left != -999)
                        MyInvertFrame(&lastRect);
                    // Define new position
                    dragRect.left = curPoint.h;
                    dragRect.top = curPoint.v;
                    dragRect.right = dragRect.left + cRect.right - cRect.left;
                    dragRect.bottom = dragRect.top + cRect.bottom - cRect.top;
                    // Find if over a character
                    charOver = 0;
                    SetRect(&lastRect, blkSiz * 24 + 1, blkSiz + 1, blkSiz * 39, blkSiz * 4 - 1);
                    if (SectRect(&dragRect, &lastRect, &sectRect))
                        charOver = 1;
                    SetRect(&lastRect, blkSiz * 24 + 1, blkSiz * 5 + 1, blkSiz * 39, blkSiz * 8 - 1);
                    if (SectRect(&dragRect, &lastRect, &sectRect))
                        charOver = 2;
                    SetRect(&lastRect, blkSiz * 24 + 1, blkSiz * 9 + 1, blkSiz * 39, blkSiz * 12 - 1);
                    if (SectRect(&dragRect, &lastRect, &sectRect))
                        charOver = 3;
                    SetRect(&lastRect, blkSiz * 24 + 1, blkSiz * 13 + 1, blkSiz * 39, blkSiz * 16 - 1);
                    if (SectRect(&dragRect, &lastRect, &sectRect))
                        charOver = 4;
                    // Invert new position
                    MyInvertFrame(&dragRect);
                    lastPoint = curPoint;
                    lastRect = dragRect;
                }
            }
        }
    }
    if (drag && !nowOver) {
        if (lastRect.left != -999)
            MyInvertFrame(&lastRect);
        if (!charOver) {   // glide back home
            fx = (float)(lastRect.left - cRect.left) / 10.0;
            fy = (float)(lastRect.top - cRect.top) / 10.0;
            for (i = 0; i <= 10; i++) {
                sectRect.left = lastRect.left - (fx * i);
                sectRect.top = lastRect.top - (fy * i);
                sectRect.right = sectRect.left + (lastRect.right - lastRect.left);
                sectRect.bottom = sectRect.top + (lastRect.bottom - lastRect.top);
                MyInvertFrame(&sectRect);
                long endTime = TickCount() + 1;
                while (TickCount() < endTime) {
                }
                MyInvertFrame(&sectRect);
            }
        }
        return charOver;
    }
    if (click && nowOver) {
        MyInvertRect(&cRect);
        return 255;
    }
    return 0;
}

SInt16 DoStandardAlert(AlertType type, short id) {
    Str255 str1, str2;
    SInt16 outItemHit;
    AlertStdAlertParamRec ap;

    GetIndString(str1, BASERES + 18, ((id - 1) * 2) + 1);
    GetIndString(str2, BASERES + 18, ((id - 1) * 2) + 2);
    ap.movable = true;
    ap.helpButton = false;
    ap.filterProc = nil;
    ap.defaultText = (StringPtr)-1;
    ap.cancelText = nil;
    ap.otherText = nil;
    ap.defaultButton = kAlertStdAlertOKButton;
    ap.cancelButton = 0;
    ap.position = kWindowDefaultPosition;
    StandardAlert(type, str1, str2, &ap, &outItemHit);
    return outItemHit;
}

void HandleError(OSErr error, long desc, long idnum) {
    Str255 errorStr, idStr, descStr, descNumStr;
    short button, mouseStateStore = gMouseState;
    Size blah;

    SetGWorld(mainPort, nil);
    MaxMem(&blah);
    NumToString(error, errorStr);
    NumToString(idnum, idStr);
    NumToString(desc, descNumStr);
    if (desc == -108)
        error = -108;
    if (desc > 77 || desc < 1)
        desc = 78;
    GetIndString(descStr, BASERES + 13, desc);
    if (error == -108) {
        gMouseState = 0;
        CursorUpdate();
        NumToString(FreeMem(), errorStr);
        ParamText(errorStr, descStr, idStr, descNumStr);
        Alert(BASERES + 11, NIL_PTR);
        ExitToShell();
    } else {
        gMouseState = 0;
        CursorUpdate();
        ParamText(errorStr, descStr, idStr, descNumStr);
        button = Alert(BASERES + 1, NIL_PTR);
        gMouseState = mouseStateStore;
        if (button == 1)
            ExitToShell();
    }
}

// 'Graphics' and 'Sounds' files are now integral to app.
void OpenGraphicsAndSound(void) {
    // No QuickTime initialization necessary under AVFoundation.

    /*
    OSErr   err;
    Str255  pathStr;
    FSSpec  fss;
    
    GetIndString(pathStr, BASERES+11, 3);
    err = FSMakeFSSpec(nil, nil, pathStr, &fss);
    if (!err) FSpOpenResFile(&fss, fsRdPerm);
    if (err) HandleError(err, 59, 0);

    GetIndString(pathStr, BASERES+11, 4);
    err = FSMakeFSSpec(nil, nil, pathStr, &fss);
    if (!err) FSpOpenResFile(&fss, fsRdPerm);
    if (err) HandleError(err, 60, 0);
*/
}

void MyHideMenuBar(void) {
    RgnHandle screenRgn;
    Rect screenRect, sRect;

    LWGetScreenRect(&sRect);
    GetWindowDeviceRect(gMainWindow, &screenRect);
    if (!gGrayRgn) {
        gBarHeight = GetMBarHeight();
        // store old grayRgn
        gGrayRgn = NewRgn();
        CopyRgn(GetGrayRgn(), gGrayRgn);
        // get Rect of gMainWindow's owner device
        screenRgn = NewRgn();
        RectRgn(screenRgn, &screenRect);
        UnionRgn(gGrayRgn, screenRgn, GetGrayRgn());
        DisposeRgn(screenRgn);
    }
#if TARGET_CARBON
    if (EqualRect(&screenRect, &sRect))
        HideMenuBar();
#else
    if (EqualRect(&screenRect, &sRect))
        LMSetMBarHeight(0);
#endif
}

/*
void MyHideMenuBar(void)
{
    GrafPtr     savePort;
    RgnHandle   screenRgn;
    Rect        scrRect;
 
    screenRgn = NewRgn();
    // store menu bar height if not stored
    if (!gBarHeight) gBarHeight = LMGetMBarHeight();
    if (!gGrayRgn)
        {
        // store grayrgn
        gGrayRgn = NewRgn();
        CopyRgn(GetGrayRgn(), gGrayRgn);
        }
    // merge original grayrgn with main device's rect
    LWGetScreenRect(&scrRect);
    RectRgn(screenRgn, &scrRect);
    UnionRgn(gGrayRgn, screenRgn, GetGrayRgn());
    LMSetMBarHeight(0);
    if (gShroudWindow)
        {
        GetPort(&savePort);
        SetPort(gShroudWindow);
        DiffRgn(GetGrayRgn(), gGrayRgn, screenRgn);
        ForeColor(blackColor);
        PaintRgn(screenRgn);
        SetPort(savePort);
        }
    DisposeRgn(screenRgn);
    MoveWindow(gMainWindow, winxpos, winypos, TRUE);
}
*/

void MyShowMenuBar(void) {
    if (gGrayRgn) {
        SectRgn(gGrayRgn, GetGrayRgn(), GetGrayRgn());
        DisposeRgn(gGrayRgn);
        gGrayRgn = 0;
#if TARGET_CARBON
        ShowMenuBar();
        DrawMenuBar();
#else
        if (gBarHeight)
            LMSetMBarHeight(gBarHeight);
        DrawMenuBar();
        DrawMenuBar();
#endif
    }
}

