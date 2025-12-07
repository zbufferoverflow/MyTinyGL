/*
 * 1.0-13-displaylist-test.c
 * Test display lists by creating and calling a list that draws a cube
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

/* Create a display list that draws a colored cube */
static GLuint create_cube_list(float size)
{
    GLuint list = glGenLists(1);
    float s = size / 2.0f;

    glNewList(list, GL_COMPILE);

    /* Front face - red */
    glBegin(GL_QUADS);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(-s, -s,  s);
        glVertex3f( s, -s,  s);
        glVertex3f( s,  s,  s);
        glVertex3f(-s,  s,  s);
    glEnd();

    /* Back face - green */
    glBegin(GL_QUADS);
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(-s, -s, -s);
        glVertex3f(-s,  s, -s);
        glVertex3f( s,  s, -s);
        glVertex3f( s, -s, -s);
    glEnd();

    /* Top face - blue */
    glBegin(GL_QUADS);
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f(-s,  s, -s);
        glVertex3f(-s,  s,  s);
        glVertex3f( s,  s,  s);
        glVertex3f( s,  s, -s);
    glEnd();

    /* Bottom face - yellow */
    glBegin(GL_QUADS);
        glColor3f(1.0f, 1.0f, 0.0f);
        glVertex3f(-s, -s, -s);
        glVertex3f( s, -s, -s);
        glVertex3f( s, -s,  s);
        glVertex3f(-s, -s,  s);
    glEnd();

    /* Right face - magenta */
    glBegin(GL_QUADS);
        glColor3f(1.0f, 0.0f, 1.0f);
        glVertex3f( s, -s, -s);
        glVertex3f( s,  s, -s);
        glVertex3f( s,  s,  s);
        glVertex3f( s, -s,  s);
    glEnd();

    /* Left face - cyan */
    glBegin(GL_QUADS);
        glColor3f(0.0f, 1.0f, 1.0f);
        glVertex3f(-s, -s, -s);
        glVertex3f(-s, -s,  s);
        glVertex3f(-s,  s,  s);
        glVertex3f(-s,  s, -s);
    glEnd();

    glEndList();

    return list;
}

int main(int argc, char *argv[])
{
    int running = 1;
    SDL_Event event;
    float angle_x = 0.0f;
    float angle_y = 0.0f;
    GLuint cube_list;

#ifdef USE_MYTINYGL
    if (mtgl_init("Display List Test - MyTinyGL", WINDOW_WIDTH, WINDOW_HEIGHT) < 0) {
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
        "Display List Test - System GL",
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

    /* Enable depth testing and backface culling */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    /* Create the cube display list */
    cube_list = create_cube_list(1.5f);
    printf("Created display list %u\n", cube_list);
    printf("glIsList(%u) = %d\n", cube_list, glIsList(cube_list));

    printf("Display list test running\n");
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
        glRotatef(angle_x, 1.0f, 0.0f, 0.0f);
        glRotatef(angle_y, 0.0f, 1.0f, 0.0f);

        /* Call the display list to draw the cube */
        glCallList(cube_list);

        angle_x += 0.5f;
        angle_y += 0.7f;
        if (angle_x >= 360.0f) angle_x -= 360.0f;
        if (angle_y >= 360.0f) angle_y -= 360.0f;

#ifdef USE_MYTINYGL
        mtgl_swap();
#else
        SDL_GL_SwapWindow(window);
#endif
    }

    glDeleteLists(cube_list, 1);

#ifdef USE_MYTINYGL
    mtgl_destroy();
#else
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif

    return 0;
}
