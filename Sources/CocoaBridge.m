//
//  CocoaBridge.m
//  Ultima3
//

#import "CocoaBridge.h"

#import "CarbonShunts.h"
#import "LWCocoaDialogController.h"
#import "UltimaIncludes.h"
#import "U3Platform.h"

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreText/CoreText.h>

extern short gUpdateWhere;
extern void HandleSpecialChoice(int theItem);

@class U3MainSurfaceView;

static short sQTSoundVolume = 100;    // was 254 but let's make sounds quieter.
static NSMutableArray *sEffectPlayers = nil;
static NSWindow *sU3MainSurfaceWindow = nil;
static U3MainSurfaceView *sU3MainSurfaceView = nil;
static U3Bitmap sU3MainBitmap;
static NSColor *sU3ForegroundColor = nil;
static NSColor *sU3BackgroundColor = nil;
static NSPoint sU3PenLocation = {0.0, 0.0};
static short sU3TextFont = 0;
static short sU3TextSize = 12;
static short sU3TextFace = 0;
static char sU3MainSurfaceToken;
static U3Bitmap *sU3SelectedBitmap;
static NSPoint sU3BitmapOrigin;
static BOOL sU3HeadlessSurface = NO;
static char sU3DiagnosticKey = 0;
static unsigned char sU3DiagnosticKeys[256];
static unsigned int sU3DiagnosticKeyIndex, sU3DiagnosticKeyCount;
static NSMutableArray *sU3PendingInput = nil;
static Point sU3MousePoint = {0, 0};
static Boolean sU3DiagnosticMousePending = false;
static void U3CocoaReplayCommands(NSArray *commands);

static NSFont *U3CocoaTextFont(CGFloat size, NSInteger face) {
    char name[256] = {0};
    NSFont *font = nil;
    if (U3PlatformCopyUTF8StringPreference(U3PreferenceGameFont, name, sizeof(name)))
        font = [NSFont fontWithName:[NSString stringWithUTF8String:name] size:size];
    if (!font) font = [NSFont userFixedPitchFontOfSize:size];
    if (!font) font = [NSFont systemFontOfSize:size];
    if (face & bold)
        font = [[NSFontManager sharedFontManager] convertFont:font toHaveTrait:NSBoldFontMask];
    return font;
}

static NSString *U3CocoaStringFromLegacyBytes(const unsigned char *bytes, NSUInteger length) {
    if (!bytes || !length)
        return @"";
    NSString *string = [[[NSString alloc] initWithBytes:bytes length:length
                                               encoding:NSMacOSRomanStringEncoding] autorelease];
    if (!string)
        string = [[[NSString alloc] initWithBytes:bytes length:length
                                           encoding:NSUTF8StringEncoding] autorelease];
    return string ? string : @"";
}

static char U3CocoaKeyCharacter(NSEvent *event) {
    if ([event modifierFlags] & NSCommandKeyMask) return 0;
    switch ([event keyCode]) {
        case 36: case 76: return 13;
        case 51: case 117: return 8;
        case 53: return 27;
        case 123: return 28; // left arrow
        case 124: return 29; // right arrow
        case 126: return 30; // up arrow
        case 125: return 31; // down arrow
        default: break;
    }
    NSString *characters = [event charactersIgnoringModifiers];
    unichar character = [characters length] ? [characters characterAtIndex:0] : 0;
    if (character > 0 && character < 128)
        return character == 127 ? 8 : (char)character;
    // Keep commands usable with non-Latin input sources, without truncating Unicode.
    static const char physicalKeys[] = "asdfhgzxcv\0bqweryt123465=97-80]ou[ip\rlj'k;\\,/nm.\t `";
    unsigned short code = [event keyCode];
    return code < sizeof(physicalKeys) - 1 ? physicalKeys[code] : 0;
}

Boolean U3CocoaIsHeadlessDiagnostic(void) {
    if (getenv("U3_VERIFY_NATIVE_INPUT")) return false;
    return getenv("U3_BOOT_CHECK") || getenv("U3_WORLD_RENDER_CHECK") ||
           getenv("U3_WORLD_INPUT_CHECK") || getenv("U3_WORLD_MOUSE_CHECK") ||
           getenv("U3_AUDIO_SELF_TEST") || getenv("U3_PARTY_FLOW_CHECK") ||
           getenv("U3_MAIN_MENU_INPUT_CHECK");
}

@interface U3MainSurfaceView : NSView
@end

@interface U3PartyPicker : NSObject {
@public
    NSPopUpButton *choices[4];
    NSButton *formButton;
}
- (void)selectionChanged:(id)sender;
@end

@interface U3CharacterEditor : NSObject <NSTextFieldDelegate> {
@public
    NSTextField *name, *remaining, *values[4];
    NSStepper *stats[4];
    NSPopUpButton *slot, *race, *career, *sex;
    NSButton *create;
}
- (void)changed:(id)sender;
- (BOOL)readDraft:(U3CharacterDraft *)draft;
@end

@implementation U3CharacterEditor
- (BOOL)readDraft:(U3CharacterDraft *)draft {
    memset(draft, 0, sizeof(*draft));
    NSData *encoded = [[name stringValue] dataUsingEncoding:NSMacOSRomanStringEncoding allowLossyConversion:NO];
    if (!encoded || encoded.length > 12) return NO;
    memcpy(draft->name, encoded.bytes, encoded.length);
    draft->race = [[race selectedItem] tag];
    draft->characterClass = [[career selectedItem] tag];
    draft->sex = [[sex selectedItem] tag];
    for (int i = 0; i < 4; ++i) draft->attributes[i] = [stats[i] integerValue];
    return U3ValidateCharacterDraft(draft);
}
- (void)changed:(id)sender {
    (void)sender;
    NSInteger sum = 0;
    for (int i = 0; i < 4; ++i) {
        NSInteger value = [stats[i] integerValue];
        sum += value;
        [values[i] setIntegerValue:value];
    }
    [remaining setStringValue:[NSString stringWithFormat:@"Points remaining: %ld", (long)(50 - sum)]];
    U3CharacterDraft draft;
    [create setEnabled:[self readDraft:&draft]];
}
- (void)controlTextDidChange:(NSNotification *)notification { [self changed:notification]; }
@end

@implementation U3PartyPicker
- (void)selectionChanged:(id)sender {
    (void)sender;
    BOOL valid = YES, any = NO;
    for (int i = 0; i < 4; ++i) {
        NSInteger selected = [[choices[i] selectedItem] tag];
        any |= selected != 0;
        for (int j = 0; j < i; ++j)
            if (selected && selected == [[choices[j] selectedItem] tag])
                valid = NO;
    }
    [formButton setEnabled:valid && any];
}
@end

@implementation U3MainSurfaceView

- (BOOL)acceptsFirstResponder { return YES; }

- (BOOL)isFlipped {
    return YES;
}

- (void)drawRect:(NSRect)dirtyRect {
    [[NSColor blackColor] setFill];
    NSRectFill(dirtyRect);

    U3Bitmap *bitmap = &sU3MainBitmap;
    if (!bitmap->pixels)
        return;
    CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(bitmap->pixels, bitmap->width,
        bitmap->height, 8, bitmap->stride, space,
        kCGImageAlphaNoneSkipLast | kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(space);
    if (!context)
        return;
    CGImageRef snapshot = CGBitmapContextCreateImage(context);
    CGContextRef display = [[NSGraphicsContext currentContext] CGContext];
    CGContextSaveGState(display);
    CGContextSetInterpolationQuality(display, kCGInterpolationNone);
    CGContextTranslateCTM(display, 0, bitmap->height);
    CGContextScaleCTM(display, 1, -1);
    CGContextDrawImage(display, CGRectMake(0, 0, bitmap->width, bitmap->height), snapshot);
    CGContextRestoreGState(display);
    CGImageRelease(snapshot);
    CGContextRelease(context);
}

@end

static void U3CocoaReplayCommands(NSArray *commands) {
    for (NSDictionary *command in commands) {
        NSString *kind = [command objectForKey:@"kind"];
        NSColor *color = [command objectForKey:@"color"];
        if (!color)
            color = [NSColor whiteColor];

        if ([kind isEqualToString:@"fill"]) {
            [color setFill];
            NSRectFill([[command objectForKey:@"rect"] rectValue]);
        } else if ([kind isEqualToString:@"frame"]) {
            [color setStroke];
            [NSBezierPath strokeRect:[[command objectForKey:@"rect"] rectValue]];
        } else if ([kind isEqualToString:@"bitmap"]) {
            NSImage *image = [command objectForKey:@"image"];
            [[NSGraphicsContext currentContext] setImageInterpolation:NSImageInterpolationNone];
            [image drawInRect:[[command objectForKey:@"rect"] rectValue]
                    fromRect:NSZeroRect operation:NSCompositingOperationSourceOver
                    fraction:1.0 respectFlipped:YES hints:nil];
        } else if ([kind isEqualToString:@"text"]) {
            NSString *text = [command objectForKey:@"text"];
            CGFloat size = [[command objectForKey:@"size"] doubleValue];
            NSInteger face = [[command objectForKey:@"face"] integerValue];
            NSFont *font = U3CocoaTextFont(size, face);

            NSDictionary *attributes = [NSDictionary dictionaryWithObjectsAndKeys:
                color, NSForegroundColorAttributeName,
                font, NSFontAttributeName,
                nil];
            NSPoint point = [[command objectForKey:@"point"] pointValue];
            NSAttributedString *attributed = [[[NSAttributedString alloc]
                initWithString:text attributes:attributes] autorelease];
            CTLineRef line = CTLineCreateWithAttributedString((CFAttributedStringRef)attributed);
            CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
            CGContextSaveGState(context);
            CGContextSetTextDrawingMode(context, kCGTextFill);
            CGContextSetTextMatrix(context, CGAffineTransformMakeScale(1, -1));
            CGContextSetTextPosition(context, point.x, point.y);
            CTLineDraw(line, context);
            CGContextRestoreGState(context);
            CFRelease(line);
        }
    }
}

static void U3CocoaEnsureDrawState(void) {
    if (!sU3ForegroundColor)
        sU3ForegroundColor = [[NSColor whiteColor] retain];
    if (!sU3BackgroundColor)
        sU3BackgroundColor = [[NSColor blackColor] retain];
}

void U3CocoaInvalidateMainSurface(void) {
    if (sU3MainSurfaceView)
        [sU3MainSurfaceView setNeedsDisplay:YES];
}

static NSColor *U3CocoaColorFromRGB16(UInt16 red, UInt16 green, UInt16 blue) {
    return [NSColor colorWithDeviceRed:(CGFloat)red / 65535.0
                                     green:(CGFloat)green / 65535.0
                                      blue:(CGFloat)blue / 65535.0
                                     alpha:1.0];
}

static NSColor *U3CocoaColorFromQuickDraw(long color) {
    switch (color) {
        case blackColor:
            return [NSColor blackColor];
        case whiteColor:
            return [NSColor whiteColor];
        case redColor:
            return [NSColor redColor];
        case greenColor:
            return [NSColor greenColor];
        case blueColor:
            return [NSColor blueColor];
        case cyanColor:
            return [NSColor cyanColor];
        case magentaColor:
            return [NSColor magentaColor];
        case yellowColor:
            return [NSColor yellowColor];
        default:
            return [NSColor whiteColor];
    }
}

static NSRect U3CocoaRectFromQuickDraw(short left, short top, short right, short bottom) {
    return NSMakeRect(left, top, MAX(0, right - left), MAX(0, bottom - top));
}

static void U3CocoaReplaceColor(NSColor **slot, NSColor *color) {
    if (*slot == color)
        return;
    [color retain];
    [*slot release];
    *slot = color;
}

static void U3CocoaAddCommand(NSDictionary *command) {
    U3CocoaEnsureDrawState();
    U3Bitmap *bitmap = sU3SelectedBitmap ? sU3SelectedBitmap : &sU3MainBitmap;
    if (bitmap->pixels) {
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef context = CGBitmapContextCreate(bitmap->pixels, bitmap->width,
            bitmap->height, 8, bitmap->stride, colorSpace,
            kCGImageAlphaNoneSkipLast | kCGBitmapByteOrder32Big);
        CGColorSpaceRelease(colorSpace);
        if (!context)
            return;
        CGContextTranslateCTM(context, 0, bitmap->height);
        CGContextScaleCTM(context, 1, -1);
        if (sU3SelectedBitmap)
            CGContextTranslateCTM(context, -sU3BitmapOrigin.x, -sU3BitmapOrigin.y);
        [NSGraphicsContext saveGraphicsState];
        [NSGraphicsContext setCurrentContext:
            [NSGraphicsContext graphicsContextWithCGContext:context flipped:YES]];
        U3CocoaReplayCommands(@[command]);
        [NSGraphicsContext restoreGraphicsState];
        CGContextRelease(context);
    }
    if (!sU3SelectedBitmap)
        U3CocoaInvalidateMainSurface();
}

U3Bitmap *U3CocoaMainBitmap(void) {
    return sU3MainBitmap.pixels ? &sU3MainBitmap : NULL;
}

Boolean U3CocoaWriteMainBitmap(const char *path) {
    U3Bitmap *bitmap = U3CocoaMainBitmap();
    if (!path || !bitmap)
        return false;
    NSBitmapImageRep *rep = [[NSBitmapImageRep alloc]
        initWithBitmapDataPlanes:NULL pixelsWide:bitmap->width pixelsHigh:bitmap->height
        bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES isPlanar:NO
        colorSpaceName:NSDeviceRGBColorSpace bitmapFormat:NSBitmapFormatAlphaNonpremultiplied
        bytesPerRow:bitmap->stride bitsPerPixel:32];
    if (!rep)
        return false;
    memcpy([rep bitmapData], bitmap->pixels, bitmap->stride * bitmap->height);
    for (size_t i = 3; i < bitmap->stride * bitmap->height; i += 4)
        [rep bitmapData][i] = 255;
    Boolean result = [[rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}]
        writeToFile:[NSString stringWithUTF8String:path] atomically:YES];
    [rep release];
    return result;
}

Boolean U3CocoaResizeMainBitmap(short width, short height) {
    if (sU3MainBitmap.width == width && sU3MainBitmap.height == height)
        return true;
    U3Bitmap replacement = {0};
    if (!U3BitmapAllocate(&replacement, width, height))
        return false;
    if (sU3MainBitmap.pixels)
        U3BitmapCopy(&replacement, (U3BitmapRect){0, 0, sU3MainBitmap.width, sU3MainBitmap.height},
            &sU3MainBitmap, (U3BitmapRect){0, 0, sU3MainBitmap.width, sU3MainBitmap.height});
    U3BitmapDispose(&sU3MainBitmap);
    sU3MainBitmap = replacement;
    return true;
}

void U3CocoaSelectBitmap(U3Bitmap *bitmap, short originX, short originY) {
    sU3SelectedBitmap = bitmap;
    sU3BitmapOrigin = NSMakePoint(originX, originY);
}

Boolean U3CocoaLoadImage(U3Bitmap *output, CFURLRef url, int width, int height,
                        int columns, int rows) {
    if (!output || !url || width <= 0 || height <= 0 || width > 4095 ||
        height > 32767 || columns <= 0 || rows <= 0 ||
        width % columns || height % rows)
        return false;
    @autoreleasepool {
        if ([[[(NSURL *)url pathExtension] lowercaseString] isEqualToString:@"pdf"]) {
            CGPDFDocumentRef document = CGPDFDocumentCreateWithURL(url);
            CGPDFPageRef page = document ? CGPDFDocumentGetPage(document, 1) : NULL;
            U3Bitmap bitmap = {0};
            Boolean success = page && U3BitmapAllocate(&bitmap, width, height);
            if (success) {
                CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
                CGContextRef context = CGBitmapContextCreate(bitmap.pixels, width, height,
                    8, bitmap.stride, space,
                    kCGImageAlphaNoneSkipLast | kCGBitmapByteOrder32Big);
                CGColorSpaceRelease(space);
                success = context != NULL;
                if (context) {
                    CGAffineTransform transform = CGPDFPageGetDrawingTransform(page,
                        kCGPDFMediaBox, CGRectMake(0, 0, width, height), 0, false);
                    CGContextConcatCTM(context, transform);
                    CGContextDrawPDFPage(context, page);
                    CGContextRelease(context);
                }
            }
            if (document)
                CGPDFDocumentRelease(document);
            if (!success) {
                U3BitmapDispose(&bitmap);
                return false;
            }
            U3BitmapDispose(output);
            *output = bitmap;
            return true;
        }
        NSImage *image = [[NSImage alloc] initWithContentsOfURL:(NSURL *)url];
        if (!image)
            return false;
        NSRect proposed = NSMakeRect(0, 0, width, height);
        CGImageRef decoded = [image CGImageForProposedRect:&proposed context:nil hints:nil];
        if (!decoded) {
            [image release];
            return false;
        }
        size_t sourceWidth = CGImageGetWidth(decoded);
        size_t sourceHeight = CGImageGetHeight(decoded);
        if (sourceWidth < (size_t)columns || sourceHeight < (size_t)rows) {
            [image release];
            return false;
        }
        U3Bitmap bitmap = {0};
        if (!U3BitmapAllocate(&bitmap, width, height)) {
            [image release];
            return false;
        }
        CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
        CGContextRef context = CGBitmapContextCreate(bitmap.pixels, width, height, 8,
            bitmap.stride, space, kCGImageAlphaNoneSkipLast | kCGBitmapByteOrder32Big);
        CGColorSpaceRelease(space);
        Boolean success = context != NULL;
        if (context) {
            CGContextSetInterpolationQuality(context, kCGInterpolationNone);
            int tileWidth = width / columns, tileHeight = height / rows;
            for (int row = 0; row < rows && success; ++row) {
                for (int column = 0; column < columns; ++column) {
                    size_t left = sourceWidth * column / columns;
                    size_t top = sourceHeight * row / rows;
                    CGRect source = CGRectMake(left, top,
                        sourceWidth * (column + 1) / columns - left,
                        sourceHeight * (row + 1) / rows - top);
                    CGImageRef tile = CGImageCreateWithImageInRect(decoded, source);
                    if (!tile) {
                        success = false;
                        break;
                    }
                    /* Bitmap storage is top-down; Quartz user space is bottom-up. */
                    CGContextDrawImage(context, CGRectMake(column * tileWidth,
                        height - (row + 1) * tileHeight, tileWidth, tileHeight), tile);
                    CGImageRelease(tile);
                }
            }
            CGContextRelease(context);
        }
        [image release];
        if (!success) {
            U3BitmapDispose(&bitmap);
            return false;
        }
        U3BitmapDispose(output);
        *output = bitmap;
        return true;
    }
}

Boolean U3CocoaImageSelfTest(void) {
    NSBitmapImageRep *fixture = [[NSBitmapImageRep alloc]
        initWithBitmapDataPlanes:NULL pixelsWide:2 pixelsHigh:2 bitsPerSample:8
        samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace
        bitmapFormat:NSBitmapFormatAlphaNonpremultiplied bytesPerRow:8 bitsPerPixel:32];
    const unsigned char pixels[] = {255,0,0,255, 0,255,0,255,
                                    0,0,255,255, 255,255,255,255};
    memcpy([fixture bitmapData], pixels, sizeof(pixels));
    NSURL *url = [NSURL fileURLWithPath:[NSTemporaryDirectory()
        stringByAppendingPathComponent:[[NSUUID UUID].UUIDString stringByAppendingString:@".png"]]];
    Boolean success = [[fixture representationUsingType:NSBitmapImageFileTypePNG properties:@{}]
        writeToURL:url atomically:YES];
    [fixture release];
    U3Bitmap bitmap = {0};
    success = success && U3CocoaLoadImage(&bitmap, (CFURLRef)url, 6, 4, 2, 2);
    if (success) {
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 6; ++x) {
                const unsigned char *expected = pixels + ((y / 2) * 2 + x / 3) * 4;
                const unsigned char *actual = bitmap.pixels + y * bitmap.stride + x * 4;
                for (int channel = 0; channel < 3; ++channel)
                    if (abs(actual[channel] - expected[channel]) > 1)
                        success = false;
            }
        }
    }
    [[NSFileManager defaultManager] removeItemAtURL:url error:NULL];
    success = !U3CocoaLoadImage(&bitmap, (CFURLRef)url, 6, 4, 2, 2) && success;
    U3BitmapDispose(&bitmap);
    for (NSString *name in @[@"Exodus.png", @"Shrine.jpg", @"Commands.pdf"]) {
        NSURL *asset = [(NSURL *)ResourcesDirectoryURL() URLByAppendingPathComponent:name];
        Boolean loaded = U3CocoaLoadImage(&bitmap, (CFURLRef)asset, 128, 128, 1, 1);
        Boolean nonblack = false;
        if (loaded) {
            for (size_t i = 0; i < bitmap.stride * bitmap.height; i += 4)
                if (bitmap.pixels[i] || bitmap.pixels[i+1] || bitmap.pixels[i+2])
                    nonblack = true;
        }
        fprintf(stderr, "Image asset %s: %s\n", [name UTF8String],
                loaded && nonblack ? "passed" : "FAILED");
        success = success && loaded && nonblack;
        U3BitmapDispose(&bitmap);
    }
    NSURL *previewURL = [(NSURL *)ResourcesDirectoryURL() URLByAppendingPathComponent:@"Exodus.png"];
    Boolean previewPassed = U3CocoaResizeMainBitmap(384, 256) &&
        U3CocoaLoadImage(&bitmap, (CFURLRef)previewURL, 384, 256, 1, 1);
    if (previewPassed) {
        U3CocoaDrawBitmap(&bitmap, (U3BitmapRect){0, 0, 384, 256}, 0, 0, 384, 256);
        NSBitmapImageRep *rendered = [[NSBitmapImageRep alloc]
            initWithBitmapDataPlanes:NULL pixelsWide:384 pixelsHigh:256 bitsPerSample:8
            samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace
            bitmapFormat:NSBitmapFormatAlphaNonpremultiplied bytesPerRow:384 * 4 bitsPerPixel:32];
        U3MainSurfaceView *view = [[U3MainSurfaceView alloc] initWithFrame:NSMakeRect(0, 0, 384, 256)];
        [NSGraphicsContext saveGraphicsState];
        CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
        CGContextRef context = CGBitmapContextCreate([rendered bitmapData], 384, 256, 8,
            384 * 4, space, kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
        CGColorSpaceRelease(space);
        CGContextTranslateCTM(context, 0, 256);
        CGContextScaleCTM(context, 1, -1);
        [NSGraphicsContext setCurrentContext:
            [NSGraphicsContext graphicsContextWithCGContext:context flipped:YES]];
        [view drawRect:[view bounds]];
        [NSGraphicsContext restoreGraphicsState];
        CGContextRelease(context);
        unsigned char *actual = [rendered bitmapData];
        for (size_t i = 0; i < bitmap.stride * bitmap.height; ++i)
            if (i % 4 != 3 && abs(actual[i] - bitmap.pixels[i]) > 2)
                previewPassed = false;
        if (getenv("U3_RENDER_PREVIEW"))
            previewPassed = [[rendered representationUsingType:NSBitmapImageFileTypePNG properties:@{}]
                writeToFile:[NSString stringWithUTF8String:getenv("U3_RENDER_PREVIEW")]
                atomically:YES] && previewPassed;
        [view release];
        [rendered release];
    }
    U3BitmapDispose(&bitmap);
    fprintf(stderr, "Cocoa surface pixel test: %s\n", previewPassed ? "passed" : "FAILED");
    success = success && previewPassed;
    return success;
}

void U3CocoaDrawBitmap(const U3Bitmap *bitmap, U3BitmapRect source,
                       short x, short y, short width, short height) {
    if (U3BitmapCopy(&sU3MainBitmap, (U3BitmapRect){x, y, width, height}, bitmap, source))
        U3CocoaInvalidateMainSurface();
}

static NSString *U3CocoaStringFromPascal(ConstStr255Param text) {
    if (!text)
        return @"";

    NSUInteger length = text[0];
    return U3CocoaStringFromLegacyBytes(text + 1, length);
}

void U3CocoaSetForegroundQuickDrawColor(long color) {
    U3CocoaEnsureDrawState();
    U3CocoaReplaceColor(&sU3ForegroundColor, U3CocoaColorFromQuickDraw(color));
}

void U3CocoaSetBackgroundQuickDrawColor(long color) {
    U3CocoaEnsureDrawState();
    U3CocoaReplaceColor(&sU3BackgroundColor, U3CocoaColorFromQuickDraw(color));
}

void U3CocoaSetForegroundRGB(UInt16 red, UInt16 green, UInt16 blue) {
    U3CocoaEnsureDrawState();
    U3CocoaReplaceColor(&sU3ForegroundColor, U3CocoaColorFromRGB16(red, green, blue));
}

void U3CocoaSetBackgroundRGB(UInt16 red, UInt16 green, UInt16 blue) {
    U3CocoaEnsureDrawState();
    U3CocoaReplaceColor(&sU3BackgroundColor, U3CocoaColorFromRGB16(red, green, blue));
}

void U3CocoaSetTextFont(short font) {
    sU3TextFont = font;
}

void U3CocoaSetTextSize(short size) {
    sU3TextSize = size > 0 ? size : 12;
}

void U3CocoaSetTextFace(short face) {
    sU3TextFace = face;
}

void U3CocoaMoveTo(short h, short v) {
    sU3PenLocation = NSMakePoint(h, v);
}

void U3CocoaGetPen(Point *point) {
    if (point) {
        point->h = (short)sU3PenLocation.x;
        point->v = (short)sU3PenLocation.y;
    }
}

void U3CocoaGetBackground(uint8_t color[3]) {
    U3CocoaEnsureDrawState();
    NSColor *rgb = [sU3BackgroundColor colorUsingColorSpace:[NSColorSpace deviceRGBColorSpace]];
    color[0] = (uint8_t)([rgb redComponent] * 255);
    color[1] = (uint8_t)([rgb greenComponent] * 255);
    color[2] = (uint8_t)([rgb blueComponent] * 255);
}

void U3CocoaPaintRect(short left, short top, short right, short bottom) {
    U3CocoaEnsureDrawState();
    U3CocoaAddCommand([NSDictionary dictionaryWithObjectsAndKeys:
        @"fill", @"kind",
        sU3ForegroundColor, @"color",
        [NSValue valueWithRect:U3CocoaRectFromQuickDraw(left, top, right, bottom)], @"rect",
        nil]);
}

void U3CocoaEraseRect(short left, short top, short right, short bottom) {
    U3CocoaEnsureDrawState();
    U3CocoaAddCommand([NSDictionary dictionaryWithObjectsAndKeys:
        @"fill", @"kind",
        sU3BackgroundColor, @"color",
        [NSValue valueWithRect:U3CocoaRectFromQuickDraw(left, top, right, bottom)], @"rect",
        nil]);
}

void U3CocoaFrameRect(short left, short top, short right, short bottom) {
    U3CocoaEnsureDrawState();
    U3CocoaAddCommand([NSDictionary dictionaryWithObjectsAndKeys:
        @"frame", @"kind",
        sU3ForegroundColor, @"color",
        [NSValue valueWithRect:U3CocoaRectFromQuickDraw(left, top, right, bottom)], @"rect",
        nil]);
}

void U3CocoaDrawPascalString(ConstStr255Param text) {
    U3CocoaEnsureDrawState();
    NSString *string = U3CocoaStringFromPascal(text);
    if ([string length] == 0)
        return;

    U3CocoaAddCommand([NSDictionary dictionaryWithObjectsAndKeys:
        @"text", @"kind",
        sU3ForegroundColor, @"color",
        string, @"text",
        [NSValue valueWithPoint:sU3PenLocation], @"point",
        [NSNumber numberWithShort:sU3TextSize], @"size",
        [NSNumber numberWithShort:sU3TextFace], @"face",
        [NSNumber numberWithShort:sU3TextFont], @"font",
        nil]);
    sU3PenLocation.x += U3CocoaTextWidth(text);
}

short U3CocoaTextWidth(ConstStr255Param text) {
    NSAttributedString *string = [[[NSAttributedString alloc]
        initWithString:U3CocoaStringFromPascal(text)
        attributes:@{NSFontAttributeName: U3CocoaTextFont(sU3TextSize, sU3TextFace)}] autorelease];
    CTLineRef line = CTLineCreateWithAttributedString((CFAttributedStringRef)string);
    double width = CTLineGetTypographicBounds(line, NULL, NULL, NULL);
    CFRelease(line);
    return (short)MIN(32767, ceil(width));
}

void U3CocoaDrawBytes(const void *textBuf, short firstByte, short byteCount) {
    if (!textBuf || byteCount <= 0)
        return;

    const unsigned char *bytes = (const unsigned char *)textBuf + MAX(0, firstByte);
    Str255 pstring;
    short length = MIN(byteCount, 255);
    pstring[0] = (unsigned char)length;
    memcpy(pstring + 1, bytes, (size_t)length);
    U3CocoaDrawPascalString(pstring);
}

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
        if (U3CocoaIsHeadlessDiagnostic())
            return;
        NSAutoreleasePool *myPool = [[NSAutoreleasePool alloc] init];
        NSApplication *application = [NSApplication sharedApplication];
        [application setActivationPolicy:NSApplicationActivationPolicyRegular];
        [myPool release];
    }
}

void *U3CocoaCreateMainSurface(short xposn, short yposn, short width, short height) {
    if (!U3CocoaResizeMainBitmap(width, height))
        return NULL;
    if (U3CocoaIsHeadlessDiagnostic()) {
        sU3HeadlessSurface = YES;
        return &sU3MainSurfaceToken;
    }
    CocoaInit();
    NSAutoreleasePool *myPool = [[NSAutoreleasePool alloc] init];

    if (!sU3MainSurfaceWindow) {
        NSScreen *screen = [NSScreen mainScreen];
        CGFloat screenHeight = screen ? [screen frame].size.height : 900.0;
        NSRect contentRect = NSMakeRect((CGFloat)xposn,
                                        screenHeight - ((CGFloat)yposn + (CGFloat)height),
                                        (CGFloat)width,
                                        (CGFloat)height);
        NSUInteger styleMask = NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask;
        sU3MainSurfaceWindow = [[NSWindow alloc] initWithContentRect:contentRect
                                                           styleMask:styleMask
                                                             backing:NSBackingStoreBuffered
                                                               defer:NO];
        [sU3MainSurfaceWindow setTitle:@"Ultima III"];
        [sU3MainSurfaceWindow setReleasedWhenClosed:NO];
        sU3MainSurfaceView = [[U3MainSurfaceView alloc] initWithFrame:NSMakeRect(0, 0, width, height)];
        [sU3MainSurfaceWindow setContentView:sU3MainSurfaceView];
        [sU3MainSurfaceView release];
    } else {
        [sU3MainSurfaceWindow setContentSize:NSMakeSize(width, height)];
        [sU3MainSurfaceView setFrameSize:NSMakeSize(width, height)];
    }

    [sU3MainSurfaceWindow center];
    [sU3MainSurfaceWindow makeKeyAndOrderFront:nil];
    [NSApp finishLaunching];
    [NSApp activateIgnoringOtherApps:YES];
    U3CocoaPumpEvents();

    [myPool release];
    return &sU3MainSurfaceToken;
}

void U3CocoaReactivateMainSurface(void) {
    if (U3CocoaIsHeadlessDiagnostic() || !sU3MainSurfaceWindow)
        return;
    CocoaInit();
    [NSApp finishLaunching];
    [NSApp activateIgnoringOtherApps:YES];
    [sU3MainSurfaceWindow makeKeyAndOrderFront:nil];
    [sU3MainSurfaceWindow makeFirstResponder:sU3MainSurfaceView];
    U3CocoaPumpEvents();
}

@interface U3CocoaMenuTarget : NSObject
@end

@implementation U3CocoaMenuTarget
- (void)saveGame:(id)sender {
    (void)sender;
    U3CocoaQueueDiagnosticKey('q');
}
- (void)handleSpecialMenuItem:(id)sender {
    if ([sender respondsToSelector:@selector(tag)])
        HandleSpecialChoice((int)[sender tag]);
}
@end

void U3CocoaInstallMenus(void) {
    if (U3CocoaIsHeadlessDiagnostic())
        return;
    CocoaInit();
    NSMenu *bar = [[[NSMenu alloc] initWithTitle:@""] autorelease];
    NSMenuItem *appItem = [[[NSMenuItem alloc] initWithTitle:@"Ultima III" action:NULL keyEquivalent:@""] autorelease];
    NSMenu *appMenu = [[[NSMenu alloc] initWithTitle:@"Ultima III"] autorelease];
    NSMenuItem *quit = [appMenu addItemWithTitle:@"Quit Ultima III" action:@selector(terminate:) keyEquivalent:@"q"];
    [quit setTarget:NSApp];
    [appItem setSubmenu:appMenu];
    [bar addItem:appItem];

    static U3CocoaMenuTarget *target = nil;
    if (!target) target = [[U3CocoaMenuTarget alloc] init];
    NSMenuItem *fileItem = [[[NSMenuItem alloc] initWithTitle:@"File" action:NULL keyEquivalent:@""] autorelease];
    NSMenu *fileMenu = [[[NSMenu alloc] initWithTitle:@"File"] autorelease];
    NSMenuItem *save = [fileMenu addItemWithTitle:@"Save Game" action:@selector(saveGame:) keyEquivalent:@"s"];
    [save setTarget:target];
    [fileItem setSubmenu:fileMenu];
    [bar addItem:fileItem];

    NSMenuItem *specialItem = [[[NSMenuItem alloc] initWithTitle:@"Special" action:NULL keyEquivalent:@""] autorelease];
    NSMenu *specialMenu = [[[NSMenu alloc] initWithTitle:@"Special"] autorelease];
    NSArray *specialTitles = @[@"Sound", @"Music", @"Speech", @"Constrain Speed",
                               @"Auto-Combat", @"Double Size", @"-", @"Full Screen"];
    for (NSUInteger index = 0; index < [specialTitles count]; ++index) {
        if (index == 6) {
            [specialMenu addItem:[NSMenuItem separatorItem]];
            continue;
        }
        NSMenuItem *item = [specialMenu addItemWithTitle:[specialTitles objectAtIndex:index]
                                                   action:@selector(handleSpecialMenuItem:)
                                            keyEquivalent:@""];
        [item setTag:(NSInteger)index + 1];
        [item setTarget:target];
    }
    [specialItem setSubmenu:specialMenu];
    [bar addItem:specialItem];
    [NSApp setMainMenu:bar];
}

static Boolean U3CocoaIsGameInput(NSEvent *event) {
    if ([NSApp modalWindow] || [event window] != sU3MainSurfaceWindow) return false;
    switch ([event type]) {
        case NSKeyDown: return U3CocoaKeyCharacter(event) != 0;
        case NSLeftMouseDown: case NSRightMouseDown: case NSOtherMouseDown: return true;
        default: return false;
    }
}

void U3CocoaPumpEvents(void) {
    if (!NSApp)
        return;

    NSDate *limitDate = [NSDate dateWithTimeIntervalSinceNow:0];
    NSEvent *event;
    while ((event = [NSApp nextEventMatchingMask:NSAnyEventMask
                                       untilDate:limitDate
                                          inMode:NSDefaultRunLoopMode
                                         dequeue:YES])) {
        if (U3CocoaIsGameInput(event)) {
            if (!sU3PendingInput) sU3PendingInput = [[NSMutableArray alloc] init];
            [sU3PendingInput addObject:event];
        } else {
            [NSApp sendEvent:event];
        }
    }
    [NSApp updateWindows];
}

void U3CocoaPresentMainSurface(void) {
    if (!sU3HeadlessSurface) [sU3MainSurfaceView displayIfNeeded];
}

void U3CocoaFlushInput(void) {
    U3CocoaPumpEvents();
    [sU3PendingInput removeAllObjects];
    sU3DiagnosticKey = 0;
    sU3DiagnosticMousePending = false;
    sU3DiagnosticKeyIndex = sU3DiagnosticKeyCount = 0;
}

Boolean U3CocoaHasMainSurface(void) {
    return sU3MainSurfaceWindow != nil || sU3HeadlessSurface;
}

Boolean U3CocoaUsesNativeUI(void) {
    return true;
}

void U3CocoaRunApplication(void) {
    if (!U3CocoaHasMainSurface() || sU3HeadlessSurface)
        return;

    CocoaInit();
    [sU3MainSurfaceWindow makeKeyAndOrderFront:nil];
    [NSApp finishLaunching];
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp run];
}

Boolean U3CocoaPollKeyMouse(Boolean includeMouse, long timeoutTicks, char *outKey,
                            Boolean *outMouse) {
    if (outMouse)
        *outMouse = false;
    if (sU3HeadlessSurface) {
        if (sU3DiagnosticMousePending) {
            sU3DiagnosticMousePending = false;
            if (outMouse)
                *outMouse = true;
            return includeMouse;
        }
        if (outKey)
            *outKey = sU3DiagnosticKey ? sU3DiagnosticKey :
                (sU3DiagnosticKeyIndex < sU3DiagnosticKeyCount ? sU3DiagnosticKeys[sU3DiagnosticKeyIndex++] : 0);
        sU3DiagnosticKey = 0;
        return outKey && *outKey != 0;
    }
    CocoaInit();
    NSAutoreleasePool *myPool = [[NSAutoreleasePool alloc] init];

    if (outKey)
        *outKey = 0;

    NSTimeInterval timeoutSeconds = timeoutTicks > 0 ? ((NSTimeInterval)timeoutTicks / 60.0) : 0.0;
    NSDate *limitDate = [NSDate dateWithTimeIntervalSinceNow:timeoutSeconds];
    Boolean handledInput = false;
    NSEvent *event = nil;
    if ([sU3PendingInput count]) {
        event = [[[sU3PendingInput objectAtIndex:0] retain] autorelease];
        [sU3PendingInput removeObjectAtIndex:0];
    } else event = [NSApp nextEventMatchingMask:NSAnyEventMask
                                       untilDate:limitDate
                                          inMode:NSDefaultRunLoopMode
                                         dequeue:YES];
    if (event && U3CocoaIsGameInput(event)) {
        switch ([event type]) {
            case NSKeyDown: {
                char character = U3CocoaKeyCharacter(event);
                if (character != 0) {
                    if (outKey)
                        *outKey = character;
                    handledInput = true;
                }
                break;
            }
            case NSLeftMouseDown:
            case NSRightMouseDown:
            case NSOtherMouseDown:
                if (outMouse)
                    *outMouse = true;
                if ([event window]) {
                    NSPoint location = [event locationInWindow];
                    CGFloat height = [[[event window] contentView] bounds].size.height;
                    sU3MousePoint.h = (short)location.x;
                    sU3MousePoint.v = (short)(height - location.y);
                }
                handledInput = includeMouse;
                break;
            default:
                break;
        }
    }
    if (event && !handledInput && !U3CocoaIsGameInput(event)) [NSApp sendEvent:event];
    [NSApp updateWindows];

    [myPool release];
    return handledInput;
}

void U3CocoaGetMousePoint(Point *point) {
    if (point)
        *point = sU3MousePoint;
}

void U3CocoaQueueDiagnosticKey(char key) {
    if (sU3HeadlessSurface) {
        sU3DiagnosticMousePending = false;
        sU3DiagnosticKey = key;
        return;
    }
    NSString *characters = [NSString stringWithFormat:@"%c", key];
    NSEvent *event = [NSEvent keyEventWithType:NSKeyDown location:NSZeroPoint
        modifierFlags:0 timestamp:0 windowNumber:[sU3MainSurfaceWindow windowNumber]
        context:nil characters:characters charactersIgnoringModifiers:characters
        isARepeat:NO keyCode:49];
    [NSApp postEvent:event atStart:YES];
}

void U3CocoaQueueDiagnosticKeys(const char *keys) {
    size_t count = keys ? strlen(keys) : 0;
    if (!sU3HeadlessSurface) {
        for (size_t i = 0; i < count; ++i) {
            NSString *characters = [NSString stringWithFormat:@"%c", keys[i]];
            NSEvent *event = [NSEvent keyEventWithType:NSKeyDown location:NSZeroPoint
                modifierFlags:0 timestamp:0 windowNumber:[sU3MainSurfaceWindow windowNumber]
                context:nil characters:characters charactersIgnoringModifiers:characters
                isARepeat:NO keyCode:49];
            [NSApp postEvent:event atStart:NO];
        }
        return;
    }
    if (count > sizeof(sU3DiagnosticKeys)) count = sizeof(sU3DiagnosticKeys);
    memcpy(sU3DiagnosticKeys, keys ? keys : "", count);
    sU3DiagnosticKeyIndex = 0;
    sU3DiagnosticKeyCount = (unsigned int)count;
}

void U3CocoaTextCheckpoint(const char *name) {
    const char *prefix = getenv("U3_COMMAND_TEXT_CHECK");
    if (!prefix) return;
    char path[1024];
    int length = snprintf(path, sizeof(path), "%s-%s.png", prefix, name);
    if (length < 0 || length >= sizeof(path) || !U3CocoaWriteMainBitmap(path))
        exit(EXIT_FAILURE);
    fprintf(stderr, "Command text: %s\n", name);
}

Boolean U3CocoaKeyboardSelfTest(void) {
    struct { unsigned short code; NSString *text; NSUInteger flags; char expected; } cases[] = {
        {17, @"t", 0, 't'}, {17, @"T", NSShiftKeyMask, 'T'},
        {17, @"\u3145", 0, 't'}, {18, @"1", 0, '1'},
        {123, @"\uf702", 0, 28}, {124, @"\uf703", 0, 29},
        {126, @"\uf700", 0, 30}, {125, @"\uf701", 0, 31},
        {51, @"\x7f", 0, 8}, {76, @"\x03", 0, 13},
        {53, @"\x1b", 0, 27}, {1, @"s", NSCommandKeyMask, 0},
        {122, @"\uf704", 0, 0}
    };
    for (unsigned int i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        NSEvent *event = [NSEvent keyEventWithType:NSKeyDown location:NSZeroPoint
            modifierFlags:cases[i].flags timestamp:0 windowNumber:0 context:nil
            characters:cases[i].text charactersIgnoringModifiers:cases[i].text
            isARepeat:NO keyCode:cases[i].code];
        if (U3CocoaKeyCharacter(event) != cases[i].expected) return false;
    }
    return true;
}

void U3CocoaQueueDiagnosticMouse(short x, short y) {
    sU3MousePoint.h = x;
    sU3MousePoint.v = y;
    if (sU3HeadlessSurface) {
        sU3DiagnosticKey = 0;
        sU3DiagnosticMousePending = true;
        return;
    }
    if (!sU3MainSurfaceWindow)
        return;
    NSPoint location = NSMakePoint(x, [[sU3MainSurfaceWindow contentView] bounds].size.height - y);
    NSEvent *event = [NSEvent mouseEventWithType:NSLeftMouseDown location:location
        modifierFlags:0 timestamp:0 windowNumber:[sU3MainSurfaceWindow windowNumber]
        context:nil eventNumber:0 clickCount:1 pressure:1.0];
    [NSApp postEvent:event atStart:YES];
}

Boolean U3CocoaChooseParty(const unsigned char names[20][16], const Boolean available[20], short selection[4]) {
    if (!names || !available || !selection)
        return false;
    @autoreleasepool {
        BOOL any = NO;
        for (int i = 0; i < 20; ++i)
            any |= available[i];
        NSAlert *alert = [[[NSAlert alloc] init] autorelease];
        if (!any) {
            [alert setMessageText:@"No available characters"];
            [alert setInformativeText:@"Create a character before forming a party."];
            [alert addButtonWithTitle:@"OK"];
            [alert runModal];
            U3CocoaReactivateMainSurface();
            return false;
        }
        [alert setMessageText:@"Form a Party"];
        [alert addButtonWithTitle:@"Form Party"];
        [alert addButtonWithTitle:@"Cancel"];
        U3PartyPicker *picker = [[[U3PartyPicker alloc] init] autorelease];
        picker->formButton = [[alert buttons] objectAtIndex:0];
        NSView *accessory = [[[NSView alloc] initWithFrame:NSMakeRect(0, 0, 360, 152)] autorelease];
        for (int slot = 0; slot < 4; ++slot) {
            NSTextField *label = [NSTextField labelWithString:[NSString stringWithFormat:@"Member %d", slot + 1]];
            [label setFrame:NSMakeRect(0, 116 - slot * 36, 90, 24)];
            [accessory addSubview:label];
            NSPopUpButton *choice = [[[NSPopUpButton alloc] initWithFrame:
                NSMakeRect(96, 116 - slot * 36, 264, 28) pullsDown:NO] autorelease];
            picker->choices[slot] = choice;
            [choice setAutoenablesItems:NO];
            [choice addItemWithTitle:@"None"];
            [[choice lastItem] setTag:0];
            for (int i = 0; i < 20; ++i) {
                NSUInteger length = MIN(names[i][0], 15);
                if (!length) continue;
                NSString *name = U3CocoaStringFromLegacyBytes(names[i] + 1, length);
                [choice addItemWithTitle:[NSString stringWithFormat:@"%d. %@", i + 1, name]];
                [[choice lastItem] setTag:i + 1];
                [[choice lastItem] setEnabled:available[i]];
            }
            [choice setTarget:picker];
            [choice setAction:@selector(selectionChanged:)];
            [accessory addSubview:choice];
        }
        [picker selectionChanged:nil];
        [alert setAccessoryView:accessory];
        if ([alert runModal] != NSAlertFirstButtonReturn) {
            U3CocoaReactivateMainSurface();
            return false;
        }
        for (int i = 0; i < 4; ++i)
            selection[i] = (short)[[picker->choices[i] selectedItem] tag];
        U3CocoaReactivateMainSurface();
        return true;
    }
}

Boolean U3CocoaCreateCharacter(const Boolean available[20], short *outSlot, U3CharacterDraft *draft) {
    if (!available || !outSlot || !draft) return false;
    @autoreleasepool {
        NSAlert *alert = [[[NSAlert alloc] init] autorelease];
        BOOL any = NO;
        for (int i = 0; i < 20; ++i) any |= available[i];
        if (!any) {
            [alert setMessageText:@"Roster is full"];
            [alert addButtonWithTitle:@"OK"];
            [alert runModal];
            return false;
        }
        [alert setMessageText:@"Create a Character"];
        [alert addButtonWithTitle:@"Create"];
        [alert addButtonWithTitle:@"Cancel"];
        U3CharacterEditor *editor = [[[U3CharacterEditor alloc] init] autorelease];
        editor->create = [[alert buttons] objectAtIndex:0];
        NSView *content = [[[NSView alloc] initWithFrame:NSMakeRect(0, 0, 380, 360)] autorelease];
        NSArray *labels = @[@"Roster slot", @"Name", @"Sex", @"Race", @"Class",
            @"Strength", @"Dexterity", @"Intelligence", @"Wisdom"];
        for (int row = 0; row < 9; ++row) {
            CGFloat y = 326 - row * 35;
            NSTextField *label = [NSTextField labelWithString:labels[row]];
            [label setFrame:NSMakeRect(0, y, 110, 24)];
            [content addSubview:label];
            if (row == 1) {
                editor->name = [[[NSTextField alloc] initWithFrame:NSMakeRect(116, y, 260, 24)] autorelease];
                [editor->name setDelegate:editor];
                [content addSubview:editor->name];
            } else if (row < 5) {
                NSPopUpButton *popup = [[[NSPopUpButton alloc] initWithFrame:NSMakeRect(116, y, 260, 26) pullsDown:NO] autorelease];
                [popup setTarget:editor]; [popup setAction:@selector(changed:)];
                if (row == 0) {
                    editor->slot = popup;
                    for (int i = 0; i < 20; ++i) if (available[i]) {
                        [popup addItemWithTitle:[NSString stringWithFormat:@"%d", i + 1]];
                        [[popup lastItem] setTag:i + 1];
                    }
                } else {
                    NSArray *names;
                    const char *codes;
                    if (row == 2) { editor->sex = popup; names = @[@"Female", @"Male", @"Other"]; codes = "FMO"; }
                    else if (row == 3) { editor->race = popup; names = (NSArray *)StringsArray(CFSTR("Races")); codes = "HEDBF"; }
                    else { editor->career = popup; names = (NSArray *)StringsArray(CFSTR("Classes")); codes = "FCWTPBLIDAR"; }
                    for (NSUInteger i = 0; i < MIN(names.count, strlen(codes)); ++i) {
                        [popup addItemWithTitle:names[i]];
                        [[popup lastItem] setTag:codes[i]];
                    }
                }
                [content addSubview:popup];
            } else {
                int index = row - 5;
                editor->values[index] = [NSTextField labelWithString:@""];
                [editor->values[index] setFrame:NSMakeRect(116, y, 60, 24)];
                [content addSubview:editor->values[index]];
                NSStepper *stepper = [[[NSStepper alloc] initWithFrame:NSMakeRect(182, y - 1, 20, 28)] autorelease];
                editor->stats[index] = stepper;
                [stepper setMinValue:5]; [stepper setMaxValue:25]; [stepper setIncrement:1];
                [stepper setValueWraps:NO]; [stepper setIntegerValue:draft->attributes[index]];
                [stepper setTarget:editor]; [stepper setAction:@selector(changed:)];
                [content addSubview:stepper];
            }
        }
        [editor->race selectItemWithTag:draft->race];
        [editor->career selectItemWithTag:draft->characterClass];
        [editor->sex selectItemWithTag:draft->sex];
        editor->remaining = [NSTextField labelWithString:@""];
        [editor->remaining setFrame:NSMakeRect(116, 5, 260, 24)];
        [content addSubview:editor->remaining];
        [editor changed:nil];
        [alert setAccessoryView:content];
        [[alert window] setInitialFirstResponder:editor->name];
        if ([alert runModal] != NSAlertFirstButtonReturn) return false;
        U3CharacterDraft result;
        if (![editor readDraft:&result]) return false;
        *draft = result;
        *outSlot = (short)[[editor->slot selectedItem] tag];
        return true;
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
    if (U3CocoaIsHeadlessDiagnostic())
        return true;
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

void PlaySoundFileQT(CFStringRef soundName, Boolean async) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if (U3CocoaIsHeadlessDiagnostic()) {
        [pool release];
        return;
    }
    static NSString *sSoundsDirectory = nil;
    if (!sSoundsDirectory)
        sSoundsDirectory = [[[NSBundle mainBundle] pathForResource:@"SoundsPCM" ofType:nil] retain];
    if (sSoundsDirectory) {
        NSString *key = [(NSString *)soundName copy];
        static NSArray *sSoundFiles = nil;
        if (!sSoundFiles)
            sSoundFiles = [[[NSFileManager defaultManager] subpathsAtPath:sSoundsDirectory] retain];
        if (!sEffectPlayers)
            sEffectPlayers = [[NSMutableArray alloc] init];
        for (NSInteger i = [sEffectPlayers count] - 1; i >= 0; --i) {
            AVAudioPlayer *oldPlayer = [sEffectPlayers objectAtIndex:i];
            if (![oldPlayer isPlaying])
                [sEffectPlayers removeObjectAtIndex:i];
        }

        NSString *prefix = [(NSString *)soundName stringByAppendingString:@"."];
        NSString *targetFile = nil;
        for (NSString *filename in sSoundFiles) {
            if ([filename hasPrefix:prefix]) {
                targetFile = [sSoundsDirectory stringByAppendingPathComponent:filename];
                break;
            }
        }
        if (targetFile) {
            NSError *error = nil;
            AVAudioPlayer *player = [[AVAudioPlayer alloc]
                initWithContentsOfURL:[NSURL fileURLWithPath:targetFile] error:&error];
            if (player && [player prepareToPlay]) {
                [player setVolume:MIN(1.0f, (float)sQTSoundVolume / 100.0f)];
                [sEffectPlayers addObject:player];
                [player play];
                CFAbsoluteTime timeout = CFAbsoluteTimeGetCurrent() + 8.0;
                if (!async) {
                    while ([player isPlaying] && CFAbsoluteTimeGetCurrent() < timeout) {
                        if (U3CocoaHasMainSurface())
                            U3CocoaPumpEvents();
                        else {
                            EventRecord theEvent;
                            WaitNextEvent(everyEvent, &theEvent, 6, nil);
                            if (theEvent.what == kHighLevelEvent)
                                AEProcessAppleEvent(&theEvent);
                        }
                        [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.05]];
                    }
                }
            } else if (error) {
                NSLog(@"Cannot play sound '%@': %@", key, [error localizedDescription]);
            }
            [player release];
        } else {
            NSLog(@"Cannot find sound '%@'", key);
        }
        [key release];
    }
    [pool release];
}

void SetSoundVolumePercent(short newVolume) {
    if (newVolume >= 0 && newVolume <= 100) {
        short newQTVolume = (short)((float)newVolume * 1.5);    // was 2.55 but sfx are so loud compared to music.
        if (newQTVolume != sQTSoundVolume) {
            sQTSoundVolume = newQTVolume;
            for (AVAudioPlayer *player in sEffectPlayers)
                [player setVolume:MIN(1.0f, (float)sQTSoundVolume / 100.0f)];
        }
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
                U3CocoaReactivateMainSurface();
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
