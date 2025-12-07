/*
 * MyTinyGL - OpenGL 1.x Fixed Function Pipeline
 * lists.c - Display list implementation
 */

#include "lists.h"
#include "mytinygl.h"
#include "allocation.h"

#define INITIAL_LIST_CAPACITY 16
#define INITIAL_COMMAND_CAPACITY 64

/* Initialize list store */
int list_store_init(list_store_t *store)
{
    store->lists = NULL;
    store->count = 0;
    store->capacity = 0;
    return 0;
}

/* Free all lists */
void list_store_free(list_store_t *store)
{
    if (store->lists) {
        for (size_t i = 0; i < store->count; i++) {
            if (store->lists[i].commands) {
                mtgl_free(store->lists[i].commands);
            }
        }
        mtgl_free(store->lists);
        store->lists = NULL;
    }
    store->count = 0;
    store->capacity = 0;
}

/* Find a contiguous range of free slots, returns first index or -1 if not found */
static int find_free_range(list_store_t *store, GLsizei range)
{
    if (store->count == 0) return -1;

    int consecutive = 0;
    int start = -1;

    for (size_t i = 0; i < store->count; i++) {
        if (!store->lists[i].allocated) {
            if (consecutive == 0) start = (int)i;
            consecutive++;
            if (consecutive >= range) return start;
        } else {
            consecutive = 0;
        }
    }
    return -1;
}

/* Allocate a range of list IDs, returns first ID or 0 on failure */
GLuint list_alloc_range(list_store_t *store, GLsizei range)
{
    if (range <= 0) return 0;

    /* First, try to find a contiguous free range (reuse deleted IDs) */
    int free_start = find_free_range(store, range);
    if (free_start >= 0) {
        for (GLsizei i = 0; i < range; i++) {
            display_list_t *list = &store->lists[free_start + i];
            list->commands = NULL;
            list->count = 0;
            list->capacity = 0;
            list->valid = GL_FALSE;
            list->allocated = GL_TRUE;
        }
        return (GLuint)(free_start + 1);  /* 1-based ID */
    }

    /* No free range found, need to add new slots */
    if (store->count + range > GL_MAX_LISTS) {
        return 0;  /* Hard limit reached */
    }

    /* Grow array if needed */
    if (store->lists == NULL) {
        store->capacity = INITIAL_LIST_CAPACITY;
        while (store->capacity < (size_t)range) {
            store->capacity *= 2;
        }
        store->lists = mtgl_calloc(store->capacity, sizeof(display_list_t));
        if (!store->lists) return 0;
    } else if (store->count + range > store->capacity) {
        size_t new_capacity = store->capacity * 2;
        while (new_capacity < store->count + range) {
            new_capacity *= 2;
        }
        if (new_capacity > GL_MAX_LISTS) {
            new_capacity = GL_MAX_LISTS;
        }
        display_list_t *new_lists = mtgl_realloc(store->lists, new_capacity * sizeof(display_list_t));
        if (!new_lists) return 0;  /* Keep old pointer valid on failure */
        /* Zero new entries */
        memset(&new_lists[store->capacity], 0, (new_capacity - store->capacity) * sizeof(display_list_t));
        store->lists = new_lists;
        store->capacity = new_capacity;
    }

    GLuint first_id = (GLuint)(store->count + 1); /* 1-based ID */

    /* Initialize new lists */
    for (GLsizei i = 0; i < range; i++) {
        display_list_t *list = &store->lists[store->count + i];
        list->commands = NULL;
        list->count = 0;
        list->capacity = 0;
        list->valid = GL_FALSE;
        list->allocated = GL_TRUE;
    }

    store->count += range;
    return first_id;
}

/* Get list by ID (1-based), returns NULL if invalid or not allocated */
display_list_t *list_get(list_store_t *store, GLuint id)
{
    if (id == 0 || id > store->count) return NULL;
    display_list_t *list = &store->lists[id - 1];
    if (!list->allocated) return NULL;
    return list;
}

/* Free a list by ID */
void list_free(list_store_t *store, GLuint id)
{
    if (id == 0 || id > store->count) return;
    display_list_t *list = &store->lists[id - 1];
    if (!list->allocated) return;

    if (list->commands) {
        mtgl_free(list->commands);
        list->commands = NULL;
    }
    list->count = 0;
    list->capacity = 0;
    list->valid = GL_FALSE;
    list->allocated = GL_FALSE;  /* Mark as free for reuse */
}

/* Clear a list (remove all commands but keep allocated) */
void list_clear(display_list_t *list)
{
    if (!list) return;
    list->count = 0;
    list->valid = GL_FALSE;
}

/* Add a command to a list */
int list_add_command(display_list_t *list, const list_command_t *cmd)
{
    if (!list || !cmd) return -1;

    /* Grow command array if needed */
    if (list->commands == NULL) {
        list->capacity = INITIAL_COMMAND_CAPACITY;
        list->commands = mtgl_alloc(list->capacity * sizeof(list_command_t));
        if (!list->commands) return -1;
    } else if (list->count >= list->capacity) {
        size_t new_capacity = list->capacity * 2;
        list_command_t *new_commands = mtgl_realloc(list->commands, new_capacity * sizeof(list_command_t));
        if (!new_commands) return -1;
        list->commands = new_commands;
        list->capacity = new_capacity;
    }

    list->commands[list->count++] = *cmd;
    return 0;
}

/*
 * Recording helper functions
 * These return 1 if execution should be skipped (GL_COMPILE mode)
 * Return 0 if should execute (not compiling, or GL_COMPILE_AND_EXECUTE)
 */

/* Helper: record command to current list */
static int record_cmd(const list_command_t *cmd)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;

    display_list_t *list = list_get(&ctx->lists, ctx->list_index);
    if (list) {
        list_add_command(list, cmd);
    }

    /* Return 1 to skip execution if GL_COMPILE only */
    return (ctx->list_mode == GL_COMPILE);
}

int list_record_begin(GLenum mode)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_BEGIN, .data.begin = { mode } });
}

int list_record_end(void)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_END });
}

int list_record_vertex(GLfloat x, GLfloat y, GLfloat z)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_VERTEX, .data.vertex = { x, y, z } });
}

int list_record_color(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_COLOR, .data.color = { r, g, b, a } });
}

int list_record_texcoord(GLfloat s, GLfloat t)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_TEXCOORD, .data.texcoord = { s, t } });
}

int list_record_normal(GLfloat x, GLfloat y, GLfloat z)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_NORMAL, .data.normal = { x, y, z } });
}

int list_record_call_list(GLuint list)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_CALL_LIST, .data.call_list = { list } });
}

/* Matrix commands */
int list_record_translatef(GLfloat x, GLfloat y, GLfloat z)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_TRANSLATEF, .data.translatef = { x, y, z } });
}

int list_record_rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_ROTATEF, .data.rotatef = { angle, x, y, z } });
}

int list_record_scalef(GLfloat x, GLfloat y, GLfloat z)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_SCALEF, .data.scalef = { x, y, z } });
}

int list_record_push_matrix(void)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_PUSH_MATRIX });
}

int list_record_pop_matrix(void)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_POP_MATRIX });
}

int list_record_load_identity(void)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_LOAD_IDENTITY });
}

int list_record_mult_matrixf(const GLfloat *m)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    list_command_t cmd = { CMD_MULT_MATRIXF };
    for (int i = 0; i < 16; i++) cmd.data.matrix.m[i] = m[i];
    return record_cmd(&cmd);
}

int list_record_load_matrixf(const GLfloat *m)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    list_command_t cmd = { CMD_LOAD_MATRIXF };
    for (int i = 0; i < 16; i++) cmd.data.matrix.m[i] = m[i];
    return record_cmd(&cmd);
}

int list_record_matrix_mode(GLenum mode)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_MATRIX_MODE, .data.matrix_mode = { mode } });
}

int list_record_ortho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_ORTHO, .data.ortho = { left, right, bottom, top, near_val, far_val } });
}

int list_record_frustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_FRUSTUM, .data.frustum = { left, right, bottom, top, near_val, far_val } });
}

/* State commands */
int list_record_enable(GLenum cap)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_ENABLE, .data.enable = { cap } });
}

int list_record_disable(GLenum cap)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_DISABLE, .data.enable = { cap } });
}

int list_record_bind_texture(GLenum target, GLuint texture)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_BIND_TEXTURE, .data.bind_texture = { target, texture } });
}

int list_record_blend_func(GLenum sfactor, GLenum dfactor)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_BLEND_FUNC, .data.blend_func = { sfactor, dfactor } });
}

int list_record_depth_func(GLenum func)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_DEPTH_FUNC, .data.depth_func = { func } });
}

int list_record_depth_mask(GLboolean flag)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_DEPTH_MASK, .data.depth_mask = { flag } });
}

int list_record_cull_face(GLenum mode)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_CULL_FACE, .data.cull_face = { mode } });
}

int list_record_front_face(GLenum mode)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_FRONT_FACE, .data.front_face = { mode } });
}

int list_record_shade_model(GLenum mode)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_SHADE_MODEL, .data.shade_model = { mode } });
}

/* Lighting commands */
int list_record_lightf(GLenum light, GLenum pname, GLfloat param)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_LIGHTF, .data.lightf = { light, pname, param } });
}

int list_record_lightfv(GLenum light, GLenum pname, const GLfloat *params)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    list_command_t cmd = { CMD_LIGHTFV, .data.lightfv = { light, pname } };
    for (int i = 0; i < 4; i++) cmd.data.lightfv.params[i] = params[i];
    return record_cmd(&cmd);
}

int list_record_materialf(GLenum face, GLenum pname, GLfloat param)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    return record_cmd(&(list_command_t){ CMD_MATERIALF, .data.materialf = { face, pname, param } });
}

int list_record_materialfv(GLenum face, GLenum pname, const GLfloat *params)
{
    GLState *ctx = gl_get_current_context();
    if (!ctx || ctx->list_index == 0) return 0;
    list_command_t cmd = { CMD_MATERIALFV, .data.materialfv = { face, pname } };
    for (int i = 0; i < 4; i++) cmd.data.materialfv.params[i] = params[i];
    return record_cmd(&cmd);
}
