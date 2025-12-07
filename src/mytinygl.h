/*
 * MyTinyGL - OpenGL 1.x Software Renderer
 * Copyright (c) 2025 zbufferoverflow (Eliezer Solinger)
 * https://github.com/zbufferoverflow/MyTinyGL
 * SPDX-License-Identifier: MIT
 *
 * mytinygl.h - Internal state structures
 */

#ifndef MYTINYGL_INTERNAL_H
#define MYTINYGL_INTERNAL_H

#include "GL/gl.h"
#include "graphics.h"
#include "framebuffer.h"
#include "textures.h"
#include "vbo.h"
#include "lists.h"
#include <stdint.h>

#define MAX_MATRIX_STACK_DEPTH 24
#define MAX_LIGHTS 8
#define MAX_LIST_CALL_DEPTH 64

/* State flags */
#define FLAG_INSIDE_BEGIN_END  (1 << 0)
#define FLAG_DEPTH_TEST        (1 << 1)
#define FLAG_CULL_FACE         (1 << 2)
#define FLAG_BLEND             (1 << 3)
#define FLAG_TEXTURE_2D        (1 << 4)
#define FLAG_LIGHTING          (1 << 5)
#define FLAG_FOG               (1 << 6)
#define FLAG_NORMALIZE         (1 << 7)
#define FLAG_COLOR_MATERIAL    (1 << 8)
#define FLAG_ALPHA_TEST        (1 << 9)
#define FLAG_SCISSOR_TEST      (1 << 10)
#define FLAG_STENCIL_TEST      (1 << 11)

/* Client state flags */
#define CLIENT_VERTEX_ARRAY        (1 << 0)
#define CLIENT_COLOR_ARRAY         (1 << 1)
#define CLIENT_TEXTURE_COORD_ARRAY (1 << 2)
#define CLIENT_NORMAL_ARRAY        (1 << 3)

/* Vertex array pointer */
typedef struct {
    GLint size;
    GLenum type;
    GLsizei stride;
    const void *pointer;
} array_pointer_t;

/* Light source */
typedef struct {
    color_t ambient;
    color_t diffuse;
    color_t specular;
    vec4_t position;         /* Eye-space position (w=0 for directional) */
    vec3_t spot_direction;
    GLfloat spot_exponent;
    GLfloat spot_cutoff;     /* Degrees, 180 = no spotlight */
    GLfloat constant_attenuation;
    GLfloat linear_attenuation;
    GLfloat quadratic_attenuation;
    GLboolean enabled;
} light_t;

/* Material properties */
typedef struct {
    color_t ambient;
    color_t diffuse;
    color_t specular;
    color_t emission;
    GLfloat shininess;
} material_t;

/* Vertex data buffer (per-context for thread safety) */
#define INITIAL_VERTEX_CAPACITY 64

typedef struct {
    vertex_t *data;
    size_t count;
    size_t capacity;
} vertex_buffer_t;

typedef struct {
    /* Clear values */
    color_t clear_color;
    GLdouble clear_depth;

    /* Viewport */
    GLint viewport_x, viewport_y;
    GLsizei viewport_w, viewport_h;

    /* Current vertex attributes */
    color_t current_color;
    vec2_t current_texcoord;
    vec3_t current_normal;

    /* Matrix state */
    GLenum matrix_mode;
    GLfloat modelview_matrix[MAX_MATRIX_STACK_DEPTH][16];
    GLfloat projection_matrix[MAX_MATRIX_STACK_DEPTH][16];
    GLfloat texture_matrix[MAX_MATRIX_STACK_DEPTH][16];
    GLint modelview_stack_depth;
    GLint projection_stack_depth;
    GLint texture_stack_depth;

    /* Primitive assembly */
    GLenum primitive_mode;

    /* State flags */
    uint32_t flags;

    /* Blend function */
    GLenum blend_src;
    GLenum blend_dst;

    /* Culling */
    GLenum cull_face_mode;
    GLenum front_face;

    /* Depth testing */
    GLenum depth_func;
    GLboolean depth_mask;

    /* Alpha testing */
    GLenum alpha_func;
    GLfloat alpha_ref;

    /* Scissor test */
    GLint scissor_x, scissor_y;
    GLsizei scissor_w, scissor_h;

    /* Stencil test */
    GLenum stencil_func;        /* GL_ALWAYS, GL_EQUAL, etc. */
    GLint stencil_ref;          /* Reference value */
    GLuint stencil_mask;        /* Comparison mask */
    GLenum stencil_fail;        /* Op when stencil fails */
    GLenum stencil_zfail;       /* Op when stencil passes but depth fails */
    GLenum stencil_zpass;       /* Op when both pass */
    GLuint stencil_writemask;   /* Write mask */
    GLint stencil_clear;        /* Clear value */

    /* Depth range */
    GLdouble depth_near;
    GLdouble depth_far;

    /* Color mask */
    GLboolean color_mask_r;
    GLboolean color_mask_g;
    GLboolean color_mask_b;
    GLboolean color_mask_a;

    /* Line/point size */
    GLfloat line_width;
    GLfloat point_size;

    /* Polygon mode (stub - always GL_FILL) */
    GLenum polygon_mode_front;
    GLenum polygon_mode_back;

    /* Textures */
    texture_store_t textures;
    GLuint bound_texture_2d;  /* Currently bound GL_TEXTURE_2D */

    /* Texture environment */
    GLenum tex_env_mode;      /* GL_MODULATE, GL_DECAL, GL_REPLACE, GL_ADD */
    color_t tex_env_color;

    /* Hints */
    GLenum perspective_correction_hint;

    /* Raster position (window coordinates) */
    GLint raster_pos_x;
    GLint raster_pos_y;
    GLboolean raster_pos_valid;

    /* Fog */
    GLenum fog_mode;
    GLfloat fog_density;
    GLfloat fog_start;
    GLfloat fog_end;
    color_t fog_color;

    /* Lighting */
    light_t lights[MAX_LIGHTS];
    material_t material_front;
    material_t material_back;
    color_t light_model_ambient;
    GLboolean light_model_local_viewer;
    GLboolean light_model_two_side;
    GLenum color_material_face;
    GLenum color_material_mode;
    GLenum shade_model;

    /* VBO (OpenGL 1.5) */
    buffer_store_t buffers;
    GLuint bound_array_buffer;
    GLuint bound_element_buffer;

    /* Vertex arrays */
    uint32_t client_state;
    array_pointer_t vertex_pointer;
    array_pointer_t color_pointer;
    array_pointer_t texcoord_pointer;
    array_pointer_t normal_pointer;

    /* Framebuffer */
    framebuffer_t framebuffer;

    /* Display lists */
    list_store_t lists;
    GLuint list_base;           /* Base offset for glCallLists */
    GLuint list_index;          /* Currently compiling list (0 = none) */
    GLenum list_mode;           /* GL_COMPILE or GL_COMPILE_AND_EXECUTE */
    GLuint list_call_depth;     /* Recursion depth for glCallList */

    /* Vertex buffer (per-context for thread safety) */
    vertex_buffer_t vertices;

    /* Error state */
    GLenum error;

} GLState;

/* Context management */
GLState *gl_create_context(int32_t width, int32_t height);
void gl_destroy_context(GLState *ctx);
void gl_make_current(GLState *ctx);

/* Get current context */
GLState *gl_get_current_context(void);

/* Vertex buffer helpers */
void vertex_buffer_push(GLState *ctx, vertex_t v);
vertex_t *vertex_buffer_data(GLState *ctx);
size_t vertex_buffer_count(GLState *ctx);
void vertex_buffer_clear(GLState *ctx);

/* Error handling */
void gl_set_error(GLState *ctx, GLenum error);

/* Rasterization functions (raster.c) */
vec4_t transform_vertex(GLState *ctx, float x, float y, float z, float w);
void ndc_to_screen(GLState *ctx, float x, float y, int32_t *sx, int32_t *sy);
void flush_points(GLState *ctx);
void flush_lines(GLState *ctx);
void flush_line_strip(GLState *ctx);
void flush_line_loop(GLState *ctx);
void flush_triangles(GLState *ctx);
void flush_triangle_strip(GLState *ctx);
void flush_triangle_fan(GLState *ctx);
void flush_quads(GLState *ctx);
void flush_quad_strip(GLState *ctx);
void flush_polygon(GLState *ctx);

#endif /* MYTINYGL_INTERNAL_H */
