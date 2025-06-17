//
//  CocoaBridge.h
//  Ultima3
//

void WrapCarbonWindowInCocoa(void *windowRef, short xposn, short yposn, short width, short height);
CFStringRef CopyExpireDateString(void);
CFStringRef CopyAppVersionString(void);
int ThisReleaseNumber(void);
void ThreadSleepTicks(int numTicks);
short NumberForPrefsKey(CFStringRef prefsKey);
CFURLRef GraphicsDirectoryURL(void);
CFURLRef ResourcesDirectoryURL(void);
CFArrayRef CopyGraphicsDirectoryItems(void);
CFStringRef CopyCatStrings(CFStringRef str1, CFStringRef str2);
void PlaySoundFileQT(CFStringRef soundName, Boolean async);
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
void *LWCreateMetalView(void *windowRef);
