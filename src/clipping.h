/*
 * MyTinyGL - OpenGL 1.x Software Renderer
 * Copyright (c) 2025 zbufferoverflow (Eliezer Solinger)
 * https://github.com/zbufferoverflow/MyTinyGL
 * SPDX-License-Identifier: MIT
 *
 * clipping.h - Frustum clipping
 */

#ifndef MYTINYGL_CLIPPING_H
#define MYTINYGL_CLIPPING_H

#include "graphics.h"

/* Maximum vertices after clipping (triangle clipped against 6 planes) */
#define MAX_CLIP_VERTS 12

/* Plane IDs for snapping */
#define PLANE_NEAR   0
#define PLANE_FAR    1
#define PLANE_LEFT   2
#define PLANE_RIGHT  3
#define PLANE_BOTTOM 4
#define PLANE_TOP    5

/* Plane distance functions (positive = inside, negative = outside) */
static inline float clip_near(vec4_t *v)   { return v->z + v->w; }  /* z >= -w */
static inline float clip_far(vec4_t *v)    { return v->w - v->z; }  /* z <= w */
static inline float clip_left(vec4_t *v)   { return v->x + v->w; }  /* x >= -w */
static inline float clip_right(vec4_t *v)  { return v->w - v->x; }  /* x <= w */
static inline float clip_bottom(vec4_t *v) { return v->y + v->w; }  /* y >= -w */
static inline float clip_top(vec4_t *v)    { return v->w - v->y; }  /* y <= w */

/* Snap vertex to plane after interpolation to fix floating-point precision issues.
 * For extreme coordinates like 1e10, interpolation can place vertices incorrectly
 * due to precision loss. This forces the vertex to lie exactly on the clip plane. */
static inline void snap_to_plane(vertex_t *v, int plane_id)
{
    switch (plane_id) {
        case PLANE_LEFT:   v->position.x = -v->position.w; break;  /* x = -w */
        case PLANE_RIGHT:  v->position.x =  v->position.w; break;  /* x = w */
        case PLANE_BOTTOM: v->position.y = -v->position.w; break;  /* y = -w */
        case PLANE_TOP:    v->position.y =  v->position.w; break;  /* y = w */
        case PLANE_NEAR:   v->position.z = -v->position.w; break;  /* z = -w */
        case PLANE_FAR:    v->position.z =  v->position.w; break;  /* z = w */
    }
}

/* Clip polygon against a single plane using Sutherland-Hodgman algorithm */
static inline int clip_polygon_plane_id(vertex_t *in, int in_count, vertex_t *out,
                                        float (*plane_func)(vec4_t *), int plane_id)
{
    if (in_count == 0) return 0;

    int out_count = 0;
    vertex_t *prev = &in[in_count - 1];
    float prev_dist = plane_func(&prev->position);

    for (int i = 0; i < in_count; i++) {
        vertex_t *curr = &in[i];
        float curr_dist = plane_func(&curr->position);

        if (prev_dist >= 0) {
            /* Previous vertex is inside */
            if (curr_dist >= 0) {
                /* Both inside: emit current */
                out[out_count++] = *curr;
            } else {
                /* Going out: emit intersection */
                float denom = prev_dist - curr_dist;
                if (fabsf(denom) > 1e-10f) {
                    float t = prev_dist / denom;
                    out[out_count] = vertex_lerp(prev, curr, t);
                    snap_to_plane(&out[out_count], plane_id);
                    out_count++;
                }
            }
        } else {
            /* Previous vertex is outside */
            if (curr_dist >= 0) {
                /* Coming in: emit intersection, then current */
                float denom = prev_dist - curr_dist;
                if (fabsf(denom) > 1e-10f) {
                    float t = prev_dist / denom;
                    out[out_count] = vertex_lerp(prev, curr, t);
                    snap_to_plane(&out[out_count], plane_id);
                    out_count++;
                }
                out[out_count++] = *curr;
            }
            /* Both outside: emit nothing */
        }

        prev = curr;
        prev_dist = curr_dist;
    }

    return out_count;
}

/* Clip triangle against all 6 frustum planes
 * Input: triangle (3 vertices in clip space)
 * Output: clipped polygon vertices (up to MAX_CLIP_VERTS)
 * Returns: number of output vertices (0 if fully clipped)
 */
static inline int clip_triangle(vertex_t *triangle, vertex_t *out)
{
    vertex_t temp1[MAX_CLIP_VERTS];
    vertex_t temp2[MAX_CLIP_VERTS];
    int count;

    /* Clip against each plane in sequence */
    count = clip_polygon_plane_id(triangle, 3, temp1, clip_near, PLANE_NEAR);
    if (count == 0) return 0;
    count = clip_polygon_plane_id(temp1, count, temp2, clip_far, PLANE_FAR);
    if (count == 0) return 0;
    count = clip_polygon_plane_id(temp2, count, temp1, clip_left, PLANE_LEFT);
    if (count == 0) return 0;
    count = clip_polygon_plane_id(temp1, count, temp2, clip_right, PLANE_RIGHT);
    if (count == 0) return 0;
    count = clip_polygon_plane_id(temp2, count, temp1, clip_bottom, PLANE_BOTTOM);
    if (count == 0) return 0;
    count = clip_polygon_plane_id(temp1, count, out, clip_top, PLANE_TOP);

    return count;
}

/* Cohen-Sutherland line clipping outcodes */
#define OUTCODE_INSIDE 0
#define OUTCODE_LEFT   1
#define OUTCODE_RIGHT  2
#define OUTCODE_BOTTOM 4
#define OUTCODE_TOP    8
#define OUTCODE_NEAR   16
#define OUTCODE_FAR    32

/* Compute outcode for a point in clip space */
static inline int compute_outcode(vec4_t *v)
{
    int code = OUTCODE_INSIDE;
    if (v->x < -v->w) code |= OUTCODE_LEFT;
    else if (v->x > v->w) code |= OUTCODE_RIGHT;
    if (v->y < -v->w) code |= OUTCODE_BOTTOM;
    else if (v->y > v->w) code |= OUTCODE_TOP;
    if (v->z < -v->w) code |= OUTCODE_NEAR;
    else if (v->z > v->w) code |= OUTCODE_FAR;
    return code;
}

/* Clip line segment against frustum. Returns 1 if visible, 0 if fully clipped.
 * Modifies v0 and v1 in place with clipped vertices. */
static inline int clip_line(vertex_t *v0, vertex_t *v1)
{
    int code0 = compute_outcode(&v0->position);
    int code1 = compute_outcode(&v1->position);

    while (1) {
        if (!(code0 | code1)) {
            /* Both inside - accept */
            return 1;
        }
        if (code0 & code1) {
            /* Both outside same half-plane - reject */
            return 0;
        }

        /* Pick point outside the frustum */
        int code_out = code0 ? code0 : code1;
        float t = 0.0f;
        int plane_id = -1;

        /* Find intersection with clip plane */
        vec4_t *p0 = &v0->position;
        vec4_t *p1 = &v1->position;
        float d0 = 0.0f, d1 = 0.0f;

        if (code_out & OUTCODE_LEFT) {
            /* x = -w plane: x + w = 0 */
            d0 = p0->x + p0->w;
            d1 = p1->x + p1->w;
            plane_id = PLANE_LEFT;
        } else if (code_out & OUTCODE_RIGHT) {
            /* x = w plane: w - x = 0 */
            d0 = p0->w - p0->x;
            d1 = p1->w - p1->x;
            plane_id = PLANE_RIGHT;
        } else if (code_out & OUTCODE_BOTTOM) {
            /* y = -w plane: y + w = 0 */
            d0 = p0->y + p0->w;
            d1 = p1->y + p1->w;
            plane_id = PLANE_BOTTOM;
        } else if (code_out & OUTCODE_TOP) {
            /* y = w plane: w - y = 0 */
            d0 = p0->w - p0->y;
            d1 = p1->w - p1->y;
            plane_id = PLANE_TOP;
        } else if (code_out & OUTCODE_NEAR) {
            /* z = -w plane: z + w = 0 */
            d0 = p0->z + p0->w;
            d1 = p1->z + p1->w;
            plane_id = PLANE_NEAR;
        } else if (code_out & OUTCODE_FAR) {
            /* z = w plane: w - z = 0 */
            d0 = p0->w - p0->z;
            d1 = p1->w - p1->z;
            plane_id = PLANE_FAR;
        }

        /* Avoid division by zero */
        float denom = d0 - d1;
        if (fabsf(denom) < 1e-10f) {
            /* Line is parallel to plane, reject */
            return 0;
        }
        t = d0 / denom;

        /* Compute interpolated vertex and snap to plane */
        vertex_t clipped = vertex_lerp(v0, v1, t);
        snap_to_plane(&clipped, plane_id);

        if (code_out == code0) {
            *v0 = clipped;
            code0 = compute_outcode(&v0->position);
        } else {
            *v1 = clipped;
            code1 = compute_outcode(&v1->position);
        }
    }
}

#endif /* MYTINYGL_CLIPPING_H */
