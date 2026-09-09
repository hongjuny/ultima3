//
//  U3Platform.h
//  Ultima3
//
//  Boundary for host platform services such as input, timing, random numbers,
//  preferences, and application lifecycle.
//

#ifndef U3Platform_h
#define U3Platform_h

#include "U3Types.h"

typedef enum U3PreferenceKey {
    U3PreferenceSoundDisabled = 0,
    U3PreferenceMusicDisabled,
    U3PreferenceSpeechDisabled,
    U3PreferenceUnconstrainedSpeed,
    U3PreferenceOriginalSize,
    U3PreferenceFullScreen,
    U3PreferenceClassicAppearance,
    U3PreferenceIncludeWind,
    U3PreferenceNoDiagonals,
    U3PreferenceAutoSave,
    U3PreferenceManualCombat,
    U3PreferenceNoAutoHeal,
    U3PreferenceAsyncSound,
    U3PreferenceDontAskDisplayMode,
    U3PreferenceNoEducateAboutFullScreen,
    U3PreferenceFullScreenResolutionChange,
    U3PreferenceHealThreshold,
    U3PreferenceSoundVolume,
    U3PreferenceMusicVolume,
    U3PreferenceCurrentWindowX,
    U3PreferenceCurrentWindowY,
    U3PreferenceSaveWindowX,
    U3PreferenceSaveWindowY,
    U3PreferenceGameFont,
    U3PreferenceTileSet
} U3PreferenceKey;

bool U3PlatformPollInput(U3InputEvent *event, uint32_t timeoutTicks);
bool U3PlatformGetKeyMouse(uint8_t mode);
int16_t U3PlatformWaitKeyMouse(void);
char U3PlatformCursorKey(bool usePenLocation);
void U3PlatformGetDirection(int16_t mode);
void U3PlatformFlushInputEvents(void);
void U3PlatformFlushAllEvents(void);
void U3PlatformWaitTicks(int32_t ticks);
uint32_t U3PlatformTickCount(void);
int16_t U3PlatformRandomRaw(void);
uint16_t U3PlatformRandom(uint16_t low, uint16_t high);
void U3PlatformObscureCursor(void);

bool U3PlatformGetBooleanPreference(U3PreferenceKey key);
int32_t U3PlatformGetIntegerPreference(U3PreferenceKey key);
bool U3PlatformHasPreference(U3PreferenceKey key);
bool U3PlatformCopyPascalStringPreference(U3PreferenceKey key, uint8_t *outString, size_t outSize);
bool U3PlatformCopyUTF8StringPreference(U3PreferenceKey key, char *outString, size_t outSize);
void U3PlatformSetBooleanPreference(U3PreferenceKey key, bool value);
void U3PlatformSetIntegerPreference(U3PreferenceKey key, int32_t value);
void U3PlatformRemovePreference(U3PreferenceKey key);
void U3PlatformSynchronizePreferences(void);

bool U3PlatformShouldQuit(void);
void U3PlatformRequestQuit(void);

#endif /* U3Platform_h */
