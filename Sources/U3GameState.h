//
//  U3GameState.h
//  Ultima3
//
//  Portable container for game/domain state. This begins as a mirror of the
//  legacy global memory layout so extraction can proceed without changing
//  behavior.
//

#ifndef U3GameState_h
#define U3GameState_h

#include "U3Types.h"

enum {
    U3PlayerSlotCount = 21,
    U3PlayerRecordSize = 65,
    U3PartySize = 64,
    U3MonsterStateSize = 256,
    U3TalkStateSize = 256,
    U3DungeonStateSize = 2048,
    U3MacroSize = 32,
    U3TileArraySize = 128,
    U3ZeroPageSize = 255
};

typedef struct U3GameState {
    uint8_t player[U3PlayerSlotCount][U3PlayerRecordSize];
    uint8_t oldPlayer[U3PlayerSlotCount][U3PlayerRecordSize];
    /* Version-1 snapshot field: keep its serialized size independent of the
       one-based legacy Party array, which includes an extra padding byte. */
    uint8_t party[U3PartySize];
    uint8_t monsters[U3MonsterStateSize];
    uint8_t talk[U3TalkStateSize];
    uint8_t dungeon[U3DungeonStateSize];
    uint8_t macro[U3MacroSize];
    uint8_t tileArray[U3TileArraySize];
    int16_t zeroPage[U3ZeroPageSize];

    uint8_t careerTable[12];
    uint8_t weaponUseTable[12];
    uint8_t armourUseTable[12];
    uint8_t moonXTable[8];
    uint8_t moonYTable[8];
    uint8_t locationX[20];
    uint8_t locationY[20];
    uint8_t experience[17];

    int32_t x;
    int32_t y;
    int32_t sourceX;
    int32_t sourceY;
    int32_t deltaX;
    int32_t deltaY;

    int16_t currentMapID;
    int16_t currentMapSize;
    int32_t mapOffset;
    int16_t torchTurns;
    int16_t timeNegateTurns;
    int16_t moon[2];
    int16_t moonDisplay[2];
    int8_t dungeonLevel;
    int16_t heading;
    int16_t exitDungeon;
} U3GameState;

#endif /* U3GameState_h */
