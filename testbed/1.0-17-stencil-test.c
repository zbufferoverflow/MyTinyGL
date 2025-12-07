/*
 * 1.0-17-stencil-test.c
 * Test stencil buffer functions (stubs in MyTinyGL, functional in system GL)
 * This test verifies the stencil API doesn't crash, even if not implemented
 * Compiles with both system GL and MyTinyGL
 */

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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
    float angle = 0.0f;

#ifdef USE_MYTINYGL
    if (mtgl_init("Stencil Test - MyTinyGL", WINDOW_WIDTH, WINDOW_HEIGHT) < 0) {
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
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    window = SDL_CreateWindow(
        "Stencil Test - System GL",
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
    glOrtho(-1.5, 1.5, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0.2f, 0.2f, 0.3f, 1.0f);

    printf("Stencil test running\n");
    printf("Testing stencil API calls don't crash\n");
    printf("Press ESC to exit\n\n");

    /* Test stencil function calls - these should not crash */
    printf("Calling glStencilFunc...\n");
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    printf("Calling glStencilOp...\n");
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    printf("Calling glStencilMask...\n");
    glStencilMask(0xFF);
    printf("Calling glClearStencil...\n");
    glClearStencil(0);
    printf("All stencil API calls succeeded!\n\n");

    /* Main loop */
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = 0;
                }
            }
        }

        /* Clear buffers including stencil */
        glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        /* Enable stencil test */
        glEnable(GL_STENCIL_TEST);

        /* First pass: write to stencil buffer */
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

        /* Draw stencil shape (circle approximation) */
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(0.0f, 0.0f);
        for (int i = 0; i <= 32; i++) {
            float a = (float)i * 2.0f * 3.14159f / 32.0f;
            glVertex2f(0.5f * cosf(a), 0.5f * sinf(a));
        }
        glEnd();

        /* Second pass: render only where stencil == 1 */
        glStencilFunc(GL_EQUAL, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        glLoadIdentity();
        glRotatef(angle, 0.0f, 0.0f, 1.0f);

        /* Draw rotating square */
        glBegin(GL_QUADS);
            glColor3f(1.0f, 0.0f, 0.0f);
            glVertex2f(-0.8f, -0.8f);
            glColor3f(0.0f, 1.0f, 0.0f);
            glVertex2f(0.8f, -0.8f);
            glColor3f(0.0f, 0.0f, 1.0f);
            glVertex2f(0.8f, 0.8f);
            glColor3f(1.0f, 1.0f, 0.0f);
            glVertex2f(-0.8f, 0.8f);
        glEnd();

        glDisable(GL_STENCIL_TEST);

        angle += 0.5f;
        if (angle >= 360.0f) angle -= 360.0f;

#ifdef USE_MYTINYGL
        mtgl_swap();
#else
        SDL_GL_SwapWindow(window);
#endif
    }

    printf("Stencil test completed successfully!\n");

#ifdef USE_MYTINYGL
    mtgl_destroy();
#else
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif

    return 0;
}
