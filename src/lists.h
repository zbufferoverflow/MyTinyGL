/*
 * MyTinyGL - OpenGL 1.x Software Renderer
 * Copyright (c) 2025 zbufferoverflow (Eliezer Solinger)
 * https://github.com/zbufferoverflow/MyTinyGL
 * SPDX-License-Identifier: MIT
 *
 * lists.h - Display list structures
 */

#ifndef MYTINYGL_LISTS_H
#define MYTINYGL_LISTS_H

#include "GL/gl.h"
#include <stdint.h>
#include <stddef.h>

/* Maximum number of display lists */
#define GL_MAX_LISTS 1024

/* Display list command opcodes */
typedef enum {
    CMD_END = 0,
    CMD_BEGIN,
    CMD_VERTEX,
    CMD_COLOR,
    CMD_TEXCOORD,
    CMD_NORMAL,
    CMD_TRANSLATEF,
    CMD_ROTATEF,
    CMD_SCALEF,
    CMD_PUSH_MATRIX,
    CMD_POP_MATRIX,
    CMD_LOAD_IDENTITY,
    CMD_MULT_MATRIXF,
    CMD_LOAD_MATRIXF,
    CMD_MATRIX_MODE,
    CMD_ORTHO,
    CMD_FRUSTUM,
    CMD_ENABLE,
    CMD_DISABLE,
    CMD_BIND_TEXTURE,
    CMD_BLEND_FUNC,
    CMD_DEPTH_FUNC,
    CMD_DEPTH_MASK,
    CMD_CULL_FACE,
    CMD_FRONT_FACE,
    CMD_SHADE_MODEL,
    CMD_LIGHTF,
    CMD_LIGHTFV,
    CMD_MATERIALF,
    CMD_MATERIALFV,
    CMD_CALL_LIST,
} list_opcode_t;

/* Display list command - variable size depending on opcode */
typedef struct {
    list_opcode_t opcode;
    union {
        struct { GLenum mode; } begin;
        struct { GLfloat x, y, z; } vertex;
        struct { GLfloat r, g, b, a; } color;
        struct { GLfloat s, t; } texcoord;
        struct { GLfloat x, y, z; } normal;
        struct { GLfloat x, y, z; } translatef;
        struct { GLfloat angle, x, y, z; } rotatef;
        struct { GLfloat x, y, z; } scalef;
        struct { GLfloat m[16]; } matrix;
        struct { GLenum mode; } matrix_mode;
        struct { GLdouble left, right, bottom, top, near_val, far_val; } ortho;
        struct { GLdouble left, right, bottom, top, near_val, far_val; } frustum;
        struct { GLenum cap; } enable;
        struct { GLenum target; GLuint texture; } bind_texture;
        struct { GLenum sfactor, dfactor; } blend_func;
        struct { GLenum func; } depth_func;
        struct { GLboolean flag; } depth_mask;
        struct { GLenum mode; } cull_face;
        struct { GLenum mode; } front_face;
        struct { GLenum mode; } shade_model;
        struct { GLenum light; GLenum pname; GLfloat param; } lightf;
        struct { GLenum light; GLenum pname; GLfloat params[4]; } lightfv;
        struct { GLenum face; GLenum pname; GLfloat param; } materialf;
        struct { GLenum face; GLenum pname; GLfloat params[4]; } materialfv;
        struct { GLuint list; } call_list;
    } data;
} list_command_t;

/* Display list */
typedef struct {
    list_command_t *commands;
    size_t count;
    size_t capacity;
    GLboolean valid;
    GLboolean allocated;  /* Whether this slot is in use */
} display_list_t;

/* Display list store */
typedef struct {
    display_list_t *lists;
    size_t count;
    size_t capacity;
} list_store_t;

/* Initialize list store */
int list_store_init(list_store_t *store);

/* Free all lists */
void list_store_free(list_store_t *store);

/* Allocate a range of list IDs, returns first ID or 0 on failure */
GLuint list_alloc_range(list_store_t *store, GLsizei range);

/* Get list by ID (1-based), returns NULL if invalid */
display_list_t *list_get(list_store_t *store, GLuint id);

/* Free a list by ID */
void list_free(list_store_t *store, GLuint id);

/* Clear a list (remove all commands but keep allocated) */
void list_clear(display_list_t *list);

/* Add a command to a list */
int list_add_command(display_list_t *list, const list_command_t *cmd);

/* Recording functions - return 1 if should skip execution */
int list_record_begin(GLenum mode);
int list_record_end(void);
int list_record_vertex(GLfloat x, GLfloat y, GLfloat z);
int list_record_color(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
int list_record_texcoord(GLfloat s, GLfloat t);
int list_record_normal(GLfloat x, GLfloat y, GLfloat z);
int list_record_call_list(GLuint list);

/* Matrix recording functions */
int list_record_translatef(GLfloat x, GLfloat y, GLfloat z);
int list_record_rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
int list_record_scalef(GLfloat x, GLfloat y, GLfloat z);
int list_record_push_matrix(void);
int list_record_pop_matrix(void);
int list_record_load_identity(void);
int list_record_mult_matrixf(const GLfloat *m);
int list_record_load_matrixf(const GLfloat *m);
int list_record_matrix_mode(GLenum mode);
int list_record_ortho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val);
int list_record_frustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val);

/* State recording functions */
int list_record_enable(GLenum cap);
int list_record_disable(GLenum cap);
int list_record_bind_texture(GLenum target, GLuint texture);
int list_record_blend_func(GLenum sfactor, GLenum dfactor);
int list_record_depth_func(GLenum func);
int list_record_depth_mask(GLboolean flag);
int list_record_cull_face(GLenum mode);
int list_record_front_face(GLenum mode);
int list_record_shade_model(GLenum mode);

/* Lighting recording functions */
int list_record_lightf(GLenum light, GLenum pname, GLfloat param);
int list_record_lightfv(GLenum light, GLenum pname, const GLfloat *params);
int list_record_materialf(GLenum face, GLenum pname, GLfloat param);
int list_record_materialfv(GLenum face, GLenum pname, const GLfloat *params);

#endif /* MYTINYGL_LISTS_H */
