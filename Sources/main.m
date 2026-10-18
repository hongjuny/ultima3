#import <Cocoa/Cocoa.h>
#import <stdlib.h>
#include <unistd.h>
#include <string.h>

#import "CocoaBridge.h"
#import "UltimaMain.h"
#import "U3Audio.h"
#import "U3Platform.h"

extern Boolean U3LegacyBitmapSelfTest(void);
extern bool U3IOSelfTest(void);
extern Boolean U3PartySelectionSelfTest(void);
extern Boolean U3CharacterCreationSelfTest(void);
extern Boolean gDone;
extern unsigned char Party[65];
extern short gSongPlaying;
extern long lastSaveNumberOfMoves;

/* Opt-in process-exit diagnostic. The driver gives this app copy its own
 * bundle identifier, muted preference domain, and temporary save directory. */
static NSString *sQuitCheckScenario;
static NSTimer *sQuitCheckTimer;
static NSData *sQuitCheckOriginalSave;
static NSString *sQuitCheckSavePath;
static NSUInteger sQuitCheckPhase, sQuitCheckPolls, sQuitCheckCancelPolls, sQuitCheckActions, sQuitCheckButtons;
static BOOL sQuitCheckStarted, sQuitCheckResumed, sQuitCheckHealerReturned;
static NSTimeInterval sQuitCheckDeadline;

static void U3QuitCheckFail(const char *message) {
    fprintf(stderr, "Native game quit: FAILED: %s\n", message);
    fflush(stderr);
    _exit(3);
}

static NSButton *U3QuitCheckButton(NSWindow *window, NSString *title) {
    NSMutableArray *views = [NSMutableArray arrayWithObject:window.contentView];
    while (views.count) {
        NSView *view = [[[views lastObject] retain] autorelease];
        [views removeLastObject];
        if ([view isKindOfClass:NSButton.class] && [[(NSButton *)view title] isEqual:title])
            return (NSButton *)view;
        [views addObjectsFromArray:view.subviews];
    }
    return nil;
}

static void U3QuitCheckPostAction(BOOL commandKey) {
    NSWindow *window = nil;
    for (NSWindow *candidate in NSApp.windows)
        if ([candidate.title isEqual:@"Ultima III"]) { window = candidate; break; }
    if (!window) U3QuitCheckFail("game window is missing");
    NSTimeInterval timestamp = NSProcessInfo.processInfo.systemUptime;
    ++sQuitCheckActions;
    fprintf(stderr, "Native game quit: action=%s scenario=%s\n", commandKey ? "Cmd-Q" : "close",
        sQuitCheckScenario.UTF8String);
    if (commandKey) {
        [NSApp postEvent:[NSEvent keyEventWithType:NSKeyDown location:NSZeroPoint
            modifierFlags:NSCommandKeyMask timestamp:timestamp windowNumber:window.windowNumber context:nil
            characters:@"q" charactersIgnoringModifiers:@"q" isARepeat:NO keyCode:12] atStart:YES];
    } else {
        NSButton *close = [window standardWindowButton:NSWindowCloseButton];
        if (!close || !close.enabled) U3QuitCheckFail("close control is unavailable");
        NSRect bounds = close.bounds;
        NSPoint point = [close convertPoint:NSMakePoint(NSMidX(bounds), NSMidY(bounds)) toView:nil];
        [NSApp postEvent:[NSEvent mouseEventWithType:NSLeftMouseUp location:point modifierFlags:0
            timestamp:timestamp + 0.01 windowNumber:window.windowNumber context:nil
            eventNumber:302 clickCount:1 pressure:0] atStart:YES];
        [NSApp postEvent:[NSEvent mouseEventWithType:NSLeftMouseDown location:point modifierFlags:0
            timestamp:timestamp windowNumber:window.windowNumber context:nil
            eventNumber:301 clickCount:1 pressure:1] atStart:YES];
    }
}

static void U3QuitCheckTick(void) {
    if (sQuitCheckPhase == 4) return;
    if (NSProcessInfo.processInfo.systemUptime >= sQuitCheckDeadline)
        U3QuitCheckFail("native quit action or alert did not complete");
    BOOL healer = [sQuitCheckScenario isEqual:@"healer-discard"];
    NSWindow *modal = NSApp.modalWindow;
    if (sQuitCheckPhase == 0) {
        if (modal || (!healer && !U3CocoaCanSaveGame())) return;
        sQuitCheckPhase = 1;
        U3QuitCheckPostAction(![sQuitCheckScenario isEqual:@"journey-discard"]);
    } else if (sQuitCheckPhase == 2) {
        if (modal || sQuitCheckPolls <= sQuitCheckCancelPolls || !U3CocoaCanSaveGame()) return;
        if (gDone || ![[NSData dataWithContentsOfFile:sQuitCheckSavePath] isEqual:sQuitCheckOriginalSave])
            U3QuitCheckFail("Cancel did not preserve the running game and save");
        sQuitCheckResumed = YES;
        fprintf(stderr, "Native game quit: Cancel resumed actual Game input\n");
        sQuitCheckPhase = 3;
        U3QuitCheckPostAction(NO);
    } else if (modal) {
        BOOL cancel = sQuitCheckPhase == 1 && [sQuitCheckScenario isEqual:@"journey-cancel"];
        NSString *title = cancel ? @"Cancel" :
            ([sQuitCheckScenario isEqual:@"journey-save"] ? @"Save and Quit" : @"Quit Without Saving");
        NSButton *button = U3QuitCheckButton(modal, title);
        if (!button) return;
        if (healer && U3QuitCheckButton(modal, @"Save and Quit"))
            U3QuitCheckFail("healer action incorrectly permits saving");
        ++sQuitCheckButtons;
        sQuitCheckCancelPolls = sQuitCheckPolls;
        sQuitCheckPhase = cancel ? 2 : 4;
        fprintf(stderr, "Native game quit: button=%s\n", title.UTF8String);
        [button performClick:nil];
    }
}

/* Called only by explicit diagnostic checkpoints in the real Game loop. */
void U3NativeQuitGameCheckpoint(const char *stage) {
    if (!sQuitCheckScenario) return;
    if (!strcmp(stage, "polled")) { ++sQuitCheckPolls; return; }
    if (!strcmp(stage, "healer-returned")) { sQuitCheckHealerReturned = YES; return; }
    if (sQuitCheckStarted) return;
    sQuitCheckStarted = YES;
    sQuitCheckSavePath = [[[NSString stringWithUTF8String:getenv("U3_SAVE_DIRECTORY")]
        stringByAppendingPathComponent:@"Roster-v1.plist"] copy];
    sQuitCheckOriginalSave = [[NSData dataWithContentsOfFile:sQuitCheckSavePath] retain];
    if (!sQuitCheckOriginalSave.length) U3QuitCheckFail("test party was not persisted");
    /* Give Save and Quit a real unsaved move to persist. */
    Party[11] = 17; Party[12] = Party[13] = Party[14] = 0;
    sQuitCheckDeadline = NSProcessInfo.processInfo.systemUptime + 8;
    sQuitCheckTimer = [[NSTimer timerWithTimeInterval:0.02 repeats:YES block:^(NSTimer *unused) {
        (void)unused;
        U3QuitCheckTick();
    }] retain];
    [[NSRunLoop currentRunLoop] addTimer:sQuitCheckTimer forMode:NSDefaultRunLoopMode];
    [[NSRunLoop currentRunLoop] addTimer:sQuitCheckTimer forMode:NSModalPanelRunLoopMode];
    fprintf(stderr, "Native game quit: checkpoint=%s scenario=%s\n", stage, sQuitCheckScenario.UTF8String);
}

static BOOL U3QuitCheckFinish(void) {
    [sQuitCheckTimer invalidate];
    BOOL save = [sQuitCheckScenario isEqual:@"journey-save"];
    BOOL cancel = [sQuitCheckScenario isEqual:@"journey-cancel"];
    BOOL healer = [sQuitCheckScenario isEqual:@"healer-discard"];
    NSData *current = sQuitCheckSavePath ? [NSData dataWithContentsOfFile:sQuitCheckSavePath] : nil;
    BOOL passed = sQuitCheckStarted && sQuitCheckPhase == 4 && gDone && !gSongPlaying &&
        sQuitCheckActions == (cancel ? 2 : 1) && sQuitCheckButtons == (cancel ? 2 : 1) &&
        (!cancel || sQuitCheckResumed) && (!healer || sQuitCheckHealerReturned) &&
        (save ? (lastSaveNumberOfMoves == 17 && current.length && ![current isEqual:sQuitCheckOriginalSave]) :
            [current isEqual:sQuitCheckOriginalSave]);
    fprintf(stderr, "Native game quit: %s %s after actual Game and cleanup returned\n",
        sQuitCheckScenario.UTF8String, passed ? "passed" : "FAILED");
    [sQuitCheckTimer release]; [sQuitCheckOriginalSave release]; [sQuitCheckSavePath release];
    return passed;
}

int main(int argc, char *argv[]) {
    if (getenv("U3_NATIVE_QUIT_GAME_CHECK")) {
        @autoreleasepool {
            const char *directory = getenv("U3_SAVE_DIRECTORY");
            NSString *scenario = [NSString stringWithUTF8String:getenv("U3_NATIVE_QUIT_GAME_CHECK")];
            NSString *saveDirectory = directory ? [[NSString stringWithUTF8String:directory]
                stringByResolvingSymlinksInPath] : nil;
            NSString *temporary = [NSTemporaryDirectory() stringByResolvingSymlinksInPath];
            BOOL isolatedDirectory = [saveDirectory hasPrefix:[temporary stringByAppendingString:@"/"]] &&
                [[NSFileManager defaultManager] contentsOfDirectoryAtPath:saveDirectory error:NULL].count == 0;
            if (!directory || !*directory ||
                !isolatedDirectory ||
                ![NSBundle.mainBundle.bundleIdentifier hasPrefix:@"local.ultima3.nativequit."] ||
                ![@[@"journey-save", @"journey-discard", @"journey-cancel", @"healer-discard"] containsObject:scenario] ||
                !U3PlatformGetBooleanPreference(U3PreferenceSoundDisabled) ||
                !U3PlatformGetBooleanPreference(U3PreferenceMusicDisabled) ||
                !U3PlatformGetBooleanPreference(U3PreferenceSpeechDisabled)) {
                fprintf(stderr, "Native game quit check requires an isolated test app, temporary save directory and muted test preferences.\n");
                return 2;
            }
            sQuitCheckScenario = [scenario copy];
            alarm(30);
        }
    }
    if (getenv("U3_BOOT_CHECK") || getenv("U3_WORLD_RENDER_CHECK") ||
        getenv("U3_WORLD_INPUT_CHECK") || getenv("U3_WORLD_MOUSE_CHECK")) {
        if (!getenv("U3_SAVE_DIRECTORY")) {
            fprintf(stderr, "Boot check requires U3_SAVE_DIRECTORY for isolated storage.\n");
            return 2;
        }
        alarm(90);
    }
    if (getenv("U3_IO_SELF_TEST")) {
        @autoreleasepool {
            bool passed = U3IOSelfTest();
            fprintf(stderr, "Save container test: %s\n", passed ? "passed" : "FAILED");
            return passed ? 0 : 1;
        }
    }
    if (getenv("U3_NATIVE_UI_SELF_TEST")) {
        @autoreleasepool {
            alarm(15);
            Boolean passed = U3CocoaNativeWindowSelfTest();
            alarm(0);
            fprintf(stderr, "Native window input test: %s\n", passed ? "passed" : "FAILED");
            return passed ? 0 : 1;
        }
    }
    if (getenv("U3_AUDIO_SELF_TEST")) {
        @autoreleasepool {
            bool passed = U3AudioMusicSelfTest();
            fprintf(stderr, "Music playback test: %s\n", passed ? "passed" : "FAILED");
            return passed ? 0 : 1;
        }
    }
    if (getenv("U3_RENDER_SELF_TEST")) {
        @autoreleasepool {
            Boolean passed = U3LegacyBitmapSelfTest();
            fprintf(stderr, "Legacy bitmap test: %s\n", passed ? "passed" : "FAILED");
            Boolean partyPassed = U3PartySelectionSelfTest();
            fprintf(stderr, "Party selection test: %s\n", partyPassed ? "passed" : "FAILED");
            passed = partyPassed && passed;
            Boolean characterPassed = U3CharacterCreationSelfTest();
            fprintf(stderr, "Character creation test: %s\n", characterPassed ? "passed" : "FAILED");
            passed = characterPassed && passed;
            Boolean imagesPassed = U3CocoaImageSelfTest();
            fprintf(stderr, "Image decoding test: %s\n", imagesPassed ? "passed" : "FAILED");
            passed = passed && imagesPassed;
            return passed ? 0 : 1;
        }
    }
    @autoreleasepool {
        int result = Ultima3_main();
        U3CocoaFinishApplication();
        if (sQuitCheckScenario) {
            BOOL passed = U3QuitCheckFinish();
            alarm(0);
            [sQuitCheckScenario release];
            return passed ? result : 3;
        }
        if (!gDone && !getenv("U3_SKIP_APP_RUN") && !getenv("U3_STARTUP_RENDER_CHECK") &&
            !getenv("U3_BOOT_CHECK") && !getenv("U3_WORLD_RENDER_CHECK") &&
            !getenv("U3_WORLD_INPUT_CHECK") && !getenv("U3_WORLD_MOUSE_CHECK"))
            U3CocoaRunApplication();
        return result;
    }
}
