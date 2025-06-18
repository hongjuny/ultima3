#import "CarbonShunts.h"
// Text & text input routines

#import "UltimaText.h"
#import "UltimaIncludes.h"

#import "CocoaBridge.h"
#import "UltimaGraphics.h"
#import "UltimaMacIF.h"
#import "UltimaMain.h"
#import "UltimaMisc.h"
#import "UltimaSound.h"

extern RgnHandle        UpdateRgn;
extern int              wx, wy, tx, ty;
extern Boolean          gDone, gHasOddTextPort;
extern int              xpos, ypos;
extern short            gUpdateWhere, gMouseState;
extern unsigned char    Talk[256], gCurFrame;
extern CGrafPtr         textPort, textOddPort, mainPort, updatePort;
extern char             gKeyPress;
extern GDHandle         mainDevice;
extern Str255           gString;
extern short            blkSiz;

bool IsNewline(unsigned char ch);

void ShowClickMessage(void) {
    ForeColor(whiteColor);
    BackColor(blackColor);
    TextFont(3);
    TextSize(9);
    MoveTo(blkSiz * 1.25, blkSiz * 22.75);
    GetPascalStringFromArrayByIndex(gString, CFSTR("MoreMessages"), 94);    //GetIndString(gString, BASERES+14, 95);
    UDrawThemePascalString(gString, kThemeCurrentPortFont);
    ForeColor(blackColor);
}

void ClearUpdatePort(void) {
    Rect myRect;

    SetGWorld(updatePort, nil);
    SetRect(&myRect, blkSiz * 24, blkSiz * 17, blkSiz * 40, blkSiz * 24);
    ForeColor(blackColor);
    PaintRect(&myRect);
    SetGWorld(mainPort, nil);
}

void UTextScroll(void) {
    Rect myRect;

    DrawFramePiece(9, 23, 23);
    Boolean classic = CFPreferencesGetAppBooleanValue(U3PrefClassicAppearance, kCFPreferencesCurrentApplication, NULL);
    Boolean constrained = !CFPreferencesGetAppBooleanValue(U3PrefSpeedUnconstrain, kCFPreferencesCurrentApplication, NULL);
    unsigned char scrollAmount = (constrained && !classic) ? blkSiz / 4 : blkSiz;
    unsigned char i;
    for (i = 0; i < blkSiz; i += scrollAmount) {
        SetGWorld(updatePort, nil);
        SetRect(&myRect, blkSiz * 24, blkSiz * 17, blkSiz * 40, blkSiz * 24);
        ForeColor(whiteColor);
        BackColor(blackColor);
        ScrollRect(&myRect, 0, -scrollAmount, UpdateRgn);
        SetGWorld(mainPort, nil);
        TextScrollAreaUpdate();
        ForceUpdateMain();
        if (constrained)
            ThreadSleepTicks(1);
    }
    ForeColor(blackColor);
    BackColor(whiteColor);
}

OSStatus UDrawThemePascalString(ConstStr255Param inPString, ThemeFontID inFontID) {
    OSStatus error = paramErr;

    if (inPString != NULL) {
        CFStringRef cfstring = CFStringCreateWithPascalString(kCFAllocatorDefault, inPString, CFStringGetSystemEncoding());
        // or kCFStringEncodingMacRoman?
        if (cfstring) {
            // measure so we can figure out the bounding box based
            // on the current pen and text metrics

            Point dimensions = {0, 0};    // will have height and width
            SInt16 baseline;              // will be negative
            Point pen;
            Rect bounds;

            GetThemeTextDimensions(cfstring, inFontID, kThemeStateActive, false, &dimensions, &baseline);

            GetPen(&pen);

            bounds.left = pen.h;
            bounds.bottom = pen.v - baseline;    // baseline is negative value
            bounds.right = bounds.left + dimensions.h + 1;
            bounds.top = bounds.bottom - dimensions.v;

            error = DrawThemeTextBox(cfstring, inFontID, kThemeStateActive, false, &bounds, teFlushDefault, NULL);

            CFRelease(cfstring);
        }
    }

    return error;
}

short UThemePascalStringWidth(ConstStr255Param inPString, ThemeFontID inFontID) {
    if (inPString != NULL) {
        CFStringRef cfstring = CFStringCreateWithPascalString(kCFAllocatorDefault, inPString, CFStringGetSystemEncoding());
        // or kCFStringEncodingMacRoman?
        if (cfstring) {
            // measure so we can figure out the bounding box based
            // on the current pen and text metrics

            Point dimensions = {0, 0};    // will have height and width
            SInt16 baseline;              // will be negative
            short result;

            GetThemeTextDimensions(cfstring, inFontID, kThemeStateActive, false, &dimensions, &baseline);

            result = dimensions.h + 1;
            CFRelease(cfstring);
            return result;
        }
    }

    return 0;
}

void UPrintWinEncode(Str255 gString) {
    short low, high, byte;
    byte = 1;
    while (byte <= gString[0]) {
        low = gString[byte] & 0x0F;
        high = (gString[byte] >> 4) & 0x0F;
        gString[byte] = (low << 4) + high;
        byte++;
    }
    UPrintWin(gString);
}

void UPrintMessage(short msgNum) {
    GetPascalStringFromArrayByIndex(gString, CFSTR("Messages"), msgNum - 1);    //GetIndString(gString, BASERES+12, msgNum);
    UPrintWin(gString);
}

void UPrintMessageRewrapped(short msgNum) {
    GetPascalStringFromArrayByIndex(gString, CFSTR("Messages"), msgNum - 1);    //GetIndString(gString, BASERES+12, msgNum);
    if (!CFPreferencesGetAppBooleanValue(U3PrefClassicAppearance, kCFPreferencesCurrentApplication, NULL))
        RewrapString(gString, false);
    UPrintWin(gString);
}

void UPrintWin(Str255 gString) {
    if (gDone)
        return;

    tx = wx;
    ty = wy;
    UPrint(gString, tx, ty);
    wx = tx;
    wy = ty;
}

void UIncTx(void) {
    tx++;
}

void UIncTy(void) {
    ty++;
    if (ty > 23) {
        ty = 23;
        UTextScroll();
    }
}
/*
void DebugPrint(long num, short x, short y)
{
    Str255      dString;
 
    NumToString(num, dString);
    MoveTo(x*blkSiz, y*blkSiz+(blkSiz-2));
    ForeColor(whiteColor);
    BackColor(blackColor);
    DrawString(dString);
    ForeColor(blackColor);
    BackColor(whiteColor);
}
*/
void UPrintNum(short number, short x, short y) {
    Str255 gString;
    NumToString(number, gString);
    UPrint(gString, x, y);
}

void UPrintNumPad(short number, short pad) {
    char offset;
    Str255 gString;
    NumToString(number, gString);
    while (gString[0] < pad) {
        for (offset = gString[0]; offset > 0; offset--) {
            gString[offset + 1] = gString[offset];
        }
        gString[1] = '0';
        gString[0]++;
    }
    UPrint(gString, tx, ty);
}

void UPrintChar(char ch, short x, short y) {
    Str255 gString;
    gString[0] = 1;
    gString[1] = ch;
    UPrint(gString, x, y);
}

void CenterMessage(short which, short y) {
    GetPascalStringFromArrayByIndex(gString, CFSTR("MoreMessages"), which - 1);    //GetIndString(gString, BASERES+14, which);
    UCenterAt(gString, 20 - (gString[0] / 2), y);
    /*  if (prefs.modernAppearance)
        {
        NewPrint(gString, 320 - PixelsWideString(gString)/2, y*blkSiz);
        }
    else
        {
        UPrint(gString, 20-(gString[0]/2), y);
        }*/
}

void Message(short which, short x, short y) {
    GetPascalStringFromArrayByIndex(gString, CFSTR("MoreMessages"), which - 1);    //GetIndString(gString, BASERES+14, which);
    UPrint(gString, x, y);
}

short PixelsWideString(Str255 gString) {
    if (!CFPreferencesGetAppBooleanValue(U3PrefClassicAppearance, kCFPreferencesCurrentApplication, NULL)) {
        SetNewFont(false);
        return UThemePascalStringWidth(gString, kThemeCurrentPortFont);
    } else
        return (gString[0] * blkSiz);
}

void SetNewFont(Boolean force) {
    Str255 fontName;
    static short sFontNum = -1;

    if (force)
        sFontNum = -1;

    if (sFontNum == -1) {
        CFStringRef fontNameRef = CFPreferencesCopyAppValue(U3PrefGameFont, kCFPreferencesCurrentApplication);
        if (fontNameRef) {
            ConstStringPtr fontStr = CFStringGetPascalStringPtr(fontNameRef, kCFStringEncodingMacRoman);
            if (!fontStr) {
                CFStringGetPascalString(fontNameRef, fontName, 255, kCFStringEncodingMacRoman);
                fontStr = fontName;
            }
            if (fontStr)
                GetFNum(fontStr, &sFontNum);
            CFRelease(fontNameRef);
        }

        if (sFontNum >= -1 && sFontNum < 1) {   // try default if not available
            GetIndString(fontName, BASERES + 11, 7);
            GetFNum(fontName, &sFontNum);
        }
        if (sFontNum >= -1 && sFontNum < 1) {   // try Arial Black if not available
            GetIndString(fontName, BASERES + 11, 8);
            GetFNum(fontName, &sFontNum);
        }
        if (sFontNum >= -1 && sFontNum < 1) {   // try Helvetica if not available
            GetIndString(fontName, BASERES + 11, 9);
            GetFNum(fontName, &sFontNum);
        }
        if (sFontNum >= -1 && sFontNum < 1) {
            sFontNum = 0;
        }
    }
    TextFont(sFontNum);
    TextFace(0);
    TextSize(blkSiz);
    ForeColor(whiteColor);
    BackColor(blackColor);
}

void UCenterAt(Str255 gString, short x, short y) {
    short width;
    Rect rect;

    if (CFPreferencesGetAppBooleanValue(U3PrefClassicAppearance, kCFPreferencesCurrentApplication, NULL)) {
        UPrint(gString, x, y);
        return;
    }
    width = gString[0] * blkSiz;
    SetRect(&rect, x * blkSiz, y * blkSiz, x * blkSiz + width, (y + 1) * blkSiz);
    BackColor(blackColor);
    EraseRect(&rect);
    NewPrint(gString, rect.left + (rect.right - rect.left) / 2 - (PixelsWideString(gString) / 2), rect.top);
}

void NewPrint(Str255 gString, short x, short y) {
    CGrafPtr printPort;
    GrafPtr curPort;
    Rect fromRect, toRect;
    short width;
    Boolean inScrollArea;
    OSErr err;

    MoveTo(x + PixelsWideString(gString), y);
    if (gString[0] < 1)
        return;
    inScrollArea = (x > (23 * blkSiz) && y > (15 * blkSiz) && gCurFrame == 1);
    width = PixelsWideString(gString);
    SetRect(&fromRect, 0, 0, width, blkSiz);
    BlockMove(&fromRect, &toRect, sizeof(Rect));
    OffsetRect(&toRect, x, y);
    err = NewGWorld(&printPort, 32, &fromRect, nil, nil, 0);
    GetPort(&curPort);
    if (!err)
        SetGWorld(printPort, nil);
    else
        BlockMove(&toRect, &fromRect, sizeof(Rect));
    SetNewFont(false);
    EraseRect(&fromRect);
    MoveTo(fromRect.left, fromRect.bottom - ceilf((float)blkSiz * 0.1875));
    ForeColor(whiteColor);
    BackColor(blackColor);
    UDrawThemePascalString(gString, kThemeCurrentPortFont);
    TextFace(0);
    SetPort(curPort);
    ForeColor(blackColor);
    BackColor(whiteColor);
    if (!err) {
        if (inScrollArea)
            CopyBits(LWPortCopyBits(printPort), LWPortCopyBits(updatePort), &fromRect, &toRect, srcCopy, nil);
        else
            CopyBits(LWPortCopyBits(printPort), LWPortCopyBits((CGrafPtr)curPort), &fromRect, &toRect, srcCopy, nil);
        DisposeGWorld(printPort);
    }
    tx += (width / blkSiz) + 1;    // gross but it works
}

void UPrint(Str255 gString, char x, char y) {
    Str255 str;
    char length, pos;
    Rect FromRect, ToRect;
    Boolean inScrollArea;
    GrafPtr curPort;

    GetPort(&curPort);
    inScrollArea = (x > 23 && y > 15 && gCurFrame == 1);
    tx = x;
    ty = y;
    if (!CFPreferencesGetAppBooleanValue(U3PrefClassicAppearance, kCFPreferencesCurrentApplication, NULL)) {
        // step thru & deal with CRs!
        str[0] = 0;
        pos = 1;
        while (pos <= gString[0]) {
            if (IsNewline(gString[pos])) {
                if (str[0]) {
                    NewPrint(str, tx * blkSiz, ty * blkSiz);
                }
                str[0] = 0;
                UIncTy();
                tx = 24;
            } else {
                str[++str[0]] = gString[pos] & 0x7F;
            }
            pos++;
        }
        if (str[0]) {
            NewPrint(str, tx * blkSiz, ty * blkSiz);
        }
        if (inScrollArea) {
            TextScrollAreaUpdate();
        }
        ForceUpdateMain();
        return;
    }
    FromRect.top = 0;
    FromRect.bottom = blkSiz;
    ForeColor(blackColor);
    BackColor(whiteColor);
    length = gString[0];
    pos = 1;
    while (length > 0) {
        if (IsNewline(gString[pos])) {
            UIncTy();
            tx = 24;
        } else {
            if (gString[pos] >= ' ') {
                CGrafPtr theTextPort = textPort;
                if (gHasOddTextPort && tx % 2) {
                    theTextPort = textOddPort;
                }
                ToRect.left = tx * blkSiz;
                ToRect.top = ty * blkSiz;
                ToRect.right = ToRect.left + blkSiz;
                ToRect.bottom = ToRect.top + blkSiz;
                FromRect.left = (gString[pos] - 0x20) * blkSiz;
                FromRect.right = FromRect.left + blkSiz;
                if (inScrollArea) {
                    CopyBits(LWPortCopyBits(theTextPort), LWPortCopyBits(updatePort), &FromRect, &ToRect, srcCopy, nil);
                } else {
                    CopyBits(LWPortCopyBits(theTextPort), LWPortCopyBits((CGrafPtr)curPort), &FromRect, &ToRect, srcCopy, nil);
                }
                UIncTx();
            }
        }
        pos++;
        length--;
    }
    if (inScrollArea) {
        TextScrollAreaUpdate();
    }
    ForceUpdateMain();
    MoveTo(tx * blkSiz, ty * blkSiz);
}

void UPrint2(Str255 gString, float y) {
    short x;
    char length, pos;
    tx = 2;
    ty = y;
    length = gString[0];
    x = (blkSiz * 12) - (StringWidth(gString) / 2);
    MoveTo(x, y * blkSiz + (blkSiz - 2));
    BackColor(blackColor);
    DrawText(gString, 1, length);
    for (pos = 1; pos <= length; pos++) {
        UIncTx();
    }
}

void UInputText(short x, short y, Str255 dest, short maxChar, Boolean numOnly) {
    unsigned char ch;
    Boolean done = false;

    dest[0] = 0;
    while (!done) {
        dest[++dest[0]] = ' ';
        dest[++dest[0]] = ' ';
        dest[++dest[0]] = ' ';
        UPrint(dest, x, y);
        dest[0] -= 3;
        UPrint(dest, x, y);
        ch = CursorKey(true);
        if (gDone)
            ch = 13;
        if (ch > 'Z')
            ch -= 32;
        if (numOnly && ch != 3 && ch != 8 && ch != 13 && (ch < '0' || ch > '9'))
            ch = 0;
        switch (ch) {
            case 0: break;
            case 8:
                if (dest[0] > 0)
                    dest[0]--;
                break;
            case 3:
            case 13: done = true; break;
            default:
                if (dest[0] < maxChar)
                    dest[++dest[0]] = ch;
                break;
        }
    }
}

short UInputNum(short x, short y) {
    Str255 numStr;
    long number;

    UInputText(x, y, numStr, 2, true);
    if (numStr[0] == 0)
        return 0;
    StringToNum(numStr, &number);
    if (number < 0)
        number = Absolute(number);
    return number;

    /*
    char    key, digit1, digit2;
    tx=x;
    ty=y;
UInpBegin:
    if (gUpdateWhere==7) SaveWideArea();
    key=CursorKey(false);
    if (gDone) return 0;
    if ((key<'0') || (key>'9')) goto UInpBegin;
    UPrintChar(key,tx,ty);
    digit1=key-48;
UInp2nd:
    if (gUpdateWhere==7) SaveWideArea();
    key=CursorKey(false);
    if (gDone) return 0;
    if (key!=13 && key!=3) goto UINotCR;
    digit2=digit1;
    digit1=0;
    goto UIFinalCR;
UINotCR:
    if (key!=8) goto UINotBS;
    UPrint("\p ",tx-1,ty);
    tx--;
    goto UInpBegin;
UINotBS:
    if ((key<'0') || (key>'9')) goto UInp2nd;
    UPrintChar(key,tx,ty);
    digit2=key-48;
UInp3rd:
    if (gUpdateWhere==7) SaveWideArea();
    key=CursorKey(false);
    if (gDone) return 0;
    if (key==13 || key==3) goto UIFinalCR;
    if (key!=8) goto UInp3rd;
    UPrint("\p ",tx-1,ty);
    tx--;
    goto UInp2nd;
UIFinalCR:
    return (digit1*10)+digit2;
*/
}

long UInputBigNum(short x, short y) {
    Str255 numStr;
    long number;

    UInputText(x, y, numStr, 4, true);
    if (numStr[0] == 0)
        return 0;
    StringToNum(numStr, &number);
    if (number < 0)
        number = Absolute(number);
    return number;
    /*
    long        value;
    short       length;
    Str255      number;
    Boolean     numDone;
    char        key;
     
    tx=x;
    ty=y;
    if (gDone) return 0;
    length=0;
    numDone=FALSE;
    while (!numDone && !gDone)
        {
        key=CursorKey(false);
        if (key==13 || key==3) numDone=TRUE;
        if (key==8 && length>0) { UPrint("\p ",tx-1,ty); tx--; length--; }
        if (key>='0' && key<='9' && length<4)
            {
            UPrintChar(key,tx,ty);
            length++;
            number[length]=key;
            }
        }
    number[0]=length;
    if (length<1)
        {
        number[0]=1;
        number[1]='0';
        }
    StringToNum(number, &value);
    if (value<0) value=Absolute(value);
    return value;
*/
}

void DrawPrompt(void) {
    DrawFramePiece(8, 23, 23);
    DrawFramePieceScroll(12, 24, 23);
    TextScrollAreaUpdate();
    wx = 25;
    wy = 23;
}

void PrintSpell(short spell) {
    GetPascalStringFromArrayByIndex(gString, CFSTR("Spells"), spell);
    UPrintWin(gString);
}

void PrintWeapon(short weapon) {
    GetPascalStringFromArrayByIndex(gString, CFSTR("WeaponsArmour"), weapon);
    UPrintWin(gString);
}

void PrintWeaponList(short weapon) {
    UPrintChar('A' + weapon, wx, wy);
    UPrintChar(':', wx + 1, wy);
    wx += 2;
    GetPascalStringFromArrayByIndex(gString, CFSTR("WeaponsArmour"), weapon);
    if (gString[gString[0]] != 's' && gString[0] < 7)
        gString[++gString[0]] = 's';
    UPrintWin(gString);
    GetPascalStringFromArrayByIndex(gString, CFSTR("WeaponsArmour"), weapon + 24);
    gString[++gString[0]] = 'g';
    gString[++gString[0]] = 'p';
    if (!CFPreferencesGetAppBooleanValue(U3PrefClassicAppearance, kCFPreferencesCurrentApplication, NULL))
        NewPrint(gString, 40 * blkSiz - UThemePascalStringWidth(gString, kThemeCurrentPortFont), wy * blkSiz);
    else
        UPrint(gString, 40 - gString[0], wy);
    UPrintWin("\p\n");
}

void PrintArmour(short armour) {
    Str255 temp;
    GetPascalStringFromArrayByIndex(temp, CFSTR("WeaponsArmour"), armour + 16);
    UPrintWin(temp);
}

void PrintArmourList(short armour) {
    UPrintChar('A' + armour, wx, wy);
    UPrintChar(':', wx + 1, wy);
    wx += 2;
    GetPascalStringFromArrayByIndex(gString, CFSTR("WeaponsArmour"), armour + 16);
    if (gString[gString[0]] != 's' && gString[0] < 7)
        gString[++gString[0]] = 's';
    UPrintWin(gString);
    GetPascalStringFromArrayByIndex(gString, CFSTR("WeaponsArmour"), armour + 40);
    gString[++gString[0]] = 'g';
    gString[++gString[0]] = 'p';
    if (!CFPreferencesGetAppBooleanValue(U3PrefClassicAppearance, kCFPreferencesCurrentApplication, NULL))
        NewPrint(gString, 40 * blkSiz - UThemePascalStringWidth(gString, kThemeCurrentPortFont), wy * blkSiz);
    else
        UPrint(gString, 40 - gString[0], wy);
    UPrintWin("\p\n");
}

void PrintTile(short tile, Boolean plural) {
    Str255 theString;
    GetPascalStringFromArrayByIndex(theString, (plural) ? CFSTR("TilesPlural") : CFSTR("Tiles"), tile);
    UPrintWin(theString);
}

/* monster name depended upon player's x and y location.
even y locations contain the normal name (Thief, Orc, Skeleton, Giant ...)
odd y locations indicate a variant.
if x is even, it's variant one (Brigand, Goblin, Ghoul, Golem ...)
if x is odd, it's variant two (Cutpurse, Troll, Zombie, Titan ...)
*/
void PrintMonster(short which, Boolean plural, char variant) { /* $8457 */
    if (which > 44 && variant > 0) {   // Ö2 = >23
        PrintTile((which - 46) + 63 + variant, plural);
    } else {
        PrintTile(which / 2, plural);
    }
    /*
    short   value;
    if ((ypos & 1)!=0)
        {
        value = which-0x2E;
        if (value<0) { PrintTile(which/2, plural); return; }
        value = value+(xpos & 1);
        PrintTile(value + 64, plural);
        }
    else
        {
        PrintTile(which/2, plural);
        }
*/
}

short GetChar(void) {
    short chnum, oldMouseState;
    oldMouseState = gMouseState;
    gMouseState = 4;
    CursorUpdate();
    chnum = GetKey();
    gMouseState = oldMouseState;
    chnum -= '0';
    return chnum;
}

short GetKey(void) {
    short val;
    while (!GetKeyMouse(0)) {
    }
    val = gKeyPress;
    while (val > 95) {
        val -= 32;
    }
    UPrintChar(val, wx, wy);
    UPrintWin("\p\n");
    return val;
}

void Speak(short perNum, short shnum) { /* $8924 */
    short tlkptr, talk;
    Str255 speechStr, outStr;

    speechStr[0] = 0;
    tlkptr = 0;
    outStr[0] = 0;
    while (perNum > 0 && tlkptr < 256) {
        while (Talk[tlkptr] != 0 && tlkptr < 256)
            tlkptr++;
        perNum--;
        tlkptr++;
    }
    //  tlkptr++;
    while (Talk[tlkptr] != 0 && tlkptr < 256) {
        talk = Talk[tlkptr];
        if (talk == 0xFF) {
            outStr[++outStr[0]] = '\n';
            speechStr[++speechStr[0]] = ' ';
        } else {
            talk = talk & 0x7F;
            outStr[++outStr[0]] = talk;
            if (talk >= 'A' && talk <= 'Z')
                talk += 32;
            if (talk == '-')
                talk = ' ';
            if ((talk >= 'a' && talk <= 'z') || talk == '?' || talk == '&' || talk == '!' || talk == ':' || talk == ' ' ||
                (talk >= '0' && talk <= '9'))
                speechStr[++speechStr[0]] = talk;
        }
        tlkptr++;
    }
    if (!CFPreferencesGetAppBooleanValue(U3PrefClassicAppearance, kCFPreferencesCurrentApplication, NULL)) {
        RewrapString(outStr, false);
    }
    UPrint(outStr, tx, ty);
    if (speechStr[0] > 2)
        Speech(speechStr, shnum);
}

void WinText(short grey) {
    RGBColor color;
    SetUpFont();
    TextSize(blkSiz);
    color.red = grey * 256;
    color.green = grey * 256;
    color.blue = grey * 256;
    RGBForeColor(&color);
    UPrint2("\p   And so it came to", 2.5);
    UPrint2("\ppass  that  on  this", 4.5);
    UPrint2("\pday EXODUS,hell-born", 6.5);
    UPrint2("\pincarnate  of  evil,", 8.5);
    UPrint2("\pwas vanquished  from", 10.5);
    UPrint2("\pSOSARIA.   What  now", 12.5);
    UPrint2("\plies  ahead  in  the", 14.5);
    UPrint2("\pULTIMA saga can only", 16.5);
    UPrint2("\pbe pure speculation!", 18.5);
    UPrint2("\pOnward to ULTIMA IV!", 20.5);
}

// returns 0 if search string not found.
unsigned char StringLocation(Str255 source, Str255 search) {
    unsigned char i = 1;
    while (i < (source[0] - search[0])) {
        unsigned char j = 0;
        while (j < search[0] && (source[j + i] == search[j + 1])) {
            j++;
        }
        if (j == search[0])
            return i;
        i++;
    }
    return 0;
}

void SearchReplace(Str255 source, Str255 search, Str255 replace) {
    if (1) {
        char i = 1;
        char diff = replace[0] - search[0];
        while (i < source[0] - search[0]) {
            Boolean match = true;
            char offset = 0;
            while (match && offset < search[0]) {
                match = (source[i + offset] == search[1 + offset]);
                offset++;
            }
            if (match) {
                if (diff != 0)
                    BlockMoveData(source + i + search[0], source + i + replace[0], source[0] - i + diff + 1);
                BlockMoveData(replace + 1, source + i, replace[0]);
                source[0] += diff;
            }
            ++i;
        }
    } else {
        Handle sourceHandle = NewHandle(source[0]);
        HLock(sourceHandle);
        BlockMoveData(&source[1], *sourceHandle, source[0]);
        Munger(sourceHandle, 0, search + 1, search[0], replace + 1, replace[0]);
        Size newSize = GetHandleSize(sourceHandle);
        BlockMoveData(*sourceHandle, source + 1, newSize);
        source[0] = (unsigned char)newSize;
        HUnlock(sourceHandle);
        DisposeHandle(sourceHandle);
    }
}

bool IsNewline(unsigned char ch) {
    return (ch == 0xB5 || ch == 0x0A);
}

void RewrapString(Str255 str, Boolean withCursor) {
    unsigned char temp;
    short width, begin, i, max;

    max = blkSiz * (15 - withCursor);
    SetNewFont(false);
    begin = 1;
    while (begin<str[0] && IsNewline(str[begin])) {
        begin++;
    }
    i = str[0];
    while (i > begin && IsNewline(str[i])) {
        i--;
    }
    while (i >= begin) {
        if (IsNewline(str[i])) {
            str[i] = ' ';
        }
        i--;
    }
    i = 1;
    while (i < str[0] && IsNewline(str[i])) {
        i++;
    }
    while (i <= str[0] && !IsNewline(str[i])) {
        while (i<=str[0] && str[i] != ' ' && !IsNewline(str[i])) {
            i++;
        }
        temp = str[begin - 1];
        str[begin - 1] = i - begin;
        width = UThemePascalStringWidth(str + begin - 1, kThemeCurrentPortFont);
        str[begin - 1] = temp;
        if (width > max) {
            i--;
            while (i > 0 && str[i] != ' ') {
                i--;
            }
            if (i > 0) {
                str[i] = 0x0A;
                begin = i + 1;
            }
        }
        i++;
    }
}

void AddString(Str255 str1, Str255 str2) {
    BlockMove(str2 + 1, str1 + str1[0] + 1, str2[0]);
    str1[0] += str2[0];
}

