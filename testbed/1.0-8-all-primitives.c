/*
 * 1.0-8-all-primitives.c
 * Test all OpenGL 1.x primitive types
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

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600
#define PI 3.14159265358979f

int main(int argc, char *argv[])
{
    int running = 1;
    SDL_Event event;

#ifdef USE_MYTINYGL
    if (mtgl_init("All Primitives - MyTinyGL", WINDOW_WIDTH, WINDOW_HEIGHT) < 0) {
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
        "All Primitives - System GL",
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
    float aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
    glOrtho(-aspect, aspect, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0.15f, 0.15f, 0.2f, 1.0f);

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

        /* Row 1: Point and Line primitives */

        /* GL_POINTS - top left */
        glBegin(GL_POINTS);
            for (int i = 0; i < 20; i++) {
                float t = i / 19.0f;
                glColor3f(1.0f, t, 0.0f);
                glVertex2f(-1.2f + t * 0.3f, 0.75f + sinf(t * PI * 2) * 0.1f);
            }
        glEnd();

        /* GL_LINES - top center-left */
        glBegin(GL_LINES);
            glColor3f(1.0f, 0.0f, 0.0f);
            glVertex2f(-0.7f, 0.85f);
            glColor3f(0.0f, 1.0f, 0.0f);
            glVertex2f(-0.5f, 0.65f);

            glColor3f(0.0f, 1.0f, 0.0f);
            glVertex2f(-0.5f, 0.85f);
            glColor3f(0.0f, 0.0f, 1.0f);
            glVertex2f(-0.3f, 0.65f);
        glEnd();

        /* GL_LINE_STRIP - top center */
        glBegin(GL_LINE_STRIP);
            glColor3f(1.0f, 1.0f, 0.0f);
            glVertex2f(-0.1f, 0.65f);
            glColor3f(0.0f, 1.0f, 1.0f);
            glVertex2f(0.0f, 0.85f);
            glColor3f(1.0f, 0.0f, 1.0f);
            glVertex2f(0.1f, 0.65f);
            glColor3f(1.0f, 1.0f, 1.0f);
            glVertex2f(0.2f, 0.85f);
            glColor3f(0.5f, 0.5f, 1.0f);
            glVertex2f(0.3f, 0.65f);
        glEnd();

        /* GL_LINE_LOOP - top center-right (pentagon) */
        glBegin(GL_LINE_LOOP);
            for (int i = 0; i < 5; i++) {
                float angle = i * 2.0f * PI / 5.0f - PI / 2.0f;
                float t = i / 4.0f;
                glColor3f(1.0f - t, t, 0.5f);
                glVertex2f(0.6f + cosf(angle) * 0.12f, 0.75f + sinf(angle) * 0.12f);
            }
        glEnd();

        /* Row 2: Triangle primitives */

        /* GL_TRIANGLES - middle left */
        glBegin(GL_TRIANGLES);
            glColor3f(1.0f, 0.0f, 0.0f);
            glVertex2f(-1.2f, 0.4f);
            glColor3f(0.0f, 1.0f, 0.0f);
            glVertex2f(-1.0f, 0.4f);
            glColor3f(0.0f, 0.0f, 1.0f);
            glVertex2f(-1.1f, 0.15f);

            glColor3f(1.0f, 1.0f, 0.0f);
            glVertex2f(-0.9f, 0.4f);
            glColor3f(0.0f, 1.0f, 1.0f);
            glVertex2f(-0.7f, 0.4f);
            glColor3f(1.0f, 0.0f, 1.0f);
            glVertex2f(-0.8f, 0.15f);
        glEnd();

        /* GL_TRIANGLE_STRIP - middle center */
        glBegin(GL_TRIANGLE_STRIP);
            glColor3f(1.0f, 0.0f, 0.0f);
            glVertex2f(-0.3f, 0.4f);
            glColor3f(0.0f, 1.0f, 0.0f);
            glVertex2f(-0.3f, 0.15f);
            glColor3f(0.0f, 0.0f, 1.0f);
            glVertex2f(-0.1f, 0.4f);
            glColor3f(1.0f, 1.0f, 0.0f);
            glVertex2f(-0.1f, 0.15f);
            glColor3f(1.0f, 0.0f, 1.0f);
            glVertex2f(0.1f, 0.4f);
            glColor3f(0.0f, 1.0f, 1.0f);
            glVertex2f(0.1f, 0.15f);
            glColor3f(1.0f, 1.0f, 1.0f);
            glVertex2f(0.3f, 0.4f);
            glColor3f(0.5f, 0.5f, 0.5f);
            glVertex2f(0.3f, 0.15f);
        glEnd();

        /* GL_TRIANGLE_FAN - middle right (octagon) */
        glBegin(GL_TRIANGLE_FAN);
            glColor3f(1.0f, 1.0f, 1.0f);  /* Center */
            glVertex2f(0.7f, 0.275f);
            for (int i = 0; i <= 8; i++) {
                float angle = i * 2.0f * PI / 8.0f;
                float t = (i % 8) / 7.0f;
                glColor3f(1.0f - t * 0.5f, t, 0.5f + t * 0.5f);
                glVertex2f(0.7f + cosf(angle) * 0.15f, 0.275f + sinf(angle) * 0.12f);
            }
        glEnd();

        /* Row 3: Quad primitives */

        /* GL_QUADS - bottom left */
        glBegin(GL_QUADS);
            glColor3f(1.0f, 0.0f, 0.0f);
            glVertex2f(-1.2f, -0.1f);
            glColor3f(0.0f, 1.0f, 0.0f);
            glVertex2f(-1.0f, -0.1f);
            glColor3f(0.0f, 0.0f, 1.0f);
            glVertex2f(-1.0f, -0.35f);
            glColor3f(1.0f, 1.0f, 0.0f);
            glVertex2f(-1.2f, -0.35f);

            glColor3f(1.0f, 0.5f, 0.0f);
            glVertex2f(-0.9f, -0.1f);
            glColor3f(0.5f, 1.0f, 0.0f);
            glVertex2f(-0.7f, -0.1f);
            glColor3f(0.0f, 0.5f, 1.0f);
            glVertex2f(-0.7f, -0.35f);
            glColor3f(1.0f, 0.0f, 0.5f);
            glVertex2f(-0.9f, -0.35f);
        glEnd();

        /* GL_QUAD_STRIP - bottom center */
        glBegin(GL_QUAD_STRIP);
            glColor3f(1.0f, 0.0f, 0.0f);
            glVertex2f(-0.3f, -0.1f);
            glColor3f(0.8f, 0.0f, 0.0f);
            glVertex2f(-0.3f, -0.35f);
            glColor3f(0.0f, 1.0f, 0.0f);
            glVertex2f(-0.1f, -0.1f);
            glColor3f(0.0f, 0.8f, 0.0f);
            glVertex2f(-0.1f, -0.35f);
            glColor3f(0.0f, 0.0f, 1.0f);
            glVertex2f(0.1f, -0.1f);
            glColor3f(0.0f, 0.0f, 0.8f);
            glVertex2f(0.1f, -0.35f);
            glColor3f(1.0f, 1.0f, 0.0f);
            glVertex2f(0.3f, -0.1f);
            glColor3f(0.8f, 0.8f, 0.0f);
            glVertex2f(0.3f, -0.35f);
        glEnd();

        /* GL_POLYGON - bottom right (hexagon) */
        glBegin(GL_POLYGON);
            for (int i = 0; i < 6; i++) {
                float angle = i * 2.0f * PI / 6.0f - PI / 2.0f;
                float t = i / 5.0f;
                glColor3f(1.0f, t, 1.0f - t);
                glVertex2f(0.7f + cosf(angle) * 0.15f, -0.225f + sinf(angle) * 0.12f);
            }
        glEnd();

        /* Row 4: Labels (drawn with line primitives) */
        /* Simple visual separators */
        glBegin(GL_LINES);
            glColor3f(0.4f, 0.4f, 0.4f);
            /* Horizontal lines between rows */
            glVertex2f(-1.3f, 0.55f);
            glVertex2f(1.0f, 0.55f);
            glVertex2f(-1.3f, 0.05f);
            glVertex2f(1.0f, 0.05f);
            glVertex2f(-1.3f, -0.45f);
            glVertex2f(1.0f, -0.45f);
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
