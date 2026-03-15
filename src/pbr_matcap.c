#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <libdragon.h>

#include "pbr_matcap.h"
#include "pbr_u88.h"

#define PI_F 3.14159265358979323846f
#define INV_PI_F (1.0f / PI_F)
#define HALF_INV_PI_F (0.5f / PI_F)
#define FRESNEL_F0_DEFAULT 0.04f
#define INV_MATCAP_W (1.0f / (float)MATCAP_W)
#define INV_MATCAP_H (1.0f / (float)MATCAP_H)

#define COLOR_COMPONENTS 3
#define MATCAP_COUNT 3

static uint8_t g_fresnel_mask_u8[MATCAP_TEXELS];
static bool g_fresnel_masks_init = false;

static inline void set_hdr_const(hdri_color_t *dst, uint16_t v)
{
    for(int i = 0; i < COLOR_COMPONENTS; i++)
        dst->c[i] = v;
}

static inline void set_matcap_const(matcap_color_t* dst, uint8_t rgb, uint8_t a)
{
    for(int i = 0; i < COLOR_COMPONENTS; i++)
        dst->c[i] = rgb;
    dst->c[3] = a;
}

bool alloc_matcaps(MatcapSet *m)
{
    memset(m, 0, sizeof(*m));
    m->w = MATCAP_W;
    m->h = MATCAP_H;

    size_t texels = (size_t)m->w * (size_t)m->h;
    m->diffuse = malloc(sizeof(matcap_color_t) * texels);
    m->spec25 = malloc(sizeof(matcap_color_t) * texels);
    m->spec75 = malloc(sizeof(matcap_color_t) * texels);

    if (!m->diffuse || !m->spec25 || !m->spec75) {
        free_matcaps(m);
        return false;
    }
    return true;
}

void free_matcaps(MatcapSet *m)
{
    if (!m) return;
    free(m->diffuse);
    free(m->spec25);
    free(m->spec75);
    m->diffuse = NULL;
    m->spec25 = NULL;
    m->spec75 = NULL;
    m->w = 0;
    m->h = 0;
}

static inline float clampf_local(float x, float lo, float hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static inline int wrap_x(int x, int w)
{
    x %= w;
    if (x < 0) x += w;
    return x;
}

static inline int clamp_y(int y, int h)
{
    if (y < 0) return 0;
    if (y >= h) return h - 1;
    return y;
}

static inline void  dir_to_paraboloid_uv(const fm_vec3_t *dir, float *u, float *v)
{
    float m;
    float uu, vv;

    if (dir->z >= 0.0f)
    {
        // front hemisphere
        m = 1.0f / (1.0f + dir->z);

        uu = 0.5f * (dir->x * m + 1.0f);
        vv = 0.5f * (-dir->y * m + 1.0f);

        *u = uu * 0.5f  + 0.5;
        *v = vv;
    }
    else
    {
        // back hemisphere
        m = 1.0f / (1.0f - dir->z);

        uu = 0.5f * (-dir->x * m + 1.0f);
        vv = 0.5f * (dir->y * m + 1.0f);

        *u = uu * 0.5f;
        *v = vv;
    }
}

static inline void hdri_sample_equirect_nearest(const hdri_color_t *img,
                                                int w,
                                                int h,
                                                float u,
                                                float v,
                                                hdri_color_t *out_rgb)
{
    if (!img || w <= 0 || h <= 0) {
        set_hdr_const(out_rgb, U88_ONE);
        return;
    }

    int ix = wrap_x((int)(u * (float)w), w);
    int iy = clamp_y((int)(v * (float)h), h);
    *out_rgb = img[(size_t)iy * (size_t)w + (size_t)ix];
}

static inline void hdri_sample_specular_pair_equirect_nearest(const HDRISet *hdri,
                                                               float u,
                                                               float v,
                                                               hdri_color_t *out_s25,
                                                               hdri_color_t *out_s75)
{
    if (!hdri || !hdri->specular_interleaved || hdri->specular_w <= 0 || hdri->specular_h <= 0) {
        set_hdr_const(out_s25, U88_ONE);
        set_hdr_const(out_s75, U88_ONE);
        return;
    }

    int ix = wrap_x((int)(u * (float)hdri->specular_w), hdri->specular_w);
    int iy = clamp_y((int)(v * (float)hdri->specular_h), hdri->specular_h);
    size_t base = ((size_t)iy * (size_t)hdri->specular_w + (size_t)ix) * 2u;
    *out_s25 = hdri->specular_interleaved[base + 0u];
    *out_s75 = hdri->specular_interleaved[base + 1u];
}

static inline void rotate_y(fm_vec3_t *v, float cos_a, float sin_a)
{
    float x = v->x;
    float z = v->z;
    v->x = x * cos_a - z * sin_a;
    v->z = x * sin_a + z * cos_a;
}

static inline fm_vec3_t reflect_view_about_normal(const fm_vec3_t *n_view)
{
    float ndotv = n_view->z;
    fm_vec3_t r = {{
        2.0f * ndotv * n_view->x,
        2.0f * ndotv * n_view->y,
        2.0f * ndotv * ndotv - 1.0f
    }};
    return r;
}

static inline bool compute_forward_view(int x, int y, fm_vec3_t *out)
{
    // Transposed matcap layout (16x32): normal.y maps to texel X, normal.x maps to texel Y.
    int x_flip = (MATCAP_W - 1) - x;
    float nx = ((float)y + 0.5f) * INV_MATCAP_H * 2.0f - 1.0f;
    float ny = 1.0f - (((float)x_flip + 0.5f) * INV_MATCAP_W * 2.0f);
    float rr = nx * nx + ny * ny;
    if (rr > 1.0f) {
        *out = (fm_vec3_t){{0.0f, 0.0f, 1.0f}};
        return false;
    }
    *out = (fm_vec3_t){{nx, ny, sqrtf(1.0f - rr)}};
    return true;
}

static void init_fresnel_masks(float f0)
{
    if (g_fresnel_masks_init) return;

    for (int y = 0; y < MATCAP_H; y++) {
        for (int x = 0; x < MATCAP_W; x++) {
            size_t idx = (size_t)y * (size_t)MATCAP_W + (size_t)x;

            fm_vec3_t forward_view;
            bool valid = compute_forward_view(x, y, &forward_view);
            if (!valid) {
                g_fresnel_mask_u8[idx] = 255;
                continue;
            }

            float x = sqrtf((forward_view.x * forward_view.x) + (forward_view.y * forward_view.y));
            x = clampf_local(x, 0.0f, 1.0f);
            float x2 = x * x;
            float x4 = x2 * x2;
            float x5 = x4 * x;
            float fres = f0 + (1.0f - f0) * x5;
            g_fresnel_mask_u8[idx] = fres * 255.0f;
        }
    }

    g_fresnel_masks_init = true;
}

// function to evaluate a GGX light intensity
// returns the vector of light intensity where each component is {diffuse, specness0, specness1}
// N - normal vector
// V - view vector
// L - light vector
// specness - array of specness values
// out - output vector
static void EvaluateGGX(const fm_vec3_t *N, const fm_vec3_t *V, const fm_vec3_t *L, const float specness[2], fm_vec3_t *out)
{
    for(int i = 0; i < 3; i++)
        out->v[i] = 0.0f;

    float NdotV = fm_vec3_dot(N, V);
    float NdotL = fm_vec3_dot(N, L);
    if (NdotV <= 0.0f || NdotL <= 0.0f) return;

    out->x = NdotL;

    fm_vec3_t H;
    fm_vec3_add(&H, V, L);
    float lenH2 = fm_vec3_dot(&H, &H);
    if (lenH2 <= 1e-8f) return;

    float invLenH = 1.0f / sqrtf(lenH2);
    float NdotH = clampf_local(fm_vec3_dot(N, &H) * invLenH, 0.0f, 1.0f);

    for (int i = 0; i < 2; ) {
        float a = specness[i] * specness[i];
        float a2 = a * a;
        float NdotH2 = NdotH * NdotH;
        float denom = NdotH2 * (a2 - 1.0f) + 1.0f;
        float D = a2 / (PI_F * denom * denom + 1e-6f);

        float k = specness[i] + 1.0f;
        k = (k * k) * 0.125f;

        float Gv = NdotV / (NdotV * (1.0f - k) + k);
        float Gl = NdotL / (NdotL * (1.0f - k) + k);
        float G = Gv * Gl;

        float brdf = (D * G) / (4.0f * NdotV * NdotL + 1e-6f);
        float lit = brdf * NdotL;
        i++;
        out->v[i] = lit;
    }
}

void generate_matcaps(const CameraState *cam, const LightingState *lights, const HDRISet *hdri, MatcapSet *out)
{
    LightingState lightingstate = *lights;
    float exposure, hdri_strength, emission_strength;
    // RGB is u3.5, so 1.0 == 64.
    exposure = lightingstate.exposure * (64.0f);
    hdri_strength = lightingstate.hdri_strength * (1.0f / 8.0f / 64.0f);
    emission_strength = lightingstate.emission_strength * lightingstate.exposure * (64.0f);

    init_fresnel_masks(FRESNEL_F0_DEFAULT);
    matcap_color_t *out_maps[3] = {out->diffuse, out->spec25, out->spec75};

    fm_vec3_t cam_forward = {{
        cam->forward[0],
        -cam->forward[1],
        cam->forward[2]
    }};
    fm_vec3_norm(&cam_forward, &cam_forward);

    fm_vec3_t world_up = {{0.0f, 1.0f, 0.0f}};
    fm_vec3_t cam_right;
    fm_vec3_cross(&cam_right, &world_up, &cam_forward);
    fm_vec3_norm(&cam_right, &cam_right);

    fm_vec3_t cam_up;
    fm_vec3_cross(&cam_up, &cam_forward, &cam_right);

    const fm_vec3_t V = {{0.0f, 0.0f, 1.0f}};
    const float specness[2] = {0.25f, 0.75f};
    int light_count = lights ? lights->count : 0;
    if (light_count > 4) light_count = 4;

    matcap_color_t outside_px[3] = {0};
    // compute one color for pixels outside the RR, we assume that the direction for them is -forward
    {
        fm_vec3_t outside_dir_env = {{
            -cam_forward.x,
            -cam_forward.y,
            -cam_forward.z
        }};
        fm_vec3_norm(&outside_dir_env, &outside_dir_env);

        float u_diff = 0, v_diff = 0;

        // paraboloid is not exactly equirectangular, but at 16x32 it's close enough, and its a lot faster
        dir_to_paraboloid_uv(&outside_dir_env, &u_diff, &v_diff);

        // {diffuse, s25, s75}
        float_color_t totals[3] = {0};
        hdri_color_t hdri_values[3] = {0};

        hdri_sample_equirect_nearest(hdri->diffuse, hdri->diffuse_w, hdri->diffuse_h, u_diff, v_diff, &hdri_values[0]);
        hdri_sample_specular_pair_equirect_nearest(hdri, u_diff, v_diff, &hdri_values[1], &hdri_values[2]);

        for(int k = 0; k < MATCAP_COUNT; k++) {
            for (int c = 0; c < COLOR_COMPONENTS; c++) {
                totals[k].c[c] = hdri_values[k].c[c] * hdri_strength;
            }
        }

        for (int li = 0; li < light_count; li++) {
            fm_vec3_t L_world = {{
                lightingstate.dir[li][0],
                lightingstate.dir[li][1],
                lightingstate.dir[li][2]
            }};
            fm_vec3_norm(&L_world, &L_world);

            fm_vec3_t L_view = {{
                fm_vec3_dot(&L_world, &cam_right),
                fm_vec3_dot(&L_world, &cam_up),
                fm_vec3_dot(&L_world, &cam_forward)
            }};

            fm_vec3_t ggx;
            EvaluateGGX(&(fm_vec3_t){{0.0f, 0.0f, 1.0f}}, &V, &L_view, specness, &ggx);
            if (ggx.z <= 0.0f) continue;

            for(int k = 0; k < MATCAP_COUNT; k++) {
                for (int c = 0; c < COLOR_COMPONENTS; c++) {
                    totals[k].c[c] += (lightingstate.color[li][c] * ggx.v[k]);
                }
            }
        }

        for(int k = 0; k < MATCAP_COUNT; k++) {
            for (int c = 0; c < COLOR_COMPONENTS; c++) {
                totals[k].c[c] = fminf(totals[k].c[c] * 8 * exposure, 255.0f);
                outside_px[k].c[c] = (uint8_t)(totals[k].c[c]);
            }
        }
    }

    for (int y = 0; y < MATCAP_H; y++) {
        for (int x = 0; x < MATCAP_W; x++) {
            size_t idx = (size_t)y * (size_t)MATCAP_W + (size_t)x;

            fm_vec3_t forward_view;
            if (!compute_forward_view(x, y, &forward_view)) {
                for (int m = 0; m < MATCAP_COUNT; m++) {
                    out_maps[m][idx] = outside_px[m];
                }
                continue;
            }

            fm_vec3_t diffuse_dir_env = {{
                cam_right.x * forward_view.x + cam_up.x * forward_view.y + cam_forward.x * forward_view.z,
                cam_right.y * forward_view.x + cam_up.y * forward_view.y + cam_forward.y * forward_view.z,
                cam_right.z * forward_view.x + cam_up.z * forward_view.y + cam_forward.z * forward_view.z
            }};

            fm_vec3_t spec_dir_view = reflect_view_about_normal(&forward_view);
            fm_vec3_t spec_dir_env = {{
                cam_right.x * spec_dir_view.x + cam_up.x * spec_dir_view.y + cam_forward.x * spec_dir_view.z,
                cam_right.y * spec_dir_view.x + cam_up.y * spec_dir_view.y + cam_forward.y * spec_dir_view.z,
                cam_right.z * spec_dir_view.x + cam_up.z * spec_dir_view.y + cam_forward.z * spec_dir_view.z
            }};

            float u_diff = 0, v_diff = 0;
            float u_spec = 0, v_spec = 0;

            // paraboloid is not exactly equirectangular, but at 16x32 it's close enough, and its a lot faster
            dir_to_paraboloid_uv(&diffuse_dir_env, &u_diff, &v_diff);
            dir_to_paraboloid_uv(&spec_dir_env, &u_spec, &v_spec);

            // {diffuse, s25, s75}
            float_color_t totals[3] = {0};
            hdri_color_t hdri_values[3] = {0};

            hdri_sample_equirect_nearest(hdri->diffuse, hdri->diffuse_w, hdri->diffuse_h, u_diff, v_diff, &hdri_values[0]);
            hdri_sample_specular_pair_equirect_nearest(hdri, u_spec, v_spec, &hdri_values[1], &hdri_values[2]);

            for(int k = 0; k < MATCAP_COUNT; k++) {
                for (int c = 0; c < COLOR_COMPONENTS; c++) {
                    totals[k].c[c] = hdri_values[k].c[c] * hdri_strength;
                }
            }

            for (int li = 0; li < light_count; li++) {
                fm_vec3_t L_world = {{
                    lightingstate.dir[li][0],
                    lightingstate.dir[li][1],
                    lightingstate.dir[li][2]
                }};
                fm_vec3_norm(&L_world, &L_world);

                fm_vec3_t L_view = {{
                    fm_vec3_dot(&L_world, &cam_right),
                    fm_vec3_dot(&L_world, &cam_up),
                    fm_vec3_dot(&L_world, &cam_forward)
                }};

                fm_vec3_t ggx;
                EvaluateGGX(&forward_view, &V, &L_view, specness, &ggx);
                if (ggx.z <= 0.0f) continue;

                for(int k = 0; k < MATCAP_COUNT; k++) {
                    for (int c = 0; c < COLOR_COMPONENTS; c++) {
                        totals[k].c[c] += (lightingstate.color[li][c] * ggx.v[k]);
                    }
                }
            }

            for(int k = 0; k < MATCAP_COUNT; k++) {
                for (int c = 0; c < COLOR_COMPONENTS; c++) {
                    totals[k].c[c] = fminf(totals[k].c[c] * exposure, 255.0f);
                    out_maps[k][idx].c[c] = (uint8_t)(totals[k].c[c]);
                }
            }
            // alpha of the diffuse is the fresnel coefficient from f0 to 255=1.0 on the edges
            out_maps[0][idx].c[3] = g_fresnel_mask_u8[idx];
        }
    }

    // (0,0) is used as the emission/passthspec texel when the packed normal is zero.

    uint8_t emission = (uint8_t)fminf(emission_strength, 255.0f);
    set_matcap_const(&out->diffuse[0], emission, 0u);
    set_matcap_const(&out->spec25[0], 0u, 0u);
    set_matcap_const(&out->spec75[0], 0u, 0u);

}
