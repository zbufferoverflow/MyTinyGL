/*
 * MyTinyGL - OpenGL 1.x Software Renderer
 * Copyright (c) 2025 zbufferoverflow (Eliezer Solinger)
 * https://github.com/zbufferoverflow/MyTinyGL
 * SPDX-License-Identifier: MIT
 *
 * gl.h - OpenGL 1.x function declarations
 */

#ifndef MYTINYGL_GL_H
#define MYTINYGL_GL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Data types */
typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef void GLvoid;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef int GLsizei;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef long GLsizeiptr;
typedef long GLintptr;

/* Boolean values */
#define GL_FALSE 0
#define GL_TRUE  1

/* Primitive types */
#define GL_POINTS         0x0000
#define GL_LINES          0x0001
#define GL_LINE_LOOP      0x0002
#define GL_LINE_STRIP     0x0003
#define GL_TRIANGLES      0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRIANGLE_FAN   0x0006
#define GL_QUADS          0x0007
#define GL_QUAD_STRIP     0x0008
#define GL_POLYGON        0x0009

/* Clear buffer bits */
#define GL_DEPTH_BUFFER_BIT   0x00000100
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_COLOR_BUFFER_BIT   0x00004000

/* Matrix modes */
#define GL_MODELVIEW  0x1700
#define GL_PROJECTION 0x1701
#define GL_TEXTURE    0x1702

/* Enable/Disable caps */
#define GL_FOG           0x0B60
#define GL_DEPTH_TEST    0x0B71
#define GL_CULL_FACE     0x0B44
#define GL_BLEND         0x0BE2
#define GL_TEXTURE_2D    0x0DE1
#define GL_LIGHTING      0x0B50
#define GL_SCISSOR_TEST  0x0C11
#define GL_STENCIL_TEST  0x0B90
#define GL_LIGHT0        0x4000
#define GL_LIGHT1        0x4001
#define GL_LIGHT2        0x4002
#define GL_LIGHT3        0x4003
#define GL_LIGHT4        0x4004
#define GL_LIGHT5        0x4005
#define GL_LIGHT6        0x4006
#define GL_LIGHT7        0x4007

/* Texture parameters */
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S     0x2802
#define GL_TEXTURE_WRAP_T     0x2803
#define GL_NEAREST            0x2600
#define GL_LINEAR             0x2601
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_LINEAR_MIPMAP_NEAREST  0x2701
#define GL_NEAREST_MIPMAP_LINEAR  0x2702
#define GL_LINEAR_MIPMAP_LINEAR   0x2703
#define GL_REPEAT             0x2901
#define GL_CLAMP              0x2900
#define GL_CLAMP_TO_EDGE      0x812F

/* Texture environment */
#define GL_TEXTURE_ENV        0x2300
#define GL_TEXTURE_ENV_MODE   0x2200
#define GL_TEXTURE_ENV_COLOR  0x2201
#define GL_MODULATE           0x2100
#define GL_DECAL              0x2101
#define GL_REPLACE            0x1E01
#define GL_ADD                0x0104

/* Pixel storage modes */
#define GL_PACK_ALIGNMENT     0x0D05
#define GL_UNPACK_ALIGNMENT   0x0CF5

/* Pixel formats */
#define GL_LUMINANCE       0x1909
#define GL_LUMINANCE_ALPHA 0x190A
#define GL_RGB             0x1907
#define GL_RGBA            0x1908

/* Data types for pixels */
#define GL_UNSIGNED_BYTE 0x1401

/* Front face */
#define GL_CW  0x0900
#define GL_CCW 0x0901

/* Cull face */
#define GL_FRONT 0x0404
#define GL_BACK  0x0405
#define GL_FRONT_AND_BACK 0x0408

/* Polygon mode */
#define GL_POINT 0x1B00
#define GL_LINE  0x1B01
#define GL_FILL  0x1B02

/* Blend functions */
#define GL_ZERO                     0
#define GL_ONE                      1
#define GL_SRC_COLOR                0x0300
#define GL_ONE_MINUS_SRC_COLOR      0x0301
#define GL_SRC_ALPHA                0x0302
#define GL_ONE_MINUS_SRC_ALPHA      0x0303
#define GL_DST_ALPHA                0x0304
#define GL_ONE_MINUS_DST_ALPHA      0x0305
#define GL_DST_COLOR                0x0306
#define GL_ONE_MINUS_DST_COLOR      0x0307
#define GL_SRC_ALPHA_SATURATE       0x0308
#define GL_CONSTANT_COLOR           0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR 0x8002
#define GL_CONSTANT_ALPHA           0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA 0x8004

/* Depth/Alpha test functions */
#define GL_NEVER    0x0200
#define GL_LESS     0x0201
#define GL_EQUAL    0x0202
#define GL_LEQUAL   0x0203
#define GL_GREATER  0x0204
#define GL_NOTEQUAL 0x0205
#define GL_GEQUAL   0x0206
#define GL_ALWAYS   0x0207

/* Alpha test */
#define GL_ALPHA_TEST       0x0BC0
#define GL_ALPHA_TEST_FUNC  0x0BC1
#define GL_ALPHA_TEST_REF   0x0BC2

/* Stencil operations */
#define GL_KEEP      0x1E00
#define GL_INCR      0x1E02
#define GL_DECR      0x1E03
#define GL_INVERT    0x150A
#define GL_INCR_WRAP 0x8507
#define GL_DECR_WRAP 0x8508

/* Hint targets */
#define GL_PERSPECTIVE_CORRECTION_HINT 0x0C50
#define GL_POINT_SMOOTH_HINT           0x0C51
#define GL_LINE_SMOOTH_HINT            0x0C52
#define GL_FOG_HINT                    0x0C54

/* Hint modes */
#define GL_DONT_CARE 0x1100
#define GL_FASTEST   0x1101
#define GL_NICEST    0x1102

/* Fog modes */
#define GL_EXP       0x0800
#define GL_EXP2      0x0801

/* Fog parameters */
#define GL_FOG_MODE    0x0B65
#define GL_FOG_DENSITY 0x0B62
#define GL_FOG_START   0x0B63
#define GL_FOG_END     0x0B64
#define GL_FOG_COLOR   0x0B66

/* Light parameters */
#define GL_AMBIENT               0x1200
#define GL_DIFFUSE               0x1201
#define GL_SPECULAR              0x1202
#define GL_POSITION              0x1203
#define GL_SPOT_DIRECTION        0x1204
#define GL_SPOT_EXPONENT         0x1205
#define GL_SPOT_CUTOFF           0x1206
#define GL_CONSTANT_ATTENUATION  0x1207
#define GL_LINEAR_ATTENUATION    0x1208
#define GL_QUADRATIC_ATTENUATION 0x1209

/* Material parameters */
#define GL_EMISSION              0x1600
#define GL_SHININESS             0x1601
#define GL_AMBIENT_AND_DIFFUSE   0x1602
#define GL_COLOR_INDEXES         0x1603

/* Light model parameters */
#define GL_LIGHT_MODEL_AMBIENT       0x0B53
#define GL_LIGHT_MODEL_LOCAL_VIEWER  0x0B51
#define GL_LIGHT_MODEL_TWO_SIDE      0x0B52

/* Color material */
#define GL_COLOR_MATERIAL        0x0B57

/* Shade model */
#define GL_FLAT                  0x1D00
#define GL_SMOOTH                0x1D01
#define GL_PHONG                 0x1D02  /* MyTinyGL extension: per-fragment lighting */

/* Normalize */
#define GL_NORMALIZE             0x0BA1

/* Buffer object targets (OpenGL 1.5) */
#define GL_ARRAY_BUFFER         0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893

/* Buffer usage hints */
#define GL_STREAM_DRAW  0x88E0
#define GL_STREAM_READ  0x88E1
#define GL_STREAM_COPY  0x88E2
#define GL_STATIC_DRAW  0x88E4
#define GL_STATIC_READ  0x88E5
#define GL_STATIC_COPY  0x88E6
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_DYNAMIC_READ 0x88E9
#define GL_DYNAMIC_COPY 0x88EA

/* Client state arrays */
#define GL_VERTEX_ARRAY        0x8074
#define GL_COLOR_ARRAY         0x8076
#define GL_TEXTURE_COORD_ARRAY 0x8078
#define GL_NORMAL_ARRAY        0x8075

/* Data types */
#define GL_UNSIGNED_SHORT 0x1403
#define GL_UNSIGNED_INT   0x1405
#define GL_FLOAT          0x1406

/* Display list modes */
#define GL_COMPILE             0x1300
#define GL_COMPILE_AND_EXECUTE 0x1301

/* Errors */
#define GL_NO_ERROR          0
#define GL_INVALID_ENUM      0x0500
#define GL_INVALID_VALUE     0x0501
#define GL_INVALID_OPERATION 0x0502
#define GL_STACK_OVERFLOW    0x0503
#define GL_STACK_UNDERFLOW   0x0504
#define GL_OUT_OF_MEMORY     0x0505

/* String queries */
#define GL_VENDOR                     0x1F00
#define GL_RENDERER                   0x1F01
#define GL_VERSION                    0x1F02
#define GL_EXTENSIONS                 0x1F03

/* State queries - Current values */
#define GL_CURRENT_COLOR              0x0B00
#define GL_CURRENT_INDEX              0x0B01
#define GL_CURRENT_NORMAL             0x0B02
#define GL_CURRENT_TEXTURE_COORDS     0x0B03
#define GL_CURRENT_RASTER_COLOR       0x0B04
#define GL_CURRENT_RASTER_INDEX       0x0B05
#define GL_CURRENT_RASTER_TEXTURE_COORDS 0x0B06
#define GL_CURRENT_RASTER_POSITION    0x0B07
#define GL_CURRENT_RASTER_POSITION_VALID 0x0B08
#define GL_CURRENT_RASTER_DISTANCE    0x0B09

/* State queries - Vertex array */
#define GL_VERTEX_ARRAY               0x8074
#define GL_NORMAL_ARRAY               0x8075
#define GL_COLOR_ARRAY                0x8076
#define GL_INDEX_ARRAY                0x8077
#define GL_TEXTURE_COORD_ARRAY        0x8078
#define GL_EDGE_FLAG_ARRAY            0x8079
#define GL_VERTEX_ARRAY_SIZE          0x807A
#define GL_VERTEX_ARRAY_TYPE          0x807B
#define GL_VERTEX_ARRAY_STRIDE        0x807C
#define GL_NORMAL_ARRAY_TYPE          0x807E
#define GL_NORMAL_ARRAY_STRIDE        0x807F
#define GL_COLOR_ARRAY_SIZE           0x8081
#define GL_COLOR_ARRAY_TYPE           0x8082
#define GL_COLOR_ARRAY_STRIDE         0x8083
#define GL_TEXTURE_COORD_ARRAY_SIZE   0x8088
#define GL_TEXTURE_COORD_ARRAY_TYPE   0x8089
#define GL_TEXTURE_COORD_ARRAY_STRIDE 0x808A

/* State queries - Matrix */
#define GL_MODELVIEW_MATRIX           0x0BA6
#define GL_PROJECTION_MATRIX          0x0BA7
#define GL_TEXTURE_MATRIX             0x0BA8
#define GL_MODELVIEW_STACK_DEPTH      0x0BA3
#define GL_PROJECTION_STACK_DEPTH     0x0BA4
#define GL_TEXTURE_STACK_DEPTH        0x0BA5
#define GL_MATRIX_MODE                0x0BA0

/* State queries - Viewport */
#define GL_VIEWPORT                   0x0BA2
#define GL_DEPTH_RANGE                0x0B70

/* State queries - Transformation */
#define GL_NORMALIZE                  0x0BA1
#define GL_RESCALE_NORMAL             0x803A

/* State queries - Coloring */
#define GL_SHADE_MODEL                0x0B54
#define GL_COLOR_MATERIAL             0x0B57
#define GL_COLOR_MATERIAL_FACE        0x0B55
#define GL_COLOR_MATERIAL_PARAMETER   0x0B56
#define GL_FOG                        0x0B60
#define GL_FOG_INDEX                  0x0B61
#define GL_FOG_DENSITY                0x0B62
#define GL_FOG_START                  0x0B63
#define GL_FOG_END                    0x0B64
#define GL_FOG_MODE                   0x0B65
#define GL_FOG_COLOR                  0x0B66

/* State queries - Lighting */
#define GL_LIGHTING                   0x0B50
#define GL_LIGHT_MODEL_LOCAL_VIEWER   0x0B51
#define GL_LIGHT_MODEL_TWO_SIDE       0x0B52
#define GL_LIGHT_MODEL_AMBIENT        0x0B53
#define GL_LIGHT_MODEL_COLOR_CONTROL  0x81F8
#define GL_SINGLE_COLOR               0x81F9
#define GL_SEPARATE_SPECULAR_COLOR    0x81FA

/* State queries - Rasterization */
#define GL_POINT_SIZE                 0x0B11
#define GL_POINT_SIZE_MIN             0x8126
#define GL_POINT_SIZE_MAX             0x8127
#define GL_POINT_FADE_THRESHOLD_SIZE  0x8128
#define GL_POINT_DISTANCE_ATTENUATION 0x8129
#define GL_POINT_SMOOTH               0x0B10
#define GL_POINT_SIZE_RANGE           0x0B12
#define GL_POINT_SIZE_GRANULARITY     0x0B13
#define GL_LINE_SMOOTH                0x0B20
#define GL_LINE_WIDTH                 0x0B21
#define GL_LINE_WIDTH_RANGE           0x0B22
#define GL_LINE_WIDTH_GRANULARITY     0x0B23
#define GL_LINE_STIPPLE               0x0B24
#define GL_LINE_STIPPLE_PATTERN       0x0B25
#define GL_LINE_STIPPLE_REPEAT        0x0B26
#define GL_CULL_FACE                  0x0B44
#define GL_CULL_FACE_MODE             0x0B45
#define GL_FRONT_FACE                 0x0B46
#define GL_POLYGON_SMOOTH             0x0B41
#define GL_POLYGON_MODE               0x0B40
#define GL_POLYGON_OFFSET_FACTOR      0x8038
#define GL_POLYGON_OFFSET_UNITS       0x2A00
#define GL_POLYGON_OFFSET_POINT       0x2A01
#define GL_POLYGON_OFFSET_LINE        0x2A02
#define GL_POLYGON_OFFSET_FILL        0x8037
#define GL_POLYGON_STIPPLE            0x0B42

/* State queries - Texturing */
#define GL_TEXTURE_1D                 0x0DE0
#define GL_TEXTURE_2D                 0x0DE1
#define GL_TEXTURE_3D                 0x806F
#define GL_TEXTURE_BINDING_1D         0x8068
#define GL_TEXTURE_BINDING_2D         0x8069
#define GL_TEXTURE_BINDING_3D         0x806A
#define GL_TEXTURE_GEN_S              0x0C60
#define GL_TEXTURE_GEN_T              0x0C61
#define GL_TEXTURE_GEN_R              0x0C62
#define GL_TEXTURE_GEN_Q              0x0C63

/* State queries - Pixel operations */
#define GL_SCISSOR_TEST               0x0C11
#define GL_SCISSOR_BOX                0x0C10
#define GL_ALPHA_TEST                 0x0BC0
#define GL_ALPHA_TEST_FUNC            0x0BC1
#define GL_ALPHA_TEST_REF             0x0BC2
#define GL_STENCIL_TEST               0x0B90
#define GL_STENCIL_FUNC               0x0B92
#define GL_STENCIL_VALUE_MASK         0x0B93
#define GL_STENCIL_REF                0x0B97
#define GL_STENCIL_FAIL               0x0B94
#define GL_STENCIL_PASS_DEPTH_FAIL    0x0B95
#define GL_STENCIL_PASS_DEPTH_PASS    0x0B96
#define GL_STENCIL_WRITEMASK          0x0B98
#define GL_STENCIL_CLEAR_VALUE        0x0B91
#define GL_DEPTH_TEST                 0x0B71
#define GL_DEPTH_FUNC                 0x0B74
#define GL_DEPTH_WRITEMASK            0x0B72
#define GL_DEPTH_CLEAR_VALUE          0x0B73
#define GL_BLEND                      0x0BE2
#define GL_BLEND_SRC                  0x0BE1
#define GL_BLEND_DST                  0x0BE0
#define GL_BLEND_SRC_RGB              0x80C9
#define GL_BLEND_SRC_ALPHA            0x80CB
#define GL_BLEND_DST_RGB              0x80C8
#define GL_BLEND_DST_ALPHA            0x80CA
#define GL_BLEND_COLOR                0x8005
#define GL_DITHER                     0x0BD0
#define GL_INDEX_LOGIC_OP             0x0BF1
#define GL_COLOR_LOGIC_OP             0x0BF2
#define GL_LOGIC_OP_MODE              0x0BF0

/* State queries - Framebuffer */
#define GL_DRAW_BUFFER                0x0C01
#define GL_READ_BUFFER                0x0C02
#define GL_INDEX_WRITEMASK            0x0C21
#define GL_COLOR_WRITEMASK            0x0C23
#define GL_DEPTH_WRITEMASK            0x0B72
#define GL_STENCIL_WRITEMASK          0x0B98
#define GL_COLOR_CLEAR_VALUE          0x0C22

/* State queries - Pixel transfer */
#define GL_MAP_COLOR                  0x0D10
#define GL_MAP_STENCIL                0x0D11
#define GL_INDEX_SHIFT                0x0D12
#define GL_INDEX_OFFSET               0x0D13
#define GL_RED_SCALE                  0x0D14
#define GL_RED_BIAS                   0x0D15
#define GL_GREEN_SCALE                0x0D18
#define GL_GREEN_BIAS                 0x0D19
#define GL_BLUE_SCALE                 0x0D1A
#define GL_BLUE_BIAS                  0x0D1B
#define GL_ALPHA_SCALE                0x0D1C
#define GL_ALPHA_BIAS                 0x0D1D
#define GL_DEPTH_SCALE                0x0D1E
#define GL_DEPTH_BIAS                 0x0D1F
#define GL_ZOOM_X                     0x0D16
#define GL_ZOOM_Y                     0x0D17

/* State queries - Pixel storage */
#define GL_UNPACK_SWAP_BYTES          0x0CF0
#define GL_UNPACK_LSB_FIRST           0x0CF1
#define GL_UNPACK_ROW_LENGTH          0x0CF2
#define GL_UNPACK_SKIP_ROWS           0x0CF3
#define GL_UNPACK_SKIP_PIXELS         0x0CF4
#define GL_UNPACK_ALIGNMENT           0x0CF5
#define GL_PACK_SWAP_BYTES            0x0D00
#define GL_PACK_LSB_FIRST             0x0D01
#define GL_PACK_ROW_LENGTH            0x0D02
#define GL_PACK_SKIP_ROWS             0x0D03
#define GL_PACK_SKIP_PIXELS           0x0D04
#define GL_PACK_ALIGNMENT             0x0D05

/* State queries - Hints */
#define GL_PERSPECTIVE_CORRECTION_HINT 0x0C50
#define GL_POINT_SMOOTH_HINT          0x0C51
#define GL_LINE_SMOOTH_HINT           0x0C52
#define GL_POLYGON_SMOOTH_HINT        0x0C53
#define GL_FOG_HINT                   0x0C54
#define GL_GENERATE_MIPMAP_HINT       0x8192

/* State queries - Implementation limits */
#define GL_MAX_LIGHTS                 0x0D31
#define GL_MAX_CLIP_PLANES            0x0D32
#define GL_MAX_TEXTURE_SIZE           0x0D33
#define GL_MAX_PIXEL_MAP_TABLE        0x0D34
#define GL_MAX_ATTRIB_STACK_DEPTH     0x0D35
#define GL_MAX_MODELVIEW_STACK_DEPTH  0x0D36
#define GL_MAX_NAME_STACK_DEPTH       0x0D37
#define GL_MAX_PROJECTION_STACK_DEPTH 0x0D38
#define GL_MAX_TEXTURE_STACK_DEPTH    0x0D39
#define GL_MAX_VIEWPORT_DIMS          0x0D3A
#define GL_MAX_CLIENT_ATTRIB_STACK_DEPTH 0x0D3B
#define GL_MAX_TEXTURE_UNITS          0x84E2
#define GL_MAX_TEXTURE_LOD_BIAS       0x84FD
#define GL_MAX_ELEMENTS_VERTICES      0x80E8
#define GL_MAX_ELEMENTS_INDICES       0x80E9
#define GL_MAX_3D_TEXTURE_SIZE        0x8073
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE  0x851C

/* State queries - Framebuffer characteristics */
#define GL_SUBPIXEL_BITS              0x0D50
#define GL_INDEX_BITS                 0x0D51
#define GL_RED_BITS                   0x0D52
#define GL_GREEN_BITS                 0x0D53
#define GL_BLUE_BITS                  0x0D54
#define GL_ALPHA_BITS                 0x0D55
#define GL_DEPTH_BITS                 0x0D56
#define GL_STENCIL_BITS               0x0D57
#define GL_ACCUM_RED_BITS             0x0D58
#define GL_ACCUM_GREEN_BITS           0x0D59
#define GL_ACCUM_BLUE_BITS            0x0D5A
#define GL_ACCUM_ALPHA_BITS           0x0D5B

/* State queries - Miscellaneous */
#define GL_LIST_BASE                  0x0B32
#define GL_LIST_INDEX                 0x0B33
#define GL_LIST_MODE                  0x0B30
#define GL_ATTRIB_STACK_DEPTH         0x0BB0
#define GL_CLIENT_ATTRIB_STACK_DEPTH  0x0BB1
#define GL_NAME_STACK_DEPTH           0x0D70
#define GL_RENDER_MODE                0x0C40
#define GL_SELECTION_BUFFER_SIZE      0x0DF4
#define GL_FEEDBACK_BUFFER_SIZE       0x0DF1
#define GL_FEEDBACK_BUFFER_TYPE       0x0DF2
#define GL_DOUBLEBUFFER               0x0C32
#define GL_STEREO                     0x0C33
#define GL_AUX_BUFFERS                0x0C00
#define GL_RGBA_MODE                  0x0C31
#define GL_INDEX_MODE                 0x0C30
#define GL_SAMPLE_BUFFERS             0x80A8
#define GL_SAMPLES                    0x80A9
#define GL_SAMPLE_COVERAGE_VALUE      0x80AA
#define GL_SAMPLE_COVERAGE_INVERT     0x80AB

/* OpenGL 1.5 buffer object queries */
#define GL_BUFFER_SIZE                0x8764
#define GL_BUFFER_USAGE               0x8765
#define GL_BUFFER_ACCESS              0x88BB
#define GL_BUFFER_MAPPED              0x88BC
#define GL_BUFFER_MAP_POINTER         0x88BD
#define GL_ARRAY_BUFFER_BINDING       0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING 0x8895
#define GL_VERTEX_ARRAY_BUFFER_BINDING 0x8896
#define GL_NORMAL_ARRAY_BUFFER_BINDING 0x8897
#define GL_COLOR_ARRAY_BUFFER_BINDING  0x8898
#define GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING 0x889A

/* State functions */
void glEnable(GLenum cap);
void glDisable(GLenum cap);
void glClear(GLbitfield mask);
void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void glClearDepth(GLclampd depth);
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);

/* Matrix functions */
void glMatrixMode(GLenum mode);
void glLoadIdentity(void);
void glPushMatrix(void);
void glPopMatrix(void);
void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near, GLdouble far);
void glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near, GLdouble far);
void glTranslatef(GLfloat x, GLfloat y, GLfloat z);
void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void glScalef(GLfloat x, GLfloat y, GLfloat z);
void glMultMatrixf(const GLfloat *m);
void glLoadMatrixf(const GLfloat *m);

/* Vertex specification */
void glBegin(GLenum mode);
void glEnd(void);
void glVertex2f(GLfloat x, GLfloat y);
void glVertex3f(GLfloat x, GLfloat y, GLfloat z);
void glVertex2i(GLint x, GLint y);
void glVertex3i(GLint x, GLint y, GLint z);
void glColor3f(GLfloat red, GLfloat green, GLfloat blue);
void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glColor3ub(GLubyte red, GLubyte green, GLubyte blue);
void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
void glTexCoord2f(GLfloat s, GLfloat t);
void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz);

/* Texture functions */
void glGenTextures(GLsizei n, GLuint *textures);
void glDeleteTextures(GLsizei n, const GLuint *textures);
void glBindTexture(GLenum target, GLuint texture);
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void glTexParameteri(GLenum target, GLenum pname, GLint param);

/* Pixel transfer */
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
void glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void glRasterPos2i(GLint x, GLint y);
void glRasterPos2f(GLfloat x, GLfloat y);
void glRasterPos3f(GLfloat x, GLfloat y, GLfloat z);

/* Fog */
void glFogi(GLenum pname, GLint param);
void glFogf(GLenum pname, GLfloat param);
void glFogfv(GLenum pname, const GLfloat *params);

/* Lighting */
void glLightf(GLenum light, GLenum pname, GLfloat param);
void glLighti(GLenum light, GLenum pname, GLint param);
void glLightfv(GLenum light, GLenum pname, const GLfloat *params);
void glLightiv(GLenum light, GLenum pname, const GLint *params);
void glGetLightfv(GLenum light, GLenum pname, GLfloat *params);
void glMaterialf(GLenum face, GLenum pname, GLfloat param);
void glMateriali(GLenum face, GLenum pname, GLint param);
void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params);
void glMaterialiv(GLenum face, GLenum pname, const GLint *params);
void glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params);
void glLightModelf(GLenum pname, GLfloat param);
void glLightModeli(GLenum pname, GLint param);
void glLightModelfv(GLenum pname, const GLfloat *params);
void glLightModeliv(GLenum pname, const GLint *params);
void glColorMaterial(GLenum face, GLenum mode);
void glShadeModel(GLenum mode);

/* Buffer objects (OpenGL 1.5) */
void glGenBuffers(GLsizei n, GLuint *buffers);
void glDeleteBuffers(GLsizei n, const GLuint *buffers);
void glBindBuffer(GLenum target, GLuint buffer);
void glBufferData(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);

/* Vertex arrays */
void glEnableClientState(GLenum array);
void glDisableClientState(GLenum array);
void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer);
void glDrawArrays(GLenum mode, GLint first, GLsizei count);
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);

/* Misc */
void glFlush(void);
void glFinish(void);
GLenum glGetError(void);
void glBlendFunc(GLenum sfactor, GLenum dfactor);

/* State queries */
void glGetIntegerv(GLenum pname, GLint *params);
void glGetFloatv(GLenum pname, GLfloat *params);
void glGetDoublev(GLenum pname, GLdouble *params);
void glGetBooleanv(GLenum pname, GLboolean *params);
GLboolean glIsEnabled(GLenum cap);
const GLubyte *glGetString(GLenum name);
void glCullFace(GLenum mode);
void glFrontFace(GLenum mode);
void glDepthFunc(GLenum func);
void glDepthMask(GLboolean flag);
void glAlphaFunc(GLenum func, GLclampf ref);
void glHint(GLenum target, GLenum mode);
void glLineWidth(GLfloat width);
void glPointSize(GLfloat size);
void glPolygonMode(GLenum face, GLenum mode);
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
void glDepthRange(GLclampd near, GLclampd far);
void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void glPixelStorei(GLenum pname, GLint param);

/* Stencil functions (stubs - not fully implemented) */
void glStencilFunc(GLenum func, GLint ref, GLuint mask);
void glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass);
void glStencilMask(GLuint mask);
void glClearStencil(GLint s);
void glTexEnvi(GLenum target, GLenum pname, GLint param);
void glTexEnvf(GLenum target, GLenum pname, GLfloat param);
void glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params);
GLboolean glIsTexture(GLuint texture);
GLboolean glIsBuffer(GLuint buffer);

/* Light/Material queries */
void glGetLightfv(GLenum light, GLenum pname, GLfloat *params);
void glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params);

/* Display lists */
GLuint glGenLists(GLsizei range);
void glDeleteLists(GLuint list, GLsizei range);
void glNewList(GLuint list, GLenum mode);
void glEndList(void);
void glCallList(GLuint list);
void glCallLists(GLsizei n, GLenum type, const GLvoid *lists);
void glListBase(GLuint base);
GLboolean glIsList(GLuint list);

#ifdef __cplusplus
}
#endif

#endif /* MYTINYGL_GL_H */
