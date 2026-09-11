#ifndef U3_BITMAP_H
#define U3_BITMAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Owned, top-down RGBX8 pixels; the fourth byte is ignored for display.
   Initialize to {0} before allocation. */
typedef struct {
    uint8_t *pixels;
    int width;
    int height;
    size_t stride;
} U3Bitmap;

typedef struct {
    int x, y, width, height;
} U3BitmapRect;

bool U3BitmapAllocate(U3Bitmap *bitmap, int width, int height);
bool U3BitmapScroll(U3Bitmap *bitmap, U3BitmapRect rect, int dx, int dy,
                    const uint8_t background[3]);
void U3BitmapDispose(U3Bitmap *bitmap);
/* Nearest-neighbor srcCopy; clips without changing the scaling transform.
   Source pixels outside the bitmap leave the destination unchanged.
   Same-bitmap copies use a snapshot, including overlapping scaled copies. */
bool U3BitmapCopy(U3Bitmap *destination, U3BitmapRect destinationRect,
                  const U3Bitmap *source, U3BitmapRect sourceRect);
/* Binary mask: dark pixels copy the source; light pixels preserve destination. */
bool U3BitmapCopyMasked(U3Bitmap *destination, U3BitmapRect destinationRect,
    const U3Bitmap *source, U3BitmapRect sourceRect,
    const U3Bitmap *mask, U3BitmapRect maskRect);

#endif
