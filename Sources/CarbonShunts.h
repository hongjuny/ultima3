//
//  CarbonShunts.h
//  Ultima3
//

#ifndef CarbonShunts_h
#define CarbonShunts_h

#include <Carbon/Carbon.h>

void ForceUpdateMain(void);
void LWSetArrowCursor(void);
void LWSetDialogPort(DialogPtr theDialog);
void LWGetScreenRect(Rect* rect);
const BitMap * LWPortCopyBits(CGrafPtr port);
OSErr LWGetDialogControl(DialogRef inDialog, SInt16 inItemNo, ControlRef* outControl);
void LWGetPortForeColor(CGrafPtr port, RGBColor* color);
void LWGetPortBackColor(CGrafPtr port, RGBColor* color);
void LWGetPortBounds(CGrafPtr port, Rect* bounds);
void LWGetPortPenLocation(CGrafPtr port, Point* point);
void LWGetWindowBounds(WindowRef window, Rect* bounds);
Boolean LWIsControlActive(ControlHandle control);
Boolean LWIsMenuItemEnabled(MenuRef menu, MenuItemIndex item);
void LWDisableMenuItem(MenuRef theMenu, short item);
void LWEnableMenuItem(MenuRef theMenu, short item);
OSStatus LWValidWindowRect(WindowRef window, const Rect* bounds);
OSStatus LWInvalWindowRect(WindowRef window, const Rect* bounds);
Boolean GoodHandle(Handle h);
void LWBlockZero(void *destPtr, Size byteCount);
void DefineDefaultItem(DialogPtr theDialog, short item);

// Older Carbon APIs like BlockMove() and ObscureCursor() are not available
// when compiling for modern macOS SDKs. Provide simple shims so legacy source
// files continue to build. These implementations are minimal but sufficient for
// the game's needs.
#ifndef BlockMove
#include <string.h>
static inline void BlockMove(const void *src, void *dst, Size len) {
    memmove(dst, src, (size_t)len);
}
#endif

#ifndef ObscureCursor
static inline void ObscureCursor(void) {
//    HideCursor();
}
#endif

#endif /* CarbonShunts_h */
