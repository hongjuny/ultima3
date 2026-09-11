#include "U3Bitmap.h"

#include <stdlib.h>
#include <string.h>

void U3BitmapDispose(U3Bitmap *bitmap) {
    if (!bitmap)
        return;
    free(bitmap->pixels);
    *bitmap = (U3Bitmap){0};
}

bool U3BitmapAllocate(U3Bitmap *bitmap, int width, int height) {
    if (!bitmap || width <= 0 || height <= 0 ||
        (size_t)width > SIZE_MAX / 4 ||
        (size_t)height > SIZE_MAX / ((size_t)width * 4))
        return false;
    size_t stride = (size_t)width * 4;
    uint8_t *pixels = calloc((size_t)height, stride);
    if (!pixels)
        return false;
    for (size_t offset = 3; offset < (size_t)height * stride; offset += 4)
        pixels[offset] = 255;
    U3BitmapDispose(bitmap);
    *bitmap = (U3Bitmap){pixels, width, height, stride};
    return true;
}

bool U3BitmapScroll(U3Bitmap *bitmap, U3BitmapRect rect, int dx, int dy,
                    const uint8_t background[3]) {
    if (!bitmap || !bitmap->pixels || !background || rect.width <= 0 || rect.height <= 0)
        return false;
    int64_t left = rect.x > 0 ? rect.x : 0;
    int64_t top = rect.y > 0 ? rect.y : 0;
    int64_t right = (int64_t)rect.x + rect.width;
    int64_t bottom = (int64_t)rect.y + rect.height;
    if (right > bitmap->width) right = bitmap->width;
    if (bottom > bitmap->height) bottom = bitmap->height;
    if (right <= left || bottom <= top || (!dx && !dy)) return true;
    U3Bitmap snapshot = {0};
    if (!U3BitmapAllocate(&snapshot, (int)(right - left), (int)(bottom - top)))
        return false;
    for (int y = 0; y < snapshot.height; ++y)
        memcpy(snapshot.pixels + y * snapshot.stride,
               bitmap->pixels + (top + y) * bitmap->stride + left * 4, snapshot.stride);
    for (int64_t y = top; y < bottom; ++y) {
        for (int64_t x = left; x < right; ++x) {
            int64_t sx = x - left - dx, sy = y - top - dy;
            uint8_t *pixel = bitmap->pixels + y * bitmap->stride + x * 4;
            if (sx >= 0 && sx < snapshot.width && sy >= 0 && sy < snapshot.height)
                memcpy(pixel, snapshot.pixels + sy * snapshot.stride + sx * 4, 4);
            else {
                memcpy(pixel, background, 3);
                pixel[3] = 255;
            }
        }
    }
    U3BitmapDispose(&snapshot);
    return true;
}

static bool U3BitmapCopyInternal(U3Bitmap *destination, U3BitmapRect destinationRect,
                  const U3Bitmap *source, U3BitmapRect sourceRect,
                  const U3Bitmap *mask, U3BitmapRect maskRect) {
    if (!destination || !source || !destination->pixels || !source->pixels ||
        destinationRect.width <= 0 || destinationRect.height <= 0 ||
        sourceRect.width <= 0 || sourceRect.height <= 0)
        return false;
    if (mask && (!mask->pixels || maskRect.width <= 0 || maskRect.height <= 0))
        return false;

    int64_t left = destinationRect.x > 0 ? destinationRect.x : 0;
    int64_t top = destinationRect.y > 0 ? destinationRect.y : 0;
    int64_t right = (int64_t)destinationRect.x + destinationRect.width;
    int64_t bottom = (int64_t)destinationRect.y + destinationRect.height;
    if (right > destination->width) right = destination->width;
    if (bottom > destination->height) bottom = destination->height;
    if (left >= right || top >= bottom)
        return true;

    U3Bitmap snapshot = {0}, maskSnapshot = {0};
    if (mask && mask->pixels == destination->pixels) {
        if (!U3BitmapAllocate(&maskSnapshot, mask->width, mask->height))
            return false;
        for (int y = 0; y < mask->height; ++y)
            memcpy(maskSnapshot.pixels + y * maskSnapshot.stride,
                   mask->pixels + y * mask->stride, maskSnapshot.stride);
        mask = &maskSnapshot;
    }
    if (destination->pixels == source->pixels) {
        if (!U3BitmapAllocate(&snapshot, source->width, source->height)) {
            U3BitmapDispose(&maskSnapshot);
            return false;
        }
        for (int y = 0; y < source->height; ++y)
            memcpy(snapshot.pixels + y * snapshot.stride,
                   source->pixels + y * source->stride, snapshot.stride);
        source = &snapshot;
    }

    for (int64_t y = top; y < bottom; ++y) {
        int64_t sy = sourceRect.y +
            (y - destinationRect.y) * sourceRect.height / destinationRect.height;
        if (sy < 0 || sy >= source->height)
            continue;
        for (int64_t x = left; x < right; ++x) {
            int64_t sx = sourceRect.x +
                (x - destinationRect.x) * sourceRect.width / destinationRect.width;
            if (sx < 0 || sx >= source->width)
                continue;
            if (mask) {
                int64_t mx = maskRect.x + (x - destinationRect.x) * maskRect.width / destinationRect.width;
                int64_t my = maskRect.y + (y - destinationRect.y) * maskRect.height / destinationRect.height;
                if (mx < 0 || my < 0 || mx >= mask->width || my >= mask->height)
                    continue;
                const uint8_t *pixel = mask->pixels + (size_t)my * mask->stride + (size_t)mx * 4;
                if (pixel[0] + pixel[1] + pixel[2] >= 384)
                    continue;
            }
            memcpy(destination->pixels + (size_t)y * destination->stride + (size_t)x * 4,
                   source->pixels + (size_t)sy * source->stride + (size_t)sx * 4, 4);
        }
    }
    U3BitmapDispose(&snapshot);
    U3BitmapDispose(&maskSnapshot);
    return true;
}

bool U3BitmapCopy(U3Bitmap *destination, U3BitmapRect destinationRect,
                  const U3Bitmap *source, U3BitmapRect sourceRect) {
    return U3BitmapCopyInternal(destination, destinationRect, source, sourceRect,
                                NULL, (U3BitmapRect){0});
}

bool U3BitmapCopyMasked(U3Bitmap *destination, U3BitmapRect destinationRect,
    const U3Bitmap *source, U3BitmapRect sourceRect,
    const U3Bitmap *mask, U3BitmapRect maskRect) {
    return mask && U3BitmapCopyInternal(destination, destinationRect, source, sourceRect, mask, maskRect);
}
