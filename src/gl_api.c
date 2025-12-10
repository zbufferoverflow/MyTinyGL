/*
 * MyTinyGL - OpenGL 1.x Fixed Function Pipeline
 * gl_api.c - OpenGL API function implementations
 */

#include "GL/gl.h"
#include "graphics.h"
#include "mytinygl.h"
#include "lighting.h"
#include "allocation.h"
#include <string.h>
#include <math.h>

/* Thread-local context pointer for multi-threaded safety */
#if defined(_MSC_VER)
    #define THREAD_LOCAL __declspec(thread)
#elif defined(__GNUC__) || defined(__clang__)
    #define THREAD_LOCAL __thread
#else
    #define THREAD_LOCAL  /* Fallback: not thread-safe */
#endif

static THREAD_LOCAL GLState *ctx = NULL;

/* Macro for early return if no context is set */
#define CHECK_CTX() do { if (!ctx) return; } while(0)
#define CHECK_CTX_RET(val) do { if (!ctx) return (val); } while(0)

/* Error handling - only set if no error already recorded */
void gl_set_error(GLState *c, GLenum error)
{
    if (c && c->error == GL_NO_ERROR) {
        c->error = error;
    }
}

/* Vertex buffer helpers (per-context for thread safety) */
void vertex_buffer_push(GLState *c, vertex_t v)
{
    if (!c) return;

    vertex_buffer_t *vb = &c->vertices;

    if (vb->data == NULL) {
        vb->capacity = INITIAL_VERTEX_CAPACITY;
        vb->data = mtgl_alloc(vb->capacity * sizeof(vertex_t));
        if (!vb->data) {
            gl_set_error(c, GL_OUT_OF_MEMORY);
            return;
        }
    } else if (vb->count >= vb->capacity) {
        size_t new_capacity = vb->capacity * 2;
        vertex_t *new_data = mtgl_realloc(vb->data, new_capacity * sizeof(vertex_t));
        if (!new_data) {
            gl_set_error(c, GL_OUT_OF_MEMORY);
            return;
        }
        vb->data = new_data;
        vb->capacity = new_capacity;
    }
    vb->data[vb->count++] = v;
}

vertex_t *vertex_buffer_data(GLState *c)
{
    return c ? c->vertices.data : NULL;
}

size_t vertex_buffer_count(GLState *c)
{
    return c ? c->vertices.count : 0;
}

void vertex_buffer_clear(GLState *c)
{
    if (c) c->vertices.count = 0;
}

/* Context management */

GLState *gl_create_context(int32_t width, int32_t height)
{
    GLState *c = mtgl_alloc(sizeof(GLState));
    if (!c) return NULL;

    /* Init framebuffer */
    if (framebuffer_init(&c->framebuffer, width, height) < 0) {
        mtgl_free(c);
        return NULL;
    }

    c->clear_color = color(0.0f, 0.0f, 0.0f, 1.0f);
    c->clear_depth = 1.0;

    c->viewport_x = 0;
    c->viewport_y = 0;
    c->viewport_w = width;
    c->viewport_h = height;

    c->current_color = color(1.0f, 1.0f, 1.0f, 1.0f);
    c->current_texcoord = vec2(0.0f, 0.0f);
    c->current_normal = vec3(0.0f, 0.0f, 1.0f);

    c->matrix_mode = GL_MODELVIEW;
    c->modelview_stack_depth = 0;
    c->projection_stack_depth = 0;
    c->texture_stack_depth = 0;

    /* Initialize all matrix stack levels to identity */
    for (int i = 0; i < MAX_MATRIX_STACK_DEPTH; i++) {
        mat4_to_array(mat4_identity(), c->modelview_matrix[i]);
        mat4_to_array(mat4_identity(), c->projection_matrix[i]);
        mat4_to_array(mat4_identity(), c->texture_matrix[i]);
    }

    c->primitive_mode = 0;
    c->flags = 0;

    c->blend_src = GL_ONE;
    c->blend_dst = GL_ZERO;
    c->cull_face_mode = GL_BACK;
    c->front_face = GL_CCW;
    c->depth_func = GL_LESS;
    c->depth_mask = GL_TRUE;

    /* Alpha test - OpenGL defaults */
    c->alpha_func = GL_ALWAYS;
    c->alpha_ref = 0.0f;

    /* Scissor test - default to full viewport */
    c->scissor_x = 0;
    c->scissor_y = 0;
    c->scissor_w = width;
    c->scissor_h = height;

    /* Stencil test - OpenGL defaults */
    c->stencil_func = GL_ALWAYS;
    c->stencil_ref = 0;
    c->stencil_mask = 0xFFFFFFFF;
    c->stencil_fail = GL_KEEP;
    c->stencil_zfail = GL_KEEP;
    c->stencil_zpass = GL_KEEP;
    c->stencil_writemask = 0xFFFFFFFF;
    c->stencil_clear = 0;

    /* Depth range - default to [0, 1] */
    c->depth_near = 0.0;
    c->depth_far = 1.0;

    /* Color mask - default to all enabled */
    c->color_mask_r = GL_TRUE;
    c->color_mask_g = GL_TRUE;
    c->color_mask_b = GL_TRUE;
    c->color_mask_a = GL_TRUE;

    /* Line/point size - default to 1 */
    c->line_width = 1.0f;
    c->point_size = 1.0f;

    /* Polygon mode - default to filled */
    c->polygon_mode_front = GL_FILL;
    c->polygon_mode_back = GL_FILL;

    /* Init texture store */
    texture_store_init(&c->textures);
    c->bound_texture_2d = 0;

    /* Texture environment - default to GL_MODULATE */
    c->tex_env_mode = GL_MODULATE;
    c->tex_env_color = color(0.0f, 0.0f, 0.0f, 0.0f);

    /* Hints - default to nicest for quality */
    c->perspective_correction_hint = GL_DONT_CARE;

    /* Raster position - default to origin, valid */
    c->raster_pos_x = 0;
    c->raster_pos_y = 0;
    c->raster_pos_valid = GL_TRUE;

    /* Fog - OpenGL defaults */
    c->fog_mode = GL_EXP;
    c->fog_density = 1.0f;
    c->fog_start = 0.0f;
    c->fog_end = 1.0f;
    c->fog_color = color(0.0f, 0.0f, 0.0f, 0.0f);

    /* VBO - OpenGL 1.5 */
    buffer_store_init(&c->buffers);
    c->bound_array_buffer = 0;
    c->bound_element_buffer = 0;

    /* Vertex arrays */
    c->client_state = 0;
    memset(&c->vertex_pointer, 0, sizeof(array_pointer_t));
    memset(&c->color_pointer, 0, sizeof(array_pointer_t));
    memset(&c->texcoord_pointer, 0, sizeof(array_pointer_t));
    memset(&c->normal_pointer, 0, sizeof(array_pointer_t));

    /* Lighting - OpenGL defaults */
    for (int i = 0; i < MAX_LIGHTS; i++) {
        light_init(&c->lights[i], i);
    }

    /* Material - OpenGL defaults */
    material_init(&c->material_front);
    c->material_back = c->material_front;

    /* Light model */
    c->light_model_ambient = color(0.2f, 0.2f, 0.2f, 1.0f);
    c->light_model_local_viewer = GL_FALSE;
    c->light_model_two_side = GL_FALSE;

    /* Color material */
    c->color_material_face = GL_FRONT_AND_BACK;
    c->color_material_mode = GL_AMBIENT_AND_DIFFUSE;

    /* Shade model */
    c->shade_model = GL_SMOOTH;

    /* Display lists */
    list_store_init(&c->lists);
    c->list_base = 0;
    c->list_index = 0;
    c->list_mode = 0;
    c->list_call_depth = 0;

    /* Vertex buffer */
    c->vertices.data = NULL;
    c->vertices.count = 0;
    c->vertices.capacity = 0;

    /* Error state */
    c->error = GL_NO_ERROR;

    return c;
}

void gl_destroy_context(GLState *c)
{
    if (c) {
        list_store_free(&c->lists);
        buffer_store_free(&c->buffers);
        texture_store_free(&c->textures);
        framebuffer_free(&c->framebuffer);
        if (c->vertices.data) {
            mtgl_free(c->vertices.data);
        }
        mtgl_free(c);
    }
}

void gl_make_current(GLState *c)
{
    ctx = c;
}

GLState *gl_get_current_context(void)
{
    return ctx;
}

/* Helper to build vertex from current state */
static void emit_vertex(float x, float y, float z, float w)
{
    /* Compute eye-space position (after modelview, before projection) */
    vec4_t v = vec4(x, y, z, w);
    mat4_t mv = mat4_from_array(ctx->modelview_matrix[ctx->modelview_stack_depth]);
    vec4_t eye = mat4_mul_vec4(mv, v);
    float eye_z = -eye.z;  /* Negate because OpenGL looks down -Z */
    vec3_t eye_pos = vec3(eye.x, eye.y, eye.z);

    /* Transform normal to eye-space using inverse-transpose of modelview.
     * This correctly handles non-uniform scaling. */
    vec3_t obj_normal = ctx->current_normal;
    mat4_t normal_mat = mat4_normal_matrix(mv);
    vec4_t n4 = mat4_mul_vec4(normal_mat, vec4(obj_normal.x, obj_normal.y, obj_normal.z, 0.0f));
    vec3_t eye_normal = vec3(n4.x, n4.y, n4.z);
    /* Always normalize the result since the normal matrix may not preserve length */
    eye_normal = vec3_normalize(eye_normal);

    /* Get base vertex color */
    color_t vert_color = ctx->current_color;

    /* Apply color material if enabled */
    if ((ctx->flags & FLAG_LIGHTING) && (ctx->flags & FLAG_COLOR_MATERIAL)) {
        GLenum mode = ctx->color_material_mode;
        GLenum face = ctx->color_material_face;

        /* Clamp color components to [0, 1] before using as material property */
        color_t clamped_color = color_clamp(vert_color);

        if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
            if (mode == GL_AMBIENT || mode == GL_AMBIENT_AND_DIFFUSE)
                ctx->material_front.ambient = clamped_color;
            if (mode == GL_DIFFUSE || mode == GL_AMBIENT_AND_DIFFUSE)
                ctx->material_front.diffuse = clamped_color;
            if (mode == GL_SPECULAR)
                ctx->material_front.specular = clamped_color;
            if (mode == GL_EMISSION)
                ctx->material_front.emission = clamped_color;
        }
        if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
            if (mode == GL_AMBIENT || mode == GL_AMBIENT_AND_DIFFUSE)
                ctx->material_back.ambient = clamped_color;
            if (mode == GL_DIFFUSE || mode == GL_AMBIENT_AND_DIFFUSE)
                ctx->material_back.diffuse = clamped_color;
            if (mode == GL_SPECULAR)
                ctx->material_back.specular = clamped_color;
            if (mode == GL_EMISSION)
                ctx->material_back.emission = clamped_color;
        }
    }

    /* Compute per-vertex lighting for GL_FLAT and GL_SMOOTH (Gouraud shading) */
    /* For GL_PHONG, lighting is computed per-fragment in the rasterizer */
    /* Note: Two-sided lighting for Gouraud shading is handled per-fragment
     * since we don't know the face orientation at vertex time. The front material
     * is used here; back-face lighting is applied during rasterization. */
    if ((ctx->flags & FLAG_LIGHTING) && ctx->shade_model != GL_PHONG) {
        vert_color = compute_lighting(ctx, eye_pos, eye_normal, &ctx->material_front);
    }

    /* Full transform for clip-space position */
    vec4_t pos = transform_vertex(ctx, x, y, z, w);

    /* Apply texture matrix to texture coordinates */
    vec2_t texcoord = ctx->current_texcoord;
    mat4_t tex_mat = mat4_from_array(ctx->texture_matrix[ctx->texture_stack_depth]);
    vec4_t tex4 = mat4_mul_vec4(tex_mat, vec4(texcoord.x, texcoord.y, 0.0f, 1.0f));
    /* Perspective divide if w != 1 (for projective texturing) */
    if (tex4.w != 0.0f && tex4.w != 1.0f) {
        texcoord = vec2(tex4.x / tex4.w, tex4.y / tex4.w);
    } else {
        texcoord = vec2(tex4.x, tex4.y);
    }

    /* Build vertex */
    vertex_t vert;
    vert.position = pos;
    vert.color = vert_color;
    vert.texcoord = texcoord;
    vert.normal = obj_normal;
    vert.eye_z = eye_z;
    vert.eye_pos = eye_pos;
    vert.eye_normal = eye_normal;

    vertex_buffer_push(ctx, vert);
}

/* Helper to get flag from cap */
static uint32_t cap_to_flag(GLenum cap)
{
    switch (cap) {
        case GL_DEPTH_TEST:     return FLAG_DEPTH_TEST;
        case GL_CULL_FACE:      return FLAG_CULL_FACE;
        case GL_BLEND:          return FLAG_BLEND;
        case GL_TEXTURE_2D:     return FLAG_TEXTURE_2D;
        case GL_LIGHTING:       return FLAG_LIGHTING;
        case GL_FOG:            return FLAG_FOG;
        case GL_NORMALIZE:      return FLAG_NORMALIZE;
        case GL_COLOR_MATERIAL: return FLAG_COLOR_MATERIAL;
        case GL_ALPHA_TEST:     return FLAG_ALPHA_TEST;
        case GL_SCISSOR_TEST:   return FLAG_SCISSOR_TEST;
        case GL_STENCIL_TEST:   return FLAG_STENCIL_TEST;
        default:                return 0;
    }
}

/* State functions */

void glEnable(GLenum cap)
{
    CHECK_CTX();
    if (list_record_enable(cap)) return;

    uint32_t flag = cap_to_flag(cap);
    if (flag) {
        ctx->flags |= flag;
        return;
    }

    if (cap >= GL_LIGHT0 && cap <= GL_LIGHT7) {
        ctx->lights[cap - GL_LIGHT0].enabled = GL_TRUE;
        return;
    }

    gl_set_error(ctx, GL_INVALID_ENUM);
}

void glDisable(GLenum cap)
{
    CHECK_CTX();
    if (list_record_disable(cap)) return;

    uint32_t flag = cap_to_flag(cap);
    if (flag) {
        ctx->flags &= ~flag;
        return;
    }

    if (cap >= GL_LIGHT0 && cap <= GL_LIGHT7) {
        ctx->lights[cap - GL_LIGHT0].enabled = GL_FALSE;
        return;
    }

    gl_set_error(ctx, GL_INVALID_ENUM);
}

void glClear(GLbitfield mask)
{
    CHECK_CTX();
    framebuffer_t *fb = &ctx->framebuffer;

    /* Determine clear region (scissor or full buffer) */
    int32_t x0, y0, x1, y1;
    if (ctx->flags & FLAG_SCISSOR_TEST) {
        x0 = ctx->scissor_x;
        y0 = ctx->scissor_y;
        x1 = ctx->scissor_x + ctx->scissor_w;
        y1 = ctx->scissor_y + ctx->scissor_h;
        /* Clamp to framebuffer bounds */
        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
        if (x1 > fb->width) x1 = fb->width;
        if (y1 > fb->height) y1 = fb->height;
    } else {
        x0 = 0;
        y0 = 0;
        x1 = fb->width;
        y1 = fb->height;
    }

    if (mask & GL_COLOR_BUFFER_BIT) {
        pixel_t clear_pixel = color_to_rgba32(ctx->clear_color);
        for (int32_t y = y0; y < y1; y++) {
            for (int32_t x = x0; x < x1; x++) {
                fb->color[y * fb->width + x] = clear_pixel;
            }
        }
    }
    if (mask & GL_DEPTH_BUFFER_BIT) {
        float clear_depth = (float)ctx->clear_depth;
        for (int32_t y = y0; y < y1; y++) {
            for (int32_t x = x0; x < x1; x++) {
                fb->depth[y * fb->width + x] = clear_depth;
            }
        }
    }
    if (mask & GL_STENCIL_BUFFER_BIT) {
        uint8_t clear_stencil = (uint8_t)(ctx->stencil_clear & 0xFF);
        for (int32_t y = y0; y < y1; y++) {
            for (int32_t x = x0; x < x1; x++) {
                fb->stencil[y * fb->width + x] = clear_stencil;
            }
        }
    }
}

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    CHECK_CTX();
    ctx->clear_color = color(red, green, blue, alpha);
}

void glClearDepth(GLclampd depth)
{
    CHECK_CTX();
    ctx->clear_depth = depth;
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    CHECK_CTX();
    ctx->viewport_x = x;
    ctx->viewport_y = y;
    ctx->viewport_w = width;
    ctx->viewport_h = height;
}

/* Matrix functions */

/* Helper to get current matrix pointer */
static GLfloat *current_matrix(void)
{
    switch (ctx->matrix_mode) {
        case GL_MODELVIEW:
            return ctx->modelview_matrix[ctx->modelview_stack_depth];
        case GL_PROJECTION:
            return ctx->projection_matrix[ctx->projection_stack_depth];
        case GL_TEXTURE:
            return ctx->texture_matrix[ctx->texture_stack_depth];
        default:
            return ctx->modelview_matrix[ctx->modelview_stack_depth];
    }
}

static GLint *current_stack_depth(void)
{
    switch (ctx->matrix_mode) {
        case GL_MODELVIEW:  return &ctx->modelview_stack_depth;
        case GL_PROJECTION: return &ctx->projection_stack_depth;
        case GL_TEXTURE:    return &ctx->texture_stack_depth;
        default:            return &ctx->modelview_stack_depth;
    }
}

void glMatrixMode(GLenum mode)
{
    CHECK_CTX();
    if (list_record_matrix_mode(mode)) return;
    ctx->matrix_mode = mode;
}

void glLoadIdentity(void)
{
    CHECK_CTX();
    if (list_record_load_identity()) return;
    mat4_to_array(mat4_identity(), current_matrix());
}

void glPushMatrix(void)
{
    CHECK_CTX();
    if (list_record_push_matrix()) return;

    GLint *depth = current_stack_depth();
    if (*depth >= MAX_MATRIX_STACK_DEPTH - 1) {
        gl_set_error(ctx, GL_STACK_OVERFLOW);
        return;
    }

    mat4_t src = mat4_from_array(current_matrix());
    (*depth)++;
    mat4_to_array(src, current_matrix());
}

void glPopMatrix(void)
{
    CHECK_CTX();
    if (list_record_pop_matrix()) return;

    GLint *depth = current_stack_depth();
    if (*depth <= 0) {
        gl_set_error(ctx, GL_STACK_UNDERFLOW);
        return;
    }

    (*depth)--;
}

void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near, GLdouble far)
{
    CHECK_CTX();
    if (list_record_ortho(left, right, bottom, top, near, far)) return;
    GLfloat *m = current_matrix();
    mat4_t result = mat4_mul(mat4_from_array(m), mat4_ortho(left, right, bottom, top, near, far));
    mat4_to_array(result, m);
}

void glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near, GLdouble far)
{
    CHECK_CTX();
    if (list_record_frustum(left, right, bottom, top, near, far)) return;
    GLfloat *m = current_matrix();
    mat4_t result = mat4_mul(mat4_from_array(m), mat4_frustum(left, right, bottom, top, near, far));
    mat4_to_array(result, m);
}

void glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
    CHECK_CTX();
    if (list_record_translatef(x, y, z)) return;
    GLfloat *m = current_matrix();
    mat4_t result = mat4_mul(mat4_from_array(m), mat4_translate(x, y, z));
    mat4_to_array(result, m);
}

void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    CHECK_CTX();
    if (list_record_rotatef(angle, x, y, z)) return;
    GLfloat *m = current_matrix();
    mat4_t result = mat4_mul(mat4_from_array(m), mat4_rotate(angle, x, y, z));
    mat4_to_array(result, m);
}

void glScalef(GLfloat x, GLfloat y, GLfloat z)
{
    CHECK_CTX();
    if (list_record_scalef(x, y, z)) return;
    GLfloat *m = current_matrix();
    mat4_t result = mat4_mul(mat4_from_array(m), mat4_scale(x, y, z));
    mat4_to_array(result, m);
}

void glMultMatrixf(const GLfloat *mult)
{
    CHECK_CTX();
    if (list_record_mult_matrixf(mult)) return;
    GLfloat *m = current_matrix();
    mat4_t result = mat4_mul(mat4_from_array(m), mat4_from_array(mult));
    mat4_to_array(result, m);
}

void glLoadMatrixf(const GLfloat *m)
{
    CHECK_CTX();
    if (list_record_load_matrixf(m)) return;
    mat4_to_array(mat4_from_array(m), current_matrix());
}

/* Vertex specification */

void glBegin(GLenum mode)
{
    CHECK_CTX();
    if (list_record_begin(mode)) return;

    /* Check for nested glBegin */
    if (ctx->flags & FLAG_INSIDE_BEGIN_END) {
        gl_set_error(ctx, GL_INVALID_OPERATION);
        return;
    }

    /* Validate primitive mode */
    if (mode > GL_POLYGON) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }

    ctx->primitive_mode = mode;
    ctx->flags |= FLAG_INSIDE_BEGIN_END;
}

void glEnd(void)
{
    CHECK_CTX();
    if (list_record_end()) return;

    /* Check for glEnd without glBegin */
    if (!(ctx->flags & FLAG_INSIDE_BEGIN_END)) {
        gl_set_error(ctx, GL_INVALID_OPERATION);
        return;
    }

    ctx->flags &= ~FLAG_INSIDE_BEGIN_END;

    switch (ctx->primitive_mode) {
        case GL_POINTS:        flush_points(ctx); break;
        case GL_LINES:         flush_lines(ctx); break;
        case GL_LINE_STRIP:    flush_line_strip(ctx); break;
        case GL_LINE_LOOP:     flush_line_loop(ctx); break;
        case GL_TRIANGLES:     flush_triangles(ctx); break;
        case GL_TRIANGLE_STRIP: flush_triangle_strip(ctx); break;
        case GL_TRIANGLE_FAN:  flush_triangle_fan(ctx); break;
        case GL_QUADS:         flush_quads(ctx); break;
        case GL_QUAD_STRIP:    flush_quad_strip(ctx); break;
        case GL_POLYGON:       flush_polygon(ctx); break;
    }

    vertex_buffer_clear(ctx);
}

void glVertex2f(GLfloat x, GLfloat y)
{
    CHECK_CTX();
    if (list_record_vertex(x, y, 0.0f)) return;
    emit_vertex(x, y, 0.0f, 1.0f);
}

void glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
    CHECK_CTX();
    if (list_record_vertex(x, y, z)) return;
    emit_vertex(x, y, z, 1.0f);
}

void glVertex2i(GLint x, GLint y)
{
    glVertex2f((float)x, (float)y);
}

void glVertex3i(GLint x, GLint y, GLint z)
{
    glVertex3f((float)x, (float)y, (float)z);
}

/* Helper to check if float is valid (not NaN or Inf) */
static inline int is_valid_float(float f)
{
    return !isnan(f) && !isinf(f);
}

void glColor3f(GLfloat r, GLfloat g, GLfloat b)
{
    CHECK_CTX();
    /* Clamp colors to valid range, treat NaN/Inf as 0 */
    if (!is_valid_float(r)) r = 0.0f;
    if (!is_valid_float(g)) g = 0.0f;
    if (!is_valid_float(b)) b = 0.0f;
    if (r < 0.0f) r = 0.0f; else if (r > 1.0f) r = 1.0f;
    if (g < 0.0f) g = 0.0f; else if (g > 1.0f) g = 1.0f;
    if (b < 0.0f) b = 0.0f; else if (b > 1.0f) b = 1.0f;
    if (list_record_color(r, g, b, 1.0f)) return;
    ctx->current_color = color(r, g, b, 1.0f);
}

void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    CHECK_CTX();
    /* Clamp colors to valid range, treat NaN/Inf as 0 */
    if (!is_valid_float(r)) r = 0.0f;
    if (!is_valid_float(g)) g = 0.0f;
    if (!is_valid_float(b)) b = 0.0f;
    if (!is_valid_float(a)) a = 1.0f;
    if (r < 0.0f) r = 0.0f; else if (r > 1.0f) r = 1.0f;
    if (g < 0.0f) g = 0.0f; else if (g > 1.0f) g = 1.0f;
    if (b < 0.0f) b = 0.0f; else if (b > 1.0f) b = 1.0f;
    if (a < 0.0f) a = 0.0f; else if (a > 1.0f) a = 1.0f;
    if (list_record_color(r, g, b, a)) return;
    ctx->current_color = color(r, g, b, a);
}

void glColor3ub(GLubyte r, GLubyte g, GLubyte b)
{
    CHECK_CTX();
    float rf = r / 255.0f, gf = g / 255.0f, bf = b / 255.0f;
    if (list_record_color(rf, gf, bf, 1.0f)) return;
    ctx->current_color = color(rf, gf, bf, 1.0f);
}

void glColor4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{
    CHECK_CTX();
    float rf = r / 255.0f, gf = g / 255.0f, bf = b / 255.0f, af = a / 255.0f;
    if (list_record_color(rf, gf, bf, af)) return;
    ctx->current_color = color(rf, gf, bf, af);
}

void glTexCoord2f(GLfloat s, GLfloat t)
{
    CHECK_CTX();
    if (list_record_texcoord(s, t)) return;
    ctx->current_texcoord = vec2(s, t);
}

void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
    CHECK_CTX();
    if (list_record_normal(nx, ny, nz)) return;
    ctx->current_normal = vec3(nx, ny, nz);
}

/* Texture functions */

void glGenTextures(GLsizei n, GLuint *textures)
{
    CHECK_CTX();
    if (n < 0) {
        gl_set_error(ctx, GL_INVALID_VALUE);
        return;
    }
    for (GLsizei i = 0; i < n; i++) {
        textures[i] = texture_alloc(&ctx->textures);
    }
}

void glDeleteTextures(GLsizei n, const GLuint *textures)
{
    CHECK_CTX();
    if (n < 0) {
        gl_set_error(ctx, GL_INVALID_VALUE);
        return;
    }
    for (GLsizei i = 0; i < n; i++) {
        if (textures[i] == ctx->bound_texture_2d) {
            ctx->bound_texture_2d = 0;
        }
        texture_free(&ctx->textures, textures[i]);
    }
}

void glBindTexture(GLenum target, GLuint texture)
{
    CHECK_CTX();
    if (list_record_bind_texture(target, texture)) return;
    if (target != GL_TEXTURE_2D) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }
    ctx->bound_texture_2d = texture;
}

void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    CHECK_CTX();

    /* Only support GL_TEXTURE_2D, level 0, GL_UNSIGNED_BYTE */
    if (target != GL_TEXTURE_2D) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }
    if (level != 0 || type != GL_UNSIGNED_BYTE) {
        gl_set_error(ctx, GL_INVALID_VALUE);
        return;
    }
    if (width < 0 || height < 0) {
        gl_set_error(ctx, GL_INVALID_VALUE);
        return;
    }

    texture_t *tex = texture_get(&ctx->textures, ctx->bound_texture_2d);
    if (!tex) return;

    const uint8_t *data = (const uint8_t *)pixels;

    switch (format) {
        case GL_RGBA:
            texture_upload_rgba(tex, width, height, data);
            break;
        case GL_RGB:
            texture_upload_rgb(tex, width, height, data);
            break;
        case GL_LUMINANCE:
            texture_upload_luminance(tex, width, height, data);
            break;
        case GL_LUMINANCE_ALPHA:
            texture_upload_luminance_alpha(tex, width, height, data);
            break;
        default:
            gl_set_error(ctx, GL_INVALID_ENUM);
            break;
    }
}

void glTexParameteri(GLenum target, GLenum pname, GLint param)
{
    CHECK_CTX();
    if (target != GL_TEXTURE_2D) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }

    texture_t *tex = texture_get(&ctx->textures, ctx->bound_texture_2d);
    if (!tex) return;

    switch (pname) {
        case GL_TEXTURE_MIN_FILTER:
            /* Valid min filters: NEAREST, LINEAR, and mipmap variants */
            if (param != GL_NEAREST && param != GL_LINEAR &&
                param != GL_NEAREST_MIPMAP_NEAREST && param != GL_LINEAR_MIPMAP_NEAREST &&
                param != GL_NEAREST_MIPMAP_LINEAR && param != GL_LINEAR_MIPMAP_LINEAR) {
                gl_set_error(ctx, GL_INVALID_ENUM);
                return;
            }
            tex->min_filter = param;
            break;
        case GL_TEXTURE_MAG_FILTER:
            /* Valid mag filters: NEAREST, LINEAR only */
            if (param != GL_NEAREST && param != GL_LINEAR) {
                gl_set_error(ctx, GL_INVALID_ENUM);
                return;
            }
            tex->mag_filter = param;
            break;
        case GL_TEXTURE_WRAP_S:
            /* Valid wrap modes: REPEAT, CLAMP, CLAMP_TO_EDGE */
            if (param != GL_REPEAT && param != GL_CLAMP && param != GL_CLAMP_TO_EDGE) {
                gl_set_error(ctx, GL_INVALID_ENUM);
                return;
            }
            tex->wrap_s = param;
            break;
        case GL_TEXTURE_WRAP_T:
            if (param != GL_REPEAT && param != GL_CLAMP && param != GL_CLAMP_TO_EDGE) {
                gl_set_error(ctx, GL_INVALID_ENUM);
                return;
            }
            tex->wrap_t = param;
            break;
        default:
            gl_set_error(ctx, GL_INVALID_ENUM);
            break;
    }
}

/* Misc */

void glFlush(void)
{
    CHECK_CTX();
    /* Software renderer - nothing to flush */
}

void glFinish(void)
{
    CHECK_CTX();
    /* Software renderer - nothing to wait for */
}

GLenum glGetError(void)
{
    if (!ctx) return GL_NO_ERROR;
    GLenum err = ctx->error;
    ctx->error = GL_NO_ERROR;
    return err;
}

/* Helper to validate blend factor */
static int is_valid_blend_factor(GLenum factor, int is_src)
{
    switch (factor) {
        case GL_ZERO:
        case GL_ONE:
        case GL_SRC_COLOR:
        case GL_ONE_MINUS_SRC_COLOR:
        case GL_DST_COLOR:
        case GL_ONE_MINUS_DST_COLOR:
        case GL_SRC_ALPHA:
        case GL_ONE_MINUS_SRC_ALPHA:
        case GL_DST_ALPHA:
        case GL_ONE_MINUS_DST_ALPHA:
        case GL_CONSTANT_COLOR:
        case GL_ONE_MINUS_CONSTANT_COLOR:
        case GL_CONSTANT_ALPHA:
        case GL_ONE_MINUS_CONSTANT_ALPHA:
            return 1;
        case GL_SRC_ALPHA_SATURATE:
            /* GL_SRC_ALPHA_SATURATE is only valid for source factor */
            return is_src;
        default:
            return 0;
    }
}

void glBlendFunc(GLenum sfactor, GLenum dfactor)
{
    CHECK_CTX();
    if (list_record_blend_func(sfactor, dfactor)) return;
    if (!is_valid_blend_factor(sfactor, 1) || !is_valid_blend_factor(dfactor, 0)) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }
    ctx->blend_src = sfactor;
    ctx->blend_dst = dfactor;
}

void glCullFace(GLenum mode)
{
    CHECK_CTX();
    if (list_record_cull_face(mode)) return;
    if (mode != GL_FRONT && mode != GL_BACK && mode != GL_FRONT_AND_BACK) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }
    ctx->cull_face_mode = mode;
}

void glFrontFace(GLenum mode)
{
    CHECK_CTX();
    if (list_record_front_face(mode)) return;
    if (mode != GL_CW && mode != GL_CCW) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }
    ctx->front_face = mode;
}

void glDepthFunc(GLenum func)
{
    CHECK_CTX();
    if (list_record_depth_func(func)) return;
    if (func < GL_NEVER || func > GL_ALWAYS) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }
    ctx->depth_func = func;
}

void glDepthMask(GLboolean flag)
{
    CHECK_CTX();
    if (list_record_depth_mask(flag)) return;
    ctx->depth_mask = flag;
}

void glAlphaFunc(GLenum func, GLclampf ref)
{
    CHECK_CTX();
    if (func < GL_NEVER || func > GL_ALWAYS) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }
    ctx->alpha_func = func;
    /* Clamp reference to [0, 1] */
    if (ref < 0.0f) ref = 0.0f;
    if (ref > 1.0f) ref = 1.0f;
    ctx->alpha_ref = ref;
}

void glLineWidth(GLfloat width)
{
    CHECK_CTX();
    if (width <= 0.0f || isnan(width) || isinf(width)) {
        gl_set_error(ctx, GL_INVALID_VALUE);
        return;
    }
    ctx->line_width = width;
    /* Note: line width > 1 not implemented, stored for glGet* queries */
}

void glPointSize(GLfloat size)
{
    CHECK_CTX();
    if (size <= 0.0f || isnan(size) || isinf(size)) {
        gl_set_error(ctx, GL_INVALID_VALUE);
        return;
    }
    ctx->point_size = size;
    /* Note: point size > 1 not implemented, stored for glGet* queries */
}

void glPolygonMode(GLenum face, GLenum mode)
{
    CHECK_CTX();
    if (face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }
    if (mode != GL_POINT && mode != GL_LINE && mode != GL_FILL) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }
    if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
        ctx->polygon_mode_front = mode;
    }
    if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
        ctx->polygon_mode_back = mode;
    }
    /* Note: GL_POINT and GL_LINE modes not implemented, always renders filled */
}

void glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    CHECK_CTX();
    if (width < 0 || height < 0) {
        gl_set_error(ctx, GL_INVALID_VALUE);
        return;
    }
    ctx->scissor_x = x;
    ctx->scissor_y = y;
    ctx->scissor_w = width;
    ctx->scissor_h = height;
}

void glDepthRange(GLclampd near, GLclampd far)
{
    CHECK_CTX();
    /* Clamp to [0, 1] */
    if (near < 0.0) near = 0.0;
    if (near > 1.0) near = 1.0;
    if (far < 0.0) far = 0.0;
    if (far > 1.0) far = 1.0;
    ctx->depth_near = near;
    ctx->depth_far = far;
}

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    CHECK_CTX();
    ctx->color_mask_r = red;
    ctx->color_mask_g = green;
    ctx->color_mask_b = blue;
    ctx->color_mask_a = alpha;
}

/* Stencil functions - stubs only, stencil buffer not implemented */

void glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
    CHECK_CTX();
    /* Validate func parameter */
    switch (func) {
        case GL_NEVER:
        case GL_LESS:
        case GL_LEQUAL:
        case GL_GREATER:
        case GL_GEQUAL:
        case GL_EQUAL:
        case GL_NOTEQUAL:
        case GL_ALWAYS:
            break;
        default:
            gl_set_error(ctx, GL_INVALID_ENUM);
            return;
    }
    ctx->stencil_func = func;
    ctx->stencil_ref = ref;
    ctx->stencil_mask = mask;
}

void glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass)
{
    CHECK_CTX();
    /* Validate parameters */
    GLenum ops[] = { sfail, dpfail, dppass };
    for (int i = 0; i < 3; i++) {
        switch (ops[i]) {
            case GL_KEEP:
            case GL_ZERO:
            case GL_REPLACE:
            case GL_INCR:
            case GL_INCR_WRAP:
            case GL_DECR:
            case GL_DECR_WRAP:
            case GL_INVERT:
                break;
            default:
                gl_set_error(ctx, GL_INVALID_ENUM);
                return;
        }
    }
    ctx->stencil_fail = sfail;
    ctx->stencil_zfail = dpfail;
    ctx->stencil_zpass = dppass;
}

void glStencilMask(GLuint mask)
{
    CHECK_CTX();
    ctx->stencil_writemask = mask;
}

void glClearStencil(GLint s)
{
    CHECK_CTX();
    ctx->stencil_clear = s;
}

void glPixelStorei(GLenum pname, GLint param)
{
    CHECK_CTX();
    /* Validate alignment values */
    if (pname == GL_PACK_ALIGNMENT || pname == GL_UNPACK_ALIGNMENT) {
        if (param != 1 && param != 2 && param != 4 && param != 8) {
            gl_set_error(ctx, GL_INVALID_VALUE);
            return;
        }
        /* Store but don't use - we always assume tightly packed data */
    } else {
        gl_set_error(ctx, GL_INVALID_ENUM);
    }
}

void glHint(GLenum target, GLenum mode)
{
    CHECK_CTX();
    if (mode != GL_DONT_CARE && mode != GL_FASTEST && mode != GL_NICEST) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }
    switch (target) {
        case GL_PERSPECTIVE_CORRECTION_HINT:
            ctx->perspective_correction_hint = mode;
            break;
        case GL_POINT_SMOOTH_HINT:
        case GL_LINE_SMOOTH_HINT:
        case GL_FOG_HINT:
            /* Silently accept but ignore */
            break;
        default:
            gl_set_error(ctx, GL_INVALID_ENUM);
            break;
    }
}

/* Pixel transfer */

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
    CHECK_CTX();
    if (type != GL_UNSIGNED_BYTE || !pixels) return;

    framebuffer_t *fb = &ctx->framebuffer;
    uint8_t *dst = (uint8_t *)pixels;

    for (GLsizei row = 0; row < height; row++) {
        GLint src_y = y + row;
        /* Flip Y - OpenGL has origin at bottom-left */
        GLint fb_y = fb->height - 1 - src_y;

        if (fb_y < 0 || fb_y >= fb->height) {
            /* Out of bounds - fill with zeros */
            if (format == GL_RGBA) {
                memset(dst + row * width * 4, 0, width * 4);
            } else if (format == GL_RGB) {
                memset(dst + row * width * 3, 0, width * 3);
            }
            continue;
        }

        for (GLsizei col = 0; col < width; col++) {
            GLint src_x = x + col;
            uint8_t r = 0, g = 0, b = 0, a = 255;

            if (src_x >= 0 && src_x < fb->width) {
                uint32_t pixel = fb->color[fb_y * fb->width + src_x];
                /* Internal format is ABGR (a<<24|b<<16|g<<8|r) */
                r = pixel & 0xFF;
                g = (pixel >> 8) & 0xFF;
                b = (pixel >> 16) & 0xFF;
                a = (pixel >> 24) & 0xFF;
            }

            if (format == GL_RGBA) {
                size_t idx = (row * width + col) * 4;
                dst[idx + 0] = r;
                dst[idx + 1] = g;
                dst[idx + 2] = b;
                dst[idx + 3] = a;
            } else if (format == GL_RGB) {
                size_t idx = (row * width + col) * 3;
                dst[idx + 0] = r;
                dst[idx + 1] = g;
                dst[idx + 2] = b;
            }
        }
    }
}

/* Alpha test comparison helper for glDrawPixels */
static inline int drawpixels_alpha_test(GLenum func, float incoming, float ref)
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

/* Depth test comparison helper for glDrawPixels */
static inline int drawpixels_depth_test(GLenum func, float incoming, float stored)
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

/* Blend factor helper for glDrawPixels */
static inline color_t drawpixels_blend_factor(GLenum factor, color_t src, color_t dst)
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

void glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
    CHECK_CTX();
    if (type != GL_UNSIGNED_BYTE || !pixels || !ctx->raster_pos_valid) return;

    framebuffer_t *fb = &ctx->framebuffer;
    const uint8_t *src = (const uint8_t *)pixels;

    int alpha_test_enabled = ctx->flags & FLAG_ALPHA_TEST;
    int depth_test_enabled = ctx->flags & FLAG_DEPTH_TEST;
    int blend_enabled = ctx->flags & FLAG_BLEND;

    /* Pixels are drawn at depth 0 (front of the depth range) */
    float pixel_depth = 0.0f;

    for (GLsizei row = 0; row < height; row++) {
        GLint dst_y = ctx->raster_pos_y + row;
        /* Flip Y - OpenGL has origin at bottom-left */
        GLint fb_y = fb->height - 1 - dst_y;

        if (fb_y < 0 || fb_y >= fb->height) continue;

        for (GLsizei col = 0; col < width; col++) {
            GLint dst_x = ctx->raster_pos_x + col;

            if (dst_x < 0 || dst_x >= fb->width) continue;

            uint8_t r, g, b, a = 255;

            if (format == GL_RGBA) {
                size_t idx = (row * width + col) * 4;
                r = src[idx + 0];
                g = src[idx + 1];
                b = src[idx + 2];
                a = src[idx + 3];
            } else if (format == GL_RGB) {
                size_t idx = (row * width + col) * 3;
                r = src[idx + 0];
                g = src[idx + 1];
                b = src[idx + 2];
            } else if (format == GL_LUMINANCE) {
                uint8_t l = src[row * width + col];
                r = g = b = l;
            } else if (format == GL_LUMINANCE_ALPHA) {
                size_t idx = (row * width + col) * 2;
                uint8_t l = src[idx + 0];
                r = g = b = l;
                a = src[idx + 1];
            } else {
                continue;
            }

            /* Alpha test - use configurable function and reference */
            if (alpha_test_enabled) {
                float alpha_val = a / 255.0f;
                if (!drawpixels_alpha_test(ctx->alpha_func, alpha_val, ctx->alpha_ref)) {
                    continue;
                }
            }

            /* Depth test */
            if (depth_test_enabled) {
                float stored_depth = framebuffer_getdepth(fb, dst_x, fb_y);
                if (!drawpixels_depth_test(ctx->depth_func, pixel_depth, stored_depth)) {
                    continue;
                }
            }

            color_t src_color = color(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);

            /* Alpha blending */
            if (blend_enabled) {
                uint32_t dst_pixel = fb->color[fb_y * fb->width + dst_x];
                color_t dst_color = color_from_rgba32(dst_pixel);
                color_t sf = drawpixels_blend_factor(ctx->blend_src, src_color, dst_color);
                color_t df = drawpixels_blend_factor(ctx->blend_dst, src_color, dst_color);
                src_color = color_clamp(color_add(color_mul(src_color, sf), color_mul(dst_color, df)));
            }

            /* Write depth if depth test enabled and passed */
            if (depth_test_enabled && ctx->depth_mask) {
                framebuffer_putdepth(fb, dst_x, fb_y, pixel_depth);
            }

            fb->color[fb_y * fb->width + dst_x] = color_to_rgba32(src_color);
        }
    }
}

void glRasterPos2i(GLint x, GLint y)
{
    CHECK_CTX();
    /* Transform through modelview and projection, then to window coords */
    vec4_t clip = transform_vertex(ctx, (float)x, (float)y, 0.0f, 1.0f);

    /* Check if clipped */
    if (clip.w <= 0.0f) {
        ctx->raster_pos_valid = GL_FALSE;
        return;
    }

    /* Perspective divide to NDC */
    float ndc_x = clip.x / clip.w;
    float ndc_y = clip.y / clip.w;

    /* NDC to window coordinates */
    int32_t sx, sy;
    ndc_to_screen(ctx, ndc_x, ndc_y, &sx, &sy);

    ctx->raster_pos_x = sx;
    ctx->raster_pos_y = ctx->framebuffer.height - 1 - sy;  /* Flip to GL convention */
    ctx->raster_pos_valid = GL_TRUE;
}

void glRasterPos2f(GLfloat x, GLfloat y)
{
    glRasterPos3f(x, y, 0.0f);
}

void glRasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
    CHECK_CTX();
    vec4_t clip = transform_vertex(ctx, x, y, z, 1.0f);

    if (clip.w <= 0.0f) {
        ctx->raster_pos_valid = GL_FALSE;
        return;
    }

    float ndc_x = clip.x / clip.w;
    float ndc_y = clip.y / clip.w;

    int32_t sx, sy;
    ndc_to_screen(ctx, ndc_x, ndc_y, &sx, &sy);

    ctx->raster_pos_x = sx;
    ctx->raster_pos_y = ctx->framebuffer.height - 1 - sy;
    ctx->raster_pos_valid = GL_TRUE;
}

/* Fog */

/* Helper to validate fog mode */
static int is_valid_fog_mode(GLenum mode)
{
    return mode == GL_LINEAR || mode == GL_EXP || mode == GL_EXP2;
}

void glFogi(GLenum pname, GLint param)
{
    CHECK_CTX();
    switch (pname) {
        case GL_FOG_MODE:
            if (!is_valid_fog_mode((GLenum)param)) {
                gl_set_error(ctx, GL_INVALID_ENUM);
                return;
            }
            ctx->fog_mode = (GLenum)param;
            break;
        default:
            gl_set_error(ctx, GL_INVALID_ENUM);
            break;
    }
}

void glFogf(GLenum pname, GLfloat param)
{
    CHECK_CTX();
    switch (pname) {
        case GL_FOG_MODE:
            if (!is_valid_fog_mode((GLenum)(int)param)) {
                gl_set_error(ctx, GL_INVALID_ENUM);
                return;
            }
            ctx->fog_mode = (GLenum)(int)param;
            break;
        case GL_FOG_DENSITY:
            if (param < 0.0f) {
                gl_set_error(ctx, GL_INVALID_VALUE);
                return;
            }
            ctx->fog_density = param;
            break;
        case GL_FOG_START:
            ctx->fog_start = param;
            break;
        case GL_FOG_END:
            ctx->fog_end = param;
            break;
        default:
            gl_set_error(ctx, GL_INVALID_ENUM);
            break;
    }
}

void glFogfv(GLenum pname, const GLfloat *params)
{
    CHECK_CTX();
    switch (pname) {
        case GL_FOG_MODE:
            if (!is_valid_fog_mode((GLenum)(int)params[0])) {
                gl_set_error(ctx, GL_INVALID_ENUM);
                return;
            }
            ctx->fog_mode = (GLenum)(int)params[0];
            break;
        case GL_FOG_DENSITY:
            if (params[0] < 0.0f) {
                gl_set_error(ctx, GL_INVALID_VALUE);
                return;
            }
            ctx->fog_density = params[0];
            break;
        case GL_FOG_START:
            ctx->fog_start = params[0];
            break;
        case GL_FOG_END:
            ctx->fog_end = params[0];
            break;
        case GL_FOG_COLOR:
            ctx->fog_color = color(params[0], params[1], params[2], params[3]);
            break;
        default:
            gl_set_error(ctx, GL_INVALID_ENUM);
            break;
    }
}

/* Buffer objects (OpenGL 1.5) */

void glGenBuffers(GLsizei n, GLuint *buffers)
{
    CHECK_CTX();
    if (n < 0) {
        gl_set_error(ctx, GL_INVALID_VALUE);
        return;
    }
    buffer_gen(&ctx->buffers, n, buffers);
}

void glDeleteBuffers(GLsizei n, const GLuint *buffers)
{
    CHECK_CTX();
    if (n < 0) {
        gl_set_error(ctx, GL_INVALID_VALUE);
        return;
    }
    for (GLsizei i = 0; i < n; i++) {
        if (buffers[i] == ctx->bound_array_buffer) {
            ctx->bound_array_buffer = 0;
        }
        if (buffers[i] == ctx->bound_element_buffer) {
            ctx->bound_element_buffer = 0;
        }
    }
    buffer_delete(&ctx->buffers, n, buffers);
}

void glBindBuffer(GLenum target, GLuint buffer)
{
    CHECK_CTX();
    switch (target) {
        case GL_ARRAY_BUFFER:
            ctx->bound_array_buffer = buffer;
            break;
        case GL_ELEMENT_ARRAY_BUFFER:
            ctx->bound_element_buffer = buffer;
            break;
        default:
            gl_set_error(ctx, GL_INVALID_ENUM);
            break;
    }
}

void glBufferData(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage)
{
    CHECK_CTX();
    if (size < 0) {
        gl_set_error(ctx, GL_INVALID_VALUE);
        return;
    }

    GLuint buf_id = 0;
    switch (target) {
        case GL_ARRAY_BUFFER:
            buf_id = ctx->bound_array_buffer;
            break;
        case GL_ELEMENT_ARRAY_BUFFER:
            buf_id = ctx->bound_element_buffer;
            break;
        default:
            gl_set_error(ctx, GL_INVALID_ENUM);
            return;
    }

    buffer_t *buf = buffer_get(&ctx->buffers, buf_id);
    if (buf) {
        buffer_data(buf, size, data, usage);
    }
}

void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data)
{
    CHECK_CTX();
    if (offset < 0 || size < 0) {
        gl_set_error(ctx, GL_INVALID_VALUE);
        return;
    }

    GLuint buf_id = 0;
    switch (target) {
        case GL_ARRAY_BUFFER:
            buf_id = ctx->bound_array_buffer;
            break;
        case GL_ELEMENT_ARRAY_BUFFER:
            buf_id = ctx->bound_element_buffer;
            break;
        default:
            gl_set_error(ctx, GL_INVALID_ENUM);
            return;
    }

    buffer_t *buf = buffer_get(&ctx->buffers, buf_id);
    if (!buf) {
        gl_set_error(ctx, GL_INVALID_OPERATION);
        return;
    }
    if (buffer_sub_data(buf, offset, size, data) != 0) {
        gl_set_error(ctx, GL_INVALID_VALUE);
    }
}

/* Vertex arrays */

void glEnableClientState(GLenum array)
{
    CHECK_CTX();
    switch (array) {
        case GL_VERTEX_ARRAY:
            ctx->client_state |= CLIENT_VERTEX_ARRAY;
            break;
        case GL_COLOR_ARRAY:
            ctx->client_state |= CLIENT_COLOR_ARRAY;
            break;
        case GL_TEXTURE_COORD_ARRAY:
            ctx->client_state |= CLIENT_TEXTURE_COORD_ARRAY;
            break;
        case GL_NORMAL_ARRAY:
            ctx->client_state |= CLIENT_NORMAL_ARRAY;
            break;
        default:
            gl_set_error(ctx, GL_INVALID_ENUM);
            break;
    }
}

void glDisableClientState(GLenum array)
{
    CHECK_CTX();
    switch (array) {
        case GL_VERTEX_ARRAY:
            ctx->client_state &= ~CLIENT_VERTEX_ARRAY;
            break;
        case GL_COLOR_ARRAY:
            ctx->client_state &= ~CLIENT_COLOR_ARRAY;
            break;
        case GL_TEXTURE_COORD_ARRAY:
            ctx->client_state &= ~CLIENT_TEXTURE_COORD_ARRAY;
            break;
        case GL_NORMAL_ARRAY:
            ctx->client_state &= ~CLIENT_NORMAL_ARRAY;
            break;
        default:
            gl_set_error(ctx, GL_INVALID_ENUM);
            break;
    }
}

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    CHECK_CTX();
    if (size < 2 || size > 4) {
        gl_set_error(ctx, GL_INVALID_VALUE);
        return;
    }
    if (type != GL_FLOAT && type != GL_UNSIGNED_BYTE) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }
    if (stride < 0) {
        gl_set_error(ctx, GL_INVALID_VALUE);
        return;
    }
    ctx->vertex_pointer.size = size;
    ctx->vertex_pointer.type = type;
    ctx->vertex_pointer.stride = stride;
    ctx->vertex_pointer.pointer = pointer;
}

void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    CHECK_CTX();
    if (size < 3 || size > 4) {
        gl_set_error(ctx, GL_INVALID_VALUE);
        return;
    }
    if (type != GL_FLOAT && type != GL_UNSIGNED_BYTE) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }
    if (stride < 0) {
        gl_set_error(ctx, GL_INVALID_VALUE);
        return;
    }
    ctx->color_pointer.size = size;
    ctx->color_pointer.type = type;
    ctx->color_pointer.stride = stride;
    ctx->color_pointer.pointer = pointer;
}

void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    CHECK_CTX();
    if (size < 1 || size > 4) {
        gl_set_error(ctx, GL_INVALID_VALUE);
        return;
    }
    if (type != GL_FLOAT && type != GL_UNSIGNED_BYTE) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }
    if (stride < 0) {
        gl_set_error(ctx, GL_INVALID_VALUE);
        return;
    }
    ctx->texcoord_pointer.size = size;
    ctx->texcoord_pointer.type = type;
    ctx->texcoord_pointer.stride = stride;
    ctx->texcoord_pointer.pointer = pointer;
}

void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
    CHECK_CTX();
    if (type != GL_FLOAT && type != GL_UNSIGNED_BYTE) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }
    if (stride < 0) {
        gl_set_error(ctx, GL_INVALID_VALUE);
        return;
    }
    ctx->normal_pointer.size = 3;  /* Normals are always 3 components */
    ctx->normal_pointer.type = type;
    ctx->normal_pointer.stride = stride;
    ctx->normal_pointer.pointer = pointer;
}

/* Helper to get pointer to array data (handles VBO offset) */
static const void *get_array_pointer(const array_pointer_t *arr)
{
    if (ctx->bound_array_buffer) {
        buffer_t *buf = buffer_get(&ctx->buffers, ctx->bound_array_buffer);
        if (buf && buf->data) {
            return (const uint8_t *)buf->data + (size_t)arr->pointer;
        }
        return NULL;
    }
    return arr->pointer;
}

/* Helper to get element from array */
static void get_array_element(const array_pointer_t *arr, const void *base, GLint index,
                               float *out, int default_count)
{
    if (!base || index < 0) {
        /* Default values */
        for (int i = 0; i < default_count; i++) {
            out[i] = (i < 3) ? 0.0f : 1.0f;
        }
        return;
    }

    GLsizei stride = arr->stride;
    if (stride == 0) {
        /* Tightly packed */
        int comp_size = (arr->type == GL_FLOAT) ? (int)sizeof(GLfloat) : (int)sizeof(GLubyte);
        stride = arr->size * comp_size;
    }

    /* Validate stride is positive */
    if (stride <= 0) {
        for (int i = 0; i < default_count; i++) {
            out[i] = (i < 3) ? 0.0f : 1.0f;
        }
        return;
    }

    const uint8_t *ptr = (const uint8_t *)base + (size_t)index * (size_t)stride;

    for (int i = 0; i < arr->size && i < default_count; i++) {
        if (arr->type == GL_FLOAT) {
            out[i] = ((const GLfloat *)ptr)[i];
        } else if (arr->type == GL_UNSIGNED_BYTE) {
            out[i] = ((const GLubyte *)ptr)[i] / 255.0f;
        }
    }
    /* Fill remaining with defaults */
    for (int i = arr->size; i < default_count; i++) {
        out[i] = (i == 3) ? 1.0f : 0.0f;
    }
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    CHECK_CTX();
    if (count < 0) {
        gl_set_error(ctx, GL_INVALID_VALUE);
        return;
    }
    const void *vertex_base = get_array_pointer(&ctx->vertex_pointer);
    const void *color_base = (ctx->client_state & CLIENT_COLOR_ARRAY) ?
                             get_array_pointer(&ctx->color_pointer) : NULL;
    const void *texcoord_base = (ctx->client_state & CLIENT_TEXTURE_COORD_ARRAY) ?
                                get_array_pointer(&ctx->texcoord_pointer) : NULL;
    const void *normal_base = (ctx->client_state & CLIENT_NORMAL_ARRAY) ?
                              get_array_pointer(&ctx->normal_pointer) : NULL;

    if (!(ctx->client_state & CLIENT_VERTEX_ARRAY) || !vertex_base) {
        return;
    }

    glBegin(mode);
    for (GLsizei i = 0; i < count; i++) {
        GLint idx = first + i;
        float v[4], c[4], t[2], n[3];

        /* Get vertex position */
        get_array_element(&ctx->vertex_pointer, vertex_base, idx, v, 4);

        /* Get color if enabled */
        if (color_base) {
            get_array_element(&ctx->color_pointer, color_base, idx, c, 4);
            glColor4f(c[0], c[1], c[2], c[3]);
        }

        /* Get texcoord if enabled */
        if (texcoord_base) {
            get_array_element(&ctx->texcoord_pointer, texcoord_base, idx, t, 2);
            glTexCoord2f(t[0], t[1]);
        }

        /* Get normal if enabled */
        if (normal_base) {
            get_array_element(&ctx->normal_pointer, normal_base, idx, n, 3);
            glNormal3f(n[0], n[1], n[2]);
        }

        /* Emit vertex */
        if (ctx->vertex_pointer.size == 2) {
            glVertex2f(v[0], v[1]);
        } else {
            glVertex3f(v[0], v[1], v[2]);
        }
    }
    glEnd();
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
    CHECK_CTX();
    if (count < 0) {
        gl_set_error(ctx, GL_INVALID_VALUE);
        return;
    }

    /* Validate index type */
    if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }

    const void *vertex_base = get_array_pointer(&ctx->vertex_pointer);
    const void *color_base = (ctx->client_state & CLIENT_COLOR_ARRAY) ?
                             get_array_pointer(&ctx->color_pointer) : NULL;
    const void *texcoord_base = (ctx->client_state & CLIENT_TEXTURE_COORD_ARRAY) ?
                                get_array_pointer(&ctx->texcoord_pointer) : NULL;
    const void *normal_base = (ctx->client_state & CLIENT_NORMAL_ARRAY) ?
                              get_array_pointer(&ctx->normal_pointer) : NULL;

    /* Handle element array buffer */
    const void *index_data = indices;
    if (ctx->bound_element_buffer) {
        buffer_t *buf = buffer_get(&ctx->buffers, ctx->bound_element_buffer);
        if (buf && buf->data) {
            /* When a buffer is bound, indices is interpreted as a byte offset */
            uintptr_t offset = (uintptr_t)indices;
            if (offset >= buf->size) {
                gl_set_error(ctx, GL_INVALID_VALUE);
                return;
            }
            index_data = (const uint8_t *)buf->data + offset;
        } else {
            return;
        }
    }

    if (!(ctx->client_state & CLIENT_VERTEX_ARRAY) || !vertex_base || !index_data) {
        return;
    }

    glBegin(mode);
    for (GLsizei i = 0; i < count; i++) {
        GLuint idx;
        if (type == GL_UNSIGNED_SHORT) {
            idx = ((const GLushort *)index_data)[i];
        } else if (type == GL_UNSIGNED_INT) {
            idx = ((const GLuint *)index_data)[i];
        } else if (type == GL_UNSIGNED_BYTE) {
            idx = ((const GLubyte *)index_data)[i];
        } else {
            continue;
        }

        float v[4], c[4], t[2], n[3];

        /* Get vertex position */
        get_array_element(&ctx->vertex_pointer, vertex_base, idx, v, 4);

        /* Get color if enabled */
        if (color_base) {
            get_array_element(&ctx->color_pointer, color_base, idx, c, 4);
            glColor4f(c[0], c[1], c[2], c[3]);
        }

        /* Get texcoord if enabled */
        if (texcoord_base) {
            get_array_element(&ctx->texcoord_pointer, texcoord_base, idx, t, 2);
            glTexCoord2f(t[0], t[1]);
        }

        /* Get normal if enabled */
        if (normal_base) {
            get_array_element(&ctx->normal_pointer, normal_base, idx, n, 3);
            glNormal3f(n[0], n[1], n[2]);
        }

        /* Emit vertex */
        if (ctx->vertex_pointer.size == 2) {
            glVertex2f(v[0], v[1]);
        } else {
            glVertex3f(v[0], v[1], v[2]);
        }
    }
    glEnd();
}

/* Lighting functions */

void glLightfv(GLenum light, GLenum pname, const GLfloat *params)
{
    CHECK_CTX();
    if (list_record_lightfv(light, pname, params)) return;
    if (light < GL_LIGHT0 || light > GL_LIGHT7) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }
    int idx = light - GL_LIGHT0;
    light_t *l = &ctx->lights[idx];

    switch (pname) {
        case GL_AMBIENT:
            l->ambient = color(params[0], params[1], params[2], params[3]);
            break;
        case GL_DIFFUSE:
            l->diffuse = color(params[0], params[1], params[2], params[3]);
            break;
        case GL_SPECULAR:
            l->specular = color(params[0], params[1], params[2], params[3]);
            break;
        case GL_POSITION: {
            /* Transform position by current modelview matrix */
            vec4_t pos = vec4(params[0], params[1], params[2], params[3]);
            mat4_t mv = mat4_from_array(ctx->modelview_matrix[ctx->modelview_stack_depth]);
            l->position = mat4_mul_vec4(mv, pos);
            break;
        }
        case GL_SPOT_DIRECTION: {
            /* Transform direction by upper 3x3 of modelview */
            mat4_t mv = mat4_from_array(ctx->modelview_matrix[ctx->modelview_stack_depth]);
            vec4_t dir = mat4_mul_vec4(mv, vec4(params[0], params[1], params[2], 0.0f));
            l->spot_direction = vec3(dir.x, dir.y, dir.z);
            break;
        }
        case GL_SPOT_EXPONENT:
            l->spot_exponent = params[0];
            break;
        case GL_SPOT_CUTOFF:
            l->spot_cutoff = params[0];
            break;
        case GL_CONSTANT_ATTENUATION:
            l->constant_attenuation = params[0];
            break;
        case GL_LINEAR_ATTENUATION:
            l->linear_attenuation = params[0];
            break;
        case GL_QUADRATIC_ATTENUATION:
            l->quadratic_attenuation = params[0];
            break;
    }
}

void glLightf(GLenum light, GLenum pname, GLfloat param)
{
    CHECK_CTX();
    if (list_record_lightf(light, pname, param)) return;
    GLfloat params[4] = {param, param, param, param};
    glLightfv(light, pname, params);
}

void glLighti(GLenum light, GLenum pname, GLint param)
{
    glLightf(light, pname, (GLfloat)param);
}

void glLightiv(GLenum light, GLenum pname, const GLint *params)
{
    GLfloat fparams[4] = {(GLfloat)params[0], (GLfloat)params[1],
                          (GLfloat)params[2], (GLfloat)params[3]};
    glLightfv(light, pname, fparams);
}

void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
{
    CHECK_CTX();
    if (list_record_materialfv(face, pname, params)) return;

    /* Validate face parameter */
    if (face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }

    material_t *mat_front = (face == GL_FRONT || face == GL_FRONT_AND_BACK) ? &ctx->material_front : NULL;
    material_t *mat_back = (face == GL_BACK || face == GL_FRONT_AND_BACK) ? &ctx->material_back : NULL;

    color_t c = color(params[0], params[1], params[2], params[3]);

    switch (pname) {
        case GL_AMBIENT:
            if (mat_front) mat_front->ambient = c;
            if (mat_back)  mat_back->ambient = c;
            break;
        case GL_DIFFUSE:
            if (mat_front) mat_front->diffuse = c;
            if (mat_back)  mat_back->diffuse = c;
            break;
        case GL_SPECULAR:
            if (mat_front) mat_front->specular = c;
            if (mat_back)  mat_back->specular = c;
            break;
        case GL_EMISSION:
            if (mat_front) mat_front->emission = c;
            if (mat_back)  mat_back->emission = c;
            break;
        case GL_SHININESS:
            if (mat_front) mat_front->shininess = params[0];
            if (mat_back)  mat_back->shininess = params[0];
            break;
        case GL_AMBIENT_AND_DIFFUSE:
            if (mat_front) { mat_front->ambient = c; mat_front->diffuse = c; }
            if (mat_back)  { mat_back->ambient = c; mat_back->diffuse = c; }
            break;
        default:
            gl_set_error(ctx, GL_INVALID_ENUM);
            break;
    }
}

void glMaterialf(GLenum face, GLenum pname, GLfloat param)
{
    CHECK_CTX();
    if (list_record_materialf(face, pname, param)) return;
    GLfloat params[4] = {param, param, param, param};
    glMaterialfv(face, pname, params);
}

void glMateriali(GLenum face, GLenum pname, GLint param)
{
    glMaterialf(face, pname, (GLfloat)param);
}

void glMaterialiv(GLenum face, GLenum pname, const GLint *params)
{
    GLfloat fparams[4] = {(GLfloat)params[0], (GLfloat)params[1],
                          (GLfloat)params[2], (GLfloat)params[3]};
    glMaterialfv(face, pname, fparams);
}

void glLightModelfv(GLenum pname, const GLfloat *params)
{
    CHECK_CTX();
    switch (pname) {
        case GL_LIGHT_MODEL_AMBIENT:
            ctx->light_model_ambient = color(params[0], params[1], params[2], params[3]);
            break;
        case GL_LIGHT_MODEL_LOCAL_VIEWER:
            ctx->light_model_local_viewer = (GLboolean)(params[0] != 0.0f);
            break;
        case GL_LIGHT_MODEL_TWO_SIDE:
            ctx->light_model_two_side = (GLboolean)(params[0] != 0.0f);
            break;
    }
}

void glLightModelf(GLenum pname, GLfloat param)
{
    GLfloat params[4] = {param, param, param, param};
    glLightModelfv(pname, params);
}

void glLightModeli(GLenum pname, GLint param)
{
    glLightModelf(pname, (GLfloat)param);
}

void glLightModeliv(GLenum pname, const GLint *params)
{
    GLfloat fparams[4] = {(GLfloat)params[0], (GLfloat)params[1],
                          (GLfloat)params[2], (GLfloat)params[3]};
    glLightModelfv(pname, fparams);
}

void glColorMaterial(GLenum face, GLenum mode)
{
    CHECK_CTX();

    /* Validate face parameter */
    if (face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }

    /* Validate mode parameter */
    if (mode != GL_EMISSION && mode != GL_AMBIENT && mode != GL_DIFFUSE &&
        mode != GL_SPECULAR && mode != GL_AMBIENT_AND_DIFFUSE) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }

    ctx->color_material_face = face;
    ctx->color_material_mode = mode;
}

void glGetLightfv(GLenum light, GLenum pname, GLfloat *params)
{
    CHECK_CTX();
    if (!params) return;

    if (light < GL_LIGHT0 || light > GL_LIGHT7) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }

    int idx = light - GL_LIGHT0;
    light_t *l = &ctx->lights[idx];

    switch (pname) {
        case GL_AMBIENT:
            params[0] = l->ambient.r;
            params[1] = l->ambient.g;
            params[2] = l->ambient.b;
            params[3] = l->ambient.a;
            break;
        case GL_DIFFUSE:
            params[0] = l->diffuse.r;
            params[1] = l->diffuse.g;
            params[2] = l->diffuse.b;
            params[3] = l->diffuse.a;
            break;
        case GL_SPECULAR:
            params[0] = l->specular.r;
            params[1] = l->specular.g;
            params[2] = l->specular.b;
            params[3] = l->specular.a;
            break;
        case GL_POSITION:
            params[0] = l->position.x;
            params[1] = l->position.y;
            params[2] = l->position.z;
            params[3] = l->position.w;
            break;
        case GL_SPOT_DIRECTION:
            params[0] = l->spot_direction.x;
            params[1] = l->spot_direction.y;
            params[2] = l->spot_direction.z;
            break;
        case GL_SPOT_EXPONENT:
            params[0] = l->spot_exponent;
            break;
        case GL_SPOT_CUTOFF:
            params[0] = l->spot_cutoff;
            break;
        case GL_CONSTANT_ATTENUATION:
            params[0] = l->constant_attenuation;
            break;
        case GL_LINEAR_ATTENUATION:
            params[0] = l->linear_attenuation;
            break;
        case GL_QUADRATIC_ATTENUATION:
            params[0] = l->quadratic_attenuation;
            break;
        default:
            gl_set_error(ctx, GL_INVALID_ENUM);
            break;
    }
}

void glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params)
{
    CHECK_CTX();
    if (!params) return;

    material_t *mat;
    if (face == GL_FRONT) {
        mat = &ctx->material_front;
    } else if (face == GL_BACK) {
        mat = &ctx->material_back;
    } else {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }

    switch (pname) {
        case GL_AMBIENT:
            params[0] = mat->ambient.r;
            params[1] = mat->ambient.g;
            params[2] = mat->ambient.b;
            params[3] = mat->ambient.a;
            break;
        case GL_DIFFUSE:
            params[0] = mat->diffuse.r;
            params[1] = mat->diffuse.g;
            params[2] = mat->diffuse.b;
            params[3] = mat->diffuse.a;
            break;
        case GL_SPECULAR:
            params[0] = mat->specular.r;
            params[1] = mat->specular.g;
            params[2] = mat->specular.b;
            params[3] = mat->specular.a;
            break;
        case GL_EMISSION:
            params[0] = mat->emission.r;
            params[1] = mat->emission.g;
            params[2] = mat->emission.b;
            params[3] = mat->emission.a;
            break;
        case GL_SHININESS:
            params[0] = mat->shininess;
            break;
        default:
            gl_set_error(ctx, GL_INVALID_ENUM);
            break;
    }
}

void glShadeModel(GLenum mode)
{
    CHECK_CTX();
    if (list_record_shade_model(mode)) return;
    if (mode != GL_FLAT && mode != GL_SMOOTH && mode != GL_PHONG) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }
    ctx->shade_model = mode;
}

/* ============================================================
 * Display Lists
 * ============================================================ */

GLuint glGenLists(GLsizei range)
{
    CHECK_CTX_RET(0);
    if (range < 0) {
        gl_set_error(ctx, GL_INVALID_VALUE);
        return 0;
    }
    return list_alloc_range(&ctx->lists, range);
}

void glDeleteLists(GLuint list, GLsizei range)
{
    CHECK_CTX();
    if (range < 0) {
        gl_set_error(ctx, GL_INVALID_VALUE);
        return;
    }
    for (GLsizei i = 0; i < range; i++) {
        list_free(&ctx->lists, list + i);
    }
}

void glNewList(GLuint list, GLenum mode)
{
    CHECK_CTX();
    if (ctx->list_index != 0) {
        gl_set_error(ctx, GL_INVALID_OPERATION);
        return;
    }
    if (mode != GL_COMPILE && mode != GL_COMPILE_AND_EXECUTE) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }

    display_list_t *dlist = list_get(&ctx->lists, list);
    if (!dlist) {
        gl_set_error(ctx, GL_INVALID_VALUE);
        return;
    }

    list_clear(dlist);
    ctx->list_index = list;
    ctx->list_mode = mode;
}

void glEndList(void)
{
    CHECK_CTX();
    if (ctx->list_index == 0) {
        gl_set_error(ctx, GL_INVALID_OPERATION);
        return;
    }

    display_list_t *list = list_get(&ctx->lists, ctx->list_index);
    if (list) {
        list->valid = GL_TRUE;
    }

    ctx->list_index = 0;
    ctx->list_mode = 0;
}

/* Execute a single display list */
static void execute_list(GLuint list_id)
{
    display_list_t *list = list_get(&ctx->lists, list_id);
    if (!list || !list->valid) return;

    for (size_t i = 0; i < list->count; i++) {
        list_command_t *cmd = &list->commands[i];

        switch (cmd->opcode) {
            case CMD_BEGIN:
                glBegin(cmd->data.begin.mode);
                break;
            case CMD_END:
                glEnd();
                break;
            case CMD_VERTEX:
                glVertex3f(cmd->data.vertex.x, cmd->data.vertex.y, cmd->data.vertex.z);
                break;
            case CMD_COLOR:
                glColor4f(cmd->data.color.r, cmd->data.color.g, cmd->data.color.b, cmd->data.color.a);
                break;
            case CMD_TEXCOORD:
                glTexCoord2f(cmd->data.texcoord.s, cmd->data.texcoord.t);
                break;
            case CMD_NORMAL:
                glNormal3f(cmd->data.normal.x, cmd->data.normal.y, cmd->data.normal.z);
                break;
            case CMD_TRANSLATEF:
                glTranslatef(cmd->data.translatef.x, cmd->data.translatef.y, cmd->data.translatef.z);
                break;
            case CMD_ROTATEF:
                glRotatef(cmd->data.rotatef.angle, cmd->data.rotatef.x, cmd->data.rotatef.y, cmd->data.rotatef.z);
                break;
            case CMD_SCALEF:
                glScalef(cmd->data.scalef.x, cmd->data.scalef.y, cmd->data.scalef.z);
                break;
            case CMD_PUSH_MATRIX:
                glPushMatrix();
                break;
            case CMD_POP_MATRIX:
                glPopMatrix();
                break;
            case CMD_LOAD_IDENTITY:
                glLoadIdentity();
                break;
            case CMD_MULT_MATRIXF:
                glMultMatrixf(cmd->data.matrix.m);
                break;
            case CMD_LOAD_MATRIXF:
                glLoadMatrixf(cmd->data.matrix.m);
                break;
            case CMD_MATRIX_MODE:
                glMatrixMode(cmd->data.matrix_mode.mode);
                break;
            case CMD_ENABLE:
                glEnable(cmd->data.enable.cap);
                break;
            case CMD_DISABLE:
                glDisable(cmd->data.enable.cap);
                break;
            case CMD_BIND_TEXTURE:
                glBindTexture(cmd->data.bind_texture.target, cmd->data.bind_texture.texture);
                break;
            case CMD_BLEND_FUNC:
                glBlendFunc(cmd->data.blend_func.sfactor, cmd->data.blend_func.dfactor);
                break;
            case CMD_DEPTH_FUNC:
                glDepthFunc(cmd->data.depth_func.func);
                break;
            case CMD_DEPTH_MASK:
                glDepthMask(cmd->data.depth_mask.flag);
                break;
            case CMD_CULL_FACE:
                glCullFace(cmd->data.cull_face.mode);
                break;
            case CMD_FRONT_FACE:
                glFrontFace(cmd->data.front_face.mode);
                break;
            case CMD_SHADE_MODEL:
                glShadeModel(cmd->data.shade_model.mode);
                break;
            case CMD_LIGHTF:
                glLightf(cmd->data.lightf.light, cmd->data.lightf.pname, cmd->data.lightf.param);
                break;
            case CMD_LIGHTFV:
                glLightfv(cmd->data.lightfv.light, cmd->data.lightfv.pname, cmd->data.lightfv.params);
                break;
            case CMD_MATERIALF:
                glMaterialf(cmd->data.materialf.face, cmd->data.materialf.pname, cmd->data.materialf.param);
                break;
            case CMD_MATERIALFV:
                glMaterialfv(cmd->data.materialfv.face, cmd->data.materialfv.pname, cmd->data.materialfv.params);
                break;
            case CMD_CALL_LIST:
                glCallList(cmd->data.call_list.list);
                break;
            case CMD_ORTHO:
                glOrtho(cmd->data.ortho.left, cmd->data.ortho.right,
                        cmd->data.ortho.bottom, cmd->data.ortho.top,
                        cmd->data.ortho.near_val, cmd->data.ortho.far_val);
                break;
            case CMD_FRUSTUM:
                glFrustum(cmd->data.frustum.left, cmd->data.frustum.right,
                          cmd->data.frustum.bottom, cmd->data.frustum.top,
                          cmd->data.frustum.near_val, cmd->data.frustum.far_val);
                break;
        }
    }
}

void glCallList(GLuint list)
{
    CHECK_CTX();
    if (list_record_call_list(list)) return;

    /* Check recursion depth to prevent stack overflow */
    if (ctx->list_call_depth >= MAX_LIST_CALL_DEPTH) {
        gl_set_error(ctx, GL_STACK_OVERFLOW);
        return;
    }

    ctx->list_call_depth++;
    execute_list(list);
    ctx->list_call_depth--;
}

void glCallLists(GLsizei n, GLenum type, const GLvoid *lists)
{
    CHECK_CTX();
    if (n < 0) {
        gl_set_error(ctx, GL_INVALID_VALUE);
        return;
    }
    const GLubyte *p = (const GLubyte *)lists;

    for (GLsizei i = 0; i < n; i++) {
        GLuint offset = 0;

        switch (type) {
            case GL_UNSIGNED_BYTE:
                offset = p[i];
                break;
            case GL_UNSIGNED_SHORT:
                offset = ((const GLushort *)lists)[i];
                break;
            case GL_UNSIGNED_INT:
                offset = ((const GLuint *)lists)[i];
                break;
            default:
                gl_set_error(ctx, GL_INVALID_ENUM);
                return;
        }

        glCallList(ctx->list_base + offset);
    }
}

void glListBase(GLuint base)
{
    CHECK_CTX();
    ctx->list_base = base;
}

GLboolean glIsList(GLuint list)
{
    CHECK_CTX_RET(GL_FALSE);
    display_list_t *dlist = list_get(&ctx->lists, list);
    return (dlist && dlist->valid) ? GL_TRUE : GL_FALSE;
}

/* ============================================================
 * State Queries
 * ============================================================ */

void glGetIntegerv(GLenum pname, GLint *params)
{
    CHECK_CTX();
    if (!params) return;

    switch (pname) {
        /* Viewport and transformation */
        case GL_VIEWPORT:
            params[0] = ctx->viewport_x;
            params[1] = ctx->viewport_y;
            params[2] = ctx->viewport_w;
            params[3] = ctx->viewport_h;
            break;
        case GL_MATRIX_MODE:
            params[0] = ctx->matrix_mode;
            break;
        case GL_MODELVIEW_STACK_DEPTH:
            params[0] = ctx->modelview_stack_depth + 1;
            break;
        case GL_PROJECTION_STACK_DEPTH:
            params[0] = ctx->projection_stack_depth + 1;
            break;
        case GL_TEXTURE_STACK_DEPTH:
            params[0] = ctx->texture_stack_depth + 1;
            break;

        /* Coloring */
        case GL_SHADE_MODEL:
            params[0] = ctx->shade_model;
            break;
        case GL_COLOR_MATERIAL_FACE:
            params[0] = ctx->color_material_face;
            break;
        case GL_COLOR_MATERIAL_PARAMETER:
            params[0] = ctx->color_material_mode;
            break;

        /* Fog */
        case GL_FOG_MODE:
            params[0] = ctx->fog_mode;
            break;

        /* Lighting */
        case GL_LIGHT_MODEL_LOCAL_VIEWER:
            params[0] = ctx->light_model_local_viewer;
            break;
        case GL_LIGHT_MODEL_TWO_SIDE:
            params[0] = ctx->light_model_two_side;
            break;

        /* Rasterization */
        case GL_CULL_FACE_MODE:
            params[0] = ctx->cull_face_mode;
            break;
        case GL_FRONT_FACE:
            params[0] = ctx->front_face;
            break;
        case GL_POLYGON_MODE:
            params[0] = ctx->polygon_mode_front;
            params[1] = ctx->polygon_mode_back;
            break;

        /* Texturing */
        case GL_TEXTURE_BINDING_2D:
            params[0] = ctx->bound_texture_2d;
            break;
        case GL_TEXTURE_ENV_MODE:
            params[0] = ctx->tex_env_mode;
            break;

        /* Scissor */
        case GL_SCISSOR_BOX:
            params[0] = ctx->scissor_x;
            params[1] = ctx->scissor_y;
            params[2] = ctx->scissor_w;
            params[3] = ctx->scissor_h;
            break;

        /* Alpha test */
        case GL_ALPHA_TEST_FUNC:
            params[0] = ctx->alpha_func;
            break;

        /* Stencil */
        case GL_STENCIL_FUNC:
            params[0] = ctx->stencil_func;
            break;
        case GL_STENCIL_VALUE_MASK:
            params[0] = ctx->stencil_mask;
            break;
        case GL_STENCIL_REF:
            params[0] = ctx->stencil_ref;
            break;
        case GL_STENCIL_FAIL:
            params[0] = ctx->stencil_fail;
            break;
        case GL_STENCIL_PASS_DEPTH_FAIL:
            params[0] = ctx->stencil_zfail;
            break;
        case GL_STENCIL_PASS_DEPTH_PASS:
            params[0] = ctx->stencil_zpass;
            break;
        case GL_STENCIL_WRITEMASK:
            params[0] = ctx->stencil_writemask;
            break;
        case GL_STENCIL_CLEAR_VALUE:
            params[0] = ctx->stencil_clear;
            break;

        /* Depth */
        case GL_DEPTH_FUNC:
            params[0] = ctx->depth_func;
            break;
        case GL_DEPTH_WRITEMASK:
            params[0] = ctx->depth_mask;
            break;

        /* Blend */
        case GL_BLEND_SRC:
            params[0] = ctx->blend_src;
            break;
        case GL_BLEND_DST:
            params[0] = ctx->blend_dst;
            break;

        /* Color mask */
        case GL_COLOR_WRITEMASK:
            params[0] = ctx->color_mask_r;
            params[1] = ctx->color_mask_g;
            params[2] = ctx->color_mask_b;
            params[3] = ctx->color_mask_a;
            break;

        /* Pixel storage */
        case GL_UNPACK_ALIGNMENT:
            params[0] = 4;  /* default */
            break;
        case GL_PACK_ALIGNMENT:
            params[0] = 4;  /* default */
            break;

        /* Hints */
        case GL_PERSPECTIVE_CORRECTION_HINT:
            params[0] = ctx->perspective_correction_hint;
            break;
        case GL_FOG_HINT:
        case GL_LINE_SMOOTH_HINT:
        case GL_POINT_SMOOTH_HINT:
        case GL_POLYGON_SMOOTH_HINT:
            params[0] = GL_DONT_CARE;
            break;

        /* Buffer objects */
        case GL_ARRAY_BUFFER_BINDING:
            params[0] = ctx->bound_array_buffer;
            break;
        case GL_ELEMENT_ARRAY_BUFFER_BINDING:
            params[0] = ctx->bound_element_buffer;
            break;

        /* Vertex arrays */
        case GL_VERTEX_ARRAY_SIZE:
            params[0] = ctx->vertex_pointer.size;
            break;
        case GL_VERTEX_ARRAY_TYPE:
            params[0] = ctx->vertex_pointer.type;
            break;
        case GL_VERTEX_ARRAY_STRIDE:
            params[0] = ctx->vertex_pointer.stride;
            break;
        case GL_COLOR_ARRAY_SIZE:
            params[0] = ctx->color_pointer.size;
            break;
        case GL_COLOR_ARRAY_TYPE:
            params[0] = ctx->color_pointer.type;
            break;
        case GL_COLOR_ARRAY_STRIDE:
            params[0] = ctx->color_pointer.stride;
            break;
        case GL_NORMAL_ARRAY_TYPE:
            params[0] = ctx->normal_pointer.type;
            break;
        case GL_NORMAL_ARRAY_STRIDE:
            params[0] = ctx->normal_pointer.stride;
            break;
        case GL_TEXTURE_COORD_ARRAY_SIZE:
            params[0] = ctx->texcoord_pointer.size;
            break;
        case GL_TEXTURE_COORD_ARRAY_TYPE:
            params[0] = ctx->texcoord_pointer.type;
            break;
        case GL_TEXTURE_COORD_ARRAY_STRIDE:
            params[0] = ctx->texcoord_pointer.stride;
            break;

        /* Display lists */
        case GL_LIST_BASE:
            params[0] = ctx->list_base;
            break;
        case GL_LIST_INDEX:
            params[0] = ctx->list_index;
            break;
        case GL_LIST_MODE:
            params[0] = ctx->list_index ? ctx->list_mode : 0;
            break;

        /* Raster position */
        case GL_CURRENT_RASTER_POSITION:
            params[0] = ctx->raster_pos_x;
            params[1] = ctx->raster_pos_y;
            params[2] = 0;
            params[3] = 1;
            break;
        case GL_CURRENT_RASTER_POSITION_VALID:
            params[0] = ctx->raster_pos_valid;
            break;

        /* Render mode */
        case GL_RENDER_MODE:
            params[0] = 0x1C00;  /* GL_RENDER = 0x1C00 */
            break;

        /* Implementation limits */
        case GL_MAX_LIGHTS:
            params[0] = MAX_LIGHTS;
            break;
        case GL_MAX_CLIP_PLANES:
            params[0] = 6;
            break;
        case GL_MAX_TEXTURE_SIZE:
            params[0] = 2048;
            break;
        case GL_MAX_3D_TEXTURE_SIZE:
            params[0] = 0;  /* not supported */
            break;
        case GL_MAX_CUBE_MAP_TEXTURE_SIZE:
            params[0] = 0;  /* not supported */
            break;
        case GL_MAX_PIXEL_MAP_TABLE:
            params[0] = 256;
            break;
        case GL_MAX_ATTRIB_STACK_DEPTH:
        case GL_MAX_CLIENT_ATTRIB_STACK_DEPTH:
            params[0] = 16;
            break;
        case GL_MAX_MODELVIEW_STACK_DEPTH:
        case GL_MAX_PROJECTION_STACK_DEPTH:
        case GL_MAX_TEXTURE_STACK_DEPTH:
            params[0] = MAX_MATRIX_STACK_DEPTH;
            break;
        case GL_MAX_NAME_STACK_DEPTH:
            params[0] = 64;
            break;
        case GL_MAX_VIEWPORT_DIMS:
            params[0] = 16384;
            params[1] = 16384;
            break;
        case GL_MAX_TEXTURE_UNITS:
            params[0] = 1;
            break;
        case GL_MAX_ELEMENTS_VERTICES:
            params[0] = 65536;
            break;
        case GL_MAX_ELEMENTS_INDICES:
            params[0] = 65536;
            break;

        /* Framebuffer characteristics */
        case GL_SUBPIXEL_BITS:
            params[0] = 4;
            break;
        case GL_INDEX_BITS:
            params[0] = 0;  /* no color index mode */
            break;
        case GL_RED_BITS:
        case GL_GREEN_BITS:
        case GL_BLUE_BITS:
        case GL_ALPHA_BITS:
            params[0] = 8;
            break;
        case GL_DEPTH_BITS:
            params[0] = 32;  /* float depth buffer */
            break;
        case GL_STENCIL_BITS:
            params[0] = 8;
            break;
        case GL_ACCUM_RED_BITS:
        case GL_ACCUM_GREEN_BITS:
        case GL_ACCUM_BLUE_BITS:
        case GL_ACCUM_ALPHA_BITS:
            params[0] = 0;  /* no accumulation buffer */
            break;

        /* Miscellaneous */
        case GL_AUX_BUFFERS:
            params[0] = 0;
            break;
        case GL_DOUBLEBUFFER:
            params[0] = GL_TRUE;
            break;
        case GL_STEREO:
            params[0] = GL_FALSE;
            break;
        case GL_RGBA_MODE:
            params[0] = GL_TRUE;
            break;
        case GL_INDEX_MODE:
            params[0] = GL_FALSE;
            break;
        case GL_SAMPLE_BUFFERS:
            params[0] = 0;  /* no multisampling */
            break;
        case GL_SAMPLES:
            params[0] = 0;
            break;

        default:
            gl_set_error(ctx, GL_INVALID_ENUM);
            break;
    }
}

void glGetFloatv(GLenum pname, GLfloat *params)
{
    CHECK_CTX();
    if (!params) return;

    switch (pname) {
        /* Matrices */
        case GL_MODELVIEW_MATRIX:
            for (int i = 0; i < 16; i++) {
                params[i] = ctx->modelview_matrix[ctx->modelview_stack_depth][i];
            }
            break;
        case GL_PROJECTION_MATRIX:
            for (int i = 0; i < 16; i++) {
                params[i] = ctx->projection_matrix[ctx->projection_stack_depth][i];
            }
            break;
        case GL_TEXTURE_MATRIX:
            for (int i = 0; i < 16; i++) {
                params[i] = ctx->texture_matrix[ctx->texture_stack_depth][i];
            }
            break;

        /* Current values */
        case GL_CURRENT_COLOR:
            params[0] = ctx->current_color.r;
            params[1] = ctx->current_color.g;
            params[2] = ctx->current_color.b;
            params[3] = ctx->current_color.a;
            break;
        case GL_CURRENT_NORMAL:
            params[0] = ctx->current_normal.x;
            params[1] = ctx->current_normal.y;
            params[2] = ctx->current_normal.z;
            break;
        case GL_CURRENT_TEXTURE_COORDS:
            params[0] = ctx->current_texcoord.x;
            params[1] = ctx->current_texcoord.y;
            break;
        case GL_CURRENT_RASTER_COLOR:
            params[0] = ctx->current_color.r;
            params[1] = ctx->current_color.g;
            params[2] = ctx->current_color.b;
            params[3] = ctx->current_color.a;
            break;
        case GL_CURRENT_RASTER_POSITION:
            params[0] = (GLfloat)ctx->raster_pos_x;
            params[1] = (GLfloat)ctx->raster_pos_y;
            params[2] = 0.0f;
            params[3] = 1.0f;
            break;

        /* Viewport and depth */
        case GL_DEPTH_RANGE:
            params[0] = (GLfloat)ctx->depth_near;
            params[1] = (GLfloat)ctx->depth_far;
            break;
        case GL_VIEWPORT:
            params[0] = (GLfloat)ctx->viewport_x;
            params[1] = (GLfloat)ctx->viewport_y;
            params[2] = (GLfloat)ctx->viewport_w;
            params[3] = (GLfloat)ctx->viewport_h;
            break;
        case GL_DEPTH_CLEAR_VALUE:
            params[0] = (GLfloat)ctx->clear_depth;
            break;

        /* Color */
        case GL_COLOR_CLEAR_VALUE:
            params[0] = ctx->clear_color.r;
            params[1] = ctx->clear_color.g;
            params[2] = ctx->clear_color.b;
            params[3] = ctx->clear_color.a;
            break;

        /* Fog */
        case GL_FOG_COLOR:
            params[0] = ctx->fog_color.r;
            params[1] = ctx->fog_color.g;
            params[2] = ctx->fog_color.b;
            params[3] = ctx->fog_color.a;
            break;
        case GL_FOG_DENSITY:
            params[0] = ctx->fog_density;
            break;
        case GL_FOG_START:
            params[0] = ctx->fog_start;
            break;
        case GL_FOG_END:
            params[0] = ctx->fog_end;
            break;

        /* Light model */
        case GL_LIGHT_MODEL_AMBIENT:
            params[0] = ctx->light_model_ambient.r;
            params[1] = ctx->light_model_ambient.g;
            params[2] = ctx->light_model_ambient.b;
            params[3] = ctx->light_model_ambient.a;
            break;

        /* Alpha test */
        case GL_ALPHA_TEST_REF:
            params[0] = ctx->alpha_ref;
            break;

        /* Blend */
        case GL_BLEND_COLOR:
            params[0] = 0.0f;
            params[1] = 0.0f;
            params[2] = 0.0f;
            params[3] = 0.0f;
            break;

        /* Rasterization */
        case GL_POINT_SIZE:
            params[0] = ctx->point_size;
            break;
        case GL_POINT_SIZE_RANGE:
            params[0] = 1.0f;
            params[1] = 64.0f;
            break;
        case GL_POINT_SIZE_GRANULARITY:
            params[0] = 1.0f;
            break;
        case GL_LINE_WIDTH:
            params[0] = ctx->line_width;
            break;
        case GL_LINE_WIDTH_RANGE:
            params[0] = 1.0f;
            params[1] = 16.0f;
            break;
        case GL_LINE_WIDTH_GRANULARITY:
            params[0] = 1.0f;
            break;
        case GL_POLYGON_OFFSET_FACTOR:
            params[0] = 0.0f;  /* not implemented */
            break;
        case GL_POLYGON_OFFSET_UNITS:
            params[0] = 0.0f;
            break;

        /* Texture env */
        case GL_TEXTURE_ENV_COLOR:
            params[0] = ctx->tex_env_color.r;
            params[1] = ctx->tex_env_color.g;
            params[2] = ctx->tex_env_color.b;
            params[3] = ctx->tex_env_color.a;
            break;

        /* Scissor (as float) */
        case GL_SCISSOR_BOX:
            params[0] = (GLfloat)ctx->scissor_x;
            params[1] = (GLfloat)ctx->scissor_y;
            params[2] = (GLfloat)ctx->scissor_w;
            params[3] = (GLfloat)ctx->scissor_h;
            break;

        /* Implementation limits as float */
        case GL_MAX_TEXTURE_LOD_BIAS:
            params[0] = 2.0f;
            break;

        default: {
            /* Try integer query and convert */
            GLint iparams[4] = {0};
            GLenum saved_error = ctx->error;
            ctx->error = GL_NO_ERROR;
            glGetIntegerv(pname, iparams);
            if (ctx->error == GL_NO_ERROR) {
                params[0] = (GLfloat)iparams[0];
                ctx->error = saved_error;
            } else {
                ctx->error = saved_error;
                gl_set_error(ctx, GL_INVALID_ENUM);
            }
            break;
        }
    }
}

void glGetDoublev(GLenum pname, GLdouble *params)
{
    CHECK_CTX();
    if (!params) return;

    /* Most queries just convert from float */
    GLfloat fparams[16];
    glGetFloatv(pname, fparams);

    /* Determine how many values to copy based on pname */
    int count = 1;
    switch (pname) {
        case GL_MODELVIEW_MATRIX:
        case GL_PROJECTION_MATRIX:
        case GL_TEXTURE_MATRIX:
            count = 16;
            break;
        case GL_CURRENT_COLOR:
        case GL_CURRENT_RASTER_COLOR:
        case GL_CURRENT_RASTER_POSITION:
        case GL_VIEWPORT:
        case GL_SCISSOR_BOX:
        case GL_COLOR_CLEAR_VALUE:
        case GL_FOG_COLOR:
        case GL_LIGHT_MODEL_AMBIENT:
        case GL_BLEND_COLOR:
        case GL_TEXTURE_ENV_COLOR:
        case GL_COLOR_WRITEMASK:
            count = 4;
            break;
        case GL_CURRENT_NORMAL:
            count = 3;
            break;
        case GL_CURRENT_TEXTURE_COORDS:
        case GL_DEPTH_RANGE:
        case GL_POINT_SIZE_RANGE:
        case GL_LINE_WIDTH_RANGE:
        case GL_MAX_VIEWPORT_DIMS:
        case GL_POLYGON_MODE:
            count = 2;
            break;
    }

    for (int i = 0; i < count; i++) {
        params[i] = (GLdouble)fparams[i];
    }
}

void glGetBooleanv(GLenum pname, GLboolean *params)
{
    CHECK_CTX();
    if (!params) return;

    /* Handle enable/disable states directly */
    switch (pname) {
        case GL_DEPTH_TEST:
            params[0] = (ctx->flags & FLAG_DEPTH_TEST) ? GL_TRUE : GL_FALSE;
            return;
        case GL_CULL_FACE:
            params[0] = (ctx->flags & FLAG_CULL_FACE) ? GL_TRUE : GL_FALSE;
            return;
        case GL_BLEND:
            params[0] = (ctx->flags & FLAG_BLEND) ? GL_TRUE : GL_FALSE;
            return;
        case GL_TEXTURE_2D:
            params[0] = (ctx->flags & FLAG_TEXTURE_2D) ? GL_TRUE : GL_FALSE;
            return;
        case GL_LIGHTING:
            params[0] = (ctx->flags & FLAG_LIGHTING) ? GL_TRUE : GL_FALSE;
            return;
        case GL_FOG:
            params[0] = (ctx->flags & FLAG_FOG) ? GL_TRUE : GL_FALSE;
            return;
        case GL_NORMALIZE:
            params[0] = (ctx->flags & FLAG_NORMALIZE) ? GL_TRUE : GL_FALSE;
            return;
        case GL_COLOR_MATERIAL:
            params[0] = (ctx->flags & FLAG_COLOR_MATERIAL) ? GL_TRUE : GL_FALSE;
            return;
        case GL_ALPHA_TEST:
            params[0] = (ctx->flags & FLAG_ALPHA_TEST) ? GL_TRUE : GL_FALSE;
            return;
        case GL_SCISSOR_TEST:
            params[0] = (ctx->flags & FLAG_SCISSOR_TEST) ? GL_TRUE : GL_FALSE;
            return;
        case GL_STENCIL_TEST:
            params[0] = (ctx->flags & FLAG_STENCIL_TEST) ? GL_TRUE : GL_FALSE;
            return;
        case GL_DEPTH_WRITEMASK:
            params[0] = ctx->depth_mask;
            return;
        case GL_COLOR_WRITEMASK:
            params[0] = ctx->color_mask_r;
            params[1] = ctx->color_mask_g;
            params[2] = ctx->color_mask_b;
            params[3] = ctx->color_mask_a;
            return;
        case GL_DOUBLEBUFFER:
            params[0] = GL_TRUE;
            return;
        case GL_STEREO:
            params[0] = GL_FALSE;
            return;
        case GL_RGBA_MODE:
            params[0] = GL_TRUE;
            return;
        case GL_INDEX_MODE:
            params[0] = GL_FALSE;
            return;
        case GL_CURRENT_RASTER_POSITION_VALID:
            params[0] = ctx->raster_pos_valid;
            return;
        case GL_LIGHT_MODEL_LOCAL_VIEWER:
            params[0] = ctx->light_model_local_viewer;
            return;
        case GL_LIGHT_MODEL_TWO_SIDE:
            params[0] = ctx->light_model_two_side;
            return;
        case GL_VERTEX_ARRAY:
            params[0] = (ctx->client_state & CLIENT_VERTEX_ARRAY) ? GL_TRUE : GL_FALSE;
            return;
        case GL_COLOR_ARRAY:
            params[0] = (ctx->client_state & CLIENT_COLOR_ARRAY) ? GL_TRUE : GL_FALSE;
            return;
        case GL_NORMAL_ARRAY:
            params[0] = (ctx->client_state & CLIENT_NORMAL_ARRAY) ? GL_TRUE : GL_FALSE;
            return;
        case GL_TEXTURE_COORD_ARRAY:
            params[0] = (ctx->client_state & CLIENT_TEXTURE_COORD_ARRAY) ? GL_TRUE : GL_FALSE;
            return;
    }

    /* Handle lights */
    if (pname >= GL_LIGHT0 && pname <= GL_LIGHT7) {
        params[0] = ctx->lights[pname - GL_LIGHT0].enabled;
        return;
    }

    /* For other values, convert from integer */
    GLint iparams[16];
    glGetIntegerv(pname, iparams);
    params[0] = (iparams[0] != 0) ? GL_TRUE : GL_FALSE;
}

GLboolean glIsEnabled(GLenum cap)
{
    CHECK_CTX_RET(GL_FALSE);

    switch (cap) {
        case GL_DEPTH_TEST:     return (ctx->flags & FLAG_DEPTH_TEST) ? GL_TRUE : GL_FALSE;
        case GL_CULL_FACE:      return (ctx->flags & FLAG_CULL_FACE) ? GL_TRUE : GL_FALSE;
        case GL_BLEND:          return (ctx->flags & FLAG_BLEND) ? GL_TRUE : GL_FALSE;
        case GL_TEXTURE_2D:     return (ctx->flags & FLAG_TEXTURE_2D) ? GL_TRUE : GL_FALSE;
        case GL_LIGHTING:       return (ctx->flags & FLAG_LIGHTING) ? GL_TRUE : GL_FALSE;
        case GL_FOG:            return (ctx->flags & FLAG_FOG) ? GL_TRUE : GL_FALSE;
        case GL_NORMALIZE:      return (ctx->flags & FLAG_NORMALIZE) ? GL_TRUE : GL_FALSE;
        case GL_COLOR_MATERIAL: return (ctx->flags & FLAG_COLOR_MATERIAL) ? GL_TRUE : GL_FALSE;
        case GL_ALPHA_TEST:     return (ctx->flags & FLAG_ALPHA_TEST) ? GL_TRUE : GL_FALSE;
        case GL_SCISSOR_TEST:   return (ctx->flags & FLAG_SCISSOR_TEST) ? GL_TRUE : GL_FALSE;
        case GL_STENCIL_TEST:   return (ctx->flags & FLAG_STENCIL_TEST) ? GL_TRUE : GL_FALSE;
        default:
            if (cap >= GL_LIGHT0 && cap <= GL_LIGHT7) {
                return ctx->lights[cap - GL_LIGHT0].enabled;
            }
            gl_set_error(ctx, GL_INVALID_ENUM);
            return GL_FALSE;
    }
}

const GLubyte *glGetString(GLenum name)
{
    CHECK_CTX_RET(NULL);

    switch (name) {
        case GL_VENDOR:     return (const GLubyte *)"zbufferoverflow";
        case GL_RENDERER:   return (const GLubyte *)"MyTinyGL Software Renderer";
        case GL_VERSION:    return (const GLubyte *)"1.5 MyTinyGL (github.com/zbufferoverflow/MyTinyGL)";
        case GL_EXTENSIONS: return (const GLubyte *)"";
        default:
            gl_set_error(ctx, GL_INVALID_ENUM);
            return NULL;
    }
}

/* ===== Texture Environment ===== */

void glTexEnvi(GLenum target, GLenum pname, GLint param)
{
    CHECK_CTX();

    if (target != GL_TEXTURE_ENV) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }

    if (pname != GL_TEXTURE_ENV_MODE) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }

    switch (param) {
        case GL_MODULATE:
        case GL_DECAL:
        case GL_REPLACE:
        case GL_BLEND:
        case GL_ADD:
            ctx->tex_env_mode = param;
            break;
        default:
            gl_set_error(ctx, GL_INVALID_ENUM);
            break;
    }
}

void glTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
    glTexEnvi(target, pname, (GLint)param);
}

void glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params)
{
    CHECK_CTX();
    if (!params) return;

    if (target != GL_TEXTURE_ENV) {
        gl_set_error(ctx, GL_INVALID_ENUM);
        return;
    }

    if (pname == GL_TEXTURE_ENV_MODE) {
        glTexEnvi(target, pname, (GLint)params[0]);
    } else if (pname == GL_TEXTURE_ENV_COLOR) {
        ctx->tex_env_color = color(params[0], params[1], params[2], params[3]);
    } else {
        gl_set_error(ctx, GL_INVALID_ENUM);
    }
}

/* ===== Object Queries ===== */

GLboolean glIsTexture(GLuint texture)
{
    CHECK_CTX_RET(GL_FALSE);

    if (texture == 0) return GL_FALSE;

    texture_t *tex = texture_get(&ctx->textures, texture);
    return (tex != NULL) ? GL_TRUE : GL_FALSE;
}

GLboolean glIsBuffer(GLuint buffer)
{
    CHECK_CTX_RET(GL_FALSE);

    if (buffer == 0) return GL_FALSE;

    buffer_t *buf = buffer_get(&ctx->buffers, buffer);
    return (buf != NULL) ? GL_TRUE : GL_FALSE;
}
