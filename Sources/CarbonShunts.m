#import "CarbonShunts.h"
#import <Cocoa/Cocoa.h>

extern CGrafPtr mainPort; // Legacy drawing port, unused under Cocoa

void ForceUpdateMain(void) {
    [[NSGraphicsContext currentContext] flushGraphics];
}

void LWSetArrowCursor(void) {
    [[NSCursor arrowCursor] set];
}

void LWSetDialogPort(DialogPtr theDialog) {
    (void)theDialog; // no-op with Cocoa drawing
}

void LWGetScreenRect(Rect *rect) {
    NSRect screen = [[NSScreen mainScreen] frame];
    rect->left   = (short)screen.origin.x;
    rect->top    = (short)screen.origin.y;
    rect->right  = (short)(screen.origin.x + screen.size.width);
    rect->bottom = (short)(screen.origin.y + screen.size.height);
}

const BitMap *LWPortCopyBits(CGrafPtr port) {
    (void)port; // QuickDraw port unsupported
    return NULL;
}

OSErr LWGetDialogControl(DialogRef inDialog, SInt16 inItemNo, ControlRef *outControl) {
    (void)inDialog; (void)inItemNo; if (outControl) *outControl = NULL;
    return noErr;
}

void LWGetPortForeColor(CGrafPtr port, RGBColor *color) {
    (void)port;
    NSColor *c = [NSColor blackColor];
    CGFloat r,g,b; [c getRed:&r green:&g blue:&b alpha:NULL];
    color->red   = r * 65535; 
    color->green = g * 65535; 
    color->blue  = b * 65535; 
}

void LWGetPortBackColor(CGrafPtr port, RGBColor *color) {
    (void)port;
    NSColor *c = [NSColor whiteColor];
    CGFloat r,g,b; [c getRed:&r green:&g blue:&b alpha:NULL];
    color->red   = r * 65535; 
    color->green = g * 65535; 
    color->blue  = b * 65535; 
}

void LWGetPortBounds(CGrafPtr port, Rect *bounds) {
    (void)port;
    bounds->left = bounds->top = 0;
    bounds->right = bounds->bottom = 0;
}

void LWGetPortPenLocation(CGrafPtr port, Point *point) {
    (void)port;
    point->h = point->v = 0;
}

void LWGetWindowBounds(WindowRef window, Rect *bounds) {
    NSWindow *win = (__bridge NSWindow *)window;
    NSRect f = [win frame];
    bounds->left   = (short)f.origin.x;
    bounds->top    = (short)f.origin.y;
    bounds->right  = (short)(f.origin.x + f.size.width);
    bounds->bottom = (short)(f.origin.y + f.size.height);
}

Boolean LWIsControlActive(ControlHandle control) {
    NSControl *ctrl = (__bridge NSControl *)control;
    return [ctrl isEnabled];
}

Boolean LWIsMenuItemEnabled(MenuRef menu, MenuItemIndex item) {
    NSMenu *m = (__bridge NSMenu *)menu;
    if (item <= 0 || item > [m numberOfItems]) return false;
    NSMenuItem *mi = [m itemAtIndex:item-1];
    return [mi isEnabled];
}

void LWDisableMenuItem(MenuRef theMenu, short item) {
    NSMenu *m = (__bridge NSMenu *)theMenu;
    if (item > 0 && item <= [m numberOfItems])
        [[m itemAtIndex:item-1] setEnabled:NO];
}

void LWEnableMenuItem(MenuRef theMenu, short item) {
    NSMenu *m = (__bridge NSMenu *)theMenu;
    if (item > 0 && item <= [m numberOfItems])
        [[m itemAtIndex:item-1] setEnabled:YES];
}

OSStatus LWValidWindowRect(WindowRef window, const Rect *bounds) {
    (void)window; (void)bounds;
    return noErr;
}

OSStatus LWInvalWindowRect(WindowRef window, const Rect *bounds) {
    (void)window; (void)bounds;
    return noErr;
}

Boolean GoodHandle(Handle h) {
    return (h != nil);
}

void LWBlockZero(void *destPtr, Size byteCount) {
    memset(destPtr, 0, (size_t)byteCount);
}

void DefineDefaultItem(DialogPtr theDialog, short item) {
    (void)theDialog; (void)item;
}

