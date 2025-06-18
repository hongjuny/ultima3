//
//  CarbonShunts.h
//  Ultima3
//

#ifndef CarbonShunts_h
#define CarbonShunts_h

#ifdef __APPLE__
#include <Carbon/Carbon.h>
#else
/* Minimal Carbon compatibility layer for non-macOS builds */
typedef unsigned char   Boolean;
typedef struct Rect { short top, left, bottom, right; } Rect;
typedef struct Point { short v, h; } Point;
typedef struct RGBColor { unsigned short red, green, blue; } RGBColor;
typedef void *          CGrafPtr;
typedef void *          GDHandle;
typedef void *          WindowRef;
typedef void *          DialogPtr;
typedef void *          DialogRef;
typedef void *          ControlRef;
typedef void *          ControlHandle;
typedef void *          MenuRef;
typedef unsigned short  MenuItemIndex;
typedef void *          RgnHandle;
typedef void *          PixMapHandle;
typedef void *          BitMap;
typedef void *          Handle;
typedef int             OSErr;
typedef int             OSStatus;
typedef int             Size;
typedef int             SInt16;
#define noErr 0

static inline void SetRect(Rect *r, short l, short t, short rgt, short btm) {
    r->left = l; r->top = t; r->right = rgt; r->bottom = btm;
}

static inline void OffsetRect(Rect *r, short dh, short dv) {
    r->left += dh; r->right += dh; r->top += dv; r->bottom += dv;
}

static inline void SetGWorld(CGrafPtr port, GDHandle dev) {
    (void)port; (void)dev;
}

static inline void CopyBits(const BitMap *srcBits, const BitMap *dstBits,
                            const Rect *srcRect, const Rect *dstRect,
                            short mode, RgnHandle maskRgn) {
    (void)srcBits; (void)dstBits; (void)srcRect; (void)dstRect;
    (void)mode; (void)maskRgn;
}
#endif

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

#endif /* CarbonShunts_h */
