// New Ultima code

#import "UltimaNew.h"

#import "UltimaIncludes.h"
#import "CarbonShunts.h"
#import "CocoaBridge.h"
#import "U3Renderer.h"
#import "U3Platform.h"
#import "UltimaGraphics.h"
#import "UltimaMacIF.h"
#import "UltimaMain.h"
#import "UltimaMisc.h"
#import "UltimaText.h"

CFDictionaryRef savedScreenModeDict;

extern CGrafPtr             mainPort;
extern WindowPtr            gMainWindow;
extern GDHandle             mainDevice;
extern short                chStatsCur, gDepth, blkSiz;
extern unsigned char        gStoreDirect, careerTable[12];
extern unsigned char        Player[21][65], Party[64];
extern UniversalProcPtr     DialogFilterProc;
extern int                  xpos, ypos;
extern char                 gCurWeapons[32], gCurArmours[32];
extern Boolean              gStatsActive, gUnusualSize;
extern short                gCurNumWeapons, gCurNumArmours;

short               prefFRefNum, prefVRefNums;
short               filterOK, filterCancel;
Boolean             fullScreenModeByResolutionChange;
CGrafPtr            helpWorld=nil, buttonPort=nil;

// Local Routines

#define IDTC_DONE           1
#define IDTC_TERMINATE      2
#define IDTC_POPUP          3
Boolean TerminateCharacterDialog(void) {
    Boolean dialogDone, didTerminate = FALSE;
    ControlHandle ctrl;
    ControlHandle terminateButton;
    MenuHandle rosMenu;
    short c, i, temp, rosNum, itemHit, menuEntry[21];
    DialogPtr theDialog;
    GrafPtr curPort;
    Rect myRect;
    Handle myHandle;
    Str255 final, str;

    theDialog = GetNewDialog(BASERES + 23, nil, (WindowPtr)-1);
    LWGetDialogControl(theDialog, IDTC_POPUP, &ctrl);
    LWGetDialogControl(theDialog, IDTC_TERMINATE, &terminateButton);
#if TARGET_CARBON
    rosMenu = GetControlPopupMenuHandle(ctrl);
#else
    rosMenu = GetMenuHandle(1003);
#endif
    LWDisableMenuItem(rosMenu, 1);
    temp = 0;
    for (i = 1; i < 21; i++) {
        if (Player[i][0] > 22) {
            NumToString(i, final);
            final[++final[0]] = '-';
            final[++final[0]] = ' ';
            str[0] = 0;
            c = 0;
            while (Player[i][c] > 22) {
                str[c + 1] = Player[i][c];
                str[0] = ++c;
            }
            BlockMoveData(str + 1, final + final[0] + 1, str[0]);
            final[0] += str[0];
            AppendMenu(rosMenu, final);
            if (Player[i][16] != 0)
                LWDisableMenuItem(rosMenu, temp + 2);
            menuEntry[temp++] = i;
        }
    }
    SetControlMaximum(ctrl, 99);
    GetPort(&curPort);
    LWSetDialogPort(theDialog);
    ShowWindow(GetDialogWindow(theDialog));
    DefineDefaultItem(theDialog, 1);
    HiliteControl(terminateButton, 255);
    dialogDone = FALSE;
    ConfigureFilter(IDTC_DONE, IDTC_DONE);
    while (!dialogDone) {
        ModalDialog((ModalFilterUPP)DialogFilterProc, &itemHit);
        switch (itemHit) {
            case IDTC_DONE: dialogDone = TRUE; break;
            case IDTC_TERMINATE:
                Player[rosNum][0] = 0;
                dialogDone = TRUE;
                didTerminate = TRUE;
                break;
            case IDTC_POPUP:
                GetDialogItem(theDialog, itemHit, &temp, &myHandle, &myRect);
                i = GetControlValue((ControlHandle)myHandle);
                if (i > 1) {
                    HiliteControl(terminateButton, 0);
                    rosNum = menuEntry[i - 2];
                }
                break;
        }
    }
    SetPort(curPort);
    DisposeDialog(theDialog);
    return didTerminate;
}

#define IDFP_FORM 1
#define IDFP_CANCEL 2
#define IDFP_MEM1 3
#define IDFP_MEM2 4
#define IDFP_MEM3 5
#define IDFP_MEM4 6
static Boolean PartySelectionIsValid(const short selection[4]) {
    if (Party[7]) return false;
    Boolean any = false;
    for (int i = 0; i < 4; ++i) {
        short member = selection[i];
        if (!member) continue;
        if (member < 1 || member > 20 || Player[member][0] <= 22 || Player[member][16])
            return false;
        for (int j = 0; j < i; ++j)
            if (selection[j] == member) return false;
        any = true;
    }
    return any;
}

Boolean U3ApplyPartySelection(const short selection[4]) {
    if (!PartySelectionIsValid(selection)) return false;
    memset(Party, 0, sizeof(Party));
    short position = 1;
    for (int i = 0; i < 4; ++i) {
        if (selection[i]) {
            Party[position + 6] = selection[i];
            Player[selection[i]][16] = 255;
            Party[2] = position++;
        }
    }
    Party[1] = 0x7E;
    Party[6] = 255;
    xpos = Party[4] = 42;
    ypos = Party[5] = 20;
    return true;
}

Boolean U3StorePartySelection(const short selection[4]) {
    if (!U3ApplyPartySelection(selection)) return false;
    PutParty();
    PutRoster();
    ResetSosaria();
    GetMiscStuff(0);
    PutMiscStuff();
    return true;
}

Boolean U3PartySelectionSelfTest(void) {
    unsigned char savedPlayers[21][65], savedParty[64];
    int savedX = xpos, savedY = ypos;
    memcpy(savedPlayers, Player, sizeof(Player));
    memcpy(savedParty, Party, sizeof(Party));
    memset(Player, 0, sizeof(Player));
    memset(Party, 0, sizeof(Party));
    Player[1][0] = 'A'; Player[20][0] = 'Z';
    Boolean passed = PartySelectionIsValid((short[4]){1, 0, 20, 0}) &&
        !PartySelectionIsValid((short[4]){0, 0, 0, 0}) &&
        !PartySelectionIsValid((short[4]){1, 1, 0, 0}) &&
        !PartySelectionIsValid((short[4]){21, 0, 0, 0}) &&
        !PartySelectionIsValid((short[4]){-1, 0, 0, 0}) &&
        !PartySelectionIsValid((short[4]){2, 0, 0, 0});
    Player[1][16] = 255;
    passed = passed && !PartySelectionIsValid((short[4]){1, 0, 0, 0});
    Party[7] = 20;
    passed = passed && !PartySelectionIsValid((short[4]){20, 0, 0, 0});
    Party[7] = 0;
    Player[1][16] = 0;
    passed = U3ApplyPartySelection((short[4]){20, 0, 1, 0}) && passed;
    passed = passed && Party[2] == 2 && Party[7] == 20 && Party[8] == 1 &&
        Player[1][16] == 255 && Player[20][16] == 255 &&
        Party[1] == 0x7E && Party[4] == 42 && Party[5] == 20 && Party[3] == 0;
    passed = passed && !U3ApplyPartySelection((short[4]){1, 0, 0, 0}) && Party[7] == 20;
    memcpy(Player, savedPlayers, sizeof(Player));
    memcpy(Party, savedParty, sizeof(Party));
    xpos = savedX; ypos = savedY;
    return passed;
}

Boolean FormPartyDialog(void) {
    ControlHandle ctrl;
    short c, i, temp, itemHit, menuEntry[21], sel[4] = {0, 0, 0, 0};
    Boolean dialogDone, canForm, didForm = FALSE;
    DialogPtr theDialog;
    GrafPtr curPort;
    Rect myRect;
    Handle myHandle;
    MenuHandle rosMenu1, rosMenu2, rosMenu3, rosMenu4;
    Str255 final, str;

    if (Party[7] != 0) {
        U3RenderClearBottom();
        CenterMessage(4, 13);
        CenterMessage(5, 16);
        ShowClickMessage();
        U3PlatformWaitKeyMouse();
        return FALSE;
    }
    if (U3CocoaHasMainSurface()) {
        unsigned char names[20][16] = {{0}};
        Boolean available[20];
        for (int member = 1; member <= 20; ++member) {
            unsigned char length = 0;
            while (length < 13 && Player[member][length] > 22) {
                names[member - 1][length + 1] = Player[member][length];
                ++length;
            }
            names[member - 1][0] = length;
            available[member - 1] = length && !Player[member][16];
        }
        if (!U3CocoaChooseParty(names, available, sel)) return false;
        return U3StorePartySelection(sel);
    }
    theDialog = GetNewDialog(BASERES + 21, nil, (WindowPtr)-1);
#if TARGET_CARBON
    LWGetDialogControl(theDialog, IDFP_MEM1, &ctrl);
    SetControlMaximum(ctrl, 99);
    rosMenu1 = GetControlPopupMenuHandle(ctrl);
    LWGetDialogControl(theDialog, IDFP_MEM2, &ctrl);
    SetControlMaximum(ctrl, 99);
    rosMenu2 = GetControlPopupMenuHandle(ctrl);
    LWGetDialogControl(theDialog, IDFP_MEM3, &ctrl);
    SetControlMaximum(ctrl, 99);
    rosMenu3 = GetControlPopupMenuHandle(ctrl);
    LWGetDialogControl(theDialog, IDFP_MEM4, &ctrl);
    SetControlMaximum(ctrl, 99);
    rosMenu4 = GetControlPopupMenuHandle(ctrl);
#else
    rosMenu1 = rosMenu2 = rosMenu3 = rosMenu4 = GetMenuHandle(1003);
#endif
    LWDisableMenuItem(rosMenu1, 1);
    LWDisableMenuItem(rosMenu2, 1);
    LWDisableMenuItem(rosMenu3, 1);
    LWDisableMenuItem(rosMenu4, 1);
    temp = 0;
    for (i = 1; i < 21; i++) {
        if (Player[i][0] > 22) {
            NumToString(i, final);
            final[++final[0]] = '-';
            final[++final[0]] = ' ';
            str[0] = 0;
            c = 0;
            while (Player[i][c] > 22) {
                str[c + 1] = Player[i][c];
                str[0] = ++c;
            }
            BlockMoveData(str + 1, final + final[0] + 1, str[0]);
            final[0] += str[0];
            AppendMenu(rosMenu1, final);
            if (Player[i][16] != 0)
                LWDisableMenuItem(rosMenu1, temp + 2);
            AppendMenu(rosMenu2, final);
            if (Player[i][16] != 0)
                LWDisableMenuItem(rosMenu2, temp + 2);
            AppendMenu(rosMenu3, final);
            if (Player[i][16] != 0)
                LWDisableMenuItem(rosMenu3, temp + 2);
            AppendMenu(rosMenu4, final);
            if (Player[i][16] != 0)
                LWDisableMenuItem(rosMenu4, temp + 2);
            menuEntry[temp++] = i;
        }
    }

    GetPort(&curPort);
    LWSetDialogPort(theDialog);
    //TextFont(3); TextSize(9);
    ShowWindow(GetDialogWindow(theDialog));
    GetDialogItem(theDialog, 1, &temp, &myHandle, &myRect);
    PenSize(3, 3);
    InsetRect(&myRect, -4, -4);
    FrameRoundRect(&myRect, 16, 16);
    HiliteControl((ControlHandle)myHandle, 255);
    dialogDone = FALSE;
    ConfigureFilter(IDFP_FORM, IDFP_CANCEL);
    while (!dialogDone) {
        ModalDialog((ModalFilterUPP)DialogFilterProc, &itemHit);
        switch (itemHit) {
            case IDFP_FORM:
                didForm = U3StorePartySelection(sel);
                dialogDone = didForm;
                break;
            case IDFP_CANCEL: dialogDone = TRUE; break;
            case IDFP_MEM1:
            case IDFP_MEM2:
            case IDFP_MEM3:
            case IDFP_MEM4:
                GetDialogItem(theDialog, itemHit, &temp, &myHandle, &myRect);
                i = GetControlValue((ControlHandle)myHandle);
                if (i == 1) {
                    i = 0;
                } else {
                    i = menuEntry[i - 2];
                }
                sel[itemHit - IDFP_MEM1] = i;
                canForm = TRUE;
                if (!sel[0] && !sel[1] && !sel[2] && !sel[3])
                    canForm = FALSE;
                for (temp = 0; temp < 4; temp++) {
                    for (i = 0; i < 4; i++) {
                        if (temp != i && sel[temp] != 0 && sel[temp] == sel[i])
                            canForm = FALSE;
                    }
                }
                GetDialogItem(theDialog, IDFP_FORM, &temp, &myHandle, &myRect);
                if (canForm) {
                    HiliteControl((ControlHandle)myHandle, 0);
                } else {
                    HiliteControl((ControlHandle)myHandle, 255);
                }
                break;
        }
    }
    SetPort(curPort);
    DisposeDialog(theDialog);
    return didForm;
}

void GetButtons(void) {
    if (buttonPort)
        return;

    Rect buttonRect;
    SetRect(&buttonRect, 0, 0, 1088, 2400);
    OSErr error = NewGWorld(&buttonPort, 32, &buttonRect, nil, nil, 0);
    if (error == noErr)
        DrawNamedImage(CFSTR("Buttons.png"), buttonPort, &buttonRect);

}

void DisposeButtons(void) {
    DisposeGWorld(buttonPort);
    buttonPort = nil;
}

// all nums for standard size (blkSiz=16).  All buttons are actually 4x this size, however.
short butLeft[16] = {0, 0, 0, 100, 100, 100, 100, 100, 157};
short butRight[16] = {100, 100, 100, 272, 272, 272, 272, 153, 257};
short butTop[16] = {0, 200, 400, 0, 75, 150, 225, 300, 400};
short butBottom[16] = {100, 300, 500, 25, 100, 175, 250, 325, 500};
short butOffsetX[16] = {67, 202, 473, 26, 205, 384, 384, 563, 338};
short butOffsetY[16] = {219, 219, 219, 200, 200, 200, 200, 200, 219};

void U3ButtonBounds(Rect *rect, short butNum) {
    float scale = (float)blkSiz / 16.0f;
    SetRect(rect, butOffsetX[butNum] * scale, butOffsetY[butNum] * scale,
            (butOffsetX[butNum] + butRight[butNum] - butLeft[butNum]) * scale,
            (butOffsetY[butNum] + butBottom[butNum] - butTop[butNum]) * scale);
}

void DrawButton(short butNum, Boolean pushed, Boolean dim) {
    Rect fromRect, toRect;
    short height;

    GetButtons();
    SetButtonRect(&fromRect, butNum);
    height = fromRect.bottom - fromRect.top;
    if (pushed)
        OffsetRect(&fromRect, 0, height);
    if (dim)
        OffsetRect(&fromRect, 0, height * 2);
    toRect = fromRect;
    OffsetRect(&toRect, (butOffsetX[butNum] * 4) - toRect.left, (butOffsetY[butNum] * 4) - toRect.top);
    float mult = (float)blkSiz / 64.0;    // blkSiz normally 16, but buttons are 4x
    toRect.left *= mult;
    toRect.top *= mult;
    toRect.right *= mult;
    toRect.bottom *= mult;
    /*
    if (blkSiz!=16) {
        float mult = (float)blkSiz/16.0;
        toRect.left *=mult; toRect.top *=mult;
        toRect.right *=mult; toRect.bottom *=mult; }
*/
    ForeColor(blackColor);
    BackColor(whiteColor);
    U3ButtonBounds(&toRect, butNum);
    CopyBits(LWPortCopyBits(buttonPort), LWPortCopyBits(mainPort), &fromRect, &toRect, srcCopy, nil);
}

void SetButtonRect(Rect *rect, short butNum) {
    SetRect(rect, butLeft[butNum], butTop[butNum], butRight[butNum], butBottom[butNum]);
    rect->left *= 4;
    rect->top *= 4;
    rect->right *= 4;
    rect->bottom *= 4;
}

Boolean HandleButtonClick(Point point, short butNum) {
    Point mouse;
    Rect fromRect, toRect, pushRect;
    Boolean onceThrough = TRUE, last = FALSE, pushed = FALSE;

    SetButtonRect(&fromRect, butNum);
    toRect = fromRect;
    OffsetRect(&toRect, (butOffsetX[butNum] * 4) - toRect.left, (butOffsetY[butNum] * 4) - toRect.top);

    float mult = (float)blkSiz / 64.0;    // blkSiz normally 16, but buttons are 4x
    toRect.left *= mult;
    toRect.top *= mult;
    toRect.right *= mult;
    toRect.bottom *= mult;
    /*
    if (blkSiz!=16) {
        mult = (float)blkSiz/16.0;
        toRect.left *=mult; toRect.top *=mult;
        toRect.right *=mult; toRect.bottom *=mult; }
*/
    if (!PtInRect(point, &toRect))
        return FALSE;    // not even on button
    pushRect = toRect;
    OffsetRect(&pushRect, 0, pushRect.bottom - pushRect.top);
    while (StillDown() || onceThrough) {
        onceThrough = FALSE;
        GetMouse(&mouse);
        pushed = PtInRect(mouse, &toRect);
        if (last != pushed) {
            last = pushed;
            DrawButton(butNum, pushed, FALSE);
            ForceUpdateMain();
            if (pushed)
                U3PlatformWaitTicks(10);
        }
    }
    DrawButton(butNum, FALSE, FALSE);
    return pushed;
}

// Defaults defined here (where 0 or false isn't the default)
void ValidatePrefs(void) {
    // Healing Threshold
    if (!U3PlatformHasPreference(U3PreferenceHealThreshold))
        U3PlatformSetIntegerPreference(U3PreferenceHealThreshold, 150);
}
void DoAutoHeal(void) {
    short clss, c, maxhp, hp, lowest, whoToHeal = -1, whoToCast = -1;
    Boolean whoToCastHealIsMulti, isMulti, isCler;

    if (!U3PlatformGetBooleanPreference(U3PreferenceNoAutoHeal)) {
        // pick someone to heal (lowest hp as long as need 25 hp or more)
        lowest = U3PlatformGetIntegerPreference(U3PreferenceHealThreshold);
        for (c = 0; c <= 3; c++) {
            hp = Player[Party[7 + c]][26] * 256 + Player[Party[7 + c]][27];
            maxhp = Player[Party[7 + c]][28] * 256 + Player[Party[7 + c]][29];
            if (hp < lowest && hp <= (maxhp - 25)) {
                if (CheckAlive(c)) {
                    lowest = hp;
                    whoToHeal = c;
                }
            }
        }
        if (whoToHeal > -1) {
            for (c = 0; c <= 3; c++) {
                if (CheckAlive(c)) {
                    clss = Player[Party[7 + c]][23];
                    isMulti = (clss == careerTable[8] || clss == careerTable[10]);
                    isCler = (clss == careerTable[1] || clss == careerTable[4] || clss == careerTable[7] || isMulti);
                    if ((isCler || isMulti) && Player[Party[7 + c]][25] >= 10) {
                        whoToCast = c;
                        whoToCastHealIsMulti = isMulti;
                    }
                }
            }
            if (whoToCast > -1) {
                AddMacro('1' + whoToHeal);
                if (whoToCastHealIsMulti)
                    AddMacro('C');
                AddMacro('C');
                AddMacro('1' + whoToCast);
                AddMacro('C');
            }
        }
    }
}

void MyInvertRect(Rect *rect) {
    SetGWorld(mainPort, nil);
    InvertRect(rect);
    ForceUpdateMain();
    /*
    short       error;
    Rect        offRect;
    CGrafPtr    invertWorld;
    GrafPtr     savePort;
    CGrafPtr destPort = (CGrafPtr)GetWindowPort(gMainWindow);
    
    BlockMoveData(rect, &offRect, sizeof(Rect));
    OffsetRect(&offRect, -offRect.left, -offRect.top);
    error = NewGWorld(&invertWorld, 32, &offRect, nil, nil, 0);
    if (error) { InvertRect(rect); } // old fashioned invert
    else {
        GetPort(&savePort);
        CopyBits(LWPortCopyBits(destPort),
                 LWPortCopyBits(invertWorld),
                 rect, &offRect, srcCopy, nil);
        SetGWorld(invertWorld, nil);
        InvertRect(&offRect);
        SetGWorld(mainPort, nil);
        SetPort(savePort);
        CopyBits(LWPortCopyBits(invertWorld),
                 LWPortCopyBits(destPort),
                 &offRect, rect, srcCopy, nil);
        DisposeGWorld(invertWorld); }
*/
}

void MyInvertFrame(Rect *rect) {
    Rect iRect;

    iRect = *rect;
    iRect.bottom = iRect.top + 1;
    InvertRect(&iRect);
    iRect = *rect;
    iRect.top = iRect.bottom - 1;
    InvertRect(&iRect);
    iRect = *rect;
    iRect.right = iRect.left + 1;
    iRect.top++;
    iRect.bottom--;
    InvertRect(&iRect);
    iRect = *rect;
    iRect.left = iRect.right - 1;
    iRect.top++;
    iRect.bottom--;
    InvertRect(&iRect);
}

void HandleStatsClick(Point mouse) {
    Boolean hitSomething = FALSE;
    long longVal;
    short amount, i, num, rosNum;
    float scaler = (float)blkSiz / 16.0;
    Rect rect;

    if (Party[3] == 0x80) {
        gStatsActive = FALSE;
        return;
    }    // not in combat
    rosNum = Party[6 + chStatsCur];
    SetRect(&rect, blkSiz, blkSiz, blkSiz * 23, blkSiz * 23);
    if (PtInRect(mouse, &rect)) {
        // Distribute Food
        SetRect(&rect, 253 * scaler, 323 * scaler, 347 * scaler, 340 * scaler);
        if (PtInRect(mouse, &rect) && WidgetClick(rect, TRUE, FALSE)) {
            hitSomething = TRUE;
            longVal = 0;
            for (i = 1; i <= Party[2]; i++) {
                num = (Player[Party[6 + i]][32] * 100) + Player[Party[6 + i]][33];
                longVal += num;
            }
            for (i = 1; i <= Party[2]; i++) {
                num = longVal;
                if (i < Party[2])
                    num = (longVal / (Party[2] - (i - 1)));
                Player[Party[6 + i]][32] = num / 100;
                Player[Party[6 + i]][33] = num - (Player[Party[6 + i]][32] * 100);
                longVal -= num;
            }
            DrawFancyRecord(FALSE);
            ShowChars(false);
        }
        // Gather Gold
        SetRect(&rect, 253 * scaler, 346 * scaler, 347 * scaler, 362 * scaler);
        if (PtInRect(mouse, &rect) && WidgetClick(rect, TRUE, FALSE)) {
            hitSomething = TRUE;
            JoinGold(chStatsCur);
            DrawFancyRecord(FALSE);
        }
        SetRect(&rect, 131 * scaler, 183 * scaler, 230 * scaler, 195 * scaler);    // Gold
        if (PtInRect(mouse, &rect)) {
            hitSomething = TRUE;
            num = WidgetClick(rect, FALSE, TRUE);                        // no click, just drag
            amount = (Player[rosNum][35] * 256) + Player[rosNum][36];    // amount=gold amt
            if (num >= 1 && num <= 4 && amount) {
                HandEquip(chStatsCur, num, 'G', 0, amount);
                DrawFancyRecord(FALSE);
            }
        }
        SetRect(&rect, 287 * scaler, 129 * scaler, 339 * scaler, 139 * scaler);    // torches
        if (PtInRect(mouse, &rect)) {
            hitSomething = TRUE;
            num = WidgetClick(rect, TRUE, TRUE);
            if (num == 255) {
                if (Party[3] == 1)
                    AddMacro('0' + chStatsCur);
                AddMacro('I');
            } else {
                if (num && Player[rosNum][15]) {
                    HandEquip(chStatsCur, num, 'E', 'T', Player[rosNum][15]);
                    DrawFancyRecord(FALSE);
                }
            }
        }
        SetRect(&rect, 287 * scaler, 143 * scaler, 339 * scaler, 153 * scaler);    // gems
        if (PtInRect(mouse, &rect)) {
            hitSomething = TRUE;
            num = WidgetClick(rect, TRUE, TRUE);
            if (num == 255) {
                AddMacro('0' + chStatsCur);
                AddMacro('P');
            } else {
                if (num && Player[rosNum][37]) {
                    HandEquip(chStatsCur, num, 'E', 'G', Player[rosNum][37]);
                    DrawFancyRecord(FALSE);
                }
            }
        }
        SetRect(&rect, 287 * scaler, 157 * scaler, 339 * scaler, 167 * scaler);    // keys
        if (PtInRect(mouse, &rect)) {
            hitSomething = TRUE;
            num = WidgetClick(rect, TRUE, TRUE);
            if (num == 255) {
                AddMacro('U');
            } else {
                if (num && Player[rosNum][38]) {
                    HandEquip(chStatsCur, num, 'E', 'K', Player[rosNum][38]);
                    DrawFancyRecord(FALSE);
                }
            }
        }
        SetRect(&rect, 287 * scaler, 171 * scaler, 339 * scaler, 181 * scaler);    // powders
        if (PtInRect(mouse, &rect)) {
            hitSomething = TRUE;
            num = WidgetClick(rect, TRUE, TRUE);
            if (num == 255) {
                AddMacro('0' + chStatsCur);
                AddMacro('N');
            } else {
                if (num && Player[rosNum][39]) {
                    HandEquip(chStatsCur, num, 'E', 'P', Player[rosNum][39]);
                    DrawFancyRecord(FALSE);
                }
            }
        }
        for (i = 0; i < gCurNumWeapons; i++) {   // weapons
            SetRect(&rect, 41 * scaler, (223 + (i * 11)) * scaler, 122 * scaler, (234 + (i * 11)) * scaler);
            if (i > 7) {
                OffsetRect(&rect, 100 * scaler, -88 * scaler);
                rect.right -= 6 * scaler;
            }
            if (PtInRect(mouse, &rect)) {
                hitSomething = TRUE;
                num = WidgetClick(rect, TRUE, TRUE);
                if (num == 255) {
                    ReadyWeapon(chStatsCur, gCurWeapons[i]);
                    DrawFancyRecord(FALSE);
                } else {
                    if (num) {
                        HandEquip(chStatsCur, num, 'W', gCurWeapons[i], 0);
                        DrawFancyRecord(FALSE);
                    }
                }
            }
        }
        for (i = 0; i < gCurNumArmours; i++) {   // armours
            SetRect(&rect, 260 * scaler, (223 + (i * 11)) * scaler, 342 * scaler, (234 + (i * 11)) * scaler);
            if (PtInRect(mouse, &rect)) {
                hitSomething = TRUE;
                num = WidgetClick(rect, TRUE, TRUE);
                if (num == 255) {
                    WearArmour(chStatsCur, gCurArmours[i]);
                    DrawFancyRecord(FALSE);
                } else {
                    if (num) {
                        HandEquip(chStatsCur, num, 'A', gCurArmours[i], 0);
                        DrawFancyRecord(FALSE);
                    }
                }
            }
        }
        if (!hitSomething)
            gStatsActive = FALSE;
    } else {
        gStatsActive = FALSE;
    }
}

void ConfigureFilter(short setOK, short setCancel) {
    filterOK = setOK;
    filterCancel = setCancel;
}

pascal Boolean DialogFilter(DialogPtr theDlg, EventRecord *event, short *itemHit) {
    GrafPtr curPort;
    Boolean buttonPressed = FALSE;
    short thePart;
    Rect rect;
    WindowPtr whichWindow;
    Point winPos;

    switch (event->what) {
        case mouseDown:
            thePart = FindWindow(event->where, &whichWindow);
            if (whichWindow == GetDialogWindow(theDlg) && thePart == inDrag) {
                GetPort(&curPort);
                SetPortWindowPort(whichWindow);
                LWGetScreenRect(&rect);
                DragWindow(whichWindow, event->where, &rect);
                LWGetWindowBounds(whichWindow, &rect);
                winPos.h = rect.left;
                winPos.v = rect.top;
                LocalToGlobal(&winPos);
                SetPort(curPort);
                return TRUE;
            }
            return FALSE;
            break;
        case keyDown:
            switch ((event->message) & charCodeMask) {
                case 0x0D:
                case 0x03:
                    *itemHit = filterOK;
                    buttonPressed = TRUE;
                    break;
                case 0x1B:
                    *itemHit = filterCancel;
                    buttonPressed = TRUE;
                    break;
                case '.':
                    if (event->modifiers & cmdKey) {
                        *itemHit = filterCancel;
                        buttonPressed = TRUE;
                        break;
                    }
            }
            if (buttonPressed) {
                DialogItemType theType;
                Handle theHandle;
                GetDialogItem(theDlg, *itemHit, &theType, &theHandle, &rect);
                if (LWIsControlActive((ControlHandle)theHandle)) {
                    HiliteControl((ControlHandle)theHandle, kControlButtonPart);
                    long endTime = U3PlatformTickCount() + 8;
                    while (U3PlatformTickCount() < endTime) {
                    }
                    HiliteControl((ControlHandle)theHandle, 0);
                } else {
                    buttonPressed = FALSE;
                }
            }
            return buttonPressed;
            break;
        case updateEvt:
            //HandleUpdate();
            //return true;
            return FALSE;
            break;
        default: return FALSE; break;
    }
}

#define IDAS_FULLSCREEN 2
#define IDAS_WINDOW 3
//#define IDAS_DONTASK      4
void SetUpDisplayDialog(void) {
    if (U3CocoaUsesNativeUI()) {
        U3PlatformSetBooleanPreference(U3PreferenceFullScreen, false);
        U3PlatformSetBooleanPreference(U3PreferenceDontAskDisplayMode, true);
        return;
    }
    Boolean dialogDone;
    short /*temp,*/ itemHit;
    DialogPtr theDialog;
    GrafPtr curPort;
    //Rect          myRect;
    //Handle            myHandle;

    if (!U3PlatformGetBooleanPreference(U3PreferenceDontAskDisplayMode)) {
        theDialog = GetNewDialog(BASERES + 24, nil, (WindowPtr)-1);
        GetPort(&curPort);
        LWSetDialogPort(theDialog);
        ShowWindow(GetDialogWindow(theDialog));
        dialogDone = FALSE;
        if (U3PlatformGetBooleanPreference(U3PreferenceFullScreen)) {
            DefineDefaultItem(theDialog, IDAS_FULLSCREEN);
            ConfigureFilter(IDAS_FULLSCREEN, IDAS_WINDOW);
        } else {
            DefineDefaultItem(theDialog, IDAS_WINDOW);
            ConfigureFilter(IDAS_WINDOW, IDAS_WINDOW);
        }
        while (!dialogDone) {
            ModalDialog((ModalFilterUPP)DialogFilterProc, &itemHit);
            switch (itemHit) {
                case IDAS_FULLSCREEN:
                    U3PlatformSetBooleanPreference(U3PreferenceFullScreen, true);
                    dialogDone = true;
                    break;
                case IDAS_WINDOW:
                    U3PlatformSetBooleanPreference(U3PreferenceFullScreen, false);
                    dialogDone = true;
                    break;
                    /*case IDAS_DONTASK:
                    GetDialogItem(theDialog, itemHit, &temp, &myHandle, &myRect);
                    SetControlValue((ControlHandle)myHandle, !GetControlValue((ControlHandle)myHandle));
                    break;*/
            }
        }
        //GetDialogItem(theDialog, IDAS_DONTASK, &temp, &myHandle, &myRect);
        //CFBooleanRef dontAsk = (GetControlValue((ControlHandle)myHandle)) ? kCFBooleanTrue : kCFBooleanFalse ;
        U3PlatformSetBooleanPreference(U3PreferenceDontAskDisplayMode, true);
        SetPort(curPort);
        DisposeDialog(theDialog);
    }
    if (U3PlatformGetBooleanPreference(U3PreferenceFullScreen))
        SetUpDisplay();
}

void SetUpDisplay(void) {
    if (!U3PlatformGetBooleanPreference(U3PreferenceNoEducateAboutFullScreen)) {
        ResetCursor();
        int result = EducateAboutFullScreen();
        if (result == 0)
            U3PlatformSetBooleanPreference(U3PreferenceNoEducateAboutFullScreen, true);
    }

    short curx = U3PlatformGetIntegerPreference(U3PreferenceCurrentWindowX);
    short cury = U3PlatformGetIntegerPreference(U3PreferenceCurrentWindowY);
    SetSaveWindowPrefPosn(curx, cury);

    if (!U3PlatformGetBooleanPreference(U3PreferenceFullScreenResolutionChange)) {
        fullScreenModeByResolutionChange = FALSE;
        int screenWidth, screenHeight;
        savedScreenModeDict = CGDisplayCurrentMode(kCGDirectMainDisplay);
        CFNumberRef number = CFDictionaryGetValue(savedScreenModeDict, kCGDisplayWidth);
        CFNumberGetValue(number, kCFNumberIntType, &screenWidth);
        number = CFDictionaryGetValue(savedScreenModeDict, kCGDisplayHeight);
        CFNumberGetValue(number, kCFNumberIntType, &screenHeight);
        screenHeight -= (GetMBarHeight() + 8);
        int blkSizX = (int)truncf((float)screenWidth / 40.0);
        int blkSizY = (int)truncf((float)screenHeight / 24.0);
        blkSiz = (blkSizX < blkSizY) ? blkSizX : blkSizY;
        if (blkSiz < 8)
            blkSiz = 16;
        gUnusualSize = (blkSiz != 16);

        TearDownGWorlds();
        SetUpGWorlds();
        GetGraphics();
        GetPortraits();

        Rect winRect;
        SetRect(&winRect, 0, 0, blkSiz * 40, blkSiz * 24);
        SizeWindow(gMainWindow, winRect.right, winRect.bottom, true);
        InvalWindowRect(gMainWindow, &winRect);
    } else {
        fullScreenModeByResolutionChange = TRUE;
        savedScreenModeDict = CGDisplayCurrentMode(kCGDirectMainDisplay);
        boolean_t isExactMatch = false;
        int bestWidth = 640;
        int bestHeight = 400;
        if (!U3PlatformGetBooleanPreference(U3PreferenceOriginalSize)) {
            bestWidth *= 2;
            bestHeight *= 2;
        }
        CFDictionaryRef bestModeDict =
            CGDisplayBestModeForParameters(kCGDirectMainDisplay, 32, bestWidth, bestHeight, &isExactMatch);
        mainPort = nil;
        mainDevice = nil;
        CGDisplayErr cgErr = CGDisplaySwitchToMode(kCGDirectMainDisplay, bestModeDict);
        if (cgErr != kCGErrorSuccess)
            SysBeep(1);
        GetGWorld(&mainPort, &mainDevice);
    }
    return;
}

void RestoreDisplay(void) {
    if (!fullScreenModeByResolutionChange) {
        AdaptToWindow(false);
    } else {
        mainPort = nil;
        mainDevice = nil;
        CGDisplayErr cgErr = CGDisplaySwitchToMode(kCGDirectMainDisplay, savedScreenModeDict);
        if (cgErr != kCGErrorSuccess) {
            SysBeep(1);
            return;
        }
        GetGWorld(&mainPort, &mainDevice);
    }

    short savx = U3PlatformGetIntegerPreference(U3PreferenceSaveWindowX);
    short savy = U3PlatformGetIntegerPreference(U3PreferenceSaveWindowY);
    MoveWindow(gMainWindow, savx, savy, false);
    ForceOnScreen(gMainWindow);
}

void AdaptToWindow(Boolean forceOnScreen) {
    Boolean doubleSize = !U3PlatformGetBooleanPreference(U3PreferenceOriginalSize);
    int newBlockSize = (doubleSize) ? 32 : 16;
    if (newBlockSize == doubleSize)
        return;

    TearDownGWorlds();
    blkSiz = newBlockSize;
    gUnusualSize = (blkSiz != 16);
    SetUpGWorlds();
    GetGraphics();
    GetPortraits();

    Rect winRect;
    SetRect(&winRect, 0, 0, blkSiz * 40, blkSiz * 24);
    SizeWindow(gMainWindow, winRect.right, winRect.bottom, true);
    BackColor(blackColor);
    EraseRect(&winRect);

    BackColor(whiteColor);
    InvalWindowRect(gMainWindow, &winRect);
    if (forceOnScreen)
        ForceOnScreen(gMainWindow);
}

#define FORCEMARGIN 16
void ForceOnScreen(WindowPtr win) {
    Rect wr, gr, ur, mr;
    short dist;

    LWGetScreenRect(&mr);
    GetWindowDeviceRect(win, &gr);
    GetGlobalWindowRect(win, &wr);

    if (EqualRect(&gr, &mr)) {   // on main screen
        if (wr.top < (gr.top + 35)) {   // must be completely below menu bar
            dist = (gr.top + 35) - wr.top;
            wr.top += dist;
            wr.bottom += dist;
            MoveWindow(win, wr.left, wr.top, false);
        }
    } else {   // on other screen, must be below top
        if (wr.top < gr.top) {   // must be completely below menu bar
            dist = gr.top - wr.top;
            wr.top += dist;
            wr.bottom += dist;
            MoveWindow(win, wr.left, wr.top, false);
        }
    }

    UnionRect(&wr, &gr, &ur);
    if (!EqualRect(&gr, &ur)) {
        if (wr.left > (gr.right - FORCEMARGIN)) {   // too far right
            dist = (wr.left - (gr.right - FORCEMARGIN));
            wr.left -= dist;
            wr.right -= dist;
        }
        if (wr.right < (gr.left + FORCEMARGIN)) {   // too far left
            dist = ((gr.left + FORCEMARGIN) - wr.right);
            wr.left += dist;
            wr.right += dist;
        }
        if (wr.top > (gr.bottom - FORCEMARGIN)) {   // too far down
            dist = (wr.top - (gr.bottom - FORCEMARGIN));
            wr.top -= dist;
            wr.bottom -= dist;
        }
        if (wr.top < (gr.top + FORCEMARGIN)) {   // too far up
            dist = (gr.top + FORCEMARGIN) - wr.top;
            wr.top += dist;
            wr.bottom += dist;
        }
        MoveWindow(win, wr.left, wr.top, false);
    }
}

void GetGlobalWindowRect(WindowPtr win, Rect *rect) {
    Rect myRect;
    GrafPtr savePort;
    Point wp;

    GetPort(&savePort);
    SetPortWindowPort(win);
    LWGetWindowBounds(win, &myRect);
    wp.h = myRect.left;
    wp.v = myRect.top;
    LocalToGlobal(&wp);
    rect->left = wp.h;
    rect->top = wp.v;
    wp.h = myRect.right;
    wp.v = myRect.bottom;
    LocalToGlobal(&wp);
    rect->right = wp.h;
    rect->bottom = wp.v;
    SetPort(savePort);
}

short GetWindowDevice(WindowPtr win) {
    (void)win;
    return 32;
}

void GetWindowDeviceRect(WindowPtr win, Rect *gr) {
    (void)win;
    LWGetScreenRect(gr);
}

Boolean SetUpHelpWorld(void) {
    Rect theRect;
    SetRect(&theRect, 0, 0, 400, 753);
    OSErr err = NewGWorld(&helpWorld, 32, &theRect, nil, nil, 0);
    if (err == noErr)
        return DrawNamedImage(CFSTR("RaceClassInfo.gif"), helpWorld, &theRect);
    return false;
}

void DestroyHelpWorld(void) {
    PixMapHandle pm;

    if (!helpWorld)
        return;
    pm = GetGWorldPixMap(helpWorld);
    UnlockPixels(pm);
    DisposeGWorld(helpWorld);
    helpWorld = nil;
}

void DrawRaceHelp(short race, short x, short y) {
    Rect fromRect, toRect;
    GrafPtr savePort;
    RGBColor saveFore, saveBack;
    PixMapHandle pm;

    if (!helpWorld)
        return;
    pm = GetGWorldPixMap(helpWorld);
    if (!LockPixels(pm)) {
        DestroyHelpWorld();
        SetUpHelpWorld();
    }
    GetPort(&savePort);
    SetRect(&fromRect, 0, 0, 400, 34);
    toRect = fromRect;
    OffsetRect(&fromRect, 0, ((race - 1) * 34));
    OffsetRect(&toRect, x, y);
    LWGetPortForeColor(savePort, &saveFore);
    LWGetPortBackColor(savePort, &saveBack);
    ForeColor(blackColor);
    BackColor(whiteColor);
    CopyBits(LWPortCopyBits(helpWorld), LWPortCopyBits(savePort), &fromRect, &toRect, ditherCopy, nil);
    RGBForeColor(&saveFore);
    RGBBackColor(&saveBack);
    UnlockPixels(pm);
}

void DrawClassHelp(short clss, short x, short y) {
    Rect fromRect, toRect;
    GrafPtr savePort;
    RGBColor saveFore, saveBack;
    PixMapHandle pm;

    if (!helpWorld)
        return;
    pm = GetGWorldPixMap(helpWorld);
    if (!LockPixels(pm)) {
        DestroyHelpWorld();
        SetUpHelpWorld();
    }
    GetPort(&savePort);
    SetRect(&fromRect, 0, 0, 400, 53);
    toRect = fromRect;
    OffsetRect(&fromRect, 0, 170 + ((clss - 1) * 53));
    OffsetRect(&toRect, x, y);
    LWGetPortForeColor(savePort, &saveFore);
    LWGetPortBackColor(savePort, &saveBack);
    ForeColor(blackColor);
    BackColor(whiteColor);
    CopyBits(LWPortCopyBits(helpWorld), LWPortCopyBits(savePort), &fromRect, &toRect, ditherCopy, nil);
    RGBForeColor(&saveFore);
    RGBBackColor(&saveBack);
    UnlockPixels(pm);
}

void RenderCharStats(short ch, const Rect *rect) {   // 0-3
    Rect fromRect, barRect;
    CGrafPtr statWorld;
    GrafPtr curPort;
    short ros, i, num, maxnum;
    OSErr err;
    float scale;
    Str255 str, numStr;
    Str255 levelStr = "\pLevel ";
    RGBColor color;
    static short fontNumber = -1;
    int fontSize = (int)(9.0 * (blkSiz / 16.0));
    int labelOffset = 2 + ((fontSize - 9) / 3);

    Boolean classic = U3PlatformGetBooleanPreference(U3PreferenceClassicAppearance);
    Boolean showPortraits = !classic;
    if (fontNumber == -1)
        GetFNum("\pHelvetica", &fontNumber);
    SetRect(&fromRect, 0, 0, blkSiz * 15, blkSiz * 3);
    err = NewGWorld(&statWorld, 0, &fromRect, nil, nil, 0);
    if (err) {
        BackColor(redColor);
        EraseRect(rect);
        ForeColor(whiteColor);
        TextMode(srcOr);
        MoveTo(rect->left + 5, rect->bottom - 5);
        DrawString("\pOut of Memory");
        return;
    }
    GetPort(&curPort);
    SetGWorld(statWorld, nil);
    BackColor(blackColor);
    EraseRect(&fromRect);
    ros = Party[ch + 7];
    if (Player[ros][0]) {   // character here
        Boolean classic = U3PlatformGetBooleanPreference(U3PreferenceClassicAppearance);
        BackColor(whiteColor);
        // Image or condition initial
        if (!showPortraits)
            U3RenderPrintCharAt(Player[ros][17], 14, 0);
        else
            DrawPortrait(ch, statWorld);
        // Name
        str[0] = 0;
        i = 0;
        while (i < 13 && Player[ros][i] != 0) {
            str[++str[0]] = Player[ros][i++] & 0x7F;
        }
        if (classic)
            U3RenderPrintPascalStringAt(str, 7 - (str[0] / 2) + showPortraits, 0);
        else
            NewPrint(str, (blkSiz * 7.5) - PixelsWideString(str) / 2, 0);
        // Sex/Race/Class
        str[0] = 3;
        str[1] = Player[ros][24];
        str[2] = Player[ros][22];
        str[3] = Player[ros][23];
        U3RenderPrintPascalStringAt(str, 1 + showPortraits, 1);
        if (!classic) {   // draw bars & such
            // Hit Points
            num = (Player[ros][26] * 256) + (Player[ros][27]);
            SetRect(&barRect, blkSiz * 5, blkSiz * 2 + 1, blkSiz * 9.5, blkSiz * 3 - 4);
            maxnum = Player[ros][28] * 256 + Player[ros][29];
            scale = (float)(barRect.right - barRect.left) / (float)maxnum;
            // Paint bar
            color.red = color.green = color.blue = 32767;
            RGBForeColor(&color);
            PaintRect(&barRect);
            barRect.right = barRect.left + (num * scale);
            if (barRect.right > barRect.left + 1) {
                color.red = 65535;
                color.green = color.blue = 12000;
                RGBForeColor(&color);
                PaintRect(&barRect);
                // Highlight line
                Rect markRect;
                color.red = 65535;
                color.green = color.blue = 32768;
                RGBForeColor(&color);
                markRect.left = barRect.left + (blkSiz / 16);
                markRect.top = barRect.top + (blkSiz / 16);
                markRect.right = barRect.right - (blkSiz / 16) - 1;
                markRect.bottom = markRect.top + (blkSiz / 16);
                PaintRect(&markRect);
                // Shadow line
                color.red = 49152;
                color.green = color.blue = 0;
                RGBForeColor(&color);
                markRect.bottom = barRect.bottom;
                markRect.top = markRect.bottom - (blkSiz / 16);
                PaintRect(&markRect);
            }
            if (blkSiz >= 16.0) {
                TextSize(fontSize);
                TextFont(fontNumber);
                TextMode(srcOr);
                TextFace(bold);
                ForeColor(blackColor);
                if (num < 51)
                    ForeColor(whiteColor);
                NumToString(num, str);
                str[++str[0]] = '/';
                NumToString(maxnum, numStr);
                BlockMove(numStr + 1, str + str[0] + 1, numStr[0]);
                str[0] += numStr[0];
                SetRect(&barRect, blkSiz * 5, blkSiz * 2 + 1, blkSiz * 9.5, blkSiz * 3 - 4);
                MoveTo(barRect.left + (barRect.right - barRect.left) / 2 - UThemePascalStringWidth(str, kThemeCurrentPortFont) / 2,
                       barRect.bottom - labelOffset);
                UDrawThemePascalString(str, kThemeCurrentPortFont);
            }
            // Mana
            num = Player[ros][25];
            maxnum = MaxMana(ros);
            SetRect(&barRect, blkSiz * 5, blkSiz + 1, blkSiz * 9.5, blkSiz * 2 - 4);
            scale = (float)(barRect.right - barRect.left) / (float)maxnum;
            // Paint bar
            color.red = color.green = color.blue = 32767;
            RGBForeColor(&color);
            PaintRect(&barRect);
            barRect.right = barRect.left + (num * scale);
            if (maxnum > 0 && (barRect.right > barRect.left + 1)) {
                color.red = 0;
                color.green = color.blue = 49152;
                RGBForeColor(&color);
                PaintRect(&barRect);
                // Highlight line
                Rect markRect;
                color.red = 0;
                color.green = color.blue = 65535;
                RGBForeColor(&color);
                markRect.left = barRect.left + (blkSiz / 16);
                markRect.top = barRect.top + (blkSiz / 16);
                markRect.right = barRect.right - (blkSiz / 16) - 1;
                markRect.bottom = markRect.top + (blkSiz / 16);
                PaintRect(&markRect);
                // Shadow line
                color.red = 0;
                color.green = color.blue = 28000;
                RGBForeColor(&color);
                markRect.bottom = barRect.bottom;
                markRect.top = markRect.bottom - (blkSiz / 16);
                PaintRect(&markRect);
            }
            if (blkSiz >= 16.0) {
                TextSize(fontSize);
                TextFont(fontNumber);
                TextMode(srcOr);
                TextFace(bold);
                ForeColor(blackColor);
                MoveTo(barRect.left + 2, barRect.bottom - 2);
                if (maxnum < 1) {
                    TextFace(italic);
                    GetPascalStringFromArrayByIndex(str, CFSTR("MoreMessages"), 74);
                }    //GetIndString(str, BASERES+14, 75); }
                else {
                    NumToString(num, str);
                    str[++str[0]] = '/';
                    NumToString(maxnum, numStr);
                    BlockMove(numStr + 1, str + str[0] + 1, numStr[0]);
                    str[0] += numStr[0];
                }
                SetRect(&barRect, blkSiz * 5, blkSiz + 1, blkSiz * 9.5, blkSiz * 2 - 4);
                MoveTo(barRect.left + (barRect.right - barRect.left) / 2 - UThemePascalStringWidth(str, kThemeCurrentPortFont) / 2,
                       barRect.bottom - labelOffset);
                UDrawThemePascalString(str, kThemeCurrentPortFont);
            }
            // Level / experience
            num = (unsigned char)Player[ros][31];
            maxnum = 100;
            SetRect(&barRect, blkSiz * 10, blkSiz + 1, blkSiz * 14.5, blkSiz * 2 - 4);
            scale = (float)(barRect.right - barRect.left) / (float)maxnum;
            // Paint bar
            color.red = color.green = 32768;
            color.blue = 9000;
            RGBForeColor(&color);
            PaintRect(&barRect);
            barRect.right = barRect.left + (num * scale);
            if (barRect.right > barRect.left + 1) {
                color.red = color.green = 49152;
                color.blue = 12000;
                RGBForeColor(&color);
                PaintRect(&barRect);
                // Highlight line
                color.red = color.green = 65535;
                color.blue = 32000;
                RGBForeColor(&color);
                Rect markRect;
                markRect.left = barRect.left + (blkSiz / 16);
                markRect.top = barRect.top + (blkSiz / 16);
                markRect.right = barRect.right - (blkSiz / 16) - 1;
                markRect.bottom = markRect.top + (blkSiz / 16);
                PaintRect(&markRect);
                // Shadow line
                color.red = color.green = 24576;
                color.blue = 6000;
                RGBForeColor(&color);
                markRect.bottom = barRect.bottom;
                markRect.top = markRect.bottom - (blkSiz / 16);
                PaintRect(&markRect);
            }
            if (blkSiz >= 16.0) {
                TextSize(fontSize);
                TextFont(fontNumber);
                TextMode(srcOr);
                TextFace(bold);
                ForeColor(blackColor);
                MoveTo(barRect.left + 2, barRect.bottom - 2);
                BlockMove(levelStr, str, levelStr[0] + 1);
                NumToString(Player[ros][30] + 1, numStr);
                BlockMove(numStr + 1, str + str[0] + 1, numStr[0]);
                str[0] += numStr[0];
                SetRect(&barRect, blkSiz * 10, blkSiz + 1, blkSiz * 14.5, blkSiz * 2 - 4);
                MoveTo(barRect.left + (barRect.right - barRect.left) / 2 - UThemePascalStringWidth(str, kThemeCurrentPortFont) / 2,
                       barRect.bottom - labelOffset);
                UDrawThemePascalString(str, kThemeCurrentPortFont);
            }

            // Food
            num = Player[ros][32] * 100 + Player[ros][33];
            if (num > 150)
                ForeColor(whiteColor);
            else {
                scale = (num - 50) * 655;
                if (scale < 0)
                    scale = 0;
                color.red = 65535;
                color.green = color.blue = scale;
                RGBForeColor(&color);
            }
            MoveTo(blkSiz * 10, blkSiz * 3 - (labelOffset + 3));
            Str255 outStr = "\pFood: ";
            Str255 numStr;
            NumToString(num, numStr);
            BlockMoveData(numStr + 1, outStr + outStr[0] + 1, numStr[0]);
            outStr[0] += numStr[0];
            TextSize(fontSize + 2);
            UDrawThemePascalString(outStr, kThemeCurrentPortFont);    // kThemeSmallSystemFont
        } else                                                        // the old fashioned way
        {
            // Hit Points
            num = (Player[ros][26] * 256) + (Player[ros][27]);
            str[0] = 2;
            str[1] = 'H';
            str[2] = ':';
            NumToString(num, numStr);
            if (num < 10)
                str[++str[0]] = '0';
            if (num < 100)
                str[++str[0]] = '0';
            if (num < 1000)
                str[++str[0]] = '0';
            BlockMoveData(numStr + 1, str + str[0] + 1, numStr[0]);
            str[0] += numStr[0];
            U3RenderPrintPascalStringAt(str, 1 + showPortraits, 2);
            // Mana
            num = Player[ros][25];
            str[0] = 2;
            str[1] = 'M';
            str[2] = ':';
            NumToString(num, numStr);
            if (num < 10)
                str[++str[0]] = '0';
            BlockMoveData(numStr + 1, str + str[0] + 1, numStr[0]);
            str[0] += numStr[0];
            U3RenderPrintPascalStringAt(str, 5 + showPortraits, 1);
            // Level
            num = Player[ros][30] + 1;
            str[0] = 2;
            str[1] = 'L';
            str[2] = ':';
            NumToString(num, numStr);
            if (num < 10)
                str[++str[0]] = '0';
            BlockMoveData(numStr + 1, str + str[0] + 1, numStr[0]);
            str[0] += numStr[0];
            U3RenderPrintPascalStringAt(str, 10 + showPortraits, 1);
            // Food
            num = Player[ros][32] * 100 + Player[ros][33];
            str[0] = 2;
            str[1] = 'F';
            str[2] = ':';
            NumToString(num, numStr);
            if (num < 10)
                str[++str[0]] = '0';
            if (num < 100)
                str[++str[0]] = '0';
            if (num < 1000)
                str[++str[0]] = '0';
            BlockMoveData(numStr + 1, str + str[0] + 1, numStr[0]);
            str[0] += numStr[0];
            U3RenderPrintPascalStringAt(str, 8 + showPortraits, 2);
        }
    }
    SetPort(curPort);
    ForeColor(blackColor);
    BackColor(whiteColor);
    CopyBits(LWPortCopyBits(statWorld), LWPortCopyBits(GetWindowPort(gMainWindow)),    //mainPort
             &fromRect, rect, srcCopy, nil);
    DisposeGWorld(statWorld);
}
