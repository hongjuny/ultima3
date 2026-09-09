// Shunts

#import "CarbonShunts.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern CGrafPtr mainPort;

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
