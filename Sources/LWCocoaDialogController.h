/*
 *  LWCocoaDialogController.h
 *  Ultima3
 *
 */

#import <Cocoa/Cocoa.h>


@interface LWCocoaDialogController : NSObject {
	IBOutlet NSObjectController		*mController;
	IBOutlet NSWindow				*mWindow;
}

- (IBAction) terminateWithTagAsCode:(id)sender;

- (NSObjectController *) controller;
- (NSWindow *) window;
- (void) validateChangedValueForKey:(NSString *)keyPath;

@end
