//
//  U3PlatformLegacy.m
//  Ultima3
//
//  Legacy implementation of the portable platform boundary.
//

#import "U3Platform.h"

#import "CocoaBridge.h"
#import "UltimaIncludes.h"
#import "UltimaGraphics.h"
#import "UltimaMacIF.h"
#import "UltimaMain.h"
#import "UltimaMisc.h"

extern short Random(void);
extern void ObscureCursor(void);

static CFStringRef U3LegacyPreferenceNameForKey(U3PreferenceKey key) {
    switch (key) {
        case U3PreferenceSoundDisabled: return U3PrefSoundInactive;
        case U3PreferenceMusicDisabled: return U3PrefMusicInactive;
        case U3PreferenceSpeechDisabled: return U3PrefSpeechInactive;
        case U3PreferenceUnconstrainedSpeed: return U3PrefSpeedUnconstrain;
        case U3PreferenceOriginalSize: return U3PrefOriginalSize;
        case U3PreferenceFullScreen: return U3PrefFullScreen;
        case U3PreferenceClassicAppearance: return U3PrefClassicAppearance;
        case U3PreferenceIncludeWind: return U3PrefIncludeWind;
        case U3PreferenceNoDiagonals: return U3PrefNoDiagonals;
        case U3PreferenceAutoSave: return U3PrefAutoSave;
        case U3PreferenceManualCombat: return U3PrefManualCombat;
        case U3PreferenceNoAutoHeal: return U3PrefNoAutoHeal;
        case U3PreferenceAsyncSound: return U3PrefAsyncSound;
        case U3PreferenceDontAskDisplayMode: return U3PrefDontAskDisplayMode;
        case U3PreferenceNoEducateAboutFullScreen: return U3PrefNoEducateAboutFullScreen;
        case U3PreferenceFullScreenResolutionChange: return U3PrefFullScreenResChange;
        case U3PreferenceHealThreshold: return U3PrefHealThreshold;
        case U3PreferenceSoundVolume: return U3PrefSoundVolume;
        case U3PreferenceMusicVolume: return U3PrefMusicVolume;
        case U3PreferenceCurrentWindowX: return U3PrefCurWindowX;
        case U3PreferenceCurrentWindowY: return U3PrefCurWindowY;
        case U3PreferenceSaveWindowX: return U3PrefSaveWindowX;
        case U3PreferenceSaveWindowY: return U3PrefSaveWindowY;
        case U3PreferenceGameFont: return U3PrefGameFont;
        case U3PreferenceTileSet: return U3PrefTileSet;
    }
    return NULL;
}

bool U3PlatformPollInput(U3InputEvent *event, uint32_t timeoutTicks) {
    (void)event;
    return U3PlatformGetKeyMouse(timeoutTicks == 0 ? 2 : 1);
}

bool U3PlatformGetKeyMouse(uint8_t mode) {
    return GetKeyMouse(mode);
}

int16_t U3PlatformWaitKeyMouse(void) {
    return WaitKeyMouse();
}

char U3PlatformCursorKey(bool usePenLocation) {
    return CursorKey(usePenLocation);
}

void U3PlatformGetDirection(int16_t mode) {
    GetDirection(mode);
}

void U3PlatformFlushInputEvents(void) {
    FlushEvents(keyDownMask | keyUpMask, 0);
    FlushEvents(mDownMask | mUpMask, 0);
}

void U3PlatformFlushAllEvents(void) {
    FlushEvents(everyEvent, 0);
}

void U3PlatformWaitTicks(int32_t ticks) {
    ThreadSleepTicks(ticks);
}

uint32_t U3PlatformTickCount(void) {
    return (uint32_t)TickCount();
}

int16_t U3PlatformRandomRaw(void) {
    return Random();
}

uint16_t U3PlatformRandom(uint16_t low, uint16_t high) {
    return RandNum(low, high);
}

void U3PlatformObscureCursor(void) {
    ObscureCursor();
}

bool U3PlatformGetBooleanPreference(U3PreferenceKey key) {
    CFStringRef keyName = U3LegacyPreferenceNameForKey(key);
    return keyName && CFPreferencesGetAppBooleanValue(keyName, kCFPreferencesCurrentApplication, NULL);
}

int32_t U3PlatformGetIntegerPreference(U3PreferenceKey key) {
    CFStringRef keyName = U3LegacyPreferenceNameForKey(key);
    if (!keyName)
        return 0;
    return CFPreferencesGetAppIntegerValue(keyName, kCFPreferencesCurrentApplication, NULL);
}

bool U3PlatformHasPreference(U3PreferenceKey key) {
    CFStringRef keyName = U3LegacyPreferenceNameForKey(key);
    if (!keyName)
        return false;
    CFTypeRef value = CFPreferencesCopyAppValue(keyName, kCFPreferencesCurrentApplication);
    if (!value)
        return false;
    CFRelease(value);
    return true;
}

bool U3PlatformCopyPascalStringPreference(U3PreferenceKey key, uint8_t *outString, size_t outSize) {
    CFStringRef keyName = U3LegacyPreferenceNameForKey(key);
    if (!keyName || !outString || outSize == 0)
        return false;
    CFStringRef value = CFPreferencesCopyAppValue(keyName, kCFPreferencesCurrentApplication);
    if (!value)
        return false;
    Boolean success = CFStringGetPascalString(value, outString, outSize, kCFStringEncodingMacRoman);
    CFRelease(value);
    return success;
}

bool U3PlatformCopyUTF8StringPreference(U3PreferenceKey key, char *outString, size_t outSize) {
    CFStringRef keyName = U3LegacyPreferenceNameForKey(key);
    if (!keyName || !outString || outSize == 0)
        return false;
    CFStringRef value = CFPreferencesCopyAppValue(keyName, kCFPreferencesCurrentApplication);
    if (!value)
        return false;
    Boolean success = CFStringGetCString(value, outString, outSize, kCFStringEncodingUTF8);
    CFRelease(value);
    return success;
}

void U3PlatformSetBooleanPreference(U3PreferenceKey key, bool value) {
    CFStringRef keyName = U3LegacyPreferenceNameForKey(key);
    if (keyName)
        CFPreferencesSetAppValue(keyName, value ? kCFBooleanTrue : kCFBooleanFalse, kCFPreferencesCurrentApplication);
}

void U3PlatformSetIntegerPreference(U3PreferenceKey key, int32_t value) {
    CFStringRef keyName = U3LegacyPreferenceNameForKey(key);
    if (!keyName)
        return;
    CFNumberRef number = CFNumberCreate(NULL, kCFNumberSInt32Type, &value);
    CFPreferencesSetAppValue(keyName, number, kCFPreferencesCurrentApplication);
    CFRelease(number);
}

void U3PlatformRemovePreference(U3PreferenceKey key) {
    CFStringRef keyName = U3LegacyPreferenceNameForKey(key);
    if (keyName)
        CFPreferencesSetAppValue(keyName, NULL, kCFPreferencesCurrentApplication);
}

void U3PlatformSynchronizePreferences(void) {
    CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
}

bool U3PlatformShouldQuit(void) {
    return false;
}

void U3PlatformRequestQuit(void) {
}
