/*
 * MyTinyGL - OpenGL 1.x Software Renderer
 * Copyright (c) 2025 zbufferoverflow (Eliezer Solinger)
 * https://github.com/zbufferoverflow/MyTinyGL
 * SPDX-License-Identifier: MIT
 *
 * lighting.h - Lighting calculations
 */

#ifndef MYTINYGL_LIGHTING_H
#define MYTINYGL_LIGHTING_H

#include "graphics.h"
#include "mytinygl.h"
#include <math.h>

/* Initialize a light with OpenGL defaults */
static inline void light_init(light_t *light, int index)
{
    light->ambient  = color(0, 0, 0, 1);
    /* GL_LIGHT0 has white diffuse/specular, others are black */
    light->diffuse  = (index == 0) ? color(1, 1, 1, 1) : color(0, 0, 0, 1);
    light->specular = (index == 0) ? color(1, 1, 1, 1) : color(0, 0, 0, 1);
    light->position = vec4(0, 0, 1, 0);  /* Directional by default */
    light->spot_direction = vec3(0, 0, -1);
    light->spot_exponent = 0;
    light->spot_cutoff = 180;
    light->constant_attenuation = 1;
    light->linear_attenuation = 0;
    light->quadratic_attenuation = 0;
    light->enabled = GL_FALSE;
}

/* Initialize a material with OpenGL defaults */
static inline void material_init(material_t *mat)
{
    mat->ambient   = color(0.2f, 0.2f, 0.2f, 1);
    mat->diffuse   = color(0.8f, 0.8f, 0.8f, 1);
    mat->specular  = color(0, 0, 0, 1);
    mat->emission  = color(0, 0, 0, 1);
    mat->shininess = 0;
}

/*
 * Compute lighting for a single vertex/fragment.
 *
 * eye_pos: Position in eye-space
 * eye_normal: Normal in eye-space (should be normalized)
 * mat: Material properties to use
 *
 * Returns: Lit color (RGB components, alpha from material diffuse)
 */
static inline color_t compute_lighting(GLState *ctx, vec3_t eye_pos, vec3_t eye_normal, material_t *mat)
{
    /* Start with emission + global ambient */
    color_t result = color_add(mat->emission, color_mul(mat->ambient, ctx->light_model_ambient));
    result.a = mat->diffuse.a;

    /* Normalize normal if enabled */
    vec3_t N = eye_normal;
    if (ctx->flags & FLAG_NORMALIZE) {
        N = vec3_normalize(N);
    }

    /* Process each light */
    for (int i = 0; i < MAX_LIGHTS; i++) {
        if (!ctx->lights[i].enabled) continue;

        light_t *light = &ctx->lights[i];

        /* Light direction and attenuation */
        vec3_t L;
        float attenuation = 1.0f;

        if (light->position.w == 0.0f) {
            /* Directional light */
            L = vec3_normalize(vec3(light->position.x, light->position.y, light->position.z));
        } else {
            /* Positional light */
            vec3_t light_pos = vec3(light->position.x, light->position.y, light->position.z);
            vec3_t to_light = vec3_sub(light_pos, eye_pos);
            float dist = vec3_length(to_light);

            /* Avoid division by zero when light is at vertex position */
            if (dist < 1e-6f) {
                dist = 1e-6f;
            }
            L = vec3_scale(to_light, 1.0f / dist);

            /* Attenuation - ensure denominator is never zero */
            float atten_denom = light->constant_attenuation +
                                light->linear_attenuation * dist +
                                light->quadratic_attenuation * dist * dist;
            if (atten_denom < 1e-6f) {
                atten_denom = 1e-6f;
            }
            attenuation = 1.0f / atten_denom;

            /* Spotlight */
            if (light->spot_cutoff < 180.0f) {
                vec3_t spot_dir = vec3_normalize(light->spot_direction);
                float cos_angle = -vec3_dot(L, spot_dir);
                float cos_cutoff = cosf(light->spot_cutoff * 3.14159265f / 180.0f);

                if (cos_angle < cos_cutoff) {
                    attenuation = 0.0f;
                } else {
                    attenuation *= powf(cos_angle, light->spot_exponent);
                }
            }
        }

        if (attenuation <= 0.0f) continue;

        /* Ambient contribution */
        result = color_add(result, color_scale(color_mul(mat->ambient, light->ambient), attenuation));

        /* Diffuse contribution */
        float NdotL = vec3_dot(N, L);
        if (NdotL > 0.0f) {
            result = color_add(result, color_scale(color_mul(mat->diffuse, light->diffuse), attenuation * NdotL));

            /* Specular contribution (Blinn-Phong) */
            if (mat->shininess > 0.0f) {
                vec3_t V = ctx->light_model_local_viewer
                    ? vec3_normalize(vec3_scale(eye_pos, -1.0f))
                    : vec3(0.0f, 0.0f, 1.0f);
                vec3_t H = vec3_normalize(vec3_add(L, V));
                float NdotH = vec3_dot(N, H);

                if (NdotH > 0.0f) {
                    float spec = powf(NdotH, mat->shininess) * attenuation;
                    result = color_add(result, color_scale(color_mul(mat->specular, light->specular), spec));
                }
            }
        }
    }

    /* Clamp result */
    result = color_clamp(result);
    return result;
}

#endif /* MYTINYGL_LIGHTING_H */
