//
//  PrefsDialogController.h
//  Ultima3
//

#import <Cocoa/Cocoa.h>
#import "LWCocoaDialogController.h"

@interface PrefsDialogController : LWCocoaDialogController {
	IBOutlet NSPopUpButton *mThemesButton;
	IBOutlet NSPopUpButton *mFontsButton;
	IBOutlet NSTextField   *mHealThresholdField;
}

- (IBAction) useClassicDefaults:(id)sender;

- (IBAction) useModernDefaults:(id)sender;

@end

