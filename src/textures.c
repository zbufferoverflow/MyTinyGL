/*
 * MyTinyGL - OpenGL 1.x Fixed Function Pipeline
 * textures.c - Texture management implementation
 */

#include "textures.h"
#include "graphics.h"
#include "allocation.h"
#include "GL/gl.h"

#define INITIAL_TEXTURE_CAPACITY 16

/* Initialize texture store */
int texture_store_init(texture_store_t *store)
{
    store->textures = NULL;
    store->count = 0;
    store->capacity = 0;
    return 0;
}

/* Free all textures and storage */
void texture_store_free(texture_store_t *store)
{
    if (store->textures) {
        for (size_t i = 0; i < store->count; i++) {
            if (store->textures[i].pixels) {
                mtgl_free(store->textures[i].pixels);
            }
            if (store->textures[i].mip1_pixels) {
                mtgl_free(store->textures[i].mip1_pixels);
            }
        }
        mtgl_free(store->textures);
        store->textures = NULL;
    }
    store->count = 0;
    store->capacity = 0;
}

/* Initialize a texture slot with defaults */
static void texture_init_slot(texture_t *tex)
{
    tex->width = 0;
    tex->height = 0;
    tex->pixels = NULL;
    tex->mip1_width = 0;
    tex->mip1_height = 0;
    tex->mip1_pixels = NULL;
    tex->min_filter = GL_NEAREST;
    tex->mag_filter = GL_NEAREST;
    tex->wrap_s = GL_REPEAT;
    tex->wrap_t = GL_REPEAT;
    tex->allocated = 1;
}

/* Allocate a new texture, returns 1-based ID or 0 on failure */
uint32_t texture_alloc(texture_store_t *store)
{
    /* First, try to find a free slot (reuse deleted IDs) */
    for (size_t i = 0; i < store->count; i++) {
        if (!store->textures[i].allocated) {
            texture_init_slot(&store->textures[i]);
            return (uint32_t)(i + 1);  /* 1-based ID */
        }
    }

    /* No free slot, need to add a new one */
    if (store->count >= GL_MAX_TEXTURES) {
        return 0;  /* Hard limit reached */
    }

    /* Grow array if needed */
    if (store->textures == NULL) {
        store->capacity = INITIAL_TEXTURE_CAPACITY;
        store->textures = mtgl_calloc(store->capacity, sizeof(texture_t));
        if (!store->textures) return 0;
    } else if (store->count >= store->capacity) {
        size_t new_capacity = store->capacity * 2;
        if (new_capacity > GL_MAX_TEXTURES) {
            new_capacity = GL_MAX_TEXTURES;
        }
        texture_t *new_textures = mtgl_realloc(store->textures, new_capacity * sizeof(texture_t));
        if (!new_textures) return 0;  /* Keep old pointer valid on failure */
        /* Zero new entries */
        memset(&new_textures[store->capacity], 0, (new_capacity - store->capacity) * sizeof(texture_t));
        store->textures = new_textures;
        store->capacity = new_capacity;
    }

    /* Initialize new texture with defaults */
    texture_init_slot(&store->textures[store->count]);
    store->count++;
    return (uint32_t)store->count; /* 1-based ID */
}

/* Free a texture by ID */
void texture_free(texture_store_t *store, uint32_t id)
{
    if (id == 0 || id > store->count) return;

    texture_t *tex = &store->textures[id - 1];
    if (!tex->allocated) return;

    if (tex->pixels) {
        mtgl_free(tex->pixels);
        tex->pixels = NULL;
    }
    if (tex->mip1_pixels) {
        mtgl_free(tex->mip1_pixels);
        tex->mip1_pixels = NULL;
    }
    tex->width = 0;
    tex->height = 0;
    tex->mip1_width = 0;
    tex->mip1_height = 0;
    tex->allocated = 0;  /* Mark as free for reuse */
}

/* Get texture by ID (1-based), returns NULL if invalid or not allocated */
texture_t *texture_get(texture_store_t *store, uint32_t id)
{
    if (id == 0 || id > store->count) return NULL;
    texture_t *tex = &store->textures[id - 1];
    if (!tex->allocated) return NULL;
    return tex;
}

/* Invalidate mipmap when base texture changes */
static void texture_invalidate_mipmap(texture_t *tex)
{
    if (tex->mip1_pixels) {
        mtgl_free(tex->mip1_pixels);
        tex->mip1_pixels = NULL;
    }
    tex->mip1_width = 0;
    tex->mip1_height = 0;
}

/* Upload RGBA data (no conversion needed) */
int texture_upload_rgba(texture_t *tex, int32_t width, int32_t height, const uint8_t *data)
{
    if (width <= 0 || height <= 0) return -1;
    if (width > MYTGL_MAX_TEXTURE_SIZE || height > MYTGL_MAX_TEXTURE_SIZE) return -1;

    size_t pixel_count = (size_t)width * (size_t)height;

    /* Allocate new buffer - don't use realloc to avoid leak on failure */
    uint32_t *new_pixels = mtgl_alloc(pixel_count * sizeof(uint32_t));
    if (!new_pixels) return -1;

    /* Free old pixels after successful allocation */
    if (tex->pixels) {
        mtgl_free(tex->pixels);
    }

    /* Invalidate mipmap - base texture is changing */
    texture_invalidate_mipmap(tex);

    tex->pixels = new_pixels;
    tex->width = width;
    tex->height = height;

    /* Copy RGBA bytes to RGBA32 */
    for (size_t i = 0; i < pixel_count; i++) {
        tex->pixels[i] = rgba_bytes_to_rgba32(
            data[i * 4 + 0],
            data[i * 4 + 1],
            data[i * 4 + 2],
            data[i * 4 + 3]);
    }

    return 0;
}

/* Upload RGB data (convert to RGBA with alpha=255) */
int texture_upload_rgb(texture_t *tex, int32_t width, int32_t height, const uint8_t *data)
{
    if (width <= 0 || height <= 0) return -1;
    if (width > MYTGL_MAX_TEXTURE_SIZE || height > MYTGL_MAX_TEXTURE_SIZE) return -1;

    size_t pixel_count = (size_t)width * (size_t)height;

    uint32_t *new_pixels = mtgl_alloc(pixel_count * sizeof(uint32_t));
    if (!new_pixels) return -1;

    if (tex->pixels) {
        mtgl_free(tex->pixels);
    }

    /* Invalidate mipmap - base texture is changing */
    texture_invalidate_mipmap(tex);

    tex->pixels = new_pixels;
    tex->width = width;
    tex->height = height;

    /* Convert RGB to RGBA32 */
    for (size_t i = 0; i < pixel_count; i++) {
        tex->pixels[i] = rgb_bytes_to_rgba32(
            data[i * 3 + 0],
            data[i * 3 + 1],
            data[i * 3 + 2]);
    }

    return 0;
}

/* Upload luminance data (convert to RGBA: L,L,L,255) */
int texture_upload_luminance(texture_t *tex, int32_t width, int32_t height, const uint8_t *data)
{
    if (width <= 0 || height <= 0) return -1;
    if (width > MYTGL_MAX_TEXTURE_SIZE || height > MYTGL_MAX_TEXTURE_SIZE) return -1;

    size_t pixel_count = (size_t)width * (size_t)height;

    uint32_t *new_pixels = mtgl_alloc(pixel_count * sizeof(uint32_t));
    if (!new_pixels) return -1;

    if (tex->pixels) {
        mtgl_free(tex->pixels);
    }

    /* Invalidate mipmap - base texture is changing */
    texture_invalidate_mipmap(tex);

    tex->pixels = new_pixels;
    tex->width = width;
    tex->height = height;

    /* Convert Luminance to RGBA32 */
    for (size_t i = 0; i < pixel_count; i++) {
        tex->pixels[i] = luminance_to_rgba32(data[i]);
    }

    return 0;
}

/* Upload luminance+alpha data (convert to RGBA: L,L,L,A) */
int texture_upload_luminance_alpha(texture_t *tex, int32_t width, int32_t height, const uint8_t *data)
{
    if (width <= 0 || height <= 0) return -1;
    if (width > MYTGL_MAX_TEXTURE_SIZE || height > MYTGL_MAX_TEXTURE_SIZE) return -1;

    size_t pixel_count = (size_t)width * (size_t)height;

    uint32_t *new_pixels = mtgl_alloc(pixel_count * sizeof(uint32_t));
    if (!new_pixels) return -1;

    if (tex->pixels) {
        mtgl_free(tex->pixels);
    }

    /* Invalidate mipmap - base texture is changing */
    texture_invalidate_mipmap(tex);

    tex->pixels = new_pixels;
    tex->width = width;
    tex->height = height;

    /* Convert Luminance+Alpha to RGBA32 */
    for (size_t i = 0; i < pixel_count; i++) {
        tex->pixels[i] = luminance_alpha_to_rgba32(
            data[i * 2 + 0],
            data[i * 2 + 1]);
    }

    return 0;
}

/* Helper to get texel with wrapping */
static inline uint32_t get_texel_wrapped(const texture_t *tex, int32_t x, int32_t y)
{
    /* Wrap S - use proper euclidean modulo for negative coordinates */
    if (tex->wrap_s == GL_REPEAT) {
        x = ((x % tex->width) + tex->width) % tex->width;
    } else {
        if (x < 0) x = 0;
        else if (x >= tex->width) x = tex->width - 1;
    }

    /* Wrap T - use proper euclidean modulo for negative coordinates */
    if (tex->wrap_t == GL_REPEAT) {
        y = ((y % tex->height) + tex->height) % tex->height;
    } else {
        if (y < 0) y = 0;
        else if (y >= tex->height) y = tex->height - 1;
    }

    return tex->pixels[y * tex->width + x];
}

/* Bilinear interpolation of 4 texels */
static inline uint32_t bilinear_filter(uint32_t c00, uint32_t c10, uint32_t c01, uint32_t c11, float fx, float fy)
{
    color_t col00 = color_from_rgba32(c00);
    color_t col10 = color_from_rgba32(c10);
    color_t col01 = color_from_rgba32(c01);
    color_t col11 = color_from_rgba32(c11);

    /* Bilinear: lerp horizontally, then vertically */
    color_t top    = color_lerp(col00, col10, fx);
    color_t bottom = color_lerp(col01, col11, fx);
    color_t result = color_lerp(top, bottom, fy);

    return color_to_rgba32(result);
}

/* Generate mip level 1 (quarter resolution) lazily using box filter
 * Returns 0 on success, -1 on failure */
static int texture_generate_mip1(texture_t *tex)
{
    /* Already generated */
    if (tex->mip1_pixels) return 0;

    /* Need base texture */
    if (!tex->pixels || tex->width < 2 || tex->height < 2) return -1;

    tex->mip1_width = tex->width / 2;
    tex->mip1_height = tex->height / 2;

    size_t mip_size = (size_t)tex->mip1_width * (size_t)tex->mip1_height;
    tex->mip1_pixels = mtgl_alloc(mip_size * sizeof(uint32_t));
    if (!tex->mip1_pixels) {
        tex->mip1_width = 0;
        tex->mip1_height = 0;
        return -1;
    }

    /* Box filter: average 2x2 blocks */
    for (int32_t y = 0; y < tex->mip1_height; y++) {
        for (int32_t x = 0; x < tex->mip1_width; x++) {
            int32_t sx = x * 2;
            int32_t sy = y * 2;

            /* Sample 4 source texels */
            uint32_t c00 = tex->pixels[sy * tex->width + sx];
            uint32_t c10 = tex->pixels[sy * tex->width + sx + 1];
            uint32_t c01 = tex->pixels[(sy + 1) * tex->width + sx];
            uint32_t c11 = tex->pixels[(sy + 1) * tex->width + sx + 1];

            /* Average using color utilities */
            color_t col = color_from_rgba32(c00);
            col = color_add(col, color_from_rgba32(c10));
            col = color_add(col, color_from_rgba32(c01));
            col = color_add(col, color_from_rgba32(c11));
            col = color_scale(col, 0.25f);

            tex->mip1_pixels[y * tex->mip1_width + x] = color_to_rgba32(col);
        }
    }

    return 0;
}

/* Helper to get texel from mip1 with wrapping */
static inline uint32_t get_mip1_texel_wrapped(const texture_t *tex, int32_t x, int32_t y)
{
    /* Wrap S */
    if (tex->wrap_s == GL_REPEAT) {
        x = ((x % tex->mip1_width) + tex->mip1_width) % tex->mip1_width;
    } else {
        if (x < 0) x = 0;
        else if (x >= tex->mip1_width) x = tex->mip1_width - 1;
    }

    /* Wrap T */
    if (tex->wrap_t == GL_REPEAT) {
        y = ((y % tex->mip1_height) + tex->mip1_height) % tex->mip1_height;
    } else {
        if (y < 0) y = 0;
        else if (y >= tex->mip1_height) y = tex->mip1_height - 1;
    }

    return tex->mip1_pixels[y * tex->mip1_width + x];
}

/* Sample from base level (level 0) with specified filter */
static uint32_t texture_sample_base(const texture_t *tex, float u, float v, int32_t filter)
{
    float tx = u * tex->width - 0.5f;
    float ty = v * tex->height - 0.5f;

    if (filter == GL_LINEAR) {
        int32_t x0 = (int32_t)floorf(tx);
        int32_t y0 = (int32_t)floorf(ty);
        int32_t x1 = x0 + 1;
        int32_t y1 = y0 + 1;

        float fx = tx - x0;
        float fy = ty - y0;

        uint32_t c00 = get_texel_wrapped(tex, x0, y0);
        uint32_t c10 = get_texel_wrapped(tex, x1, y0);
        uint32_t c01 = get_texel_wrapped(tex, x0, y1);
        uint32_t c11 = get_texel_wrapped(tex, x1, y1);

        return bilinear_filter(c00, c10, c01, c11, fx, fy);
    } else {
        int32_t x = (int32_t)floorf(tx + 0.5f);
        int32_t y = (int32_t)floorf(ty + 0.5f);

        if (x < 0) x = 0;
        if (x >= tex->width) x = tex->width - 1;
        if (y < 0) y = 0;
        if (y >= tex->height) y = tex->height - 1;

        return tex->pixels[y * tex->width + x];
    }
}

/* Sample from mip level 1 */
static uint32_t texture_sample_mip1(texture_t *tex, float u, float v, int32_t filter)
{
    /* Generate mip1 lazily if needed */
    if (texture_generate_mip1(tex) != 0) {
        /* Fall back to base level if mip generation fails */
        return 0xFFFFFFFF;
    }

    /* Convert to texel coordinates in mip1 */
    float tx = u * tex->mip1_width - 0.5f;
    float ty = v * tex->mip1_height - 0.5f;

    if (filter == GL_LINEAR || filter == GL_LINEAR_MIPMAP_NEAREST || filter == GL_LINEAR_MIPMAP_LINEAR) {
        int32_t x0 = (int32_t)floorf(tx);
        int32_t y0 = (int32_t)floorf(ty);
        int32_t x1 = x0 + 1;
        int32_t y1 = y0 + 1;

        float fx = tx - x0;
        float fy = ty - y0;

        uint32_t c00 = get_mip1_texel_wrapped(tex, x0, y0);
        uint32_t c10 = get_mip1_texel_wrapped(tex, x1, y0);
        uint32_t c01 = get_mip1_texel_wrapped(tex, x0, y1);
        uint32_t c11 = get_mip1_texel_wrapped(tex, x1, y1);

        return bilinear_filter(c00, c10, c01, c11, fx, fy);
    } else {
        int32_t x = (int32_t)floorf(tx + 0.5f);
        int32_t y = (int32_t)floorf(ty + 0.5f);

        if (x < 0) x = 0;
        if (x >= tex->mip1_width) x = tex->mip1_width - 1;
        if (y < 0) y = 0;
        if (y >= tex->mip1_height) y = tex->mip1_height - 1;

        return tex->mip1_pixels[y * tex->mip1_width + x];
    }
}

/* Sample texture at UV coordinates with wrapping and filtering
 * lod: positive = minifying (use min_filter), zero/negative = magnifying (use mag_filter)
 * LOD ~1.0 means each screen pixel covers ~2x2 texels (use mip level 1)
 */
uint32_t texture_sample_lod(const texture_t *tex, float u, float v, float lod)
{
    if (!tex->pixels || tex->width <= 0 || tex->height <= 0) {
        return 0xFFFFFFFF; /* White if no texture */
    }

    /* Apply wrap mode for UV */
    if (tex->wrap_s == GL_REPEAT) {
        u = u - (float)(int)u;
        if (u < 0) u += 1.0f;
    } else {
        if (u < 0.0f) u = 0.0f;
        if (u > 1.0f) u = 1.0f;
    }

    if (tex->wrap_t == GL_REPEAT) {
        v = v - (float)(int)v;
        if (v < 0) v += 1.0f;
    } else {
        if (v < 0.0f) v = 0.0f;
        if (v > 1.0f) v = 1.0f;
    }

    /* Choose filter based on LOD:
     * lod > 0 means minification (texture is smaller on screen than in memory)
     * lod <= 0 means magnification (texture is larger on screen)
     */
    int32_t filter = (lod > 0.0f) ? tex->min_filter : tex->mag_filter;

    /* Standard OpenGL mipmap level selection:
     * LOD 0 = level 0 (base), LOD 1 = level 1, LOD 2 = level 2, etc.
     * We only have level 0 and level 1, so clamp LOD to [0, 1] range.
     *
     * *_MIPMAP_NEAREST: round LOD to nearest level
     * *_MIPMAP_LINEAR: blend between adjacent levels (trilinear)
     */
    if (filter == GL_NEAREST_MIPMAP_NEAREST || filter == GL_LINEAR_MIPMAP_NEAREST) {
        /* Round to nearest mip level */
        if (lod >= 0.5f) {
            return texture_sample_mip1((texture_t *)tex, u, v, filter);
        }
        /* Fall through to sample base level */
        filter = (filter == GL_NEAREST_MIPMAP_NEAREST) ? GL_NEAREST : GL_LINEAR;
    } else if (filter == GL_NEAREST_MIPMAP_LINEAR || filter == GL_LINEAR_MIPMAP_LINEAR) {
        /* Trilinear: blend between mip levels */
        if (lod > 0.0f) {
            /* Clamp LOD to our max level (1) */
            float clamped_lod = (lod > 1.0f) ? 1.0f : lod;

            /* Sample both levels */
            int32_t base_filter = (filter == GL_NEAREST_MIPMAP_LINEAR) ? GL_NEAREST : GL_LINEAR;
            uint32_t c0 = texture_sample_base((texture_t *)tex, u, v, base_filter);
            uint32_t c1 = texture_sample_mip1((texture_t *)tex, u, v, filter);

            /* Blend based on fractional LOD */
            color_t col0 = color_from_rgba32(c0);
            color_t col1 = color_from_rgba32(c1);
            color_t result = color_lerp(col0, col1, clamped_lod);
            return color_to_rgba32(result);
        }
        /* LOD <= 0: just use base level */
        filter = (filter == GL_NEAREST_MIPMAP_LINEAR) ? GL_NEAREST : GL_LINEAR;
    }

    /* Convert to texel coordinates
     * UV [0,1] maps to texel centers at [0.5, width-0.5] / width
     * So we scale by width and offset by -0.5 to get texel index */
    float tx = u * tex->width - 0.5f;
    float ty = v * tex->height - 0.5f;

    if (filter == GL_LINEAR) {
        /* Bilinear filtering */
        int32_t x0 = (int32_t)floorf(tx);
        int32_t y0 = (int32_t)floorf(ty);
        int32_t x1 = x0 + 1;
        int32_t y1 = y0 + 1;

        float fx = tx - x0;
        float fy = ty - y0;

        /* Sample 4 texels with wrapping */
        uint32_t c00 = get_texel_wrapped(tex, x0, y0);
        uint32_t c10 = get_texel_wrapped(tex, x1, y0);
        uint32_t c01 = get_texel_wrapped(tex, x0, y1);
        uint32_t c11 = get_texel_wrapped(tex, x1, y1);

        return bilinear_filter(c00, c10, c01, c11, fx, fy);
    } else {
        /* Nearest neighbor filtering - round to nearest texel */
        int32_t x = (int32_t)floorf(tx + 0.5f);
        int32_t y = (int32_t)floorf(ty + 0.5f);

        /* Clamp to valid range */
        if (x < 0) x = 0;
        if (x >= tex->width) x = tex->width - 1;
        if (y < 0) y = 0;
        if (y >= tex->height) y = tex->height - 1;

        return tex->pixels[y * tex->width + x];
    }
}

/* Backward-compatible wrapper - assumes magnification (lod=0) */
uint32_t texture_sample(const texture_t *tex, float u, float v)
{
    return texture_sample_lod(tex, u, v, 0.0f);
}

/* Sample at exact texel coordinates (no filtering) */
uint32_t texture_sample_nearest(const texture_t *tex, int32_t x, int32_t y)
{
    if (!tex->pixels || tex->width <= 0 || tex->height <= 0) {
        return 0xFFFFFFFF;
    }

    /* Wrap coordinates */
    if (tex->wrap_s == GL_REPEAT) {
        x = x % tex->width;
        if (x < 0) x += tex->width;
    } else {
        if (x < 0) x = 0;
        if (x >= tex->width) x = tex->width - 1;
    }

    if (tex->wrap_t == GL_REPEAT) {
        y = y % tex->height;
        if (y < 0) y += tex->height;
    } else {
        if (y < 0) y = 0;
        if (y >= tex->height) y = tex->height - 1;
    }

    return tex->pixels[y * tex->width + x];
}

/* Clear texture data */
void texture_clear(texture_t *tex)
{
    if (tex->pixels) {
        mtgl_free(tex->pixels);
        tex->pixels = NULL;
    }
    if (tex->mip1_pixels) {
        mtgl_free(tex->mip1_pixels);
        tex->mip1_pixels = NULL;
    }
    tex->width = 0;
    tex->height = 0;
    tex->mip1_width = 0;
    tex->mip1_height = 0;
}
