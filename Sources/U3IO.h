//
//  U3IO.h
//  Ultima3
//
//  Boundary for resource loading, save/load, and data migration.
//

#ifndef U3IO_h
#define U3IO_h

#include "U3GameState.h"
#include "U3Types.h"

typedef enum U3ResourceKind {
    U3ResourceKindMisc = 0,
    U3ResourceKindMap,
    U3ResourceKindParty,
    U3ResourceKindRoster,
    U3ResourceKindMonster,
    U3ResourceKindTalk,
    U3ResourceKindDemo,
    U3ResourceKindPreferences,
    U3ResourceKindSoundResource,
    U3ResourceKindSignature,
    U3ResourceKindConsoleScreen,
    U3ResourceKindImage,
    U3ResourceKindStringTable
} U3ResourceKind;

typedef struct U3DataBuffer {
    const uint8_t *bytes;
    size_t size;
    void *owner;
} U3DataBuffer;

typedef struct U3MutableDataBuffer {
    uint8_t *bytes;
    size_t size;
    void *owner;
} U3MutableDataBuffer;

typedef enum U3SaveContainerOpenResult {
    U3SaveContainerOpenResultFailed = 0,
    U3SaveContainerOpenResultOpened,
    U3SaveContainerOpenResultCreatedEmpty,
    U3SaveContainerOpenResultMigratedLegacy
} U3SaveContainerOpenResult;

U3SaveContainerOpenResult U3IOOpenSaveContainer(void);
int32_t U3IOLastError(void);
void U3IOFlushSaveContainer(void);
bool U3IOLoadResource(U3ResourceKind kind, int16_t resourceID, U3DataBuffer *outBuffer);
void U3IOReleaseResource(U3DataBuffer *buffer);
bool U3IOCreateMutableResource(U3ResourceKind kind, int16_t resourceID, size_t size, const uint8_t *name, U3MutableDataBuffer *outBuffer);
bool U3IOCopyResource(U3ResourceKind kind, int16_t sourceResourceID, int16_t destinationResourceID, const uint8_t *name);
bool U3IOOpenMutableResource(U3ResourceKind kind, int16_t resourceID, U3MutableDataBuffer *outBuffer);
bool U3IOResizeMutableResource(U3MutableDataBuffer *buffer, size_t size);
void U3IOCloseMutableResource(U3MutableDataBuffer *buffer, bool commit);
void U3IOReleaseLegacyResourceHandle(void *resourceHandle);

bool U3IOLoadGame(U3GameState *state);
bool U3IOSaveGame(const U3GameState *state);
bool U3IOLoadRoster(U3GameState *state);
bool U3IOSaveRoster(const U3GameState *state);
bool U3IOLoadWorld(U3GameState *state, int16_t mapID);
bool U3IOSaveWorld(const U3GameState *state);

#endif /* U3IO_h */
