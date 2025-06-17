#import "CocoaShunts.h"
#import <Cocoa/Cocoa.h>

void ForceUpdateMain(void) {
    NSWindow *mainWindow = [NSApp mainWindow];
    if (mainWindow) {
        [mainWindow displayIfNeeded];
    } else {
        [NSApp updateWindows];
    }
}

void LWSetArrowCursor(void) {
    [[NSCursor arrowCursor] set];
}
