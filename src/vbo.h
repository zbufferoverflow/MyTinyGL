/*
 * MyTinyGL - OpenGL 1.x Software Renderer
 * Copyright (c) 2025 zbufferoverflow (Eliezer Solinger)
 * https://github.com/zbufferoverflow/MyTinyGL
 * SPDX-License-Identifier: MIT
 *
 * vbo.h - Vertex Buffer Objects (OpenGL 1.5)
 */

#ifndef MYTINYGL_VBO_H
#define MYTINYGL_VBO_H

#include "GL/gl.h"
#include <stdint.h>
#include <stddef.h>

#define GL_MAX_BUFFERS 256

/* Buffer object */
typedef struct {
    void *data;
    GLsizeiptr size;
    GLenum usage;
    GLboolean allocated;
} buffer_t;

/* Buffer store (dynamic array) */
typedef struct {
    buffer_t *buffers;
    size_t count;
    size_t capacity;
} buffer_store_t;

/* Initialize buffer store */
void buffer_store_init(buffer_store_t *store);

/* Free all buffers */
void buffer_store_free(buffer_store_t *store);

/* Allocate buffer IDs */
void buffer_gen(buffer_store_t *store, GLsizei n, GLuint *buffers);

/* Delete buffers */
void buffer_delete(buffer_store_t *store, GLsizei n, const GLuint *buffers);

/* Get buffer by ID (returns NULL if invalid) */
buffer_t *buffer_get(buffer_store_t *store, GLuint id);

/* Upload data to buffer */
void buffer_data(buffer_t *buf, GLsizeiptr size, const void *data, GLenum usage);

/* Update subset of buffer data. Returns 0 on success, -1 on invalid range */
int buffer_sub_data(buffer_t *buf, GLintptr offset, GLsizeiptr size, const void *data);

#endif /* MYTINYGL_VBO_H */
