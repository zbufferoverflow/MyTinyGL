/*
 * MyTinyGL - OpenGL 1.x Software Renderer
 * Copyright (c) 2025 zbufferoverflow (Eliezer Solinger)
 * https://github.com/zbufferoverflow/MyTinyGL
 * SPDX-License-Identifier: MIT
 *
 * allocation.h - Memory allocation wrappers
 */

#ifndef MYTINYGL_ALLOCATION_H
#define MYTINYGL_ALLOCATION_H

#include <stdlib.h>
#include <string.h>

static inline void *mtgl_alloc(size_t size) {
    return malloc(size);
}

static inline void *mtgl_calloc(size_t count, size_t size) {
    return calloc(count, size);
}

static inline void *mtgl_realloc(void *ptr, size_t size) {
    return realloc(ptr, size);
}

static inline void mtgl_free(void *ptr) {
    free(ptr);
}

#endif /* MYTINYGL_ALLOCATION_H */
