//
//  U3RendererLegacy.m
//  Ultima3
//
//  Legacy implementation of the portable renderer boundary.
//

#import "U3Renderer.h"

#import "UltimaDngn.h"
#import "UltimaGraphics.h"
#import "UltimaMain.h"
#import "UltimaText.h"

#include <string.h>

extern int wx, wy;

void U3RenderClearScreen(void) {
    ClearScreen();
}

void U3RenderClearBottom(void) {
    ClearBottom();
}

void U3RenderClearTiles(void) {
    ClearTiles();
}

void U3RenderDrawFrame(int16_t frameID) {
    DrawFrame(frameID);
}

void U3RenderDrawFramePiece(int16_t pieceID, int16_t x, int16_t y) {
    DrawFramePiece(pieceID, x, y);
}

void U3RenderDrawMap(const U3GameState *state, uint8_t x, uint8_t y) {
    (void)state;
    DrawMap(x, y);
}

void U3RenderDrawMiniMap(const U3GameState *state) {
    (void)state;
    DrawMiniMap();
}

void U3RenderDrawDungeon(const U3GameState *state) {
    (void)state;
    DrawDungeon();
}

void U3RenderDrawTiles(const U3GameState *state) {
    (void)state;
    DrawTiles();
}

void U3RenderDrawPrompt(void) {
    DrawPrompt();
}

void U3RenderShowCharacters(const U3GameState *state, bool force) {
    (void)state;
    ShowChars(force);
}

void U3RenderPrintText(const char *text) {
    if (!text)
        return;

    Str255 pascalText;
    size_t length = strlen(text);
    if (length > 255)
        length = 255;
    pascalText[0] = (unsigned char)length;
    memcpy(pascalText + 1, text, length);
    UPrintWin(pascalText);
}

void U3RenderPrintPascalString(uint8_t *text) {
    UPrintWin(text);
}

void U3RenderPrintPascalStringAt(uint8_t *text, int16_t x, int16_t y) {
    UPrint(text, x, y);
}

void U3RenderPrintCharAt(char ch, int16_t x, int16_t y) {
    UPrintChar(ch, x, y);
}

void U3RenderPrintMessage(int16_t messageID) {
    UPrintMessage(messageID);
}

void U3RenderPrintMessageRewrapped(int16_t messageID) {
    UPrintMessageRewrapped(messageID);
}

void U3RenderPrintNumber(int32_t number, int16_t pad) {
    if (pad > 0) {
        U3RenderPrintNumberPadded(number, pad);
    } else {
        U3RenderPrintNumberAt(number, wx, wy);
    }
}

void U3RenderPrintNumberAt(int32_t number, int16_t x, int16_t y) {
    UPrintNum((short)number, (short)x, (short)y);
}

void U3RenderPrintNumberPadded(int32_t number, int16_t pad) {
    UPrintNumPad((short)number, (short)pad);
}

void U3RenderBeginUpdate(void) {
}

void U3RenderEndUpdate(void) {
}

void U3RenderSetCursorForCommand(U3Command command, U3Direction direction) {
    (void)command;
    (void)direction;
}
