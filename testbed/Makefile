CC = gcc
CFLAGS = -Wall -g -std=c99
CFLAGS_MYTINYGL = -I../include -DUSE_MYTINYGL
LDFLAGS_SYSGL = -lSDL2 -lGL -lm
LDFLAGS_MYTINYGL = -lSDL2 -L../lib -lMyTinyGL -lm

SOURCES = 1.0-0-clear-screen.c 1.0-1-rotating-lines.c 1.0-2-helloworld-triangle.c 1.0-3-clipping-test.c 1.0-4-primitives-test.c 1.0-5-culling-test.c 1.0-6-zbuffer-test.c 1.0-7-textured-cube.c 1.0-8-all-primitives.c 1.0-9-fog-test.c 1.0-10-lighting-test.c 1.0-11-blend-test.c 1.0-12-filter-test.c 1.0-13-displaylist-test.c 1.0-14-mipmap-test.c 1.0-15-texenv-test.c 1.0-16-validation-test.c 1.0-17-stencil-test.c 1.5-0-vbo-test.c
TARGETS_SYSGL = $(SOURCES:.c=-sysgl.run)
TARGETS_MYTINYGL = $(SOURCES:.c=-mytinygl.run)

all: mytinygl

sysgl: $(TARGETS_SYSGL)

mytinygl: $(TARGETS_MYTINYGL)

%-sysgl.run: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS_SYSGL)

%-mytinygl.run: %.c
	$(CC) $(CFLAGS) $(CFLAGS_MYTINYGL) -o $@ $< $(LDFLAGS_MYTINYGL)

clean:
	rm -f $(TARGETS_SYSGL) $(TARGETS_MYTINYGL)

.PHONY: all sysgl mytinygl clean
