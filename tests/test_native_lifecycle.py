"""Exercise actual quit/save routing and scoped pools without GUI or user files."""
from pathlib import Path
import platform
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def body(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


HARNESS = r'''
#import <Foundation/Foundation.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char Boolean;
typedef unsigned char Str255[256];
#define TRUE 1
#define FALSE 0
#define BASERES 400
#define NIL_PTR NULL
enum { U3PreferenceAutoSave, SAVEID=1, NSAlertFirstButtonReturn=1000, NSTerminateCancel=0, NSTerminateLater=2 };
typedef NSInteger NSApplicationTerminateReply;
@class NSApplication;
static Boolean gDone, gInterrupt, sU3GameInputReady, sU3SystemTerminationPending, headless, nativeSurface=TRUE, dirty;
static unsigned char Party[65], gCurFrame=1;
static short gMouseState=1, g835E;
static int gUpdateWhere=3, saves, commands, alerts, legacyChoice=1, choice;
static BOOL saveResult=YES;
static long lastSaveNumberOfMoves=100;
static int poolDeallocs, terminationReplies;
static void check(BOOL passed, const char *message) {
    if (!passed) { fprintf(stderr, "FAIL: %s\n", message); exit(1); }
}
@interface TestApp : NSObject { id _modalWindow; }
@property(nonatomic, retain) id modalWindow;
- (void)replyToApplicationShouldTerminate:(BOOL)reply;
@end
@implementation TestApp
@synthesize modalWindow=_modalWindow;
- (void)replyToApplicationShouldTerminate:(BOOL)reply { check(reply, "cleanup permits approved system quit"); ++terminationReplies; }
- (void)dealloc { [_modalWindow release]; [super dealloc]; }
@end
static TestApp *testApp;
#define NSApp testApp
@interface NSAlert : NSObject { NSString *_messageText, *_informativeText; }
@property(nonatomic,copy) NSString *messageText, *informativeText;
- (id)addButtonWithTitle:(NSString *)title;
- (NSInteger)runModal;
@end
@implementation NSAlert
@synthesize messageText=_messageText, informativeText=_informativeText;
- (id)addButtonWithTitle:(NSString *)title { return title; }
- (NSInteger)runModal { ++alerts; return NSAlertFirstButtonReturn + choice; }
- (void)dealloc { [_messageText release]; [_informativeText release]; [super dealloc]; }
@end
@interface NSMenuItem : NSObject { SEL _action; }
@property(nonatomic) SEL action;
@end
@implementation NSMenuItem
@synthesize action=_action;
@end
@interface PoolProbe : NSObject @end
@implementation PoolProbe
- (void)dealloc { ++poolDeallocs; [super dealloc]; }
@end
Boolean U3CocoaIsHeadlessDiagnostic(void) { return headless; }
Boolean U3CocoaHasMainSurface(void) { return nativeSurface; }
bool U3IOHasUnsavedChanges(void) { return dirty; }
Boolean QuitSave(short mode) { (void)mode; ++saves; return saveResult; }
void HandleFileChoice(int item) { check(item == SAVEID, "menu dispatches actual save action"); ++commands; }
Boolean U3PlatformGetBooleanPreference(int key) { (void)key; return FALSE; }
void CursorUpdate(void) {}
short Alert(short resource, void *filter) { (void)resource; (void)filter; ++alerts; return legacyChoice; }
void GetPascalStringFromArrayByIndex(unsigned char *string, CFStringRef table, int item) {
    (void)table; (void)item; string[0]=0;
}
void ParamText(const unsigned char *a, const unsigned char *b, const unsigned char *c, const unsigned char *d) {
    (void)a; (void)b; (void)c; (void)d;
}
__FUNCTIONS__
@interface MenuTarget : NSObject
- (void)saveGame:(id)sender;
- (void)quitGame:(id)sender;
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender;
- (BOOL)windowShouldClose:(id)sender;
- (BOOL)validateMenuItem:(NSMenuItem *)item;
@end
@implementation MenuTarget
__METHODS__
@end
#define U3_COCOA_SCOPED_POOL void *iteration __attribute__((cleanup(U3CocoaDrainAutoreleasePool),unused))=U3CocoaCreateAutoreleasePool()
static void earlyReturnPool(void) { U3_COCOA_SCOPED_POOL; [[[PoolProbe alloc] init] autorelease]; return; }
int main(void) {
 @autoreleasepool {
    testApp=[[TestApp alloc] init];
    MenuTarget *target=[[[MenuTarget alloc] init] autorelease];
    NSMenuItem *save=[[[NSMenuItem alloc] init] autorelease]; save.action=@selector(saveGame:);
    Party[11]=101;
    U3CocoaSetGameInputReady(FALSE);
    check(![target validateMenuItem:save], "save disabled during nested game action");
    [target saveGame:nil]; check(commands==0, "nested prompt does not receive save key or action");
    U3CocoaSetGameInputReady(TRUE);
    check([target validateMenuItem:save], "save enabled at stable surface prompt");
    [target saveGame:nil]; check(commands==1, "save menu invokes actual save once");
    Party[3]=1; [target saveGame:nil]; check(commands==1, "save rejected inside town"); Party[3]=0;
    choice=1; // Cancel among Save/Cancel/Discard.
    check(!QuitDialog() && saves==0 && alerts==1, "one unsaved move prompts and cancel preserves game");
    choice=0; saveResult=NO;
    check(!QuitDialog() && saves==1, "failed Save and Quit cancels termination");
    [target quitGame:nil]; check(!gDone && saves==2, "native quit preserves game after write failure");
    saveResult=YES;
    check(QuitDialog() && saves==3, "successful Save and Quit permits termination");
    U3CocoaSetGameInputReady(FALSE); choice=0;
    check(!QuitDialog(), "unsafe action offers cancel, without performing save");
    choice=1;
    check(QuitDialog() && saves==3, "explicit discard during action requires no save");
    gCurFrame=3; dirty=TRUE; choice=0;
    check(!QuitDialog(), "failed roster writes still prompt outside journey");
    dirty=FALSE; int prior=alerts;
    check(QuitDialog() && alerts==prior, "clean main menu quits directly");
    gCurFrame=1; Party[11]=100; U3CocoaSetGameInputReady(TRUE); prior=alerts;
    choice=1;
    check(!QuitDialog() && alerts==prior+1, "unchanged move count cannot hide healing or inventory changes");
    Party[11]=101; U3CocoaSetGameInputReady(TRUE); choice=1;
    [target quitGame:nil]; check(!gDone && !gInterrupt, "Cmd-Q cancel does not end game or intro");
    choice=2;
    check([target applicationShouldTerminate:nil]==NSTerminateLater && gDone && gInterrupt,
          "system quit defers AppKit termination while game unwinds");
    check(terminationReplies==0, "system termination waits for game cleanup");
    U3CocoaFinishApplication(); U3CocoaFinishApplication();
    check(terminationReplies==1, "system termination receives one reply after cleanup");
    gDone=gInterrupt=FALSE;
    check(![target windowShouldClose:nil] && gDone, "window close follows same protected quit path");
    gDone=gInterrupt=FALSE;
    testApp.modalWindow=@"active character dialog"; prior=alerts;
    [target quitGame:nil]; check(!gDone && alerts==prior, "modal dialog cannot recursively open quit prompt");
    prior=commands;
    [target saveGame:nil]; check(commands==prior, "save cannot run while a modal dialog is active");
    testApp.modalWindow=nil;
    nativeSurface=FALSE; legacyChoice=1; saveResult=NO;
    check(!QuitDialog(), "legacy quit also propagates save failure");
    headless=TRUE; check(U3CocoaConfirmQuit(TRUE)==2, "headless mode never opens quit UI");
    for(int i=0;i<1000;++i) {
        U3_COCOA_SCOPED_POOL;
        [[[PoolProbe alloc] init] autorelease];
        if(i==500) continue;
        if(i==999) break;
    }
    check(poolDeallocs==1000, "iteration pools drain on continue and break");
    earlyReturnPool(); check(poolDeallocs==1001, "iteration pool drains on early return");
    [testApp release];
    fprintf(stderr, "Native lifecycle tests passed\n");
 }
 return 0;
}
'''


@unittest.skipUnless(platform.system() == 'Darwin' and shutil.which('clang'), 'Requires macOS developer tools')
class NativeLifecycleTests(unittest.TestCase):
    def test_quit_save_routing_and_iteration_pools(self):
        cocoa=(ROOT/'Sources/CocoaBridge.m').read_text()
        legacy=(ROOT/'Sources/UltimaMacIF.c').read_text()
        functions='\n'.join(body(cocoa,sig) for sig in (
            'void *U3CocoaCreateAutoreleasePool(void)', 'void U3CocoaDrainAutoreleasePool(void **pool)',
            'void U3CocoaSetGameInputReady(Boolean ready)', 'Boolean U3CocoaCanSaveGame(void)',
            'void U3CocoaFinishApplication(void)',
            'static Boolean U3CocoaTraceNativeInputEnabled(void) {',
            'static void U3CocoaTraceNativeAction(const char *action) {',
            'short U3CocoaConfirmQuit(Boolean canSave)'))
        functions+='\n'+body(legacy,'Boolean QuitDialog(void)')
        methods='\n'.join(body(cocoa,sig) for sig in (
            '- (void)saveGame:(id)sender', '- (void)quitGame:(id)sender',
            '- (NSApplicationTerminateReply)applicationShouldTerminate:',
            '- (BOOL)windowShouldClose:(id)sender', '- (BOOL)validateMenuItem:'))
        with tempfile.TemporaryDirectory(prefix='u3-lifecycle-') as temporary:
            source=Path(temporary)/'lifecycle.m'
            executable=Path(temporary)/'lifecycle'
            source.write_text(HARNESS.replace('__FUNCTIONS__',functions).replace('__METHODS__',methods))
            compiled=subprocess.run(['clang','-fsanitize=address,undefined','-g','-Wall','-Wextra',
                '-framework','Foundation',str(source),'-o',str(executable)],capture_output=True,text=True,timeout=45)
            self.assertEqual(compiled.returncode,0,compiled.stderr)
            checked=subprocess.run([str(executable)],capture_output=True,text=True,timeout=30)
            self.assertEqual(checked.returncode,0,checked.stdout+checked.stderr)
            self.assertIn('Native lifecycle tests passed',checked.stderr)


if __name__=='__main__':
    unittest.main()
