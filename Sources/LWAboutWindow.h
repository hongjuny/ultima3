#import <Cocoa/Cocoa.h>

@interface LWAboutView : NSView
@property (nonatomic, copy) NSString *versionString;
@property (nonatomic, copy) NSString *copyrightString;
@end

@interface LWAboutWindow : NSWindow
@end

void ShowAboutWindow(void);
