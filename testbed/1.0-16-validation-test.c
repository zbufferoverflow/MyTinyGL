/*
 * 1.0-16-validation-test.c
 * Test input validation: NaN/Inf handling, color clamping, etc.
 * This test passes invalid values and checks the renderer doesn't crash
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

/* Generate NaN and Inf for testing */
static float get_nan(void) {
    return 0.0f / 0.0f;
}

static float get_inf(void) {
    return 1.0f / 0.0f;
}

int main(int argc, char *argv[])
{
    int running = 1;
    SDL_Event event;
    int test_phase = 0;
    int frame_count = 0;

#ifdef USE_MYTINYGL
    if (mtgl_init("Validation Test - MyTinyGL", WINDOW_WIDTH, WINDOW_HEIGHT) < 0) {
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
        "Validation Test - System GL",
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

    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);

    printf("Validation test running\n");
    printf("Testing input validation - renderer should not crash\n");
    printf("Press SPACE to advance through test phases\n");
    printf("Press ESC to exit\n\n");

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
                if (event.key.keysym.sym == SDLK_SPACE) {
                    test_phase++;
                    if (test_phase > 8) test_phase = 0;
                }
            }
        }

        glClear(GL_COLOR_BUFFER_BIT);

        switch (test_phase) {
            case 0:
                /* Test 1: Normal rendering (baseline) */
                if (frame_count == 0) printf("Phase 0: Normal rendering (baseline)\n");
                glColor3f(0.0f, 1.0f, 0.0f);
                glBegin(GL_TRIANGLES);
                    glVertex2f(-0.5f, -0.5f);
                    glVertex2f(0.5f, -0.5f);
                    glVertex2f(0.0f, 0.5f);
                glEnd();
                break;

            case 1:
                /* Test 2: Color values out of range */
                if (frame_count == 0) printf("Phase 1: Color out of range (clamping test)\n");
                glColor3f(2.0f, -1.0f, 0.5f);  /* Should clamp to 1,0,0.5 */
                glBegin(GL_TRIANGLES);
                    glVertex2f(-0.5f, -0.5f);
                    glVertex2f(0.5f, -0.5f);
                    glVertex2f(0.0f, 0.5f);
                glEnd();
                break;

            case 2:
                /* Test 3: NaN color values */
                if (frame_count == 0) printf("Phase 2: NaN color values\n");
                glColor3f(get_nan(), 0.5f, 0.5f);
                glBegin(GL_TRIANGLES);
                    glVertex2f(-0.5f, -0.5f);
                    glVertex2f(0.5f, -0.5f);
                    glVertex2f(0.0f, 0.5f);
                glEnd();
                break;

            case 3:
                /* Test 4: Inf vertex coordinates */
                if (frame_count == 0) printf("Phase 3: Inf vertex coordinates (should be clipped)\n");
                glColor3f(1.0f, 0.0f, 1.0f);
                glBegin(GL_TRIANGLES);
                    glVertex2f(get_inf(), -0.5f);
                    glVertex2f(0.5f, -0.5f);
                    glVertex2f(0.0f, 0.5f);
                glEnd();
                /* Draw valid triangle too */
                glColor3f(0.0f, 0.5f, 1.0f);
                glBegin(GL_TRIANGLES);
                    glVertex2f(-0.3f, -0.3f);
                    glVertex2f(0.3f, -0.3f);
                    glVertex2f(0.0f, 0.3f);
                glEnd();
                break;

            case 4:
                /* Test 5: NaN texture coordinates */
                if (frame_count == 0) printf("Phase 4: NaN texture coordinates\n");
                glColor3f(1.0f, 1.0f, 0.0f);
                glBegin(GL_TRIANGLES);
                    glTexCoord2f(get_nan(), 0.0f);
                    glVertex2f(-0.5f, -0.5f);
                    glTexCoord2f(1.0f, get_nan());
                    glVertex2f(0.5f, -0.5f);
                    glTexCoord2f(0.5f, 1.0f);
                    glVertex2f(0.0f, 0.5f);
                glEnd();
                break;

            case 5:
                /* Test 6: Invalid glLineWidth/glPointSize */
                if (frame_count == 0) printf("Phase 5: Invalid line width/point size\n");
                glLineWidth(get_nan());
                glPointSize(get_inf());
                glLineWidth(-5.0f);
                glPointSize(0.0f);
                /* Reset to valid */
                glLineWidth(1.0f);
                glPointSize(1.0f);
                glColor3f(0.0f, 1.0f, 1.0f);
                glBegin(GL_TRIANGLES);
                    glVertex2f(-0.5f, -0.5f);
                    glVertex2f(0.5f, -0.5f);
                    glVertex2f(0.0f, 0.5f);
                glEnd();
                break;

            case 6:
                /* Test 7: Very large coordinates */
                if (frame_count == 0) printf("Phase 6: Very large coordinates\n");
                glColor3f(1.0f, 0.5f, 0.0f);
                glBegin(GL_TRIANGLES);
                    glVertex2f(-1e10f, -0.5f);
                    glVertex2f(1e10f, -0.5f);
                    glVertex2f(0.0f, 1e10f);
                glEnd();
                /* Draw valid small triangle */
                glColor3f(0.5f, 1.0f, 0.5f);
                glBegin(GL_TRIANGLES);
                    glVertex2f(-0.2f, -0.2f);
                    glVertex2f(0.2f, -0.2f);
                    glVertex2f(0.0f, 0.2f);
                glEnd();
                break;

            case 7:
                /* Test 8: Degenerate triangles (zero area) */
                if (frame_count == 0) printf("Phase 7: Degenerate triangles (zero area)\n");
                glColor3f(1.0f, 0.0f, 0.0f);
                /* Collinear points */
                glBegin(GL_TRIANGLES);
                    glVertex2f(-0.5f, 0.0f);
                    glVertex2f(0.0f, 0.0f);
                    glVertex2f(0.5f, 0.0f);
                glEnd();
                /* Same point */
                glBegin(GL_TRIANGLES);
                    glVertex2f(0.0f, 0.0f);
                    glVertex2f(0.0f, 0.0f);
                    glVertex2f(0.0f, 0.0f);
                glEnd();
                /* Draw valid triangle as indicator */
                glColor3f(0.0f, 1.0f, 0.0f);
                glBegin(GL_TRIANGLES);
                    glVertex2f(-0.3f, -0.3f);
                    glVertex2f(0.3f, -0.3f);
                    glVertex2f(0.0f, 0.3f);
                glEnd();
                break;

            case 8:
                /* Test 9: Mixed valid/invalid in same primitive */
                if (frame_count == 0) printf("Phase 8: Mixed valid/invalid vertices\n");
                glBegin(GL_TRIANGLES);
                    glColor3f(1.0f, 0.0f, 0.0f);
                    glVertex2f(-0.8f, -0.5f);
                    glColor3f(get_nan(), 1.0f, 0.0f);
                    glVertex2f(-0.4f, -0.5f);
                    glColor3f(0.0f, 0.0f, 1.0f);
                    glVertex2f(-0.6f, 0.5f);
                glEnd();
                /* Valid triangle for comparison */
                glColor3f(0.0f, 1.0f, 0.0f);
                glBegin(GL_TRIANGLES);
                    glVertex2f(0.4f, -0.5f);
                    glVertex2f(0.8f, -0.5f);
                    glVertex2f(0.6f, 0.5f);
                glEnd();
                break;
        }

        frame_count++;

#ifdef USE_MYTINYGL
        mtgl_swap();
#else
        SDL_GL_SwapWindow(window);
#endif
    }

    printf("\nValidation test completed - renderer survived all tests!\n");

#ifdef USE_MYTINYGL
    mtgl_destroy();
#else
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif

    return 0;
}
