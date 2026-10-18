"""Check aspect-fit mouse mapping and scoped native-loop ownership without a GUI."""

from pathlib import Path
import os
import platform
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


def function_source(source, signature):
    start = source.index(signature)
    body = source.index("{", start)
    depth, end = 1, body + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


HARNESS = r'''
#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import <CoreText/CoreText.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static void check(BOOL ok, const char *message) {
    if (!ok) { fprintf(stderr, "FAIL: %s\n", message); exit(1); }
}
/* GEOMETRY */
static int liveProbes, liveEvents, dispatched;
static id sU3MainSurfaceWindow;
static short gUpdateWhere = 5;
static Boolean gDone;
static BOOL checkDeadline;
static NSDate *originalDeadline;
@interface TestWindow : NSObject
- (NSInteger)windowNumber;
@end
@implementation TestWindow
- (NSInteger)windowNumber { return 37; }
@end
@interface LifetimeProbe : NSObject @end
@implementation LifetimeProbe
- (id)init { if ((self = [super init])) ++liveProbes; return self; }
- (void)dealloc { --liveProbes; [super dealloc]; }
@end
static void transient(void) { [[[LifetimeProbe alloc] init] autorelease]; }
@interface TestEvent : NSObject {
    BOOL _game;
    NSEventType _type;
    NSPoint _location;
    char _key;
}
@property(nonatomic) BOOL game;
@property(nonatomic) NSEventType type;
@property(nonatomic) NSPoint locationInWindow;
@property(nonatomic) char key;
- (id)window;
- (NSInteger)windowNumber;
@end
@implementation TestEvent
@synthesize game = _game, type = _type, locationInWindow = _location, key = _key;
- (id)window { return _game ? sU3MainSurfaceWindow : nil; }
- (NSInteger)windowNumber { return _game ? 37 : 0; }
- (id)init { if ((self = [super init])) ++liveEvents; return self; }
- (void)dealloc { --liveEvents; [super dealloc]; }
@end
@interface TestView : NSObject {
    NSRect _bounds;
    NSPoint _windowOrigin;
    BOOL _flipped;
}
@property(nonatomic) NSRect bounds;
@property(nonatomic) NSPoint windowOrigin;
@property(nonatomic) BOOL isFlipped;
- (NSPoint)convertPoint:(NSPoint)point fromView:(id)view;
@end
@implementation TestView
@synthesize bounds = _bounds, windowOrigin = _windowOrigin, isFlipped = _flipped;
- (NSPoint)convertPoint:(NSPoint)point fromView:(id)view {
    return NSMakePoint(NSMinX(_bounds) + point.x - _windowOrigin.x,
        NSMinY(_bounds) + (_flipped ? NSHeight(_bounds) - (point.y - _windowOrigin.y) : point.y - _windowOrigin.y));
}
@end
@interface TestApp : NSObject {
    NSUInteger _remaining;
    NSUInteger _nonGameRemaining;
    TestEvent *_next;
}
@property(nonatomic) NSUInteger remaining;
@property(nonatomic) NSUInteger nonGameRemaining;
@property(nonatomic, retain) TestEvent *next;
- (TestEvent *)nextEventMatchingMask:(NSEventMask)mask untilDate:(NSDate *)date
    inMode:(NSString *)mode dequeue:(BOOL)dequeue;
- (void)sendEvent:(TestEvent *)event;
- (void)updateWindows;
- (id)modalWindow;
@end
@implementation TestApp
@synthesize remaining = _remaining, nonGameRemaining = _nonGameRemaining, next = _next;
- (TestEvent *)nextEventMatchingMask:(NSEventMask)mask untilDate:(NSDate *)date
    inMode:(NSString *)mode dequeue:(BOOL)dequeue {
    check(liveProbes == 0, "previous event's transient objects drained before next event");
    if (checkDeadline) {
        if (!originalDeadline) originalDeadline = [date retain];
        check(date == originalDeadline, "draining AppKit events preserves the original poll deadline");
    }
    if (_nonGameRemaining) {
        --_nonGameRemaining;
        transient();
        TestEvent *event = [[[TestEvent alloc] init] autorelease];
        event.type = NSMouseMoved;
        return event;
    }
    if (_next) {
        TestEvent *event = [[_next retain] autorelease]; [_next release]; _next = nil; return event;
    }
    if (!_remaining) return nil;
    transient();
    TestEvent *event = [[[TestEvent alloc] init] autorelease];
    event.game = (--_remaining % 2) == 0; event.type = NSKeyDown; event.key = 'g';
    return event;
}
- (void)sendEvent:(TestEvent *)event { ++dispatched; transient(); }
- (void)updateWindows { transient(); }
- (id)modalWindow { return nil; }
- (void)dealloc { [_next release]; [super dealloc]; }
@end
#define NSApp testApp
#define NSEvent TestEvent
static TestApp *testApp;
static TestView *sU3MainSurfaceView;
static NSMutableArray *sU3PendingInput;
static BOOL sU3HeadlessSurface;
static struct { int width, height; } sU3MainBitmap;
static Point sU3MousePoint;
static Boolean sU3DiagnosticMousePending;
static char sU3DiagnosticKey;
static unsigned char sU3DiagnosticKeys[256];
static unsigned sU3DiagnosticKeyIndex, sU3DiagnosticKeyCount;
static char U3CocoaKeyCharacter(TestEvent *event) { return event.key; }
static void CocoaInit(void) {}
/* EVENT_FUNCTIONS */
static NSColor *sU3ForegroundColor, *sU3BackgroundColor;
static NSPoint sU3PenLocation;
static short sU3TextSize = 12, sU3TextFace, sU3TextFont;
static void U3CocoaEnsureDrawState(void) {
    if (!sU3ForegroundColor) sU3ForegroundColor = [[NSColor whiteColor] retain];
    if (!sU3BackgroundColor) sU3BackgroundColor = [[NSColor blackColor] retain];
}
static void U3CocoaAddCommand(NSDictionary *command) {
    check(command[@"kind"] != nil, "drawing command remains alive during synchronous consumption"); transient();
}
static short U3CocoaTextWidth(ConstStr255Param text) { transient(); return 13; }
static NSFont *U3CocoaTextFont(CGFloat size, NSInteger face) {
    transient(); return [NSFont fontWithName:@"Menlo" size:size];
}
/* DRAW_FUNCTIONS */
static void geometry(void) {
    NSRect sizes[] = {NSMakeRect(0, 0, 640, 384), NSMakeRect(0, 0, 1280, 768),
        NSMakeRect(0, 0, 320, 192), NSMakeRect(0, 0, 1920, 1080),
        NSMakeRect(0, 0, 1024, 1024), NSMakeRect(0, 0, 1001, 719), NSMakeRect(17, 29, 1200, 900)};
    for (NSUInteger n = 0; n < sizeof(sizes) / sizeof(sizes[0]); ++n) {
        for (int flipped = 0; flipped <= 1; ++flipped) {
            NSPoint pixels[] = {NSMakePoint(.5, .5), NSMakePoint(639.5, 383.5),
                NSMakePoint(320.5, 192.5), NSMakePoint(0, 0)};
            for (NSUInteger p = 0; p < sizeof(pixels) / sizeof(pixels[0]); ++p) {
                NSPoint view = U3CocoaViewPointForBitmapPoint(pixels[p], sizes[n], flipped, 640, 384);
                Point mapped;
                check(U3CocoaMapViewPoint(view, sizes[n], flipped, 640, 384, &mapped), "scaled point is inside displayed bitmap");
                check(mapped.h == floor(pixels[p].x) && mapped.v == floor(pixels[p].y), "inverse scaling and Y orientation preserve bitmap pixel");
            }
            NSPoint outside[] = {NSMakePoint(-.01, 20), NSMakePoint(640, 20),
                NSMakePoint(20, -.01), NSMakePoint(20, 384)};
            for (NSUInteger p = 0; p < sizeof(outside) / sizeof(outside[0]); ++p) {
                NSPoint view = U3CocoaViewPointForBitmapPoint(outside[p], sizes[n], flipped, 640, 384);
                Point mapped = {123, 456};
                check(!U3CocoaMapViewPoint(view, sizes[n], flipped, 640, 384, &mapped), "letterbox and excluded edges cannot click game controls");
                check(mapped.h == 456 && mapped.v == 123, "rejected point does not overwrite previous mouse position");
            }
        }
    }
    Point point;
    check(!U3CocoaMapViewPoint(NSMakePoint(NAN, 0), sizes[0], YES, 640, 384, &point), "reject nonfinite point");
    check(!U3CocoaMapViewPoint(NSZeroPoint, NSZeroRect, YES, 640, 384, &point), "reject empty view");
    check(!U3CocoaMapViewPoint(NSZeroPoint, NSMakeRect(0, 0, INFINITY, 384), YES, 640, 384, &point), "reject nonfinite bounds");
    check(!U3CocoaMapViewPoint(NSZeroPoint, sizes[0], YES, 0, 384, &point), "reject empty bitmap");
    check(!U3CocoaMapViewPoint(NSZeroPoint, sizes[0], YES, 32768, 384, &point), "reject Point overflow");
}
static void eventOwnership(void) {
    sU3MainSurfaceWindow = [[TestWindow alloc] init];
    testApp = [[TestApp alloc] init]; testApp.remaining = 100;
    U3CocoaPumpEvents();
    check(liveProbes == 0 && liveEvents == 50 && dispatched == 50 && sU3PendingInput.count == 50,
        "pump drains transients while queued input remains owned");
    for (int i = 0; i < 50; ++i) {
        char key = 0; Boolean mouse = true;
        check(U3CocoaPollKeyMouse(YES, 0, &key, &mouse) && key == 'g' && !mouse, "queued input survives event pool");
        check(liveProbes == 0 && liveEvents == 49 - i, "poll drains consumed event and update transients");
    }
    sU3MainSurfaceView = [[TestView alloc] init];
    sU3MainSurfaceView.bounds = NSMakeRect(0, 0, 1280, 768);
    sU3MainSurfaceView.windowOrigin = NSMakePoint(23, 31); sU3MainSurfaceView.isFlipped = YES;
    sU3MainBitmap.width = 640; sU3MainBitmap.height = 384;
    TestEvent *event = [[TestEvent alloc] init]; event.game = YES; event.type = NSLeftMouseDown;
    event.locationInWindow = NSMakePoint(23 + 960, 31 + 576); testApp.next = event; [event release];
    char key = 'x'; Boolean mouse = false;
    check(U3CocoaPollKeyMouse(YES, 0, &key, &mouse) && mouse && !key && sU3MousePoint.h == 480 && sU3MousePoint.v == 96,
        "native polling converts window coordinates through flipped scaled view");
    event = [[TestEvent alloc] init]; event.game = YES; event.type = NSLeftMouseDown;
    event.locationInWindow = NSMakePoint(23 + 960, 31 + 576); testApp.next = event; [event release];
    testApp.nonGameRemaining = 24;
    int beforeBacklog = dispatched;
    checkDeadline = YES;
    check(U3CocoaPollKeyMouse(YES, 0, &key, &mouse) && mouse && dispatched == beforeBacklog + 24,
        "one poll dispatches the preceding AppKit backlog and returns the pending game click");
    checkDeadline = NO; [originalDeadline release]; originalDeadline = nil;
    event = [[TestEvent alloc] init]; event.game = YES; event.type = NSLeftMouseDown;
    event.locationInWindow = NSMakePoint(23 + 960, 31 + 576); testApp.next = event; [event release];
    testApp.nonGameRemaining = 300; beforeBacklog = dispatched;
    check(!U3CocoaPollKeyMouse(YES, 0, &key, &mouse) && dispatched == beforeBacklog + 256,
        "continuous AppKit traffic cannot monopolize one game poll");
    check(U3CocoaPollKeyMouse(YES, 0, &key, &mouse) && mouse && dispatched == beforeBacklog + 300,
        "input behind a large backlog remains available after the bounded poll");
    sU3MainSurfaceView.bounds = NSMakeRect(0, 0, 1600, 900);
    event = [[TestEvent alloc] init]; event.game = YES; event.type = NSLeftMouseDown;
    event.locationInWindow = NSMakePoint(23 + 10, 31 + 450); testApp.next = event; [event release];
    mouse = true; key = 'x';
    check(!U3CocoaPollKeyMouse(YES, 0, &key, &mouse) && !mouse && !key, "native polling ignores letterbox click");
    int beforeChrome = dispatched;
    event = [[TestEvent alloc] init]; event.game = YES; event.type = NSLeftMouseDown;
    event.locationInWindow = NSMakePoint(23 + 10, 31 + 910); testApp.next = event; [event release];
    check(!U3CocoaPollKeyMouse(YES, 0, &key, &mouse) && dispatched == beforeChrome + 1,
        "native polling dispatches title-bar and close-control clicks to AppKit");
    event = [[TestEvent alloc] init]; event.game = YES; event.type = NSLeftMouseDown;
    event.locationInWindow = NSMakePoint(23 + 10, 31 + 910); testApp.next = event; [event release];
    U3CocoaPumpEvents();
    check(dispatched == beforeChrome + 2 && !sU3PendingInput.count,
        "event pump dispatches window chrome instead of queuing it as game input");
    check(liveEvents == 0 && liveProbes == 0, "mouse polling drains event ownership");
    [sU3MainSurfaceView release]; sU3MainSurfaceView = nil;
    [sU3PendingInput release]; sU3PendingInput = nil;
    [testApp release]; testApp = nil;
    [sU3MainSurfaceWindow release]; sU3MainSurfaceWindow = nil;
}
int main(void) { @autoreleasepool {
    geometry(); eventOwnership();
    const unsigned char text[] = {4, 'T', 'e', 's', 't'};
    for (int i = 0; i < 1000; ++i) {
        U3CocoaPaintRect(1, 2, 3, 4); check(!liveProbes, "paint transients released immediately");
        U3CocoaEraseRect(1, 2, 3, 4); check(!liveProbes, "erase transients released immediately");
        U3CocoaFrameRect(1, 2, 3, 4); check(!liveProbes, "frame transients released immediately");
        U3CocoaDrawPascalString(text); check(!liveProbes, "text command transients released immediately");
        check(U3CocoaMeasuredTextWidth(text) > 0 && !liveProbes, "measured text transients released immediately");
    }
    [sU3ForegroundColor release]; [sU3BackgroundColor release];
    puts("Aspect-fit mouse mapping, polling, queued event ownership and render pool tests passed");
} return 0; }
'''


@unittest.skipUnless(platform.system() == "Darwin" and shutil.which("clang"),
                     "Native view tests require macOS developer tools")
class NativeViewInputTests(unittest.TestCase):
    def test_geometry_and_ownership_without_gui(self):
        source = (ROOT / "Sources/CocoaBridge.m").read_text()
        geometry = "\n".join(function_source(source, signature) for signature in (
            "static NSRect U3CocoaBitmapDestinationRect(",
            "static Boolean U3CocoaMapViewPoint(", "static NSPoint U3CocoaViewPointForBitmapPoint(",
        ))
        events = "\n".join(function_source(source, signature) for signature in (
            "static Boolean U3CocoaTraceNativeInputEnabled(void) {",
            "static void U3CocoaTraceNativeEvent(",
            "static Boolean U3CocoaIsGameInput(", "void U3CocoaPumpEvents(void) {", "Boolean U3CocoaPollKeyMouse(",
        ))
        drawing = "\n".join(function_source(source, signature) for signature in (
            "static NSString *U3CocoaStringFromLegacyBytes(", "static NSString *U3CocoaStringFromPascal(",
            "static NSRect U3CocoaRectFromQuickDraw(", "void U3CocoaPaintRect(",
            "void U3CocoaEraseRect(", "void U3CocoaFrameRect(", "void U3CocoaDrawPascalString(",
        ))
        drawing += "\n" + function_source(source, "short U3CocoaTextWidth(").replace(
            "short U3CocoaTextWidth(", "short U3CocoaMeasuredTextWidth(", 1)
        harness = HARNESS.replace("/* GEOMETRY */", geometry).replace(
            "/* EVENT_FUNCTIONS */", events).replace("/* DRAW_FUNCTIONS */", drawing)
        with tempfile.TemporaryDirectory(prefix="u3-native-view-") as temporary:
            path = Path(temporary) / "view.m"
            binary = Path(temporary) / "view"
            path.write_text(harness)
            compiled = subprocess.run(
                ["clang", "-fsanitize=address,undefined", "-g", "-Wno-deprecated-declarations",
                 str(path), "-framework", "Foundation", "-framework", "AppKit",
                 "-framework", "Carbon", "-framework", "CoreText", "-o", str(binary)],
                capture_output=True, text=True, timeout=45,
            )
            self.assertEqual(compiled.returncode, 0, compiled.stderr)
            checked = subprocess.run([str(binary)], capture_output=True, text=True, timeout=30,
                env={**os.environ, "U3_TRACE_NATIVE_INPUT": "1"})
            self.assertEqual(checked.returncode, 0, checked.stdout + checked.stderr)
            self.assertIn("tests passed", checked.stdout)
            self.assertIn("stage=poll-accepted", checked.stderr)
            self.assertIn("stage=poll-dispatched", checked.stderr)
            self.assertIn("stage=pump-dispatched", checked.stderr)


if __name__ == "__main__":
    unittest.main()
