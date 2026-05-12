#include <math.h>

#include "common.h"

static inline int int_clamp(int value, int min, int max) {
    if (value <= min) {
        return min;
    }
    if (value >= max) {
        return max;
    }
    return value;
}

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

static inline fix16 fix16_ceil(fix16 value) {
    return (value & FIX16_INTEGER_BITS) + ((value & FIX16_FRACTIONAL_BITS) != 0 ? FIX16_ONE : 0);
}

static inline fix16 fix16_abs(fix16 value) {
    return value >= 0 ? value : -value;
}

static inline fix16 fix16_min(fix16 left, fix16 right) {
    return left < right ? left : right;
}

static inline fix16 fix16_max(fix16 left, fix16 right) {
    return left > right ? left : right;
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

void draw_line_iterative(u32 *pixels, int width, int height, f32 start[2], f32 end[2], u32 color) {
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

// Follows line rasterization rules described here:
// https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-rasterizer-stage-rules
void draw_line_non_iterative(
    u32 *pixels, int width, int height,
    f32 start[2], f32 end[2],
    u32 color
) {
    fix16 x0 = fix16_from_float(start[0]);
    fix16 x1 = fix16_from_float(end[0]);

    fix16 y0 = fix16_from_float(start[1]);
    fix16 y1 = fix16_from_float(end[1]);

    // Bias x to the left: a point right between two pixels can only belong to the left one.

    int min_x = fix16_ceil(fix16_min(x0, x1) - FIX16_ONE) / FIX16_ONE;
    int max_x = fix16_ceil(fix16_max(x0, x1) - FIX16_ONE) / FIX16_ONE;

    if (max_x < 0 || min_x >= width) {
        return;
    }
    min_x = int_clamp(min_x, 0, width - 1);
    max_x = int_clamp(max_x, 0, width - 1);

    // Bias y to the top: a point right between two pixels can only belong to the top one.

    int min_y = fix16_ceil(fix16_min(y0, y1) - FIX16_ONE) / FIX16_ONE;
    int max_y = fix16_ceil(fix16_max(y0, y1) - FIX16_ONE) / FIX16_ONE;

    if (max_y < 0 || min_y >= height) {
        return;
    }
    min_y = int_clamp(min_y, 0, height - 1);
    max_y = int_clamp(max_y, 0, height - 1);

    // Line equation: Ax + By + C = 0

    fix16 dx = x1 - x0;
    fix16 dy = y1 - y0;

    fix16 A = -dy;
    fix16 B = dx;
    fix16 C = fix16_mul(dy, x0) - fix16_mul(dx, y0);

    bool line_is_y_major = fix16_abs(dx) < fix16_abs(dy);

    fix16 x_half_step = fix16_mul(A, FIX16_ONE / 2);
    fix16 y_half_step = fix16_mul(B, FIX16_ONE / 2);

    for (int bitmap_y = min_y; bitmap_y <= max_y; bitmap_y += 1) {
        for (int bitmap_x = min_x; bitmap_x <= max_x; bitmap_x += 1) {
            fix16 x = bitmap_x * FIX16_ONE;
            fix16 y = bitmap_y * FIX16_ONE;

            // Check where the top/right/bottom/left corners of the "diamond" test area are located
            // relative to the line.

            fix16 sign_pixel_corner = fix16_mul(A, x) + fix16_mul(B, y) + C;

            fix16 sign_top = sign_pixel_corner + x_half_step;
            fix16 sign_right = sign_pixel_corner + 2 * x_half_step + y_half_step;
            fix16 sign_bottom = sign_pixel_corner + x_half_step + 2 * y_half_step;
            fix16 sign_left = sign_pixel_corner + y_half_step;

            // The diamond area is fully contained within one of the half-planes.

            if (sign_top > 0 && sign_right > 0 && sign_bottom > 0 && sign_left > 0) {
                continue;
            }
            if (sign_top < 0 && sign_right < 0 && sign_bottom < 0 && sign_left < 0) {
                continue;
            }

            bool pixel_contains_start_point =
                (x < x0 && x0 <= x + FIX16_ONE) &&
                (y < y0 && y0 <= y + FIX16_ONE);

            bool pixel_contains_end_point =
                (x < x1 && x1 <= x + FIX16_ONE) &&
                (y < y1 && y1 <= y + FIX16_ONE);

            // Most common case: line endpoints are somewhere far away and all we need to check is
            // whether a line goes through the diamond area.

            if (!pixel_contains_start_point && !pixel_contains_end_point) {
                if (sign_top != 0 || sign_right != 0 || sign_bottom != 0 || sign_left != 0) {
                    pixels[bitmap_y * width + bitmap_x] = color;
                }

                if (sign_bottom == 0 || (line_is_y_major && sign_right == 0)) {
                    pixels[bitmap_y * width + bitmap_x] = color;
                }

                continue;
            }

            // More precise checks for when the current pixel contains one of the line endpoints.

            bool diamond_contains_start_point =
                fix16_abs(x0 - x - FIX16_ONE / 2) + (y0 - y) - FIX16_ONE <= 0 &&
                fix16_abs(x0 - x - FIX16_ONE / 2) - (y0 - y) < 0;

            bool diamond_contains_end_point = 
                fix16_abs(x1 - x - FIX16_ONE / 2) + (y1 - y) - FIX16_ONE <= 0 &&
                fix16_abs(x1 - x - FIX16_ONE / 2) - (y1 - y) < 0;

            if (line_is_y_major) {
                diamond_contains_start_point =
                    diamond_contains_start_point ||
                    x0 == x + FIX16_ONE && y0 == y + FIX16_ONE / 2;

                diamond_contains_end_point =
                    diamond_contains_end_point ||
                    x1 == x + FIX16_ONE && y1 == y + FIX16_ONE / 2;
            }

            // The line needs to exit the diamond area for a pixel to get filled.
            if (diamond_contains_end_point) {
                continue;
            }

            // Given that the line ending point is not inside of the diamond area, this means that
            // if the starting point is contained inside of a diamond area, the line definitely went
            // through it and left it.
            if (diamond_contains_start_point) {
                pixels[bitmap_y * width + bitmap_x] = color;
                continue;
            }

            // Both endpoints of the line are contained within a single pixel and neither of them
            // are inside the diamond area. Check if the endpoints are contained within different
            // "corners" of the pixel to decide whether the line crosses the diamond area.
            if (line_is_y_major) {
                if (
                    y0 < y + FIX16_ONE / 2 && y1 > y + FIX16_ONE / 2 ||
                    y1 < y + FIX16_ONE / 2 && y0 > y + FIX16_ONE / 2
                ) {
                    pixels[bitmap_y * width + bitmap_x] = color;
                }
            } else {
                if (
                    x0 < x + FIX16_ONE / 2 && x1 > x + FIX16_ONE / 2 ||
                    x1 < x + FIX16_ONE / 2 && x0 > x + FIX16_ONE / 2
                ) {
                    pixels[bitmap_y * width + bitmap_x] = color;
                }
            }
        }
    }
}

void draw_line(u32 *pixels, int width, int height, f32 start[2], f32 end[2], u32 color) {
    draw_line_non_iterative(pixels, width, height, start, end, color);
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
