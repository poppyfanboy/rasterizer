#include <math.h>

#include "common.h"

// 32-bit 16.16 fixed point number
typedef i32 fix16;

#define FIX16_ONE 0x00010000
#define FIX16_FRACTIONAL_BITS 0x0000ffff
#define FIX16_INTEGER_BITS (~FIX16_FRACTIONAL_BITS)

static inline fix16 fix16_mul(fix16 left, fix16 right) {
    i64 product = (i64)left * right;
    // Do the rounding using the most significant bit from the part which is getting shifted out.
    return (product >> 16) + ((product & FIX16_FRACTIONAL_BITS) >> 15);
}

static inline fix16 fix16_floor(fix16 value) {
    return value & FIX16_INTEGER_BITS;
}

static inline fix16 fix16_abs(fix16 value) {
    return value >= 0 ? value : -value;
}

static inline fix16 fix16_from_float(f32 value) {
    // Add 0.5, so that after truncation the value gets effectively rounded.
    return (value * FIX16_ONE) + (value >= 0 ? 0.5F : -0.5F);
}

void clear_screen(u32 *pixels, int width, int height, f32 color[3]) {
    u32 color_packed =
        (u32)(color[0] * 255) << 16 |
        (u32)(color[1] * 255) << 8 |
        (u32)(color[2] * 255);

    for (int i = 0; i < width * height; i += 1) {
        pixels[i] = color_packed;
    }
}

void draw_line_horizontal(
    u32 *pixels, int width, int height,
    fix16 x0, fix16 y0,
    fix16 x1, fix16 y1,
    u32 color
) {
    // All the math is done as if the gradient is positive.
    // Save original direction to calculate bitmap coordinates at each step.

    int direction_x = x0 < x1 ? 1 : -1;
    int direction_y = y0 < y1 ? 1 : -1;

    if (x0 > x1) {
        fix16 x0_fract = x0 - fix16_floor(x0);
        fix16 mirrored_x0 = fix16_floor(x0) + (FIX16_ONE - x0_fract - 1);
        fix16 mirrored_x1 = mirrored_x0 + (x0 - x1);

        x0 = mirrored_x0;
        x1 = mirrored_x1;
    }

    if (y0 > y1) {
        fix16 y0_fract = y0 - fix16_floor(y0);
        fix16 mirrored_y0 = fix16_floor(y0) + (FIX16_ONE - y0_fract - 1);
        fix16 mirrored_y1 = mirrored_y0 + (y0 - y1);

        y0 = mirrored_y0;
        y1 = mirrored_y1;
    }

    // Top-left corner coordinates of the current pixel.

    fix16 x = fix16_floor(x0);
    fix16 y = fix16_floor(y0);
    int bitmap_x = x / FIX16_ONE;
    int bitmap_y = y / FIX16_ONE;

    // If the mid-point is to the left from the start point, do one step manually.

    fix16 x0_fract = x0 - x;
    fix16 y0_fract = y0 - y;

    if (x0 >= x + FIX16_ONE / 2 && x1 > x + FIX16_ONE) {
        bool line_covers_start_pixel =
            x0_fract + y0_fract - (FIX16_ONE + FIX16_ONE / 2) <= 0 &&
            x0_fract - y0_fract - FIX16_ONE / 2 < 0;

        if (line_covers_start_pixel) {
            pixels[bitmap_y * width + bitmap_x] = color;
        }

        x += FIX16_ONE;
        bitmap_x += direction_x;
    }

    // Line equation: Ax + By + C = 0

    fix16 dx = x1 - x0;
    fix16 dy = y1 - y0;

    fix16 A = -dy;
    fix16 B = dx;
    fix16 C = fix16_mul(dy, x0) - fix16_mul(dx, y0);

    // Check if the end point is (at the very least) on the other side of the pixel.
    // This is needed for the mid-point calculation to make sense.

    if (x1 >= x + FIX16_ONE / 2) {
        fix16 error = fix16_mul(A, (x + FIX16_ONE / 2)) + fix16_mul(B, y + FIX16_ONE) + C;

        while (true) {
            if (error < 0) {
                error += B;
                y += FIX16_ONE;
                bitmap_y += direction_y;
            }
            if (x1 <= x + FIX16_ONE) {
                break;
            }

            pixels[bitmap_y * width + bitmap_x] = color;

            error += A;
            x += FIX16_ONE;
            bitmap_x += direction_x;
        }
    }

    fix16 x1_fract = x1 - x;
    fix16 y1_fract = y1 - y;

    bool line_covers_end_pixel =
        x1_fract + y1_fract - (FIX16_ONE + FIX16_ONE / 2) > 0 ||
        x1_fract - y1_fract - FIX16_ONE / 2 >= 0;

    if (line_covers_end_pixel) {
        pixels[bitmap_y * width + bitmap_x] = color;
    }
}

void draw_line_vertical(
    u32 *pixels, int width, int height,
    fix16 x0, fix16 y0,
    fix16 x1, fix16 y1,
    u32 color
) {
    // All the math is done as if the gradient is positive.
    // Save original direction to calculate bitmap coordinates at each step.

    int direction_x = x0 < x1 ? 1 : -1;
    int direction_y = y0 < y1 ? 1 : -1;

    if (x0 > x1) {
        fix16 x0_fract = x0 - fix16_floor(x0);
        fix16 mirrored_x0 = fix16_floor(x0) + (FIX16_ONE - x0_fract - 1);
        fix16 mirrored_x1 = mirrored_x0 + (x0 - x1);

        x0 = mirrored_x0;
        x1 = mirrored_x1;
    }

    if (y0 > y1) {
        fix16 y0_fract = y0 - fix16_floor(y0);
        fix16 mirrored_y0 = fix16_floor(y0) + (FIX16_ONE - y0_fract - 1);
        fix16 mirrored_y1 = mirrored_y0 + (y0 - y1);

        y0 = mirrored_y0;
        y1 = mirrored_y1;
    }

    // Top-left corner coordinates of the current pixel.

    fix16 x = fix16_floor(x0);
    fix16 y = fix16_floor(y0);
    int bitmap_x = x / FIX16_ONE;
    int bitmap_y = y / FIX16_ONE;

    // If the mid-point is to the top from the start point, do one step manually.

    fix16 x0_fract = x0 - x;
    fix16 y0_fract = y0 - y;

    if (y0 >= y + FIX16_ONE / 2 && y1 > y + FIX16_ONE) {
        bool line_covers_start_pixel =
            x0_fract + y0_fract - (FIX16_ONE + FIX16_ONE / 2) < 0 &&
            x0_fract - y0_fract + FIX16_ONE / 2 > 0;

        if (line_covers_start_pixel) {
            pixels[bitmap_y * width + bitmap_x] = color;
        }

        y += FIX16_ONE;
        bitmap_y += direction_y;
    }

    // Line equation: Ax + By + C = 0

    fix16 dx = x1 - x0;
    fix16 dy = y1 - y0;

    fix16 A = -dy;
    fix16 B = dx;
    fix16 C = fix16_mul(dy, x0) - fix16_mul(dx, y0);

    // Check if the end point is at the very least on the other side of the pixel.
    // This is needed for the mid-point calculation to make sense.

    if (y1 >= y + FIX16_ONE / 2) {
        fix16 error = fix16_mul(A, (x + FIX16_ONE)) + fix16_mul(B, y + FIX16_ONE / 2) + C;

        while (true) {
            if (error > 0) {
                error += A;
                x += FIX16_ONE;
                bitmap_x += direction_x;
            }
            if (y1 <= y + FIX16_ONE) {
                break;
            }

            pixels[bitmap_y * width + bitmap_x] = color;

            error += B;
            y += FIX16_ONE;
            bitmap_y += direction_y;
        }
    }

    fix16 x1_fract = x1 - x;
    fix16 y1_fract = y1 - y;

    bool line_covers_end_pixel =
        x1_fract + y1_fract - (FIX16_ONE + FIX16_ONE / 2) >= 0 ||
        x1_fract - y1_fract + FIX16_ONE / 2 <= 0;

    if (line_covers_end_pixel) {
        pixels[bitmap_y * width + bitmap_x] = color;
    }
}

// Loosely based on line rasterization rules from here:
// https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-rasterizer-stage-rules
void draw_line(u32 *pixels, int width, int height, f32 start[2], f32 end[2], u32 color) {
    fix16 x0 = fix16_from_float(start[0]);
    fix16 x1 = fix16_from_float(end[0]);

    fix16 y0 = fix16_from_float(start[1]);
    fix16 y1 = fix16_from_float(end[1]);

    if (fix16_abs(x1 - x0) >= fix16_abs(y1 - y0)) {
        draw_line_horizontal(pixels, width, height, x0, y0, x1, y1, color);
    } else {
        draw_line_vertical(pixels, width, height, x0, y0, x1, y1, color);
    }
}

void render_lines(u32 *pixels, int width, int height, f64 time) {
    clear_screen(pixels, width, height, (f32[]){ 0.1F, 0.1F, 0.1F });

    {
        f32 top_left[2] = { 200 + 50 * cosf(2.0 * time), 200 + 50 * sinf(2.0 * time) };
        f32 rectangle_width = 100;
        f32 rectangle_height = 50;

        draw_line(
            pixels, width, height,
            (f32[]){ top_left[0], top_left[1] },
            (f32[]){ top_left[0] + rectangle_width, top_left[1] },
            0xffff00
        );
        draw_line(
            pixels, width, height,
            (f32[]){ top_left[0], top_left[1] + rectangle_height },
            (f32[]){ top_left[0] + rectangle_width, top_left[1] + rectangle_height },
            0xffff00
        );
        draw_line(
            pixels, width, height,
            (f32[]){ top_left[0], top_left[1] },
            (f32[]){ top_left[0], top_left[1] + rectangle_height },
            0xffff00
        );
        draw_line(
            pixels, width, height,
            (f32[]){ top_left[0] + rectangle_width, top_left[1] },
            (f32[]){ top_left[0] + rectangle_width, top_left[1] + rectangle_height },
            0xffff00
        );
    }

    {
        f32 radius = 100;
        f32 center[2] = { 300 + 50 * cosf(1.5 * time), 300 + 50 * sinf(1.5 * time) };

        f32 offset[2] = { radius * cosf(2.5 * time), radius * sinf(2.5 * time) };
        f32 start[2] = { center[0] - offset[0], center[1] - offset[1] };
        f32 end[2] = { center[0] + offset[0], center[1] + offset[1] };

        draw_line(pixels, width, height, start, center, 0x29adff);
        draw_line(pixels, width, height, center, end, 0xffec27);
        draw_line(pixels, width, height, start, end, 0xff77a8);
    }

    {
        f32 radius = 100;
        f32 center[2] = { 400.5F, 400.5F };

        f32 offset[2] = { radius * cosf(-1.5 * time), radius * sinf(-1.5 * time) };
        f32 start[2] = { center[0] - offset[0], center[1] - offset[1] };
        f32 end[2] = { center[0] + offset[0], center[1] + offset[1] };

        draw_line(pixels, width, height, start, center, 0x29adff);
        draw_line(pixels, width, height, center, end, 0xffec27);
        draw_line(pixels, width, height, start, end, 0xff77a8);
    }

    {
        f32 radius = 100;
        f32 center[2] = { 300.25F, 400.25F };

        f32 offset[2] = { radius * cosf(1.5 * time), radius * sinf(1.5 * time) };
        f32 start[2] = { center[0] - offset[0], center[1] - offset[1] };
        f32 end[2] = { center[0] + offset[0], center[1] + offset[1] };

        draw_line(pixels, width, height, start, center, 0x29adff);
        draw_line(pixels, width, height, center, end, 0xffec27);
        draw_line(pixels, width, height, start, end, 0xff77a8);
    }
}

void render_gradient(u32 *pixels, int width, int height, f64 time) {
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

void render(u32 *pixels, int width, int height, f64 time) {
    render_lines(pixels, width, height, time);
}
