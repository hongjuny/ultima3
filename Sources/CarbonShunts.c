// Shunts

#import "CarbonShunts.h"
#import "CocoaBridge.h"
#include "U3Bitmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern CGrafPtr mainPort;
typedef struct U3LegacyWorld {
    U3Bitmap bitmap;
    PixMap pixmap;
    PixMapPtr pixmapPointer;
    struct U3LegacyWorld *next;
} U3LegacyWorld;

static U3LegacyWorld *sWorlds;
static CGrafPtr sCurrentPort;

static U3LegacyWorld *U3FindWorld(const void *token) {
    for (U3LegacyWorld *world = sWorlds; world; world = world->next)
        if ((const void *)world == token)
            return world;
    return NULL;
}

static void U3SelectPort(CGrafPtr port) {
    sCurrentPort = port;
    U3LegacyWorld *world = U3FindWorld(port);
    U3CocoaSelectBitmap(world ? &world->bitmap : NULL,
        world ? world->pixmap.bounds.left : 0, world ? world->pixmap.bounds.top : 0);
}
extern void GetPascalStringFromArrayByIndex(StringPtr pstringPtr, CFStringRef identifier, int index);

enum {
    kU3BaseResourceID = 400
};

void ForceUpdateMain(void) {
    U3CocoaPresentMainSurface();
}

void LWSetArrowCursor(void) {
}

void LWSetDialogPort(DialogPtr theDialog) {
    (void)theDialog;
}

void LWGetScreenRect(Rect *rect) {
    CGRect bounds = CGDisplayBounds(CGMainDisplayID());

    if (CGRectGetWidth(bounds) <= 0 || CGRectGetHeight(bounds) <= 0) {
        rect->left = 0;
        rect->top = 0;
        rect->right = 1280;
        rect->bottom = 768;
        return;
    }

    rect->left = (short)CGRectGetMinX(bounds);
    rect->top = (short)CGRectGetMinY(bounds);
    rect->right = (short)CGRectGetMaxX(bounds);
    rect->bottom = (short)CGRectGetMaxY(bounds);
}

const BitMap *LWPortCopyBits(CGrafPtr port) {
    return (const BitMap *)port;
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
    U3LegacyWorld *world = U3FindWorld(port);
    if (world) {
        *bounds = world->pixmap.bounds;
        return;
    }
    LWGetScreenRect(bounds);
}

void LWGetPortPenLocation(CGrafPtr port, Point *point) {
    (void)port;
    U3CocoaGetPen(point);
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
    U3CocoaDrawPascalString(text);
}

void DrawText(const void *textBuf, short firstByte, short byteCount) {
    U3CocoaDrawBytes(textBuf, firstByte, byteCount);
}

void TextFont(short font) {
    U3CocoaSetTextFont(font);
}

void TextSize(short size) {
    U3CocoaSetTextSize(size);
}

void TextFace(short face) {
    U3CocoaSetTextFace(face);
}

void TextMode(short mode) {
    (void)mode;
}

void ForeColor(long color) {
    U3CocoaSetForegroundQuickDrawColor(color);
}

void BackColor(long color) {
    U3CocoaSetBackgroundQuickDrawColor(color);
}

void RGBForeColor(const RGBColor *color) {
    if (color)
        U3CocoaSetForegroundRGB(color->red, color->green, color->blue);
}

void RGBBackColor(const RGBColor *color) {
    if (color)
        U3CocoaSetBackgroundRGB(color->red, color->green, color->blue);
}

void OpColor(const RGBColor *color) {
    (void)color;
}

void MoveTo(short h, short v) {
    U3CocoaMoveTo(h, v);
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
    if (rect)
        U3CocoaPaintRect(rect->left, rect->top, rect->right, rect->bottom);
}

void EraseRect(const Rect *rect) {
    if (rect)
        U3CocoaEraseRect(rect->left, rect->top, rect->right, rect->bottom);
}

void FrameRect(const Rect *rect) {
    if (rect)
        U3CocoaFrameRect(rect->left, rect->top, rect->right, rect->bottom);
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
    (void)updateRgn;
    if (!rect) return;
    U3LegacyWorld *world = U3FindWorld(sCurrentPort);
    U3Bitmap *bitmap = world ? &world->bitmap : U3CocoaMainBitmap();
    U3BitmapRect area = {rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top};
    if (world) {
        area.x -= world->pixmap.bounds.left;
        area.y -= world->pixmap.bounds.top;
    }
    uint8_t background[3];
    U3CocoaGetBackground(background);
    if (U3BitmapScroll(bitmap, area, dh, dv, background) && !world)
        U3CocoaInvalidateMainSurface();
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
    U3CocoaGetMousePoint(mouseLoc);
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
        *port = (GrafPtr)sCurrentPort;
}

void SetPort(GrafPtr port) {
    U3SelectPort((CGrafPtr)port);
}

void SetPortWindowPort(WindowRef window) {
    U3SelectPort((CGrafPtr)window);
}

CGrafPtr GetWindowPort(WindowRef window) {
    return (CGrafPtr)window;
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
    U3CocoaGetPen(point);
}

OSErr NewGWorld(GWorldPtr *offscreenGWorld, short pixelDepth, const Rect *boundsRect, CTabHandle cTable, GDHandle aGDevice, GWorldFlags flags) {
    (void)pixelDepth;
    (void)cTable;
    (void)aGDevice;
    (void)flags;
    if (!offscreenGWorld)
        return paramErr;
    *offscreenGWorld = nil;
    if (!boundsRect)
        return paramErr;
    int width = boundsRect->right - boundsRect->left;
    int height = boundsRect->bottom - boundsRect->top;
    /* Our legacy callers mask rowBytes with 0x7fff; the wide font atlas needs it. */
    if (width <= 0 || width > 8191 || height <= 0)
        return paramErr;
    U3LegacyWorld *world = calloc(1, sizeof(*world));
    if (!world)
        return memFullErr;
    if (!U3BitmapAllocate(&world->bitmap, width, height)) {
        free(world);
        return memFullErr;
    }
    world->pixmap.baseAddr = (Ptr)world->bitmap.pixels;
    world->pixmap.rowBytes = (short)(world->bitmap.stride | 0x8000);
    world->pixmap.bounds = *boundsRect;
    world->pixmap.pixelSize = 32;
    world->pixmap.pixelType = 16; /* QuickDraw direct RGB pixels. */
    world->pixmap.cmpCount = 3;
    world->pixmap.cmpSize = 8;
    world->pixmap.hRes = world->pixmap.vRes = 72 << 16;
    world->pixmapPointer = &world->pixmap;
    world->next = sWorlds;
    sWorlds = world;
    *offscreenGWorld = (GWorldPtr)world;
    return noErr;
}

void DisposeGWorld(GWorldPtr offscreenGWorld) {
    U3LegacyWorld **link = &sWorlds;
    while (*link && (GWorldPtr)*link != offscreenGWorld)
        link = &(*link)->next;
    if (!*link)
        return;
    U3LegacyWorld *world = *link;
    *link = world->next;
    if (sCurrentPort == (CGrafPtr)world)
        U3SelectPort(nil);
    U3BitmapDispose(&world->bitmap);
    free(world);
}

void GetGWorld(CGrafPtr *port, GDHandle *gdh) {
    if (port)
        *port = sCurrentPort;
    if (gdh)
        *gdh = nil;
}

void SetGWorld(CGrafPtr port, GDHandle gdh) {
    U3SelectPort(port);
    (void)gdh;
}

PixMapHandle GetGWorldPixMap(GWorldPtr offscreenGWorld) {
    U3LegacyWorld *world = U3FindWorld(offscreenGWorld);
    return world ? &world->pixmapPointer : nil;
}

Boolean LockPixels(PixMapHandle pm) {
    return pm && *pm && (*pm)->baseAddr;
}

Ptr GetPixBaseAddr(PixMapHandle pixels) {
    return pixels && *pixels ? (*pixels)->baseAddr : NULL;
}

void UnlockPixels(PixMapHandle pm) {
    (void)pm;
}

static U3Bitmap *U3ResolveBitmap(const void *port, const Rect *rect, U3BitmapRect *local) {
    if (!port || !rect)
        return NULL;
    U3LegacyWorld *world = U3FindWorld(port);
    *local = (U3BitmapRect){rect->left, rect->top,
        rect->right - rect->left, rect->bottom - rect->top};
    if (world) {
        local->x -= world->pixmap.bounds.left;
        local->y -= world->pixmap.bounds.top;
        return &world->bitmap;
    }
    return (CGrafPtr)port == mainPort ? U3CocoaMainBitmap() : NULL;
}

void CopyBits(const BitMap *srcBits, const BitMap *dstBits, const Rect *srcRect, const Rect *dstRect, short mode, RgnHandle maskRgn) {
    U3BitmapRect from = {0}, to = {0};
    U3Bitmap *source = U3ResolveBitmap(srcBits, srcRect, &from);
    U3Bitmap *destination = U3ResolveBitmap(dstBits, dstRect, &to);
    if (maskRgn || (mode != srcCopy && mode != ditherCopy))
        return;
    if (U3BitmapCopy(destination, to, source, from) && destination == U3CocoaMainBitmap())
        U3CocoaInvalidateMainSurface();
}

void CopyMask(const BitMap *srcBits, const BitMap *maskBits, const BitMap *dstBits, const Rect *srcRect, const Rect *maskRect, const Rect *dstRect) {
    U3BitmapRect from = {0}, maskArea = {0}, to = {0};
    U3Bitmap *source = U3ResolveBitmap(srcBits, srcRect, &from);
    U3Bitmap *mask = U3ResolveBitmap(maskBits, maskRect, &maskArea);
    U3Bitmap *destination = U3ResolveBitmap(dstBits, dstRect, &to);
    if (U3BitmapCopyMasked(destination, to, source, from, mask, maskArea) &&
        destination == U3CocoaMainBitmap())
        U3CocoaInvalidateMainSurface();
}

Boolean U3LegacyBitmapSelfTest(void) {
    CGrafPtr savedPort = sCurrentPort;
    CGrafPtr savedMain = mainPort;
    static char testMainToken;
    GWorldPtr source = nil, destination = nil;
    Rect bounds = {10, 20, 12, 22};
    Rect enlarged = {0, 0, 4, 4};
    Boolean passed = false;
    if (NewGWorld(&source, 32, &bounds, nil, nil, 0) != noErr ||
        NewGWorld(&destination, 32, &enlarged, nil, nil, 0) != noErr)
        goto cleanup;
    SetGWorld(source, nil);
    RGBColor color = {65535, 0, 0};
    RGBForeColor(&color);
    PaintRect(&bounds);
    PixMapHandle pixels = GetGWorldPixMap(source);
    if (!LockPixels(pixels) || !(*pixels)->baseAddr ||
        ((*pixels)->rowBytes & 0x3fff) != 8)
        goto cleanup;
    CopyBits(LWPortCopyBits(source), LWPortCopyBits(destination),
             &bounds, &enlarged, srcCopy, nil);
    U3LegacyWorld *world = U3FindWorld(destination);
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            uint8_t *pixel = world->bitmap.pixels + y * world->bitmap.stride + x * 4;
            if (pixel[0] != 255 || pixel[1] != 0 || pixel[2] != 0) {
                fprintf(stderr, "Unexpected bitmap pixel (%d,%d): %u %u %u %u\n",
                        x, y, pixel[0], pixel[1], pixel[2], pixel[3]);
                goto cleanup;
            }
        }
    }
    if (!U3CocoaResizeMainBitmap(4, 4))
        goto cleanup;
    mainPort = (CGrafPtr)&testMainToken;
    SetGWorld(mainPort, nil);
    RGBColor blue = {0, 0, 65535};
    RGBForeColor(&blue);
    PaintRect(&enlarged);
    CopyBits(LWPortCopyBits(mainPort), LWPortCopyBits(destination),
             &enlarged, &enlarged, srcCopy, nil);
    if (world->bitmap.pixels[0] != 0 || world->bitmap.pixels[2] != 255)
        goto cleanup;
    CopyBits(LWPortCopyBits(source), LWPortCopyBits(mainPort),
             &bounds, &enlarged, srcCopy, nil);
    CopyBits(LWPortCopyBits(mainPort), LWPortCopyBits(destination),
             &enlarged, &enlarged, srcCopy, nil);
    if (world->bitmap.pixels[0] != 255 || world->bitmap.pixels[2] != 0)
        goto cleanup;
    for (int y = 0; y < 4; ++y)
        for (int x = 2; x < 4; ++x)
            memset(world->bitmap.pixels + y * world->bitmap.stride + x * 4, 255, 4);
    RGBForeColor(&blue);
    PaintRect(&enlarged);
    CopyMask(LWPortCopyBits(source), LWPortCopyBits(destination), LWPortCopyBits(mainPort),
             &bounds, &enlarged, &enlarged);
    U3Bitmap *screen = U3CocoaMainBitmap();
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            uint8_t *pixel = screen->pixels + y * screen->stride + x * 4;
            if (pixel[0] != (x < 2 ? 255 : 0) || pixel[2] != (x < 2 ? 0 : 255))
                goto cleanup;
        }
    }
    passed = true;
cleanup:
    mainPort = savedMain;
    DisposeGWorld(source);
    DisposeGWorld(destination);
    SetGWorld(savedPort, nil);
    return passed;
}

Boolean U3LegacyDrawImageURL(CFURLRef url, CGrafPtr port, const Rect *bounds,
                             int columns, int rows) {
    U3LegacyWorld *world = U3FindWorld(port);
    if (!bounds || (!world && (!port || port != mainPort)))
        return false;
    U3Bitmap image = {0};
    int width = bounds->right - bounds->left;
    int height = bounds->bottom - bounds->top;
    if (!U3CocoaLoadImage(&image, url, width, height, columns, rows))
        return false;
    Boolean success = true;
    U3BitmapRect source = {0, 0, width, height};
    if (world) {
        U3BitmapRect destination = {bounds->left - world->pixmap.bounds.left,
            bounds->top - world->pixmap.bounds.top, width, height};
        success = U3BitmapCopy(&world->bitmap, destination, &image, source);
    } else {
        U3CocoaDrawBitmap(&image, source, bounds->left, bounds->top, width, height);
    }
    U3BitmapDispose(&image);
    return success;
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

void SetAntiAliasedTextEnabled(Boolean enabled, short minimumSize) {
    (void)enabled;
    (void)minimumSize;
}

MenuRef GetMenuHandle(MenuID menuID) {
    (void)menuID;
    return nil;
}

void DeleteMenuItem(MenuRef menu, MenuItemIndex item) {
    (void)menu;
    (void)item;
}

void DrawMenuBar(void) {
}

short GetMBarHeight(void) {
    return 0;
}

void MoveWindow(WindowRef window, short hGlobal, short vGlobal, Boolean front) {
    (void)window;
    (void)hGlobal;
    (void)vGlobal;
    (void)front;
}

void SizeWindow(WindowRef window, short w, short h, Boolean update) {
    (void)window;
    (void)w;
    (void)h;
    (void)update;
}

void ShowWindow(WindowRef window) {
    (void)window;
}

void HideWindow(WindowRef window) {
    (void)window;
}

void DisposeWindow(WindowRef window) {
    (void)window;
}

void BringToFront(WindowRef window) {
    (void)window;
}

RgnHandle NewRgn(void) {
    return nil;
}

void DisposeRgn(RgnHandle rgn) {
    (void)rgn;
}

void RectRgn(RgnHandle rgn, const Rect *rect) {
    (void)rgn;
    (void)rect;
}

void CopyRgn(RgnHandle srcRgn, RgnHandle dstRgn) {
    (void)srcRgn;
    (void)dstRgn;
}

void UnionRgn(RgnHandle srcRgnA, RgnHandle srcRgnB, RgnHandle dstRgn) {
    (void)srcRgnA;
    (void)srcRgnB;
    (void)dstRgn;
}

void SectRgn(RgnHandle srcRgnA, RgnHandle srcRgnB, RgnHandle dstRgn) {
    (void)srcRgnA;
    (void)srcRgnB;
    (void)dstRgn;
}

void DiffRgn(RgnHandle srcRgnA, RgnHandle srcRgnB, RgnHandle dstRgn) {
    (void)srcRgnA;
    (void)srcRgnB;
    (void)dstRgn;
}

Boolean EqualRgn(RgnHandle rgnA, RgnHandle rgnB) {
    return rgnA == rgnB;
}

RgnHandle GetGrayRgn(void) {
    return nil;
}

MenuBarHandle GetMCInfo(void) {
    return nil;
}
