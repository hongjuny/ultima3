//
//  CarbonShunts.h
//  Ultima3
//

#ifndef CarbonShunts_h
#define CarbonShunts_h

#ifdef __APPLE__
#include <Carbon/Carbon.h>
#ifndef SetRect
static inline void SetRect(Rect *r, short l, short t, short rgt, short btm) {
    r->left = l; r->top = t; r->right = rgt; r->bottom = btm;
}
#endif
/* QuickDraw is removed from modern SDKs. Provide simple stubs when the
   declarations are missing so legacy source continues to compile. */
#ifndef OffsetRect
static inline void OffsetRect(Rect *r, short dh, short dv) {
    r->left += dh; r->right += dh; r->top += dv; r->bottom += dv;
}
#endif
#ifndef SetGWorld
static inline void SetGWorld(CGrafPtr port, GDHandle dev) {
    (void)port; (void)dev;
}
#endif
#ifndef CopyBits
static inline void CopyBits(const BitMap *srcBits, const BitMap *dstBits,
                            const Rect *srcRect, const Rect *dstRect,
                            short mode, RgnHandle maskRgn) {
    (void)srcBits; (void)dstBits; (void)srcRect; (void)dstRect;
    (void)mode; (void)maskRgn;
}
#endif
#ifndef CopyMask
static inline void CopyMask(const BitMap *srcBits, const BitMap *maskBits,
                            const BitMap *dstBits, const Rect *srcRect,
                            const Rect *maskRect, const Rect *dstRect) {
    (void)srcBits; (void)maskBits; (void)dstBits; (void)srcRect;
    (void)maskRect; (void)dstRect;
}
#endif
#ifndef OpColor
static inline void OpColor(const RGBColor *color) { (void)color; }
#endif
#ifndef PenMode
static inline void PenMode(short mode) { (void)mode; }
#endif
#ifndef ForeColor
static inline void ForeColor(short c) { (void)c; }
#endif
#ifndef BackColor
static inline void BackColor(short c) { (void)c; }
#endif
#ifndef PaintRect
static inline void PaintRect(const Rect *r) { (void)r; }
#endif
#ifndef TextFont
static inline void TextFont(short font) { (void)font; }
#endif
#ifndef TextSize
static inline void TextSize(short size) { (void)size; }
#endif
#ifndef TextMode
static inline void TextMode(short mode) { (void)mode; }
#endif
#ifndef TextFace
static inline void TextFace(short face) { (void)face; }
#endif
#ifndef StringWidth
#include <string.h>
static inline short StringWidth(const unsigned char *str) {
    return str ? (short)(str[0] * 6) : 0;
}
#endif
#ifndef MoveTo
static inline void MoveTo(short h, short v) { (void)h; (void)v; }
#endif
#ifndef LineTo
static inline void LineTo(short h, short v) { (void)h; (void)v; }
#endif
#ifndef DrawString
static inline void DrawString(const unsigned char *str) { (void)str; }
#endif
#ifndef RGBForeColor
static inline void RGBForeColor(const RGBColor *color) { (void)color; }
#endif
#if defined(__LP64__)
#ifndef LWPolyHandle
#include <stddef.h> /* for NULL */
typedef void *LWPolyHandle;
static inline LWPolyHandle LWOpenPoly(void) { return NULL; }
static inline void LWClosePoly(void) {}
static inline void LWPaintPoly(LWPolyHandle poly) { (void)poly; }
static inline void LWKillPoly(LWPolyHandle poly) { (void)poly; }
#ifndef OpenPoly
#define OpenPoly  LWOpenPoly
#define ClosePoly LWClosePoly
#define PaintPoly LWPaintPoly
#define KillPoly  LWKillPoly
#endif
#endif /* LWPolyHandle */
#endif /* __LP64__ */
#ifndef blackColor
#define blackColor   33
#define whiteColor   30
#define redColor     205
#define greenColor   341
#define blueColor    409
#define cyanColor    273
#define magentaColor 137
#define yellowColor  69
#endif
#ifndef srcCopy
#define srcCopy     0
#define srcOr       1
#define blend       32
#define ditherCopy  64
#define addOver     34
#endif
/* NewGWorld and related calls were removed from modern macOS SDKs.  Provide
   simple stubs so the legacy graphics code continues to compile. */
#ifndef NewGWorld
static inline OSErr NewGWorld(CGrafPtr *port, short depth, const Rect *rect,
                              void *ctab, GDHandle device, unsigned int flags)
{
    (void)port; (void)depth; (void)rect; (void)ctab; (void)device; (void)flags;
    return noErr;
}
#endif
#ifndef DisposeGWorld
static inline void DisposeGWorld(CGrafPtr port) { (void)port; }
#endif
#ifndef LockPixels
static inline OSErr LockPixels(PixMapHandle pm) { (void)pm; return noErr; }
#endif
#ifndef UnlockPixels
static inline void UnlockPixels(PixMapHandle pm) { (void)pm; }
#endif
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
static inline void ForeColor(short c) { (void)c; }
static inline void BackColor(short c) { (void)c; }
static inline void PaintRect(const Rect *r) { (void)r; }
#define bold   1
#define italic 2
static inline void TextFont(short font) { (void)font; }
static inline void TextSize(short size) { (void)size; }
static inline void TextMode(short mode) { (void)mode; }
static inline void TextFace(short face) { (void)face; }
static inline short StringWidth(const unsigned char *str) { return str ? (short)(str[0] * 6) : 0; }
static inline void MoveTo(short h, short v) { (void)h; (void)v; }
static inline void LineTo(short h, short v) { (void)h; (void)v; }
static inline void DrawString(const unsigned char *str) { (void)str; }
static inline void RGBForeColor(const RGBColor *color) { (void)color; }
typedef void *LWPolyHandle;
#include <stddef.h> /* for NULL */
static inline LWPolyHandle LWOpenPoly(void) { return NULL; }
static inline void LWClosePoly(void) {}
static inline void LWPaintPoly(LWPolyHandle poly) { (void)poly; }
static inline void LWKillPoly(LWPolyHandle poly) { (void)poly; }
#ifndef OpenPoly
#define OpenPoly  LWOpenPoly
#define ClosePoly LWClosePoly
#define PaintPoly LWPaintPoly
#define KillPoly  LWKillPoly
#endif
static inline OSErr NewGWorld(CGrafPtr *port, short depth, const Rect *rect, void *ctab, GDHandle device, unsigned int flags) { (void)port; (void)depth; (void)rect; (void)ctab; (void)device; (void)flags; return noErr; }
static inline void DisposeGWorld(CGrafPtr port) { (void)port; }
static inline OSErr LockPixels(PixMapHandle pm) { (void)pm; return noErr; }
static inline void UnlockPixels(PixMapHandle pm) { (void)pm; }
#define blackColor   33
#define whiteColor   30
#define redColor     205
#define greenColor   341
#define blueColor    409
#define cyanColor    273
#define magentaColor 137
#define yellowColor  69
#define srcCopy     0
#define srcOr       1
#define blend       32
#define ditherCopy  64
#define addOver     34
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

/* Additional stubs for legacy APIs missing on non-Carbon builds. These
   implementations simply satisfy the compiler and do not provide real
   QuickDraw behaviour. */
#ifndef CursHandle
typedef void *CursHandle;
#endif

#ifndef PicHandle
typedef void *PicHandle;
#endif

#ifndef Random
#include <stdlib.h>
static inline long Random(void) { return rand(); }
#endif

#ifndef GetPort
static inline void GetPort(CGrafPtr *port) { if (port) *port = NULL; }
#endif
#ifndef SetPort
static inline void SetPort(CGrafPtr port) { (void)port; }
#endif
#ifndef NewRgn
static inline RgnHandle NewRgn(void) { return NULL; }
#endif
#ifndef DisposeRgn
static inline void DisposeRgn(RgnHandle rgn) { (void)rgn; }
#endif
#ifndef GetClip
static inline void GetClip(RgnHandle rgn) { (void)rgn; }
#endif
#ifndef SetClip
static inline void SetClip(RgnHandle rgn) { (void)rgn; }
#endif
#ifndef ClipRect
static inline void ClipRect(const Rect *r) { (void)r; }
#endif
#ifndef GetPicture
static inline PicHandle GetPicture(short id) { (void)id; return NULL; }
#endif
#ifndef DrawPicture
static inline void DrawPicture(PicHandle pic, const Rect *r) { (void)pic; (void)r; }
#endif
#ifndef ReleaseResource
static inline void ReleaseResource(Handle h) { (void)h; }
#endif
#ifndef GetCursor
static inline CursHandle GetCursor(short id) { (void)id; return NULL; }
#endif
#ifndef SetCursor
static inline void SetCursor(const void *c) { (void)c; }
#endif

#endif /* CarbonShunts_h */
