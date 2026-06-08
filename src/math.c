#include <string.h>
#include <math.h>

typedef float f32;
typedef double f64;

#define X 0
#define Y 1
#define Z 2
#define W 3

#define R 0
#define G 1
#define B 2
#define A 3

static inline void f32x2_copy(f32 const source[2], f32 dest[2]) {
    dest[0] = source[0];
    dest[1] = source[1];
}

static inline void f32x2_add(f32 const left[2], f32 const right[2], f32 result[2]) {
    result[0] = left[0] + right[0];
    result[1] = left[1] + right[1];
}

static inline f32 *f32x2_add_assign(f32 left[2], f32 const right[2]) {
    f32x2_add(left, right, left);
    return left;
}

static inline void f32x2_subtract(f32 const left[2], f32 const right[2], f32 result[2]) {
    result[0] = left[0] - right[0];
    result[1] = left[1] - right[1];
}

static inline f32 *f32x2_subtract_assign(f32 left[2], f32 const right[2]) {
    f32x2_subtract(left, right, left);
    return left;
}

static inline f32 *f32x2_scale(f32 vector[2], f32 scalar) {
    vector[0] *= scalar;
    vector[1] *= scalar;
    return vector;
}

static inline void f32x3_copy(f32 const source[3], f32 dest[3]) {
    dest[0] = source[0];
    dest[1] = source[1];
    dest[2] = source[2];
}

static inline f32 f32x3_dot(f32 const left[3], f32 const right[3]) {
    return left[0] * right[0] + left[1] * right[1] + left[2] * right[2];
}

static inline void f32x3_cross(f32 const left[3], f32 const right[3], f32 result[3]) {
    result[0] = left[1] * right[2] - left[2] * right[1];
    result[1] = left[2] * right[0] - left[0] * right[2];
    result[2] = left[0] * right[1] - left[1] * right[0];
}

static inline void f32x3_add(f32 const left[3], f32 const right[3], f32 result[3]) {
    result[0] = left[0] + right[0];
    result[1] = left[1] + right[1];
    result[2] = left[2] + right[2];
}

static inline f32 *f32x3_add_assign(f32 left[3], f32 const right[3]) {
    f32x3_add(left, right, left);
    return left;
}

static inline void f32x3_subtract(f32 const left[3], f32 const right[3], f32 result[3]) {
    result[0] = left[0] - right[0];
    result[1] = left[1] - right[1];
    result[2] = left[2] - right[2];
}

static inline f32 *f32x3_subtract_assign(f32 left[3], f32 const right[3]) {
    f32x3_subtract(left, right, left);
    return left;
}

static inline f32 *f32x3_scale(f32 vector[3], f32 scalar) {
    vector[0] *= scalar;
    vector[1] *= scalar;
    vector[2] *= scalar;
    return vector;
}

static inline f32 *f32x3_negate(f32 vector[3]) {
    vector[0] = -vector[0];
    vector[1] = -vector[1];
    vector[2] = -vector[2];
    return vector;
}

static inline void f32x3_normalize(f32 vector[3]) {
    f32 length = sqrtf(f32x3_dot(vector, vector));
    vector[0] /= length;
    vector[1] /= length;
    vector[2] /= length;
}

static inline void f32x4_copy(f32 const source[4], f32 dest[4]) {
    dest[0] = source[0];
    dest[1] = source[1];
    dest[2] = source[2];
    dest[3] = source[3];
}

static inline f32 f32x4_dot(f32 const left[4], f32 const right[4]) {
    return left[0] * right[0] + left[1] * right[1] + left[2] * right[2] + left[3] * right[3];
}

static inline void f32x4_normalize(f32 vector[4]) {
    f32 length = sqrtf(f32x4_dot(vector, vector));
    vector[0] /= length;
    vector[1] /= length;
    vector[2] /= length;
    vector[3] /= length;
}

void f32x3x3_multiply_vector(f32 const matrix[3][3], f32 const vector[3], f32 result[3]) {
    f32 temp[3];
    temp[0] = f32x3_dot(matrix[0], vector);
    temp[1] = f32x3_dot(matrix[1], vector);
    temp[2] = f32x3_dot(matrix[2], vector);

    f32x3_copy(temp, result);
}

static inline void f32x4x4_zero(f32 matrix[4][4]) {
    memset(matrix, 0, 4 * 4 * sizeof(f32));
}

void f32x4x4_multiply_vector(f32 const matrix[4][4], f32 const vector[4], f32 result[4]) {
    f32 temp[4];
    temp[0] = f32x4_dot(matrix[0], vector);
    temp[1] = f32x4_dot(matrix[1], vector);
    temp[2] = f32x4_dot(matrix[2], vector);
    temp[3] = f32x4_dot(matrix[3], vector);

    f32x4_copy(temp, result);
}

static inline void quaternion_identity(f32 quaternion[4]) {
    quaternion[X] = 0;
    quaternion[Y] = 0;
    quaternion[Z] = 0;
    quaternion[W] = 1;
}

static inline void quaternion_from_vector(f32 const vector[3], f32 quaternion[4]) {
    quaternion[X] = vector[X];
    quaternion[Y] = vector[Y];
    quaternion[Z] = vector[Z];
    quaternion[W] = 0;
}

static inline void quaternion_from_axis_angle(f32 const axis[3], f32 angle, f32 result[4]) {
    f32 sine = sinf(angle / 2);
    f32 cosine = cosf(angle / 2);

    result[X] = sine * axis[X];
    result[Y] = sine * axis[Y];
    result[Z] = sine * axis[Z];
    result[W] = cosine;
}

void quaternion_multiply(f32 const left[4], f32 const right[4], f32 result[4]) {
    f32 temp[4];
    temp[X] = left[W] * right[X] + left[X] * right[W] + left[Y] * right[Z] - left[Z] * right[Y];
    temp[Y] = left[W] * right[Y] - left[X] * right[Z] + left[Y] * right[W] + left[Z] * right[X];
    temp[Z] = left[W] * right[Z] + left[X] * right[Y] - left[Y] * right[X] + left[Z] * right[W];
    temp[W] = left[W] * right[W] - left[X] * right[X] - left[Y] * right[Y] - left[Z] * right[Z];

    f32x4_copy(temp, result);
}

static inline void quaternion_conjugate(f32 const quaternion[4], f32 result[4]) {
    result[X] = -quaternion[X];
    result[Y] = -quaternion[Y];
    result[Z] = -quaternion[Z];
    result[W] = quaternion[W];
}

typedef struct {
    f32 translation[3];
    f32 orientation[4];
} Transform;

static inline void transform_identity(Transform *transform) {
    f32x3_copy((f32[]){ 0, 0, 0 }, transform->translation);
    quaternion_identity(transform->orientation);
}

void transform_rotate_around(Transform *transform, f32 const axis[3], f32 angle) {
    f32 rotation[4];
    quaternion_from_axis_angle(axis, angle, rotation);
    quaternion_multiply(rotation, transform->orientation, transform->orientation);

    // Maybe use this instead:
    // https://allenchou.net/2014/02/game-math-fast-re-normalization-of-unit-vectors
    f32x4_normalize(transform->orientation);
}

static inline void transform_translate(Transform *transform, f32 const translation[3]) {
    f32x3_add_assign(transform->translation, translation);
}

void transform_apply(Transform const *transform, f32 vector[3]) {
    f32 result[4];
    quaternion_from_vector(vector, result);

    f32 orientation_conjugate[4];
    quaternion_conjugate(transform->orientation, orientation_conjugate);

    quaternion_multiply(transform->orientation, result, result);
    quaternion_multiply(result, orientation_conjugate, result);

    f32x3_add_assign(result, transform->translation);

    f32x3_copy(result, vector);
}

typedef struct {
    f32 yaw;
    f32 pitch;
    f32 position[3];
} Camera;

void camera_apply(Camera const *camera, f32 vector[3]) {
    Transform transform;
    transform_identity(&transform);

    f32 translation[3];
    f32x3_copy(camera->position, translation);
    f32x3_negate(translation);

    transform_translate(&transform, translation);
    transform_rotate_around(&transform, (f32[]){ 0, 1, 0 }, camera->yaw);
    transform_rotate_around(&transform, (f32[]){ 1, 0, 0 }, camera->pitch);

    transform_apply(&transform, vector);
}

void perspective_projection(
    f32 aspect_ratio,
    f32 vertical_fov,
    f32 near,
    f32 far,
    f32 result[4][4]
) {
    f32 tangent = tanf(vertical_fov / 2);

    f32x4x4_zero(result);
    result[0][0] = 1 / (aspect_ratio * tangent);
    result[1][1] = 1 / tangent;
    result[2][2] = near / (near - far);
    result[2][3] = -near * far / (near - far);
    result[3][2] = -1;
}
