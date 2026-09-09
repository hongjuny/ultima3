//
//  U3Types.h
//  Ultima3
//
//  Portable types shared by the modernization boundary headers.
//

#ifndef U3Types_h
#define U3Types_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct U3Point {
    int16_t x;
    int16_t y;
} U3Point;

typedef struct U3Rect {
    int16_t left;
    int16_t top;
    int16_t right;
    int16_t bottom;
} U3Rect;

typedef enum U3Direction {
    U3DirectionNone = 0,
    U3DirectionNorth,
    U3DirectionSouth,
    U3DirectionWest,
    U3DirectionEast,
    U3DirectionNorthWest,
    U3DirectionNorthEast,
    U3DirectionSouthWest,
    U3DirectionSouthEast
} U3Direction;

typedef enum U3Command {
    U3CommandNone = 0,
    U3CommandPass,
    U3CommandMove,
    U3CommandAttack,
    U3CommandBoard,
    U3CommandCast,
    U3CommandDescend,
    U3CommandEnter,
    U3CommandFire,
    U3CommandGetChest,
    U3CommandHandEquip,
    U3CommandIgnite,
    U3CommandJoinGold,
    U3CommandKlimb,
    U3CommandLook,
    U3CommandModifyOrder,
    U3CommandPeerGem,
    U3CommandSteal,
    U3CommandTransact,
    U3CommandUnlock,
    U3CommandExit,
    U3CommandYell,
    U3CommandStats,
    U3CommandAbort,
    U3CommandQuit
} U3Command;

typedef struct U3InputEvent {
    U3Command command;
    U3Direction direction;
    uint32_t rawKey;
    bool isMouse;
} U3InputEvent;

#endif /* U3Types_h */
