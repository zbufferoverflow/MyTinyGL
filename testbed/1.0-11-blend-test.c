/*
 * 1.0-11-blend-test.c
 * Test alpha blending with overlapping transparent quads
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
    if (mtgl_init("Blend Test - MyTinyGL", WINDOW_WIDTH, WINDOW_HEIGHT) < 0) {
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

    window = SDL_CreateWindow(
        "Blend Test - System GL",
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
    glFrustum(-aspect * 0.1, aspect * 0.1, -0.1, 0.1, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0.2f, 0.2f, 0.3f, 1.0f);

    /* Enable depth testing */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    /* Enable blending */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    printf("Blend test running\n");
    printf("Press ESC to exit\n");

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

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glLoadIdentity();
        glTranslatef(0.0f, 0.0f, -5.0f);
        glRotatef(angle * 0.3f, 0.0f, 1.0f, 0.0f);

        /* Draw opaque background quad (white) */
        glDisable(GL_BLEND);
        glBegin(GL_QUADS);
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glVertex3f(-2.0f, -2.0f, -1.0f);
            glVertex3f( 2.0f, -2.0f, -1.0f);
            glVertex3f( 2.0f,  2.0f, -1.0f);
            glVertex3f(-2.0f,  2.0f, -1.0f);
        glEnd();
        glEnable(GL_BLEND);

        /* Draw transparent red quad */
        glBegin(GL_QUADS);
            glColor4f(1.0f, 0.0f, 0.0f, 0.5f);
            glVertex3f(-1.5f, -1.0f, 0.0f);
            glVertex3f( 0.5f, -1.0f, 0.0f);
            glVertex3f( 0.5f,  1.0f, 0.0f);
            glVertex3f(-1.5f,  1.0f, 0.0f);
        glEnd();

        /* Draw transparent green quad (overlapping) */
        glBegin(GL_QUADS);
            glColor4f(0.0f, 1.0f, 0.0f, 0.5f);
            glVertex3f(-0.5f, -1.0f, 0.2f);
            glVertex3f( 1.5f, -1.0f, 0.2f);
            glVertex3f( 1.5f,  1.0f, 0.2f);
            glVertex3f(-0.5f,  1.0f, 0.2f);
        glEnd();

        /* Draw transparent blue quad (overlapping both) */
        glBegin(GL_QUADS);
            glColor4f(0.0f, 0.0f, 1.0f, 0.5f);
            glVertex3f(-0.5f, -1.5f, 0.4f);
            glVertex3f( 0.5f, -1.5f, 0.4f);
            glVertex3f( 0.5f,  0.5f, 0.4f);
            glVertex3f(-0.5f,  0.5f, 0.4f);
        glEnd();

        /* Draw semi-transparent rotating triangles */
        glPushMatrix();
        glRotatef(angle, 0.0f, 0.0f, 1.0f);

        glBegin(GL_TRIANGLES);
            /* Yellow triangle */
            glColor4f(1.0f, 1.0f, 0.0f, 0.6f);
            glVertex3f(0.0f, 1.2f, 0.5f);
            glVertex3f(-1.0f, -0.5f, 0.5f);
            glVertex3f(1.0f, -0.5f, 0.5f);
        glEnd();

        glPopMatrix();

        /* Draw additive blend example (top-right) */
        glPushMatrix();
        glTranslatef(1.5f, 1.0f, 0.6f);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);  /* Additive blend */

        glBegin(GL_TRIANGLES);
            glColor4f(1.0f, 0.0f, 0.0f, 0.5f);
            glVertex3f(-0.3f, -0.3f, 0.0f);
            glVertex3f( 0.3f, -0.3f, 0.0f);
            glVertex3f( 0.0f,  0.3f, 0.0f);
        glEnd();

        glBegin(GL_TRIANGLES);
            glColor4f(0.0f, 1.0f, 0.0f, 0.5f);
            glVertex3f(-0.2f, -0.4f, 0.0f);
            glVertex3f( 0.4f, -0.1f, 0.0f);
            glVertex3f( 0.1f,  0.4f, 0.0f);
        glEnd();

        glBegin(GL_TRIANGLES);
            glColor4f(0.0f, 0.0f, 1.0f, 0.5f);
            glVertex3f(-0.4f, -0.1f, 0.0f);
            glVertex3f( 0.2f, -0.4f, 0.0f);
            glVertex3f(-0.1f,  0.4f, 0.0f);
        glEnd();

        glPopMatrix();

        /* Restore normal blending */
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        angle += 0.5f;
        if (angle >= 360.0f) angle -= 360.0f;

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
