//
//  CocoaBridge.h
//  Ultima3
//

#include "U3Bitmap.h"
#include "U3Types.h"
Boolean U3CocoaCreateCharacter(const Boolean available[20], short *slot, U3CharacterDraft *draft);
void U3CocoaInstallMenus(void);
Boolean U3CocoaChooseParty(const unsigned char names[20][16], const Boolean available[20], short selection[4]);
void U3CocoaQueueDiagnosticKey(char key);
void U3CocoaQueueDiagnosticMouse(short x, short y);
Boolean U3CocoaWriteMainBitmap(const char *path);
U3Bitmap *U3CocoaMainBitmap(void);
Boolean U3CocoaResizeMainBitmap(short width, short height);
void U3CocoaInvalidateMainSurface(void);
Boolean U3CocoaLoadImage(U3Bitmap *output, CFURLRef url, int width, int height,
                        int columns, int rows);
Boolean U3CocoaImageSelfTest(void);
void U3CocoaSelectBitmap(U3Bitmap *bitmap, short originX, short originY);

void U3CocoaDrawBitmap(const U3Bitmap *bitmap, U3BitmapRect source,
                       short x, short y, short width, short height);
void WrapCarbonWindowInCocoa(void *windowRef, short xposn, short yposn, short width, short height);
void *U3CocoaCreateMainSurface(short xposn, short yposn, short width, short height);
void U3CocoaPumpEvents(void);
Boolean U3CocoaHasMainSurface(void);
Boolean U3CocoaUsesNativeUI(void);
void U3CocoaRunApplication(void);
Boolean U3CocoaPollKeyMouse(Boolean includeMouse, long timeoutTicks, char *outKey,
                            Boolean *outMouse);
void U3CocoaGetMousePoint(Point *point);
void U3CocoaSetForegroundQuickDrawColor(long color);
void U3CocoaSetBackgroundQuickDrawColor(long color);
void U3CocoaSetForegroundRGB(UInt16 red, UInt16 green, UInt16 blue);
void U3CocoaSetBackgroundRGB(UInt16 red, UInt16 green, UInt16 blue);
void U3CocoaSetTextFont(short font);
void U3CocoaSetTextSize(short size);
void U3CocoaSetTextFace(short face);
void U3CocoaMoveTo(short h, short v);
void U3CocoaPaintRect(short left, short top, short right, short bottom);
void U3CocoaEraseRect(short left, short top, short right, short bottom);
void U3CocoaFrameRect(short left, short top, short right, short bottom);
void U3CocoaDrawPascalString(ConstStr255Param text);
short U3CocoaTextWidth(ConstStr255Param text);
void U3CocoaDrawBytes(const void *textBuf, short firstByte, short byteCount);
CFStringRef CopyExpireDateString(void);
CFStringRef CopyAppVersionString(void);
int ThisReleaseNumber(void);
void ThreadSleepTicks(int numTicks);
CFURLRef GraphicsDirectoryURL(void);
CFURLRef ResourcesDirectoryURL(void);
CFArrayRef CopyGraphicsDirectoryItems(void);
CFStringRef CopyCatStrings(CFStringRef str1, CFStringRef str2);
void PlaySoundFileQT(CFStringRef soundName, Boolean async);
Boolean U3CocoaIsHeadlessDiagnostic(void);
void SetSoundVolumePercent(short newVolume);
Boolean ShouldNotifyUser(void);
Boolean GetSystemVersion(unsigned *majorVersion, unsigned *minorVersion, unsigned *bugFixVersion);
CFArrayRef StringsArray(CFStringRef identifier);
void GetPascalStringFromArrayByIndex(StringPtr pstringPtr, CFStringRef identifier, int index);
int EducateAboutFullScreen(void);
Boolean SetCursorNamed(CFStringRef cursorName, float scale);
int RunCocoaDialog(CFStringRef nibName, CFMutableDictionaryRef valuesDict, CFStringRef controllerClassName);
void LWOpenURL(CFStringRef urlString);
void SetRefMenuIcons(MenuRef theMenu);
