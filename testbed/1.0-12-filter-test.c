/*
 * 1.0-12-filter-test.c
 * Test texture filtering: GL_NEAREST vs GL_LINEAR (bilinear)
 * Press SPACE to toggle between filtering modes
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

/* Create a simple 8x8 checkerboard texture */
static void create_checkerboard_texture(GLuint *tex_id)
{
    #define TEX_SIZE 8
    uint8_t pixels[TEX_SIZE * TEX_SIZE * 3];

    for (int y = 0; y < TEX_SIZE; y++) {
        for (int x = 0; x < TEX_SIZE; x++) {
            int idx = (y * TEX_SIZE + x) * 3;
            if ((x + y) % 2 == 0) {
                /* White */
                pixels[idx + 0] = 255;
                pixels[idx + 1] = 255;
                pixels[idx + 2] = 255;
            } else {
                /* Red */
                pixels[idx + 0] = 255;
                pixels[idx + 1] = 0;
                pixels[idx + 2] = 0;
            }
        }
    }

    glGenTextures(1, tex_id);
    glBindTexture(GL_TEXTURE_2D, *tex_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEX_SIZE, TEX_SIZE, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    #undef TEX_SIZE
}

int main(int argc, char *argv[])
{
    int running = 1;
    SDL_Event event;
    float angle = 0.0f;
    int use_linear = 0;  /* Start with GL_NEAREST */
    GLuint texture;

#ifdef USE_MYTINYGL
    if (mtgl_init("Filter Test - MyTinyGL", WINDOW_WIDTH, WINDOW_HEIGHT) < 0) {
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
        "Filter Test - System GL",
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

    /* Enable texturing */
    glEnable(GL_TEXTURE_2D);

    /* Create checkerboard texture */
    create_checkerboard_texture(&texture);

    /* Start with nearest filtering */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    printf("Texture filtering test running\n");
    printf("Press SPACE to toggle between GL_NEAREST and GL_LINEAR (bilinear)\n");
    printf("Press ESC to exit\n");
    printf("Current mode: GL_NEAREST\n");

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
                    use_linear = !use_linear;
                    if (use_linear) {
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        printf("Switched to GL_LINEAR (bilinear filtering)\n");
                    } else {
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                        printf("Switched to GL_NEAREST (no filtering)\n");
                    }
                }
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glLoadIdentity();
        glTranslatef(0.0f, 0.0f, -3.0f);
        glRotatef(angle * 0.3f, 1.0f, 0.0f, 0.0f);
        glRotatef(angle, 0.0f, 1.0f, 0.0f);

        glColor3f(1.0f, 1.0f, 1.0f);

        /* Draw a large quad with the texture (magnified) */
        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.5f, -1.5f, 0.0f);
            glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.5f, -1.5f, 0.0f);
            glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.5f,  1.5f, 0.0f);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.5f,  1.5f, 0.0f);
        glEnd();

        angle += 0.3f;
        if (angle >= 360.0f) angle -= 360.0f;

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
