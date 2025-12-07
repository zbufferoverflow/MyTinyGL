# Changelog

## [0.5.0] - 2025-12-06

### Added - OpenGL 1.5 VBO Support
- Vertex Buffer Objects (VBO) implementation
  - `glGenBuffers`, `glDeleteBuffers`, `glBindBuffer`
  - `glBufferData`, `glBufferSubData`
  - GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER targets
  - GL_STATIC_DRAW, GL_DYNAMIC_DRAW, GL_STREAM_DRAW usage hints
- Vertex Arrays
  - `glEnableClientState`, `glDisableClientState`
  - `glVertexPointer`, `glColorPointer`, `glTexCoordPointer`, `glNormalPointer`
  - `glDrawArrays`, `glDrawElements`
  - Support for both VBO and client-side arrays
- New files: src/vbo.h, src/vbo.c
- New test: testbed/1.5-0-vbo-test.c

## [0.4.0] - 2025-12-06

### Added - Fog Support
- OpenGL fog implementation per specification
  - `glFogi`, `glFogf`, `glFogfv`
  - GL_LINEAR, GL_EXP, GL_EXP2 modes
  - Eye-space fog coordinate calculation
- New test: testbed/1.0-9-fog-test.c

### Fixed
- Fog now uses eye-space Z coordinates instead of depth buffer values

## [0.3.0] - 2025-12-06

### Added - GLU and Pixel Transfer
- GLU functions in include/GL/glu.h (static inline implementations):
  - `gluPerspective` - Symmetric perspective projection
  - `gluLookAt` - View matrix construction
  - `gluOrtho2D` - 2D orthographic projection
  - `gluProject` - Object to window coordinates
  - `gluUnProject` - Window to object coordinates
  - `gluPickMatrix` - Picking region
  - Original Gauss-Jordan matrix inversion
- Pixel transfer operations:
  - `glReadPixels` - Read from framebuffer
  - `glDrawPixels` - Draw to framebuffer
  - `glRasterPos2i`, `glRasterPos2f`, `glRasterPos3f`

## [0.2.0] - 2025-12-06

### Added - All Primitive Types
- Complete primitive type support:
  - GL_POINTS
  - GL_LINES, GL_LINE_STRIP, GL_LINE_LOOP
  - GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN
  - GL_QUADS, GL_QUAD_STRIP, GL_POLYGON
- Sutherland-Hodgman frustum clipping (all 6 planes)
- src/clipping.c, src/clipping.h
- New tests: 1.0-3 through 1.0-8

### Added - Texture Mapping
- Texture management:
  - `glGenTextures`, `glDeleteTextures`, `glBindTexture`
  - `glTexImage2D` (GL_RGB, GL_RGBA, GL_LUMINANCE, GL_LUMINANCE_ALPHA)
  - `glTexParameteri` (filtering and wrapping)
- Perspective-correct texture interpolation
- src/textures.c, src/textures.h
- New test: testbed/1.0-7-textured-cube.c

### Added - Depth Buffer
- Z-buffer implementation
- `glEnable(GL_DEPTH_TEST)`, `glDepthFunc`, `glDepthMask`
- All comparison functions (GL_LESS, GL_LEQUAL, etc.)
- New test: testbed/1.0-6-zbuffer-test.c

### Added - Face Culling
- `glCullFace` (GL_FRONT, GL_BACK, GL_FRONT_AND_BACK)
- `glFrontFace` (GL_CW, GL_CCW)
- New test: testbed/1.0-5-culling-test.c

### Added - Blending
- `glBlendFunc` with common blend modes
- GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, etc.

## [0.1.0] - 2025-12-06

### Added - Initial Release
- Project structure (include, src, lib, testbed)
- GL/gl.h header with OpenGL 1.x function declarations
- Math primitives in graphics.h:
  - vec2_t, vec3_t, vec4_t with operations
  - mat4_t with identity, multiply, translate, scale, rotate, ortho, frustum
  - color_t with RGBA32 conversion, lerp, clamp
  - vertex_t interleaved struct (position, color, texcoord, normal)
- Framebuffer implementation:
  - framebuffer_init/free, clear_color, clear_depth
  - putpixel, getpixel, putdepth, getdepth
  - hline, vline, drawline (Bresenham with run-length slicing)
- SDL2 integration helper (include/mytinygl/sdl.h)
- OpenGL state management:
  - glEnable/glDisable with bit flags
  - glClearColor, glClearDepth, glViewport
  - glMatrixMode, glLoadIdentity, glPushMatrix, glPopMatrix
  - glOrtho, glFrustum, glTranslatef, glRotatef, glScalef
  - glMultMatrixf, glLoadMatrixf
  - glBegin, glEnd
  - glVertex2f, glVertex3f, glVertex2i, glVertex3i
  - glColor3f, glColor4f, glColor3ub, glColor4ub
  - glTexCoord2f, glNormal3f
  - glBlendFunc, glCullFace, glFrontFace
  - glFlush, glFinish
- Dynamic vertex buffer with automatic growth
- Triangle rasterization with Gouraud shading
- Testbed with hello triangle and rotating lines examples
- Makefiles for library and testbed (sysgl/mytinygl targets)

### Technical
- C99 standard
- Little-endian only
- stdint.h types for portability
- Column-major matrices (OpenGL convention)
- RGBA8888 color format
- Float depth buffer
