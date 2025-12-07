/*
 * MyTinyGL - OpenGL 1.x Software Renderer
 * Copyright (c) 2025 zbufferoverflow (Eliezer Solinger)
 * https://github.com/zbufferoverflow/MyTinyGL
 * SPDX-License-Identifier: MIT
 *
 * textures.h - Texture management
 */

#ifndef MYTINYGL_TEXTURES_H
#define MYTINYGL_TEXTURES_H

#include <stdint.h>
#include <stddef.h>

/* Texture limits */
#define MYTGL_MAX_TEXTURE_SIZE 2048
#define GL_MAX_TEXTURES 256

/* Texture object */
typedef struct {
    int32_t width;
    int32_t height;
    uint32_t *pixels;  /* Always RGBA8888 internally (level 0) */

    /* Mipmap level 1 (quarter resolution) - generated lazily */
    int32_t mip1_width;
    int32_t mip1_height;
    uint32_t *mip1_pixels;

    /* Texture parameters */
    int32_t min_filter;
    int32_t mag_filter;
    int32_t wrap_s;
    int32_t wrap_t;

    /* Allocation tracking */
    uint8_t allocated;
} texture_t;

/* Texture storage (dynamic array with hard limit) */
typedef struct {
    texture_t *textures;
    size_t count;
    size_t capacity;
} texture_store_t;

/* Texture store management */
int texture_store_init(texture_store_t *store);
void texture_store_free(texture_store_t *store);

/* Texture allocation - returns texture ID (1-based like OpenGL), 0 on failure */
uint32_t texture_alloc(texture_store_t *store);
void texture_free(texture_store_t *store, uint32_t id);
texture_t *texture_get(texture_store_t *store, uint32_t id);

/* Texture data upload - converts input format to internal RGBA */
int texture_upload_rgba(texture_t *tex, int32_t width, int32_t height, const uint8_t *data);
int texture_upload_rgb(texture_t *tex, int32_t width, int32_t height, const uint8_t *data);
int texture_upload_luminance(texture_t *tex, int32_t width, int32_t height, const uint8_t *data);
int texture_upload_luminance_alpha(texture_t *tex, int32_t width, int32_t height, const uint8_t *data);

/* Texture sampling */
uint32_t texture_sample(const texture_t *tex, float u, float v);
uint32_t texture_sample_lod(const texture_t *tex, float u, float v, float lod);
uint32_t texture_sample_nearest(const texture_t *tex, int32_t x, int32_t y);

/* Utility */
void texture_clear(texture_t *tex);

#endif /* MYTINYGL_TEXTURES_H */
