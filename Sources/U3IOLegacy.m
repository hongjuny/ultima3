//
//  U3IOLegacy.m
//  Ultima3
//
//  Legacy implementation of the portable I/O boundary.
//

#import "U3IO.h"

#import "UltimaIncludes.h"

#include <string.h>

extern void GetIndString(StringPtr theString, short strListID, short index);
extern OSErr FSMakeFSSpec(short vRefNum, long dirID, ConstStr255Param fileName, FSSpec *spec);
extern OSErr FSpDelete(const FSSpec *spec);
extern void FSpCreateResFile(const FSSpec *spec, OSType creator, OSType fileType, ScriptCode scriptTag);
extern short FSpOpenResFile(const FSSpec *spec, SignedByte permission);
extern OSErr IsAliasFile(const FSSpec *fileFSSpec, Boolean *aliasFileFlag, Boolean *folderFlag);
extern OSErr ResolveAliasFile(FSSpec *theSpec, Boolean resolveAliasChains, Boolean *targetIsFolder, Boolean *wasAliased);
extern OSErr GetEOF(short refNum, long *logEOF);
extern OSErr FSClose(short refNum);

static int32_t sLastError = 0;
static short sRosterRefNum = -1;

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

int32_t U3IOLastError(void) {
    return sLastError;
}

U3SaveContainerOpenResult U3IOOpenSaveContainer(void) {
    Str255 pathStr;
    long error;
    FSSpec fss;
    short prefVRefNum;
    SInt32 prefDirID;

    sLastError = noErr;
    error = FindFolder(kOnSystemDisk, kPreferencesFolderType, kDontCreateFolder, &prefVRefNum, &prefDirID);
    if (error) {
        sLastError = error;
        return U3SaveContainerOpenResultFailed;
    }

    GetIndString(pathStr, BASERES + 11, 2);    // Ultima III Roster
    error = FSMakeFSSpec(prefVRefNum, prefDirID, pathStr, &fss);
    if (error != noErr && error != fnfErr) {
        sLastError = error;
        return U3SaveContainerOpenResultFailed;
    }

    if (error == noErr) {
        Boolean isAlias, isFolder;
        OSErr err = IsAliasFile(&fss, &isAlias, &isFolder);
        if (err == noErr && isAlias)
            ResolveAliasFile(&fss, true, &isFolder, &isAlias);
        sRosterRefNum = FSpOpenResFile(&fss, fsRdWrPerm);
        if (sRosterRefNum == -1) {
            sLastError = ResError();
            return U3SaveContainerOpenResultFailed;
        }
        UseResFile(sRosterRefNum);
        return U3SaveContainerOpenResultOpened;
    }

    FSpDelete(&fss);
    FSpCreateResFile(&fss, 'Ult3', 'RSTR', smSystemScript);
    if (ResError()) {
        sLastError = ResError();
        return U3SaveContainerOpenResultFailed;
    }
    sRosterRefNum = FSpOpenResFile(&fss, fsRdWrPerm);
    if (sRosterRefNum == -1) {
        sLastError = ResError();
        return U3SaveContainerOpenResultFailed;
    }

    GetIndString(pathStr, BASERES + 11, 15);    // old ":Roster"
    FSSpec oldRosterFSS;
    error = FSMakeFSSpec(0, 0, pathStr, &oldRosterFSS);
    if (error == noErr) {
        error = permErr;
        int oldRosterRefNum = FSpOpenResFile(&oldRosterFSS, fsRdPerm);
        if (oldRosterRefNum != -1) {
            long oldNumBytes;
            error = GetEOF(oldRosterRefNum, &oldNumBytes);
            if (error == noErr && oldNumBytes > 0) {
                Ptr buffer = NewPtr(oldNumBytes);
                if (buffer) {
                    error = FSReadFork(oldRosterRefNum, fsFromStart, 0, oldNumBytes, buffer, NULL);
                    if (error == noErr)
                        error = FSWriteFork(sRosterRefNum, fsFromStart, 0, oldNumBytes, buffer, NULL);
                    DisposePtr(buffer);
                }
            }
            error = FSClose(oldRosterRefNum);
        }
        FSClose(sRosterRefNum);
        sRosterRefNum = FSpOpenResFile(&fss, fsRdWrPerm);
        if (sRosterRefNum == -1) {
            sLastError = ResError();
            return U3SaveContainerOpenResultFailed;
        }
        UseResFile(sRosterRefNum);
        if (error == noErr)
            return U3SaveContainerOpenResultMigratedLegacy;
    }

    UseResFile(sRosterRefNum);
    return U3SaveContainerOpenResultCreatedEmpty;
}

void U3IOFlushSaveContainer(void) {
    if (sRosterRefNum != -1)
        UpdateResFile(sRosterRefNum);
}

bool U3IOLoadResource(U3ResourceKind kind, int16_t resourceID, U3DataBuffer *outBuffer) {
    if (!outBuffer)
        return false;

    outBuffer->bytes = NULL;
    outBuffer->size = 0;
    outBuffer->owner = NULL;

    ResType type = U3LegacyResourceTypeForKind(kind);
    if (type == 0) {
        sLastError = paramErr;
        return false;
    }

    Handle handle = GetResource(type, resourceID);
    if (!handle) {
        sLastError = ResError();
        return false;
    }
    LoadResource(handle);
    if (!*handle) {
        sLastError = ResError();
        ReleaseResource(handle);
        return false;
    }

    HLock(handle);
    outBuffer->bytes = (const uint8_t *)*handle;
    outBuffer->size = (size_t)GetHandleSize(handle);
    outBuffer->owner = handle;
    return true;
}

void U3IOReleaseResource(U3DataBuffer *buffer) {
    if (!buffer)
        return;
    if (buffer->owner) {
        HUnlock((Handle)buffer->owner);
        ReleaseResource((Handle)buffer->owner);
    }
    buffer->bytes = NULL;
    buffer->size = 0;
    buffer->owner = NULL;
}

bool U3IOCreateMutableResource(U3ResourceKind kind, int16_t resourceID, size_t size, const uint8_t *name, U3MutableDataBuffer *outBuffer) {
    if (!outBuffer)
        return false;

    outBuffer->bytes = NULL;
    outBuffer->size = 0;
    outBuffer->owner = NULL;

    ResType type = U3LegacyResourceTypeForKind(kind);
    if (type == 0) {
        sLastError = paramErr;
        return false;
    }

    Handle handle = NewHandleClear((Size)size);
    if (!handle) {
        sLastError = MemError();
        return false;
    }

    AddResource(handle, type, resourceID, name);
    if (ResError()) {
        sLastError = ResError();
        DisposeHandle(handle);
        return false;
    }

    outBuffer->bytes = (uint8_t *)*handle;
    outBuffer->size = (size_t)GetHandleSize(handle);
    outBuffer->owner = handle;
    return true;
}

bool U3IOCopyResource(U3ResourceKind kind, int16_t sourceResourceID, int16_t destinationResourceID, const uint8_t *name) {
    U3DataBuffer sourceBuffer;
    U3MutableDataBuffer destinationBuffer;

    if (!U3IOLoadResource(kind, sourceResourceID, &sourceBuffer))
        return false;
    if (!U3IOCreateMutableResource(kind, destinationResourceID, sourceBuffer.size, name, &destinationBuffer)) {
        U3IOReleaseResource(&sourceBuffer);
        return false;
    }
    memcpy(destinationBuffer.bytes, sourceBuffer.bytes, sourceBuffer.size);
    U3IOCloseMutableResource(&destinationBuffer, true);
    U3IOReleaseResource(&sourceBuffer);
    return true;
}

bool U3IOOpenMutableResource(U3ResourceKind kind, int16_t resourceID, U3MutableDataBuffer *outBuffer) {
    if (!outBuffer)
        return false;

    outBuffer->bytes = NULL;
    outBuffer->size = 0;
    outBuffer->owner = NULL;

    ResType type = U3LegacyResourceTypeForKind(kind);
    if (type == 0) {
        sLastError = paramErr;
        return false;
    }

    Handle handle = GetResource(type, resourceID);
    if (!handle) {
        sLastError = ResError();
        return false;
    }
    LoadResource(handle);
    if (!*handle) {
        sLastError = ResError();
        ReleaseResource(handle);
        return false;
    }

    outBuffer->bytes = (uint8_t *)*handle;
    outBuffer->size = (size_t)GetHandleSize(handle);
    outBuffer->owner = handle;
    return true;
}

void U3IOCloseMutableResource(U3MutableDataBuffer *buffer, bool commit) {
    if (!buffer)
        return;
    if (buffer->owner) {
        Handle handle = (Handle)buffer->owner;
        if (commit) {
            ChangedResource(handle);
            WriteResource(handle);
        }
        ReleaseResource(handle);
    }
    buffer->bytes = NULL;
    buffer->size = 0;
    buffer->owner = NULL;
}

void U3IOReleaseLegacyResourceHandle(void *resourceHandle) {
    if (resourceHandle)
        ReleaseResource((Handle)resourceHandle);
}

bool U3IOResizeMutableResource(U3MutableDataBuffer *buffer, size_t size) {
    if (!buffer || !buffer->owner)
        return false;
    Handle handle = (Handle)buffer->owner;
    SetHandleSize(handle, (Size)size);
    if (MemError()) {
        sLastError = MemError();
        return false;
    }
    buffer->bytes = (uint8_t *)*handle;
    buffer->size = (size_t)GetHandleSize(handle);
    return true;
}

bool U3IOLoadGame(U3GameState *state) {
    (void)state;
    return false;
}

bool U3IOSaveGame(const U3GameState *state) {
    (void)state;
    return false;
}

bool U3IOLoadRoster(U3GameState *state) {
    (void)state;
    return false;
}

bool U3IOSaveRoster(const U3GameState *state) {
    (void)state;
    return false;
}

bool U3IOLoadWorld(U3GameState *state, int16_t mapID) {
    (void)state;
    (void)mapID;
    return false;
}

bool U3IOSaveWorld(const U3GameState *state) {
    (void)state;
    return false;
}
