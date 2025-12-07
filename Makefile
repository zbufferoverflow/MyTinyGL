CC = gcc
AR = ar
CFLAGS = -Wall -g -std=c99 -I./include

SRC = src/gl_api.c src/raster.c src/textures.c src/vbo.c src/lists.c
OBJ = $(SRC:.c=.o)
LIB = lib/libMyTinyGL.a

all: $(LIB)

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(LIB)

.PHONY: all clean
