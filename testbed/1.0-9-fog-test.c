/*
 * 1.0-9-fog-test.c
 * Test OpenGL fog modes
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
    int fog_mode = 0;
    const char *mode_names[] = {"GL_LINEAR", "GL_EXP", "GL_EXP2"};
    GLenum modes[] = {GL_LINEAR, GL_EXP, GL_EXP2};
    float angle = 0.0f;

#ifdef USE_MYTINYGL
    if (mtgl_init("Fog Test - MyTinyGL", WINDOW_WIDTH, WINDOW_HEIGHT) < 0) {
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
        "Fog Test - System GL",
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
    glFrustum(-aspect * 0.1, aspect * 0.1, -0.1, 0.1, 0.1, 50.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* Gray fog color */
    GLfloat fog_color[] = {0.5f, 0.5f, 0.5f, 1.0f};
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);

    /* Enable depth testing */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    /* Setup fog - eye-space distances */
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_START, 2.0f);   /* Fog starts at 2 units */
    glFogf(GL_FOG_END, 25.0f);    /* Full fog at 25 units */
    glFogf(GL_FOG_DENSITY, 0.1f); /* For EXP/EXP2 modes */
    glFogfv(GL_FOG_COLOR, fog_color);

    printf("Press SPACE to cycle fog modes, ESC to quit\n");
    printf("Current mode: %s\n", mode_names[fog_mode]);

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
                    fog_mode = (fog_mode + 1) % 3;
                    glFogi(GL_FOG_MODE, modes[fog_mode]);
                    printf("Fog mode: %s\n", mode_names[fog_mode]);
                }
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();

        /* Draw a row of cubes at different distances */
        for (int i = 0; i < 8; i++) {
            float z = -2.0f - i * 3.0f;  /* z = -2, -5, -8, -11, ... */

            glPushMatrix();
            glTranslatef(0.0f, 0.0f, z);
            glRotatef(angle + i * 20.0f, 0.0f, 1.0f, 0.0f);

            /* Draw a colored cube */
            glBegin(GL_QUADS);
                /* Front - red */
                glColor3f(1.0f, 0.0f, 0.0f);
                glVertex3f(-0.5f, -0.5f,  0.5f);
                glVertex3f( 0.5f, -0.5f,  0.5f);
                glVertex3f( 0.5f,  0.5f,  0.5f);
                glVertex3f(-0.5f,  0.5f,  0.5f);
                /* Back - green */
                glColor3f(0.0f, 1.0f, 0.0f);
                glVertex3f( 0.5f, -0.5f, -0.5f);
                glVertex3f(-0.5f, -0.5f, -0.5f);
                glVertex3f(-0.5f,  0.5f, -0.5f);
                glVertex3f( 0.5f,  0.5f, -0.5f);
                /* Top - blue */
                glColor3f(0.0f, 0.0f, 1.0f);
                glVertex3f(-0.5f,  0.5f,  0.5f);
                glVertex3f( 0.5f,  0.5f,  0.5f);
                glVertex3f( 0.5f,  0.5f, -0.5f);
                glVertex3f(-0.5f,  0.5f, -0.5f);
                /* Bottom - yellow */
                glColor3f(1.0f, 1.0f, 0.0f);
                glVertex3f(-0.5f, -0.5f, -0.5f);
                glVertex3f( 0.5f, -0.5f, -0.5f);
                glVertex3f( 0.5f, -0.5f,  0.5f);
                glVertex3f(-0.5f, -0.5f,  0.5f);
                /* Right - magenta */
                glColor3f(1.0f, 0.0f, 1.0f);
                glVertex3f( 0.5f, -0.5f,  0.5f);
                glVertex3f( 0.5f, -0.5f, -0.5f);
                glVertex3f( 0.5f,  0.5f, -0.5f);
                glVertex3f( 0.5f,  0.5f,  0.5f);
                /* Left - cyan */
                glColor3f(0.0f, 1.0f, 1.0f);
                glVertex3f(-0.5f, -0.5f, -0.5f);
                glVertex3f(-0.5f, -0.5f,  0.5f);
                glVertex3f(-0.5f,  0.5f,  0.5f);
                glVertex3f(-0.5f,  0.5f, -0.5f);
            glEnd();

            glPopMatrix();
        }

        angle += 0.5f;

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
