//
//  CocoaBridge.m
//  Ultima3
//

#import "CocoaBridge.h"

#import "LWCocoaDialogController.h"
#import "UltimaIncludes.h"

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import <AVFoundation/AVFoundation.h>

extern short gUpdateWhere;

static float sSoundVolume = 1.0f;
static NSMutableDictionary *sSoundPlayers = nil;

@implementation LWCocoaDialogController

- (NSObjectController *)controller {
    return mController;
}

- (NSWindow *)window {
    return mWindow;
}

- (IBAction)terminateWithTagAsCode:(id)sender {
    int result = 1;
    if ([sender respondsToSelector:@selector(tag)])
        result = (int)[sender performSelector:@selector(tag)];
    [NSApp stopModalWithCode:result];
}

- (void)validateChangedValueForKey:(NSString *)keyPath {
    // No implementation, for subclasses to react to
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
    static BOOL isReacting = false;
    if (!isReacting) {
        isReacting = true;
        if ([keyPath hasPrefix:@"content."])
            keyPath = [keyPath substringFromIndex:8];
        [self validateChangedValueForKey:keyPath];
        isReacting = false;
    }
}

@end

// __________________________________________________________________________________________
#pragma mark -

@interface LWWindowManager : NSObject <NSWindowDelegate> {
    NSMutableDictionary *mCocoaWindowsByRef;
}

@end

// __________________________________________________________________________________________
#pragma mark -

@implementation LWWindowManager

- (id)init {
    if ((self = [super init]) != nil) {
        mCocoaWindowsByRef = [[NSMutableDictionary alloc] init];
    }
    return self;
}

- (void)dealloc {
    [mCocoaWindowsByRef release];
    [super dealloc];
}

+ (LWWindowManager *)sharedInstance {
    static LWWindowManager *sSharedInstance = nil;
    if (!sSharedInstance)
        sSharedInstance = [[LWWindowManager alloc] init];
    return sSharedInstance;
}

+ (NSString *)keyForWindowRef:(void *)windowRef {
    return [NSString stringWithFormat:@"%p", windowRef];
}

- (void)addCocoaParentWindow:(NSWindow *)theWindow forCarbonChildWindowRef:(void *)windowRef {
    [mCocoaWindowsByRef setObject:theWindow forKey:[LWWindowManager keyForWindowRef:windowRef]];
    [theWindow setDelegate:self];
}

- (WindowRef)windowRefOfWindow:(NSWindow *)theWindow {
    if ([[theWindow childWindows] count])
        return (WindowRef)[[[theWindow childWindows] objectAtIndex:0] windowRef];
    return nil;
}

- (NSWindow *)windowForWindowRef:(void *)windowRef {
    return [mCocoaWindowsByRef objectForKey:[LWWindowManager keyForWindowRef:windowRef]];
}

- (void)windowDidResize:(NSNotification *)notification {
    NSWindow *theWindow = [notification object];
    WindowRef winRef = [self windowRefOfWindow:theWindow];
    if (winRef) {
        float titleBarHeight = [theWindow frame].size.height - [[theWindow contentView] bounds].size.height;
        NSScreen *relevantScreen = [theWindow screen];
        short newCarbonOriginX = [theWindow frame].origin.x;
        short newCarbonOriginY = [relevantScreen frame].size.height - NSMaxY([theWindow frame]) + titleBarHeight;
        MoveWindow(winRef, newCarbonOriginX, newCarbonOriginY, false);
        short width = [theWindow frame].size.width;
        short height = [theWindow frame].size.height - titleBarHeight;
        SizeWindow(winRef, width, height, true);
        QDFlushPortBuffer(GetWindowPort(winRef), NULL);
    }
}

//- (void)windowDidBecomeMain:(NSNotification *)notification {
//    NSLog(@"windowDidBecomeMain %p", [notification object]);
//    NSWindow *theWindow = [notification object];
//    WindowRef winRef = [self windowRefOfWindow:theWindow];
//}

//- (void)windowDidResignMain:(NSNotification *)notification {
//    NSLog(@"windowDidResignMain %p", [notification object]);
//    NSWindow *theWindow = [notification object];
//    WindowRef winRef = [self windowRefOfWindow:theWindow];
//}

- (BOOL)windowShouldClose:(NSWindow *)theWindow {
    BOOL result = false;
    WindowRef winRef = [self windowRefOfWindow:theWindow];
    // trigger stuff where it asks to save, etc.
    if (result) {
        [theWindow setDelegate:nil];
        [mCocoaWindowsByRef removeObjectForKey:[LWWindowManager keyForWindowRef:winRef]];
    }
    return result;
}

- (void)updateFurnitureForWindowRef:(void *)windowRef {
    NSWindow *theWindow = [self windowForWindowRef:windowRef];
    if (theWindow) {
        [theWindow setDocumentEdited:IsWindowModified((WindowRef)windowRef)];

        FSSpec winProxySpec = {0};
        if (noErr == GetWindowProxyFSSpec((WindowRef)windowRef, &winProxySpec)) {
            FSRef asFSRef;
            if (noErr == FSpMakeFSRef(&winProxySpec, &asFSRef)) {
                CFURLRef asURL = CFURLCreateFromFSRef(NULL, &asFSRef);
                if (asURL) {
                    [theWindow setRepresentedURL:(NSURL *)asURL];
                    CFRelease(asURL);
                }
            }
        }

        CFStringRef windowTitle = NULL;
        if (noErr == CopyWindowTitleAsCFString((WindowRef)windowRef, &windowTitle)) {
            [theWindow setTitle:(NSString *)windowTitle];
            CFRelease(windowTitle);
        }
    }
}

- (void)makeWindowRefActive:(void *)windowRef {
    NSWindow *theWindow = [self windowForWindowRef:windowRef];
    if (theWindow) {
        [theWindow makeKeyAndOrderFront:nil];
        //WindowRef winRef = [self windowRefOfWindow:theWindow];
    }
}

- (Boolean)isWindowRefActive:(void *)windowRef {
    Boolean result = false;
    NSWindow *theWindow = [self windowForWindowRef:windowRef];
    if (theWindow)
        result = [theWindow isMainWindow];
    else
        result = IsWindowActive((WindowRef)windowRef);

    return result;
}

@end

// __________________________________________________________________________________________
#pragma mark -

void CocoaInit(void) {
    static BOOL sDidInit = false;
    if (!sDidInit) {
        sDidInit = true;
        NSAutoreleasePool *myPool = [[NSAutoreleasePool alloc] init];
        NSApplicationLoad();
        [[[NSWindow alloc] init] release];
        [myPool release];
    }
}

void WrapCarbonWindowInCocoa(void *windowRef, short xposn, short yposn, short width, short height) {
    CocoaInit();
    NSAutoreleasePool *myPool = [[NSAutoreleasePool alloc] init];
    @try {
        NSWindow *carbonFramelessWindow = [[NSWindow alloc] initWithWindowRef:windowRef];
        [carbonFramelessWindow setHasShadow:NO];

        NSRect cocoaWinRect;
        cocoaWinRect.origin.x = (float)xposn;
#warning this is not a permanent solution, will not work with multiple screens.
        cocoaWinRect.origin.y = [[NSScreen mainScreen] frame].size.height - (yposn + height);
        cocoaWinRect.size.width = (float)width;
        cocoaWinRect.size.height = (float)height;

        //2012-11-08 15:55:02.634 Ultima III[31439:f0b] *** Terminating app due to uncaught exception 'NSInternalInconsistencyException', reason: 'Error (1000) creating CGSWindow on line 259'

        NSLog(@"Creating NSWindow");
        NSWindow *cocoaWindow = [[NSWindow alloc]
            initWithContentRect:cocoaWinRect
                      styleMask:NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask
                        backing:NSBackingStoreBuffered
                          defer:YES];
        [[LWWindowManager sharedInstance] addCocoaParentWindow:cocoaWindow forCarbonChildWindowRef:windowRef];
        [cocoaWindow addChildWindow:carbonFramelessWindow ordered:NSWindowAbove];
        [cocoaWindow makeKeyAndOrderFront:nil];

    } @catch (NSException *exception) {
        NSLog(@"WrapCarbonWindowInCocoa %@", exception);
    }
    [myPool release];
}

CFStringRef CopyExpireDateString(void) {
    CFStringRef result = nil;
#warning CopyExpireDateString commented out -- no beta expiration
    /*    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSString *buildDateString = [[[NSString alloc] initWithCString:__DATE__] autorelease];
    NSCalendarDate *buildDate = [NSCalendarDate dateWithString:buildDateString calendarFormat:@"%b %d %Y"];
    NSCalendarDate *expiryDate = nil;
    BOOL tokenExists = NO;
    if (buildDate)
        {
        NSCalendarDate *now = [NSCalendarDate calendarDate];
     
        //NSString *fmt = @"%a %m%d/%y %I:%M %p";
        //NSLog(@"%d=([buildDate compare:now] == NSOrderedAscending)\nb:<%@ %p>=%@ (%@)\nn:<%@ %p>=%@ (%@)", ([buildDate compare:now] == NSOrderedAscending), [buildDate className],    buildDate,    [buildDate descriptionWithCalendarFormat:fmt], buildDate, [now className],        now,        [now descriptionWithCalendarFormat:fmt], now);
     
        if ([buildDate compare:now] == NSOrderedAscending)
            {
            expiryDate = [buildDate dateByAddingYears:0 months:0 days:31 hours:0 minutes:0 seconds:0];
            NSString *expTokenPath = [NSHomeDirectory() stringByAppendingPathComponent:@"Library/Preferences/SysCheck_B10"];
            tokenExists = [[NSFileManager defaultManager] fileExistsAtPath:expTokenPath];
            if ([now compare:expiryDate] == NSOrderedAscending && !tokenExists)
                result = (CFStringRef)[[expiryDate descriptionWithCalendarFormat:@"%B %e, %Y"] retain];
            else if (!tokenExists)
                {
                [@"b" writeToFile:expTokenPath atomically:YES];
                NSDate *backDate = [[[NSDate alloc] initWithTimeInterval:-625000 sinceDate:buildDate] autorelease];
                NSDictionary *backDict = [NSDictionary dictionaryWithObjectsAndKeys:backDate, NSFileCreationDate, backDate, NSFileModificationDate, nil];
                [[NSFileManager defaultManager] changeFileAttributes:backDict atPath:expTokenPath];
                }
            }
        }
    if (!result)
        NSLog(@"Expired [T:%d B:%@, E:%@]", tokenExists, buildDate, expiryDate);
    [pool release];*/
    return result;
}

Boolean ShouldNotifyUser(void) {
    Boolean result = YES;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSDate *lastNotifiedUser = [[NSUserDefaults standardUserDefaults] objectForKey:(NSString *)U3PrefInformedNewVersionDate];
    NSDate *now = [NSDate date];
    if (lastNotifiedUser) {
        float dayInterval = [[NSUserDefaults standardUserDefaults] floatForKey:(NSString *)U3PrefInformDayInterval];
        if (dayInterval <= 0.0 || dayInterval > 365)
            dayInterval = 7.0;
        NSDate *cutoffDate =
            [[[NSDate alloc] initWithTimeInterval:dayInterval * 24 * 60 * 60 sinceDate:lastNotifiedUser] autorelease];
        result = ([cutoffDate compare:now] == NSOrderedAscending);
    }
    [[NSUserDefaults standardUserDefaults] setObject:now forKey:(NSString *)U3PrefInformedNewVersionDate];
    [pool release];
    return result;
}

CFStringRef CopyAppVersionString(void) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSString *result = [[NSString alloc]
        initWithFormat:@"%@ (%@)", [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"],
                       [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"], nil];
    [pool release];
    return (CFStringRef)result;
}

int ThisReleaseNumber(void) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    int result = [[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"] intValue];
    [pool release];
    return result;
}

void ThreadSleepTicks(int numTicks) {
    if (numTicks < 1)
        return;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    float seconds = (float)numTicks / 60.0;
    NSDate *endDate = [NSDate dateWithTimeIntervalSinceNow:seconds];
    [NSThread sleepUntilDate:endDate];
    [pool release];
}

short NumberForPrefsKey(CFStringRef prefsKey) {
    int asInt = [[NSUserDefaults standardUserDefaults] integerForKey:(NSString *)prefsKey];
    short result = asInt;
    return result;
}

CFURLRef GraphicsDirectoryURL(void) {
    static CFURLRef sResult = nil;
    if (!sResult) {
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        NSString *tilesPath = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"Graphics"];
        sResult = (CFURLRef)[[NSURL alloc] initFileURLWithPath:tilesPath];
        [pool release];
    }
    return sResult;
}

CFURLRef ResourcesDirectoryURL(void) {
    static CFURLRef sResult = nil;
    if (!sResult) {
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        sResult = (CFURLRef)[[NSURL alloc] initFileURLWithPath:[[NSBundle mainBundle] resourcePath]];
        [pool release];
    }
    return sResult;
}

CFArrayRef CopyGraphicsDirectoryItems(void) {
    CFArrayRef result = nil;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSString *tilesPath = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"Graphics"];
    result = (CFArrayRef)[[[NSFileManager defaultManager] subpathsAtPath:tilesPath] retain];
    [pool release];
    return result;
}

CFStringRef CopyCatStrings(CFStringRef str1, CFStringRef str2) {
    CFStringRef result = nil;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    result = (CFStringRef)[[(NSString *)str1 stringByAppendingString:(NSString *)str2] retain];
    [pool release];
    return result;
}

Boolean GetSystemVersion(unsigned *majorVersion, unsigned *minorVersion, unsigned *bugFixVersion) {
    OSErr err = noErr;

    SInt32 signedMajor = 0, signedMinor = 0, signedBugFix = 0;
    if ((err = Gestalt(gestaltSystemVersionMajor, &signedMajor)) != noErr)
        goto fail;
    if ((err = Gestalt(gestaltSystemVersionMinor, &signedMinor)) != noErr)
        goto fail;
    if ((err = Gestalt(gestaltSystemVersionBugFix, &signedBugFix)) != noErr)
        goto fail;
    if (majorVersion)
        *majorVersion = signedMajor;
    if (minorVersion)
        *minorVersion = signedMinor;
    if (bugFixVersion)
        *bugFixVersion = signedBugFix;

    return true;

fail:
    NSLog(@"Unable to obtain system version: %ld", (long)err);
    if (majorVersion)
        *majorVersion = 10;
    if (minorVersion)
        *minorVersion = 9;
    if (bugFixVersion)
        *bugFixVersion = 0;
    return false;
}

int EducateAboutFullScreen(void) {
    SInt16 itemHit;
    AlertStdAlertParamRec alertRec;
    alertRec.movable = TRUE;
    alertRec.helpButton = FALSE;
    alertRec.filterProc = nil;
    alertRec.defaultText = "\pOK";
    alertRec.cancelText = "\pDon't Remind Me";
    alertRec.otherText = nil;
    alertRec.defaultButton = kAlertStdAlertOKButton;
    alertRec.cancelButton = 0;
    alertRec.position = kWindowDefaultPosition;
    Str255 msg;
    GetPascalStringFromArrayByIndex(msg, CFSTR("Messages"), 263);

    OSErr result = StandardAlert(kAlertNoteAlert, "\pFull Screen Mode", msg, &alertRec, &itemHit);
    if (result != noErr)
        return 0;
    return (itemHit != kAlertStdAlertCancelButton);
}

CFArrayRef StringsArray(CFStringRef identifier) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    static NSMutableDictionary *sStrings = nil;
    if (!sStrings)
        sStrings = [[NSMutableDictionary alloc] init];

    NSArray *result = [sStrings objectForKey:(NSString *)identifier];
    if (!result) {
        NSString *stringsPath = [[NSBundle mainBundle] pathForResource:@"Strings" ofType:nil];
        stringsPath = [stringsPath stringByAppendingPathComponent:[NSString stringWithFormat:@"%@.plist", identifier]];
        if (stringsPath) {
            result = [[[NSArray alloc] initWithContentsOfFile:stringsPath] autorelease];
            if (result)
                [sStrings setObject:result forKey:(NSString *)identifier];
        }
    }

    [pool release];
    return (CFArrayRef)result;
}

void GetPascalStringFromArrayByIndex(StringPtr pstringPtr, CFStringRef identifier, int index) {
    pstringPtr[0] = 0;
    if (identifier && index >= 0) {
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        CFArrayRef stringsRef = StringsArray(identifier);
        if (!stringsRef)
            NSLog(@"no strings for id '%@'", (NSString *)identifier);
        else {
            if (index >= CFArrayGetCount(stringsRef))
                NSLog(@"%d>%ld (%@)", index, CFArrayGetCount(stringsRef), (NSString *)identifier);
            else {
                CFStringRef theStringRef = CFArrayGetValueAtIndex(stringsRef, index);
                if (theStringRef) {
                    Boolean success = CFStringGetPascalString(theStringRef, pstringPtr, 256, kCFStringEncodingMacRoman);
                    if (!success)
                        success = CFStringGetPascalString(theStringRef, pstringPtr, 256, kCFStringEncodingNonLossyASCII);
                    if (!success)
                        NSLog(@"couldn't convert %d of %@ ('%@')", index, (NSString *)identifier, (NSString *)theStringRef);
                }
            }
        }
        [pool release];
    }
}

Boolean SetCursorNamed(CFStringRef cursorName, float scale) {
    CocoaInit();
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSString *key = [NSString stringWithFormat:@"%@_%.2f", cursorName, scale];
    static NSMutableDictionary *sCursors = nil;
    if (!sCursors)
        sCursors = [[NSMutableDictionary alloc] init];
    NSCursor *theCursor = [sCursors objectForKey:key];
    if (!theCursor) {
        NSString *cursorPath = nil;
        float foundScale = 0.0;
        int testScale = -1;
        while (!cursorPath && testScale != 0) {
            int theScale = (testScale < 0) ? scale : testScale;
            NSString *testName =
                (theScale == 1) ? (NSString *)cursorName : [NSString stringWithFormat:@"%@_%dX", cursorName, theScale];
            cursorPath = [[NSBundle mainBundle] pathForResource:testName ofType:@"png" inDirectory:@"Cursors"];
            if (cursorPath)
                foundScale = (float)theScale;
            else {
                if (testScale < 0)
                    testScale = scale * 2;
                else
                    --testScale;
            }
        }

        if (!cursorPath)
            cursorPath = [[NSBundle mainBundle] pathForResource:(NSString *)cursorName ofType:@"png" inDirectory:@"Cursors"];

        NSImage *cursorImage = nil;
        if (cursorPath)
            cursorImage = [[[NSImage alloc] initWithContentsOfFile:cursorPath] autorelease];
        if (cursorImage && (scale != foundScale)) {
            NSRect orgRect = NSZeroRect;
            orgRect.size = [cursorImage size];
            NSRect scaledRect = orgRect;
            float scaleFactor = scale / foundScale;
            scaledRect.size.width *= scaleFactor;
            scaledRect.size.height *= scaleFactor;
            NSImage *scaledImage = [[[NSImage alloc] initWithSize:scaledRect.size] autorelease];
            if (scaledImage) {
                [scaledImage lockFocus];
                [cursorImage drawInRect:scaledRect fromRect:orgRect operation:NSCompositeSourceOut fraction:1.0];
                [scaledImage unlockFocus];
                cursorImage = scaledImage;
            }
        }
        if (cursorImage) {
            NSPoint hotSpot = NSMakePoint([cursorImage size].width / 2.0, [cursorImage size].height / 2.0);
            NSString *infoPath = [[cursorPath stringByDeletingPathExtension] stringByAppendingPathExtension:@"txt"];
            NSString *info = [NSString stringWithContentsOfFile:infoPath];
            if ([info length] > 9 && [info hasPrefix:@"hotspot:"]) {
                NSArray *parms = [[info substringFromIndex:8] componentsSeparatedByString:@","];
                if ([parms count] == 2)
                    hotSpot = NSMakePoint([cursorImage size].width * [[parms objectAtIndex:0] floatValue],
                                          [cursorImage size].height * [[parms objectAtIndex:1] floatValue]);
            }
            theCursor = [[[NSCursor alloc] initWithImage:cursorImage hotSpot:hotSpot] autorelease];
        }
        if (theCursor)
            [sCursors setObject:theCursor forKey:key];
    }

    [theCursor set];
    [pool release];
    return (theCursor != nil);
}

// Play a sound effect using AVFoundation.
void PlaySoundFileQT(CFStringRef soundName, Boolean async) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    static NSString *sSoundsDirectory = nil;
    if (!sSoundsDirectory)
        sSoundsDirectory = [[[NSBundle mainBundle] pathForResource:@"Sounds" ofType:nil] retain];
    if (sSoundsDirectory) {
        if (!sSoundPlayers)
            sSoundPlayers = [[NSMutableDictionary alloc] init];

        NSString *key = (NSString *)soundName;
        AVAudioPlayer *player = [sSoundPlayers objectForKey:key];
        if (!player) {
            NSArray *files = [[NSFileManager defaultManager] subpathsAtPath:sSoundsDirectory];
            NSString *targetFile = nil;
            NSString *prefix = [key stringByAppendingString:@"."];
            for (NSString *aFilename in files) {
                if ([aFilename hasPrefix:prefix]) {
                    targetFile = [sSoundsDirectory stringByAppendingPathComponent:aFilename];
                    break;
                }
            }
            if (targetFile) {
                NSURL *url = [NSURL fileURLWithPath:targetFile];
                player = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:nil];
                player.volume = sSoundVolume;
                [player prepareToPlay];
                [sSoundPlayers setObject:player forKey:key];
                [player release];
            }
        }

        if (player) {
            [player stop];
            player.currentTime = 0.0;
            [player play];
            if (!async) {
                while ([player isPlaying])
                    [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.05]];
            }
        } else {
            NSLog(@"Cannot play sound '%@'", key);
        }
    }
    [pool release];
}

void SetSoundVolumePercent(short newVolume) {
    if (newVolume >= 0 && newVolume <= 100) {
        sSoundVolume = newVolume / 100.0f;
        for (AVAudioPlayer *player in [sSoundPlayers allValues])
            player.volume = sSoundVolume;
    }
}

int RunCocoaDialog(CFStringRef nibName, CFMutableDictionaryRef valuesDict, CFStringRef controllerClassName) {
    CocoaInit();
    int result = -1;
    NSAutoreleasePool *myPool = [[NSAutoreleasePool alloc] init];

    NSNib *theNib = [[[NSNib alloc] initWithNibNamed:(NSString *)nibName bundle:[NSBundle mainBundle]] autorelease];
    if (!theNib) {
        NSBeep();
        NSLog(@"Cannot initialize nib '%@'", nibName);
    } else {
        Class contClass = nil;
        if (controllerClassName)
            contClass = NSClassFromString((NSString *)controllerClassName);
        if (contClass == nil)
            contClass = NSClassFromString(@"LWCocoaDialogController");
        LWCocoaDialogController *dialogCnt = (LWCocoaDialogController *)[[[contClass alloc] init] autorelease];
        @try {
            NSArray *topLevelObjs = nil;
            if ([theNib instantiateNibWithOwner:dialogCnt topLevelObjects:&topLevelObjs]) {
                [[dialogCnt controller] setContent:(NSMutableDictionary *)valuesDict];
                [[dialogCnt window] center];
                NSArray *watchedKeys = [[[(NSDictionary *)valuesDict allKeys] retain] autorelease];
                int i;
                for (i = [watchedKeys count] - 1; i >= 0; i--) {
                    [[dialogCnt controller] addObserver:dialogCnt
                                             forKeyPath:[NSString stringWithFormat:@"content.%@", [watchedKeys objectAtIndex:i]]
                                                options:0
                                                context:NULL];
                }
                result = [NSApp runModalForWindow:[dialogCnt window]];
                for (i = [watchedKeys count] - 1; i >= 0; i--) {
                    [[dialogCnt controller]
                        removeObserver:dialogCnt
                            forKeyPath:[NSString stringWithFormat:@"content.%@", [watchedKeys objectAtIndex:i]]];
                }
                [[dialogCnt window] orderOut:nil];
                [[dialogCnt window] close];
            }
        } @catch (NSException *e) {
            NSBeep();
            NSLog(@"!!! %@", [e description]);
        }
    }

    [myPool release];
    return result;
}

void LWOpenURL(CFStringRef urlString) {
    NSURL *asURL = [NSURL URLWithString:(NSString *)urlString];
    if (asURL)
        [[NSWorkspace sharedWorkspace] openURL:asURL];
}

void SetRefMenuIcons(MenuRef theMenu) {
    NSAutoreleasePool *myPool = [[NSAutoreleasePool alloc] init];
    int item;
    for (item = 1; item < 9; item++) {
        NSString *imageName = NULL;
        if (item == 1)
            imageName = @"MenuCommands";
        else if (item == 2)
            imageName = @"MenuSpells";
        else if (item == 3)
            imageName = @"MenuMisc";
        else if (item == 4)
            imageName = @"MenuMap";
        else if (item == 6)
            imageName = @"MenuManual";
        else if (item == 7)
            imageName = @"MenuCleric";
        else if (item == 8)
            imageName = @"MenuWizard";
        if (imageName) {
            NSString *menuIconPath = [[NSBundle mainBundle] pathForResource:imageName ofType:@"png"];
            if ([menuIconPath length]) {
                CGDataProviderRef iconProvRef = CGDataProviderCreateWithFilename([menuIconPath UTF8String]);
                if (iconProvRef) {
                    CGImageRef theImage = CGImageCreateWithPNGDataProvider(iconProvRef, NULL, true, kCGRenderingIntentDefault);
                    if (theImage) {
                        SetMenuItemIconHandle(theMenu, item, kMenuCGImageRefType, (Handle)theImage);
                        CFRelease(theImage);
                    }
                    CFRelease(iconProvRef);
                }
            }
        }
    }
    [myPool release];
}

void *LWCreateMetalView(void *windowRef) {
    CocoaInit();
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    MTKView *result = nil;
    @try {
        NSWindow *theWindow = [[LWWindowManager sharedInstance] windowForWindowRef:windowRef];
        if (theWindow) {
            result = [[[MTKView alloc] initWithFrame:[[theWindow contentView] bounds] device:MTLCreateSystemDefaultDevice()] autorelease];
            [result setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
            [[theWindow contentView] addSubview:result];
        }
    } @catch (NSException *exception) {
        NSLog(@"LWCreateMetalView %@", exception);
    }
    [pool release];
    return result;
}

