/*
 * 1.5-0-vbo-test.c
 * Test OpenGL 1.5 Vertex Buffer Objects
 * Compiles with both system GL and MyTinyGL
 */

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef USE_MYTINYGL
    #include <mytinygl/sdl.h>
#else
    #define GL_GLEXT_PROTOTYPES
    #include <GL/gl.h>
    #include <GL/glext.h>
#endif

#define WINDOW_WIDTH  640
#define WINDOW_HEIGHT 480

/* Interleaved vertex data: x, y, z, r, g, b */
static GLfloat cube_vertices[] = {
    /* Front face - red */
    -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
    /* Back face - green */
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
    /* Top face - blue */
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
    /* Bottom face - yellow */
    -0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
    /* Right face - magenta */
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
    /* Left face - cyan */
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
};

/* Simple triangle for glDrawArrays test */
static GLfloat triangle_vertices[] = {
    /* x, y, z, r, g, b */
    -0.8f, -0.8f, 0.0f,  1.0f, 0.0f, 0.0f,
     0.8f, -0.8f, 0.0f,  0.0f, 1.0f, 0.0f,
     0.0f,  0.8f, 0.0f,  0.0f, 0.0f, 1.0f,
};

int main(int argc, char *argv[])
{
    int running = 1;
    SDL_Event event;
    float angle = 0.0f;
    int mode = 0;  /* 0 = VBO cube, 1 = client arrays triangle */
    GLuint vbo = 0;

#ifdef USE_MYTINYGL
    if (mtgl_init("VBO Test - MyTinyGL", WINDOW_WIDTH, WINDOW_HEIGHT) < 0) {
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
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    window = SDL_CreateWindow(
        "VBO Test - System GL",
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
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);

    /* Create VBO and upload cube data */
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    printf("Press SPACE to toggle between VBO cube and client array triangle\n");
    printf("Press ESC to quit\n");
    printf("Mode: VBO Cube (glDrawArrays with VBO)\n");

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
                    mode = (mode + 1) % 2;
                    if (mode == 0) {
                        printf("Mode: VBO Cube (glDrawArrays with VBO)\n");
                    } else {
                        printf("Mode: Client Array Triangle (glDrawArrays without VBO)\n");
                    }
                }
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();

        if (mode == 0) {
            /* VBO mode - draw rotating cube */
            glTranslatef(0.0f, 0.0f, -3.0f);
            glRotatef(angle, 1.0f, 1.0f, 0.0f);

            /* Bind VBO and set up vertex arrays */
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_COLOR_ARRAY);

            /* Interleaved: 3 floats position, 3 floats color */
            GLsizei stride = 6 * sizeof(GLfloat);
            glVertexPointer(3, GL_FLOAT, stride, (void *)0);
            glColorPointer(3, GL_FLOAT, stride, (void *)(3 * sizeof(GLfloat)));

            /* Draw all 6 faces (24 vertices, 4 per face) */
            glDrawArrays(GL_QUADS, 0, 24);

            glDisableClientState(GL_VERTEX_ARRAY);
            glDisableClientState(GL_COLOR_ARRAY);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        } else {
            /* Client array mode - draw triangle without VBO */
            glTranslatef(0.0f, 0.0f, -2.0f);
            glRotatef(angle, 0.0f, 0.0f, 1.0f);

            /* Use client-side arrays (no VBO) */
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_COLOR_ARRAY);

            GLsizei stride = 6 * sizeof(GLfloat);
            glVertexPointer(3, GL_FLOAT, stride, triangle_vertices);
            glColorPointer(3, GL_FLOAT, stride, triangle_vertices + 3);

            glDrawArrays(GL_TRIANGLES, 0, 3);

            glDisableClientState(GL_VERTEX_ARRAY);
            glDisableClientState(GL_COLOR_ARRAY);
        }

        angle += 0.5f;

#ifdef USE_MYTINYGL
        mtgl_swap();
#else
        SDL_GL_SwapWindow(window);
#endif
    }

    /* Cleanup */
    glDeleteBuffers(1, &vbo);

#ifdef USE_MYTINYGL
    mtgl_destroy();
#else
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif

    return 0;
}
