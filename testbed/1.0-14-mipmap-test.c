/*
 * 1.0-14-mipmap-test.c
 * Test mipmap filtering with a floor plane receding into distance
 * Press SPACE to cycle through filter modes
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

/* Filter mode names */
static const char *filter_names[] = {
    "GL_NEAREST",
    "GL_LINEAR",
    "GL_NEAREST_MIPMAP_NEAREST",
    "GL_LINEAR_MIPMAP_NEAREST",
    "GL_NEAREST_MIPMAP_LINEAR",
    "GL_LINEAR_MIPMAP_LINEAR"
};

static GLenum filter_modes[] = {
    GL_NEAREST,
    GL_LINEAR,
    GL_NEAREST_MIPMAP_NEAREST,
    GL_LINEAR_MIPMAP_NEAREST,
    GL_NEAREST_MIPMAP_LINEAR,
    GL_LINEAR_MIPMAP_LINEAR
};

#define NUM_FILTERS 6

/* Create a checkerboard texture with mipmaps */
static void create_mipmap_texture(GLuint *tex_id)
{
    #define TEX_SIZE 64
    uint8_t pixels[TEX_SIZE * TEX_SIZE * 3];

    /* Generate checkerboard pattern */
    for (int y = 0; y < TEX_SIZE; y++) {
        for (int x = 0; x < TEX_SIZE; x++) {
            int idx = (y * TEX_SIZE + x) * 3;
            /* 4x4 checker pattern */
            if (((x / 4) + (y / 4)) % 2 == 0) {
                pixels[idx + 0] = 255;  /* White */
                pixels[idx + 1] = 255;
                pixels[idx + 2] = 255;
            } else {
                pixels[idx + 0] = 50;   /* Dark blue */
                pixels[idx + 1] = 50;
                pixels[idx + 2] = 200;
            }
        }
    }

    glGenTextures(1, tex_id);
    glBindTexture(GL_TEXTURE_2D, *tex_id);

    /* Upload base level */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEX_SIZE, TEX_SIZE, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    /* Generate mipmaps manually for system GL compatibility */
    #ifndef USE_MYTINYGL
    /* For system GL, generate mipmap levels */
    int size = TEX_SIZE / 2;
    int level = 1;
    uint8_t *mip_pixels = malloc(size * size * 3);
    uint8_t *prev = malloc(TEX_SIZE * TEX_SIZE * 3);
    memcpy(prev, pixels, TEX_SIZE * TEX_SIZE * 3);

    while (size >= 1) {
        int prev_size = size * 2;
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                int didx = (y * size + x) * 3;
                int sidx00 = ((y*2) * prev_size + (x*2)) * 3;
                int sidx10 = ((y*2) * prev_size + (x*2+1)) * 3;
                int sidx01 = ((y*2+1) * prev_size + (x*2)) * 3;
                int sidx11 = ((y*2+1) * prev_size + (x*2+1)) * 3;
                /* Box filter */
                mip_pixels[didx + 0] = (prev[sidx00+0] + prev[sidx10+0] + prev[sidx01+0] + prev[sidx11+0]) / 4;
                mip_pixels[didx + 1] = (prev[sidx00+1] + prev[sidx10+1] + prev[sidx01+1] + prev[sidx11+1]) / 4;
                mip_pixels[didx + 2] = (prev[sidx00+2] + prev[sidx10+2] + prev[sidx01+2] + prev[sidx11+2]) / 4;
            }
        }
        glTexImage2D(GL_TEXTURE_2D, level, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, mip_pixels);
        memcpy(prev, mip_pixels, size * size * 3);
        size /= 2;
        level++;
    }
    free(mip_pixels);
    free(prev);
    #endif

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    #undef TEX_SIZE
}

int main(int argc, char *argv[])
{
    int running = 1;
    SDL_Event event;
    int filter_idx = 2;  /* Start with GL_NEAREST_MIPMAP_NEAREST */
    GLuint texture;

#ifdef USE_MYTINYGL
    if (mtgl_init("Mipmap Test - MyTinyGL", WINDOW_WIDTH, WINDOW_HEIGHT) < 0) {
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
        "Mipmap Test - System GL",
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

    glClearColor(0.4f, 0.6f, 0.9f, 1.0f);  /* Sky blue */

    /* Enable depth testing */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    /* Enable texturing */
    glEnable(GL_TEXTURE_2D);

    /* Create texture with mipmaps */
    create_mipmap_texture(&texture);

    /* Set initial filter */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter_modes[filter_idx]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    printf("Mipmap test running\n");
    printf("Press SPACE to cycle through filter modes\n");
    printf("Press ESC to exit\n");
    printf("Current mode: %s\n", filter_names[filter_idx]);
    printf("\nLook for:\n");
    printf("- NEAREST: Sharp but aliased (shimmering) at distance\n");
    printf("- LINEAR: Smooth but still aliased at distance\n");
    printf("- MIPMAP modes: Reduced aliasing at distance\n");

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
                    filter_idx = (filter_idx + 1) % NUM_FILTERS;
                    glBindTexture(GL_TEXTURE_2D, texture);  /* Ensure texture is bound */
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter_modes[filter_idx]);
                    printf("Switched to %s (0x%04X)\n", filter_names[filter_idx], filter_modes[filter_idx]);
                }
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glLoadIdentity();

        /* Position camera looking down at floor */
        glRotatef(60.0f, 1.0f, 0.0f, 0.0f);  /* Tilt down */
        glTranslatef(0.0f, -2.0f, 0.0f);

        glColor3f(1.0f, 1.0f, 1.0f);

        /* Draw a large floor plane with tiled texture */
        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f);   glVertex3f(-20.0f, 0.0f, -20.0f);
            glTexCoord2f(20.0f, 0.0f);  glVertex3f( 20.0f, 0.0f, -20.0f);
            glTexCoord2f(20.0f, 20.0f); glVertex3f( 20.0f, 0.0f,  20.0f);
            glTexCoord2f(0.0f, 20.0f);  glVertex3f(-20.0f, 0.0f,  20.0f);
        glEnd();

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
