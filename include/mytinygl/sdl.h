/*
 * MyTinyGL - OpenGL 1.x Software Renderer
 * Copyright (c) 2025 zbufferoverflow (Eliezer Solinger)
 * https://github.com/zbufferoverflow/MyTinyGL
 * SPDX-License-Identifier: MIT
 *
 * sdl.h - SDL2 integration helpers
 */

#ifndef MYTINYGL_SDL_H
#define MYTINYGL_SDL_H

#include <SDL2/SDL.h>
#include "../../src/mytinygl.h"

static SDL_Window *mtgl_window = NULL;
static SDL_Renderer *mtgl_renderer = NULL;
static SDL_Texture *mtgl_texture = NULL;
static GLState *mtgl_ctx = NULL;

static inline int mtgl_init(const char *title, int32_t width, int32_t height)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return -1;
    }

    mtgl_window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN
    );
    if (!mtgl_window) {
        SDL_Quit();
        return -1;
    }

    mtgl_renderer = SDL_CreateRenderer(mtgl_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!mtgl_renderer) {
        SDL_DestroyWindow(mtgl_window);
        SDL_Quit();
        return -1;
    }

    mtgl_texture = SDL_CreateTexture(
        mtgl_renderer,
        SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height
    );
    if (!mtgl_texture) {
        SDL_DestroyRenderer(mtgl_renderer);
        SDL_DestroyWindow(mtgl_window);
        SDL_Quit();
        return -1;
    }

    mtgl_ctx = gl_create_context(width, height);
    if (!mtgl_ctx) {
        SDL_DestroyTexture(mtgl_texture);
        SDL_DestroyRenderer(mtgl_renderer);
        SDL_DestroyWindow(mtgl_window);
        SDL_Quit();
        return -1;
    }

    gl_make_current(mtgl_ctx);
    return 0;
}

static inline void mtgl_swap(void)
{
    SDL_UpdateTexture(
        mtgl_texture,
        NULL,
        framebuffer_get_color_buffer(&mtgl_ctx->framebuffer),
        framebuffer_get_pitch(&mtgl_ctx->framebuffer)
    );
    SDL_RenderCopy(mtgl_renderer, mtgl_texture, NULL, NULL);
    SDL_RenderPresent(mtgl_renderer);
}

static inline void mtgl_destroy(void)
{
    gl_destroy_context(mtgl_ctx);
    SDL_DestroyTexture(mtgl_texture);
    SDL_DestroyRenderer(mtgl_renderer);
    SDL_DestroyWindow(mtgl_window);
    SDL_Quit();
}

#endif /* MYTINYGL_SDL_H */
