/*
 * 1.0-15-texenv-test.c
 * Test texture environment modes: MODULATE, DECAL, REPLACE, BLEND, ADD
 * Press SPACE to cycle through texture environment modes
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

/* Texture environment mode names */
static const char *texenv_names[] = {
    "GL_MODULATE (texture * vertex color)",
    "GL_DECAL (blend by texture alpha)",
    "GL_REPLACE (texture only)",
    "GL_BLEND (blend with env color)",
    "GL_ADD (texture + vertex color)"
};

static GLenum texenv_modes[] = {
    GL_MODULATE,
    GL_DECAL,
    GL_REPLACE,
    GL_BLEND,
    GL_ADD
};

#define NUM_MODES 5

/* Create a texture with varying alpha for DECAL testing */
static void create_rgba_texture(GLuint *tex_id)
{
    #define TEX_SIZE 64
    uint8_t pixels[TEX_SIZE * TEX_SIZE * 4];

    for (int y = 0; y < TEX_SIZE; y++) {
        for (int x = 0; x < TEX_SIZE; x++) {
            int idx = (y * TEX_SIZE + x) * 4;

            /* Create a pattern with gradient alpha */
            float cx = (x - TEX_SIZE/2.0f) / (TEX_SIZE/2.0f);
            float cy = (y - TEX_SIZE/2.0f) / (TEX_SIZE/2.0f);
            float dist = sqrtf(cx*cx + cy*cy);

            /* Circular gradient */
            if (dist < 0.8f) {
                /* Yellow center */
                pixels[idx + 0] = 255;
                pixels[idx + 1] = 255;
                pixels[idx + 2] = 0;
                /* Alpha based on distance from center */
                float alpha = 1.0f - (dist / 0.8f);
                pixels[idx + 3] = (uint8_t)(alpha * 255);
            } else {
                /* Transparent outside */
                pixels[idx + 0] = 0;
                pixels[idx + 1] = 0;
                pixels[idx + 2] = 0;
                pixels[idx + 3] = 0;
            }
        }
    }

    glGenTextures(1, tex_id);
    glBindTexture(GL_TEXTURE_2D, *tex_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEX_SIZE, TEX_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    #undef TEX_SIZE
}

int main(int argc, char *argv[])
{
    int running = 1;
    SDL_Event event;
    float angle = 0.0f;
    int mode_idx = 0;
    GLuint texture;

#ifdef USE_MYTINYGL
    if (mtgl_init("TexEnv Test - MyTinyGL", WINDOW_WIDTH, WINDOW_HEIGHT) < 0) {
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
        "TexEnv Test - System GL",
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
    glOrtho(-2.0, 2.0, -1.5, 1.5, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0.2f, 0.2f, 0.3f, 1.0f);

    /* Enable texturing */
    glEnable(GL_TEXTURE_2D);

    /* Create texture */
    create_rgba_texture(&texture);

    /* Set initial texture environment */
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, texenv_modes[mode_idx]);

    /* Set texture environment color for GL_BLEND mode (cyan) */
    GLfloat env_color[] = { 0.0f, 1.0f, 1.0f, 1.0f };
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, env_color);

    printf("Texture environment test running\n");
    printf("Press SPACE to cycle through texture environment modes\n");
    printf("Press ESC to exit\n");
    printf("Current mode: %s\n", texenv_names[mode_idx]);
    printf("\nThe quad has varying vertex colors (red->green->blue->yellow)\n");
    printf("Texture is yellow circle with alpha gradient\n");

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
                    mode_idx = (mode_idx + 1) % NUM_MODES;
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, texenv_modes[mode_idx]);
                    printf("Switched to %s\n", texenv_names[mode_idx]);
                }
            }
        }

        glClear(GL_COLOR_BUFFER_BIT);

        glLoadIdentity();
        glRotatef(angle * 0.5f, 0.0f, 0.0f, 1.0f);

        /* Draw a quad with different vertex colors at each corner */
        glBegin(GL_QUADS);
            /* Bottom-left: Red */
            glColor3f(1.0f, 0.0f, 0.0f);
            glTexCoord2f(0.0f, 0.0f);
            glVertex2f(-1.0f, -1.0f);

            /* Bottom-right: Green */
            glColor3f(0.0f, 1.0f, 0.0f);
            glTexCoord2f(1.0f, 0.0f);
            glVertex2f(1.0f, -1.0f);

            /* Top-right: Blue */
            glColor3f(0.0f, 0.0f, 1.0f);
            glTexCoord2f(1.0f, 1.0f);
            glVertex2f(1.0f, 1.0f);

            /* Top-left: Yellow */
            glColor3f(1.0f, 1.0f, 0.0f);
            glTexCoord2f(0.0f, 1.0f);
            glVertex2f(-1.0f, 1.0f);
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
