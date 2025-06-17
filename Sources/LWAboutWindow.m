#import "LWAboutWindow.h"
#import "CocoaBridge.h"
#import "UltimaIncludes.h"

@implementation LWAboutView

- (void)drawRect:(NSRect)dirtyRect {
    [[NSColor windowBackgroundColor] set];
    NSRectFill(dirtyRect);

    NSDictionary *titleAttrs = @{ NSFontAttributeName: [NSFont boldSystemFontOfSize:18] };
    NSString *title = @"Ultima III";
    NSSize size = [title sizeWithAttributes:titleAttrs];
    NSPoint point = NSMakePoint(NSMidX(self.bounds) - size.width/2, NSMaxY(self.bounds) - 40);
    [title drawAtPoint:point withAttributes:titleAttrs];

    NSDictionary *textAttrs = @{ NSFontAttributeName: [NSFont systemFontOfSize:12] };
    if (self.versionString) {
        size = [self.versionString sizeWithAttributes:textAttrs];
        point = NSMakePoint(NSMidX(self.bounds) - size.width/2, NSMidY(self.bounds));
        [self.versionString drawAtPoint:point withAttributes:textAttrs];
    }
    if (self.copyrightString) {
        size = [self.copyrightString sizeWithAttributes:textAttrs];
        point = NSMakePoint(NSMidX(self.bounds) - size.width/2, 20);
        [self.copyrightString drawAtPoint:point withAttributes:textAttrs];
    }
}

@end

@implementation LWAboutWindow
@end

void ShowAboutWindow(void) {
    CocoaInit();
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    NSString *version = (NSString *)CopyAppVersionString();
    if (!version)
        version = @"";

    NSString *copyright = @"";
    Str255 copyrightStr = "\p";
    GetPascalStringFromArrayByIndex(copyrightStr, CFSTR("MoreMessages"), 98);
    if (copyrightStr[0])
        copyright = [[[NSString alloc] initWithBytes:&copyrightStr[1] length:copyrightStr[0] encoding:NSMacOSRomanStringEncoding] autorelease];

    NSRect frame = NSMakeRect(0, 0, 360, 160);
    LWAboutWindow *window = [[LWAboutWindow alloc] initWithContentRect:frame
                                           styleMask:(NSTitledWindowMask | NSClosableWindowMask)
                                             backing:NSBackingStoreBuffered
                                               defer:NO];
    LWAboutView *view = [[[LWAboutView alloc] initWithFrame:[window contentRectForFrameRect:frame]] autorelease];
    [view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    view.versionString = version;
    view.copyrightString = copyright;
    [window setContentView:view];
    [window center];
    [window makeKeyAndOrderFront:nil];
    [NSApp runModalForWindow:window];
    [window orderOut:nil];
    [window release];

    [pool release];
}
