// Shunts

#import "CarbonShunts.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern CGrafPtr mainPort;
extern void GetPascalStringFromArrayByIndex(StringPtr pstringPtr, CFStringRef identifier, int index);

enum {
    kU3BaseResourceID = 400
};

void ForceUpdateMain(void) {
}

void LWSetArrowCursor(void) {
}

void LWSetDialogPort(DialogPtr theDialog) {
    (void)theDialog;
}

void LWGetScreenRect(Rect *rect) {
    CGRect bounds = CGDisplayBounds(CGMainDisplayID());

    rect->left = (short)CGRectGetMinX(bounds);
    rect->top = (short)CGRectGetMinY(bounds);
    rect->right = (short)CGRectGetMaxX(bounds);
    rect->bottom = (short)CGRectGetMaxY(bounds);
}

const BitMap *LWPortCopyBits(CGrafPtr port) {
    (void)port;
    return nil;
}

OSErr LWGetDialogControl(DialogRef inDialog, SInt16 inItemNo, ControlRef *outControl) {
    (void)inDialog;
    (void)inItemNo;
    if (outControl)
        *outControl = nil;
    return paramErr;
}

void LWGetPortForeColor(CGrafPtr port, RGBColor *color) {
    (void)port;
    color->red = 0;
    color->green = 0;
    color->blue = 0;
}

void LWGetPortBackColor(CGrafPtr port, RGBColor *color) {
    (void)port;
    color->red = 0xFFFF;
    color->green = 0xFFFF;
    color->blue = 0xFFFF;
}

void LWGetPortBounds(CGrafPtr port, Rect *bounds) {
    (void)port;
    LWGetScreenRect(bounds);
}

void LWGetPortPenLocation(CGrafPtr port, Point *point) {
    (void)port;
    point->h = 0;
    point->v = 0;
}

void LWGetWindowBounds(WindowRef window, Rect *bounds) {
    (void)window;
    LWGetScreenRect(bounds);
}

Boolean LWIsControlActive(ControlHandle control) {
    return control != nil;
}

Boolean LWIsMenuItemEnabled(MenuRef menu, MenuItemIndex item) {
    (void)menu;
    (void)item;
    return true;
}

void LWDisableMenuItem(MenuRef theMenu, short item) {
    (void)theMenu;
    (void)item;
}

void LWEnableMenuItem(MenuRef theMenu, short item) {
    (void)theMenu;
    (void)item;
}

OSStatus LWValidWindowRect(WindowRef window, const Rect *bounds) {
    (void)window;
    (void)bounds;
    return noErr;
}

OSStatus LWInvalWindowRect(WindowRef window, const Rect *bounds) {
    (void)window;
    (void)bounds;
    return noErr;
}

Boolean GoodHandle(Handle h) {
    return h != nil;
}

void LWBlockZero(void *destPtr, Size byteCount) {
#if TARGET_CPU_68K
    long offset = (long)byteCount;
    while (offset > 0) {
        ((Ptr)destPtr)[--offset] = 0;
    }
#else
    memset(destPtr, 0, byteCount);
#endif
}

void DefineDefaultItem(DialogPtr theDialog, short item) {
    (void)theDialog;
    (void)item;
}

void BlockMove(const void *srcPtr, void *destPtr, Size byteCount) {
    memmove(destPtr, srcPtr, (size_t)byteCount);
}

void BlockMoveData(const void *srcPtr, void *destPtr, Size byteCount) {
    memmove(destPtr, srcPtr, (size_t)byteCount);
}

short Random(void) {
    return (short)arc4random();
}

void NumToString(long theNum, Str255 theString) {
    char buffer[32];
    int length = snprintf(buffer, sizeof(buffer), "%ld", theNum);

    if (length < 0)
        length = 0;
    if (length > 255)
        length = 255;
    theString[0] = (unsigned char)length;
    memcpy(theString + 1, buffer, (size_t)length);
}

void SysBeep(short duration) {
    (void)duration;
}

Size FreeMem(void) {
    return 0;
}

Size MaxMem(Size *grow) {
    if (grow)
        *grow = 0;
    return 0;
}

void GetIndString(StringPtr theString, short strListID, short index) {
    CFStringRef identifier = NULL;

    theString[0] = 0;
    switch (strListID) {
        case kU3BaseResourceID + 10:
            identifier = CFSTR("Tiles");
            break;
        case kU3BaseResourceID + 12:
            identifier = CFSTR("Messages");
            break;
        case kU3BaseResourceID + 14:
            identifier = CFSTR("MoreMessages");
            break;
        case kU3BaseResourceID + 15:
            identifier = CFSTR("Classes");
            break;
        case kU3BaseResourceID + 16:
            identifier = CFSTR("Races");
            break;
        case kU3BaseResourceID + 17:
            identifier = CFSTR("Spells");
            break;
        default:
            return;
    }
    GetPascalStringFromArrayByIndex(theString, identifier, index - 1);
}

void GetFNum(ConstStr255Param fontName, short *theNum) {
    (void)fontName;
    if (theNum)
        *theNum = 0;
}

short StringWidth(ConstStr255Param text) {
    return text ? (short)(text[0] * 8) : 0;
}

void DrawString(ConstStr255Param text) {
    (void)text;
}

void DrawText(const void *textBuf, short firstByte, short byteCount) {
    (void)textBuf;
    (void)firstByte;
    (void)byteCount;
}

void TextFont(short font) {
    (void)font;
}

void TextSize(short size) {
    (void)size;
}

void TextFace(short face) {
    (void)face;
}

void TextMode(short mode) {
    (void)mode;
}

void ForeColor(long color) {
    (void)color;
}

void BackColor(long color) {
    (void)color;
}

void RGBForeColor(const RGBColor *color) {
    (void)color;
}

void RGBBackColor(const RGBColor *color) {
    (void)color;
}

void OpColor(const RGBColor *color) {
    (void)color;
}

void MoveTo(short h, short v) {
    (void)h;
    (void)v;
}

void LineTo(short h, short v) {
    (void)h;
    (void)v;
}

void PenMode(short mode) {
    (void)mode;
}

void PenSize(short width, short height) {
    (void)width;
    (void)height;
}

void PaintRect(const Rect *rect) {
    (void)rect;
}

void EraseRect(const Rect *rect) {
    (void)rect;
}

void FrameRect(const Rect *rect) {
    (void)rect;
}

void FrameRoundRect(const Rect *rect, short ovalWidth, short ovalHeight) {
    (void)rect;
    (void)ovalWidth;
    (void)ovalHeight;
}

void InvertRect(const Rect *rect) {
    (void)rect;
}

void ClipRect(const Rect *rect) {
    (void)rect;
}

void ScrollRect(const Rect *rect, short dh, short dv, RgnHandle updateRgn) {
    (void)rect;
    (void)dh;
    (void)dv;
    (void)updateRgn;
}

void BeginUpdate(WindowRef window) {
    (void)window;
}

void EndUpdate(WindowRef window) {
    (void)window;
}

void InvalWindowRect(WindowRef window, const Rect *bounds) {
    (void)window;
    (void)bounds;
}

void QDFlushPortBuffer(CGrafPtr port, RgnHandle region) {
    (void)port;
    (void)region;
}

void GetMouse(Point *mouseLoc) {
    if (mouseLoc) {
        mouseLoc->h = 0;
        mouseLoc->v = 0;
    }
}

Boolean StillDown(void) {
    return false;
}

Boolean TrackGoAway(WindowRef window, Point point) {
    (void)window;
    (void)point;
    return false;
}

void LocalToGlobal(Point *point) {
    (void)point;
}

void GlobalToLocal(Point *point) {
    (void)point;
}

void GetPort(GrafPtr *port) {
    if (port)
        *port = nil;
}

void SetPort(GrafPtr port) {
    (void)port;
}

void SetPortWindowPort(WindowRef window) {
    (void)window;
}

CGrafPtr GetWindowPort(WindowRef window) {
    (void)window;
    return nil;
}

WindowRef NewCWindow(void *wStorage, const Rect *boundsRect, ConstStr255Param title, Boolean visible, short procID, WindowRef behind, Boolean goAwayFlag, long refCon) {
    (void)wStorage;
    (void)boundsRect;
    (void)title;
    (void)visible;
    (void)procID;
    (void)behind;
    (void)goAwayFlag;
    (void)refCon;
    return nil;
}

WindowRef NewWindow(void *wStorage, const Rect *boundsRect, ConstStr255Param title, Boolean visible, short procID, WindowRef behind, Boolean goAwayFlag, long refCon) {
    return NewCWindow(wStorage, boundsRect, title, visible, procID, behind, goAwayFlag, refCon);
}

OSStatus SetWindowContentColor(WindowRef window, const RGBColor *color) {
    (void)window;
    (void)color;
    return noErr;
}

Boolean IsWindowModified(WindowRef window) {
    (void)window;
    return false;
}

OSStatus GetWindowProxyFSSpec(WindowRef window, FSSpec *file) {
    (void)window;
    (void)file;
    return paramErr;
}

DialogRef GetNewDialog(short dialogID, void *dStorage, WindowRef behind) {
    (void)dialogID;
    (void)dStorage;
    (void)behind;
    return nil;
}

void DisposeDialog(DialogRef dialog) {
    (void)dialog;
}

void ModalDialog(ModalFilterUPP modalFilter, short *itemHit) {
    (void)modalFilter;
    if (itemHit)
        *itemHit = 1;
}

void GetDialogItemText(Handle item, Str255 text) {
    (void)item;
    if (text)
        text[0] = 0;
}

void SetDialogItemText(Handle item, ConstStr255Param text) {
    (void)item;
    (void)text;
}

void SelectDialogItemText(DialogRef dialog, short itemNo, short strtSel, short endSel) {
    (void)dialog;
    (void)itemNo;
    (void)strtSel;
    (void)endSel;
}

short Alert(short alertID, ModalFilterUPP modalFilter) {
    (void)alertID;
    (void)modalFilter;
    return 1;
}

OSStatus StandardAlert(AlertType alertType, ConstStr255Param error, ConstStr255Param explanation, const AlertStdAlertParamRec *param, short *itemHit) {
    (void)alertType;
    (void)error;
    (void)explanation;
    (void)param;
    if (itemHit)
        *itemHit = kAlertStdAlertOKButton;
    return noErr;
}

void AppendMenu(MenuRef menu, ConstStr255Param data) {
    (void)menu;
    (void)data;
}

void AppendResMenu(MenuRef menu, ResType resType) {
    (void)menu;
    (void)resType;
}

MenuRef GetNewMBar(short menuBarID) {
    (void)menuBarID;
    return nil;
}

void SetMenuBar(MenuRef menuList) {
    (void)menuList;
}

long MenuKey(short ch) {
    (void)ch;
    return 0;
}

OSStatus EnableMenuCommand(MenuRef menu, MenuCommand commandID) {
    (void)menu;
    (void)commandID;
    return noErr;
}

MenuRef GetControlPopupMenuHandle(ControlRef control) {
    (void)control;
    return nil;
}

short GetControlValue(ControlRef control) {
    (void)control;
    return 0;
}

void SetControlMaximum(ControlRef control, short maxValue) {
    (void)control;
    (void)maxValue;
}

void SetControlTitle(ControlRef control, ConstStr255Param title) {
    (void)control;
    (void)title;
}

void GetControlTitle(ControlRef control, Str255 title) {
    (void)control;
    if (title)
        title[0] = 0;
}

CursHandle GetCursor(short cursorID) {
    (void)cursorID;
    return nil;
}

void SetCursor(const void *cursor) {
    (void)cursor;
}

void GetPen(Point *point) {
    if (point) {
        point->h = 0;
        point->v = 0;
    }
}

OSErr NewGWorld(GWorldPtr *offscreenGWorld, short pixelDepth, const Rect *boundsRect, CTabHandle cTable, GDHandle aGDevice, GWorldFlags flags) {
    (void)pixelDepth;
    (void)boundsRect;
    (void)cTable;
    (void)aGDevice;
    (void)flags;
    if (offscreenGWorld)
        *offscreenGWorld = nil;
    return noErr;
}

void DisposeGWorld(GWorldPtr offscreenGWorld) {
    (void)offscreenGWorld;
}

void GetGWorld(CGrafPtr *port, GDHandle *gdh) {
    if (port)
        *port = nil;
    if (gdh)
        *gdh = nil;
}

void SetGWorld(CGrafPtr port, GDHandle gdh) {
    (void)port;
    (void)gdh;
}

PixMapHandle GetGWorldPixMap(GWorldPtr offscreenGWorld) {
    (void)offscreenGWorld;
    return nil;
}

Boolean LockPixels(PixMapHandle pm) {
    (void)pm;
    return true;
}

void UnlockPixels(PixMapHandle pm) {
    (void)pm;
}

void CopyBits(const BitMap *srcBits, const BitMap *dstBits, const Rect *srcRect, const Rect *dstRect, short mode, RgnHandle maskRgn) {
    (void)srcBits;
    (void)dstBits;
    (void)srcRect;
    (void)dstRect;
    (void)mode;
    (void)maskRgn;
}

void CopyMask(const BitMap *srcBits, const BitMap *maskBits, const BitMap *dstBits, const Rect *srcRect, const Rect *maskRect, const Rect *dstRect) {
    (void)srcBits;
    (void)maskBits;
    (void)dstBits;
    (void)srcRect;
    (void)maskRect;
    (void)dstRect;
}

void GetCPixel(short h, short v, RGBColor *pixel) {
    (void)h;
    (void)v;
    if (pixel) {
        pixel->red = 0;
        pixel->green = 0;
        pixel->blue = 0;
    }
}

void GetClip(RgnHandle rgn) {
    (void)rgn;
}

void SetClip(RgnHandle rgn) {
    (void)rgn;
}

PolyHandle OpenPoly(void) {
    return nil;
}

void ClosePoly(void) {
}

void PaintPoly(PolyHandle poly) {
    (void)poly;
}

void KillPoly(PolyHandle poly) {
    (void)poly;
}

PicHandle GetPicture(short pictureID) {
    (void)pictureID;
    return nil;
}

void DrawPicture(PicHandle myPicture, const Rect *dstRect) {
    (void)myPicture;
    (void)dstRect;
}

OSStatus GetThemeTextDimensions(CFStringRef string, ThemeFontID fontID, ThemeDrawState state, Boolean wrapToWidth, Point *bounds, short *baseline) {
    (void)fontID;
    (void)state;
    (void)wrapToWidth;
    if (bounds) {
        bounds->h = string ? (short)(CFStringGetLength(string) * 8) : 0;
        bounds->v = 12;
    }
    if (baseline)
        *baseline = 10;
    return noErr;
}

OSStatus DrawThemeTextBox(CFStringRef string, ThemeFontID fontID, ThemeDrawState state, Boolean wrapToWidth, const Rect *bounds, SInt16 justification, CGContextRef context) {
    (void)string;
    (void)fontID;
    (void)state;
    (void)wrapToWidth;
    (void)bounds;
    (void)justification;
    (void)context;
    return noErr;
}

OSErr GetGraphicsImporterForFile(const FSSpec *file, ComponentInstance *gi) {
    (void)file;
    if (gi)
        *gi = nil;
    return paramErr;
}

OSErr GraphicsImportSetGWorld(ComponentInstance gi, CGrafPtr port, GDHandle gdh) {
    (void)gi;
    (void)port;
    (void)gdh;
    return paramErr;
}

OSErr GraphicsImportSetBoundsRect(ComponentInstance gi, const Rect *bounds) {
    (void)gi;
    (void)bounds;
    return paramErr;
}

OSErr GraphicsImportGetBoundsRect(ComponentInstance gi, Rect *bounds) {
    (void)gi;
    if (bounds) {
        bounds->left = 0;
        bounds->top = 0;
        bounds->right = 0;
        bounds->bottom = 0;
    }
    return paramErr;
}

OSErr GraphicsImportDraw(ComponentInstance gi) {
    (void)gi;
    return paramErr;
}

OSErr FSMakeFSSpec(short vRefNum, long dirID, ConstStr255Param fileName, FSSpec *spec) {
    (void)vRefNum;
    (void)dirID;
    (void)fileName;
    (void)spec;
    return fnfErr;
}

OSErr FSpDelete(const FSSpec *spec) {
    (void)spec;
    return noErr;
}

void FSpCreateResFile(const FSSpec *spec, OSType creator, OSType fileType, ScriptCode scriptTag) {
    (void)spec;
    (void)creator;
    (void)fileType;
    (void)scriptTag;
}

short FSpOpenResFile(const FSSpec *spec, SignedByte permission) {
    (void)spec;
    (void)permission;
    return -1;
}

OSErr FSpMakeFSRef(const FSSpec *spec, FSRef *ref) {
    (void)spec;
    (void)ref;
    return fnfErr;
}

OSErr IsAliasFile(const FSSpec *fileFSSpec, Boolean *aliasFileFlag, Boolean *folderFlag) {
    (void)fileFSSpec;
    if (aliasFileFlag)
        *aliasFileFlag = false;
    if (folderFlag)
        *folderFlag = false;
    return noErr;
}

OSErr ResolveAliasFile(FSSpec *theSpec, Boolean resolveAliasChains, Boolean *targetIsFolder, Boolean *wasAliased) {
    (void)theSpec;
    (void)resolveAliasChains;
    if (targetIsFolder)
        *targetIsFolder = false;
    if (wasAliased)
        *wasAliased = false;
    return noErr;
}

OSErr GetEOF(short refNum, long *logEOF) {
    (void)refNum;
    if (logEOF)
        *logEOF = 0;
    return noErr;
}

OSErr FSClose(short refNum) {
    (void)refNum;
    return noErr;
}

MenuBarHandle GetMCInfo(void) {
    return nil;
}
