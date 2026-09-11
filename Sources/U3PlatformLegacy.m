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
#import "UltimaNew.h"

extern short Random(void);
extern void ObscureCursor(void);
extern char gKeyPress;
extern short gMouseKey;
extern short gCurMouseDir;
extern short gMouseState;
extern Boolean gDone;
extern void CursorUpdate(void);
extern short gUpdateWhere;
extern short blkSiz;
extern void SaveWideArea(void);
extern EventRecord gTheEvent;

extern unsigned char Player[21][65], Macro[32];

static char U3MainMenuButtonKey(Point point) {
    short buttons[] = {0, 1, 8, 2};
    char keys[] = {'R', 'O', 'A', 'J'};
    if (gUpdateWhere == 6) {
        int count = 0;
        Boolean formed = FALSE;
        for (int i = 1; i <= 20; ++i) {
            if (Player[i][0]) ++count;
            if (Player[i][16]) formed = TRUE;
        }
        buttons[0] = 3; buttons[1] = 4;
        buttons[2] = 5 + formed; buttons[3] = 7;
        keys[0] = count < 20 ? 'C' : 0;
        keys[1] = count ? 'T' : 0;
        keys[2] = count ? (formed ? 'D' : 'F') : 0;
        keys[3] = 27;
    } else if (gUpdateWhere != 5) {
        return 0;
    }
    for (int i = 0; i < 4; ++i) {
        Rect bounds;
        U3ButtonBounds(&bounds, buttons[i]);
        if (PtInRect(point, &bounds))
            return keys[i];
    }
    return 0;
}

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
    if (event) {
        event->command = U3CommandNone;
        event->direction = U3DirectionNone;
        event->rawKey = 0;
        event->isMouse = false;
    }
    char key = 0;
    Boolean isMouse = false;
    uint8_t mode = timeoutTicks == 0 ? 2 : 1;
    if (U3CocoaPollKeyMouse(true, mode == 2 ? 0 : 5, &key, &isMouse)) {
        gKeyPress = key;
        if (event) {
            event->rawKey = (uint8_t)key;
            event->isMouse = isMouse;
        }
        return true;
    }
    if (U3CocoaHasMainSurface())
        return false;
    bool result = U3PlatformGetKeyMouse(mode);
    if (result && event)
        event->rawKey = (uint8_t)gKeyPress;
    return result;
}

bool U3PlatformGetKeyMouse(uint8_t mode) {
    char key = 0;
    Boolean isMouse = false;
    gMouseKey = false;
    gKeyPress = 0;
    if (gDone) return false;
    if (Macro[0]) {
        gKeyPress = Macro[0];
        DecMacro();
        gMouseKey = true;
        return true;
    }
    if (U3CocoaPollKeyMouse(true, mode == 2 ? 0 : 5, &key, &isMouse)) {
        gKeyPress = key;
        gMouseKey = isMouse;
        if (isMouse) {
            Point mouse;
            U3CocoaGetMousePoint(&mouse);
            if (gMouseState == 4) {
                for (int i = 0; i < 4; ++i) {
                    if (mouse.h >= blkSiz * 24 && mouse.h < blkSiz * 39 &&
                        mouse.v >= blkSiz * (1 + i * 4) && mouse.v < blkSiz * (4 + i * 4)) {
                        gKeyPress = '1' + i;
                        return true;
                    }
                }
                return false;
            }
            char menuKey = U3MainMenuButtonKey(mouse);
            if (menuKey) {
                gKeyPress = menuKey;
                gMouseKey = false;
                return true;
            }
            if (gUpdateWhere == 5 || gUpdateWhere == 6)
                return false;
            gCurMouseDir = 0;
            CursorUpdate();
            if (gCurMouseDir)
                gKeyPress = (char)gCurMouseDir;
        }
        return gKeyPress != 0 || (isMouse &&
            (gUpdateWhere == 1 || gUpdateWhere == 2 || gUpdateWhere == 7));
    }
    if (U3CocoaHasMainSurface())
        return false;
    return GetKeyMouse(mode);
}

int16_t U3PlatformWaitKeyMouse(void) {
    if (gUpdateWhere == 7)
        SaveWideArea();
    while (!gDone && !U3PlatformGetKeyMouse(1)) {
    }

    return gDone ? 0 : gKeyPress;
}

char U3PlatformCursorKey(bool usePenLocation) {
    return CursorKey(usePenLocation);
}

void U3PlatformGetDirection(int16_t mode) {
    GetDirection(mode);
}

void U3PlatformFlushInputEvents(void) {
    U3CocoaFlushInput();
}

void U3PlatformFlushAllEvents(void) {
    U3CocoaFlushInput();
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
