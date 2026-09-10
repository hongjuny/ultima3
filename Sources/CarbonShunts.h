//
//  CarbonShunts.h
//  Ultima3
//

#ifndef CarbonShunts_h
#define CarbonShunts_h

#ifndef blackColor
enum {
    whiteColor = 30,
    blackColor = 33,
    yellowColor = 69,
    magentaColor = 137,
    redColor = 205,
    cyanColor = 273,
    greenColor = 341,
    blueColor = 409
};
#endif

#ifndef srcOr
#define srcOr 1
#endif

#ifndef blend
#define blend 32
#endif

#ifndef addOver
#define addOver 34
#endif

#ifndef ditherCopy
#define ditherCopy 64
#endif

#ifndef watchCursor
#define watchCursor 4
#endif

typedef Handle CursHandle;

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

Boolean U3LegacyDrawImageURL(CFURLRef url, CGrafPtr port, const Rect *bounds,
                             int columns, int rows);
OSErr NewGWorld(GWorldPtr *world, short depth, const Rect *bounds, CTabHandle table,
               GDHandle device, GWorldFlags flags);
void SetRect(Rect *rect, short left, short top, short right, short bottom);
void OffsetRect(Rect *rect, short dh, short dv);
void InsetRect(Rect *rect, short dh, short dv);
void DisposeGWorld(GWorldPtr world);
PixMapHandle GetGWorldPixMap(GWorldPtr world);
Boolean LockPixels(PixMapHandle pixels);
void UnlockPixels(PixMapHandle pixels);
Ptr GetPixBaseAddr(PixMapHandle pixels);
void GetGWorld(CGrafPtr *port, GDHandle *device);
void SetGWorld(CGrafPtr port, GDHandle device);
CGrafPtr GetWindowPort(WindowRef window);
void CopyBits(const BitMap *source, const BitMap *destination, const Rect *sourceRect,
              const Rect *destinationRect, short mode, RgnHandle mask);
void CopyMask(const BitMap *source, const BitMap *mask, const BitMap *destination,
              const Rect *sourceRect, const Rect *maskRect, const Rect *destinationRect);

#endif /* CarbonShunts_h */
