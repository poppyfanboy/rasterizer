#include "rasterizer.c"

#include <stdlib.h>

void render_gradient(
    void *rasterizer,
    u32 *pixels, int width, int height,
    f64 time,
    f64 delta_time
) {
    if (width == 0 || height == 0) {
        return;
    }

    u32 *pixel_iter = pixels;

    for (int y = 0; y < height; y += 1) {
        for (int x = 0; x < width; x += 1) {
            f32 uv[2] = { (f32)x / width, (f32)y / height };
            f32 color[3] = {
                0.5F + 0.5F * cosf(time + uv[X]),
                0.5F + 0.5F * cosf(time + uv[Y] + 2),
                0.5F + 0.5F * cosf(time + uv[X] + 4),
            };

            *pixel_iter = rgb_to_u32(color);
            pixel_iter += 1;
        }
    }
}

void render_iterative_lines(
    void *rasterizer,
    u32 *pixels, int width, int height,
    f64 time,
    f64 delta_time
) {
    if (width == 0 || height == 0) {
        return;
    }

    u32 clear_color = rgb_to_u32((f32[]){ 0.1F, 0.1F, 0.1F });
    clear_screen(pixels, width, height, clear_color);

    {
        f32 center[2] = { 300, 300 };
        f32x2_add_assign(center, (f32[]){ 50 * cos(1.5 * time), 50 * sin(1.5 * time) });

        f32 offset[2] = { cos(2.5 * time), sin(2.5 * time) };
        f32x2_scale(offset, 100);

        f32 start[2], end[2];
        f32x2_subtract(center, offset, start);
        f32x2_add(center, offset, end);

        draw_line_iterative(
            pixels, width, height,
            fix8_from_float(start[0]), fix8_from_float(start[1]),
            fix8_from_float(center[0]), fix8_from_float(center[1]),
            0xffff00
        );
        draw_line_iterative(pixels, width, height,
            fix8_from_float(center[0]), fix8_from_float(center[1]),
            fix8_from_float(end[0]), fix8_from_float(end[1]),
            0x00ffff
        );

        // This line should more or less cover both of the lines above.
        draw_line_iterative(pixels, width, height,
            fix8_from_float(start[0]), fix8_from_float(start[1]),
            fix8_from_float(end[0]), fix8_from_float(end[1]),
            0x000000
        );
    }

    {
        f32 center[2] = { 400.5F, 400.5F };

        f32 offset[2] = { cos(-1.5 * time), sin(-1.5 * time) };
        f32x2_scale(offset, 100);

        f32 start[2], end[2];
        f32x2_subtract(center, offset, start);
        f32x2_add(center, offset, end);

        draw_line_iterative(pixels, width, height,
            fix8_from_float(start[0]), fix8_from_float(start[1]),
            fix8_from_float(center[0]), fix8_from_float(center[1]),
            0xffff00
        );
        draw_line_iterative(pixels, width, height,
            fix8_from_float(center[0]), fix8_from_float(center[1]),
            fix8_from_float(end[0]), fix8_from_float(end[1]),
            0x00ffff
        );

        // This line should more or less cover both of the lines above.
        draw_line_iterative(pixels, width, height,
            fix8_from_float(start[0]), fix8_from_float(start[1]),
            fix8_from_float(end[0]), fix8_from_float(end[1]),
            0x000000
        );
    }

    {
        f32 center[2] = { 300, 400 };

        f32 offset[2] = { cos(1.5 * time), sin(1.5 * time) };
        f32x2_scale(offset, 100);

        f32 start[2], end[2];
        f32x2_subtract(center, offset, start);
        f32x2_add(center, offset, end);

        draw_line_iterative(pixels, width, height,
            fix8_from_float(start[0]), fix8_from_float(start[1]),
            fix8_from_float(center[0]), fix8_from_float(center[1]),
            0xffff00
        );
        draw_line_iterative(pixels, width, height,
            fix8_from_float(center[0]), fix8_from_float(center[1]),
            fix8_from_float(end[0]), fix8_from_float(end[1]),
            0x00ffff
        );

        // This line should more or less cover both of the lines above.
        draw_line_iterative(pixels, width, height,
            fix8_from_float(start[0]), fix8_from_float(start[1]),
            fix8_from_float(end[0]), fix8_from_float(end[1]),
            0x000000
        );
    }
}

void render_non_iterative_lines(
    void *rasterizer,
    u32 *pixels, int width, int height,
    f64 time,
    f64 delta_time
) {
    if (width == 0 || height == 0) {
        return;
    }

    u32 clear_color = rgb_to_u32((f32[]){ 0.1F, 0.1F, 0.1F });
    clear_screen(pixels, width, height, clear_color);

    f32 top_left[2] = { 100, 100 };

    f32 box_min[2], box_max[2];
    f32x2_copy(top_left, box_min);
    f32x2_copy(top_left, box_max);
    f32x2_subtract(box_min, (f32[]){ 0.5F, 0.5F }, box_min);
    f32x2_add(box_max, (f32[]){ 17 + 0.5F, 17 + 0.5F }, box_max);

    // Important to go in either CCW or CW order for all the corners to get filled.
    draw_line_non_iterative(
        pixels, width, height,
        fix8_from_float(box_min[0]), fix8_from_float(box_min[1]),
        fix8_from_float(box_max[0]), fix8_from_float(box_min[1]),
        0x404040
    );
    draw_line_non_iterative(
        pixels, width, height,
        fix8_from_float(box_max[0]), fix8_from_float(box_min[1]),
        fix8_from_float(box_max[0]), fix8_from_float(box_max[1]),
        0x404040
    );
    draw_line_non_iterative(
        pixels, width, height,
        fix8_from_float(box_max[0]), fix8_from_float(box_max[1]),
        fix8_from_float(box_min[0]), fix8_from_float(box_max[1]),
        0x404040
    );
    draw_line_non_iterative(
        pixels, width, height,
        fix8_from_float(box_min[0]), fix8_from_float(box_max[1]),
        fix8_from_float(box_min[0]), fix8_from_float(box_min[1]),
        0x404040
    );

    // The lines are from this image:
    // https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-rasterizer-stage-rules#line-rasterization-rules-aliased-without-multisampling
    f32 lines[][2][2] = {
        { { 0.5F, 0.5F }, { 1.5F, 0.75F } },
        { { 0.5F, 6.0F }, { 2.5F, 4.0F } },
        { { 1.0F, 9.5F }, { 3.0F, 7.5F } },
        { { 1.0F, 11.5F }, { 3.0F, 9.5F } },
        { { 1.5F, 2.75F }, { 0.5F, 2.5F } },
        { { 2.5F, 4.0F }, { 4.5F, 6.0F } },
        { { 2.5F, 12.0F }, { 4.5F, 14.0F } },
        { { 2.5F, 14.0F }, { 0.5F, 16.0F } },
        { { 3.0F, 7.5F }, { 5.0F, 9.5F } },
        { { 3.5F, 15.0F }, { 4.0F, 14.5F } },
        { { 3.5F, 16.0F }, { 3.5F, 15.F } },
        { { 3.75F, -0.2F }, { 3.8F, 1.1F } },
        { { 4.0F, 2.5F }, { 8.5F, 4.0F } },
        { { 4.0F, 10.5F }, { 6.0F, 12.5F } },
        { { 6.0F, 0.5F }, { 6.1F, 0.8F } },
        { { 6.0F, 4.5F }, { 6.0F, 5.5F } },
        { { 6.0F, 5.5F }, { 6.0F, 6.5F } },
        { { 6.5F, 8.0F }, { 8.5F, 10.0F } },
        { { 7.0F, 13.5F }, { 5.0F, 15.5F } },
        { { 7.5F, 1.0F }, { 7.75F, 0.8F } },
        { { 8.0F, 12.5F }, { 9.0F, 12.5F } },
        { { 8.0F, 14.5F }, { 10.0F, 14.5F } },
        { { 8.5F, 10.0F }, { 6.5F, 12.0F } },
        { { 9.0F, 11.5F }, { 8.0F, 11.5F } },
        { { 9.25F, 0.75F }, { 9.25F, 1.25F } },
        { { 10.0F, 9.5F }, { 12.0F, 11.5F } },
        { { 10.0F, 14.5F }, { 11.0F, 15.5F } },
        { { 10.5F, 2.0F }, { 9.0F, 6.5F } },
        { { 10.5F, 8.0F }, { 9.5F, 8.0F } },
        { { 11.5F, 8.0F }, { 10.5F, 8.0F } },
        { { 11.75F, 0.75F }, { 11.75F, 1.25F } },
        { { 12.0F, 11.5F }, { 10.0F, 13.5F } },
        { { 12.25F, 1.4F }, { 12.25F, 1.4F } },
        { { 12.1F, 3.2F }, { 12.25F, 3.2F } },
        { { 12.8F, 3.2F }, { 12.95F, 3.2F } },
        { { 12.3F, 3.3F }, { 12.5F, 3.3F }, },
        { { 12.6F, 3.3F }, { 12.6F, 3.6F }, },
        { { 12.1F, 3.7F }, { 12.1F, 3.9F }, },
        { { 12.9F, 3.7F }, { 12.9F, 3.9F }, },
        { { 12.5F, 6.0F }, { 13.0F, 5.5F } },
        { { 12.5F, 15.0F }, { 12.0F, 15.0F } },
        { { 13.0F, 9.5F }, { 12.5F, 9.0F } },
        { { 13.125F, 0.375F }, { 13.375F, 0.125F } },
        { { 13.375F, 1.875F }, { 13.125F, 1.625F } },
        { { 13.5F, 7.0F }, { 13.5F, 6.0F } },
        { { 13.5F, 9.0F }, { 13.5F, 10.0F } },
        { { 13.5F, 16.0F }, { 12.5F, 15.0F } },
        { { 14.0F, 4.5F }, { 14.5F, 5.0F } },
        { { 14.1F, 10.8F }, { 12.3F, 13.9F } },
        { { 14.25F, 0.1F }, { 14.75F, 0.1F } },
        { { 14.25F, 3.25F }, { 13.8F, 2.8F } },
        { { 14.625F, 3.125F }, { 15.125F, 3.375F } },
        { { 15.0F, 8.5F }, { 15.5F, 8.0F } },
        { { 15.2F, 2.8F }, { 14.8F, 2.8F } },
        { { 15.5F, 6.0F }, { 15.0F, 6.5F } },
        { { 15.625F, 0.125F }, { 15.875F, 0.375F } },
        { { 15.875F, 1.625F }, { 15.625F, 1.875F } },
        { { 15.8F, 3.2F }, { 15.8F, 2.8F } },
        { { 15.8F, 14.2F }, { 15.1F, 15.5F } },
        { { 16.1F, 10.8F }, { 14.0F, 14.5F } },
    };

    for (int i = 0; i < ARRAY_COUNT(lines); i += 1) {
        f32 start[2];
        f32x2_copy(lines[i][0], start);
        f32x2_add(start, top_left, start);

        f32 end[2];
        f32x2_copy(lines[i][1], end);
        f32x2_add(end, top_left, end);

        draw_line_non_iterative(
            pixels, width, height,
            fix8_from_float(start[0]), fix8_from_float(start[1]),
            fix8_from_float(end[0]), fix8_from_float(end[1]),
            0xffffff
        );
    }
}

void render_line_frame(
    void *rasterizer,
    u32 *pixels, int width, int height,
    f64 time,
    f64 delta_time
) {
    if (width == 0 || height == 0) {
        return;
    }

    u32 clear_color = rgb_to_u32((f32[]){ 0.1F, 0.1F, 0.1F });
    clear_screen(pixels, width, height, clear_color);

    // The whole thing must be visible at all times.
    draw_line(pixels, width, height, (f32[]){ -1, -1 }, (f32[]){ 1, -1 }, 0xffff00);
    draw_line(pixels, width, height, (f32[]){ 1, -1 }, (f32[]){ 1, 1 }, 0xffff00);
    draw_line(pixels, width, height, (f32[]){ 1, 1 }, (f32[]){ -1, 1 }, 0xffff00);
    draw_line(pixels, width, height, (f32[]){ -1, 1 }, (f32[]){ -1, -1 }, 0xffff00);
}

void render_clipped_lines(
    void *rasterizer,
    u32 *pixels, int width, int height,
    f64 time,
    f64 delta_time
) {
    if (width == 0 || height == 0) {
        return;
    }

    u32 clear_color = rgb_to_u32((f32[]){ 0.1F, 0.1F, 0.1F });
    clear_screen(pixels, width, height, clear_color);

    int const line_count = 32;

    for (int i = 0; i < line_count; i += 1) {
        f32 color[3];
        hsv_to_rgb((f32[]){ (f32)i / line_count * 360, 0.35F, 0.8F }, color);

        f32 angle = PI * ((f32)i / line_count) + time * 0.25;

        draw_line(
            pixels, width, height,
            (f32[]){ -2 * cosf(angle), -2 * sinf(angle) },
            (f32[]){ 2 * cosf(angle), 2 * sinf(angle) },
            rgb_to_u32(color)
        );
    }
}

typedef struct {
    f32 (*vertices)[3];
    int vertex_count;

    Transform transform;
    Camera camera;
} Cube;

Cube *initialize_cube(void) {
    Cube *cube = malloc(sizeof(*cube));
    cube->vertices = malloc(36 * sizeof(*cube->vertices));
    cube->vertex_count = 36;

    transform_identity(&cube->transform);
    transform_translate(&cube->transform, (f32[]){ 0, 0, -1.5F });

    cube->camera.yaw = 0;
    cube->camera.pitch = 0;
    f32x3_copy((f32[]){ 0, 0, 0 }, cube->camera.position);

    f32 cube_width = 0.75F;
    f32 cube_face_vertices[][3] = {
        { -0.5F,  0.0F, -0.5F  }, { -0.5F,  0.0F,  0.5F  }, {  0.5F,  0.0F,  0.5F  },
        { -0.5F,  0.0F, -0.5F  }, {  0.5F,  0.0F,  0.5F  }, {  0.5F,  0.0F, -0.5F  },
    };
    f32 cube_face_transforms[][3][3] = {
        { {  1,  0,  0  }, {  0,  1,  0  }, {  0,  0,  1  } }, //   0° around X-axis
        { {  1,  0,  0  }, {  0, -1,  0  }, {  0,  0, -1  } }, // 180° around X-axis
        { {  1,  0,  0  }, {  0,  0, -1  }, {  0,  1,  0  } }, //  90° around X-axis
        { {  1,  0,  0  }, {  0,  0,  1  }, {  0, -1,  0  } }, // -90° around X-axis
        { {  0, -1,  0  }, {  1,  0,  0  }, {  0,  0,  1  } }, //  90° around Z-axis
        { {  0,  1,  0  }, { -1,  0,  0  }, {  0,  0,  1  } }, // -90° around Z-axis
    };

    f32 (*vertex_iter)[3] = cube->vertices;

    for (int face_index = 0; face_index < 6; face_index += 1) {
        f32 face_offset[3] = { 0.0F, 0.5F, 0.0F };
        f32x3x3_multiply_vector(cube_face_transforms[face_index], face_offset, face_offset);
        f32x3_scale(face_offset, cube_width);

        for (int vertex_index = 0; vertex_index < 6; vertex_index += 1) {
            f32x3_copy(cube_face_vertices[vertex_index], *vertex_iter);

            f32x3x3_multiply_vector(cube_face_transforms[face_index], *vertex_iter, *vertex_iter);
            f32x3_scale(*vertex_iter, cube_width);
            f32x3_add_assign(*vertex_iter, face_offset);

            vertex_iter += 1;
        }
    }

    return cube;
}

void render_cube(Cube *cube, u32 *pixels, int width, int height, f64 time, f64 delta_time) {
    f32 rotation_axis[3] = { 1, 2, 3 };
    f32x3_normalize(rotation_axis);
    transform_rotate_around(&cube->transform, rotation_axis, 1.25F * delta_time);

    if (width == 0 || height == 0) {
        return;
    }

    u32 clear_color = rgb_to_u32((f32[]){ 0.1F, 0.1F, 0.1F });
    clear_screen(pixels, width, height, clear_color);

    f32 projection[4][4];
    perspective_projection((f32)width / height, degrees_to_radians(75), 0.1, 100, projection);

    for (int i = 0; i < cube->vertex_count; i += 3) {
        f32 triangle[3][4];

        f32x3_copy(cube->vertices[i], triangle[0]);
        f32x3_copy(cube->vertices[i + 1], triangle[1]);
        f32x3_copy(cube->vertices[i + 2], triangle[2]);

        triangle[0][W] = 1;
        triangle[1][W] = 1;
        triangle[2][W] = 1;

        for (int j = 0; j < 3; j += 1) {
            transform_apply(&cube->transform, triangle[j]);
            camera_apply(&cube->camera, triangle[j]);
            f32x4x4_multiply_vector(projection, triangle[j], triangle[j]);

            triangle[j][X] /= triangle[j][W];
            triangle[j][Y] /= triangle[j][W];
            triangle[j][Z] /= triangle[j][W];
        }

        draw_triangle(pixels, width, height, triangle[0], triangle[1], triangle[2], 0xffffff);
    }
}

typedef Cube Rasterizer;

Rasterizer *initialize(void) {
    return initialize_cube();
}

void render(Rasterizer *rasterizer, u32 *pixels, int width, int height, f64 time, f64 delta_time) {
    render_cube(rasterizer, pixels, width, height, time, delta_time);
}

#undef X
#undef Y
#undef Z
#undef W

#undef R
#undef G
#undef B
#undef A
