//
//  U3Renderer.h
//  Ultima3
//
//  Boundary for drawing game output. Implementations may use QuickDraw,
//  CoreGraphics, Metal, SDL, a terminal, or another renderer.
//

#ifndef U3Renderer_h
#define U3Renderer_h

#include "U3GameState.h"
#include "U3Types.h"

typedef enum U3RenderSurface {
    U3RenderSurfaceMain = 0,
    U3RenderSurfaceGame,
    U3RenderSurfaceText,
    U3RenderSurfaceUpdate,
    U3RenderSurfaceDungeon,
    U3RenderSurfaceOffscreen
} U3RenderSurface;

void U3RenderClearScreen(void);
void U3RenderClearBottom(void);
void U3RenderClearTiles(void);
void U3RenderDrawFrame(int16_t frameID);
void U3RenderDrawFramePiece(int16_t pieceID, int16_t x, int16_t y);
void U3RenderDrawMap(const U3GameState *state, uint8_t x, uint8_t y);
void U3RenderDrawMiniMap(const U3GameState *state);
void U3RenderDrawDungeon(const U3GameState *state);
void U3RenderDrawTiles(const U3GameState *state);
void U3RenderDrawPrompt(void);
void U3RenderShowCharacters(const U3GameState *state, bool force);

void U3RenderPrintText(const char *text);
void U3RenderPrintPascalString(uint8_t *text);
void U3RenderPrintPascalStringAt(uint8_t *text, int16_t x, int16_t y);
void U3RenderPrintCharAt(char ch, int16_t x, int16_t y);
void U3RenderPrintMessage(int16_t messageID);
void U3RenderPrintMessageRewrapped(int16_t messageID);
void U3RenderPrintNumber(int32_t number, int16_t pad);
void U3RenderPrintNumberAt(int32_t number, int16_t x, int16_t y);
void U3RenderPrintNumberPadded(int32_t number, int16_t pad);

void U3RenderBeginUpdate(void);
void U3RenderEndUpdate(void);
void U3RenderSetCursorForCommand(U3Command command, U3Direction direction);

#endif /* U3Renderer_h */
