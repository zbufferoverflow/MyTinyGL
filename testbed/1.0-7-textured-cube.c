/*
 * 1.0-7-textured-cube.c
 * Test texture mapping with a rotating cube
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

#define TEX_WIDTH  64
#define TEX_HEIGHT 64

/* Generate a checkerboard texture */
static void generate_checkerboard(uint8_t *pixels, int w, int h, int checker_size)
{
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int checker = ((x / checker_size) + (y / checker_size)) % 2;
            int idx = (y * w + x) * 3;
            if (checker) {
                pixels[idx + 0] = 255;  /* R */
                pixels[idx + 1] = 255;  /* G */
                pixels[idx + 2] = 255;  /* B */
            } else {
                pixels[idx + 0] = 50;   /* R */
                pixels[idx + 1] = 50;   /* G */
                pixels[idx + 2] = 200;  /* B */
            }
        }
    }
}

/* Draw a textured cube centered at origin with given size */
static void draw_cube(float size)
{
    float s = size / 2.0f;

    /* Front face */
    glBegin(GL_QUADS);
        glColor3f(1.0f, 1.0f, 1.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-s, -s,  s);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( s, -s,  s);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( s,  s,  s);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-s,  s,  s);
    glEnd();

    /* Back face */
    glBegin(GL_QUADS);
        glColor3f(1.0f, 1.0f, 1.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-s, -s, -s);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-s,  s, -s);
        glTexCoord2f(0.0f, 1.0f); glVertex3f( s,  s, -s);
        glTexCoord2f(0.0f, 0.0f); glVertex3f( s, -s, -s);
    glEnd();

    /* Top face */
    glBegin(GL_QUADS);
        glColor3f(1.0f, 1.0f, 1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-s,  s, -s);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-s,  s,  s);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( s,  s,  s);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( s,  s, -s);
    glEnd();

    /* Bottom face */
    glBegin(GL_QUADS);
        glColor3f(1.0f, 1.0f, 1.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-s, -s, -s);
        glTexCoord2f(0.0f, 1.0f); glVertex3f( s, -s, -s);
        glTexCoord2f(0.0f, 0.0f); glVertex3f( s, -s,  s);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-s, -s,  s);
    glEnd();

    /* Right face */
    glBegin(GL_QUADS);
        glColor3f(1.0f, 1.0f, 1.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( s, -s, -s);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( s,  s, -s);
        glTexCoord2f(0.0f, 1.0f); glVertex3f( s,  s,  s);
        glTexCoord2f(0.0f, 0.0f); glVertex3f( s, -s,  s);
    glEnd();

    /* Left face */
    glBegin(GL_QUADS);
        glColor3f(1.0f, 1.0f, 1.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-s, -s, -s);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-s, -s,  s);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-s,  s,  s);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-s,  s, -s);
    glEnd();
}

int main(int argc, char *argv[])
{
    int running = 1;
    SDL_Event event;
    float angle_x = 0.0f;
    float angle_y = 0.0f;
    GLuint texture;

#ifdef USE_MYTINYGL
    if (mtgl_init("Textured Cube - MyTinyGL", WINDOW_WIDTH, WINDOW_HEIGHT) < 0) {
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
        "Textured Cube - System GL",
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

    /* Enable depth testing and texturing */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_TEXTURE_2D);

    /* Enable perspective-correct texture mapping */
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    /* Enable backface culling */
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    /* Generate texture */
    uint8_t *tex_pixels = malloc(TEX_WIDTH * TEX_HEIGHT * 3);
    generate_checkerboard(tex_pixels, TEX_WIDTH, TEX_HEIGHT, 8);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEX_WIDTH, TEX_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, tex_pixels);

    free(tex_pixels);

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
        glTranslatef(0.0f, 0.0f, -4.0f);
        glRotatef(angle_x, 1.0f, 0.0f, 0.0f);
        glRotatef(angle_y, 0.0f, 1.0f, 0.0f);

        draw_cube(1.5f);

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

    glDeleteTextures(1, &texture);

#ifdef USE_MYTINYGL
    mtgl_destroy();
#else
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif

    return 0;
}
