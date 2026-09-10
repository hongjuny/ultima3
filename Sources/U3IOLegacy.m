// Foundation-backed save storage behind the portable I/O boundary.
#import "U3IO.h"
#import "UltimaIncludes.h"
#import <Foundation/Foundation.h>
#include <string.h>

static int32_t sLastError;
static NSMutableDictionary *sResources;
static NSURL *sSaveURL;
static BOOL sInitializing;

static ResType U3LegacyResourceTypeForKind(U3ResourceKind kind) {
    switch (kind) {
        case U3ResourceKindMisc: return (ResType)'MISC';
        case U3ResourceKindMap: return (ResType)'MAPS';
        case U3ResourceKindParty: return (ResType)'PRTY';
        case U3ResourceKindRoster: return (ResType)'ROST';
        case U3ResourceKindMonster: return (ResType)'MONS';
        case U3ResourceKindTalk: return (ResType)'TLKS';
        case U3ResourceKindDemo: return (ResType)'DEMO';
        case U3ResourceKindPreferences: return (ResType)'PREF';
        case U3ResourceKindSoundResource: return (ResType)'snd ';
        case U3ResourceKindSignature: return (ResType)'SGNT';
        case U3ResourceKindConsoleScreen: return (ResType)'CONS';
        case U3ResourceKindImage: return (ResType)'PICT';
        case U3ResourceKindStringTable: return (ResType)'STR#';
    }
    return 0;
}


static NSString *U3ResourceKey(U3ResourceKind kind, int16_t resourceID) {
    ResType type = U3LegacyResourceTypeForKind(kind);
    return type ? [NSString stringWithFormat:@"%08x:%d", (unsigned)type, resourceID] : nil;
}

static NSData *U3BundledResource(U3ResourceKind kind, int16_t resourceID) {
    Handle handle = GetResource(U3LegacyResourceTypeForKind(kind), resourceID);
    if (!handle) {
        sLastError = resNotFound;
        return nil;
    }
    LoadResource(handle);
    NSData *data = *handle ? [NSData dataWithBytes:*handle length:GetHandleSize(handle)] : nil;
    ReleaseResource(handle);
    if (!data)
        sLastError = resNotFound;
    return data;
}

static BOOL U3ValidateResources(NSDictionary *resources) {
    NSDictionary *minimumSizes = @{
        U3ResourceKey(U3ResourceKindRoster, 400): @1280,
        U3ResourceKey(U3ResourceKindParty, 400): @64,
        U3ResourceKey(U3ResourceKindPreferences, 400): @32,
        U3ResourceKey(U3ResourceKindMap, 419): @4096,
        U3ResourceKey(U3ResourceKindMonster, 419): @256};
    for (NSString *key in minimumSizes) {
        if (![resources[key] isKindOfClass:[NSData class]] ||
            [resources[key] length] < [minimumSizes[key] unsignedIntegerValue])
            return NO;
    }
    for (int i = 0; i < 6; ++i) {
        NSData *original = U3BundledResource(U3ResourceKindMisc, 400 + i);
        NSData *saved = resources[U3ResourceKey(U3ResourceKindMisc, 500 + i)];
        if (!original || ![saved isKindOfClass:[NSData class]] || saved.length < original.length)
            return NO;
    }
    return YES;
}

static BOOL U3WriteResources(NSDictionary *resources) {
    if (!U3ValidateResources(resources)) {
        sLastError = paramErr;
        return NO;
    }
    NSError *error = nil;
    NSData *data = [NSPropertyListSerialization dataWithPropertyList:
        @{@"version": @1, @"resources": resources} format:NSPropertyListBinaryFormat_v1_0
        options:0 error:&error];
    BOOL success = data && [data writeToURL:sSaveURL options:NSDataWritingAtomic error:&error];
    sLastError = success ? 0 : (error ? (int32_t)[error code] : ioErr);
    return success;
}

int32_t U3IOLastError(void) { return sLastError; }

U3SaveContainerOpenResult U3IOOpenSaveContainer(void) {
    sLastError = 0;
    [sResources release];
    sResources = nil;
    [sSaveURL release];
    sSaveURL = nil;
    sInitializing = NO;
    NSFileManager *manager = [NSFileManager defaultManager];
    NSURL *directory;
    const char *override = getenv("U3_SAVE_DIRECTORY");
    if (override && *override) {
        directory = [NSURL fileURLWithPath:[NSString stringWithUTF8String:override] isDirectory:YES];
    } else {
        directory = [[manager URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask] firstObject];
        directory = [directory URLByAppendingPathComponent:@"LairWare/Ultima III" isDirectory:YES];
    }
    if (!directory) {
        sLastError = fnfErr;
        return U3SaveContainerOpenResultFailed;
    }
    sSaveURL = [[directory URLByAppendingPathComponent:@"Roster-v1.plist"] retain];
    if ([manager fileExistsAtPath:[sSaveURL path]]) {
        NSError *error = nil;
        NSData *data = [NSData dataWithContentsOfURL:sSaveURL options:0 error:&error];
        id root = data ? [NSPropertyListSerialization propertyListWithData:data
            options:NSPropertyListImmutable format:NULL error:&error] : nil;
        id resources = [root isKindOfClass:[NSDictionary class]] ? root[@"resources"] : nil;
        if (![root isKindOfClass:[NSDictionary class]] ||
            ![root[@"version"] isEqual:@1] || ![resources isKindOfClass:[NSDictionary class]]) {
            sLastError = error ? (int32_t)[error code] : paramErr;
            return U3SaveContainerOpenResultFailed;
        }
        for (id key in resources) {
            if (![key isKindOfClass:[NSString class]] || ![resources[key] isKindOfClass:[NSData class]]) {
                sLastError = paramErr;
                return U3SaveContainerOpenResultFailed;
            }
        }
        if (!U3ValidateResources(resources)) {
            sLastError = paramErr;
            return U3SaveContainerOpenResultFailed;
        }
        sResources = [resources mutableCopy];
        return U3SaveContainerOpenResultOpened;
    }
    // Migration is deliberately explicit: never replace or silently ignore a legacy roster.
    if (!override) {
        NSString *legacy = [NSHomeDirectory() stringByAppendingPathComponent:@"Library/Preferences/Ultima III Roster"];
        NSString *adjacent = [[[[NSBundle mainBundle] bundlePath] stringByDeletingLastPathComponent]
            stringByAppendingPathComponent:@"Roster"];
        if ([manager fileExistsAtPath:legacy] || [manager fileExistsAtPath:adjacent]) {
            sLastError = paramErr;
            fprintf(stderr, "Legacy roster found; migration is required before creating a new save.\n");
            return U3SaveContainerOpenResultFailed;
        }
    }
    NSError *error = nil;
    if (![manager createDirectoryAtURL:directory withIntermediateDirectories:YES attributes:nil error:&error]) {
        sLastError = (int32_t)[error code];
        return U3SaveContainerOpenResultFailed;
    }
    sResources = [[NSMutableDictionary alloc] init];
    sInitializing = YES;
    return U3SaveContainerOpenResultCreatedEmpty;
}

void U3IOFlushSaveContainer(void) {
    if (!sResources) {
        sLastError = paramErr;
        return;
    }
    if (U3WriteResources(sResources))
        sInitializing = NO;
}

bool U3IOLoadResource(U3ResourceKind kind, int16_t resourceID, U3DataBuffer *outBuffer) {
    if (!outBuffer) return false;
    *outBuffer = (U3DataBuffer){0};
    NSString *key = U3ResourceKey(kind, resourceID);
    if (!key) { sLastError = paramErr; return false; }
    NSData *data = sResources[key];
    if (!data) data = U3BundledResource(kind, resourceID);
    if (!data) return false;
    outBuffer->owner = [data retain];
    outBuffer->bytes = [data bytes];
    outBuffer->size = [data length];
    sLastError = 0;
    return true;
}

void U3IOReleaseResource(U3DataBuffer *buffer) {
    if (!buffer) return;
    [(id)buffer->owner release];
    *buffer = (U3DataBuffer){0};
}

static bool U3MutableBuffer(NSString *key, NSMutableData *data, U3MutableDataBuffer *outBuffer) {
    if (!outBuffer || !key || !sResources || !data) {
        sLastError = paramErr;
        return false;
    }
    outBuffer->owner = [[NSDictionary alloc] initWithObjectsAndKeys:key, @"key", data, @"data", nil];
    outBuffer->bytes = [data mutableBytes];
    outBuffer->size = [data length];
    sLastError = 0;
    return true;
}

bool U3IOCreateMutableResource(U3ResourceKind kind, int16_t resourceID, size_t size,
    const uint8_t *name, U3MutableDataBuffer *outBuffer) {
    (void)name;
    if (!outBuffer) return false;
    *outBuffer = (U3MutableDataBuffer){0};
    if (size > 16 * 1024 * 1024) { sLastError = paramErr; return false; }
    return U3MutableBuffer(U3ResourceKey(kind, resourceID),
        [NSMutableData dataWithLength:size], outBuffer);
}

bool U3IOOpenMutableResource(U3ResourceKind kind, int16_t resourceID, U3MutableDataBuffer *outBuffer) {
    if (!outBuffer) return false;
    *outBuffer = (U3MutableDataBuffer){0};
    U3DataBuffer source;
    if (!U3IOLoadResource(kind, resourceID, &source)) return false;
    NSMutableData *data = [NSMutableData dataWithBytes:source.bytes length:source.size];
    U3IOReleaseResource(&source);
    return U3MutableBuffer(U3ResourceKey(kind, resourceID), data, outBuffer);
}

void U3IOCloseMutableResource(U3MutableDataBuffer *buffer, bool commit) {
    if (!buffer) return;
    NSDictionary *owner = (NSDictionary *)buffer->owner;
    if (owner && commit) {
        if (!sResources) {
            sLastError = paramErr;
        } else {
            NSMutableDictionary *candidate = [sResources mutableCopy];
            NSData *snapshot = [owner[@"data"] copy];
            candidate[owner[@"key"]] = snapshot;
            [snapshot release];
            if (sInitializing || U3WriteResources(candidate)) {
                [sResources release];
                sResources = candidate;
                candidate = nil;
                sLastError = 0;
            }
            [candidate release];
        }
    }
    [owner release];
    *buffer = (U3MutableDataBuffer){0};
}

bool U3IOCopyResource(U3ResourceKind kind, int16_t sourceResourceID, int16_t destinationResourceID, const uint8_t *name) {
    U3DataBuffer source;
    U3MutableDataBuffer destination;
    if (!U3IOLoadResource(kind, sourceResourceID, &source)) return false;
    if (!U3IOCreateMutableResource(kind, destinationResourceID, source.size, name, &destination)) {
        U3IOReleaseResource(&source);
        return false;
    }
    memcpy(destination.bytes, source.bytes, source.size);
    U3IOCloseMutableResource(&destination, true);
    U3IOReleaseResource(&source);
    return sLastError == 0;
}

bool U3IOResizeMutableResource(U3MutableDataBuffer *buffer, size_t size) {
    if (!buffer || !buffer->owner || size > 16 * 1024 * 1024) {
        sLastError = paramErr;
        return false;
    }
    NSMutableData *data = [(NSDictionary *)buffer->owner objectForKey:@"data"];
    [data setLength:size];
    buffer->bytes = [data mutableBytes];
    buffer->size = [data length];
    sLastError = 0;
    return true;
}

void U3IOReleaseLegacyResourceHandle(void *resourceHandle) {
    if (resourceHandle) ReleaseResource((Handle)resourceHandle);
}

bool U3IOLoadGame(U3GameState *state) { (void)state; return false; }
bool U3IOSaveGame(const U3GameState *state) { (void)state; return false; }
bool U3IOLoadRoster(U3GameState *state) { (void)state; return false; }
bool U3IOSaveRoster(const U3GameState *state) { (void)state; return false; }
bool U3IOLoadWorld(U3GameState *state, int16_t mapID) { (void)state; (void)mapID; return false; }
bool U3IOSaveWorld(const U3GameState *state) { (void)state; return false; }

bool U3IOSelfTest(void) {
    extern void OpenRstr(void);
    extern Boolean U3CharacterStorageSelfTest(void);
    NSString *directory = [NSTemporaryDirectory() stringByAppendingPathComponent:
        [@"u3-save-test-" stringByAppendingString:[NSUUID UUID].UUIDString]];
    const char *previous = getenv("U3_SAVE_DIRECTORY");
    NSString *savedOverride = previous ? [NSString stringWithUTF8String:previous] : nil;
    setenv("U3_SAVE_DIRECTORY", [directory fileSystemRepresentation], 1);
    bool passed = false;
    U3DataBuffer read = {0};
    U3MutableDataBuffer edit = {0};
    do {
        OpenRstr();
        if (U3IOLastError() || U3IOOpenSaveContainer() != U3SaveContainerOpenResultOpened) break;
        if (!U3CharacterStorageSelfTest()) break;
        if (!U3IOLoadResource(U3ResourceKindRoster, 400, &read) || read.size != 1280) break;
        uint8_t original = read.bytes[0];
        U3IOReleaseResource(&read);
        if (!U3IOOpenMutableResource(U3ResourceKindRoster, 400, &edit)) break;
        edit.bytes[0] = original ^ 255;
        U3IOCloseMutableResource(&edit, false);
        if (!U3IOLoadResource(U3ResourceKindRoster, 400, &read) || read.bytes[0] != original) break;
        U3IOReleaseResource(&read);
        if (!U3IOOpenMutableResource(U3ResourceKindRoster, 400, &edit)) break;
        edit.bytes[0] = original ^ 255;
        U3IOCloseMutableResource(&edit, true);
        if (U3IOLastError() || U3IOOpenSaveContainer() != U3SaveContainerOpenResultOpened) break;
        if (!U3IOLoadResource(U3ResourceKindRoster, 400, &read) || read.bytes[0] != (original ^ 255)) break;
        U3IOReleaseResource(&read);
        if (!U3IOCreateMutableResource(U3ResourceKindMisc, 999, 4, NULL, &edit)) break;
        edit.bytes[0] = 42;
        if (!U3IOResizeMutableResource(&edit, 16) || edit.bytes[0] != 42 || edit.bytes[15] != 0) break;
        U3IOCloseMutableResource(&edit, true);
        if (U3IOLastError()) break;
        NSURL *file = [[sSaveURL retain] autorelease];
        NSData *lastGood = [NSData dataWithContentsOfURL:file];
        // A failed atomic write must leave both the live state and disk unchanged.
        if (!U3IOOpenMutableResource(U3ResourceKindRoster, 400, &edit)) break;
        edit.bytes[0] = original;
        [sSaveURL release];
        sSaveURL = [[[file URLByDeletingLastPathComponent]
            URLByAppendingPathComponent:@"missing-parent/save.plist"] retain];
        U3IOCloseMutableResource(&edit, true);
        if (!U3IOLastError()) break;
        [sSaveURL release]; sSaveURL = [file retain];
        if (![[NSData dataWithContentsOfURL:file] isEqual:lastGood]) break;
        if (!U3IOLoadResource(U3ResourceKindRoster, 400, &read) || read.bytes[0] != (original ^ 255)) break;
        U3IOReleaseResource(&read);
        NSMutableDictionary *invalid = [NSPropertyListSerialization propertyListWithData:lastGood
            options:NSPropertyListMutableContainers format:NULL error:NULL];
        invalid[@"resources"][U3ResourceKey(U3ResourceKindParty, 400)] = [NSMutableData dataWithLength:1];
        NSData *truncated = [NSPropertyListSerialization dataWithPropertyList:invalid
            format:NSPropertyListBinaryFormat_v1_0 options:0 error:NULL];
        if (![truncated writeToURL:file atomically:YES]) break;
        if (U3IOOpenSaveContainer() != U3SaveContainerOpenResultFailed) break;
        if (![[NSData dataWithContentsOfURL:file] isEqual:truncated]) break;
        NSData *corrupt = [@"not a save container" dataUsingEncoding:NSUTF8StringEncoding];
        if (![corrupt writeToURL:file atomically:YES]) break;
        if (U3IOOpenSaveContainer() != U3SaveContainerOpenResultFailed) break;
        if (![[NSData dataWithContentsOfURL:file] isEqual:corrupt]) break;
        passed = true;
    } while (0);
    U3IOReleaseResource(&read);
    U3IOCloseMutableResource(&edit, false);
    [sResources release]; sResources = nil;
    [sSaveURL release]; sSaveURL = nil;
    sInitializing = NO;
    [[NSFileManager defaultManager] removeItemAtPath:directory error:NULL];
    if (savedOverride) setenv("U3_SAVE_DIRECTORY", [savedOverride UTF8String], 1);
    else unsetenv("U3_SAVE_DIRECTORY");
    return passed;
}
