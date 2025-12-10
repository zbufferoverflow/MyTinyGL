/*
 * MyTinyGL - OpenGL 1.x Fixed Function Pipeline
 * raster.c - Rasterization functions
 */

#include "GL/gl.h"
#include "graphics.h"
#include "mytinygl.h"
#include "clipping.h"
#include "lighting.h"
#include <string.h>
#include <math.h>

/* Forward declarations for helper functions */
static inline int depth_test(GLenum func, float incoming, float stored);
static inline int alpha_test(GLenum func, float incoming, float ref);
static inline color_t blend_colors(GLState *ctx, color_t src, color_t dst);

/* Write pixel with color mask support */
static inline void write_pixel_masked(GLState *ctx, int32_t x, int32_t y, color_t c)
{
    framebuffer_t *fb = &ctx->framebuffer;

    /* If all channels enabled, fast path */
    if (ctx->color_mask_r && ctx->color_mask_g && ctx->color_mask_b && ctx->color_mask_a) {
        framebuffer_putpixel(fb, x, y, color_to_rgba32(c));
        return;
    }

    /* If all channels disabled, skip write */
    if (!ctx->color_mask_r && !ctx->color_mask_g && !ctx->color_mask_b && !ctx->color_mask_a) {
        return;
    }

    /* Partial mask: read-modify-write */
    pixel_t dst_pixel = framebuffer_getpixel(fb, x, y);
    color_t dst = color_from_rgba32(dst_pixel);

    if (ctx->color_mask_r) dst.r = c.r;
    if (ctx->color_mask_g) dst.g = c.g;
    if (ctx->color_mask_b) dst.b = c.b;
    if (ctx->color_mask_a) dst.a = c.a;

    framebuffer_putpixel(fb, x, y, color_to_rgba32(dst));
}

/* Transform vertex by modelview and projection matrices */
vec4_t transform_vertex(GLState *ctx, float x, float y, float z, float w)
{
    vec4_t v = vec4(x, y, z, w);
    mat4_t mv   = mat4_from_array(ctx->modelview_matrix[ctx->modelview_stack_depth]);
    mat4_t proj = mat4_from_array(ctx->projection_matrix[ctx->projection_stack_depth]);
    v = mat4_mul_vec4(mv, v);
    v = mat4_mul_vec4(proj, v);
    return v;
}

/* Transform vertex from NDC (-1 to 1) to screen coordinates */
void ndc_to_screen(GLState *ctx, float x, float y, int32_t *sx, int32_t *sy)
{
    *sx = (int32_t)((x + 1.0f) * 0.5f * ctx->viewport_w + ctx->viewport_x);
    *sy = (int32_t)((1.0f - y) * 0.5f * ctx->viewport_h + ctx->viewport_y);
}

/* Helper: write a single pixel for line rendering with all tests/blending */
static void write_line_pixel(GLState *ctx, int32_t px, int32_t py, float depth, color_t c,
                             int depth_enabled, int blend_enabled, int scissor_enabled)
{
    framebuffer_t *fb = &ctx->framebuffer;

    /* Bounds check */
    if (px < 0 || px >= fb->width || py < 0 || py >= fb->height) return;

    /* Scissor test */
    if (scissor_enabled) {
        if (px < ctx->scissor_x || px >= ctx->scissor_x + (int)ctx->scissor_w ||
            py < ctx->scissor_y || py >= ctx->scissor_y + (int)ctx->scissor_h) {
            return;
        }
    }

    /* Depth test */
    if (depth_enabled) {
        float stored = framebuffer_getdepth(fb, px, py);
        if (!depth_test(ctx->depth_func, depth, stored)) return;
    }

    color_t final_color = c;

    /* Alpha blending */
    if (blend_enabled) {
        pixel_t dst_pixel = framebuffer_getpixel(fb, px, py);
        color_t dst = color_from_rgba32(dst_pixel);
        final_color = blend_colors(ctx, final_color, dst);
    }

    /* Write depth */
    if (depth_enabled && ctx->depth_mask) {
        framebuffer_putdepth(fb, px, py, depth);
    }

    /* Write with color mask */
    write_pixel_masked(ctx, px, py, final_color);
}

/* Draw a line with color interpolation, texturing, alpha test, depth testing, fog, blending, and line width */
static void draw_line_full(GLState *ctx, int32_t x0, int32_t y0, float z0, int32_t x1, int32_t y1, float z1,
                           color_t c0, color_t c1, float ez0, float ez1, vec2_t uv0, vec2_t uv1)
{
    framebuffer_t *fb = &ctx->framebuffer;
    int32_t dx = x1 - x0;
    int32_t dy = y1 - y0;
    int32_t adx = dx < 0 ? -dx : dx;
    int32_t ady = dy < 0 ? -dy : dy;
    int32_t sx = dx < 0 ? -1 : 1;
    int32_t sy = dy < 0 ? -1 : 1;
    int32_t err = adx - ady;

    /* Total line length for interpolation */
    int32_t total_steps = (adx > ady) ? adx : ady;
    if (total_steps == 0) total_steps = 1;
    int32_t step = 0;

    int depth_enabled = ctx->flags & FLAG_DEPTH_TEST;
    int fog_enabled = ctx->flags & FLAG_FOG;
    int blend_enabled = ctx->flags & FLAG_BLEND;
    int texture_enabled = ctx->flags & FLAG_TEXTURE_2D;
    int alpha_test_enabled = ctx->flags & FLAG_ALPHA_TEST;
    int scissor_enabled = ctx->flags & FLAG_SCISSOR_TEST;

    /* Line width: half-width for perpendicular extension */
    int line_width = (int)(ctx->line_width + 0.5f);
    if (line_width < 1) line_width = 1;
    int half_width = line_width / 2;

    /* Determine expansion direction (perpendicular to line) */
    int expand_x, expand_y;
    if (adx > ady) {
        /* More horizontal - expand vertically */
        expand_x = 0;
        expand_y = 1;
    } else {
        /* More vertical - expand horizontally */
        expand_x = 1;
        expand_y = 0;
    }

    /* Get bound texture if texturing enabled */
    texture_t *tex = NULL;
    if (texture_enabled && ctx->bound_texture_2d != 0) {
        tex = texture_get(&ctx->textures, ctx->bound_texture_2d);
    }

    int32_t cur_x = x0, cur_y = y0;

    for (;;) {
        float t = (float)step / (float)total_steps;

        /* Interpolate depth (NDC z) and apply depth range */
        float z = z0 + t * (z1 - z0);
        float depth = (z + 1.0f) * 0.5f * (ctx->depth_far - ctx->depth_near) + ctx->depth_near;

        /* Interpolate color */
        color_t c = color_lerp(c0, c1, t);

        /* Texture sampling */
        if (tex && tex->pixels) {
            float u = uv0.x + t * (uv1.x - uv0.x);
            float v = uv0.y + t * (uv1.y - uv0.y);
            uint32_t texel = texture_sample(tex, u, v);
            color_t tex_color = color_from_rgba32(texel);

            /* Alpha test - discard pixel if test fails */
            if (alpha_test_enabled && !alpha_test(ctx->alpha_func, tex_color.a, ctx->alpha_ref)) {
                continue;
            }

            /* Modulate vertex color with texture color */
            c = color_mul(c, tex_color);
        } else if (alpha_test_enabled) {
            /* Alpha test on vertex color when no texture */
            if (!alpha_test(ctx->alpha_func, c.a, ctx->alpha_ref)) {
                continue;
            }
        }

        /* Apply fog if enabled */
        if (fog_enabled) {
            float fog_coord = -(ez0 + t * (ez1 - ez0));
            float f = 1.0f;

            switch (ctx->fog_mode) {
                case GL_LINEAR:
                    if (ctx->fog_end != ctx->fog_start) {
                        f = (ctx->fog_end - fog_coord) / (ctx->fog_end - ctx->fog_start);
                    }
                    break;
                case GL_EXP:
                    f = expf(-ctx->fog_density * fog_coord);
                    break;
                case GL_EXP2: {
                    float d = ctx->fog_density * fog_coord;
                    f = expf(-d * d);
                    break;
                }
            }

            if (f < 0.0f) f = 0.0f;
            if (f > 1.0f) f = 1.0f;
            c = color_lerp_rgb(ctx->fog_color, c, f);
        }

        /* Draw the line with width */
        if (line_width == 1) {
            /* Fast path for thin lines */
            if (cur_x >= 0 && cur_x < fb->width && cur_y >= 0 && cur_y < fb->height) {
                write_line_pixel(ctx, cur_x, cur_y, depth, c, depth_enabled, blend_enabled, scissor_enabled);
            }
        } else {
            /* Wide line: draw perpendicular pixels */
            for (int w = -half_width; w < line_width - half_width; w++) {
                int32_t px = cur_x + w * expand_x;
                int32_t py = cur_y + w * expand_y;
                write_line_pixel(ctx, px, py, depth, c, depth_enabled, blend_enabled, scissor_enabled);
            }
        }

        if (cur_x == x1 && cur_y == y1) break;

        int32_t e2 = err * 2;
        if (e2 > -ady) {
            err -= ady;
            cur_x += sx;
        }
        if (e2 < adx) {
            err += adx;
            cur_y += sy;
        }
        step++;
    }
}

/* Helper: draw a single line segment with clipping, texturing, alpha test, depth, fog, and blending */
static void draw_line_segment(GLState *ctx, vertex_t *src0, vertex_t *src1)
{
    /* Copy vertices for clipping */
    vertex_t v0 = *src0;
    vertex_t v1 = *src1;

    /* Clip line to frustum */
    if (!clip_line(&v0, &v1)) {
        return;  /* Line fully clipped */
    }

    /* Perspective divide - check for degenerate w values */
    float z0, z1;
    if (fabsf(v0.position.w) >= 1e-6f) {
        float inv_w0 = 1.0f / v0.position.w;
        v0.position.x *= inv_w0;
        v0.position.y *= inv_w0;
        z0 = v0.position.z * inv_w0;
    } else {
        v0.position.x = 0.0f;
        v0.position.y = 0.0f;
        z0 = 0.0f;
    }
    if (fabsf(v1.position.w) >= 1e-6f) {
        float inv_w1 = 1.0f / v1.position.w;
        v1.position.x *= inv_w1;
        v1.position.y *= inv_w1;
        z1 = v1.position.z * inv_w1;
    } else {
        v1.position.x = 0.0f;
        v1.position.y = 0.0f;
        z1 = 0.0f;
    }

    /* Convert to screen coordinates */
    int32_t x0, y0, x1, y1;
    ndc_to_screen(ctx, v0.position.x, v0.position.y, &x0, &y0);
    ndc_to_screen(ctx, v1.position.x, v1.position.y, &x1, &y1);

    /* Draw with texturing, alpha test, depth testing, color interpolation, fog, and blending */
    draw_line_full(ctx, x0, y0, z0, x1, y1, z1, v0.color, v1.color, v0.eye_z, v1.eye_z, v0.texcoord, v1.texcoord);
}

/* Flush GL_LINES primitive */
void flush_lines(GLState *ctx)
{
    vertex_t *verts = vertex_buffer_data(ctx);
    size_t count = vertex_buffer_count(ctx);

    for (size_t i = 0; i + 1 < count; i += 2) {
        draw_line_segment(ctx, &verts[i], &verts[i + 1]);
    }
}

/* Edge function for triangle rasterization (returns twice the signed area) */
static inline float edge_function(float ax, float ay, float bx, float by, float px, float py)
{
    return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

/* Rasterize a single triangle with flat color (first vertex) */
static void rasterize_triangle_flat(GLState *ctx,
    int32_t x0, int32_t y0,
    int32_t x1, int32_t y1,
    int32_t x2, int32_t y2,
    pixel_t color)
{
    /* Bounding box */
    int32_t minX = x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2);
    int32_t minY = y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2);
    int32_t maxX = x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2);
    int32_t maxY = y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2);

    /* Clip to viewport */
    if (minX < ctx->viewport_x) minX = ctx->viewport_x;
    if (minY < ctx->viewport_y) minY = ctx->viewport_y;
    if (maxX >= ctx->viewport_x + ctx->viewport_w) maxX = ctx->viewport_x + ctx->viewport_w - 1;
    if (maxY >= ctx->viewport_y + ctx->viewport_h) maxY = ctx->viewport_y + ctx->viewport_h - 1;

    /* Triangle area (twice) */
    float area = edge_function(x0, y0, x1, y1, x2, y2);
    if (area == 0) return; /* Degenerate triangle */

    /* Rasterize */
    for (int32_t y = minY; y <= maxY; y++) {
        for (int32_t x = minX; x <= maxX; x++) {
            float w0 = edge_function(x1, y1, x2, y2, x, y);
            float w1 = edge_function(x2, y2, x0, y0, x, y);
            float w2 = edge_function(x0, y0, x1, y1, x, y);

            /* Check if inside triangle (top-left fill rule) */
            if ((area > 0 && w0 >= 0 && w1 >= 0 && w2 >= 0) ||
                (area < 0 && w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                framebuffer_putpixel(&ctx->framebuffer, x, y, color);
            }
        }
    }
}

/* Depth test comparison - returns 1 if test passes */
static inline int depth_test(GLenum func, float incoming, float stored)
{
    switch (func) {
        case GL_NEVER:    return 0;
        case GL_LESS:     return incoming < stored;
        case GL_EQUAL:    return incoming == stored;
        case GL_LEQUAL:   return incoming <= stored;
        case GL_GREATER:  return incoming > stored;
        case GL_NOTEQUAL: return incoming != stored;
        case GL_GEQUAL:   return incoming >= stored;
        case GL_ALWAYS:   return 1;
        default:          return 1;
    }
}

/* Compute blend factor as color_t */
static inline color_t get_blend_factor(GLenum factor, color_t src, color_t dst)
{
    switch (factor) {
        case GL_ZERO:                return color(0, 0, 0, 0);
        case GL_ONE:                 return color(1, 1, 1, 1);
        case GL_SRC_COLOR:           return src;
        case GL_ONE_MINUS_SRC_COLOR: return color(1 - src.r, 1 - src.g, 1 - src.b, 1 - src.a);
        case GL_DST_COLOR:           return dst;
        case GL_ONE_MINUS_DST_COLOR: return color(1 - dst.r, 1 - dst.g, 1 - dst.b, 1 - dst.a);
        case GL_SRC_ALPHA:           return color(src.a, src.a, src.a, src.a);
        case GL_ONE_MINUS_SRC_ALPHA: return color(1 - src.a, 1 - src.a, 1 - src.a, 1 - src.a);
        case GL_DST_ALPHA:           return color(dst.a, dst.a, dst.a, dst.a);
        case GL_ONE_MINUS_DST_ALPHA: return color(1 - dst.a, 1 - dst.a, 1 - dst.a, 1 - dst.a);
        case GL_SRC_ALPHA_SATURATE: {
            float f = (src.a < (1 - dst.a)) ? src.a : (1 - dst.a);
            return color(f, f, f, 1);
        }
        default:                     return color(1, 1, 1, 1);
    }
}

/* Blend source and destination colors */
static inline color_t blend_colors(GLState *ctx, color_t src, color_t dst)
{
    color_t sf = get_blend_factor(ctx->blend_src, src, dst);
    color_t df = get_blend_factor(ctx->blend_dst, src, dst);
    return color_clamp(color_add(color_mul(src, sf), color_mul(dst, df)));
}

/* Alpha test threshold (pixels with alpha below this are discarded) */
/* Alpha test comparison - returns 1 if test passes */
static inline int alpha_test(GLenum func, float incoming, float ref)
{
    switch (func) {
        case GL_NEVER:    return 0;
        case GL_LESS:     return incoming < ref;
        case GL_EQUAL:    return incoming == ref;
        case GL_LEQUAL:   return incoming <= ref;
        case GL_GREATER:  return incoming > ref;
        case GL_NOTEQUAL: return incoming != ref;
        case GL_GEQUAL:   return incoming >= ref;
        case GL_ALWAYS:   return 1;
        default:          return 1;
    }
}

/* Stencil test comparison */
static inline int stencil_test(GLenum func, GLint ref, GLuint mask, uint8_t stencil_val)
{
    GLint masked_ref = ref & mask;
    GLint masked_val = stencil_val & mask;
    switch (func) {
        case GL_NEVER:    return 0;
        case GL_LESS:     return masked_ref < masked_val;
        case GL_EQUAL:    return masked_ref == masked_val;
        case GL_LEQUAL:   return masked_ref <= masked_val;
        case GL_GREATER:  return masked_ref > masked_val;
        case GL_NOTEQUAL: return masked_ref != masked_val;
        case GL_GEQUAL:   return masked_ref >= masked_val;
        case GL_ALWAYS:   return 1;
        default:          return 1;
    }
}

/* Apply stencil operation */
static inline uint8_t stencil_op(GLenum op, uint8_t stencil_val, GLint ref)
{
    switch (op) {
        case GL_KEEP:      return stencil_val;
        case GL_ZERO:      return 0;
        case GL_REPLACE:   return (uint8_t)(ref & 0xFF);
        case GL_INCR:      return stencil_val < 255 ? stencil_val + 1 : 255;
        case GL_INCR_WRAP: return stencil_val + 1;  /* Wraps via uint8_t overflow */
        case GL_DECR:      return stencil_val > 0 ? stencil_val - 1 : 0;
        case GL_DECR_WRAP: return stencil_val - 1;  /* Wraps via uint8_t underflow */
        case GL_INVERT:    return ~stencil_val;
        default:           return stencil_val;
    }
}

/* Write stencil value with write mask */
static inline void write_stencil_masked(framebuffer_t *fb, int32_t x, int32_t y,
                                        uint8_t new_val, GLuint writemask)
{
    uint8_t old_val = framebuffer_getstencil(fb, x, y);
    uint8_t mask8 = (uint8_t)(writemask & 0xFF);
    uint8_t result = (old_val & ~mask8) | (new_val & mask8);
    framebuffer_putstencil(fb, x, y, result);
}

/* Rasterize a single triangle with per-vertex color, texcoords, depth, fog, and perspective correction */
static void rasterize_triangle_smooth(GLState *ctx,
    int32_t x0, int32_t y0, float z0, float w0_inv, color_t c0, vec2_t uv0, float ez0,
    int32_t x1, int32_t y1, float z1, float w1_inv, color_t c1, vec2_t uv1, float ez1,
    int32_t x2, int32_t y2, float z2, float w2_inv, color_t c2, vec2_t uv2, float ez2,
    vec3_t ep0, vec3_t en0, vec3_t ep1, vec3_t en1, vec3_t ep2, vec3_t en2,
    int is_back_facing)
{
    /* Bounding box */
    int32_t minX = x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2);
    int32_t minY = y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2);
    int32_t maxX = x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2);
    int32_t maxY = y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2);

    /* Clip to viewport */
    if (minX < ctx->viewport_x) minX = ctx->viewport_x;
    if (minY < ctx->viewport_y) minY = ctx->viewport_y;
    if (maxX >= ctx->viewport_x + ctx->viewport_w) maxX = ctx->viewport_x + ctx->viewport_w - 1;
    if (maxY >= ctx->viewport_y + ctx->viewport_h) maxY = ctx->viewport_y + ctx->viewport_h - 1;

    /* Clip to scissor box if enabled */
    int scissor_enabled = ctx->flags & FLAG_SCISSOR_TEST;
    if (scissor_enabled) {
        if (minX < ctx->scissor_x) minX = ctx->scissor_x;
        if (minY < ctx->scissor_y) minY = ctx->scissor_y;
        if (maxX >= ctx->scissor_x + (int32_t)ctx->scissor_w) maxX = ctx->scissor_x + ctx->scissor_w - 1;
        if (maxY >= ctx->scissor_y + (int32_t)ctx->scissor_h) maxY = ctx->scissor_y + ctx->scissor_h - 1;
    }

    /* Early exit if clipped away */
    if (minX > maxX || minY > maxY) return;

    /* Triangle area (twice) */
    float area = edge_function(x0, y0, x1, y1, x2, y2);
    if (fabsf(area) < 0.5f) return; /* Degenerate triangle (less than half a pixel) */

    float inv_area = 1.0f / area;
    int depth_enabled = ctx->flags & FLAG_DEPTH_TEST;
    int stencil_enabled = ctx->flags & FLAG_STENCIL_TEST;
    int texture_enabled = ctx->flags & FLAG_TEXTURE_2D;

    /* Perspective correction: GL_FASTEST = affine, GL_NICEST/GL_DONT_CARE = perspective correct */
    int perspective_correct = (ctx->perspective_correction_hint != GL_FASTEST);

    /* Get bound texture if texturing enabled */
    texture_t *tex = NULL;
    if (texture_enabled && ctx->bound_texture_2d != 0) {
        tex = texture_get(&ctx->textures, ctx->bound_texture_2d);
    }

    /* Pre-compute perspective-corrected UV values (u/w, v/w) */
    float u0_w = uv0.x * w0_inv, v0_w = uv0.y * w0_inv;
    float u1_w = uv1.x * w1_inv, v1_w = uv1.y * w1_inv;
    float u2_w = uv2.x * w2_inv, v2_w = uv2.y * w2_inv;

    /* Compute approximate LOD for texture filtering.
     * LOD = log2(texels_per_pixel). LOD > 0 means minification.
     * We estimate this using the ratio of UV area to screen area. */
    float tex_lod = 0.0f;
    if (tex && tex->pixels) {
        /* Compute screen-space triangle area (already have it as 'area', but that's 2x) */
        float screen_area = fabsf(area) * 0.5f;

        /* Compute UV-space triangle area (scaled to texel space) */
        float du1 = (uv1.x - uv0.x) * tex->width;
        float dv1 = (uv1.y - uv0.y) * tex->height;
        float du2 = (uv2.x - uv0.x) * tex->width;
        float dv2 = (uv2.y - uv0.y) * tex->height;
        float texel_area = fabsf(du1 * dv2 - du2 * dv1) * 0.5f;

        /* Avoid division by zero */
        if (screen_area > 0.0f) {
            float texels_per_pixel = texel_area / screen_area;
            /* LOD = log2(texels_per_pixel) for proper mipmap level selection */
            if (texels_per_pixel > 0.0f) {
                tex_lod = log2f(texels_per_pixel) * 0.5f;  /* 0.5 because area ratio, not length ratio */
                if (tex_lod < 0.0f) tex_lod = 0.0f;  /* Clamp to 0 for magnification */
            }
        }
    }

    /* Rasterize */
    for (int32_t y = minY; y <= maxY; y++) {
        for (int32_t x = minX; x <= maxX; x++) {
            float e0 = edge_function(x1, y1, x2, y2, x, y);
            float e1 = edge_function(x2, y2, x0, y0, x, y);
            float e2 = edge_function(x0, y0, x1, y1, x, y);

            /* Check if inside triangle */
            if ((area > 0 && e0 >= 0 && e1 >= 0 && e2 >= 0) ||
                (area < 0 && e0 <= 0 && e1 <= 0 && e2 <= 0)) {
                /* Barycentric coordinates */
                float b0 = e0 * inv_area;
                float b1 = e1 * inv_area;
                float b2 = e2 * inv_area;

                /* Interpolate depth (NDC z is in [-1, 1], map to depth range) */
                float z = b0 * z0 + b1 * z1 + b2 * z2;
                float depth = (z + 1.0f) * 0.5f * (ctx->depth_far - ctx->depth_near) + ctx->depth_near;

                /* Stencil test (if enabled) */
                uint8_t stencil_val = 0;
                if (stencil_enabled) {
                    stencil_val = framebuffer_getstencil(&ctx->framebuffer, x, y);
                    if (!stencil_test(ctx->stencil_func, ctx->stencil_ref, ctx->stencil_mask, stencil_val)) {
                        /* Stencil test failed - apply stencil_fail op and skip pixel */
                        uint8_t new_stencil = stencil_op(ctx->stencil_fail, stencil_val, ctx->stencil_ref);
                        write_stencil_masked(&ctx->framebuffer, x, y, new_stencil, ctx->stencil_writemask);
                        continue;
                    }
                }

                /* Depth test (only if GL_DEPTH_TEST enabled) */
                if (depth_enabled) {
                    float stored_depth = framebuffer_getdepth(&ctx->framebuffer, x, y);
                    if (!depth_test(ctx->depth_func, depth, stored_depth)) {
                        /* Depth test failed - apply stencil_zfail op if stencil enabled */
                        if (stencil_enabled) {
                            uint8_t new_stencil = stencil_op(ctx->stencil_zfail, stencil_val, ctx->stencil_ref);
                            write_stencil_masked(&ctx->framebuffer, x, y, new_stencil, ctx->stencil_writemask);
                        }
                        continue;
                    }
                }

                /* Both stencil and depth passed - apply stencil_zpass op if stencil enabled */
                if (stencil_enabled) {
                    uint8_t new_stencil = stencil_op(ctx->stencil_zpass, stencil_val, ctx->stencil_ref);
                    write_stencil_masked(&ctx->framebuffer, x, y, new_stencil, ctx->stencil_writemask);
                }

                /* Interpolate or use flat color */
                color_t c;
                if (ctx->shade_model == GL_FLAT) {
                    /* Flat shading: use last vertex color (provoking vertex per OpenGL spec) */
                    c = c2;
                } else {
                    /* Smooth/Phong: interpolate colors */
                    c = color_bary(c0, c1, c2, b0, b1, b2);
                }

                /* Per-fragment lighting for GL_PHONG, or two-sided lighting adjustment for Gouraud */
                if (ctx->flags & FLAG_LIGHTING) {
                    if (ctx->shade_model == GL_PHONG) {
                        /* Phong shading: compute full lighting per-fragment */
                        vec3_t eye_pos    = vec3_bary(ep0, ep1, ep2, b0, b1, b2);
                        vec3_t eye_normal = vec3_bary(en0, en1, en2, b0, b1, b2);

                        /* For two-sided lighting, flip normal and use back material for back faces */
                        material_t *mat = &ctx->material_front;
                        if (is_back_facing && ctx->light_model_two_side) {
                            eye_normal = vec3_scale(eye_normal, -1.0f);
                            mat = &ctx->material_back;
                        }

                        /* Compute per-fragment lighting */
                        c = compute_lighting(ctx, eye_pos, eye_normal, mat);
                    } else if (is_back_facing && ctx->light_model_two_side) {
                        /* For Gouraud shading with two-sided lighting on back faces,
                         * recompute lighting with flipped normal and back material */
                        vec3_t eye_pos    = vec3_bary(ep0, ep1, ep2, b0, b1, b2);
                        vec3_t eye_normal = vec3_bary(en0, en1, en2, b0, b1, b2);
                        eye_normal = vec3_scale(eye_normal, -1.0f);
                        c = compute_lighting(ctx, eye_pos, eye_normal, &ctx->material_back);
                    }
                }

                /* Texture sampling */
                if (tex && tex->pixels) {
                    float u, v;

                    if (perspective_correct) {
                        /* Perspective-correct interpolation */
                        float u_over_w = b0 * u0_w + b1 * u1_w + b2 * u2_w;
                        float v_over_w = b0 * v0_w + b1 * v1_w + b2 * v2_w;
                        float one_over_w = b0 * w0_inv + b1 * w1_inv + b2 * w2_inv;
                        float w = 1.0f / one_over_w;
                        u = u_over_w * w;
                        v = v_over_w * w;
                    } else {
                        /* Affine (fast) interpolation */
                        u = b0 * uv0.x + b1 * uv1.x + b2 * uv2.x;
                        v = b0 * uv0.y + b1 * uv1.y + b2 * uv2.y;
                    }

                    /* Sample texture with LOD-based filter selection */
                    uint32_t texel = texture_sample_lod(tex, u, v, tex_lod);
                    color_t tex_color = color_from_rgba32(texel);

                    /* Alpha test - discard pixel if test fails */
                    if ((ctx->flags & FLAG_ALPHA_TEST) &&
                        !alpha_test(ctx->alpha_func, tex_color.a, ctx->alpha_ref)) {
                        continue;
                    }

                    /* Apply texture environment mode */
                    switch (ctx->tex_env_mode) {
                        case GL_REPLACE:
                            /* Replace fragment color with texture color */
                            c = tex_color;
                            break;
                        case GL_DECAL:
                            /* Blend based on texture alpha (RGB only, keep fragment alpha) */
                            c = color_lerp_rgb(c, tex_color, tex_color.a);
                            break;
                        case GL_BLEND:
                            /* Blend with texture environment color per channel */
                            c = color_blend_per_channel(c, tex_color, ctx->tex_env_color);
                            break;
                        case GL_ADD:
                            /* Add texture color to fragment color (clamped later) */
                            c = color_add_rgb_mul_a(c, tex_color);
                            break;
                        case GL_MODULATE:
                        default:
                            /* Modulate (multiply) vertex color with texture color */
                            c = color_mul(c, tex_color);
                            break;
                    }
                }

                /* Apply fog if enabled */
                if (ctx->flags & FLAG_FOG) {
                    /* Interpolate eye-space z for fog */
                    float fog_coord = b0 * ez0 + b1 * ez1 + b2 * ez2;
                    float f;  /* Fog factor: 1 = no fog, 0 = full fog */

                    switch (ctx->fog_mode) {
                        case GL_LINEAR:
                            if (ctx->fog_end != ctx->fog_start) {
                                f = (ctx->fog_end - fog_coord) / (ctx->fog_end - ctx->fog_start);
                            } else {
                                f = 1.0f;
                            }
                            break;
                        case GL_EXP:
                            f = expf(-ctx->fog_density * fog_coord);
                            break;
                        case GL_EXP2:
                            {
                                float d = ctx->fog_density * fog_coord;
                                f = expf(-d * d);
                            }
                            break;
                        default:
                            f = 1.0f;
                            break;
                    }

                    /* Clamp fog factor to [0, 1] */
                    if (f < 0.0f) f = 0.0f;
                    if (f > 1.0f) f = 1.0f;

                    /* Blend fragment color with fog color */
                    c = color_lerp_rgb(ctx->fog_color, c, f);
                }

                /* Write depth after alpha test (only if depth test enabled and passed) */
                if (depth_enabled && ctx->depth_mask) {
                    framebuffer_putdepth(&ctx->framebuffer, x, y, depth);
                }

                /* Alpha blending */
                if (ctx->flags & FLAG_BLEND) {
                    pixel_t dst_pixel = framebuffer_getpixel(&ctx->framebuffer, x, y);
                    color_t dst = color_from_rgba32(dst_pixel);
                    c = blend_colors(ctx, c, dst);
                }

                /* Clamp final color and write with color mask */
                c = color_clamp(c);
                write_pixel_masked(ctx, x, y, c);
            }
        }
    }
}

/* Perspective divide: clip space to NDC, stores 1/w in position.w
 * Returns 0 on success, -1 if w is too small (degenerate vertex) */
static int perspective_divide(vertex_t *v)
{
    /* Check for w too close to zero to avoid numerical instability */
    if (fabsf(v->position.w) < 1e-6f) {
        /* Degenerate vertex - set to origin with w=1 */
        v->position.x = 0.0f;
        v->position.y = 0.0f;
        v->position.z = 0.0f;
        v->position.w = 1.0f;
        return -1;
    }
    float inv_w = 1.0f / v->position.w;
    v->position.x *= inv_w;
    v->position.y *= inv_w;
    v->position.z *= inv_w;
    v->position.w = inv_w;  /* Store 1/w for perspective-correct interpolation */
    return 0;
}

/* Check if triangle should be culled based on winding order
 * Returns 1 if triangle should be culled, 0 otherwise
 */
static int should_cull(GLState *ctx, float signed_area)
{
    if (!(ctx->flags & FLAG_CULL_FACE)) return 0;

    /* In screen space with Y pointing down (flipped from NDC):
     * The Y flip reverses winding, so:
     * negative area = CCW in original NDC
     * positive area = CW in original NDC
     */
    int is_front;
    if (ctx->front_face == GL_CCW) {
        is_front = (signed_area < 0);
    } else {
        is_front = (signed_area > 0);
    }

    if (ctx->cull_face_mode == GL_FRONT) {
        return is_front;
    } else if (ctx->cull_face_mode == GL_BACK) {
        return !is_front;
    } else { /* GL_FRONT_AND_BACK */
        return 1;
    }
}

/* Draw a single point at screen coordinates with depth testing, blending etc */
static void draw_point_at_screen(GLState *ctx, int32_t x, int32_t y, float z, color_t c, float eye_z)
{
    framebuffer_t *fb = &ctx->framebuffer;

    /* Bounds check */
    if (x < 0 || x >= fb->width || y < 0 || y >= fb->height) return;

    /* Scissor test */
    if (ctx->flags & FLAG_SCISSOR_TEST) {
        if (x < ctx->scissor_x || x >= ctx->scissor_x + (int)ctx->scissor_w ||
            y < ctx->scissor_y || y >= ctx->scissor_y + (int)ctx->scissor_h) {
            return;
        }
    }

    /* Map depth from NDC [-1,1] to [depth_near, depth_far] */
    float depth = (z + 1.0f) * 0.5f * (ctx->depth_far - ctx->depth_near) + ctx->depth_near;

    /* Depth test */
    int depth_enabled = ctx->flags & FLAG_DEPTH_TEST;
    if (depth_enabled) {
        float stored = framebuffer_getdepth(fb, x, y);
        if (!depth_test(ctx->depth_func, depth, stored)) return;
    }

    /* Alpha test */
    if (ctx->flags & FLAG_ALPHA_TEST) {
        if (!alpha_test(ctx->alpha_func, c.a, ctx->alpha_ref)) return;
    }

    /* Fog */
    if (ctx->flags & FLAG_FOG) {
        float fog_coord = -eye_z;  /* Use negated eye-space Z */
        float f = 1.0f;
        switch (ctx->fog_mode) {
            case GL_LINEAR:
                if (ctx->fog_end != ctx->fog_start) {
                    f = (ctx->fog_end - fog_coord) / (ctx->fog_end - ctx->fog_start);
                }
                break;
            case GL_EXP:
                f = expf(-ctx->fog_density * fog_coord);
                break;
            case GL_EXP2: {
                float d = ctx->fog_density * fog_coord;
                f = expf(-d * d);
                break;
            }
        }
        if (f < 0.0f) f = 0.0f;
        if (f > 1.0f) f = 1.0f;
        c = color_lerp_rgb(ctx->fog_color, c, f);
    }

    /* Write depth */
    if (depth_enabled && ctx->depth_mask) {
        framebuffer_putdepth(fb, x, y, depth);
    }

    /* Alpha blending */
    if (ctx->flags & FLAG_BLEND) {
        pixel_t dst_pixel = framebuffer_getpixel(fb, x, y);
        color_t dst = color_from_rgba32(dst_pixel);
        c = blend_colors(ctx, c, dst);
    }

    write_pixel_masked(ctx, x, y, c);
}

/* Draw a triangle as wireframe (3 edges) */
static void draw_triangle_wireframe(GLState *ctx, vertex_t *c0, vertex_t *c1, vertex_t *c2)
{
    int32_t x0, y0, x1, y1, x2, y2;
    ndc_to_screen(ctx, c0->position.x, c0->position.y, &x0, &y0);
    ndc_to_screen(ctx, c1->position.x, c1->position.y, &x1, &y1);
    ndc_to_screen(ctx, c2->position.x, c2->position.y, &x2, &y2);

    /* NDC z values (already divided by w) */
    float z0 = c0->position.z;
    float z1 = c1->position.z;
    float z2 = c2->position.z;

    /* Draw the 3 edges */
    draw_line_full(ctx, x0, y0, z0, x1, y1, z1, c0->color, c1->color, c0->eye_z, c1->eye_z, c0->texcoord, c1->texcoord);
    draw_line_full(ctx, x1, y1, z1, x2, y2, z2, c1->color, c2->color, c1->eye_z, c2->eye_z, c1->texcoord, c2->texcoord);
    draw_line_full(ctx, x2, y2, z2, x0, y0, z0, c2->color, c0->color, c2->eye_z, c0->eye_z, c2->texcoord, c0->texcoord);
}

/* Draw triangle vertices as points */
static void draw_triangle_points(GLState *ctx, vertex_t *c0, vertex_t *c1, vertex_t *c2)
{
    int32_t x0, y0, x1, y1, x2, y2;
    ndc_to_screen(ctx, c0->position.x, c0->position.y, &x0, &y0);
    ndc_to_screen(ctx, c1->position.x, c1->position.y, &x1, &y1);
    ndc_to_screen(ctx, c2->position.x, c2->position.y, &x2, &y2);

    draw_point_at_screen(ctx, x0, y0, c0->position.z, c0->color, c0->eye_z);
    draw_point_at_screen(ctx, x1, y1, c1->position.z, c1->color, c1->eye_z);
    draw_point_at_screen(ctx, x2, y2, c2->position.z, c2->color, c2->eye_z);
}

/* Render a single triangle with clipping and rasterization */
static void render_triangle(GLState *ctx, vertex_t *v0, vertex_t *v1, vertex_t *v2)
{
    vertex_t triangle[3] = { *v0, *v1, *v2 };
    vertex_t clipped[MAX_CLIP_VERTS];

    /* Clip triangle against frustum (in clip space, before perspective divide) */
    int clip_count = clip_triangle(triangle, clipped);
    if (clip_count < 3) return;

    /* Perspective divide for all clipped vertices */
    for (int j = 0; j < clip_count; j++) {
        perspective_divide(&clipped[j]);
    }

    /* Triangulate the clipped polygon (fan from first vertex) */
    for (int j = 1; j + 1 < clip_count; j++) {
        int32_t x0, y0, x1, y1, x2, y2;
        ndc_to_screen(ctx, clipped[0].position.x, clipped[0].position.y, &x0, &y0);
        ndc_to_screen(ctx, clipped[j].position.x, clipped[j].position.y, &x1, &y1);
        ndc_to_screen(ctx, clipped[j+1].position.x, clipped[j+1].position.y, &x2, &y2);

        /* Backface culling - compute signed area in screen space */
        float signed_area = (float)(x1 - x0) * (float)(y2 - y0)
                          - (float)(x2 - x0) * (float)(y1 - y0);
        if (should_cull(ctx, signed_area)) continue;

        /* Determine if this is a back-facing triangle for two-sided lighting.
         * In screen space with Y pointing down (flipped from NDC):
         * negative area = CCW in original NDC, positive area = CW in original NDC */
        int is_back_facing;
        if (ctx->front_face == GL_CCW) {
            is_back_facing = (signed_area >= 0);  /* CW in NDC = back facing */
        } else {
            is_back_facing = (signed_area < 0);   /* CCW in NDC = back facing when front is CW */
        }

        /* Get polygon mode for this face */
        GLenum poly_mode = is_back_facing ? ctx->polygon_mode_back : ctx->polygon_mode_front;

        if (poly_mode == GL_POINT) {
            /* Draw triangle vertices as points */
            draw_triangle_points(ctx, &clipped[0], &clipped[j], &clipped[j+1]);
        } else if (poly_mode == GL_LINE) {
            /* Draw triangle edges as lines */
            draw_triangle_wireframe(ctx, &clipped[0], &clipped[j], &clipped[j+1]);
        } else {
            /* GL_FILL - Use smooth shading (Gouraud) with depth, textures, fog, and perspective correction */
            rasterize_triangle_smooth(ctx,
                x0, y0, clipped[0].position.z, clipped[0].position.w, clipped[0].color, clipped[0].texcoord, clipped[0].eye_z,
                x1, y1, clipped[j].position.z, clipped[j].position.w, clipped[j].color, clipped[j].texcoord, clipped[j].eye_z,
                x2, y2, clipped[j+1].position.z, clipped[j+1].position.w, clipped[j+1].color, clipped[j+1].texcoord, clipped[j+1].eye_z,
                clipped[0].eye_pos, clipped[0].eye_normal,
                clipped[j].eye_pos, clipped[j].eye_normal,
                clipped[j+1].eye_pos, clipped[j+1].eye_normal,
                is_back_facing);
        }
    }
}

/* Flush GL_TRIANGLES primitive */
void flush_triangles(GLState *ctx)
{
    vertex_t *verts = vertex_buffer_data(ctx);
    size_t count = vertex_buffer_count(ctx);

    for (size_t i = 0; i + 2 < count; i += 3) {
        render_triangle(ctx, &verts[i], &verts[i+1], &verts[i+2]);
    }
}

/* Flush GL_QUADS primitive (each quad split into 2 triangles) */
void flush_quads(GLState *ctx)
{
    vertex_t *verts = vertex_buffer_data(ctx);
    size_t count = vertex_buffer_count(ctx);

    for (size_t i = 0; i + 3 < count; i += 4) {
        /* Quad vertices: 0, 1, 2, 3
         * Triangle 1: 0, 1, 2
         * Triangle 2: 0, 2, 3
         */
        render_triangle(ctx, &verts[i], &verts[i+1], &verts[i+2]);
        render_triangle(ctx, &verts[i], &verts[i+2], &verts[i+3]);
    }
}

/* Flush GL_TRIANGLE_STRIP primitive */
void flush_triangle_strip(GLState *ctx)
{
    vertex_t *verts = vertex_buffer_data(ctx);
    size_t count = vertex_buffer_count(ctx);

    if (count < 3) return;

    for (size_t i = 0; i + 2 < count; i++) {
        /* Alternate winding order for each triangle */
        if (i % 2 == 0) {
            render_triangle(ctx, &verts[i], &verts[i+1], &verts[i+2]);
        } else {
            render_triangle(ctx, &verts[i+1], &verts[i], &verts[i+2]);
        }
    }
}

/* Flush GL_TRIANGLE_FAN primitive */
void flush_triangle_fan(GLState *ctx)
{
    vertex_t *verts = vertex_buffer_data(ctx);
    size_t count = vertex_buffer_count(ctx);

    if (count < 3) return;

    /* First vertex is the center, fan out from there */
    for (size_t i = 1; i + 1 < count; i++) {
        render_triangle(ctx, &verts[0], &verts[i], &verts[i+1]);
    }
}

/* Flush GL_POINTS primitive with support for point_size */
void flush_points(GLState *ctx)
{
    vertex_t *verts = vertex_buffer_data(ctx);
    size_t count = vertex_buffer_count(ctx);
    framebuffer_t *fb = &ctx->framebuffer;

    int depth_enabled = ctx->flags & FLAG_DEPTH_TEST;
    int fog_enabled = ctx->flags & FLAG_FOG;
    int blend_enabled = ctx->flags & FLAG_BLEND;
    int texture_enabled = ctx->flags & FLAG_TEXTURE_2D;
    int alpha_test_enabled = ctx->flags & FLAG_ALPHA_TEST;
    int scissor_enabled = ctx->flags & FLAG_SCISSOR_TEST;

    /* Point size: half-size for extent calculation */
    int point_size = (int)(ctx->point_size + 0.5f);
    if (point_size < 1) point_size = 1;
    int half = point_size / 2;

    /* Get bound texture if texturing enabled */
    texture_t *tex = NULL;
    if (texture_enabled && ctx->bound_texture_2d != 0) {
        tex = texture_get(&ctx->textures, ctx->bound_texture_2d);
    }

    for (size_t i = 0; i < count; i++) {
        vec4_t pos = verts[i].position;

        /* Frustum clipping - check if point is inside clip volume */
        if (pos.x < -pos.w || pos.x > pos.w ||
            pos.y < -pos.w || pos.y > pos.w ||
            pos.z < -pos.w || pos.z > pos.w ||
            pos.w <= 0.0f) {
            continue;  /* Point clipped */
        }

        /* Perspective divide */
        float ndc_x = pos.x / pos.w;
        float ndc_y = pos.y / pos.w;
        float ndc_z = pos.z / pos.w;

        /* Convert to screen coordinates */
        int32_t cx, cy;
        ndc_to_screen(ctx, ndc_x, ndc_y, &cx, &cy);

        /* Depth value for all pixels in this point */
        float depth = (ndc_z + 1.0f) * 0.5f * (ctx->depth_far - ctx->depth_near) + ctx->depth_near;

        /* Start with vertex color */
        color_t c = verts[i].color;

        /* Texture sampling */
        if (tex && tex->pixels) {
            float u = verts[i].texcoord.x;
            float v = verts[i].texcoord.y;
            uint32_t texel = texture_sample(tex, u, v);
            color_t tex_color = color_from_rgba32(texel);

            /* Alpha test - discard point if test fails */
            if (alpha_test_enabled && !alpha_test(ctx->alpha_func, tex_color.a, ctx->alpha_ref)) {
                continue;
            }

            /* Modulate vertex color with texture color */
            c = color_mul(c, tex_color);
        } else if (alpha_test_enabled) {
            /* Alpha test on vertex color when no texture */
            if (!alpha_test(ctx->alpha_func, c.a, ctx->alpha_ref)) {
                continue;
            }
        }

        /* Apply fog if enabled */
        if (fog_enabled) {
            float fog_coord = -verts[i].eye_z;
            float f = 1.0f;

            switch (ctx->fog_mode) {
                case GL_LINEAR:
                    if (ctx->fog_end != ctx->fog_start) {
                        f = (ctx->fog_end - fog_coord) / (ctx->fog_end - ctx->fog_start);
                    }
                    break;
                case GL_EXP:
                    f = expf(-ctx->fog_density * fog_coord);
                    break;
                case GL_EXP2: {
                    float d = ctx->fog_density * fog_coord;
                    f = expf(-d * d);
                    break;
                }
            }

            if (f < 0.0f) f = 0.0f;
            if (f > 1.0f) f = 1.0f;
            c = color_lerp_rgb(ctx->fog_color, c, f);
        }

        /* Draw point as a square of size point_size centered at (cx, cy) */
        int x_start = cx - half;
        int x_end = x_start + point_size;
        int y_start = cy - half;
        int y_end = y_start + point_size;

        for (int py = y_start; py < y_end; py++) {
            for (int px = x_start; px < x_end; px++) {
                /* Viewport bounds check */
                if (px < 0 || px >= fb->width || py < 0 || py >= fb->height) {
                    continue;
                }

                /* Scissor test */
                if (scissor_enabled) {
                    if (px < ctx->scissor_x || px >= ctx->scissor_x + (int)ctx->scissor_w ||
                        py < ctx->scissor_y || py >= ctx->scissor_y + (int)ctx->scissor_h) {
                        continue;
                    }
                }

                /* Depth test */
                if (depth_enabled) {
                    float stored = framebuffer_getdepth(fb, px, py);
                    if (!depth_test(ctx->depth_func, depth, stored)) {
                        continue;
                    }
                }

                color_t final_color = c;

                /* Alpha blending */
                if (blend_enabled) {
                    pixel_t dst_pixel = framebuffer_getpixel(fb, px, py);
                    color_t dst = color_from_rgba32(dst_pixel);
                    final_color = blend_colors(ctx, final_color, dst);
                }

                /* Write depth */
                if (depth_enabled && ctx->depth_mask) {
                    framebuffer_putdepth(fb, px, py, depth);
                }

                write_pixel_masked(ctx, px, py, final_color);
            }
        }
    }
}

/* Flush GL_LINE_STRIP primitive */
void flush_line_strip(GLState *ctx)
{
    vertex_t *verts = vertex_buffer_data(ctx);
    size_t count = vertex_buffer_count(ctx);

    if (count < 2) return;

    for (size_t i = 0; i + 1 < count; i++) {
        draw_line_segment(ctx, &verts[i], &verts[i + 1]);
    }
}

/* Flush GL_LINE_LOOP primitive */
void flush_line_loop(GLState *ctx)
{
    vertex_t *verts = vertex_buffer_data(ctx);
    size_t count = vertex_buffer_count(ctx);

    if (count < 2) return;

    /* Draw all segments like line strip */
    for (size_t i = 0; i + 1 < count; i++) {
        draw_line_segment(ctx, &verts[i], &verts[i + 1]);
    }

    /* Close the loop: connect last vertex to first */
    if (count >= 2) {
        draw_line_segment(ctx, &verts[count - 1], &verts[0]);
    }
}

/* Flush GL_POLYGON primitive (same as triangle fan) */
void flush_polygon(GLState *ctx)
{
    vertex_t *verts = vertex_buffer_data(ctx);
    size_t count = vertex_buffer_count(ctx);

    if (count < 3) return;

    /* Triangulate as fan from first vertex */
    for (size_t i = 1; i + 1 < count; i++) {
        render_triangle(ctx, &verts[0], &verts[i], &verts[i+1]);
    }
}

/* Flush GL_QUAD_STRIP primitive */
void flush_quad_strip(GLState *ctx)
{
    vertex_t *verts = vertex_buffer_data(ctx);
    size_t count = vertex_buffer_count(ctx);

    if (count < 4) return;

    /* Each pair of vertices with the next pair forms a quad
     * Vertices: 0,1,2,3,4,5,...
     * Quad 1: 0,1,3,2 (note the order for correct winding)
     * Quad 2: 2,3,5,4
     * etc.
     */
    for (size_t i = 0; i + 3 < count; i += 2) {
        /* Split quad into two triangles with correct winding */
        render_triangle(ctx, &verts[i], &verts[i+1], &verts[i+3]);
        render_triangle(ctx, &verts[i], &verts[i+3], &verts[i+2]);
    }
}
