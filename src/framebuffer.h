/*
 * MyTinyGL - OpenGL 1.x Software Renderer
 * Copyright (c) 2025 zbufferoverflow (Eliezer Solinger)
 * https://github.com/zbufferoverflow/MyTinyGL
 * SPDX-License-Identifier: MIT
 *
 * framebuffer.h - Framebuffer management
 */

#ifndef MYTINYGL_FRAMEBUFFER_H
#define MYTINYGL_FRAMEBUFFER_H

#include "graphics.h"
#include "allocation.h"
#include <stddef.h>

typedef uint32_t pixel_t;

typedef struct {
    int32_t width;
    int32_t height;
    pixel_t *color;
    float *depth;
    uint8_t *stencil;
} framebuffer_t;

/* Framebuffer management */

/* Maximum framebuffer dimension to prevent integer overflow */
#define MYTGL_MAX_FRAMEBUFFER_DIM 16384

static inline int framebuffer_init(framebuffer_t *fb, int32_t width, int32_t height) {
    /* Validate dimensions to prevent integer overflow */
    if (width <= 0 || height <= 0) return -1;
    if (width > MYTGL_MAX_FRAMEBUFFER_DIM || height > MYTGL_MAX_FRAMEBUFFER_DIM) return -1;

    /* Check for overflow: width * height * sizeof(pixel_t) */
    size_t pixel_count = (size_t)width * (size_t)height;
    if (pixel_count > SIZE_MAX / sizeof(pixel_t)) return -1;
    if (pixel_count > SIZE_MAX / sizeof(float)) return -1;

    fb->width = width;
    fb->height = height;
    fb->color = (pixel_t *)mtgl_alloc(pixel_count * sizeof(pixel_t));
    fb->depth = (float *)mtgl_alloc(pixel_count * sizeof(float));
    fb->stencil = (uint8_t *)mtgl_alloc(pixel_count * sizeof(uint8_t));

    if (!fb->color || !fb->depth || !fb->stencil) {
        mtgl_free(fb->color);
        mtgl_free(fb->depth);
        mtgl_free(fb->stencil);
        fb->color = NULL;
        fb->depth = NULL;
        fb->stencil = NULL;
        return -1;
    }
    return 0;
}

static inline void framebuffer_free(framebuffer_t *fb) {
    mtgl_free(fb->color);
    mtgl_free(fb->depth);
    mtgl_free(fb->stencil);
    fb->color = NULL;
    fb->depth = NULL;
    fb->stencil = NULL;
}

static inline void framebuffer_clear_color(framebuffer_t *fb, color_t c) {
    pixel_t pixel = color_to_rgba32(c);
    int32_t size = fb->width * fb->height;
    for (int32_t i = 0; i < size; i++) {
        fb->color[i] = pixel;
    }
}

static inline void framebuffer_clear_depth(framebuffer_t *fb, float depth) {
    int32_t size = fb->width * fb->height;
    for (int32_t i = 0; i < size; i++) {
        fb->depth[i] = depth;
    }
}

static inline void framebuffer_clear_stencil(framebuffer_t *fb, uint8_t value) {
    int32_t size = fb->width * fb->height;
    for (int32_t i = 0; i < size; i++) {
        fb->stencil[i] = value;
    }
}

/* Pixel access */
static inline void framebuffer_putpixel(framebuffer_t *fb, int32_t x, int32_t y, pixel_t c) {
#ifndef FRAMEBUFFER_NO_BOUNDS_CHECK
    if (x < 0 || x >= fb->width || y < 0 || y >= fb->height) return;
#endif
    fb->color[y * fb->width + x] = c;
}

static inline pixel_t framebuffer_getpixel(framebuffer_t *fb, int32_t x, int32_t y) {
#ifndef FRAMEBUFFER_NO_BOUNDS_CHECK
    if (x < 0 || x >= fb->width || y < 0 || y >= fb->height) return 0;
#endif
    return fb->color[y * fb->width + x];
}

/* Depth access */
static inline void framebuffer_putdepth(framebuffer_t *fb, int32_t x, int32_t y, float d) {
#ifndef FRAMEBUFFER_NO_BOUNDS_CHECK
    if (x < 0 || x >= fb->width || y < 0 || y >= fb->height) return;
#endif
    fb->depth[y * fb->width + x] = d;
}

static inline float framebuffer_getdepth(framebuffer_t *fb, int32_t x, int32_t y) {
#ifndef FRAMEBUFFER_NO_BOUNDS_CHECK
    if (x < 0 || x >= fb->width || y < 0 || y >= fb->height) return 1.0f;
#endif
    return fb->depth[y * fb->width + x];
}

/* Stencil access */
static inline void framebuffer_putstencil(framebuffer_t *fb, int32_t x, int32_t y, uint8_t s) {
#ifndef FRAMEBUFFER_NO_BOUNDS_CHECK
    if (x < 0 || x >= fb->width || y < 0 || y >= fb->height) return;
#endif
    fb->stencil[y * fb->width + x] = s;
}

static inline uint8_t framebuffer_getstencil(framebuffer_t *fb, int32_t x, int32_t y) {
#ifndef FRAMEBUFFER_NO_BOUNDS_CHECK
    if (x < 0 || x >= fb->width || y < 0 || y >= fb->height) return 0;
#endif
    return fb->stencil[y * fb->width + x];
}

/* Get raw buffer pointer (for blitting to screen) */
static inline pixel_t *framebuffer_get_color_buffer(framebuffer_t *fb) {
    return fb->color;
}

static inline size_t framebuffer_get_pitch(framebuffer_t *fb) {
    return fb->width * sizeof(pixel_t);
}

/* Horizontal line - fast memset-style with bounds checking */
static inline void framebuffer_hline(framebuffer_t *fb, int32_t x0, int32_t x1, int32_t y, pixel_t c) {
#ifndef FRAMEBUFFER_NO_BOUNDS_CHECK
    /* Clip Y */
    if (y < 0 || y >= fb->height) return;
    /* Clip X range */
    if (x0 > x1) { int32_t tmp = x0; x0 = x1; x1 = tmp; }
    if (x1 < 0 || x0 >= fb->width) return;
    if (x0 < 0) x0 = 0;
    if (x1 >= fb->width) x1 = fb->width - 1;
#endif
    pixel_t *p = fb->color + y * fb->width + x0;
    int32_t len = x1 - x0 + 1;
    while (len--) *p++ = c;
}

/* Vertical line with bounds checking */
static inline void framebuffer_vline(framebuffer_t *fb, int32_t x, int32_t y0, int32_t y1, pixel_t c) {
#ifndef FRAMEBUFFER_NO_BOUNDS_CHECK
    /* Clip X */
    if (x < 0 || x >= fb->width) return;
    /* Clip Y range */
    if (y0 > y1) { int32_t tmp = y0; y0 = y1; y1 = tmp; }
    if (y1 < 0 || y0 >= fb->height) return;
    if (y0 < 0) y0 = 0;
    if (y1 >= fb->height) y1 = fb->height - 1;
#endif
    pixel_t *p = fb->color + y0 * fb->width + x;
    int32_t stride = fb->width;
    int32_t len = y1 - y0 + 1;
    while (len--) {
        *p = c;
        p += stride;
    }
}

/* Bresenham line - uses putpixel for bounds safety */
static inline void framebuffer_drawline(framebuffer_t *fb, int32_t x0, int32_t y0, int32_t x1, int32_t y1, pixel_t c) {
    int32_t dx = x1 - x0;
    int32_t dy = y1 - y0;
    int32_t adx = dx < 0 ? -dx : dx;
    int32_t ady = dy < 0 ? -dy : dy;
    int32_t sx = dx < 0 ? -1 : 1;
    int32_t sy = dy < 0 ? -1 : 1;
    int32_t err = adx - ady;

    for (;;) {
        framebuffer_putpixel(fb, x0, y0, c);

        if (x0 == x1 && y0 == y1) break;

        int32_t e2 = err * 2;
        if (e2 > -ady) {
            err -= ady;
            x0 += sx;
        }
        if (e2 < adx) {
            err += adx;
            y0 += sy;
        }
    }
}

#endif /* MYTINYGL_FRAMEBUFFER_H */
