#include "math.c"

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t u8;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;

typedef ptrdiff_t isize;

#define ARRAY_COUNT(array) ((isize)sizeof(array) / (isize)sizeof((array)[0]))

#define PI 3.14159265358979323846

static inline f64 degrees_to_radians(f64 degrees) {
    return degrees / 180 * PI;
}

static inline int int_max(int left, int right) {
    return left > right ? left : right;
}

// 32-bit 24.8 fixed point number.
//
// Let's say we want to support resolutions up to 2048x2048. This means the integer part precision
// has to be at least 11 bits.
//
// We need enough precision to store the result of (x0 * dy - y0 * dx), which might require
// 2p+1 = 2*11 + 1 = 23 bits in the worst case. What's left out of 32 bits is 8 bits for the
// fractional part and 1 bit for the sign.

typedef i32 fix8;

#define FIX8_PRECISION 8
#define FIX8_ONE (1 << FIX8_PRECISION)

#define FIX8_FRACTIONAL_BITS (FIX8_ONE - 1)
#define FIX8_INTEGER_BITS (~FIX8_FRACTIONAL_BITS)

static inline fix8 fix8_multiply(fix8 left, fix8 right) {
    i64 product = (i64)left * (i64)right;

    // Do the rounding using the most significant bit from the part which is getting shifted out.
    return
        (product >> FIX8_PRECISION) +
        ((product & FIX8_FRACTIONAL_BITS) >> (FIX8_PRECISION - 1));
}

static inline fix8 fix8_floor(fix8 value) {
    return value & FIX8_INTEGER_BITS;
}

static inline fix8 fix8_ceil(fix8 value) {
    return (value & FIX8_INTEGER_BITS) + ((value & FIX8_FRACTIONAL_BITS) != 0 ? FIX8_ONE : 0);
}

static inline fix8 fix8_abs(fix8 value) {
    return value >= 0 ? value : -value;
}

static inline fix8 fix8_min(fix8 left, fix8 right) {
    return left < right ? left : right;
}

static inline fix8 fix8_max(fix8 left, fix8 right) {
    return left > right ? left : right;
}

static inline fix8 fix8_from_float(f32 value) {
    // Add 0.5, so that after truncation the value gets effectively rounded.
    return (value * FIX8_ONE) + (value >= 0 ? 0.5F : -0.5F);
}

static inline u32 rgb_to_u32(f32 color[3]) {
    u32 result = 0;
    result |= (u32)(color[R] * 255) << 16;
    result |= (u32)(color[G] * 255) << 8;
    result |= (u32)(color[B] * 255);

    return result;
}

// https://en.wikipedia.org/wiki/HSL_and_HSV#HSV_to_RGB
// Domain: hue ∈ [0°..360°), saturation ∈ [0..1], value ∈ [0..1]
void hsv_to_rgb(f32 const hsv[3], f32 rgb[3]) {
    f32 h_chunk = hsv[0] / 60;
    f32 s = hsv[1];
    f32 v = hsv[2];

    f32 c = v * s;
    f32 x = c * (1 - fabsf(fmodf(h_chunk, 2) - 1));

    if (h_chunk < 1) {
        f32x3_copy((f32[]){ c, x, 0 }, rgb);
    } else if (h_chunk < 2) {
        f32x3_copy((f32[]){ x, c, 0 }, rgb);
    } else if (h_chunk < 3) {
        f32x3_copy((f32[]){ 0, c, x }, rgb);
    } else if (h_chunk < 4) {
        f32x3_copy((f32[]){ 0, x, c }, rgb);
    } else if (h_chunk < 5) {
        f32x3_copy((f32[]){ x, 0, c }, rgb);
    } else {
        f32x3_copy((f32[]){ c, 0, x }, rgb);
    }

    f32 m = v - c;
    f32x3_add_assign(rgb, (f32[]){ m, m, m });
}

typedef struct {
    int bitmap_x, bitmap_y;
    int bitmap_direction_x, bitmap_direction_y;

    fix8 x0, y0;
    fix8 x1, y1;

    fix8 x, y;
} LinePlotter;

void line_plotter_initialize(fix8 x0, fix8 y0, fix8 x1, fix8 y1, LinePlotter *plotter) {
    // All the math is done as if the gradient is positive.
    // Save original direction to calculate bitmap coordinates at each step.

    if (x0 <= x1) {
        plotter->bitmap_x = x0 / FIX8_ONE;
        plotter->bitmap_direction_x = 1;

        plotter->x0 = x0;
        plotter->x1 = x1;
    } else {
        plotter->bitmap_x = fix8_ceil(x0 - FIX8_ONE) / FIX8_ONE;
        plotter->bitmap_direction_x = -1;

        fix8 x0_fract = x0 - fix8_floor(x0);
        fix8 mirrored_x0 = fix8_floor(x0) + (FIX8_ONE - x0_fract);
        fix8 mirrored_x1 = mirrored_x0 + (x0 - x1);

        plotter->x0 = mirrored_x0;
        plotter->x1 = mirrored_x1;
    }

    if (y0 <= y1) {
        plotter->bitmap_y = y0 / FIX8_ONE;
        plotter->bitmap_direction_y = 1;

        plotter->y0 = y0;
        plotter->y1 = y1;
    } else {
        plotter->bitmap_y = fix8_ceil(y0 - FIX8_ONE) / FIX8_ONE;
        plotter->bitmap_direction_y = -1;

        fix8 y0_fract = y0 - fix8_floor(y0);
        fix8 mirrored_y0 = fix8_floor(y0) + (FIX8_ONE - y0_fract);
        fix8 mirrored_y1 = mirrored_y0 + (y0 - y1);

        plotter->y0 = mirrored_y0;
        plotter->y1 = mirrored_y1;
    }

    // Top-left corner coordinates of the current pixel.

    plotter->x = fix8_floor(plotter->x0);
    plotter->y = fix8_floor(plotter->y0);
}

static inline void line_plotter_step_x(LinePlotter *plotter) {
    plotter->x += FIX8_ONE;
    plotter->bitmap_x += plotter->bitmap_direction_x;
}

static inline void line_plotter_step_y(LinePlotter *plotter) {
    plotter->y += FIX8_ONE;
    plotter->bitmap_y += plotter->bitmap_direction_y;
}

static inline void line_plotter_put_pixel(
    LinePlotter *plotter,
    u32 *pixels, int width, int height,
    u32 color
) {
    pixels[plotter->bitmap_y * width + plotter->bitmap_x] = color;
}

void draw_line_horizontal(
    LinePlotter *plotter,
    u32 *pixels, int width, int height,
    u32 color
) {
    // Line equation: ax + by + c = 0

    fix8 dx = plotter->x1 - plotter->x0;
    fix8 dy = plotter->y1 - plotter->y0;

    fix8 a = -dy;
    fix8 b = dx;
    fix8 c = fix8_multiply(dy, plotter->x0) - fix8_multiply(dx, plotter->y0);

    if (plotter->x <= plotter->x0 + FIX8_ONE / 2) {
        line_plotter_put_pixel(plotter, pixels, width, height, color);
    } else {
        line_plotter_step_x(plotter);
    }

    fix8 error =
        fix8_multiply(a, (plotter->x + FIX8_ONE / 2)) + fix8_multiply(b, plotter->y + FIX8_ONE) + c;

    while (plotter->x + FIX8_ONE <= plotter->x1) {
        if (error < 0) {
            error += b;
            line_plotter_step_y(plotter);
        }

        line_plotter_put_pixel(plotter, pixels, width, height, color);

        error += a;
        line_plotter_step_x(plotter);
    }
}

void draw_line_vertical(
    LinePlotter *plotter,
    u32 *pixels, int width, int height,
    u32 color
) {
    // Line equation: ax + by + c = 0

    fix8 dx = plotter->x1 - plotter->x0;
    fix8 dy = plotter->y1 - plotter->y0;

    fix8 a = -dy;
    fix8 b = dx;
    fix8 c = fix8_multiply(dy, plotter->x0) - fix8_multiply(dx, plotter->y0);

    if (plotter->y <= plotter->y0 + FIX8_ONE / 2) {
        line_plotter_put_pixel(plotter, pixels, width, height, color);
    } else {
        line_plotter_step_y(plotter);
    }

    fix8 error =
        fix8_multiply(a, (plotter->x + FIX8_ONE)) + fix8_multiply(b, plotter->y + FIX8_ONE / 2) + c;

    while (plotter->y + FIX8_ONE <= plotter->y1) {
        if (error > 0) {
            error += a;
            line_plotter_step_x(plotter);
        }

        line_plotter_put_pixel(plotter, pixels, width, height, color);

        error += b;
        line_plotter_step_y(plotter);
    }
}

// Line endpoint coordinates are expected to be within the [0..width] x [0..height] area.
void draw_line_iterative(
    u32 *pixels, int width, int height,
    fix8 x0, fix8 y0, fix8 x1, fix8 y1,
    u32 color
) {
    LinePlotter plotter;
    line_plotter_initialize(x0, y0, x1, y1, &plotter);

    if (fix8_abs(x1 - x0) >= fix8_abs(y1 - y0)) {
        draw_line_horizontal(&plotter, pixels, width, height, color);
    } else {
        draw_line_vertical(&plotter, pixels, width, height, color);
    }
}

// Follows DirectX line rasterization rules.
// Line endpoint coordinates are expected to be within the [0..width] x [0..height] area.
void draw_line_non_iterative(
    u32 *pixels, int width, int height,
    fix8 x0, fix8 y0, fix8 x1, fix8 y1,
    u32 color
) {
    // Bias x to the left: a point right between two pixels can only belong to the left one.

    int min_x = fix8_ceil(fix8_min(x0, x1) - FIX8_ONE) / FIX8_ONE;
    min_x = int_max(min_x, 0);

    int max_x = fix8_ceil(fix8_max(x0, x1) - FIX8_ONE) / FIX8_ONE;
    max_x = int_max(max_x, 0);

    // Bias y to the top: a point right between two pixels can only belong to the top one.

    int min_y = fix8_ceil(fix8_min(y0, y1) - FIX8_ONE) / FIX8_ONE;
    min_y = int_max(min_y, 0);

    int max_y = fix8_ceil(fix8_max(y0, y1) - FIX8_ONE) / FIX8_ONE;
    max_y = int_max(max_y, 0);

    // Line equation: ax + by + c = 0

    fix8 dx = x1 - x0;
    fix8 dy = y1 - y0;

    fix8 a = -dy;
    fix8 b = dx;
    fix8 c = -(fix8_multiply(a, x0) + fix8_multiply(b, y0));

    bool line_is_y_major = fix8_abs(dx) < fix8_abs(dy);

    for (int bitmap_y = min_y; bitmap_y <= max_y; bitmap_y += 1) {
        for (int bitmap_x = min_x; bitmap_x <= max_x; bitmap_x += 1) {
            fix8 x = bitmap_x * FIX8_ONE;
            fix8 y = bitmap_y * FIX8_ONE;

            // Check where the top/right/bottom/left corners of the "diamond" test area are located
            // relative to the line.

            fix8 sign_top =
                fix8_multiply(a, x + FIX8_ONE / 2) + fix8_multiply(b, y) + c;
            fix8 sign_right =
                fix8_multiply(a, x + FIX8_ONE) + fix8_multiply(b, y + FIX8_ONE / 2) + c;
            fix8 sign_bottom =
                fix8_multiply(a, x + FIX8_ONE / 2) + fix8_multiply(b, y + FIX8_ONE) + c;
            fix8 sign_left =
                fix8_multiply(a, x) + fix8_multiply(b, y + FIX8_ONE / 2) + c;

            // Check if the line goes through the diamond when extended to infinity.

            bool extended_line_intersects_with_diamond = false;
            if (
                (sign_top > 0 || sign_right > 0 || sign_bottom > 0 || sign_left > 0) &&
                (sign_top < 0 || sign_right < 0 || sign_bottom < 0 || sign_left < 0)
            ) {
                extended_line_intersects_with_diamond = true;
            }
            if (sign_bottom == 0 || (line_is_y_major && sign_right == 0)) {
                extended_line_intersects_with_diamond = true;
            }
            if (!extended_line_intersects_with_diamond) {
                continue;
            }

            // Then act depending on where we are on the line relative to its endpoints.

            bool pixel_contains_start_point =
                (x < x0 && x0 <= x + FIX8_ONE) &&
                (y < y0 && y0 <= y + FIX8_ONE);

            bool pixel_contains_end_point =
                (x < x1 && x1 <= x + FIX8_ONE) &&
                (y < y1 && y1 <= y + FIX8_ONE);

            // Most common case: we are somewhere in the middle of the line (and we already know
            // that the line intersects the diamond area of the pixel we are deciding to fill).

            if (!pixel_contains_start_point && !pixel_contains_end_point) {
                pixels[bitmap_y * width + bitmap_x] = color;
                continue;
            }

            // Rare case: the current pixel contains one of the line endpoints.

            bool diamond_contains_start_point =
                fix8_abs(x0 - x - FIX8_ONE / 2) + (y0 - y) - FIX8_ONE <= 0 &&
                fix8_abs(x0 - x - FIX8_ONE / 2) - (y0 - y) < 0;

            bool diamond_contains_end_point =
                fix8_abs(x1 - x - FIX8_ONE / 2) + (y1 - y) - FIX8_ONE <= 0 &&
                fix8_abs(x1 - x - FIX8_ONE / 2) - (y1 - y) < 0;

            if (line_is_y_major) {
                diamond_contains_start_point =
                    diamond_contains_start_point || (x0 == x + FIX8_ONE && y0 == y + FIX8_ONE / 2);

                diamond_contains_end_point =
                    diamond_contains_end_point || (x1 == x + FIX8_ONE && y1 == y + FIX8_ONE / 2);
            }

            // Never fill the ending pixel if the line reached the ending point before it managed to
            // leave the diamond area.
            if (diamond_contains_end_point) {
                continue;
            }

            // Fill the starting pixel when the starting point is inside of the diamond and the line
            // managed to leave the diamond area.
            if (diamond_contains_start_point) {
                pixels[bitmap_y * width + bitmap_x] = color;
                continue;
            }

            // Otherwise the line either went through the diamond or it stopped at the ending point
            // and didn't reach the diamond.
            //
            // To decide what happened exactly we just need to check if the starting and the ending
            // point are positioned on the opposite sides relative to the current pixel center.
            if (line_is_y_major) {
                if (
                    (y0 < y + FIX8_ONE / 2 && y1 > y + FIX8_ONE / 2) ||
                    (y1 < y + FIX8_ONE / 2 && y0 > y + FIX8_ONE / 2)
                ) {
                    pixels[bitmap_y * width + bitmap_x] = color;
                }
            } else {
                if (
                    (x0 < x + FIX8_ONE / 2 && x1 > x + FIX8_ONE / 2) ||
                    (x1 < x + FIX8_ONE / 2 && x0 > x + FIX8_ONE / 2)
                ) {
                    pixels[bitmap_y * width + bitmap_x] = color;
                }
            }
        }
    }
}

typedef u8 ClipAreaPosition;

#define CLIP_AREA_POSITION_TOP_BIT ((ClipAreaPosition)1 << 0)
#define CLIP_AREA_POSITION_RIGHT_BIT ((ClipAreaPosition)1 << 1)
#define CLIP_AREA_POSITION_BOTTOM_BIT ((ClipAreaPosition)1 << 2)
#define CLIP_AREA_POSITION_LEFT_BIT ((ClipAreaPosition)1 << 3)

ClipAreaPosition clip_area_position(
    f32 const point[2],
    f32 const clip_min[2],
    f32 const clip_max[2]
) {
    ClipAreaPosition position = 0;

    if (point[X] < clip_min[X]) {
        position |= CLIP_AREA_POSITION_LEFT_BIT;
    } else if (point[X] > clip_max[X]) {
        position |= CLIP_AREA_POSITION_RIGHT_BIT;
    }

    if (point[Y] < clip_min[Y]) {
        position |= CLIP_AREA_POSITION_TOP_BIT;
    } else if (point[Y] > clip_max[Y]) {
        position |= CLIP_AREA_POSITION_BOTTOM_BIT;
    }

    return position;
}

// https://en.wikipedia.org/wiki/Cohen–Sutherland_algorithm
bool clip_line(f32 start[2], f32 end[2], f32 const clip_min[2], f32 const clip_max[2]) {
    ClipAreaPosition start_clip_position = clip_area_position(start, clip_min, clip_max);
    ClipAreaPosition end_clip_position = clip_area_position(end, clip_min, clip_max);

    // Keep trimming the line down until its obvious whether the line is completely inside of the
    // clipping area or completely outside of it.
    while (true) {
        // Both endpoints are inside of the clipping area.
        if (start_clip_position == 0 && end_clip_position == 0) {
            return true;
        }

        // Both endpoints are completely to the top / right / bottom / left from the clipping area.
        if ((start_clip_position & end_clip_position) != 0) {
            return false;
        }

        ClipAreaPosition outside_point_clip_position;
        if (start_clip_position != 0) {
            outside_point_clip_position = start_clip_position;
        } else {
            outside_point_clip_position = end_clip_position;
        }

        f32 outside_point_clipped[2];

        if ((outside_point_clip_position & CLIP_AREA_POSITION_TOP_BIT) != 0) {

            // The "outside" endpoint is above the top "y = max.y" boundary line, meanwhile the
            // other endpoint is somewhere below the boundary line.
            //
            // Calculate the x coordinate of the intersection with the boundary line:
            // intersection.x = start.x + (intersection.y - start.y) / slope

            outside_point_clipped[X] =
                start[X] + (end[X] - start[X]) * (clip_min[Y] - start[Y]) / (end[Y] - start[Y]);

            outside_point_clipped[Y] = clip_min[Y];

        } else if ((outside_point_clip_position & CLIP_AREA_POSITION_RIGHT_BIT) != 0) {

            // The "outside" endpoint is above the right "x = max.x" boundary line, meanwhile the
            // other endpoint is somewhere to the left from the boundary line.
            //
            // Calculate the y coordinate of the intersection with the boundary line:
            // intersection.y = start.y + (intersection.x - start.x) * slope

            outside_point_clipped[X] = clip_max[X];

            outside_point_clipped[Y] =
                start[Y] + (end[Y] - start[Y]) * (clip_max[X] - start[X]) / (end[X] - start[X]);

        } else if ((outside_point_clip_position & CLIP_AREA_POSITION_BOTTOM_BIT) != 0) {

            outside_point_clipped[X] =
                start[X] + (end[X] - start[X]) * (clip_max[Y] - start[Y]) / (end[Y] - start[Y]);

            outside_point_clipped[Y] = clip_max[Y];

        } else /* if ((outside_point_clip_position & CLIP_AREA_POSITION_LEFT_BIT) != 0) */ {

            outside_point_clipped[X] = clip_min[X];

            outside_point_clipped[Y] =
                start[Y] + (end[Y] - start[Y]) * (clip_min[X] - start[X]) / (end[X] - start[X]);

        }

        if (outside_point_clip_position == start_clip_position) {
            f32x2_copy(outside_point_clipped, start);
            start_clip_position = clip_area_position(start, clip_min, clip_max);
        } else {
            f32x2_copy(outside_point_clipped, end);
            end_clip_position = clip_area_position(end, clip_min, clip_max);
        }
    }
}

void draw_line(
    u32 *pixels, int width, int height,
    f32 const start[2], f32 const end[2],
    u32 color
) {
    // (-1, -1) -> (0.5, 0.5) center of the bottom-left pixel on the screen.
    // (1, 1) -> (width - 0.5, height - 0.5) center of the top-right pixel on the screen.
    f32 start_clipped[2] = {
        (start[0] + 1) / 2 * (width - 1) + 0.5F,
        (-start[1] + 1) / 2 * (height - 1) + 0.5F,
    };
    f32 end_clipped[2] = {
        (end[0] + 1) / 2 * (width - 1) + 0.5F,
        (-end[1] + 1) / 2 * (height - 1) + 0.5F,
    };

    f32 clip_min[2] = { 0, 0 };
    f32 clip_max[2] = { width, height };

    if (clip_line(start_clipped, end_clipped, clip_min, clip_max)) {
        fix8 x0 = fix8_from_float(start_clipped[0]);
        fix8 y0 = fix8_from_float(start_clipped[1]);

        fix8 x1 = fix8_from_float(end_clipped[0]);
        fix8 y1 = fix8_from_float(end_clipped[1]);

        draw_line_iterative(pixels, width, height, x0, y0, x1, y1, color);
    }
}

void draw_triangle(
    u32 *pixels, int width, int height,
    f32 vertex1[3], f32 vertex2[3], f32 vertex3[3],
    u32 color
) {
    f32 direction_1_to_2[3], direction_1_to_3[3];
    f32x3_subtract(vertex2, vertex1, direction_1_to_2);
    f32x3_subtract(vertex3, vertex1, direction_1_to_3);

    // Triangles with the CCW order are treated as front-facing.
    f32 normal_vector[3];
    f32x3_cross(direction_1_to_2, direction_1_to_3, normal_vector);

    // Render only the front-facing triangles.
    if (normal_vector[Z] >= 0) {
        draw_line(pixels, width, height, vertex1, vertex2, color);
        draw_line(pixels, width, height, vertex2, vertex3, color);
        draw_line(pixels, width, height, vertex3, vertex1, color);
    }
}

void clear_screen(u32 *pixels, int width, int height, u32 color) {
    for (int i = 0; i < width * height; i += 1) {
        pixels[i] = color;
    }
}
