//
//  PrefsDialog.m
//  Ultima3
//

#import <Cocoa/Cocoa.h>

#import "PrefsDialog.h"
#import "PrefsDialogController.h"
#import "CarbonShunts.h"
#import "CocoaBridge.h"
#import "U3Audio.h"
#import "U3Platform.h"
#import "LWIntegerTransformer.h"
#import "UltimaMain.h"
#import "UltimaGraphics.h"
#import "UltimaIncludes.h"
#import "UltimaMacIF.h"
#import "UltimaText.h"

extern WindowPtr gMainWindow;
extern short gUpdateWhere, gMouseState;
extern Boolean gAutoCombat;
extern unsigned char Party[64];
extern unsigned char gCurFrame;

static NSString *U3PreferenceStringValue(U3PreferenceKey key) {
    char value[1024];
    if (!U3PlatformCopyUTF8StringPreference(key, value, sizeof(value)))
        return nil;
    return [NSString stringWithUTF8String:value];
}

@implementation PrefsDialogController

+ (void)initialize {
    [NSValueTransformer setValueTransformer:[[[LWIntegerTransformer alloc] init] autorelease] forName:@"Integer"];
}

- (void)awakeFromNib {
    // Set up Fonts menu
    int selectedIndex = -1;
    NSString *currentFontName = U3PreferenceStringValue(U3PreferenceGameFont);
    while ([mFontsButton numberOfItems] > 2) {
        [mFontsButton removeItemAtIndex:2];
    }
    NSArray *fontNames = [[NSFontManager sharedFontManager] availableFontFamilies];
    int i = 0;
    for (i = 0; i < [fontNames count]; i++) {
        NSString *aFontName = [fontNames objectAtIndex:i];
        [mFontsButton addItemWithTitle:aFontName];
        if (![[NSUserDefaults standardUserDefaults] boolForKey:@"plainFontsMenu"]) {
            NSFont *asFont = [NSFont fontWithName:aFontName size:12];
            if (asFont) {
                NSMenuItem *theItem = [mFontsButton lastItem];
                NSAttributedString *attrStr = [[[NSAttributedString alloc]
                    initWithString:aFontName
                        attributes:[NSDictionary dictionaryWithObject:asFont forKey:NSFontAttributeName]] autorelease];
                if (attrStr)
                    [theItem setAttributedTitle:attrStr];
            }
        }

        if (currentFontName && [aFontName isEqual:currentFontName])
            selectedIndex = [mFontsButton numberOfItems] - 1;
    }
    if (selectedIndex > 0)
        [mFontsButton selectItemAtIndex:selectedIndex];
    else
        [mFontsButton selectItemAtIndex:0];

    // Set up Themes menu
    selectedIndex = -1;
    NSString *currentThemeName = U3PreferenceStringValue(U3PreferenceTileSet);
    [mThemesButton removeAllItems];
    NSArray *graphicsArray = [(NSArray *)CopyGraphicsDirectoryItems() autorelease];
    NSMutableArray *themeNames = [NSMutableArray new];
    for (i = 0; i < [graphicsArray count]; i++) {
        NSString *aGraphicFile = [graphicsArray objectAtIndex:i];
        NSRange tileRange = [aGraphicFile rangeOfString:@"-Tiles."];
        if (tileRange.location != NSNotFound) {
            NSString *themeName = [aGraphicFile substringToIndex:tileRange.location];
            [themeNames addObject:themeName];
        }
    }
    for (NSString *aThemeName in [themeNames sortedArrayUsingSelector:@selector(compare:)]) {
        [mThemesButton addItemWithTitle:aThemeName];
        if (currentThemeName && [aThemeName isEqual:currentThemeName]) {
            selectedIndex = [mThemesButton numberOfItems] - 1;
        }
    }
    if (selectedIndex > 0) {
        [mThemesButton selectItemAtIndex:selectedIndex];
    } else {
        [mThemesButton selectItemAtIndex:0];
    }

    [mHealThresholdField selectText:nil];

    [[NSUserDefaults standardUserDefaults] addObserver:self forKeyPath:(NSString *)U3PrefMusicVolume options:0 context:NULL];
    [[NSUserDefaults standardUserDefaults] addObserver:self forKeyPath:(NSString *)U3PrefMusicInactive options:0 context:NULL];
}

- (IBAction)terminateWithTagAsCode:(id)sender {
    [[NSUserDefaults standardUserDefaults] removeObserver:self forKeyPath:(NSString *)U3PrefMusicVolume];
    [[NSUserDefaults standardUserDefaults] removeObserver:self forKeyPath:(NSString *)U3PrefMusicInactive];
    return [super terminateWithTagAsCode:sender];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
    U3AudioApplyPreferences();
}

- (IBAction)useClassicDefaults:(id)sender {
    U3PlatformSetBooleanPreference(U3PreferenceIncludeWind, true);
    U3PlatformSetBooleanPreference(U3PreferenceNoDiagonals, true);
    U3PlatformSetBooleanPreference(U3PreferenceManualCombat, true);
    U3PlatformSetBooleanPreference(U3PreferenceNoAutoHeal, true);
    U3PlatformSetBooleanPreference(U3PreferenceAutoSave, true);
    U3PlatformSetBooleanPreference(U3PreferenceAsyncSound, true);
    U3PlatformSetBooleanPreference(U3PreferenceClassicAppearance, true);
    //[mFontsButton selectItemAtIndex:0];
}

- (IBAction)useModernDefaults:(id)sender {
    U3PlatformSetIntegerPreference(U3PreferenceHealThreshold, 750);
    U3PlatformRemovePreference(U3PreferenceIncludeWind);
    U3PlatformRemovePreference(U3PreferenceNoDiagonals);
    U3PlatformRemovePreference(U3PreferenceManualCombat);
    U3PlatformRemovePreference(U3PreferenceNoAutoHeal);
    U3PlatformRemovePreference(U3PreferenceAutoSave);
    U3PlatformRemovePreference(U3PreferenceAsyncSound);
    U3PlatformRemovePreference(U3PreferenceClassicAppearance);
    U3PlatformRemovePreference(U3PreferenceGameFont);
    [mFontsButton selectItemAtIndex:0];
}

@end

// ______________________________________________________________________________________________________________
#pragma mark -

void GameOptionsDialog(void) {
    short mouseStateStore = gMouseState;
    gMouseState = 0;
    CursorUpdate();
    InitCursor();

    NSAutoreleasePool *myPool = [[NSAutoreleasePool alloc] init];
    NSMutableDictionary *valuesDict = [NSMutableDictionary dictionary];
    NSString *orgTheme = [[U3PreferenceStringValue(U3PreferenceTileSet) retain] autorelease];
    if (U3PlatformGetIntegerPreference(U3PreferenceSoundVolume) < 1)
        U3PlatformSetIntegerPreference(U3PreferenceSoundVolume, 100);
    if (U3PlatformGetIntegerPreference(U3PreferenceMusicVolume) < 1)
        U3PlatformSetIntegerPreference(U3PreferenceMusicVolume, 100);
    int result = RunCocoaDialog(CFSTR("GameOptions"), (CFMutableDictionaryRef)valuesDict, CFSTR("PrefsDialogController"));
    if (result == 1) {
        ReflectPrefs();
        NSString *newTheme = U3PreferenceStringValue(U3PreferenceTileSet);
        if ((!newTheme && orgTheme) || (newTheme && !orgTheme) || ![orgTheme isEqual:newTheme])
            GetGraphics();
        SetNewFont(true);
        if (Party[3] == 0x80)
            gAutoCombat = !U3PlatformGetBooleanPreference(U3PreferenceManualCombat);
        U3AudioApplyPreferences();
    }
    [myPool release];

    DrawFrame(gCurFrame);
    if (gUpdateWhere == 3 || gUpdateWhere == 4)
        ShowChars(true);
    Rect myRect;
    LWGetWindowBounds(gMainWindow, &myRect);
    LWInvalWindowRect(gMainWindow, &myRect);
    gMouseState = mouseStateStore;
}
