/*
 * MyTinyGL - OpenGL 1.x Fixed Function Pipeline
 * vbo.c - Vertex Buffer Objects (OpenGL 1.5)
 */

#include "vbo.h"
#include "allocation.h"
#include <string.h>

#define INITIAL_BUFFER_CAPACITY 16

void buffer_store_init(buffer_store_t *store)
{
    store->buffers = NULL;
    store->count = 0;
    store->capacity = 0;
}

void buffer_store_free(buffer_store_t *store)
{
    if (store->buffers) {
        for (size_t i = 0; i < store->count; i++) {
            if (store->buffers[i].data) {
                mtgl_free(store->buffers[i].data);
            }
        }
        mtgl_free(store->buffers);
        store->buffers = NULL;
    }
    store->count = 0;
    store->capacity = 0;
}

void buffer_gen(buffer_store_t *store, GLsizei n, GLuint *buffers)
{
    for (GLsizei i = 0; i < n; i++) {
        /* First, try to find a free slot (reuse deleted IDs) */
        GLuint found_id = 0;
        for (size_t j = 0; j < store->count; j++) {
            if (!store->buffers[j].allocated) {
                store->buffers[j].allocated = GL_TRUE;
                store->buffers[j].data = NULL;
                store->buffers[j].size = 0;
                store->buffers[j].usage = 0;
                found_id = (GLuint)(j + 1);  /* 1-based ID */
                break;
            }
        }

        if (found_id != 0) {
            buffers[i] = found_id;
            continue;
        }

        /* No free slot, need to add a new one */
        if (store->count >= GL_MAX_BUFFERS) {
            buffers[i] = 0;  /* Hard limit reached */
            continue;
        }

        /* Grow array if needed */
        if (store->buffers == NULL) {
            store->capacity = INITIAL_BUFFER_CAPACITY;
            store->buffers = mtgl_calloc(store->capacity, sizeof(buffer_t));
            if (!store->buffers) {
                buffers[i] = 0;
                continue;
            }
        } else if (store->count >= store->capacity) {
            size_t new_capacity = store->capacity * 2;
            if (new_capacity > GL_MAX_BUFFERS) {
                new_capacity = GL_MAX_BUFFERS;
            }
            buffer_t *new_buffers = mtgl_realloc(store->buffers, new_capacity * sizeof(buffer_t));
            if (!new_buffers) {
                buffers[i] = 0;
                continue;
            }
            memset(&new_buffers[store->capacity], 0, (new_capacity - store->capacity) * sizeof(buffer_t));
            store->buffers = new_buffers;
            store->capacity = new_capacity;
        }

        /* Initialize new buffer */
        store->buffers[store->count].allocated = GL_TRUE;
        store->buffers[store->count].data = NULL;
        store->buffers[store->count].size = 0;
        store->buffers[store->count].usage = 0;
        store->count++;
        buffers[i] = (GLuint)store->count;  /* 1-based ID */
    }
}

void buffer_delete(buffer_store_t *store, GLsizei n, const GLuint *buffers)
{
    for (GLsizei i = 0; i < n; i++) {
        GLuint id = buffers[i];
        if (id == 0 || id > store->count) continue;

        buffer_t *buf = &store->buffers[id - 1];
        if (!buf->allocated) continue;

        if (buf->data) {
            mtgl_free(buf->data);
            buf->data = NULL;
        }
        buf->size = 0;
        buf->allocated = GL_FALSE;
    }
}

buffer_t *buffer_get(buffer_store_t *store, GLuint id)
{
    if (id == 0 || id > store->count) return NULL;
    buffer_t *buf = &store->buffers[id - 1];
    if (!buf->allocated) return NULL;
    return buf;
}

void buffer_data(buffer_t *buf, GLsizeiptr size, const void *data, GLenum usage)
{
    if (!buf) return;

    buf->usage = usage;

    if (size <= 0) {
        /* Free existing data */
        if (buf->data) {
            mtgl_free(buf->data);
            buf->data = NULL;
        }
        buf->size = 0;
        return;
    }

    /* Allocate new buffer first (to avoid leak on failure) */
    void *new_data = mtgl_alloc(size);
    if (!new_data) return;  /* Keep old data on failure */

    /* Free old data after successful allocation */
    if (buf->data) {
        mtgl_free(buf->data);
    }

    buf->data = new_data;
    buf->size = size;

    if (data) {
        memcpy(buf->data, data, size);
    }
}

int buffer_sub_data(buffer_t *buf, GLintptr offset, GLsizeiptr size, const void *data)
{
    if (!buf || !buf->data || !data) return -1;
    if (offset < 0 || size < 0) return -1;
    if (offset + size > buf->size) return -1;

    memcpy((uint8_t *)buf->data + offset, data, size);
    return 0;
}
