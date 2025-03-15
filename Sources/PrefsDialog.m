//
//  PrefsDialog.m
//  Ultima3
//

#import <Cocoa/Cocoa.h>

#import "PrefsDialog.h"
#import "PrefsDialogController.h"
#import "CarbonShunts.h"
#import "CocoaBridge.h"
#import "LWIntegerTransformer.h"
#import "UltimaMain.h"
#import "UltimaGraphics.h"
#import "UltimaIncludes.h"
#import "UltimaMacIF.h"
#import "UltimaSound.h"
#import "UltimaText.h"

extern WindowPtr gMainWindow;
extern short gUpdateWhere, gMouseState;
extern Boolean gAutoCombat;
extern unsigned char Party[64];
extern unsigned char gCurFrame;

@implementation PrefsDialogController

+ (void)initialize {
    [NSValueTransformer setValueTransformer:[[[LWIntegerTransformer alloc] init] autorelease] forName:@"Integer"];
}

- (void)awakeFromNib {
    // Set up Fonts menu
    int selectedIndex = -1;
    NSString *currentFontName = [[NSUserDefaults standardUserDefaults] objectForKey:(NSString *)U3PrefGameFont];
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
    NSString *currentThemeName = [[NSUserDefaults standardUserDefaults] objectForKey:(NSString *)U3PrefTileSet];
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
    ApplyVolumePreferences();
}

- (IBAction)useClassicDefaults:(id)sender {
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:(NSString *)U3PrefIncludeWind];
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:(NSString *)U3PrefNoDiagonals];
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:(NSString *)U3PrefManualCombat];
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:(NSString *)U3PrefNoAutoHeal];
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:(NSString *)U3PrefAutoSave];
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:(NSString *)U3PrefAsyncSound];
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:(NSString *)U3PrefClassicAppearance];
    //[[NSUserDefaults standardUserDefaults] removeObjectForKey:(NSString *)U3PrefGameFont];
    //[mFontsButton selectItemAtIndex:0];
}

- (IBAction)useModernDefaults:(id)sender {
    [[NSUserDefaults standardUserDefaults] setInteger:750 forKey:(NSString *)U3PrefHealThreshold];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:(NSString *)U3PrefIncludeWind];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:(NSString *)U3PrefNoDiagonals];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:(NSString *)U3PrefManualCombat];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:(NSString *)U3PrefNoAutoHeal];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:(NSString *)U3PrefAutoSave];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:(NSString *)U3PrefAsyncSound];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:(NSString *)U3PrefClassicAppearance];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:(NSString *)U3PrefGameFont];
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
    NSString *orgTheme = [[[[NSUserDefaults standardUserDefaults] objectForKey:(NSString *)U3PrefTileSet] retain] autorelease];
    if ([[NSUserDefaults standardUserDefaults] integerForKey:(NSString *)U3PrefSoundVolume] < 1)
        [[NSUserDefaults standardUserDefaults] setInteger:100 forKey:(NSString *)U3PrefSoundVolume];
    if ([[NSUserDefaults standardUserDefaults] integerForKey:(NSString *)U3PrefMusicVolume] < 1)
        [[NSUserDefaults standardUserDefaults] setInteger:100 forKey:(NSString *)U3PrefMusicVolume];
    int result = RunCocoaDialog(CFSTR("GameOptions"), (CFMutableDictionaryRef)valuesDict, CFSTR("PrefsDialogController"));
    if (result == 1) {
        ReflectPrefs();
        NSString *newTheme = [[NSUserDefaults standardUserDefaults] objectForKey:(NSString *)U3PrefTileSet];
        if ((!newTheme && orgTheme) || (newTheme && !orgTheme) || ![orgTheme isEqual:newTheme])
            GetGraphics();
        SetNewFont(true);
        if (Party[3] == 0x80)
            gAutoCombat = ![[NSUserDefaults standardUserDefaults] boolForKey:(NSString *)U3PrefManualCombat];
        ApplyVolumePreferences();
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
