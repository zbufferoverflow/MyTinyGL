/*
 * MyTinyGL - OpenGL 1.x Software Renderer
 * Copyright (c) 2025 zbufferoverflow (Eliezer Solinger)
 * https://github.com/zbufferoverflow/MyTinyGL
 * SPDX-License-Identifier: MIT
 *
 * graphics.h - Math primitives (vec3, vec4, mat4, color)
 */

#ifndef MYTINYGL_GRAPHICS_H
#define MYTINYGL_GRAPHICS_H

#include <math.h>
#include <stdint.h>

/* Vec3 */

typedef struct {
    float x, y, z;
} vec3_t;

static inline vec3_t vec3(float x, float y, float z) {
    return (vec3_t){x, y, z};
}

static inline vec3_t vec3_add(vec3_t a, vec3_t b) {
    return (vec3_t){a.x + b.x, a.y + b.y, a.z + b.z};
}

static inline vec3_t vec3_sub(vec3_t a, vec3_t b) {
    return (vec3_t){a.x - b.x, a.y - b.y, a.z - b.z};
}

static inline vec3_t vec3_scale(vec3_t v, float s) {
    return (vec3_t){v.x * s, v.y * s, v.z * s};
}

static inline float vec3_dot(vec3_t a, vec3_t b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline vec3_t vec3_cross(vec3_t a, vec3_t b) {
    return (vec3_t){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

static inline float vec3_length(vec3_t v) {
    return sqrtf(vec3_dot(v, v));
}

static inline vec3_t vec3_normalize(vec3_t v) {
    float len = vec3_length(v);
    if (len > 0.0f) {
        return vec3_scale(v, 1.0f / len);
    }
    return v;
}

/* Vec2 */

typedef struct {
    float x, y;
} vec2_t;

static inline vec2_t vec2(float x, float y) {
    return (vec2_t){x, y};
}

/* Vec4 */

typedef struct {
    float x, y, z, w;
} vec4_t;

static inline vec4_t vec4(float x, float y, float z, float w) {
    return (vec4_t){x, y, z, w};
}

static inline vec4_t vec4_from_vec3(vec3_t v, float w) {
    return (vec4_t){v.x, v.y, v.z, w};
}

static inline vec3_t vec4_to_vec3(vec4_t v) {
    return (vec3_t){v.x, v.y, v.z};
}

static inline vec4_t vec4_add(vec4_t a, vec4_t b) {
    return (vec4_t){a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}

static inline vec4_t vec4_sub(vec4_t a, vec4_t b) {
    return (vec4_t){a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}

static inline vec4_t vec4_scale(vec4_t v, float s) {
    return (vec4_t){v.x * s, v.y * s, v.z * s, v.w * s};
}

static inline float vec4_dot(vec4_t a, vec4_t b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

/* Mat4 - column major (OpenGL convention) */

typedef struct {
    float m[16];
} mat4_t;

static inline mat4_t mat4_identity(void) {
    return (mat4_t){{
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    }};
}

static inline mat4_t mat4_from_array(const float *arr) {
    mat4_t m;
    for (int i = 0; i < 16; i++) m.m[i] = arr[i];
    return m;
}

static inline void mat4_to_array(mat4_t m, float *arr) {
    for (int i = 0; i < 16; i++) arr[i] = m.m[i];
}

static inline vec4_t mat4_mul_vec4(mat4_t m, vec4_t v) {
    return (vec4_t){
        m.m[0]*v.x + m.m[4]*v.y + m.m[8]*v.z  + m.m[12]*v.w,
        m.m[1]*v.x + m.m[5]*v.y + m.m[9]*v.z  + m.m[13]*v.w,
        m.m[2]*v.x + m.m[6]*v.y + m.m[10]*v.z + m.m[14]*v.w,
        m.m[3]*v.x + m.m[7]*v.y + m.m[11]*v.z + m.m[15]*v.w
    };
}

static inline mat4_t mat4_mul(mat4_t a, mat4_t b) {
    mat4_t result;
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            result.m[col * 4 + row] =
                a.m[0 * 4 + row] * b.m[col * 4 + 0] +
                a.m[1 * 4 + row] * b.m[col * 4 + 1] +
                a.m[2 * 4 + row] * b.m[col * 4 + 2] +
                a.m[3 * 4 + row] * b.m[col * 4 + 3];
        }
    }
    return result;
}

static inline mat4_t mat4_translate(float x, float y, float z) {
    return (mat4_t){{
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        x, y, z, 1
    }};
}

static inline mat4_t mat4_scale(float x, float y, float z) {
    return (mat4_t){{
        x, 0, 0, 0,
        0, y, 0, 0,
        0, 0, z, 0,
        0, 0, 0, 1
    }};
}

static inline mat4_t mat4_rotate(float angle_deg, float x, float y, float z) {
    float rad = angle_deg * 3.14159265358979f / 180.0f;
    float c = cosf(rad);
    float s = sinf(rad);
    float len = sqrtf(x*x + y*y + z*z);
    if (len == 0) return mat4_identity();
    x /= len; y /= len; z /= len;

    return (mat4_t){{
        x*x*(1-c)+c,   y*x*(1-c)+z*s, x*z*(1-c)-y*s, 0,
        x*y*(1-c)-z*s, y*y*(1-c)+c,   y*z*(1-c)+x*s, 0,
        x*z*(1-c)+y*s, y*z*(1-c)-x*s, z*z*(1-c)+c,   0,
        0, 0, 0, 1
    }};
}

static inline mat4_t mat4_ortho(float left, float right, float bottom, float top, float near, float far) {
    /* Protect against divide-by-zero */
    float rl = right - left;
    float tb = top - bottom;
    float fn = far - near;
    if (rl == 0.0f || tb == 0.0f || fn == 0.0f) {
        return mat4_identity();
    }
    return (mat4_t){{
        2.0f / rl, 0, 0, 0,
        0, 2.0f / tb, 0, 0,
        0, 0, -2.0f / fn, 0,
        -(right + left) / rl,
        -(top + bottom) / tb,
        -(far + near) / fn,
        1
    }};
}

static inline mat4_t mat4_frustum(float left, float right, float bottom, float top, float near, float far) {
    /* Protect against divide-by-zero */
    float rl = right - left;
    float tb = top - bottom;
    float fn = far - near;
    if (rl == 0.0f || tb == 0.0f || fn == 0.0f) {
        return mat4_identity();
    }
    return (mat4_t){{
        2.0f * near / rl, 0, 0, 0,
        0, 2.0f * near / tb, 0, 0,
        (right + left) / rl,
        (top + bottom) / tb,
        -(far + near) / fn,
        -1,
        0, 0, -2.0f * far * near / fn, 0
    }};
}

/* Extract upper-left 3x3 from mat4, invert it, and transpose for normal transformation.
 * For orthogonal matrices (no non-uniform scaling), this equals the original 3x3.
 * Returns the result embedded in a mat4 (with w components zeroed for direction transform). */
static inline mat4_t mat4_normal_matrix(mat4_t m) {
    /* Extract 3x3 */
    float a = m.m[0], b = m.m[1], c = m.m[2];
    float d = m.m[4], e = m.m[5], f = m.m[6];
    float g = m.m[8], h = m.m[9], i = m.m[10];

    /* Compute determinant of 3x3 */
    float det = a*(e*i - f*h) - b*(d*i - f*g) + c*(d*h - e*g);
    if (fabsf(det) < 1e-10f) {
        /* Singular matrix, return identity */
        return mat4_identity();
    }

    float inv_det = 1.0f / det;

    /* Compute inverse of 3x3, then transpose (combine into one step) */
    /* inverse[i][j] = cofactor[j][i] / det, transpose swaps i,j */
    /* So result[i][j] = cofactor[i][j] / det (no swap needed) */
    mat4_t r = {{0}};
    r.m[0]  = (e*i - f*h) * inv_det;
    r.m[1]  = (d*i - f*g) * -inv_det;
    r.m[2]  = (d*h - e*g) * inv_det;
    r.m[4]  = (b*i - c*h) * -inv_det;
    r.m[5]  = (a*i - c*g) * inv_det;
    r.m[6]  = (a*h - b*g) * -inv_det;
    r.m[8]  = (b*f - c*e) * inv_det;
    r.m[9]  = (a*f - c*d) * -inv_det;
    r.m[10] = (a*e - b*d) * inv_det;
    r.m[15] = 1.0f;

    return r;
}

/* Color */

typedef struct {
    float r, g, b, a;
} color_t;

static inline color_t color(float r, float g, float b, float a) {
    return (color_t){r, g, b, a};
}

static inline color_t color_from_rgb(float r, float g, float b) {
    return (color_t){r, g, b, 1.0f};
}

static inline color_t color_add(color_t a, color_t b) {
    return (color_t){a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a};
}

static inline color_t color_mul(color_t a, color_t b) {
    return (color_t){a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a};
}

static inline color_t color_scale(color_t c, float s) {
    return (color_t){c.r * s, c.g * s, c.b * s, c.a * s};
}

/* Scale RGB only, preserve alpha */
static inline color_t color_scale_rgb(color_t c, float s) {
    return (color_t){c.r * s, c.g * s, c.b * s, c.a};
}

static inline color_t color_lerp(color_t a, color_t b, float t) {
    return color_add(color_scale(a, 1.0f - t), color_scale(b, t));
}

/* Lerp RGB only, preserve first color's alpha */
static inline color_t color_lerp_rgb(color_t a, color_t b, float t) {
    return (color_t){
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a
    };
}

/* Add RGB only, multiply alpha */
static inline color_t color_add_rgb_mul_a(color_t a, color_t b) {
    return (color_t){
        a.r + b.r,
        a.g + b.g,
        a.b + b.b,
        a.a * b.a
    };
}

/* Blend RGB with factor, multiply alpha: result.rgb = a.rgb*(1-t) + b.rgb*t, result.a = a.a * b.a */
static inline color_t color_blend_rgb_mul_a(color_t a, color_t b, float t) {
    return (color_t){
        a.r * (1.0f - t) + b.r * t,
        a.g * (1.0f - t) + b.g * t,
        a.b * (1.0f - t) + b.b * t,
        a.a * b.a
    };
}

/* Per-channel blend: result.rgb[i] = a.rgb[i]*(1-b.rgb[i]) + env.rgb[i]*b.rgb[i], result.a = a.a * b.a */
static inline color_t color_blend_per_channel(color_t a, color_t b, color_t env) {
    return (color_t){
        a.r * (1.0f - b.r) + env.r * b.r,
        a.g * (1.0f - b.g) + env.g * b.g,
        a.b * (1.0f - b.b) + env.b * b.b,
        a.a * b.a
    };
}

static inline uint32_t color_to_rgba32(color_t c) {
    /* Clamp to [0, 1] before conversion to avoid undefined behavior */
    float rf = c.r < 0.0f ? 0.0f : (c.r > 1.0f ? 1.0f : c.r);
    float gf = c.g < 0.0f ? 0.0f : (c.g > 1.0f ? 1.0f : c.g);
    float bf = c.b < 0.0f ? 0.0f : (c.b > 1.0f ? 1.0f : c.b);
    float af = c.a < 0.0f ? 0.0f : (c.a > 1.0f ? 1.0f : c.a);
    uint8_t r = (uint8_t)(rf * 255.0f);
    uint8_t g = (uint8_t)(gf * 255.0f);
    uint8_t b = (uint8_t)(bf * 255.0f);
    uint8_t a = (uint8_t)(af * 255.0f);
    return (a << 24) | (b << 16) | (g << 8) | r;
}

static inline color_t color_from_rgba32(uint32_t pixel) {
    return (color_t){
        (pixel & 0xFF) / 255.0f,
        ((pixel >> 8) & 0xFF) / 255.0f,
        ((pixel >> 16) & 0xFF) / 255.0f,
        ((pixel >> 24) & 0xFF) / 255.0f
    };
}

static inline color_t color_clamp(color_t c) {
    return (color_t){
        c.r < 0 ? 0 : (c.r > 1 ? 1 : c.r),
        c.g < 0 ? 0 : (c.g > 1 ? 1 : c.g),
        c.b < 0 ? 0 : (c.b > 1 ? 1 : c.b),
        c.a < 0 ? 0 : (c.a > 1 ? 1 : c.a)
    };
}

/* Scalar lerp */
static inline float lerpf(float a, float b, float t) {
    return a + t * (b - a);
}

/* Vec2 lerp */
static inline vec2_t vec2_lerp(vec2_t a, vec2_t b, float t) {
    return (vec2_t){
        a.x + t * (b.x - a.x),
        a.y + t * (b.y - a.y)
    };
}

/* Vec3 lerp */
static inline vec3_t vec3_lerp(vec3_t a, vec3_t b, float t) {
    return (vec3_t){
        a.x + t * (b.x - a.x),
        a.y + t * (b.y - a.y),
        a.z + t * (b.z - a.z)
    };
}

/* Vec4 lerp */
static inline vec4_t vec4_lerp(vec4_t a, vec4_t b, float t) {
    return (vec4_t){
        a.x + t * (b.x - a.x),
        a.y + t * (b.y - a.y),
        a.z + t * (b.z - a.z),
        a.w + t * (b.w - a.w)
    };
}

/* Barycentric interpolation (3 points with weights b0, b1, b2 where b0+b1+b2=1) */
static inline vec3_t vec3_bary(vec3_t a, vec3_t b, vec3_t c, float b0, float b1, float b2) {
    return (vec3_t){
        a.x * b0 + b.x * b1 + c.x * b2,
        a.y * b0 + b.y * b1 + c.y * b2,
        a.z * b0 + b.z * b1 + c.z * b2
    };
}

static inline color_t color_bary(color_t a, color_t b, color_t c, float b0, float b1, float b2) {
    return (color_t){
        a.r * b0 + b.r * b1 + c.r * b2,
        a.g * b0 + b.g * b1 + c.g * b2,
        a.b * b0 + b.b * b1 + c.b * b2,
        a.a * b0 + b.a * b1 + c.a * b2
    };
}

/* Byte-level color packing (for texture uploads) */

static inline uint32_t rgba_bytes_to_rgba32(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (a << 24) | (b << 16) | (g << 8) | r;
}

static inline uint32_t rgb_bytes_to_rgba32(uint8_t r, uint8_t g, uint8_t b) {
    return (0xFF << 24) | (b << 16) | (g << 8) | r;
}

static inline uint32_t luminance_to_rgba32(uint8_t l) {
    return (0xFF << 24) | (l << 16) | (l << 8) | l;
}

static inline uint32_t luminance_alpha_to_rgba32(uint8_t l, uint8_t a) {
    return (a << 24) | (l << 16) | (l << 8) | l;
}

/* Vertex - interleaved attributes for rasterization */

typedef struct vertex_t {
    vec4_t position;      /* Clip-space position */
    color_t color;        /* Vertex color (lit for Gouraud, unlit for Phong) */
    vec2_t texcoord;
    vec3_t normal;        /* Object-space normal (for Phong: eye-space) */
    float eye_z;          /* Eye-space Z for fog */
    vec3_t eye_pos;       /* Eye-space position (for Phong shading) */
    vec3_t eye_normal;    /* Eye-space normal (for Phong shading) */
} vertex_t;

static inline vertex_t vertex_full(vec4_t pos, color_t col, vec2_t tex, vec3_t norm, float ez) {
    vertex_t v;
    v.position = pos;
    v.color = col;
    v.texcoord = tex;
    v.normal = norm;
    v.eye_z = ez;
    v.eye_pos = vec3(0, 0, 0);
    v.eye_normal = vec3(0, 0, 1);
    return v;
}

static inline vertex_t vertex(vec4_t pos, color_t col, vec2_t tex, vec3_t norm) {
    return vertex_full(pos, col, tex, norm, 0.0f);
}

/* Interpolate all vertex attributes */
static inline vertex_t vertex_lerp(vertex_t *a, vertex_t *b, float t) {
    vertex_t v;
    v.position   = vec4_lerp(a->position, b->position, t);
    v.color      = color_lerp(a->color, b->color, t);
    v.texcoord   = vec2_lerp(a->texcoord, b->texcoord, t);
    v.normal     = vec3_lerp(a->normal, b->normal, t);
    v.eye_z      = lerpf(a->eye_z, b->eye_z, t);
    v.eye_pos    = vec3_lerp(a->eye_pos, b->eye_pos, t);
    v.eye_normal = vec3_lerp(a->eye_normal, b->eye_normal, t);
    return v;
}

#endif /* MYTINYGL_GRAPHICS_H */
