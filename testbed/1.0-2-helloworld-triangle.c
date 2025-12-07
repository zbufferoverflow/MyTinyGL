/*
 * 1.0-1-helloworld-triangle.c
 * Classic colorized triangle using SDL2 + OpenGL 1.x fixed function pipeline
 * Compiles with both system GL and MyTinyGL
 */

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef USE_MYTINYGL
    #include <mytinygl/sdl.h>
#else
    #include <GL/gl.h>
#endif

#define WINDOW_WIDTH  640
#define WINDOW_HEIGHT 480

int main(int argc, char *argv[])
{
    int running = 1;
    SDL_Event event;

#ifdef USE_MYTINYGL
    if (mtgl_init("Hello Triangle - MyTinyGL", WINDOW_WIDTH, WINDOW_HEIGHT) < 0) {
        fprintf(stderr, "mtgl_init failed\n");
        return 1;
    }
#else
    SDL_Window *window;
    SDL_GLContext context;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    window = SDL_CreateWindow(
        "Hello Triangle - System GL",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    context = SDL_GL_CreateContext(window);
    if (!context) {
        fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
#endif

    /* OpenGL setup */
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    /* Main loop */
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = 0;
            }
        }

        glClear(GL_COLOR_BUFFER_BIT);

        /* Draw colorized triangle */
        glBegin(GL_TRIANGLES);
            glColor3f(1.0f, 0.0f, 0.0f);  /* Red */
            glVertex2f(0.0f, 0.6f);

            glColor3f(0.0f, 1.0f, 0.0f);  /* Green */
            glVertex2f(-0.6f, -0.4f);

            glColor3f(0.0f, 0.0f, 1.0f);  /* Blue */
            glVertex2f(0.6f, -0.4f);
        glEnd();

#ifdef USE_MYTINYGL
        mtgl_swap();
#else
        SDL_GL_SwapWindow(window);
#endif
    }

#ifdef USE_MYTINYGL
    mtgl_destroy();
#else
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif

    return 0;
}
