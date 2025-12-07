/*
 * MyTinyGL - OpenGL 1.x Software Renderer
 * Copyright (c) 2025 zbufferoverflow (Eliezer Solinger)
 * https://github.com/zbufferoverflow/MyTinyGL
 * SPDX-License-Identifier: MIT
 *
 * glu.h - GLU utility functions (static inline implementations)
 */

#ifndef MYTINYGL_GLU_H
#define MYTINYGL_GLU_H

#include "gl.h"
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Internal: 4x4 matrix multiply C = A * B (column-major) */
static inline void glu__mat4_mul(const GLdouble A[16], const GLdouble B[16], GLdouble C[16])
{
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            C[col * 4 + row] =
                A[0 * 4 + row] * B[col * 4 + 0] +
                A[1 * 4 + row] * B[col * 4 + 1] +
                A[2 * 4 + row] * B[col * 4 + 2] +
                A[3 * 4 + row] * B[col * 4 + 3];
        }
    }
}

/* Internal: 4x4 matrix inversion using Gauss-Jordan elimination */
static inline int glu__mat4_invert(const GLdouble src[16], GLdouble dst[16])
{
    GLdouble tmp[16], aug[16];
    int i, j, k, pivot;
    GLdouble max_val, scale, swap;

    /* Copy source and create identity for augmented matrix */
    for (i = 0; i < 16; i++) {
        tmp[i] = src[i];
        aug[i] = (i % 5 == 0) ? 1.0 : 0.0;  /* Identity: 0,5,10,15 */
    }

    /* Forward elimination with partial pivoting */
    for (j = 0; j < 4; j++) {
        /* Find pivot (largest absolute value in column) */
        pivot = j;
        max_val = fabs(tmp[j * 4 + j]);
        for (i = j + 1; i < 4; i++) {
            if (fabs(tmp[j * 4 + i]) > max_val) {
                max_val = fabs(tmp[j * 4 + i]);
                pivot = i;
            }
        }

        if (max_val < 1e-12) return 0;  /* Singular matrix */

        /* Swap rows if needed */
        if (pivot != j) {
            for (k = 0; k < 4; k++) {
                swap = tmp[k * 4 + j];
                tmp[k * 4 + j] = tmp[k * 4 + pivot];
                tmp[k * 4 + pivot] = swap;

                swap = aug[k * 4 + j];
                aug[k * 4 + j] = aug[k * 4 + pivot];
                aug[k * 4 + pivot] = swap;
            }
        }

        /* Scale pivot row */
        scale = 1.0 / tmp[j * 4 + j];
        for (k = 0; k < 4; k++) {
            tmp[k * 4 + j] *= scale;
            aug[k * 4 + j] *= scale;
        }

        /* Eliminate column */
        for (i = 0; i < 4; i++) {
            if (i != j) {
                scale = tmp[j * 4 + i];
                for (k = 0; k < 4; k++) {
                    tmp[k * 4 + i] -= scale * tmp[k * 4 + j];
                    aug[k * 4 + i] -= scale * aug[k * 4 + j];
                }
            }
        }
    }

    for (i = 0; i < 16; i++) dst[i] = aug[i];
    return 1;
}

/* Internal: transform vec4 by matrix */
static inline void glu__transform(const GLdouble m[16], const GLdouble in[4], GLdouble out[4])
{
    out[0] = m[0]*in[0] + m[4]*in[1] + m[8]*in[2]  + m[12]*in[3];
    out[1] = m[1]*in[0] + m[5]*in[1] + m[9]*in[2]  + m[13]*in[3];
    out[2] = m[2]*in[0] + m[6]*in[1] + m[10]*in[2] + m[14]*in[3];
    out[3] = m[3]*in[0] + m[7]*in[1] + m[11]*in[2] + m[15]*in[3];
}

/*
 * gluPerspective - Set up a perspective projection matrix
 * Based on OpenGL specification: builds symmetric perspective frustum
 */
static inline void gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
    GLdouble half_height = zNear * tan(fovy * M_PI / 360.0);
    GLdouble half_width = half_height * aspect;
    glFrustum(-half_width, half_width, -half_height, half_height, zNear, zFar);
}

/*
 * gluLookAt - Define a viewing transformation
 * Constructs view matrix from eye position, target point, and up vector
 */
static inline void gluLookAt(GLdouble eyeX, GLdouble eyeY, GLdouble eyeZ,
                             GLdouble targetX, GLdouble targetY, GLdouble targetZ,
                             GLdouble upX, GLdouble upY, GLdouble upZ)
{
    /* Compute normalized direction vectors */
    GLdouble fwd[3] = { targetX - eyeX, targetY - eyeY, targetZ - eyeZ };
    GLdouble fwd_len = sqrt(fwd[0]*fwd[0] + fwd[1]*fwd[1] + fwd[2]*fwd[2]);
    if (fwd_len > 0.0) { fwd[0] /= fwd_len; fwd[1] /= fwd_len; fwd[2] /= fwd_len; }

    /* right = forward x up */
    GLdouble right[3] = {
        fwd[1] * upZ - fwd[2] * upY,
        fwd[2] * upX - fwd[0] * upZ,
        fwd[0] * upY - fwd[1] * upX
    };
    GLdouble right_len = sqrt(right[0]*right[0] + right[1]*right[1] + right[2]*right[2]);
    if (right_len > 0.0) { right[0] /= right_len; right[1] /= right_len; right[2] /= right_len; }

    /* true_up = right x forward */
    GLdouble true_up[3] = {
        right[1] * fwd[2] - right[2] * fwd[1],
        right[2] * fwd[0] - right[0] * fwd[2],
        right[0] * fwd[1] - right[1] * fwd[0]
    };

    /* Rotation matrix (transpose because we want inverse rotation) */
    GLfloat rot[16] = {
        (GLfloat)right[0],   (GLfloat)true_up[0], (GLfloat)(-fwd[0]), 0.0f,
        (GLfloat)right[1],   (GLfloat)true_up[1], (GLfloat)(-fwd[1]), 0.0f,
        (GLfloat)right[2],   (GLfloat)true_up[2], (GLfloat)(-fwd[2]), 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    glMultMatrixf(rot);
    glTranslatef((GLfloat)(-eyeX), (GLfloat)(-eyeY), (GLfloat)(-eyeZ));
}

/*
 * gluOrtho2D - Define a 2D orthographic projection matrix
 */
static inline void gluOrtho2D(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top)
{
    glOrtho(left, right, bottom, top, -1.0, 1.0);
}

/*
 * gluProject - Map object coordinates to window coordinates
 */
static inline GLint gluProject(GLdouble objX, GLdouble objY, GLdouble objZ,
                               const GLdouble model[16], const GLdouble proj[16],
                               const GLint viewport[4],
                               GLdouble *winX, GLdouble *winY, GLdouble *winZ)
{
    GLdouble eye[4], clip[4];
    GLdouble obj[4] = { objX, objY, objZ, 1.0 };

    glu__transform(model, obj, eye);
    glu__transform(proj, eye, clip);

    if (clip[3] == 0.0) return GL_FALSE;

    /* NDC */
    GLdouble ndc[3] = { clip[0] / clip[3], clip[1] / clip[3], clip[2] / clip[3] };

    /* Viewport transform */
    *winX = viewport[0] + viewport[2] * (ndc[0] + 1.0) * 0.5;
    *winY = viewport[1] + viewport[3] * (ndc[1] + 1.0) * 0.5;
    *winZ = (ndc[2] + 1.0) * 0.5;

    return GL_TRUE;
}

/*
 * gluUnProject - Map window coordinates to object coordinates
 */
static inline GLint gluUnProject(GLdouble winX, GLdouble winY, GLdouble winZ,
                                 const GLdouble model[16], const GLdouble proj[16],
                                 const GLint viewport[4],
                                 GLdouble *objX, GLdouble *objY, GLdouble *objZ)
{
    GLdouble combined[16], inv_combined[16];

    glu__mat4_mul(model, proj, combined);
    if (!glu__mat4_invert(combined, inv_combined)) return GL_FALSE;

    /* Window to NDC */
    GLdouble ndc[4] = {
        (winX - viewport[0]) / viewport[2] * 2.0 - 1.0,
        (winY - viewport[1]) / viewport[3] * 2.0 - 1.0,
        winZ * 2.0 - 1.0,
        1.0
    };

    GLdouble obj[4];
    glu__transform(inv_combined, ndc, obj);

    if (obj[3] == 0.0) return GL_FALSE;

    *objX = obj[0] / obj[3];
    *objY = obj[1] / obj[3];
    *objZ = obj[2] / obj[3];

    return GL_TRUE;
}

/*
 * gluPickMatrix - Define a picking region
 */
static inline void gluPickMatrix(GLdouble x, GLdouble y,
                                 GLdouble width, GLdouble height,
                                 const GLint viewport[4])
{
    if (width <= 0.0 || height <= 0.0) return;

    GLfloat tx = (GLfloat)((viewport[2] - 2.0 * (x - viewport[0])) / width);
    GLfloat ty = (GLfloat)((viewport[3] - 2.0 * (y - viewport[1])) / height);
    GLfloat sx = (GLfloat)(viewport[2] / width);
    GLfloat sy = (GLfloat)(viewport[3] / height);

    glTranslatef(tx, ty, 0.0f);
    glScalef(sx, sy, 1.0f);
}

#ifdef __cplusplus
}
#endif

#endif /* MYTINYGL_GLU_H */
