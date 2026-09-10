#include "U3Bitmap.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>

static int pixel(const U3Bitmap *bitmap, int x, int y) {
    return bitmap->pixels[y * bitmap->stride + x * 4];
}

int main(void) {
    U3Bitmap source = {0}, destination = {0};
    assert(!U3BitmapAllocate(&source, 0, 2));
    assert(U3BitmapAllocate(&source, 2, 2));
    assert(U3BitmapAllocate(&destination, 4, 4));
    source.pixels[0] = 1;
    source.pixels[4] = 2;
    source.pixels[source.stride] = 3;
    source.pixels[source.stride + 4] = 4;
    assert(source.pixels[3] == 255);
    uint8_t *original = source.pixels;
    assert(!U3BitmapAllocate(&source, -1, 1));
    assert(source.pixels == original);

    assert(U3BitmapCopy(&destination, (U3BitmapRect){0, 0, 4, 4},
                        &source, (U3BitmapRect){0, 0, 2, 2}));
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            assert(pixel(&destination, x, y) == 1 + x / 2 + 2 * (y / 2));

    assert(U3BitmapCopy(&destination, (U3BitmapRect){-2, -2, 4, 4},
                        &source, (U3BitmapRect){0, 0, 2, 2}));
    assert(pixel(&destination, 0, 0) == 4);
    assert(pixel(&destination, 1, 1) == 4);
    assert(pixel(&destination, 2, 0) == 2);

    assert(U3BitmapCopy(&source, (U3BitmapRect){0, 0, 2, 2},
                        &destination, (U3BitmapRect){0, 0, 4, 4}));
    assert(pixel(&source, 0, 0) == 4);
    assert(pixel(&source, 1, 0) == 2);
    assert(pixel(&source, 0, 1) == 3);

    /* Overlapping enlargement must read the original source, not prior writes. */
    assert(U3BitmapCopy(&destination, (U3BitmapRect){0, 0, 4, 4},
                        &destination, (U3BitmapRect){0, 0, 2, 2}));
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            assert(pixel(&destination, x, y) == 4);

    destination.pixels[0] = 9;
    destination.pixels[4] = 8;
    assert(U3BitmapCopy(&destination, (U3BitmapRect){0, 0, 2, 2},
                        &source, (U3BitmapRect){-1, -1, 2, 2}));
    assert(pixel(&destination, 0, 0) == 9);
    assert(pixel(&destination, 1, 0) == 8);
    assert(pixel(&destination, 1, 1) == 4);
    assert(U3BitmapCopy(&destination, (U3BitmapRect){INT_MAX, INT_MAX, 2, 2},
                        &source, (U3BitmapRect){0, 0, 2, 2}));
    assert(!U3BitmapCopy(&destination, (U3BitmapRect){0, 0, 0, 2},
                         &source, (U3BitmapRect){0, 0, 2, 2}));
    U3Bitmap mask = {0};
    assert(U3BitmapAllocate(&mask, 2, 2));
    mask.pixels[4] = mask.pixels[5] = mask.pixels[6] = 255;
    mask.pixels[8] = mask.pixels[9] = mask.pixels[10] = 255;
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            destination.pixels[y * destination.stride + x * 4] = 99;
    assert(U3BitmapCopyMasked(&destination, (U3BitmapRect){0, 0, 4, 4},
        &source, (U3BitmapRect){0, 0, 2, 2}, &mask, (U3BitmapRect){0, 0, 2, 2}));
    assert(pixel(&destination, 0, 0) == pixel(&source, 0, 0));
    assert(pixel(&destination, 3, 0) == 99);
    assert(pixel(&destination, 0, 3) == 99);
    assert(pixel(&destination, 3, 3) == pixel(&source, 1, 1));
    /* Mask aliases destination; preceding writes must not change later coverage. */
    assert(U3BitmapCopyMasked(&mask, (U3BitmapRect){0, 0, 2, 2},
        &source, (U3BitmapRect){0, 0, 2, 2}, &mask, (U3BitmapRect){0, 0, 1, 1}));
    assert(pixel(&mask, 1, 0) == pixel(&source, 1, 0));
    assert(pixel(&mask, 0, 1) == pixel(&source, 0, 1));
    assert(!U3BitmapCopyMasked(&destination, (U3BitmapRect){0, 0, 4, 4},
        &source, (U3BitmapRect){0, 0, 2, 2}, NULL, (U3BitmapRect){0}));
    U3BitmapDispose(&mask);
    U3BitmapDispose(&source);
    U3BitmapDispose(&destination);
    U3BitmapDispose(&destination);
    puts("bitmap tests passed");
    return 0;
}
