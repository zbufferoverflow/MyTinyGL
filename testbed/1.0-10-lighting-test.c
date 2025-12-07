/*
 * 1.0-10-lighting-test.c
 * Test OpenGL lighting with a rotating lit sphere
 * Press SPACE to cycle between GL_FLAT, GL_SMOOTH (Gouraud), and GL_PHONG shading
 * Compiles with both system GL and MyTinyGL
 */

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef USE_MYTINYGL
    #include <mytinygl/sdl.h>
    /* GL_PHONG is a MyTinyGL extension */
    #ifndef GL_PHONG
    #define GL_PHONG 0x1D02
    #endif
#else
    #include <GL/gl.h>
    /* GL_PHONG not available in standard OpenGL, map to GL_SMOOTH */
    #define GL_PHONG GL_SMOOTH
#endif

#define WINDOW_WIDTH  640
#define WINDOW_HEIGHT 480
#define PI 3.14159265358979f

/* Draw a sphere using triangle strips */
static void draw_sphere(float radius, int slices, int stacks)
{
    for (int i = 0; i < stacks; i++) {
        float lat0 = PI * (-0.5f + (float)i / stacks);
        float lat1 = PI * (-0.5f + (float)(i + 1) / stacks);
        float z0 = sinf(lat0);
        float zr0 = cosf(lat0);
        float z1 = sinf(lat1);
        float zr1 = cosf(lat1);

        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j <= slices; j++) {
            float lng = 2 * PI * (float)j / slices;
            float x = cosf(lng);
            float y = sinf(lng);

            /* Lower vertex */
            glNormal3f(x * zr0, y * zr0, z0);
            glVertex3f(radius * x * zr0, radius * y * zr0, radius * z0);

            /* Upper vertex */
            glNormal3f(x * zr1, y * zr1, z1);
            glVertex3f(radius * x * zr1, radius * y * zr1, radius * z1);
        }
        glEnd();
    }
}

int main(int argc, char *argv[])
{
    int running = 1;
    SDL_Event event;
    float angle = 0.0f;
    int shade_mode = 1;  /* 0=FLAT, 1=SMOOTH (Gouraud), 2=PHONG */
    const char *mode_names[] = { "GL_FLAT (flat)", "GL_SMOOTH (Gouraud)", "GL_PHONG (per-fragment)" };
    GLenum modes[] = { GL_FLAT, GL_SMOOTH, GL_PHONG };

#ifdef USE_MYTINYGL
    if (mtgl_init("Lighting Test - MyTinyGL", WINDOW_WIDTH, WINDOW_HEIGHT) < 0) {
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
        "Lighting Test - System GL",
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

    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);

    /* Enable depth testing */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    /* Enable lighting */
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);

    /* Light properties */
    GLfloat light_ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    GLfloat light_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

    /* Material properties - red plastic-like material */
    GLfloat mat_ambient[] = { 0.1f, 0.0f, 0.0f, 1.0f };
    GLfloat mat_diffuse[] = { 0.8f, 0.1f, 0.1f, 1.0f };
    GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat mat_shininess[] = { 50.0f };

    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

    /* Start with smooth shading (Gouraud) */
    glShadeModel(GL_SMOOTH);

    printf("Lighting test running\n");
    printf("Press SPACE to cycle shading modes: FLAT -> SMOOTH (Gouraud) -> PHONG\n");
    printf("Press ESC to exit\n");
    printf("Current mode: %s\n", mode_names[shade_mode]);

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
                    shade_mode = (shade_mode + 1) % 3;
                    glShadeModel(modes[shade_mode]);
                    printf("Switched to %s\n", mode_names[shade_mode]);
                }
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glLoadIdentity();
        glTranslatef(0.0f, 0.0f, -4.0f);

        /* Set light position (rotates around the sphere) */
        GLfloat light_position[] = {
            3.0f * cosf(angle * PI / 180.0f),
            1.0f,
            3.0f * sinf(angle * PI / 180.0f),
            1.0f  /* Positional light */
        };
        glLightfv(GL_LIGHT0, GL_POSITION, light_position);

        /* Draw the lit sphere */
        draw_sphere(1.0f, 32, 16);

        /* Draw a small unlit indicator for the light position */
        glDisable(GL_LIGHTING);
        glPushMatrix();
        glTranslatef(light_position[0], light_position[1], light_position[2]);
        glColor3f(1.0f, 1.0f, 0.0f);
        glBegin(GL_TRIANGLES);
            glVertex3f(-0.05f, -0.05f, 0.0f);
            glVertex3f(0.05f, -0.05f, 0.0f);
            glVertex3f(0.0f, 0.05f, 0.0f);
        glEnd();
        glPopMatrix();
        glEnable(GL_LIGHTING);

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
