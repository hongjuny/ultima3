//  Ultima III Main Section v1.4

#import "UltimaMain.h"

#import "UltimaIncludes.h"
#import "CarbonShunts.h"
#import "CocoaBridge.h"
#import "PrefsDialog.h"
#import "U3Audio.h"
#import "U3IO.h"
#import "U3Renderer.h"
#import "U3Platform.h"
#import "UltimaAppleEvents.h"
#import "UltimaAutocombat.h"
#import "UltimaDngn.h"
#import "UltimaGraphics.h"
#import "UltimaMacIF.h"
#import "UltimaMisc.h"
#import "UltimaNew.h"
#import "UltimaNewMap.h"
#import "UltimaSpellCombat.h"
#import "UltimaText.h"

extern short    gSongCurrent, gSongNext, gSongPlaying;
extern Boolean  gSoundIncapable, gMusicIncapable;

OSErr           gError;
unsigned char   careerTable[12];
Rect            gDragRect, myRect;
WindowPtr       gMainWindow, gShroudWindow;
Boolean         gDone, flag1, gInterrupt, gHasOddTextPort;
Boolean         gPaused, gMenuDone, gInBackground, gResurrect, gAbort, gAutoCombat;
unsigned char   gStoreDirect;
unsigned char   gBallTileBackground;
MenuHandle      gAppleMenu, gFileMenu, gEditMenu, gSpecialMenu, gRefMenu;
long            gTime[2] = {10,9};
short           gTorch, gTimeNegate, gDepth, gUpdateWhere, gMonType, gRefChange, gPauseChange;
short           gMoon[2] = {12,4}, gMoonDisp[2] = {'4','4'}, gUpdateStore;
short           gCurImage, gOrgDepth, gCurCursor, gChnum, gCurMouseDir;
short           gMouseState, gMouseKey, gCurMapID, gCurMapSize;
EventRecord     gTheEvent;
StringHandle    gFontName;
PicHandle       gFrame, gTile;
CGrafPtr        portraitPort, tilesPort, mainPort, demoPort;
CGrafPtr        framePort, tilesMaskPort, gamePort;
CGrafPtr        exodusFromPort, exodusToPort, logoFromPort, logoToPort;
CGrafPtr        gWidePort, updatePort, minitilesPort, textPort, textOddPort;
PixMapHandle    portraitPixMap, tilesPixMap, mainPixMap, demoPixMap;
PixMapHandle    framePixMap, tilesMaskPixMap, gamePixMap;
PixMapHandle    exodusFromPixMap, exodusToPixMap, logoFromPixMap, logoToPixMap;
PixMapHandle    gWidePixMap, updatePixMap, minitilesPixMap, textPixMap;
PixMapHandle    directPixMap;
GDHandle        mainDevice;
RgnHandle       UpdateRgn;
U3DataBuffer    gDemoData;
ResType         gResType;
int             xpos, ypos, xs, ys, dx, dy, ox, oy, tx, ty, wx, wy, demoptr;
long            gMapOffset;
short           /*scrollFlag[3] = {0,2,2},*/ twiddleFlag[4] = {0,3,2,1}, animFlag[4] = {0,16,0,5};
unsigned char   value, TileArray[128], offset, offset2, m5BDC;
unsigned char   gCurFrame, Player[21][65], oldplr[21][65], Party[U3LegacyPartySize];
unsigned char   Monsters[256], Talk[256], Dungeon[2048], Macro[32];
char            gMonVarType;
//const char        MonTypes[14] = {24,23,25,20,26,27,13,28,22,14,15,29,30,24};
//const char        MonBegin[13] = {4,4,4,4,4,4,0,4,4,0,0,4,4};
unsigned char   MoonXTable[8], MoonYTable[8], wpnUseTable[12], armUseTable[12];
const char      WhirlXtable[8] = {0,1,1,1,0,-1,-1,-1};
const char      WhirlYtable[8] = {1,1,0,-1,-1,-1,0,1};
char            gKeyPress;
char            gWhirlCtr, WhirlDX, WhirlDY;
char            WindDir, WindCtr, YellStat, dungeonLevel;
short           gDemoSong, WhirlX, WhirlY, zp[255], lastCard;
Str255          gString;
char            HeadX[4] = {0,1,0,-1}, HeadY[4] = {-1,0,1,0};
short           heading, gExitDungeon, mouseX, mouseY, stx, sty;
unsigned char   CharX[4], CharY[4], CharTile[4], CharShape[4], cHide;
unsigned char   MonsterX[8], MonsterY[8], MonsterTile[8], MonsterHP[8];
unsigned char   LocationX[20], LocationY[20], Experience[17];
Boolean         gShapeSwapped[256], gHorseFacingEast=false;
short           blkSiz;
char            g5521, g56E7;
long            lastSaveNumberOfMoves=0;
ModalFilterUPP  DialogFilterProc;

// ----------------------------------------------------------------------
// Local prototypes

void DoSplashScreen(void);
void FullUpdate(void);
void MainLoop(void);
static bool StoreCreatedCharacter(short slot, const U3CharacterDraft *draft);
void Intro(void);
void Demo(void);
void MainMenu(void);
void Organize(void);
void Examine(void);
void DrawCharacterList(void);
void FormParty(void);
void DisperseParty(void);
void CreateChar(void);
void ShowPoints(char points);
void KillChar(void);
void Game(void);
void StillDownPause(void);
void LetterCommand(char key);
void Pass(void);
void North(void);
void South(void);
void East(void);
void West(void);
void NorthEast(void);
void SouthEast(void);
void NorthWest(void);
void SouthWest(void);
void Attack(void);
void Board(void);
void Descend(void);
void Enter(void);
void Fire(void);
void Klimb(void);
void Look(void);
void PeerGem(void);
void Steal(void);
void Transact(void);
void Unlock(void);
void Exit(void);
void What(void);
void WhirlPool(void);
Boolean ValidTrans(char value);
Boolean ValidDir(unsigned char value);
short StatWait(void);

// ----------------------------------------------------------------------

int Ultima3_main(void) {
    long endTime;
    GrafPtr savePort;
    Rect rect;
#if __profile__
    Str32 profilePath = "\pU3 Profile";
    FSSpec fss;

    ProfilerInit(collectDetailed, bestTimeBase, 1000, 200);
#endif
    gPaused = FALSE;
    gInBackground = FALSE;
    gAbort = FALSE;
    DialogFilterProc = NewModalFilterUPP(DialogFilter);
    ToolBoxInit();
    InitCursor();

#if !DEBUGGING
#warning beta expiration INACTIVE
    if (0) {   // beta expiration code begin
        CFStringRef expiryDateStrRef = (CFStringRef)CopyExpireDateString();
        if (!expiryDateStrRef) {
            DoStandardAlert(kAlertStopAlert, 8);
            ExitToShell();
        } else {
            ConstStringPtr dateStr = CFStringGetPascalStringPtr(expiryDateStrRef, kCFStringEncodingMacRoman);
            Str255 title = "\pUltima III Beta Release";
            Str255 msg = "\pThis prerelease (testing) version of Ultima III will expire on ";
            BlockMove(dateStr + 1, msg + msg[0] + 1, dateStr[0]);
            msg[0] += dateStr[0];
            msg[++msg[0]] = '.';
            AlertStdAlertParamRec ap;
            ap.movable = true;
            ap.helpButton = false;
            ap.filterProc = nil;
            ap.defaultText = (StringPtr)-1;
            ap.cancelText = nil;
            ap.otherText = nil;
            ap.defaultButton = kAlertStdAlertOKButton;
            ap.cancelButton = 0;
            ap.position = kWindowDefaultPosition;
            SInt16 outItemHit;
            StandardAlert(kAlertNoteAlert, title, msg, &ap, &outItemHit);
            CFRelease(expiryDateStrRef);
        }
    }
#endif

    GetGWorld(&mainPort, &mainDevice);
    if (!InitializeAEHandlers())
        ExitToShell();

    EnableMenuCommand(NULL, kHICommandPreferences);
    SetAntiAliasedTextEnabled(true, 6);
    OpenGraphicsAndSound();
    U3AudioApplyPreferences();
    MenuBarInit();
    ValidatePrefs();
    //GetPrefs();
    WindowInit(0);
    if (getenv("U3_STARTUP_RENDER_CHECK")) {
        fprintf(stderr, "Startup render: window created\n");
        SetUpGWorlds();
        fprintf(stderr, "Startup render: worlds allocated\n");
        GetGraphics();
        DrawFrame(3);
        Rect logo = {blkSiz * 2, blkSiz * 2, blkSiz * 10, blkSiz * 38};
        Boolean drawn = DrawNamedImage(CFSTR("Exodus.png"), mainPort, &logo);
        U3CocoaPumpEvents();
        Boolean written = U3CocoaWriteMainBitmap(getenv("U3_STARTUP_RENDER_CHECK"));
        fprintf(stderr, "Startup render: %s\n", drawn && written ? "completed" : "FAILED");
        TearDownGWorlds();
        return drawn && written ? 0 : 1;
    }
    OpenRstr();
    CheckSystemRequirements();
    SetUpDisplayDialog();
    ReflectPrefs();
    if (!U3CocoaHasMainSurface() && U3PlatformGetBooleanPreference(U3PreferenceFullScreen)) {
        ShowHideBackground();
        // spin wheels for 2 seconds for gadgets to disappear
        endTime = U3PlatformTickCount() + 60;
        while (U3PlatformTickCount() < endTime) {
            WaitNextEvent(everyEvent, &gTheEvent, 0L, nil);
            switch (gTheEvent.what) {
                case updateEvt:
                    BeginUpdate((WindowPtr)gTheEvent.message);
                    if (gShroudWindow) {
                        GetPort(&savePort);
                        SetPortWindowPort((WindowPtr)gTheEvent.message);
                        LWGetWindowBounds(gShroudWindow, &rect);
                        ForeColor(blackColor);
                        PaintRect(&rect);
                        SetPort(savePort);
                    }
                    EndUpdate((WindowPtr)gTheEvent.message);
                    break;
                case kHighLevelEvent: AEProcessAppleEvent(&gTheEvent); break;
            }
        }
    }
    U3PlatformFlushAllEvents();

    DoSplashScreen();
    MainLoop();
#if __profile__
    FSMakeFSSpec(0, 0, profilePath, &fss);
    FSpDelete(&fss);
    ProfilerDump(profilePath);
    ProfilerTerm();
#endif
    return 0;
}

unsigned short RandNum(unsigned short lowrnd, unsigned short highrnd) {
    unsigned short qdRdm;
    long rangernd;

    qdRdm = Random();
    rangernd = (highrnd + 1) - lowrnd;
    return ((qdRdm * rangernd) / 65536) + lowrnd;
}

void MainLoop(void) {
    if (getenv("U3_BASIC_PLAY")) {
        U3PlatformSetBooleanPreference(U3PreferenceClassicAppearance, true);
        U3PlatformSetBooleanPreference(U3PreferenceMusicDisabled, true);
    }
    if (getenv("U3_MODERN_TEXT_CHECK"))
        U3PlatformSetBooleanPreference(U3PreferenceClassicAppearance, false);
    demoptr = 0;
    CreateIntroData();
    if (!getenv("U3_COMMAND_TEXT_CHECK") && !getenv("U3_MODERN_TEXT_CHECK") && !getenv("U3_MAIN_MENU_INPUT_CHECK") &&
        !getenv("U3_WORLD_INPUT_CHECK") && !getenv("U3_WORLD_MOUSE_CHECK"))
        Intro();
    U3RenderClearBottom();
    GetDemoRsrc();
    if (getenv("U3_MAIN_MENU_INPUT_CHECK")) {
        gUpdateWhere = 5;
        DrawFrame(3);
        DrawMenu();
        short savedSize = blkSiz;
        const short centers[] = {117, 252, 388, 523};
        const char expected[] = {'R', 'O', 'A', 'J'};
        for (short size = 16; size <= 32; size += 16) {
            blkSiz = size;
            for (int i = 0; i < 4; ++i) {
                U3CocoaQueueDiagnosticMouse(centers[i] * size / 16, 269 * size / 16);
                if (!U3PlatformGetKeyMouse(2) || gKeyPress != expected[i])
                    exit(EXIT_FAILURE);
            }
            U3CocoaQueueDiagnosticMouse(117 * size / 16, 60 * size / 16);
            if (U3PlatformGetKeyMouse(2))
                exit(EXIT_FAILURE);
        }
        blkSiz = savedSize;
        fprintf(stderr, "Main menu input check: all four buttons and outside clicks at 1x/2x passed\n");
        gDone = TRUE;
        return;
    }
    if (getenv("U3_RESUME_CHECK") && getenv("U3_WORLD_INPUT_CHECK")) {
        Demo();
        MainMenu();
        return;
    }
    if (getenv("U3_WORLD_RENDER_CHECK") || getenv("U3_WORLD_INPUT_CHECK") ||
        getenv("U3_WORLD_MOUSE_CHECK") || getenv("U3_PARTY_FLOW_CHECK")) {
        U3CharacterDraft draft = {{'A', 'd', 'a'}, {15, 15, 10, 10}, 'H', 'F', 'F'};
        short slot = 1;
        while (slot <= 20 && Player[slot][0]) ++slot;
        Boolean stored = slot <= 20 && StoreCreatedCharacter(slot, &draft);
        if (stored)
            memset(Party, 0, sizeof(Party));
        Boolean formed = stored && (getenv("U3_PARTY_FLOW_CHECK") ?
            U3StorePartySelection((short[4]){slot, 0, 0, 0}) :
            U3ApplyPartySelection((short[4]){slot, 0, 0, 0}));
        fprintf(stderr, "World render check: slot=%d stored=%d formed=%d io=%d\n",
                slot, stored, formed, (int)U3IOLastError());
        if (!stored || !formed) {
            fprintf(stderr, "World render check: party setup FAILED\n");
            exit(EXIT_FAILURE);
        }
        fprintf(stderr, "World render check: party ready\n");
        if (getenv("U3_MODERN_TEXT_CHECK")) {
            DrawOrganizeMenu();
            if (!U3CocoaWriteMainBitmap(getenv("U3_MODERN_TEXT_CHECK")))
                exit(EXIT_FAILURE);
        }
        DisposeButtons();
        Game();
        return;
    }
    zp[0xCF] = zp[0x10] = gDemoSong = 0;
    gSongCurrent = 0;
    gSongNext = 0;
    //WriteInfoToDisk();
    Demo();
    MainMenu();
    //PutPrefs();
    U3PlatformSynchronizePreferences();
    U3AudioStopMusic();
    U3AudioCloseMusic();
    U3AudioCloseEffects();
    MyShowMenuBar();
    if (!U3CocoaHasMainSurface() && U3PlatformGetBooleanPreference(U3PreferenceFullScreen))
        RestoreDisplay();

    return;
}

void Intro(void) {

    gUpdateWhere = 1;
    DrawFrame(3);
    FadeOnExodusUltima();
    WriteLordBritish();
    FightScene();
    GetButtons();
    if (!gInterrupt) {
        CenterMessage(54, 23);
        DrawFramePiece(12, 12, 23);
        DrawFramePiece(13, 27, 23);
        if (getenv("U3_BOOT_CHECK") || getenv("U3_WORLD_RENDER_CHECK") ||
            getenv("U3_WORLD_INPUT_CHECK") || getenv("U3_WORLD_MOUSE_CHECK")) {
            fprintf(stderr, "Boot check: intro ready\n");
            U3CocoaQueueDiagnosticKey(' ');
        }
        U3PlatformWaitKeyMouse();
    }
}

void Demo(void) {
    DrawFrame(2);
    DisableMenus();
    gUpdateWhere = 2;
    DrawDemoScreen();
    U3PlatformObscureCursor();
    if (getenv("U3_BOOT_CHECK") || getenv("U3_WORLD_RENDER_CHECK") ||
        getenv("U3_WORLD_INPUT_CHECK") || getenv("U3_WORLD_MOUSE_CHECK"))
        U3CocoaQueueDiagnosticKey(' ');
    while (!U3PlatformGetKeyMouse(2)) {
        DemoUpdate(demoptr);
        U3AudioUpdateMusic();
        demoptr++;
        if (demoptr > 511)
            demoptr = 0;
    }
    gSongCurrent = gSongNext;
    EnableMenus();
}

void MainMenu(void) {
    char theChar = 0;

    DrawMenuBar();
    while (!gDone) {
        if (theChar != '\n') {
            U3RenderClearBottom();
            DrawFrame(3);
            gUpdateWhere = 5;
            LWDisableMenuItem(gFileMenu, ABORTID);
            DrawMenu();
        }
        if (getenv("U3_BOOT_CHECK")) {
            U3CocoaPumpEvents();
            Boolean written = U3CocoaWriteMainBitmap(getenv("U3_BOOT_CHECK"));
            fprintf(stderr, "Boot check: main menu %s\n", written ? "completed" : "FAILED");
            if (!written)
                exit(EXIT_FAILURE);
            gDone = true;
            return;
        }
        gSongCurrent = gSongNext = 0;
        tx = 24;
        ty = 15;
        //theChar = U3PlatformCursorKey(false);
        if (getenv("U3_RESUME_CHECK") && getenv("U3_WORLD_INPUT_CHECK"))
            U3CocoaQueueDiagnosticKey('j');
        theChar = U3PlatformWaitKeyMouse();
        if (theChar > 95)
            theChar -= 32;
        switch (theChar) {
            case 'R':
                U3RenderClearBottom();
                Demo();
                break;
            case 'J':
                if (getenv("U3_RESUME_CHECK"))
                    fprintf(stderr, "Resume check: Journey Onward menu command received\n");
                U3RenderClearBottom();
                CenterMessage(1, 15);
                ForceUpdateMain();
                Delay(45, nil);
                if (!Party[7]) {
                    //StopAlert(BASERES+22, nil);
                    CenterMessage(2, 21);
                    //ShowClickMessage();
                    DoStandardAlert(kAlertStopAlert, 7);
                    break;
                }
                DisposeButtons();
                Game();
                gDone = TRUE;
                if (gAbort) {
                    gAbort = FALSE;
                    gDone = FALSE;
                    GetParty();          // just added these
                    GetRoster();         // two lines
                    if (Party[3] > 1) {  // if not on Sosaria,
                        Party[3] = 0;       // back to Sosaria
                        xpos = zp[0xE3];    // where they
                        ypos = zp[0xE4];    // were before!
                    }
                    DisposeDungeonGraphics();
                    CreateIntroData();
                    ClearScreen();
                    DrawExodusPict();
                    GetDemoRsrc();
                }
                break;
            case 'O': Organize(); break;
            case 'A': GameOptionsDialog(); break;
            default: theChar = '\n'; break;
        }
    }
}

void Organize(void) {
    char theChar=0;
    Boolean OrgDone;
organize:
    OrgDone = FALSE;
    while (!(OrgDone || gDone)) {
        if (gDone == TRUE)
            OrgDone = TRUE;
        if (theChar != '\n') {
            U3RenderClearBottom();
            DrawOrganizeMenu();
        }
        tx = 24;
        ty = 13;
        gUpdateWhere = 6;
        //      theChar = U3PlatformCursorKey(false);
        theChar = U3PlatformWaitKeyMouse();
        InitCursor();
        gUpdateWhere = 0;
        if (theChar > 95)
            theChar -= 32;
        switch (theChar) {
            case 'C':    // Create
                gUpdateWhere = 7;
                CreateChar();
                break;
            /*              case 'E': // Examine (now moot)
                    gUpdateWhere=7;
                    Examine();
                    break; */
            case 'F':    // Form
                gUpdateWhere = 7;
                FormParty();
                break;
            case 'D':    // Disperse
                gUpdateWhere = 7;
                DisperseParty();
                break;
            case 'T':    // Terminate
                gUpdateWhere = 7;
                KillChar();
                break;
            case 27:     // esc
            case 'B':    // Back
            case 'M':    // Main Menu
                OrgDone = TRUE;
                break;
            default: theChar = '\n'; break;
        }
    }
}

void DrawOrganizeMenu(void) {
    Boolean got, formed = FALSE;
    static short fnum = -1;
    short offx, offy, x, y, i, c, numChars = 0;
    Str255 str, desc;
    float scaler = (float)blkSiz / 16.0;
    RGBColor color;
    Rect offRect, rect;

    if (Party[7] == 0) {   // none should belong to a party
        for (i = 1; i < 21; i++) {
            Player[i][16] = 0;
        }
    }
    U3RenderPrintPascalStringAt("\p ", 11, 11);
    CenterMessage(33, 11);
    SetRect(&offRect, 0, 0, 38 * blkSiz, 133 * scaler);
    offx = 16;
    offy = 230;
    // background
    color.red = color.blue = 16384;
    color.green = 65535;
    RGBForeColor(&color);
    SetRect(&rect, 4 * scaler, 0, 27 * scaler, 133 * scaler);
    OffsetRect(&rect, (offx * scaler), (offy * scaler));
    PaintRect(&rect);
    OffsetRect(&rect, 300 * scaler, 0);
    PaintRect(&rect);

    color.red = color.blue = 0;
    color.green = 32768;
    RGBForeColor(&color);
    SetRect(&rect, 27 * scaler, 0, 103 * scaler, 133 * scaler);
    OffsetRect(&rect, (offx * scaler), (offy * scaler));
    PaintRect(&rect);
    OffsetRect(&rect, 300 * scaler, 0);
    PaintRect(&rect);

    color.red = color.blue = 0;
    color.green = 24576;
    RGBForeColor(&color);
    SetRect(&rect, 103 * scaler, 0, 146 * scaler, 133 * scaler);
    OffsetRect(&rect, (offx * scaler), (offy * scaler));
    PaintRect(&rect);
    OffsetRect(&rect, 300 * scaler, 0);
    PaintRect(&rect);

    color.red = color.blue = 0;
    color.green = 16384;
    RGBForeColor(&color);
    SetRect(&rect, 146 * scaler, 0, 304 * scaler, 133 * scaler);
    OffsetRect(&rect, (offx * scaler), (offy * scaler));
    PaintRect(&rect);
    OffsetRect(&rect, 300 * scaler, 0);
    PaintRect(&rect);
    // text
    x = offx - 2;
    y = offy + 12;
    if (fnum < 0)
        GetFNum("\pHelvetica", &fnum);
    TextFont(fnum);
    TextMode(srcOr);
    ForeColor(whiteColor);
    BackColor(blackColor);
    for (i = 1; i < 21; i++) {
        TextSize(12 * scaler);
        NumToString(i, str);
        MoveTo((x * scaler) + ((26 * scaler) - UThemePascalStringWidth(str, kThemeCurrentPortFont)), y * scaler);
        if (Player[i][16]) {
            //SetRect(&rect, (x+30)*scaler, (y-10)*scaler, (x+104)*scaler, (y+3)*scaler);
            //color.red=color.green=0; color.blue=49152;
            //RGBForeColor(&color); PaintRect(&rect);
            ForeColor(whiteColor);
        } else {
            ForeColor(blackColor);
        }
        UDrawThemePascalString(str, kThemeCurrentPortFont);
        TextFace(0);
        c = 0;
        while (Player[i][c] > 22) {
            str[c + 1] = Player[i][c];
            str[0] = ++c;
        }
        MoveTo((x + 35) * scaler, y * scaler);
        if (c == 0) { /*UDrawThemePascalString("\p  --", kThemeSystemFont);*/
        } else {
            //ThemeFontID thisFont = (Player[i][16]) ? kThemeEmphasizedSystemFont : kThemeSystemFont;
            if (Player[i][16]) {
                TextFace(bold);
                ForeColor(yellowColor);
            } else {
                TextFace(0);
                ForeColor(whiteColor);
            }
            if (Player[i][16])
                formed = TRUE;
            while (UThemePascalStringWidth(str, kThemeCurrentPortFont) > (70 * scaler)) {
                str[--str[0]] = 0xC9; // MacRoman ellipsis
            }
            UDrawThemePascalString(str, kThemeCurrentPortFont);    // was thisFont
            Str255 lvlStr = "\pLvl ";
            Str255 lvlNumStr;
            NumToString(Player[i][30] + 1, lvlNumStr);
            AddString(lvlStr, lvlNumStr);
            TextFace(0);
            ForeColor(whiteColor);
            MoveTo((x + 109) * scaler, y * scaler);
            UDrawThemePascalString(lvlStr, kThemeCurrentPortFont);
            desc[0] = 0;
            if (Player[i][17] == 'P') {
                GetPascalStringFromArrayByIndex(desc, CFSTR("MoreMessages"), 63);    //GetIndString(desc,BASERES+14,64);
                color.red = color.blue = 16384;
                color.green = 65535;
                RGBForeColor(&color);
            }
            if (Player[i][17] == 'D') {
                GetPascalStringFromArrayByIndex(desc, CFSTR("MoreMessages"), 64);    //GetIndString(desc,BASERES+14,65);
                color.green = color.blue = 16384;
                color.red = 65535;
                RGBForeColor(&color);
            }
            if (Player[i][17] == 'A') {
                GetPascalStringFromArrayByIndex(desc, CFSTR("MoreMessages"), 65);    //GetIndString(desc,BASERES+14,66);
                color.green = color.blue = color.red = 49152;
                RGBForeColor(&color);
            }
            if (desc[0])
                desc[++desc[0]] = ' ';
            c = 0;
            got = FALSE;
            while (c < 5 && !got) {
                GetPascalStringFromArrayByIndex(str, CFSTR("Races"), c++);
                got = (Player[i][22] == str[1]);
            }
            AddString(desc, str);
            desc[++desc[0]] = ' ';
            c = 0;
            got = FALSE;
            while (c < 11 && !got) {
                GetPascalStringFromArrayByIndex(str, CFSTR("Classes"), c++);
                got = (Player[i][23] == str[1]);
            }
            AddString(desc, str);
            desc[++desc[0]] = ' ';
            switch (Player[i][24]) {
                case 'F':
                    GetPascalStringFromArrayByIndex(str, CFSTR("MoreMessages"), 66); /*GetIndString(str,BASERES+14,67);*/
                    break;
                case 'M':
                    GetPascalStringFromArrayByIndex(str, CFSTR("MoreMessages"), 67); /*GetIndString(str,BASERES+14,68);*/
                    break;
                case 'O':
                    GetPascalStringFromArrayByIndex(str, CFSTR("MoreMessages"), 68); /*GetIndString(str,BASERES+14,69);*/
                    break;
            }
            AddString(desc, str);
            while (UThemePascalStringWidth(desc, kThemeCurrentPortFont) > (152 * scaler)) {
                desc[--desc[0]] = 0xC9; // MacRoman ellipsis
            }
            MoveTo((x + 153) * scaler, y * scaler);
            UDrawThemePascalString(desc, kThemeCurrentPortFont);
            ForeColor(whiteColor);
            numChars++;
        }
        y += 13;
        if (y > (140 + offy)) {
            y = offy + 12;
            x += 300;
        }
    }

    DrawButton(3, FALSE, (numChars > 19));    // create
    DrawButton(4, FALSE, (numChars < 1));     // terminate
    DrawButton(5 + formed, FALSE, (numChars < 1));
    DrawButton(7, FALSE, FALSE);
}

void Examine(void) {
    U3RenderClearBottom();
    DrawCharacterList();
    ShowClickMessage();
    U3PlatformWaitKeyMouse();
}

void DrawCharacterList(void) {   // now obsolete
    /*  char    slot, byte;
    CenterMessage(3,10); // 10 was 11
    for (slot = 1; slot<21; slot++) {
        ty = slot+11+((slot>10)*-10); // 11 was 12
        tx = 1+(slot<10)+((slot>10)*20);
        U3RenderPrintNumberAt(slot, tx, ty);
        if (Player[slot][0]!=0)
            {
            if (Player[slot][16]!=0) {
                U3RenderPrintCharAt('*',tx+1,ty);
                }
            else {
                U3RenderPrintCharAt('-',tx+1,ty);
                }
            U3RenderPrintCharAt(Player[slot][24],tx+1,ty);
            U3RenderPrintCharAt(Player[slot][22],tx,ty);
            U3RenderPrintCharAt(Player[slot][23],tx,ty);
            U3RenderPrintCharAt(Player[slot][17],tx,ty);
            tx++;
            for (byte = 0; byte<8; byte++) {
                if (Player[slot][byte]!=0) {
                    U3RenderPrintCharAt(Player[slot][byte],tx,ty);
                    }
                }
            }
        else
            {
            U3RenderPrintPascalStringAt("\p--",tx+8,ty);
            }
        }   */
}

void FormParty(void) {
    if (FormPartyDialog()) {
        U3RenderClearBottom();
        CenterMessage(4, 13);
        CenterMessage(12, 16);
        ShowClickMessage();
        U3PlatformWaitKeyMouse();
    }
    /*  char    input, byte;
    Boolean valid;
    U3RenderClearBottom();
    CenterMessage(4,13);
    if (Party[7]!=0)
        {
        CenterMessage(5,16);
        ShowClickMessage();
        U3PlatformWaitKeyMouse();
        return;
        }
    for (byte = 0; byte<64; byte++)
        {
        Party[byte] = 0;
        }
    Message(6,15,16);
    Message(7,15,17);
    Message(8,15,18);
    Message(9,15,19);
    for (byte = 1; byte<5; byte++)
        {
        valid = TRUE;
        U3RenderPrintPascalStringAt("\p   ",24,15+byte);
        input = UInputNum(24,15+byte);
        if (gDone) byte = 7;
        if (byte>1 && input==0) goto form1;
        if (input>20 || Player[input][0] == 0) {
            CenterMessage(10,21);
            valid = FALSE;
            ShowClickMessage();
            U3PlatformWaitKeyMouse();
            CenterMessage(11,21); }
        if (Player[input][16]!=0) {
            CenterMessage(41,21);
            valid = FALSE;
            ShowClickMessage();
            U3PlatformWaitKeyMouse();
            CenterMessage(42,21); }
        if (valid == TRUE)
            {
            Party[2]=byte;
            Party[byte+6] = input;
            Player[input][16] = 255;
            }
        else
            {
            for (byte = 0; byte<17; byte++)
                {
                Party[byte] = 0;
                }
            for (byte = 1; byte<21; byte++)
                {
                Player[byte][16] = 0;
                }
            PutParty();
            byte = 7;
            }
        }
form1:
        if (gDone!=TRUE)
            {
            if (byte<6)
                {
                for (byte = 1; byte<=Party[2]; byte++)
                    {
                    Player[Party[byte+6]][16] = 255;
                    }
                Party[3] = 0;
                Party[1] = 0x7E;
                Party[6] = 255; // WTF is this?
                xpos = 42;
                ypos = 20;
                Party[4] = xpos;
                Party[5] = ypos;
                PutParty();
                PutRoster();
                ResetSosaria();
                GetMiscStuff(0);
                PutMiscStuff();
                CenterMessage(12,22);
                ShowClickMessage();
                U3PlatformWaitKeyMouse();
                }
            }  */
}

void DisperseParty(void) {
    char byte;
    U3RenderClearBottom();
    CenterMessage(13, 17);
    for (byte = 1; byte < 21; byte++) {
        Player[byte][16] = 0;
    }
    PutParty();
    if (Party[7] == 0) {
        CenterMessage(14, 20);
        ShowClickMessage();
        U3PlatformWaitKeyMouse();
    } else {
        for (byte = 0; byte < 17; byte++) {
            Party[byte] = 0;
        }
        PutParty();
        CenterMessage(15, 19);
        ShowClickMessage();
        U3PlatformWaitKeyMouse();
    }
}

bool U3ValidateCharacterDraft(const U3CharacterDraft *draft) {
    if (!draft || draft->name[12] || !draft->name[0]) return false;
    bool visible = false, ended = false;
    for (int i = 0; i < 12; ++i) {
        if (!draft->name[i]) { ended = true; continue; }
        if (ended || draft->name[i] < 32 || draft->name[i] == 127) return false;
        visible |= draft->name[i] != ' ';
    }
    if (!visible || !draft->race || !draft->characterClass || !draft->sex ||
        !strchr("HEDBF", draft->race) || !strchr("FCWTPBLIDAR", draft->characterClass) ||
        !strchr("FMO", draft->sex)) return false;
    int total = 0;
    for (int i = 0; i < 4; ++i) {
        if (draft->attributes[i] < 5 || draft->attributes[i] > 25) return false;
        total += draft->attributes[i];
    }
    return total <= 50;
}

static bool BuildCharacterRecord(const U3CharacterDraft *draft, unsigned char record[65]) {
    if (!U3ValidateCharacterDraft(draft)) return false;
    memset(record, 0, 65);
    memcpy(record, draft->name, 12);
    memcpy(record + 18, draft->attributes, 4);
    record[22] = draft->race;
    record[23] = draft->characterClass;
    record[24] = draft->sex;
    record[17] = 'G';
    record[27] = record[29] = 100;
    record[32] = 1; record[33] = 50; record[36] = 150;
    record[40] = record[41] = record[48] = record[49] = 1;
    return true;
}

Boolean U3CharacterCreationSelfTest(void) {
    U3CharacterDraft draft = {{'A', 'd', 'a'}, {15, 15, 10, 10}, 'H', 'F', 'F'};
    unsigned char record[65];
    if (!BuildCharacterRecord(&draft, record) || record[17] != 'G' ||
        record[27] != 100 || record[29] != 100 || record[36] != 150 ||
        record[22] != 'H' || record[23] != 'F' || record[16] != 0) return false;
    draft.attributes[0] = 16;
    if (U3ValidateCharacterDraft(&draft)) return false;
    draft.attributes[0] = 4;
    if (U3ValidateCharacterDraft(&draft)) return false;
    draft.attributes[0] = 15; draft.race = 'X';
    if (U3ValidateCharacterDraft(&draft)) return false;
    draft.race = 'H'; memset(draft.name, 'A', 13);
    if (U3ValidateCharacterDraft(&draft)) return false;
    draft.name[12] = 0;
    if (!U3ValidateCharacterDraft(&draft)) return false;
    memset(draft.name, 0, 13); draft.name[0] = ' ';
    return !U3ValidateCharacterDraft(&draft);
}

static bool StoreCreatedCharacter(short slot, const U3CharacterDraft *draft) {
    unsigned char record[65], previous[65];
    if (slot < 1 || slot > 20 || Player[slot][0] || !BuildCharacterRecord(draft, record)) return false;
    memcpy(previous, Player[slot], sizeof(previous));
    memcpy(Player[slot], record, sizeof(record));
    PutRoster();
    if (U3IOLastError()) {
        memcpy(Player[slot], previous, sizeof(previous));
        return false;
    }
    return true;
}

Boolean U3CharacterStorageSelfTest(void) {
    unsigned char previous[21][65];
    memcpy(previous, Player, sizeof(previous));
    GetRoster();
    short slot = 1;
    while (slot <= 20 && Player[slot][0]) ++slot;
    U3CharacterDraft draft = {{'A', 'd', 'a'}, {15, 15, 10, 10}, 'H', 'F', 'F'};
    Boolean passed = slot <= 20 && StoreCreatedCharacter(slot, &draft);
    passed = passed && !StoreCreatedCharacter(slot, &draft);
    if (passed) {
        passed = U3IOOpenSaveContainer() == U3SaveContainerOpenResultOpened;
        if (passed) {
            memset(Player, 0, sizeof(Player));
            GetRoster();
            passed = !U3IOLastError() && !memcmp(Player[slot], "Ada", 3) &&
                Player[slot][17] == 'G' && Player[slot][18] == 15 &&
                Player[slot][27] == 100 && Player[slot][29] == 100 && Player[slot][16] == 0;
        }
    }
    memcpy(Player, previous, sizeof(previous));
    return passed;
}

void CreateChar(void) {
    if (U3CocoaHasMainSurface()) {
        Boolean available[20];
        for (int i = 0; i < 20; ++i) available[i] = Player[i + 1][0] == 0;
        short slot = 0;
        U3CharacterDraft draft = {{0}, {15, 15, 10, 10}, 'H', 'F', 'M'};
        if (!U3CocoaCreateCharacter(available, &slot, &draft)) return;
        if (!StoreCreatedCharacter(slot, &draft) && U3IOLastError()) {
            HandleError(U3IOLastError(), 44, BASERES);
        }
        return;
    }
    char byte, player;

    /*  U3RenderClearBottom();
    CenterMessage(16,15);
    Message(17,16,18);
    player = UInputNum(22,18);
    if ((player<1) || (player>20))
        {
        CenterMessage(18,21);
        U3PlatformWaitKeyMouse();
        }
    else
        {
        if (Player[player][0]!=0)
            {
            CenterMessage(19,21);
            U3PlatformWaitKeyMouse();
            }
        else
            { */
    player = 0;
    for (byte = 1; byte < 21; byte++) {
        if (Player[byte][0] == 0)
            player = byte;
    }
    if (player == 0) {
        DoStandardAlert(kAlertStopAlert, 6);
        //was StopAlert(BASERES+12, nil);
        return;
    }
    SetUpHelpWorld();
    player = RosterSelect();
    DrawFrame(gCurFrame);
    DrawExodusPict();
    if (player < 1 || player > 20) {
        U3RenderClearBottom();
        CenterMessage(20, 16);
        ShowClickMessage();
        U3PlatformWaitKeyMouse();
        DestroyHelpWorld();
        return;
    }
    if (gDone) {
        DestroyHelpWorld();
        return;
    }
    for (byte = 1; byte < 64; byte++) {
        Player[player][byte] = 0;
    }
    if (CharacterCreateDialog(player) == FALSE) {
        U3RenderClearBottom();
        CenterMessage(20, 16);
        ShowClickMessage();
        U3PlatformWaitKeyMouse();
        DestroyHelpWorld();
        return;
    }
    DestroyHelpWorld();
    U3RenderClearBottom();
    CenterMessage(21, 16);
    Player[player][17] = 'G';    // Good Health
    Player[player][27] = 100;    // Current Hit Points
    Player[player][29] = 100;    // Max Hit Points
    Player[player][32] = 1;
    Player[player][33] = 50;     // Food
    Player[player][36] = 150;    // Gold Pieces
    Player[player][41] = 1;      // Cloth
    Player[player][40] = 1;      // pre-readied
    Player[player][49] = 1;      // Dagger
    Player[player][48] = 1;      // pre-readied
    PutRoster();
    ShowClickMessage();
    U3PlatformWaitKeyMouse();
}
/*  }
}
            U3RenderClearBottom();
            U3RenderPrintPascalStringAt("\pEntry#",11,11);
            U3RenderPrintNumberAt(player, tx, ty);
            U3RenderPrintPascalStringAt("\pPoints:50",20,ty);
            U3RenderPrintPascalStringAt("\pName:",14,ty+2);
            U3RenderPrintPascalStringAt("\pSex:",14,ty+1);
            U3RenderPrintPascalStringAt("\pRace:",14,ty+1);
            U3RenderPrintPascalStringAt("\pType:",14,ty+1);
            U3RenderPrintPascalStringAt("\pStrength........",12,ty+1);
            U3RenderPrintPascalStringAt("\pDexterity.......",12,ty+1);
            U3RenderPrintPascalStringAt("\pIntelligence....",12,ty+1);
            U3RenderPrintPascalStringAt("\pWisdom..........",12,ty+1);
            U3RenderPrintPascalStringAt("\pO.K.:",14,ty+1);
            tx = 19;
            ty = 13;
            byte = 0;
NameBegin:
            input = U3PlatformCursorKey(false);
            if (gDone) goto CreateSave;
            if (input == 8) goto NameBS;
            if (input == 13) goto Sex;
            if (input<' ') goto NameBegin;
            Player[player][byte] = input;
            U3RenderPrintCharAt(input,tx,ty);
            byte++;
            if (byte>12) goto Sex;
            goto NameBegin;
NameBS:
            if (byte>0)
                {
                U3RenderPrintPascalStringAt("\p ",tx-1,ty);
                tx--;
                byte--;
                }
            goto NameBegin;
Sex:
            flag1 = FALSE;
            tx = 19;
            ty = 14;
            input = U3PlatformCursorKey(false);
            if (input>95) input -= 32;
            Player[player][24] = input;
            if (input == 'F') { U3RenderPrintPascalStringAt("\pFemale",tx,ty); flag1=TRUE; }
            if (input == 'O') { U3RenderPrintPascalStringAt("\pOther",tx,ty); flag1 = TRUE; }
            if (input == 'M') { U3RenderPrintPascalStringAt("\pMale",tx,ty); flag1 = TRUE; }
            if (flag1 == FALSE && gDone!=TRUE) goto Sex;
Race:
            flag1 = FALSE;
            tx = 19;
            ty = 15;
            input = U3PlatformCursorKey(false);
            if (input>95) input -= 32;
            Player[player][22] = input;
            for (byte=0; byte<5; byte++)
                {
                GetPascalStringFromArrayByIndex(tempStr, CFSTR("Races"), byte);
                if (input == tempStr[1])
                    {
                    flag1=TRUE;
                    U3RenderPrintPascalStringAt(tempStr,tx,ty);
                    }
                }
            if (flag1 == FALSE && gDone!=TRUE) goto Race;
Type:
            flag1 = FALSE;
            tx = 19;
            ty = 16;
            input = U3PlatformCursorKey(false);
            if (input>95) input -=  32;
            Player[player][23] = input;
            for (byte=0; byte<11; byte++)
                {
                if (input==careerTable[byte])
                    {
                    GetPascalStringFromArrayByIndex(tempStr, CFSTR("Classes"), byte);
                    flag1=TRUE;
                    U3RenderPrintPascalStringAt(tempStr,tx,ty);
                    }
                }
            if (flag1 == FALSE && gDone!=TRUE) goto Type;
            byte = 50;
            input = UInputNum(28,17);
            if ((input>25) || (input<5) || (input>byte)) goto CreateBegin;
            byte -= input;
            ShowPoints(byte);
            Player[player][18] = input;
            input = UInputNum(28,18);
            if ((input>25) || (input<5) || (input>byte)) goto CreateBegin;
            byte -= input;
            ShowPoints(byte);
            Player[player][19] = input;
            input = UInputNum(28,19);
            if ((input>25) || (input<5) || (input>byte)) goto CreateBegin;
            byte -= input;
            ShowPoints(byte);
            Player[player][20] = input;
            input = UInputNum(28,20);
            if ((input>25) || (input<5) || (input>byte)) goto CreateBegin;
            byte -= input;
            ShowPoints(byte);
            Player[player][21] = input;
CreateOK:
            tx = 19;
            ty = 21;
            input = U3PlatformCursorKey(false);
            if (input>95) input -= 32;
            if (input == 'Y') { U3RenderPrintCharAt('Y',tx,ty); goto CreateSave; }
            if (input!='N') goto CreateOK;
            goto CreateBegin;
CreateSave:
            if (!gDone)
                {
                Player[player][17] = 'G';
                Player[player][27] = 150;
                Player[player][29] = 150;
                Player[player][32] = 1; Player[player][33] = 50;
                Player[player][36] = 150;
                Player[player][41] = 1;
                Player[player][49] = 1;
                PutRoster();
                }
            else
                {
                Player[player][0] = 0;
                }
        }
    }
} */    // End of Old Character Creation Code

void ShowPoints(char points) {
    tx = 27;
    ty = 11;
    U3RenderPrintNumberAt(points, tx, ty);
    U3RenderPrintCharAt(' ', tx, ty);
}

void KillChar(void) {
    Boolean notDone = TRUE;

    while (notDone) {
        notDone = TerminateCharacterDialog();
        PutRoster();
        DrawOrganizeMenu();
    }
    /*  char    byte, input;
    U3RenderClearBottom();
    CenterMessage(22,15);
    Message(17,16,18);
    input = UInputNum(22,18);
    if ((input<1) || (input>20))
        {
        CenterMessage(18,20);
        ShowClickMessage();
        U3PlatformWaitKeyMouse();
        }
    else
        {
        if (Player[input][0] == 0)
            {
            CenterMessage(23,20);
            ShowClickMessage();
            U3PlatformWaitKeyMouse();
            }
        else
            {
            if (Player[input][16]!=0)
                {
                CenterMessage(24,21);
                ShowClickMessage();
                U3PlatformWaitKeyMouse();
                }
            else
                {
                for (byte = 0; byte<64; byte++)
                    {
                    Player[input][byte] = 0;
                    }
                PutRoster();
                CenterMessage(25,21);
                ShowClickMessage();
                U3PlatformWaitKeyMouse();
                }
            }
        } */
}

static void ScenarioRequire(Boolean condition, const char *name) {
    if (!condition) {
        fprintf(stderr, "Scenario failed: %s\n", name);
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "Scenario passed: %s\n", name);
}

static Boolean sDefeatScenarioCheck;

static void GameplayScenarioCheck(void) {
    U3PlatformSetBooleanPreference(U3PreferenceSpeechDisabled, true);
    U3PlatformSetBooleanPreference(U3PreferenceMusicDisabled, true);
    U3PlatformSetBooleanPreference(U3PreferenceUnconstrainedSpeed, true);
    U3PlatformSetBooleanPreference(U3PreferenceAutoSave, false);
    short member = Party[7];
    ScenarioRequire(member > 0 && member <= 20, "party fixture");
    Player[member][35] = 0;
    Player[member][36] = 100;
    Player[member][32] = 1;
    Player[member][33] = 50;
    U3CocoaQueueDiagnosticKeys("12\rn");
    Shop(1, 1);
    ScenarioRequire(Player[member][36] == 88 && Player[member][33] == 62,
                    "food purchase debits gold and adds rations");
    U3CocoaQueueDiagnosticKeys("99\r");
    Shop(1, 1);
    ScenarioRequire(Player[member][36] == 88 && Player[member][33] == 62,
                    "insufficient gold leaves balances unchanged");
    U3CocoaQueueDiagnosticKeys("\033");
    Shop(1, 1);
    ScenarioRequire(Player[member][36] == 88 && Player[member][33] == 62,
                    "cancel leaves balances unchanged");
    Player[member][23] = careerTable[2];
    Player[member][25] = 20;
    gTorch = 0;
    U3CocoaQueueDiagnosticKeys("1c");
    LetterCommand('C');
    ScenarioRequire(Player[member][25] == 10 && gTorch == 10,
                    "wizard selection and Lorum mana cost/effect");
    gTorch = 0;
    U3CocoaQueueDiagnosticKeys("1\033");
    LetterCommand('C');
    ScenarioRequire(Player[member][25] == 10 && gTorch == 0,
                    "spell cancellation preserves mana and effect");
    Player[member][25] = 0;
    U3CocoaQueueDiagnosticKeys("1c");
    LetterCommand('C');
    ScenarioRequire(Player[member][25] == 0 && gTorch == 0,
                    "insufficient mana prevents spell effect");
    int towns = 0, dungeons = 0;
    for (size_t i = 0; i < sizeof(LocationX) / sizeof(LocationX[0]); ++i) {
        short worldX = LocationX[i], worldY = LocationY[i];
        if (worldX >= 64 || worldY >= 64 || GetXYVal(worldX, worldY) / 2 != 0x0C)
            continue;
        xpos = worldX;
        ypos = worldY;
        Enter();
        ScenarioRequire(Party[3] == 2 && xpos == 1 && ypos == 32, "town entry");
        Routine6E6B();
        ScenarioRequire(Party[3] == 0 && xpos == worldX && ypos == worldY,
                        "town return restores world coordinates");
        ++towns;
    }
    ScenarioRequire(towns > 0, "town scenarios exercised");
    for (size_t i = 0; i < sizeof(LocationX) / sizeof(LocationX[0]); ++i) {
        short worldX = LocationX[i], worldY = LocationY[i];
        if (worldX >= 64 || worldY >= 64 || GetXYVal(worldX, worldY) / 2 != 0x0A)
            continue;
        xpos = worldX;
        ypos = worldY;
        gExitDungeon = 0;
        U3CocoaQueueDiagnosticKeys("k");
        Enter();
        ScenarioRequire(gExitDungeon == 1 && Party[3] == 0 &&
                        xpos == worldX && ypos == worldY && dungeonLevel == 0,
                        "dungeon entry and K ladder exit restore world coordinates");
        ScenarioRequire(!U3PlatformGetKeyMouse(2), "dungeon command consumed");
        ++dungeons;
    }
    ScenarioRequire(dungeons > 0, "dungeon scenarios exercised");
    ScenarioRequire(U3DungeonTraversalSelfTest(),
                    "dungeon traversal, doors, trap disarm/damage, chest reward and no repeat loot");
    /* Unlock the bonus spell and provide a nearby water tile for its ship. */
    Party[16] = 1;
    Player[member][25] = 90;
    short waterX = xpos + 1, waterY = ypos;
    PutXYVal(0, waterX, waterY);
    U3CocoaQueueDiagnosticKeys("1s");
    LetterCommand('C');
    Boolean shipCreated = FALSE;
    for (short y = -1; y <= 1; ++y) {
        for (short x = -1; x <= 1; ++x) {
            if (Absolute(x) + Absolute(y) == 1 && GetXYVal(xpos + x, ypos + y) == 44)
                shipCreated = TRUE;
        }
    }
    ScenarioRequire(Player[member][25] == 0 && shipCreated,
                    "Flotellum bonus spell consumes mana and creates a ship");
    ScenarioRequire(U3ManualCombatSelfTest(1, FALSE),
                    "manual pass, enemy attack, A/north kill, victory and world return");
    ScenarioRequire(U3ManualCombatSelfTest(4, FALSE),
                    "four-player turn order, east movement, enemy round and victory");
    ScenarioRequire(U3ManualCombatSelfTest(4, TRUE),
                    "blocked movement preserves tiles; dead/ashes turns skip input; victory");
    ScenarioRequire(U3AutoCombatSelfTest(),
                    "auto combat generates attack macro instead of passing");
    Party[2] = 1;
    Party[8] = Party[9] = Party[10] = 0;
    member = Party[7];
    unsigned char reserved[65];
    Player[0][17] = 'G';
    memcpy(reserved, Player[0], sizeof(reserved));
    Player[member][17] = 'G';
    Player[member][26] = 0;
    Player[member][27] = 10;
    ScenarioRequire(!HPSubtract(member, 9) && Player[member][27] == 1 &&
                    Player[member][17] == 'G', "nonlethal damage preserves living status");
    ScenarioRequire(HPSubtract(member, 1) && Player[member][27] == 0 &&
                    Player[member][17] == 'D', "lethal damage sets zero HP and death");
    ScenarioRequire(!CheckAlive(1), "empty party slot is not a living character");
    sDefeatScenarioCheck = TRUE;
    CheckAllDead();
    sDefeatScenarioCheck = FALSE;
    ScenarioRequire(gResurrect && Party[3] == 0 && xpos == 42 && ypos == 20 &&
                    Player[member][17] == 'G' && Player[member][26] == 0 &&
                    Player[member][27] == 100 && Player[member][36] == 150 &&
                    Player[member][48] == 1 && Player[member][40] == 1,
                    "party defeat restores health, starting equipment and world location");
    ScenarioRequire(memcmp(reserved, Player[0], sizeof(reserved)) == 0,
                    "resurrection leaves reserved roster slot unchanged");
    gResurrect = FALSE;
    sDefeatScenarioCheck = TRUE;
    ScenarioRequire(U3CombatDefeatSelfTest(), "enemy-caused defeat returns from Combat after resurrection");
    sDefeatScenarioCheck = FALSE;
    ScenarioRequire(U3IOOpenSaveContainer() == U3SaveContainerOpenResultOpened,
                    "resurrection save container reopens");
    Player[member][17] = 'D';
    Player[member][27] = 0;
    xpos = ypos = 0;
    GetParty();
    GetRoster();
    GetSosaria();
    ScenarioRequire(Party[3] == 0 && xpos == 42 && ypos == 20 &&
                    Player[member][17] == 'G' && Player[member][26] == 0 &&
                    Player[member][27] == 100 && Player[member][48] == 1 &&
                    Player[member][40] == 1,
                    "reloaded resurrection restores party position, health and equipment");
    gResurrect = FALSE;
    DrawMap(xpos, ypos);
    ScenarioRequire(U3CocoaWriteMainBitmap(getenv("U3_WORLD_RENDER_CHECK")), "world screenshot");
}

static void CommandTextCheck(void) {
    U3PlatformSetBooleanPreference(U3PreferenceSpeechDisabled, true);
    U3PlatformSetBooleanPreference(U3PreferenceMusicDisabled, true);
    U3PlatformSetBooleanPreference(U3PreferenceUnconstrainedSpeed, true);
    if (!U3CocoaKeyboardSelfTest()) exit(EXIT_FAILURE);
    if (U3CocoaIsHeadlessDiagnostic()) {
        short savedWhere = gUpdateWhere, savedMouse = gMouseState;
        gUpdateWhere = 2;
        gMouseState = 0;
        U3CocoaQueueDiagnosticMouse(0, 0);
        if (!U3PlatformGetKeyMouse(2)) exit(EXIT_FAILURE);
        gUpdateWhere = savedWhere;
        gMouseState = 4;
        U3CocoaQueueDiagnosticMouse(0, 0);
        if (U3PlatformGetKeyMouse(2)) exit(EXIT_FAILURE);
        U3CocoaQueueDiagnosticMouse(30 * blkSiz, 2 * blkSiz);
        if (!U3PlatformGetKeyMouse(2) || gKeyPress != '1') exit(EXIT_FAILURE);
        gMouseState = savedMouse;
    }
    memset(Monsters, 0, sizeof(Monsters));
    Monsters[0] = 0x40;
    Monsters[XMON] = xpos + 1;
    Monsters[YMON] = ypos;
    Monsters[HPMON] = 1;
    memset(Talk, 0, sizeof(Talk));
    const char *reply = "WELCOME TRAVELER!\377SEEK THE CASTLE.\377";
    memcpy(Talk + 1, reply, strlen(reply));
    U3RenderDrawPrompt();
    U3CocoaQueueDiagnosticKeys("t16");
    U3CocoaPumpEvents();
    if (U3PlatformWaitKeyMouse() != 't') exit(EXIT_FAILURE);
    LetterCommand('T');
    if (xs != xpos + 1 || ys != ypos) exit(EXIT_FAILURE);
    U3CocoaTextCheckpoint("reply");
    if (U3PlatformGetKeyMouse(2) || gKeyPress) exit(EXIT_FAILURE);
    U3CocoaQueueDiagnosticKeys("x");
    U3CocoaFlushInput();
    if (U3PlatformGetKeyMouse(2) || gKeyPress) exit(EXIT_FAILURE);
    Str255 input;
    U3CocoaQueueDiagnosticKeys("ab\bcd\r");
    UInputText(24, 23, input, 8, false);
    if (input[0] != 3 || memcmp(input + 1, "ACD", 3)) exit(EXIT_FAILURE);
    U3CocoaQueueDiagnosticKeys("1x2\b3\r");
    if (UInputNum(24, 23) != 13) exit(EXIT_FAILURE);
    U3CocoaQueueDiagnosticKeys("12\033");
    if (UInputNum(24, 23) != 0) exit(EXIT_FAILURE);
    Str255 searchText = "\pABAB", search = "\pAB", replacement = "\pXYZ";
    SearchReplace(searchText, search, replacement);
    if (searchText[0] != 6 || memcmp(searchText + 1, "XYZXYZ", 6)) exit(EXIT_FAILURE);
    if (StringLocation("\pXYZ", "\pZ") != 3 || StringLocation("\pXYZ", "\pXYZ") != 1)
        exit(EXIT_FAILURE);
    Str255 full;
    full[0] = 255;
    memset(full + 1, 'A', 255);
    SearchReplace(full, "\pA", "\pAA");
    if (full[0] != 255 || full[255] != 'A') exit(EXIT_FAILURE);
    AddString(full, "\pZ");
    if (full[0] != 255 || full[255] != 'A') exit(EXIT_FAILURE);
    Str255 longText;
    longText[0] = 255;
    memset(longText + 1, 'W', 255);
    RewrapString(longText, false);
    UPrint(longText, 24, 23);
    UPrint("\p\nEND OF TEXT", 24, 23);
    U3CocoaTextCheckpoint("long");
    fprintf(stderr, "Command text: keyboard, T/1/east/reply, editing, numeric input, cancel, 255-byte text passed\n");
}

void Game(void) {
    Boolean key;
    Boolean diagnosticInputQueued = FALSE;
    unsigned int diagnosticTurns = 0;
    int count;
    long time;

    if (getenv("U3_WORLD_RENDER_CHECK")) fprintf(stderr, "World render: Game start\n");
    ClearScreen();
    DisposeIntroData();
    GetDungeonGraphics();
    if (getenv("U3_WORLD_RENDER_CHECK")) fprintf(stderr, "World render: dungeon graphics ready\n");
    GetPortraits();
    if (getenv("U3_WORLD_RENDER_CHECK")) fprintf(stderr, "World render: portraits ready\n");
    lastCard = 0x1E;
    GetSosaria();
    if (getenv("U3_WORLD_RENDER_CHECK")) fprintf(stderr, "World render: Sosaria ready\n");
    gUpdateWhere = 3;
    DrawFrame(1);
    ClearUpdatePort();
    ox = xpos - 1;
    oy = ypos - 1;
    gTimeNegate = 0;
    gSongPlaying = 0;
    gSongCurrent = gSongNext = 1;
        if (getenv("U3_WORLD_RENDER_CHECK") || getenv("U3_WORLD_INPUT_CHECK") ||
            getenv("U3_WORLD_MOUSE_CHECK"))
        gSongCurrent = gSongNext = 0;
    LWEnableMenuItem(gFileMenu, ABORTID);
    InitCursor();
    ShowChars(true);
    if (getenv("U3_RESUME_CHECK")) {
        short member = Party[7];
        ScenarioRequire(Party[2] == 1 && member > 0 && member <= 20,
                        "fresh process loaded saved party without creating characters");
        ScenarioRequire(Party[3] == 0 && xpos == 42 && ypos == 20 &&
                        Player[member][17] == 'G' && Player[member][26] == 0 &&
                        Player[member][27] == 100 && Player[member][48] == 1 &&
                        Player[member][40] == 1,
                        "fresh process resumed resurrection health, equipment and position");
    }
    while (!gDone) {
        //      if (gUpdateWhere==3) gSongNext = 1;
        DrawMap(xpos, ypos);
        if (getenv("U3_GAMEPLAY_SCENARIO_CHECK") && getenv("U3_WORLD_RENDER_CHECK")) {
            GameplayScenarioCheck();
            gDone = TRUE;
            return;
        }
        if (getenv("U3_COMMAND_TEXT_CHECK")) {
            CommandTextCheck();
            gDone = TRUE;
            return;
        }
        if (diagnosticTurns == 2) {
            const char *outputPath = getenv("U3_WORLD_MOUSE_CHECK") ?
                getenv("U3_WORLD_MOUSE_CHECK") : getenv("U3_WORLD_INPUT_CHECK");
            U3CocoaPumpEvents();
            if (!U3CocoaWriteMainBitmap(outputPath))
                exit(EXIT_FAILURE);
            fprintf(stderr, "World input: two turns completed; next input cycle reached\n");
            gDone = TRUE;
            return;
        }
        if ((getenv("U3_WORLD_INPUT_CHECK") || getenv("U3_WORLD_MOUSE_CHECK")) && !diagnosticInputQueued) {
            fprintf(stderr, "World input: before (%d,%d)\n", xpos, ypos);
            if (getenv("U3_WORLD_MOUSE_CHECK"))
                U3CocoaQueueDiagnosticMouse(600, 384);
            else
                U3CocoaQueueDiagnosticKey(diagnosticTurns ? '4' : '6');
            diagnosticInputQueued = TRUE;
        }
        if (getenv("U3_WORLD_RENDER_CHECK")) {
            if (getenv("U3_MUSIC_TRANSITION_CHECK")) {
                U3PlatformSetBooleanPreference(U3PreferenceMusicDisabled, false);
                gSongCurrent = gSongNext = 1;
                U3AudioUpdateMusic();
                U3AudioStopMusic();
                fprintf(stderr, "Music transition: start/stop completed\n");
            }
            U3CocoaPumpEvents();
            Boolean written = U3CocoaWriteMainBitmap(getenv("U3_WORLD_RENDER_CHECK"));
            fprintf(stderr, "World render check: %s at (%d,%d)\n",
                    written ? "completed" : "FAILED", xpos, ypos);
            gDone = TRUE;
            return;
        }
        gResurrect = FALSE;
        ShowChars(false);
        CheckAllDead();
        U3RenderDrawPrompt();
        gMouseState = 1;
        count = 60; /* SUPPOSED TO BE 0x25! */
        Boolean allowDiagonal = !(U3PlatformGetBooleanPreference(U3PreferenceNoDiagonals));
        while (count > 0) {
            WhirlPool();
            //FullUpdate();
            key = U3PlatformGetKeyMouse(0);
            if (key)
                count = 0;
            count--;
            if (count == 40)
                DoAutoHeal();
        }
        if (gDone)
            return;
        if (!key)
            gKeyPress = ' ';
        if (gKeyPress > 95)
            gKeyPress -= 32;
        if ((gKeyPress >= 'A') && (gKeyPress <= 'Z')) {
            LetterCommand(gKeyPress);
            Routine6E35();
        } else {
            switch (gKeyPress) {
                case ' ': Pass(); break;
                case 28: West(); break;
                case 29: East(); break;
                case 30: North(); break;
                case 31: South(); break;
                case '4': West(); break;
                case '6': East(); break;
                case '8': North(); break;
                case '2': South(); break;
                case '1':
                    if (allowDiagonal)
                        SouthWest();
                    break;
                case '3':
                    if (allowDiagonal)
                        SouthEast();
                    break;
                case '7':
                    if (allowDiagonal)
                        NorthWest();
                    break;
                case '9':
                    if (allowDiagonal)
                        NorthEast();
                    break;
                default: break;
            }
            Routine6E35();
            if (getenv("U3_WORLD_INPUT_CHECK") || getenv("U3_WORLD_MOUSE_CHECK")) {
                ++diagnosticTurns;
                fprintf(stderr, "World input: turn=%u key=%d mouse=%d after (%d,%d)\n",
                        diagnosticTurns, (int)gKeyPress, (int)gMouseKey, xpos, ypos);
                diagnosticInputQueued = FALSE;
            }
            if (StillDown()) {
                time = U3PlatformTickCount() + 6;
                while (U3PlatformTickCount() < time) {
                }
            }
            while (StillDown()) {
                DrawMap(xpos, ypos);
                DrawMapPause();
                WhirlPool();
                FullUpdate();
                CursorUpdate();
                switch (gCurCursor) {
                    case 0:
                        U3RenderDrawPrompt();
                        Pass();
                        StillDownPause();
                        break;
                    case 1:
                        U3RenderDrawPrompt();
                        North();
                        StillDownPause();
                        break;
                    case 2:
                        U3RenderDrawPrompt();
                        South();
                        StillDownPause();
                        break;
                    case 3:
                        U3RenderDrawPrompt();
                        West();
                        StillDownPause();
                        break;
                    case 4:
                        U3RenderDrawPrompt();
                        East();
                        StillDownPause();
                        break;
                    case 22:
                        U3RenderDrawPrompt();
                        NorthWest();
                        StillDownPause();
                        break;
                    case 23:
                        U3RenderDrawPrompt();
                        NorthEast();
                        StillDownPause();
                        break;
                    case 24:
                        U3RenderDrawPrompt();
                        SouthWest();
                        StillDownPause();
                        break;
                    case 25:
                        U3RenderDrawPrompt();
                        SouthEast();
                        StillDownPause();
                        break;
                    default: break;
                }
            }
        }
    }
}

void StillDownPause(void) {
    long time = U3PlatformTickCount() + 1;

    Routine6E35();
    if (U3PlatformGetBooleanPreference(U3PreferenceUnconstrainedSpeed))
        return;
    IdleUntil(time);
}

void LetterCommand(char key) {
    switch (key) {
        case 'A': Attack(); break;
        case 'B': Board(); break;
        case 'C': Cast(0, 0); break;
        case 'D': Descend(); break;
        case 'E': Enter(); break;
        case 'F': Fire(); break;
        case 'G': GetChest(0, 0); break;
        case 'H': HandEquip(0, 0, 0, 0, 0); break;
        case 'I': Ignite(); break;
        case 'J': JoinGold(0); break;
        case 'K': Klimb(); break;
        case 'L': Look(); break;
        case 'M': ModifyOrder(); break;
        case 'N': NegateTime(0, 0); break;
        case 'O': OtherCommand(0); break;
        case 'P': PeerGem(); break;
        case 'Q': QuitSave(0); break;
        case 'R': ReadyWeapon(0, 0); break;
        case 'S': Steal(); break;
        case 'T': Transact(); break;
        case 'U': Unlock(); break;
        case 'V': Volume(); break;
        case 'W': WearArmour(0, 0); break;
        case 'X': Exit(); break;
        case 'Y': Yell(0); break;
        case 'Z': Stats(0, 0); break;
    }
}

/* ------------------- Run-time game code begins here ------------------------ */

void Pass(void) {
    U3RenderPrintMessage(23);
}

void North(void) {
    U3RenderPrintMessage(24);
    if (ValidTrans(1) == 0) {
        NoGo();
    } else {
        if (ValidDir(GetXYVal(xpos, ypos - 1)) == 0) {
            NoGo();
        } else {
            ypos--;
            ypos = MapConstrain(ypos);
        }
    }
}

void South(void) {
    U3RenderPrintMessage(25);
    if (ValidTrans(3) == 0) {
        NoGo();
    } else {
        if (ValidDir(GetXYVal(xpos, ypos + 1)) == 0) {
            NoGo();
        } else {
            ypos++;
            ypos = MapConstrain(ypos);
        }
    }
}

void East(void) {
    gHorseFacingEast = true;
    U3RenderPrintMessage(26);
    if (ValidTrans(2) == 0) {
        NoGo();
    } else {
        if (ValidDir(GetXYVal(xpos + 1, ypos)) == 0) {
            NoGo();
        } else {
            xpos++;
            xpos = MapConstrain(xpos);
        }
    }
}

void West(void) {
    gHorseFacingEast = false;
    U3RenderPrintMessage(27);
    if (ValidTrans(4) == 0) {
        NoGo();
    } else {
        if (ValidDir(GetXYVal(xpos - 1, ypos)) == 0) {
            NoGo();
        } else {
            xpos--;
            xpos = MapConstrain(xpos);
        }
    }
}

void SouthEast(void) {
    gHorseFacingEast = true;
    U3RenderPrintMessage(251);
    if ((ValidTrans(2) == 0) || (ValidTrans(3) == 0)) {
        NoGo();
    } else {
        if (ValidDir(GetXYVal(xpos + 1, ypos + 1)) == 0) {
            NoGo();
        } else {
            ypos++;
            ypos = MapConstrain(ypos);
            xpos++;
            xpos = MapConstrain(xpos);
        }
    }
}

void SouthWest(void) {
    gHorseFacingEast = false;
    U3RenderPrintMessage(250);
    if ((ValidTrans(3) == 0) || (ValidTrans(4) == 0)) {
        NoGo();
    } else {
        if (ValidDir(GetXYVal(xpos - 1, ypos + 1)) == 0) {
            NoGo();
        } else {
            ypos++;
            ypos = MapConstrain(ypos);
            xpos--;
            xpos = MapConstrain(xpos);
        }
    }
}

void NorthEast(void) {
    gHorseFacingEast = true;
    U3RenderPrintMessage(253);
    if ((ValidTrans(1) == 0) || (ValidTrans(2) == 0)) {
        NoGo();
    } else {
        if (ValidDir(GetXYVal(xpos + 1, ypos - 1)) == 0) {
            NoGo();
        } else {
            ypos--;
            ypos = MapConstrain(ypos);
            xpos++;
            xpos = MapConstrain(xpos);
        }
    }
}

void NorthWest(void) {
    gHorseFacingEast = false;
    U3RenderPrintMessage(252);
    if ((ValidTrans(1) == 0) || (ValidTrans(4) == 0)) {
        NoGo();
    } else {
        if (ValidDir(GetXYVal(xpos - 1, ypos - 1)) == 0) {
            NoGo();
        } else {
            ypos--;
            ypos = MapConstrain(ypos);
            xpos--;
            xpos = MapConstrain(xpos);
        }
    }
}

void Attack(void) {
    short monNum;

    U3RenderPrintMessage(28);
    U3PlatformGetDirection(0);
    monNum = MonsterHere(xs, ys);
    if (monNum == 255) {
        NotHere();
        return;
    }
    AttackCode(monNum);
}

void AttackCode(short whichMon) { /* $52B3 */
    short tileon;
    DrawMap(xpos, ypos);
    xs = Monsters[whichMon + XMON];
    ys = Monsters[whichMon + YMON];
    tileon = Monsters[whichMon + TILEON];
    if (tileon != 0) {
        tileon = ((tileon / 4) & 0x03) + 0x24;
    }
    PutXYVal(tileon, xs, ys);
    gMonType = Monsters[whichMon] / 2;
    gMonVarType = Monsters[VARMON + whichMon];
    Monsters[whichMon] = 0;
    if (gMonType == 0x1E) { /* Pirate */
        if (Party[1] != 0x16)
            PutXYVal(0x2C, xs, ys);
    }
    Combat();
}

void Board(void) {
    short tileOn;
    if (Party[1] != 0x7E) { /* Not 'Ranger' shape? */
        U3RenderPrintMessage(29); /*Board*/
        What2();
    } else {
        tileOn = GetXYVal(xpos, ypos);
        if (tileOn == 0x28) { /* horse */
            PutXYVal(0x04, xpos, ypos); /* replace with grass */
            Party[1] = 0x14;
            U3RenderPrintMessage(30);
            U3AudioPlaySound(U3SoundEffectMountHorse, true);    // was 0xDB
        } else if (tileOn == 0x2C)                       /* ship */
        {
            PutXYVal(0x00, xpos, ypos); /* replace with water */
            Party[1] = 0x16;
            U3RenderPrintMessage(31);
        } else {
            U3RenderPrintMessage(29); /*Board*/
            What2();
        }
    }
}

/* See UltimaSpellCombat.c for Cast() */

void Descend(void) {
    if (Party[16] == 0) {
        U3RenderPrintMessage(32);
        What2();
        return;
    }
    U3RenderPrintPascalString("\pDiorama\n");
    DrawDioramaMap();
}

void Enter(void) {
    short x, placeNum, tile, newval, chnum;
    Str255 str, addStr;

    //  U3RenderPrintMessage(33);
    if (Party[3] != 0xFF && Party[3] != 00) {
        U3RenderPrintMessage(33);
        What();
        return;
    }
    if (Party[3] == 00) {
        Party[4] = xpos;
        Party[5] = ypos;
        placeNum = 666;
        for (x = 0; x < sizeof(LocationX) / sizeof(LocationX[0]); x++) {
            if (xpos == LocationX[x] && ypos == LocationY[x])
                placeNum = x;
        }
        if (placeNum == 666) {
            U3RenderPrintMessage(33);
            What2();
            return;
        }
        zp[0xE3] = xpos;
        zp[0xE4] = ypos;
        tile = GetXYVal(xpos, ypos) / 2;
        if (tile == 0x0A) { /* Dungeon */
            GetPascalStringFromArrayByIndex(str, CFSTR("Messages"), 32);
            GetPascalStringFromArrayByIndex(addStr, CFSTR("Messages"), 33);
            AddString(str, addStr);
            U3RenderPrintPascalString(str);
            newval = 1;
            dungeonLevel = 0;
            xpos = 1;
            ypos = 1;
            heading = 1;
            goto enterdone;
        }
        if (tile == 0x0C) { /* Town */
            GetPascalStringFromArrayByIndex(str, CFSTR("Messages"), 32);
            GetPascalStringFromArrayByIndex(addStr, CFSTR("Messages"), 34);
            AddString(str, addStr);
            U3RenderPrintPascalString(str);
            newval = 2;
            ypos = 32;
            xpos = 1;
            heading = 2;
            goto enterdone;
        }
        if (tile == 0x0E) { /* Castle */
            GetPascalStringFromArrayByIndex(str, CFSTR("Messages"), 32);
            GetPascalStringFromArrayByIndex(addStr, CFSTR("Messages"), 35);
            AddString(str, addStr);
            U3RenderPrintPascalString(str);
            newval = 3;
            xpos = 32;
            ypos = 62;
            zp[0x0F] = 2;
            goto enterdone;
        }
        What2();
        return;
    }
    if (Party[3] == 0xFF) {   // Ambrosia
        if (GetXYVal(xpos, ypos) != 0xF8) {
            U3RenderPrintMessage(33);
            What2();
            return;
        }
        //      U3RenderPrintMessage(37);
        GetPascalStringFromArrayByIndex(str, CFSTR("Messages"), 32);
        GetPascalStringFromArrayByIndex(addStr, CFSTR("Messages"), 36);
        AddString(str, addStr);
        U3RenderPrintPascalString(str);
        chnum = GetChar();
        if (chnum < 1 || chnum > 4) {
            U3AudioPlaySound(U3SoundEffectError1, true);
            return;
        }
        if (CheckAlive(chnum - 1) == FALSE) {
            Incap();
            return;
        }
        gSongCurrent = gSongNext = 0x0A;
        Shrine(chnum);
        gSongNext = 0x0B;
        return;
    }
enterdone: /* $59C9 */
    gSongCurrent = gSongNext = 0;
    if (U3PlatformGetBooleanPreference(U3PreferenceAutoSave)) {
        PutRoster();
        PutParty();
        PutSosaria();
    } else
        PushSosaria();

    if (placeNum > 18) {
        LoadUltimaMap(BASERES + placeNum + 3);
    } else {
        LoadUltimaMap(BASERES + placeNum);
    }
    Party[3] = newval;
    if (Party[3] == 1) {   // Dungeon
        gSongNext = 1;
        DungeonStart(0);
        Routine6E6B();
        return;
    }
    if (Party[3] == 2) {   // Town
        gSongNext = 2;
        return;
    }
    if (placeNum != 1) {
        gSongNext = 3;
        return;
    }
    if (Party[16] == 1) {
        SafeExodus();
    }
    gSongCurrent = 0;
    gSongNext = 7;
    return;
}

void Fire(void) {
    short x, value, store;
    U3RenderPrintMessage(38);
    if (Party[1] != 0x16) {
        What2();
    } else {
        U3RenderPrintMessage(39);
        U3PlatformGetDirection(0);
        U3AudioPlaySound(U3SoundEffectShoot, true);    // was 0xEA
        xs = xpos;
        ys = ypos;
        x = 4;
    fireloop:
        x--;
        if (x < 1) {
            DrawMap(xpos, ypos);
            return;
        }
        xs = MapConstrain(xs + dx);
        ys = MapConstrain(ys + dy);
        store = MonsterHere(xs, ys);
        if (store < 128)
            goto firehit;
        store = GetXYVal(xs, ys);
        if (store >= 0x24 && store < 0x28)
            gBallTileBackground = (store & 0x3) * 2;    // It's a chest, use whatever tile the chest is on.
        else
            gBallTileBackground = store / 2;
        PutXYVal(0xF4, xs, ys);
        DrawMap(xpos, ypos);
        DrawMapPause();
        PutXYVal(store, xs, ys);
        ScrollThings();
        goto fireloop;
    firehit:
        value = GetXYVal(xs, ys);
        short monster = MonsterHere(xs, ys);
        gBallTileBackground = (monster == 255) ? value : Monsters[monster + TILEON] / 2;
        PutXYVal(0xF4, xs, ys);
        DrawMap(xpos, ypos);
        U3AudioPlaySound(U3SoundEffectHit, false);    // was 0xF7
        if (Monsters[store] == 0x3C && U3PlatformRandom(0, 255) < 128) {
            PutXYVal(value, xs, ys);
            DrawTiles();
            return;
        }
        if (U3PlatformRandom(0, 255) < 128) {
            PutXYVal(value, xs, ys);
            DrawTiles();
            return;
        } else {
            ShowHit(xs - xpos + 5, ys - ypos + 5, 0x7A, Monsters[store + TILEON] / 2);
            PrintMonster(Monsters[store] / 2, true, Monsters[store + VARMON]);
            U3RenderPrintMessage(117);
            PutXYVal(Monsters[store + TILEON], xs, ys);
            Monsters[store] = 0;
        }
    }
}

void GetChest(short spell, short chnum) {
    char tile;
    short rosNum, gold, wpn, arm, x;
    Str32 str1, str2;

    if (spell == 2) {
        rosNum = Party[6 + chnum];
        goto GetChestBooty;
    }
    if (spell == 0) {
        m5BDC = 0xFF;
        U3RenderPrintMessage(40);
        chnum = GetChar();
        if (chnum < 1 || chnum > 4) {
            U3RenderPrintMessage(41);
            U3AudioPlaySound(U3SoundEffectError1, true);
            return;
        }
        if (CheckAlive(chnum - 1) == FALSE) {
            Incap();
            return;
        }
    }
    rosNum = Party[6 + chnum];
    xs = xpos;
    ys = ypos;
    if (Party[3] != 1) { /* party not in dungeon */
        tile = GetXYVal(xpos, ypos);
        if ((tile < 0x24) || (tile > 0x27)) { /* izzit not a chest? */
            NotHere();
            return;
        }
        tile = (tile & 0x3) * 4;
        if (tile == 0)
            tile = 0x20;
        PutXYVal(tile, xpos, ypos);
    } else { /* party in dungeon */
        if (GetXYDng(xpos, ypos) != 0x40) {
            NotHere();
            return;
        }
        PutXYDng(0, xpos, ypos);
    }
    if ((m5BDC == 0) || (U3PlatformRandom(0, 255) > 127))
        goto GetChestBooty;
    switch ((U3PlatformRandom(0, 255) & U3PlatformRandom(0, 255)) & 0x03) {
        case 0:
            U3RenderPrintMessage(42);
            if (StealDisarmFail(rosNum) == FALSE) {
                U3RenderPrintMessage(46);
                U3AudioPlaySound(U3SoundEffectOuch, false);    // was 0xFA
                goto GetChestBooty;
            } else {
                InverseChar(chnum - 1);
                U3AudioPlaySound(U3SoundEffectHit, false);    // was 0xF7
                InverseChar(chnum - 1);
                HPSubtract(rosNum, U3PlatformRandom(0, 255) & 0x37);
                goto GetChestBooty;
            }
        case 1:
            U3RenderPrintMessage(43);
            if (StealDisarmFail(rosNum) == FALSE) {
                U3RenderPrintMessage(46);
                U3AudioPlaySound(U3SoundEffectOuch, false);    // was 0xFA
                goto GetChestBooty;
            } else {
                InverseChar(chnum - 1);
                U3AudioPlaySound(U3SoundEffectHit, false);    // was 0xF7
                InverseChar(chnum - 1);
                Player[rosNum][17] = 'P';
                goto GetChestBooty;
            }
        case 2:
            U3RenderPrintMessage(44);
            if (StealDisarmFail(rosNum) == FALSE) {
                U3RenderPrintMessage(46);
                U3AudioPlaySound(U3SoundEffectOuch, false);    // was 0xFA
                goto GetChestBooty;
            } else {
                BombTrap();
                return;
            }
        case 3:
            U3RenderPrintMessage(45);
            if (StealDisarmFail(rosNum) == FALSE) {
                U3RenderPrintMessage(46);
                U3AudioPlaySound(U3SoundEffectOuch, false);    // was 0xFA
                goto GetChestBooty;
            } else {
                for (x = 0; x < 4; x++) {
                    if (CheckAlive(x) == TRUE) {
                        InverseChar(x);
                        U3AudioPlaySound(U3SoundEffectHit, false);    // was 0xF7
                        InverseChar(x);
                        Player[Party[7 + x]][17] = 'P';
                    }
                }
                goto GetChestBooty;
            }
    }
GetChestBooty:
    U3AudioPlaySound(U3SoundEffectCreak, true);    // was 0xEB
    GetPascalStringFromArrayByIndex(str1, CFSTR("Messages"), 46);
    gold = U3PlatformRandom(0, 100);
    if (gold < 30)
        gold += 30;
    NumToString(gold, str2);
    AddString(str1, str2);
    str1[++str1[0]] = '\n';
    U3RenderPrintPascalString(str1);
    //  U3RenderPrintNumberPadded(gold,2);
    //  U3RenderPrintPascalString("\p\n");
    if (AddGold(rosNum, gold, TRUE) == FALSE) {
        U3RenderPrintMessage(48);
        U3AudioPlaySound(U3SoundEffectError1, true);
    }
    if (U3PlatformRandom(0, 255) > 63)
        return;
    wpn = U3PlatformRandom(0, 255);
    if (wpn < 128) {
        wpn = (U3PlatformRandom(0, 255) & wpn) & 0x07;
        if (wpn != 0) {
            GetPascalStringFromArrayByIndex(str1, CFSTR("Messages"), 48);
            GetPascalStringFromArrayByIndex(str2, CFSTR("WeaponsArmour"), wpn);
            AddString(str1, str2);
            str1[++str1[0]] = '\n';
            U3RenderPrintPascalString(str1);
            AddItem(rosNum, 48 + wpn, 1);
            return;
        }
    }
    arm = U3PlatformRandom(0, 255);
    if (arm < 128) {
        arm = (U3PlatformRandom(0, 255) & arm) & 0x03;
        if (arm != 0) {
            GetPascalStringFromArrayByIndex(str1, CFSTR("Messages"), 49);
            GetPascalStringFromArrayByIndex(str2, CFSTR("WeaponsArmour"), arm + 16);
            AddString(str1, str2);
            str1[++str1[0]] = '\n';
            U3RenderPrintPascalString(str1);
            AddItem(rosNum, 40 + arm, 1);
        }
    }
}

void BombTrap(void) { /* $5C63 */
    short chnum;
    for (chnum = 0; chnum < Party[2]; chnum++) {
        if (CheckAlive(chnum) == TRUE) {
            InverseChar(chnum);
            ForceUpdateMain();
            U3AudioPlaySound(U3SoundEffectHit, false);    // was 0xF7
            InverseChar(chnum);
            HPSubtract(Party[7 + chnum], U3PlatformRandom(0, 255) & 0x77);
            HPSubtract(Party[7 + chnum], (zp[0x13] + 1) * 8);
            ForceUpdateMain();
        }
    }
    ShowChars(false);
}

void HandEquip(short chnum1, short chnum2, short item, char what2, short value) { /* $5D8F */
    Boolean preset;
    short rosNum1, rosNum2, i, toAmount, fromAmount;

    preset = (chnum1 > 0);
    if (!preset) {   // Standard mode, ask for everything
        U3RenderPrintMessage(51);
        chnum1 = GetChar();
        if (chnum1 < 1 || chnum1 > 4) {
            U3RenderPrintMessage(41);
            U3AudioPlaySound(U3SoundEffectError1, true);
            return;
        }
        if (!Player[Party[6 + chnum1]][17]) {
            U3RenderPrintMessage(41);
            U3AudioPlaySound(U3SoundEffectError1, true);
            return;
        }
        U3RenderPrintMessage(52);
        chnum2 = GetChar();
        if (chnum2 < 1 || chnum2 > 4) {
            U3RenderPrintMessage(41);
            U3AudioPlaySound(U3SoundEffectError1, true);
            return;
        }
        if (!Player[Party[6 + chnum2]][17]) {
            U3RenderPrintMessage(41);
            U3AudioPlaySound(U3SoundEffectError1, true);
            return;
        }
    }
    if (chnum1 == chnum2) {
        U3AudioPlaySound(U3SoundEffectError1, true);
        return;
    }    // hand to-from same char
    rosNum1 = Party[6 + chnum1];
    rosNum2 = Party[6 + chnum2];
    if (!Player[rosNum1][17] || !Player[rosNum2][17]) {
        U3AudioPlaySound(U3SoundEffectError1, true);
        return;
    }
    if (!preset) {
        U3RenderPrintMessage(53);
        item = GetKey();
    }

    switch (item) {
        case 'F':
            if (!preset) {
                U3RenderPrintMessage(54);
                value = UInputBigNum(tx, ty);
                U3RenderPrintPascalString("\p\n");
            }
            fromAmount = (Player[rosNum1][32] * 100) + Player[rosNum1][33];
            toAmount = (Player[rosNum2][32] * 100) + Player[rosNum2][33];
            if (value > fromAmount) {
                U3RenderPrintMessage(55);
                U3AudioPlaySound(U3SoundEffectError1, true);
                return;
            }
            if (toAmount + value > 9999) {
                U3RenderPrintMessage(56);
                U3AudioPlaySound(U3SoundEffectError1, true);
                return;
            }
            fromAmount -= value;
            toAmount += value;
            Player[rosNum1][32] = fromAmount / 100;
            Player[rosNum1][33] = fromAmount - (Player[rosNum1][32] * 100);
            Player[rosNum2][32] = toAmount / 100;
            Player[rosNum2][33] = toAmount - (Player[rosNum2][32] * 100);
            if (!preset)
                U3RenderPrintMessage(57);
            break;
        case 'G':
            if (!preset) {
                U3RenderPrintMessage(54);
                value = UInputBigNum(tx, ty);
                U3RenderPrintPascalString("\p\n");
            }
            fromAmount = (Player[rosNum1][35] * 256) + Player[rosNum1][36];
            toAmount = (Player[rosNum2][35] * 256) + Player[rosNum2][36];
            if (value > fromAmount) {
                U3RenderPrintMessage(55);
                U3AudioPlaySound(U3SoundEffectError1, true);
                return;
            }
            if (toAmount + value > 9999) {
                U3RenderPrintMessage(56);
                U3AudioPlaySound(U3SoundEffectError1, true);
                return;
            }
            fromAmount -= value;
            toAmount += value;
            Player[rosNum1][35] = fromAmount / 256;
            Player[rosNum1][36] = fromAmount - (Player[rosNum1][35] * 256);
            Player[rosNum2][35] = toAmount / 256;
            Player[rosNum2][36] = toAmount - (Player[rosNum2][35] * 256);
            if (!preset)
                U3RenderPrintMessage(57);
            break;
        case 'E':
            if (!preset) {
                U3RenderPrintMessage(58);
                what2 = GetKey();
            }
            i = 0;
            if (what2 == 'G')
                i = 37;
            if (what2 == 'K')
                i = 38;
            if (what2 == 'P')
                i = 39;
            if (what2 == 'T')
                i = 15;
            if (i == 0) {
                U3RenderPrintMessage(59);
                U3AudioPlaySound(U3SoundEffectError1, true);
                return;
            }
            if (!preset) {
                U3RenderPrintMessage(60);
                value = UInputNum(tx, ty);
                U3RenderPrintPascalString("\p\n");
            }
            if (value > Player[rosNum1][i]) {
                U3RenderPrintMessage(55);
                U3AudioPlaySound(U3SoundEffectError1, true);
                return;
            }
            if (value + Player[rosNum2][i] > 99) {
                U3RenderPrintMessage(56);
                U3AudioPlaySound(U3SoundEffectError1, true);
                return;
            }
            Player[rosNum1][i] -= value;
            Player[rosNum2][i] += value;
            if (Player[rosNum2][i] > 99)
                Player[rosNum2][i] = 99;
            if (!preset)
                U3RenderPrintMessage(57);
            break;
        case 'W':
            if (!preset) {
                U3RenderPrintMessage(61);
                what2 = GetKey();
            }
            if (what2 < 'B' || what2 > 'P') {
                if (!preset)
                    U3RenderPrintMessage(59);
                U3AudioPlaySound(U3SoundEffectError1, true);
                return;
            }
            what2 -= 'A';
            if (Player[rosNum1][48] == what2 && Player[rosNum1][48 + what2] < 2) {
                if (!preset)
                    U3RenderPrintMessage(63);
                U3AudioPlaySound(U3SoundEffectError1, true);
                return;
            }
            if (Player[rosNum1][48 + what2] == 0) {
                if (!preset)
                    U3RenderPrintMessage(59);
                U3AudioPlaySound(U3SoundEffectError1, true);
                return;
            }
            Player[rosNum1][48 + what2]--;
            Player[rosNum2][48 + what2]++;
            if (Player[rosNum2][48 + what2] > 99)
                Player[rosNum2][48 + what2] = 99;
            if (!preset)
                U3RenderPrintMessage(57);
            break;
        case 'A':
            if (!preset) {
                U3RenderPrintMessage(62);
                what2 = GetKey();
            }
            if (what2 < 'B' || what2 > 'H') {
                U3RenderPrintMessage(59);
                U3AudioPlaySound(U3SoundEffectError1, true);
                return;
            }
            what2 -= 'A';
            if (Player[rosNum1][40] == what2 && Player[rosNum1][40 + what2] < 2) {
                if (!preset)
                    U3RenderPrintMessage(63);
                U3AudioPlaySound(U3SoundEffectError1, true);
                return;
            }
            if (Player[rosNum1][40 + what2] == 0) {
                if (!preset)
                    U3RenderPrintMessage(59);
                U3AudioPlaySound(U3SoundEffectError1, true);
                return;
            }
            Player[rosNum1][40 + what2]--;
            Player[rosNum2][40 + what2]++;
            if (Player[rosNum2][40 + what2] > 99)
                Player[rosNum2][40 + what2] = 99;
            if (!preset)
                U3RenderPrintMessage(57);
            break;
        default:
            if (!preset)
                U3RenderPrintMessage(59);
            U3AudioPlaySound(U3SoundEffectError1, true);
            break;
    }
}

void Ignite(void) { /* $5FF1 */
    short chnum, rosNum;
    U3RenderPrintMessage(64);
    if (Party[3] != 1) {
        NotHere();
        return;
    }
    U3RenderPrintMessage(65);
    chnum = GetChar();
    if (chnum < 1 || chnum > 4)
        return;
    rosNum = Party[6 + chnum];
    if (Player[rosNum][15] < 1) {
        U3RenderPrintMessage(67);
        return;
    }
    Player[rosNum][15]--;
    U3AudioPlaySound(U3SoundEffectTorchIgnite, true);    // was 0xDF
    gTorch = 255;
}

void JoinGold(short chnum) {   // 1-4, 0=ask
    short x, mainRosNum, rosNum, total, gold, transfer;

    if (!chnum) {
        U3RenderPrintMessage(66);
        chnum = GetChar();
    }
    if (chnum < 1 || chnum > 4) {
        U3RenderPrintMessage(41);
        U3AudioPlaySound(U3SoundEffectError1, true);
    } else {
        mainRosNum = Party[6 + chnum];
        total = ((Player[mainRosNum][35]) * 256) + Player[mainRosNum][36];
        for (x = 1; x < 5; x++) {
            if (x != chnum) {
                rosNum = Party[6 + x];
                gold = ((Player[rosNum][35]) * 256) + Player[rosNum][36];
                transfer = gold;
                if (total + transfer > 9999) {
                    transfer = 9999 - total;
                }
                if (transfer > 0) {
                    total += transfer;
                    gold -= transfer;
                    Player[rosNum][35] = gold / 256;
                    Player[rosNum][36] = gold - (Player[rosNum][35] * 256);
                }
            }
        }
        Player[mainRosNum][35] = total / 256;
        Player[mainRosNum][36] = total - (Player[mainRosNum][35] * 256);
    }
}

void Klimb(void) {
    unsigned char keyPressed;

    if (Party[16] != 1 || Party[3] != 0) {
        U3RenderPrintMessage(68);
        What2();
        /*      // This code steps through EVERY 'talk' message.
        short person, perNum;
        PushSosaria();
        for (keyPressed=0; keyPressed<19; keyPressed++)
            {
            U3RenderPrintPascalString("\p\nPlace #"); U3RenderPrintNumberAt(keyPressed, tx, ty); U3RenderPrintCharAt('\n', tx, ty);
            if (keyPressed>18) { LoadUltimaMap(BASERES+keyPressed+3); } else
                             { LoadUltimaMap(BASERES+keyPressed); }
            if (keyPressed>11) // Dungeon
                {
                for (person=1; person<=8; person++)
                    {
                    Speak(person, 23);
                    U3RenderPrintPascalString("\p\n");
                    if (U3PlatformCursorKey(false)=='q') person=32;
                    }
                }
            else
                {
                for (person=0; person<32; person++)
                    {
                    perNum = (Monsters[person+HPMON] & 0x0F);
                    if (Monsters[person]>0 && perNum>0)
                        {
                        Speak(perNum,Monsters[person]/4);
                        if (U3PlatformCursorKey(false)=='q') person=32;
                        }
                    }
                }
            U3RenderPrintPascalString("\p\n\nEnd of place #"); U3RenderPrintNumberAt(keyPressed, tx, ty); U3RenderPrintCharAt('\n', tx, ty);
            if (U3PlatformCursorKey(false)=='q') keyPressed=32;
            }
        PullSosaria();
*/
    } else {
        U3RenderPrintPascalString("\pKreate (Y/N):");
        keyPressed = GetKey();
        if (keyPressed == 'Y') {
            CreateNewMap();
            int i;
            for (i = 0; i < 500; i++) {
                SpawnMonster();
            }
        }
    }
}

void Look(void) {
    short temp, mon;
    U3RenderPrintMessage(69);
    U3PlatformGetDirection(0);
    U3RenderPrintPascalString("\p->");
    temp = (GetXYVal(xs, ys));
    mon = MonsterHere(xs, ys);
    if (mon == 255) {
        PrintTile(temp / 4, false);
        if (temp > 0x30 && temp < 0x7C) {    // try to make it real a la SpawnMonster()
            Boolean madeReal = false;
            int i = 0;
            while (!madeReal && i < 32) {
                if (Monsters[i] == 0) {
                    Monsters[i] = temp;
                    Monsters[i + TILEON] = (temp < 0x40) ? 0x00 : 0x04;
                    Monsters[i + XMON] = xs;
                    Monsters[i + YMON] = ys;
                    Monsters[i + HPMON] = 0x40;    // Wander
                    Monsters[i + VARMON] = 0;
                    madeReal = true;
                }
                ++i;
            }
            if (madeReal)
                PutXYVal(GetXYVal(xs - 1, ys), xs, ys);
        }
    } else {    // plural if not in town or castle.
        PrintMonster(Monsters[mon] / 2, (Party[3] != 2 && Party[3] != 3), Monsters[mon + VARMON]);
    }
    U3RenderPrintPascalString("\p\n");
}

void ModifyOrder(void) {
    short chnum1, chnum2, temp;
    U3RenderPrintMessage(70);
    chnum1 = GetChar();
    if (chnum1 < 1 || chnum1 > 4) {
        U3RenderPrintMessage(71);
    } else {
        U3RenderPrintMessage(72);
        chnum2 = GetChar();
        if (chnum2 < 1 || chnum2 > 4 || chnum1 == chnum2) {
            U3RenderPrintMessage(71);
        } else {
            temp = Party[6 + chnum1];
            Party[6 + chnum1] = Party[6 + chnum2];
            Party[6 + chnum2] = temp;
            SetRect(&myRect, 24 * blkSiz, blkSiz, 39 * blkSiz, 16 * blkSiz);
            ForeColor(blackColor);
            PaintRect(&myRect);
            ForeColor(whiteColor);
            DrawFrame(gCurFrame);
            ShowChars(true);
            U3RenderPrintMessage(73);
        }
    }
}

void NegateTime(short mode, short chnum) {
    short rosNum;
    if (mode == 0) {
        U3RenderPrintMessage(74);
        chnum = GetChar();
        if (chnum < 1 || chnum > 4) {
            U3RenderPrintPascalString("\p\n");
            return;
        }
    }
    rosNum = Party[6 + chnum];
    if (Player[rosNum][39] < 1) {
        U3RenderPrintMessage(67);
        return;
    }
    Player[rosNum][39]--;
    gTimeNegate = 10;
}

/* OtherCommand() see UltimaMisc */

void PeerGem(void) {
    short chnum, rosnum;
    U3RenderPrintMessage(75);
    chnum = GetChar();
    if (chnum < 1 || chnum > 4) {
        U3RenderPrintPascalString("\p\n");
        return;
    }
    rosnum = Party[6 + chnum];
    U3RenderPrintPascalString("\p\n");
    if (Player[rosnum][37] < 1) {
        U3RenderPrintMessage(67);
    } else {
        Player[rosnum][37]--;
        DrawMiniMap();
    }
}

Boolean QuitSave(short mode) {   // 0 = verbose
    Str32 str, str2;

    if (mode == 0)
        U3RenderPrintMessage(76);
    if (Party[3] != 0) {
        if (mode == 0)
            U3RenderPrintMessage(77);
        if (mode == 0)
            U3AudioPlaySound(U3SoundEffectError1, true);
        return FALSE;
    }

    long number = Party[14] * 1000000 + Party[13] * 10000 + Party[12] * 100 + Party[11];
    if (mode == 0) {
        GetPascalStringFromArrayByIndex(str2, CFSTR("Messages"), 77);
        //GetIndString(str2, BASERES+12, 78);
        NumToString(number, str);
        AddString(str, str2);
        U3RenderPrintPascalString(str);
    }
    lastSaveNumberOfMoves = number;
    PutRoster();
    Party[4] = xpos;
    Party[5] = ypos;
    PutParty();
    PutSosaria();
    return TRUE;
}

void ReadyWeapon(short chnum, char weapon) {
    short rosNum, key, typeNum, x;
    Boolean preset = FALSE;

    if (chnum == 0) {
        U3RenderPrintMessage(79);
        chnum = GetChar();
    }
    if (chnum < 1 || chnum > 4) {
        U3RenderPrintMessage(41);
        U3AudioPlaySound(U3SoundEffectError1, true);
        return;
    }
    if (weapon == 0) {
        U3RenderPrintMessage(80);
        key = U3PlatformWaitKeyMouse();
        if (key > 96)
            key -= 32;
        U3RenderPrintCharAt(key, wx, wy);
        weapon = key;
    } else {
        preset = TRUE;
    }
    if (weapon < 'A' || weapon > 'P') {
        U3RenderPrintMessage(81);
        U3AudioPlaySound(U3SoundEffectError1, true);
        return;
    }
    rosNum = Party[6 + chnum];
    typeNum = 0;
    for (x = 0; x < 12; x++) {
        if (Player[rosNum][23] == careerTable[x])
            typeNum = x;
    }
    if (weapon != 'P' && weapon >= wpnUseTable[typeNum]) {
        U3RenderPrintMessage(82);
        U3AudioPlaySound(U3SoundEffectError1, true);
        return;
    }
    weapon -= 'A';
    if ((weapon > 0) && (Player[rosNum][weapon + 48] < 1)) {
        U3RenderPrintMessage(81);
        U3AudioPlaySound(U3SoundEffectError1, true);
    } else {
        Player[rosNum][48] = weapon;
        if (!preset) {
            Str255 outStr, readyStr;
            U3RenderPrintPascalString("\p\n");
            GetPascalStringFromArrayByIndex(outStr, CFSTR("WeaponsArmour"), weapon);
            GetPascalStringFromArrayByIndex(readyStr, CFSTR("Messages"), 82);
            //GetIndString(readyStr, BASERES+12, 83);
            AddString(outStr, readyStr);
            U3RenderPrintPascalString(outStr);
        }
    }
}

void Steal(void) { /* $66C5 */
    short chnum, rosNum, byte;
    U3RenderPrintMessage(84);
    chnum = GetChar();
    if (chnum < 1 || chnum > 4)
        return;
    if (CheckAlive(chnum - 1) == FALSE) {
        Incap();
        return;
    }
    rosNum = Party[6 + chnum];
    U3RenderPrintMessage(85);
    U3PlatformGetDirection(0);
    if (StealDisarmFail(rosNum) == TRUE) {
    fail:
        if ((U3PlatformRandom(0, 255) & 0x03) != 0) {
            U3RenderPrintMessage(86);
            return;
        }
        for (byte = 63; byte >= 0; byte--) {
            if (Monsters[byte] == 0x48)
                Monsters[byte + HPMON] = 0xC0;
        }
        U3RenderPrintMessage(87);
        U3AudioPlaySound(U3SoundEffectOuch, false);    // was 0xFA
        U3AudioPlaySound(U3SoundEffectAlarm, true);    // was 0xE8
        return;
    } else {
        byte = GetXYVal(xs, ys);
        if (byte < 0x94 || byte > 0xE4)
            goto fail;
        xs = xs + dx + dx;
        ys = ys + dy + dy;
        byte = GetXYVal(xs, ys);
        if (byte != 0x24)
            goto fail;
        PutXYVal(0x20, xs, ys);
        GetChest(2, chnum);
    }
}

void Transact(void) {
    short tile, chnum, shopNum, monNum, perNum, rosNum;
    short hpmax, level, storeDir;

    storeDir = 0;
    if (gMouseKey)
        storeDir = gCurMouseDir;
    U3RenderPrintMessageRewrapped(88);
    chnum = GetChar();
    if (chnum < 1 || chnum > 4) {
        return;
    }
    rosNum = Party[6 + chnum];
    if (CheckAlive(chnum - 1) == 0) {
        Incap();
        return;
    }
    U3RenderPrintMessage(85);
    if (storeDir)
        AddMacro(storeDir);
    U3PlatformGetDirection(0);
    monNum = MonsterHere(xs, ys);
    if (monNum > 127) {
        tile = GetXYVal(xs, ys);
        if (tile < 0x94 || tile >= 0xE8) {
            NotHere();
            return;
        }
        xs = xs + dx;
        ys = ys + dy;
        tile = GetXYVal(xs, ys);
        if (tile != 0x40) { /* merchant */
            NotHere();
            return;
        }
        shopNum = (ypos & 0x07);
        gSongCurrent = gSongNext = 6;
        InverseChnum(chnum - 1);
        Shop(shopNum, chnum);
        UnInverseChnum(chnum - 1);
        gSongNext = Party[3];
        return;
    } else {
        if (Monsters[monNum] != 0x4C) {   // is not Lord British
            perNum = (Monsters[monNum + HPMON] & 0x0F);
            if (perNum == 0) {
                if (Party[16] == 1) {
                    U3RenderPrintMessageRewrapped(262);
                    U3AudioSpeakMessages(262, 0, Monsters[monNum] / 4);
                    //Speech(GetLocalizedPascalString("\pExodus is no more!"), Monsters[monNum]/4);
                } else {
                    U3RenderPrintMessage(89);
                    U3AudioSpeakMessages(89, 0, Monsters[monNum] / 4);
                    //Speech(GetLocalizedPascalString("\pGood day!"), Monsters[monNum]/4);
                }
                return;
            }
            Speak(perNum, Monsters[monNum] / 4);
            return;
        }
        gSongCurrent = 8;
        U3AudioUpdateMusic();    // is Lord British
        U3RenderPrintMessage(90);
        level = Player[rosNum][30];
        hpmax = (Player[rosNum][28] * 256) + Player[rosNum][29];
        if ((hpmax % 100) == 50) {   // old 150/250 etc.
            hpmax -= 50;
        }
        hpmax = hpmax / 100;
        if (level < hpmax) {
            if (Party[16] == 1) {
                U3RenderPrintMessageRewrapped(263);
                U3AudioSpeakMessages(90, 263, 19);
                //Speech(GetLocalizedPascalString("\pWelcome, my child. Sosaria thanks you!"),19);
            } else {
                U3RenderPrintMessage(91);
                U3AudioSpeakMessages(90, 91, 19);
                //Speech(GetLocalizedPascalString("\pWelcome, my child. Experience more!"),19);
            }
            return;
        }
        if (hpmax >= 25 && Party[16] == 0) {
            U3RenderPrintMessage(92);
            U3AudioSpeakMessages(90, 92, 19);
            //Speech(GetLocalizedPascalString("\pWelcome, my child. No more!"),19);
            return;
        }
        if (hpmax > 4 && (Player[rosNum][14] & 0x80) == 0) {
            U3RenderPrintMessage(93);
            U3AudioSpeakMessages(90, 93, 19);
            //Speech(GetLocalizedPascalString("\pWelcome, my child. Seek ye, the mark of kings!"),19);
            return;
        }
        hpmax = ((Player[rosNum][28] * 256) + Player[rosNum][29]);
        hpmax += 100;
        if (hpmax > 9950)
            hpmax = 9950;
        Player[rosNum][28] = hpmax / 256;
        Player[rosNum][29] = hpmax - (Player[rosNum][28] * 256);
        U3RenderPrintMessage(94);
        U3AudioSpeakMessages(90, 94, 19);
        //Speech(GetLocalizedPascalString("\pWelcome, my child. Thou art greater!"),19);
        InverseTiles();
        U3AudioPlaySound(U3SoundEffectLBLevelRise, false);    // was 0xEF
        InverseTiles();
        return;
    }
}

void Unlock(void) {
    short temp, rosNum, chnum;
    U3RenderPrintMessage(95);
    Boolean allowDiagonal = !(U3PlatformGetBooleanPreference(U3PreferenceNoDiagonals));
    if (allowDiagonal)
        U3PlatformSetBooleanPreference(U3PreferenceNoDiagonals, true);
    U3PlatformGetDirection(0);
    if (allowDiagonal)
        U3PlatformSetBooleanPreference(U3PreferenceNoDiagonals, false);
    if ((ys != ypos) && (xs == xpos)) {
        NotHere();
    } else {
        temp = GetXYVal(xs, ys);
        if (temp != 184) { /* not a door */
            NotHere();
        } else {
            U3RenderPrintMessage(96);
            chnum = GetChar();
            if (chnum < 1)
                return;
            if (chnum > 4) {
                U3RenderPrintMessage(41);
                U3AudioPlaySound(U3SoundEffectError1, true);
            } else {
                rosNum = Party[6 + chnum];
                if (Player[rosNum][38] < 1) {
                    U3RenderPrintMessage(67);
                } else {
                    Player[rosNum][38]--;
                    U3AudioPlaySound(U3SoundEffectCreak, true);    // was 0xEB
                    // see HideMonsters for visual under-tile choice equivalent
                    int mon = MonsterHere(xs - 1, ys);
                    value = (mon < 255) ? Monsters[mon + TILEON] : GetXYVal(xs - 1, ys);
                    if (value > 0x20)
                        value = 0x04;
                    PutXYVal(value, xs, ys);
                    DrawMap(xpos, ypos);
                }
            }
        }
    }
}

void Volume(void) {
    U3RenderPrintMessage(97);
    if (gSoundIncapable) {
        U3RenderPrintMessage(118);
        return;
    }
    Boolean newSoundOff = !U3PlatformGetBooleanPreference(U3PreferenceSoundDisabled);
    U3PlatformSetBooleanPreference(U3PreferenceSoundDisabled, newSoundOff);
    ReflectPrefs();
    if (!newSoundOff)
        U3RenderPrintMessage(98);
    else
        U3RenderPrintMessage(99);
}

void WearArmour(short chnum, char armour) {
    short rosNum, key, typeNum, x;
    Boolean preset = FALSE;

    if (chnum == 0) {
        U3RenderPrintMessage(100);
        chnum = GetChar();
    }
    if (chnum < 1 || chnum > 4) {
        U3RenderPrintMessage(41);
        U3AudioPlaySound(U3SoundEffectError1, true);
        return;
    }
    if (armour == 0) {
        U3RenderPrintMessage(101);
        key = U3PlatformWaitKeyMouse();
        if (key > 96)
            key -= 32;
        U3RenderPrintCharAt(key, wx, wy);
        armour = key;
    } else {
        preset = TRUE;
    }
    if (armour < 'A' || armour > 'H') {
        U3RenderPrintMessage(81);
        U3AudioPlaySound(U3SoundEffectError1, true);
        return;
    }
    rosNum = Party[6 + chnum];
    typeNum = 0;
    for (x = 0; x < 12; x++) {
        if (Player[rosNum][23] == careerTable[x])
            typeNum = x;
    }
    if (armour != 'H' && armour >= armUseTable[typeNum]) {
        U3RenderPrintMessage(82);
        U3AudioPlaySound(U3SoundEffectError1, true);
        return;
    }
    armour -= 'A';
    if ((armour > 0) && (Player[rosNum][armour + 40] < 1)) {
        U3RenderPrintMessage(81);
        U3AudioPlaySound(U3SoundEffectError1, true);
        return;
    }
    Player[rosNum][40] = armour;
    if (!preset) {
        Str255 outStr, readyStr;
        U3RenderPrintPascalString("\p\n");
        GetPascalStringFromArrayByIndex(outStr, CFSTR("WeaponsArmour"), armour + 16);
        GetPascalStringFromArrayByIndex(readyStr, CFSTR("Messages"), 82);
        //GetIndString(readyStr, BASERES+12, 83);
        AddString(outStr, readyStr);
        U3RenderPrintPascalString(outStr);
    }
}

void Exit(void) {
    Str255 outStr, addStr;

    short tileOn;
    if (Party[1] == 0x7E) {
        GetPascalStringFromArrayByIndex(outStr, CFSTR("Messages"), 101);
        GetPascalStringFromArrayByIndex(addStr, CFSTR("Messages"), 106);
        //GetIndString(outStr, BASERES+12, 102); // X-it
        //GetIndString(addStr, BASERES+12, 107); // <-WHAT?\n
        AddString(outStr, addStr);
        U3RenderPrintPascalString(outStr);
        U3AudioPlaySound(U3SoundEffectError1, true);
    } else {
        tileOn = GetXYVal(xpos, ypos);
        if (tileOn > 4) { /* not water or grass */
            U3RenderPrintMessage(102);    // X-it
            U3RenderPrintPascalString("\p\n");
            U3RenderPrintMessage(108);    // Not here!\n
            U3AudioPlaySound(U3SoundEffectError1, true);
        } else {
            PutXYVal(Party[1] * 2, xpos, ypos);
            if (Party[1] == 0x14)
                U3RenderPrintMessage(17);    // Dismount\n
            else
                U3RenderPrintMessage(103);    // X-it craft
            Party[1] = 0x7E;
        }
    }
}

void Yell(short mode) {
    short chnum, rosNum, oy;
    if (mode == 0) {
        YellStat = 0;
        U3RenderPrintMessage(104);
        OtherCommand(1);
        return;
    }
    if (YellStat != 0) {
        NoEffect();
        return;
    }
    chnum = mode - 1;
    rosNum = Party[6 + chnum];
    if ((Player[rosNum][14] & 0x40) == 0) {
        NoEffect();
        return;
    }
    oy = ypos;
    if (GetXYVal(xpos, ypos - 1) == 236) {
        oy -= 3;
    }
    if (GetXYVal(xpos, ypos + 1) == 232) {
        oy += 3;
    }
    //  if (Party[3]!=0) { NoEffect(); return; }
    if (ypos == oy) {
        NoEffect();
        return;
    }
    /*  if (xpos!=10) { NoEffect(); return; }
    if (ypos!=59 && ypos!=56) { NoEffect(); return; }
    if (ypos == 56)
        {
        ypos=59;
        }
    else
        {
        ypos=56;
        } */
    ypos = oy;
    U3RenderClearTiles();
    ForceUpdateMain();
    U3AudioPlaySound(U3SoundEffectInvocation, false);    // was 0xF4, and  was Whine(0xC0,0x40)
    U3PlatformWaitTicks(60);
    DrawMap(xpos, ypos);
}

void Stats(short mode, short chnum) {
    short rosnum, temp, x;

    if (mode == 0) {
        U3RenderPrintMessage(105);
        chnum = GetChar();
        if (chnum < 1 || chnum > 4) {
            U3RenderPrintPascalString("\p\n");
            return;
        }
    }

    if (!U3PlatformGetBooleanPreference(U3PreferenceClassicAppearance))
        DoStats(chnum);
    else {
        rosnum = Party[6 + chnum];
        for (x = 0; x < 13; x++) {
            temp = Player[rosnum][x];
            if (temp > 0)
                U3RenderPrintCharAt(temp, tx, ty);
        }
        U3RenderPrintPascalString("\p\nSTR...");
        U3RenderPrintNumberPadded(Player[rosnum][18], 2);
        U3RenderPrintPascalString("\p\nDEX...");
        U3RenderPrintNumberPadded(Player[rosnum][19], 2);
        U3RenderPrintPascalString("\p\nINT...");
        U3RenderPrintNumberPadded(Player[rosnum][20], 2);
        U3RenderPrintPascalString("\p\nWIS...");
        U3RenderPrintNumberPadded(Player[rosnum][21], 2);
        temp = StatWait();
        if (temp)
            return;
        U3RenderPrintPascalString("\p\nH.P...");
        temp = Player[rosnum][26] * 256 + Player[rosnum][27];
        U3RenderPrintNumberPadded(temp, 4);
        temp = StatWait();
        if (temp)
            return;
        U3RenderPrintPascalString("\p\nH.M...");
        temp = Player[rosnum][28] * 256 + Player[rosnum][29];
        U3RenderPrintNumberPadded(temp, 4);
        temp = StatWait();
        if (temp)
            return;
        U3RenderPrintPascalString("\p\nGOLD: ");
        temp = Player[rosnum][35] * 256 + Player[rosnum][36];
        U3RenderPrintNumberPadded(temp, 4);
        temp = StatWait();
        if (temp)
            return;
        U3RenderPrintPascalString("\p\nEXP...");
        temp = Player[rosnum][30] * 100 + Player[rosnum][31];
        U3RenderPrintNumberPadded(temp, 4);
        temp = StatWait();
        if (temp)
            return;
        U3RenderPrintPascalString("\p\nGEMS..");
        U3RenderPrintNumberPadded(Player[rosnum][37], 2);
        temp = StatWait();
        if (temp)
            return;
        U3RenderPrintPascalString("\p\nKEYS..");
        U3RenderPrintNumberPadded(Player[rosnum][38], 2);
        temp = StatWait();
        if (temp)
            return;
        U3RenderPrintPascalString("\p\nPOWD..");
        U3RenderPrintNumberPadded(Player[rosnum][39], 2);
        temp = StatWait();
        if (temp)
            return;
        U3RenderPrintPascalString("\p\nTRCH..");
        U3RenderPrintNumberPadded(Player[rosnum][15], 2);
        temp = StatWait();
        if (temp)
            return;
        value = Player[rosnum][14];
        if (value & 0x08) {
            U3RenderPrintPascalString("\p\nCARD OF DEATH");
            temp = StatWait();
            if (temp)
                return;
        }
        if (value & 0x02) {
            U3RenderPrintPascalString("\p\nCARD OF SOL");
            temp = StatWait();
            if (temp)
                return;
        }
        if (value & 0x01) {
            U3RenderPrintPascalString("\p\nCARD OF LOVE");
            temp = StatWait();
            if (temp)
                return;
        }
        if (value & 0x04) {
            U3RenderPrintPascalString("\p\nCARD OF MOONS");
            temp = StatWait();
            if (temp)
                return;
        }
        if (value & 0x10) {
            U3RenderPrintPascalString("\p\nMARK OF FORCE");
            temp = StatWait();
            if (temp)
                return;
        }
        if (value & 0x20) {
            U3RenderPrintPascalString("\p\nMARK OF FIRE");
            temp = StatWait();
            if (temp)
                return;
        }
        if (value & 0x40) {
            U3RenderPrintPascalString("\p\nMARK OF SNAKE");
            temp = StatWait();
            if (temp)
                return;
        }
        if (value & 0x80) {
            U3RenderPrintPascalString("\p\nMARK OF KINGS");
            temp = StatWait();
            if (temp)
                return;
        }
        U3RenderPrintPascalString("\p\nWEAPON:");
        PrintWeapon(Player[rosnum][48]);
        temp = StatWait();
        if (temp)
            return;
        U3RenderPrintPascalString("\p\nARMOUR:");
        PrintArmour(Player[rosnum][40]);
        temp = StatWait();
        if (temp)
            return;
        U3RenderPrintPascalString("\p\n**WEAPONS**\n");
        for (x = 15; x >= 0; x--) {
            if (x == 0) {
                U3RenderPrintPascalString("\p02-Hands-(A)\n**ARMOUR**\n");
            } else {
                if (Player[rosnum][x + 48]) {
                    U3RenderPrintNumberPadded(Player[rosnum][x + 48], 2);
                    wx += 2;
                    U3RenderPrintPascalString("\p-");
                    PrintWeapon(x);
                    U3RenderPrintPascalString("\p-(");
                    U3RenderPrintCharAt(x + 65, tx, ty);
                    wx++;
                    U3RenderPrintPascalString("\p)");
                    temp = StatWait();
                    if (temp)
                        return;
                    U3RenderPrintPascalString("\p\n");
                }
            }
        }
        for (x = 7; x >= 0; x--) {
            if (x == 0) {
                U3RenderPrintPascalString("\p01-Skin-(A)\n");
            } else {
                if (Player[rosnum][x + 40]) {
                    U3RenderPrintNumberPadded(Player[rosnum][x + 40], 2);
                    wx += 2;
                    U3RenderPrintPascalString("\p-");
                    PrintArmour(x);
                    U3RenderPrintPascalString("\p-(");
                    U3RenderPrintCharAt(x + 65, tx, ty);
                    wx++;
                    U3RenderPrintPascalString("\p)");
                    temp = StatWait();
                    if (temp)
                        return;
                    U3RenderPrintPascalString("\p\n");
                }
            }
        }
    }
}

void What(void) { /* $5135 */
    U3RenderPrintMessage(106);
    U3AudioPlaySound(U3SoundEffectError2, true);    // was 0xFE
}

void What2(void) { /* $5279 */
    U3RenderPrintMessage(107);
    U3AudioPlaySound(U3SoundEffectError1, true);
}

void NotHere(void) { /* $5288 */
    U3RenderPrintMessage(108);
    U3AudioPlaySound(U3SoundEffectError1, true);
}

void CheckAllDead(void) { /* $71B4 */
    short button;
    char byte, rosNum, chNum;
    Boolean alive;
    RGBColor color;
    Rect myRect;
    long time;

    alive = FALSE;
    for (byte = 0; byte < Party[2] && byte < 4; byte++) {
        if (CheckAlive(byte) == TRUE)
            alive = TRUE;
    }
    if (!alive) {
        wx = 25;
        wy = 23;
        U3RenderPrintMessage(109);    // ALL PLAYERS OUT!
        if (U3PlatformGetBooleanPreference(U3PreferenceAutoSave)) {
            PutParty();
            PutRoster();
            PutSosaria();
        }
        gMouseState = 666;
        gSongCurrent = gSongNext = 0;
        U3AudioUpdateMusic();
        U3PlatformFlushAllEvents();
        if (!sDefeatScenarioCheck)
            U3PlatformWaitKeyMouse();
        gMouseState = 0;
        CursorUpdate();
        button = sDefeatScenarioCheck ? 1 : Alert(BASERES + 13, nil);
        gMouseState = 666;
        CursorUpdate();
        ForceUpdateMain();
        time = U3PlatformTickCount() + 60;
        while (U3PlatformTickCount() < time) {
            U3PlatformGetKeyMouse(0);
            U3PlatformWaitTicks(1);
        }    // handle update
        if (button == 2) {
            while (!gDone) {
                U3PlatformWaitKeyMouse();
            }
            return;
        } else {
            U3RenderPrintMessage(249);    // Resurrecting!
            time = U3PlatformTickCount() + 300;
            U3AudioPlaySound(U3SoundEffectBigDeath, true);    // was 0xE0
            DrawGamePortToMain(1);
            SetGWorld(gamePort, 0);
            color.red = color.green = color.blue = 32768;
            OpColor(&color);
            ForeColor(redColor);
            SetRect(&myRect, 0, 0, blkSiz * 22, blkSiz * 22);
            PenMode(blend);
            PaintRect(&myRect);
            ForeColor(blackColor);
            PenMode(srcOr);
            SetGWorld(mainPort, nil);
            DrawGamePortToMain(0);
            ForceUpdateMain();
            for (chNum = 0; chNum < Party[2] && chNum < 4; chNum++) {
                rosNum = Party[chNum + 7];
                if (rosNum < 1 || rosNum > 20)
                    continue;
                for (byte = 35; byte < 64; byte++) {
                    Player[rosNum][byte] = 0;
                }
                Player[rosNum][15] = 0;    // no torches too
                if (Player[rosNum][32] < 1)
                    Player[rosNum][32] = 1;    // Some food
                Player[rosNum][36] = 150;      // Gold Pieces
                Player[rosNum][41] = 1;        // Cloth
                Player[rosNum][40] = 1;        //  in use
                Player[rosNum][49] = 1;        // Dagger
                Player[rosNum][48] = 1;        //  in use
                Player[rosNum][17] = 'G';      // Good Health
                Player[rosNum][27] = 100;      // Current Hit Points
                Player[rosNum][26] = 0;        // Current Hit Points
            }
            Party[3] = 0;
            Party[1] = 0x7E;
            xpos = 42;
            ypos = 20;
            Party[4] = xpos;
            Party[5] = ypos;
            zp[0xE3] = xpos;
            zp[0xE4] = ypos;
            PutParty();
            PutRoster();
            ResetSosaria();
            GetMiscStuff(0);
            PutMiscStuff();
            GetSosaria();
            gUpdateWhere = 3;
            ox = xpos - 1;
            oy = ypos - 1;
            gTimeNegate = 0;
            gSongCurrent = gSongNext = 1;
            ShowChars(false);
            ForceUpdateMain();
            //IdleUntil(time);
            while (U3PlatformTickCount() < time) {
                U3AudioUpdateMusic();
            }
            U3RenderPrintPascalString("\p\n\n\n\n\n\n\n\n");
            U3AudioPlaySound(U3SoundEffectBigDeath, true);    // was 0xE0
            gResurrect = TRUE;
        }
    }
}

Boolean CheckAlive(short member) { /* $75BA */
    short rosNum;
    if (member < 0 || member >= 4 || member >= Party[2])
        return FALSE;
    rosNum = Party[member + 7];
    if (rosNum < 1 || rosNum > 20)
        return FALSE;
    if (Player[rosNum][17] == 'G')
        return TRUE;
    if (Player[rosNum][17] == 'P')
        return TRUE;
    return FALSE;
}

void ShowChars(Boolean force) { /* $7338 methinx */
    Boolean somethingChanged = FALSE;
    short i, num, ros;
    Rect rect;
    static short oldStatus[4], oldHP[4], oldMaxHP[4], oldMana[4], oldFood[4], oldExp[4];

    for (i = 0; i < 4; i++) {
        Boolean thisChanged = FALSE;
        rect.left = 24 * blkSiz;
        rect.right = 39 * blkSiz;
        rect.top = i * (blkSiz * 4) + blkSiz;
        rect.bottom = rect.top + (blkSiz * 3);
        ros = Party[7 + i];

        thisChanged = force;
        num = Player[ros][17];
        if (num != oldStatus[i]) {
            oldStatus[i] = num;
            thisChanged = true;
        }
        num = Player[ros][25];
        if (num != oldMana[i]) {
            oldMana[i] = num;
            thisChanged = true;
        }
        num = Player[ros][26] * 256 + Player[ros][27];    // hp
        if (num != oldHP[i]) {
            oldHP[i] = num;
            thisChanged = true;
        }
        num = Player[ros][28] * 256 + Player[ros][29];    // max hp
        if (num != oldMaxHP[i]) {
            oldMaxHP[i] = num;
            thisChanged = true;
        }
        num = Player[ros][30] * 100 + Player[ros][31];    // exp
        if (num != oldExp[i]) {
            oldExp[i] = num;
            thisChanged = true;
        }
        num = Player[ros][32] * 100 + Player[ros][33];    // food
        if (num != oldFood[i]) {
            oldFood[i] = num;
            thisChanged = true;
        }
        if (thisChanged) {
            RenderCharStats(i, &rect);
            somethingChanged = TRUE;
        }
    }
    if (somethingChanged)
        ForceUpdateMain();
}

void WhirlPool(void) { /* $7665 */
    unsigned char byte;
    int swap, newx, newy;

    gWhirlCtr--;
    if (gWhirlCtr > 0)
        return;

    gWhirlCtr = 4;

    if (Party[3] != 0) {
        DrawMap(xpos, ypos);
        return;
    }

    byte = U3PlatformRandom(0, 7);
    if (byte == 0)
        goto Whirl1;
    newx = MapConstrain(WhirlX + WhirlDX);
    newy = MapConstrain(WhirlY + WhirlDY);
    byte = GetXYVal(newx, newy);
    if (byte == 0)
        goto Whirl2;
    if (byte != 0x2C)
        goto Whirl1;
    PutXYVal(0x30, newx, newy);
    swap = WhirlX;
    WhirlX = newx;
    newx = swap;
    swap = WhirlY;
    WhirlY = newy;
    newy = swap;
    PutXYVal(0, newx, newy);
    DrawMap(xpos, ypos);
    U3AudioPlaySound(U3SoundEffectSink, true);    // was 0xF5
    U3RenderPrintMessageRewrapped(110);
    U3RenderPrintPascalString("\p\n");
    U3RenderDrawPrompt();
    return;
Whirl1: /* $76E6 */
    byte = U3PlatformRandom(0, 7);
    WhirlDX = WhirlXtable[byte];
    WhirlDY = WhirlYtable[byte];
    goto Whirl3;
Whirl2: /* $76FB */
    PutXYVal(0x30, newx, newy);
    PutXYVal(0, WhirlX, WhirlY);
    WhirlX = newx;
    WhirlY = newy;
    DrawMap(xpos, ypos);
Whirl3:                                         /* $771A */
    if ((WhirlX == xpos) && (WhirlY == ypos)) { /* Whirlpool hit party */
        GoWhirlPool();
    }
}

void GoWhirlPool(void) { /* 772D */
    short temp;
    long time;
    Party[1] = 24; /* Be one with the whirlpool */
    DrawMap(xpos, ypos);
    if (U3PlatformGetBooleanPreference(U3PreferenceClassicAppearance))
        U3RenderPrintMessage(111);
    else
        U3RenderPrintMessageRewrapped(256);

    short saveCurSong = gSongCurrent;
    gSongCurrent = gSongNext = 0;
    U3AudioUpdateMusic();
    U3AudioPlaySound(U3SoundEffectSink, true);    // was 0xF5
    temp = gUpdateWhere;
    gUpdateWhere = 8;
    CursorUpdate();
    time = U3PlatformTickCount();
    while ((time + 320) > U3PlatformTickCount()) {
        long endTime = U3PlatformTickCount() + 1;
        AnimateTiles();
        //DrawTiles();
        //ForceUpdateMain();
        U3PlatformGetKeyMouse(0);
        IdleUntil(endTime);
    }
    gUpdateWhere = temp;
    if (Party[3] == 0) {
        Party[4] = xpos;
        xs = xpos;
        Party[5] = ypos;
        ys = ypos;
        PutXYVal(0, xpos, ypos);
        WhirlX = 2;
        WhirlY = 0x3E;
        Party[1] = 0x16; /* frigate */
        //gSongCurrent=gSongNext=0;
        PushSosaria();
        if (U3PlatformGetBooleanPreference(U3PreferenceAutoSave)) {
            PutRoster();
            PutParty();
            PutSosaria();
        }
        U3RenderClearTiles();
        Boolean classic = (U3PlatformGetBooleanPreference(U3PreferenceClassicAppearance));
        if (classic)
            U3RenderPrintMessage(112);
        else
            U3RenderPrintMessageRewrapped(257);
        LoadUltimaMap(421);
        temp = gUpdateWhere;
        gUpdateWhere = 0;
        CursorUpdate();
        time = U3PlatformTickCount();
        while ((time + 320) > U3PlatformTickCount()) {
            U3PlatformGetKeyMouse(0);
            AnimateTiles();
            U3PlatformWaitTicks(10);
        }
        gUpdateWhere = temp;
        gSongNext = 0x0B;
        Party[1] = 0x7E;
        xpos = 32;
        ypos = 54;
        Party[3] = 255;
        if (classic)
            U3RenderPrintMessage(113);
        else
            U3RenderPrintMessageRewrapped(258);
        DrawMap(xpos, ypos);
        U3RenderDrawPrompt();
        return;
    } else if (Party[3] == 0xFF) {
        gUpdateWhere = temp;
        //gSongCurrent=gSongNext=0;
        PullSosaria();
        U3RenderPrintMessage(114);
        U3RenderClearTiles();
        U3RenderPrintMessage(115);
        xpos = Party[4];
        ypos = Party[5];
        Party[3] = 0;
        Party[1] = 0x16;
        if (U3PlatformGetBooleanPreference(U3PreferenceAutoSave)) {
            PutRoster();
            PutParty();
        }
        gSongNext = 1;
    } else if (Party[3] < 128) {
        gUpdateWhere = temp;
        temp = 0xFF;
        while (temp != 0) {
            xpos = U3PlatformRandom(0, gCurMapSize - 1);
            ypos = U3PlatformRandom(0, gCurMapSize - 1);
            temp = GetXYVal(xpos, ypos);
        }
        gSongNext = saveCurSong;
        U3AudioUpdateMusic();
    }
    DrawMap(xpos, ypos);
    U3RenderDrawPrompt();
}

void InverseChnum(char which) {
    myRect.top = which * (blkSiz * 4);
    myRect.bottom = myRect.top + blkSiz;
    myRect.left = 31 * blkSiz;
    myRect.right = myRect.left + blkSiz;
    MyInvertRect(&myRect);
}

void UnInverseChnum(char which) {
    Str32 str;

    str[0] = 1;
    str[1] = '1' + which;
    UCenterAt(str, 31, which * 4);
}

void InverseTiles(void) {
    long endTime = U3PlatformTickCount() + 6;

    SetRect(&myRect, blkSiz, blkSiz, blkSiz * 23, blkSiz * 23);
    MyInvertRect(&myRect);
    ForceUpdateMain();
    IdleUntil(endTime);
}

void InverseChar(char which) {
    if (U3PlatformGetBooleanPreference(U3PreferenceClassicAppearance)) {
        myRect.top = which * (blkSiz * 4) + blkSiz;
        myRect.bottom = myRect.top + (blkSiz * 3);
        myRect.left = blkSiz * 24;
        myRect.right = blkSiz * 39;
        MyInvertRect(&myRect);
    } else {
        myRect.top = which * (blkSiz * 4) + blkSiz;
        myRect.bottom = myRect.top + blkSiz;
        myRect.left = blkSiz * 26;
        myRect.right = blkSiz * 39;
        MyInvertRect(&myRect);
    }
}

void ClearChar(char which) {
    myRect.top = which * (blkSiz * 4) + blkSiz;
    myRect.bottom = myRect.top + (blkSiz * 3);
    myRect.left = blkSiz * 24;
    myRect.right = blkSiz * 39;
    ForeColor(blackColor);
    PaintRect(&myRect);
    ForeColor(whiteColor);
}

Boolean ValidDir(unsigned char value) { /* $4702 */
    char byte, byte2;
    Boolean GoodPlace;

    GoodPlace = FALSE;
    if (Party[1] == 0x16) { /* Ship */
        if ((value == 00) || (value == 48))
            GoodPlace = TRUE; /* Water or whirlpool OK */
    } else {
        if (value == 128) { /* Forcefield */
            InverseTiles();
            U3AudioPlaySound(U3SoundEffectForceField, false);    // was 0xFC
            for (byte = 0; byte < 4; byte++) {
                if (!(Player[Party[byte + 7]][14] & 0x10)) {
                    HPSubtract(Party[byte + 7], 99);
                    InverseChar(byte);
                    U3AudioPlaySound(U3SoundEffectHit, false);    // was 0xF7
                    InverseChar(byte);
                    InverseTiles();
                    byte = 5;
                } else {
                    GoodPlace = TRUE;
                }
            }
        }
        if (value == 132) { /* Lava */
            GoodPlace = TRUE;
            U3AudioPlaySound(U3SoundEffectAttack, false);    // was 0xF8
            for (byte = 0; byte < 4; byte++) {
                byte2 = Party[byte + 7];
                if (!(Player[byte2][14] & 0x20)) {
                    if ((Player[byte2][17] == 'G') || (Player[byte2][17] == 'P')) {
                        HPSubtract(Party[byte + 7], 50);
                        InverseChar(byte);
                        U3AudioPlaySound(U3SoundEffectHit, false);    // was 0xF7
                        InverseChar(byte);
                    }
                }
            }
        }
        if ((value == 136) || (value == 248))
            value = 4;
        if ((value < 48) && (value != 0) && (value != 16)) {
            GoodPlace = TRUE;
            if (Party[1] == 20)
                U3AudioPlaySound(U3SoundEffectHorseWalk, true);    // was 0xEC, TRUE
            else
                U3AudioPlaySound(U3SoundEffectStep, true);    // was 0xF6, TRUE
        }
    }
    return GoodPlace;
}

Boolean ValidTrans(char value) {
    if (Party[16]) {
        return TRUE;
    }
    Boolean cango = true;
    if (Party[1] == 0x16 && (U3PlatformGetBooleanPreference(U3PreferenceIncludeWind))) {
        if ((value != WindDir) && (WindDir != 0)) {
            cango = TRUE;
        } else {
            cango = FALSE;
        }
    } else {
        cango = TRUE;
    }
    return cango;
}

void HPAdd(short member, short amount) {
    long hp, maxhp;
    hp = (Player[member][26] * 256 + Player[member][27]);
    maxhp = (Player[member][28] * 256 + Player[member][29]);
    hp += amount;
    if (hp > maxhp)
        hp = maxhp;
    Player[member][26] = hp / 256;
    Player[member][27] = hp - (Player[member][26] * 256);
}

Boolean HPSubtract(short rosNum, short amount) { /* $7181 */
    long originalHP, hp;
    hp = (Player[rosNum][26] * 256 + Player[rosNum][27]);
    originalHP = hp;
    hp -= amount;
    if (hp < 1) {
        Player[rosNum][26] = 0;
        Player[rosNum][27] = 0;
        Player[rosNum][17] = 'D';
        if (U3PlatformGetBooleanPreference(U3PreferenceAutoSave)) {
            Party[4] = xpos;
            Party[5] = ypos;
            PutParty();
            PutRoster();
        }
        if (originalHP > 0) {
            if (Player[rosNum][24] == 'F')
                U3AudioPlaySound(U3SoundEffectDeathFemale, true);    // was 0xE6, TRUE
            else
                U3AudioPlaySound(U3SoundEffectDeathMale, true);    // was 0xE5, TRUE
        }
        return TRUE;
    }
    Player[rosNum][26] = hp / 256;
    Player[rosNum][27] = hp - (Player[rosNum][26] * 256);
    return FALSE;
}

void NoGo(void) {
    U3RenderPrintMessage(116);
    U3AudioPlaySound(U3SoundEffectBump, true);    // was 0xE7
    U3PlatformFlushInputEvents();
}

void DoWind(void) { /* $4C3C */
    unsigned newdir, olddir;
    if (Party[3] == 1)
        return;
    WindCtr--;
    olddir = WindDir;
    if (WindCtr < 0) {
        WindCtr = 32; /* SUPPOSED TO BE 8! */
        newdir = WindDir;
        while (newdir == WindDir) {
            newdir = U3PlatformRandom(0, 8);
            if (newdir > 4)
                newdir -= 4;
        }
        WindDir = newdir;
    }
    if (olddir != WindDir) {
        ShowWind();
    }
}

void ShowWind(void) {
    Str32 str;
    short x;

    if (Party[3] == 1) {
        DngInfo();
        return;
    }
    if (Party[3] == 0x80 || !U3PlatformGetBooleanPreference(U3PreferenceIncludeWind)) {
        for (x = 6; x <= 17; x++) {
            DrawFramePiece(10, x, 23);
        }
    } else {
        DrawFramePiece(12, 6, 23);
        DrawFramePiece(13, 17, 23);
        switch (WindDir) {
            case 0:
                GetPascalStringFromArrayByIndex(str, CFSTR("MoreMessages"), 42);    //GetIndString(str, BASERES+14, 43);
                UCenterAt(str, 7, 23);
                break;
            case 1:
                GetPascalStringFromArrayByIndex(str, CFSTR("MoreMessages"), 43);    //GetIndString(str, BASERES+14, 44);
                UCenterAt(str, 7, 23);
                break;
            case 2:
                GetPascalStringFromArrayByIndex(str, CFSTR("MoreMessages"), 44);    //GetIndString(str, BASERES+14, 45);
                UCenterAt(str, 7, 23);
                break;
            case 3:
                GetPascalStringFromArrayByIndex(str, CFSTR("MoreMessages"), 45);    //GetIndString(str, BASERES+14, 46);
                UCenterAt(str, 7, 23);
                break;
            case 4:
                GetPascalStringFromArrayByIndex(str, CFSTR("MoreMessages"), 46);    //GetIndString(str, BASERES+14, 47);
                UCenterAt(str, 7, 23);
                break;
        }
    }
}

void FullUpdate(void) { /* $4A13 */
    DoWind();
    ScrollThings();
    TwiddleFlags();
    ExodusLights();
    AnimateTiles();
    DrawTiles();
}

void ScrollThings(void) { /* $4A26 */
                          // originally, scroll Forcefield, Moongate every time, and Water and Lava every third time.
                          // now we scroll everything every time, just the water and lava more more slowly.
    int amount = (int)ceilf((float)blkSiz / 16.0);
    if (amount < 1)
        amount = 1;
    ScrollShape(0x40, amount * 2);    // forcefield
    ScrollShape(0x44, amount * 2);    // moongate
    ScrollShape(0x00, amount);        // water
    ScrollShape(0x42, amount);        // lava
}

void TwiddleFlags(void) { /* $4A52-$4AAF */
    twiddleFlag[1]--;
    if (twiddleFlag[1] < 1) {
        twiddleFlag[1] = 3;
        SwapShape(0x0E);
    }    // Castle flag changes every fourth pass
    twiddleFlag[2]--;
    if (twiddleFlag[2] < 1) {
        twiddleFlag[2] = 2;
        SwapShape(0x0C);
    }    // Towne flag changes every third pass
    twiddleFlag[3]--;
    if (twiddleFlag[3] < 1) {
        twiddleFlag[3] = 1;
        SwapShape(0x16);
    }    // Ship flag changes every second pass
}

void AnimateTiles(void) { /* $4B48 */
    short temp;
A4B48:
    animFlag[3]--;
    if (animFlag[3] < 1) {
        animFlag[3] = 5;
        return;
    }
    animFlag[1]--;
    if (animFlag[1] < 0)
        animFlag[1] = 19;             /* from 15 to 19 */
    temp = (animFlag[1] * 2) + 32;    // giving a final range of 0x20-0x46
    if (temp >= 0x44)
        SwapShape(temp - 0x2A);    // also swap Serpent, Man-O-War, and Pirate.
    if (temp == 0x20)
        SwapShape(0x7E);    // also swap party symbol/l/l/l/l / ranger
    if (temp == 62)
        temp = 24;
    if (temp > 62)
        temp += 64; /* added */
    SwapShape(temp);
    if (temp >= 0x2E && temp <= 0x3C) {    // need to swap variants too
        char var = ((temp / 2) - 23) * 2 + 80;
        SwapShape(var++ * 2);
        SwapShape(var * 2);
    }
    goto A4B48;
}

/* ------------------- Run-time game code ends here ------------------------ */

short WaitKeyMouse(void) {
    if (gUpdateWhere == 7)
        SaveWideArea();
    while (!GetKeyMouse(1)) {
    }

    return gKeyPress;
}

short StatWait(void) {
    short RetOrEsc;
    char key;
    RetOrEsc = 0;
    key = U3PlatformWaitKeyMouse();
    if (gDone)
        key = 27;
    if (key == 27) {
        RetOrEsc = 1;
        U3RenderPrintPascalString("\p\n");
    }
    return RetOrEsc;
}

// 0 = return true for keypresses only
// 1 = also return true for mousedowns
// 2 = as 1, but no speed constraints.
Boolean GetKeyMouse(unsigned char mode) {
    Boolean key;
    static long nextHideTick = 0, nextNullTick = 0;
    long nowTick;
    short updateStore, mouseStateStore;

    Boolean constrain = (!U3PlatformGetBooleanPreference(U3PreferenceUnconstrainedSpeed));
    key = FALSE;
    gKeyPress = 0;
    if (Macro[0] != 0) {
        gMouseKey = TRUE;
        gKeyPress = Macro[0];
        DecMacro();
        key = TRUE;
        return key;
    }
    CursorUpdate();
    gMouseKey = FALSE;
    long ticksToWait = (mode == 2) ? 0 : constrain * 5;
    //printf("gkm %d (%d ticks) gUpdateWhere=%d\n", mode, ticksToWait, gUpdateWhere);
    Boolean musicOn = !U3PlatformGetBooleanPreference(U3PreferenceMusicDisabled);
    WaitNextEvent(everyEvent, &gTheEvent, ticksToWait, nil);
    switch (gTheEvent.what) {
        case nullEvent:
            nowTick = U3PlatformTickCount();
            if (nowTick >= nextNullTick) {
                if (gUpdateWhere == 3)
                    FullUpdate();
                else if (gUpdateWhere == 8) {
                    ScrollThings();
                    DrawTiles();
                }
                U3AudioUpdateMusic();
                ShowHideReference();
                CursorUpdate();
                ShowMenuBarIfNecessary();
                if (U3PlatformGetBooleanPreference(U3PreferenceFullScreen) && !gInBackground &&
                    nowTick > nextHideTick) {
                    if (!ShouldSuppressMenuBarHiding())
                        MyHideMenuBar();
                    nextHideTick = nowTick + 90;
                }
                nextNullTick = nowTick + 1;
            }
            break;
        case mouseDown:
            HandleMouseDown();
            if (mode > 0)
                key = TRUE;
            if (mode == 0 && gKeyPress != 0)
                key = TRUE;
            break;
        case activateEvt:
            if (gTheEvent.modifiers & 0x01)
                WakeUp();
            else
                Hibernate();
            break;
        case autoKey:
            gKeyPress = LoWord(gTheEvent.message);
            if ((gTheEvent.modifiers & cmdKey)) {
                if (U3PlatformGetBooleanPreference(U3PreferenceFullScreen))
                    MyShowMenuBar();
                HandleMenuChoice(MenuKey(gKeyPress));
            } else {
                key = TRUE;
            }
            break;
        case keyDown:
            gKeyPress = LoWord(gTheEvent.message);
            if ((unsigned char)gKeyPress == 192 /*&& Party[16]==1*/) {    // 192 = inverted question mark, opt shift slash
                mouseStateStore = gMouseState;
                gMouseState = 0;
                CursorUpdate();
                updateStore = gUpdateWhere;
                gUpdateWhere = 0;
                if (gTheEvent.modifiers & cmdKey)
                    SpawnUntilFull();
                ShowMonsterList();
                U3PlatformWaitKeyMouse();
                gMouseState = mouseStateStore;
                gUpdateWhere = updateStore;
            }
            if ((gTheEvent.modifiers & cmdKey)) {
                if (gKeyPress == '.')
                    gAutoCombat = FALSE;
                if (U3PlatformGetBooleanPreference(U3PreferenceFullScreen))
                    MyShowMenuBar();
                HandleMenuChoice(MenuKey(gKeyPress));
            } else {
                U3PlatformObscureCursor();
                key = TRUE;
            }
            break;
        case updateEvt: HandleUpdate(); break;
        case app4Evt:
            if (LoWord(gTheEvent.message) == 1) {
                WakeUp();
            } else {
                Hibernate();
            }
            break;
        case kHighLevelEvent: AEProcessAppleEvent(&gTheEvent); break;
        default: break;
    }
    if (gDone == TRUE)
        key = TRUE;
    return key;
}

void IdleUntil(long endTime) {
    if (gDone)
        return;
    U3PlatformWaitTicks(endTime - U3PlatformTickCount());
}

void DrawMapPause(void) {
    if (!U3PlatformGetBooleanPreference(U3PreferenceUnconstrainedSpeed))
        U3PlatformWaitTicks(4);
}

void DoSplashScreen(void) {
    U3AudioOpenEffects();
    SetUpFont();
    DisableMenus();
    SetUpGWorlds();
    GetFont();
    GetGraphics();
    GetMiscStuff(100);
    GetRoster();
    GetParty();
    U3AudioSetUpSpeech();
    U3AudioSetUpMusic();
    SetUpDragRect();
    InitMacro();
    InitCursor();
    U3PlatformObscureCursor();
}
