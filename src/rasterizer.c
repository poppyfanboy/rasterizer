#include <math.h>

#include "common.h"

void render(u32 *pixels, int width, int height, f64 time) {
    for (int y = 0; y < height; y += 1) {
        for (int x = 0; x < width; x += 1) {
            f32 uv[2] = {(f32)x / width, (f32)y / height};
            f32 color[3] = {
                0.5F + 0.5F * cosf(time + uv[0]),
                0.5F + 0.5F * cosf(time + uv[1] + 2),
                0.5F + 0.5F * cosf(time + uv[0] + 4),
            };

            pixels[y * width + x] =
                (u32)(color[0] * 255) << 16 |
                (u32)(color[1] * 255) << 8 |
                (u32)(color[2] * 255);
        }
    }
}
