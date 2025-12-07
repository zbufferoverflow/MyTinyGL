/*
 * 1.0-18-suzanne-test.c
 * Render Blender's Suzanne monkey head with lighting
 * Press SPACE to toggle wireframe mode
 * Press L to toggle lighting
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

#include "suzanne_data.h"

#define WINDOW_WIDTH  640
#define WINDOW_HEIGHT 480

static void draw_suzanne(void)
{
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < SUZANNE_FACE_COUNT; i++) {
        for (int j = 0; j < 3; j++) {
            int vi = suzanne_faces[i][j][0];
            int ni = suzanne_faces[i][j][1];
            glNormal3f(suzanne_normals[ni][0],
                       suzanne_normals[ni][1],
                       suzanne_normals[ni][2]);
            glVertex3f(suzanne_vertices[vi][0],
                       suzanne_vertices[vi][1],
                       suzanne_vertices[vi][2]);
        }
    }
    glEnd();
}

int main(int argc, char *argv[])
{
    int running = 1;
    SDL_Event event;
    float angle_y = 0.0f;
    float angle_x = 20.0f;
    int wireframe = 0;
    int lighting = 1;

#ifdef USE_MYTINYGL
    if (mtgl_init("Suzanne Test - MyTinyGL", WINDOW_WIDTH, WINDOW_HEIGHT) < 0) {
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
        "Suzanne Test - System GL",
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

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearColor(0.2f, 0.2f, 0.3f, 1.0f);

    /* Lighting setup */
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);

    /* Main light - warm white from upper right */
    GLfloat light0_pos[] = {3.0f, 3.0f, 3.0f, 0.0f};
    GLfloat light0_diffuse[] = {1.0f, 0.95f, 0.9f, 1.0f};
    GLfloat light0_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);

    /* Fill light - cool blue from lower left */
    GLfloat light1_pos[] = {-2.0f, -1.0f, 2.0f, 0.0f};
    GLfloat light1_diffuse[] = {0.3f, 0.4f, 0.5f, 1.0f};
    glLightfv(GL_LIGHT1, GL_POSITION, light1_pos);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);

    /* Ambient light */
    GLfloat ambient[] = {0.15f, 0.15f, 0.2f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);

    /* Material - orange/brown monkey */
    GLfloat mat_ambient[] = {0.3f, 0.2f, 0.1f, 1.0f};
    GLfloat mat_diffuse[] = {0.8f, 0.5f, 0.3f, 1.0f};
    GLfloat mat_specular[] = {0.4f, 0.4f, 0.4f, 1.0f};
    GLfloat mat_shininess = 32.0f;
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);

#ifdef USE_MYTINYGL
    glShadeModel(GL_PHONG);  /* Per-fragment lighting */
#else
    glShadeModel(GL_SMOOTH); /* System GL doesn't support GL_PHONG */
#endif

    printf("Suzanne test running\n");
    printf("SPACE: toggle wireframe\n");
    printf("L: toggle lighting\n");
    printf("ESC: exit\n");

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
                    wireframe = !wireframe;
                    if (wireframe) {
                        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                        printf("Wireframe mode\n");
                    } else {
                        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                        printf("Solid mode\n");
                    }
                }
                if (event.key.keysym.sym == SDLK_l) {
                    lighting = !lighting;
                    if (lighting) {
                        glEnable(GL_LIGHTING);
                        printf("Lighting ON\n");
                    } else {
                        glDisable(GL_LIGHTING);
                        glColor3f(0.8f, 0.5f, 0.3f);
                        printf("Lighting OFF\n");
                    }
                }
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();

        /* Position camera */
        glTranslatef(0.0f, 0.0f, -2.2f);
        glRotatef(angle_x, 1.0f, 0.0f, 0.0f);
        glRotatef(angle_y, 0.0f, 1.0f, 0.0f);

        draw_suzanne();

        angle_y += 0.5f;

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
