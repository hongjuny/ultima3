// Shunts

#import "CarbonShunts.h"

extern CGrafPtr mainPort;

void ForceUpdateMain(void) {
#if TARGET_CARBON
    QDFlushPortBuffer(mainPort, nil);
#endif
}

void LWSetArrowCursor(void) {
    Cursor arrowCursor;
#if TARGET_CARBON
    GetQDGlobalsArrow(&arrowCursor);
    SetCursor(&arrowCursor);
#else
    SetCursor(&qd.arrow);
#endif
}

void LWSetDialogPort(DialogPtr theDialog) {
#if TARGET_CARBON
    SetPortDialogPort(theDialog);
#else
    SetPort(theDialog);
#endif
}

void LWGetScreenRect(Rect *rect) {
#if TARGET_CARBON
    BitMap qdbm;

    GetQDGlobalsScreenBits(&qdbm);
    *rect = qdbm.bounds;
#else
    *rect = qd.screenBits.bounds;
#endif
}

const BitMap *LWPortCopyBits(CGrafPtr port) {
#if TARGET_CARBON
    return GetPortBitMapForCopyBits(port);
#else
    return &((GrafPtr)port)->portBits;
#endif
}

OSErr LWGetDialogControl(DialogRef inDialog, SInt16 inItemNo, ControlRef *outControl) {
#if TARGET_CARBON
    return GetDialogItemAsControl(inDialog, inItemNo, outControl);
#else
    short temp;
    Rect rect;
    GetDialogItem(inDialog, inItemNo, &temp, (Handle *)outControl, &rect);
    return nil;
#endif
}

void LWGetPortForeColor(CGrafPtr port, RGBColor *color) {
#if TARGET_CARBON
    GetPortForeColor(port, color);
#else
    *color = port->rgbFgColor;
#endif
}

void LWGetPortBackColor(CGrafPtr port, RGBColor *color) {
#if TARGET_CARBON
    GetPortBackColor(port, color);
#else
    *color = port->rgbBkColor;
#endif
}

void LWGetPortBounds(CGrafPtr port, Rect *bounds) {
#if TARGET_CARBON
    GetPortBounds(port, bounds);
#else
    *bounds = port->portRect;
#endif
}

void LWGetPortPenLocation(CGrafPtr port, Point *point) {
#if TARGET_CARBON
    GetPortPenLocation(port, point);
#else
    point->h = port->pnLoc.h;
    point->v = port->pnLoc.v;
#endif
}

void LWGetWindowBounds(WindowRef window, Rect *bounds) {
#if TARGET_CARBON
    GrafPtr curPort;

    curPort = GetWindowPort(window);
    GetPortBounds(curPort, bounds);
#else
    *bounds = window->portRect;
#endif
}

Boolean LWIsControlActive(ControlHandle control) {
#if TARGET_CARBON
    return IsControlActive(control);
#else
    return ((*control)->contrlHilite == 0);
#endif
}

Boolean LWIsMenuItemEnabled(MenuRef menu, MenuItemIndex item) {
#if TARGET_CARBON
    return IsMenuItemEnabled(menu, item);
#else
    return ((*menu)->enableFlags & (1 << (item + 1)));
#endif
}

void LWDisableMenuItem(MenuRef theMenu, short item) {
#if TARGET_CARBON
    DisableMenuItem(theMenu, item);
#else
    DisableItem(theMenu, item);
#endif
}

void LWEnableMenuItem(MenuRef theMenu, short item) {
#if TARGET_CARBON
    EnableMenuItem(theMenu, item);
#else
    EnableItem(theMenu, item);
#endif
}

OSStatus LWValidWindowRect(WindowRef window, const Rect *bounds) {
#if TARGET_CARBON
    ValidWindowRect(window, bounds);
#else
    ValidRect(bounds);
#endif
    return noErr;
}

OSStatus LWInvalWindowRect(WindowRef window, const Rect *bounds) {
#if TARGET_CARBON
    InvalWindowRect(window, bounds);
#else
    InvalRect(bounds);
#endif
    return noErr;
}

Boolean GoodHandle(Handle h) {
    if (h == nil)
        return false;
#if TARGET_CARBON
    return IsHandleValid(h);
#else
    return true;
#endif
}

void LWBlockZero(void *destPtr, Size byteCount) {
#if TARGET_CPU_68K
    long offset = (long)byteCount;
    while (offset > 0) {
        ((Ptr)destPtr)[--offset] = 0;
    }
#else
    BlockZero(destPtr, byteCount);
#endif
}

void DefineDefaultItem(DialogPtr theDialog, short item) {
    SetDialogDefaultItem(theDialog, item);
}
