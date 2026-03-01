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
#define INV_MATCAP_SIZE (1.0f / (float)MATCAP_SIZE)

static float g_fresnel_mask_f32[MATCAP_SIZE * MATCAP_SIZE];
static uint16_t g_fresnel_mask_u88[MATCAP_SIZE * MATCAP_SIZE];
static bool g_fresnel_masks_init = false;
static int g_hdri_sample_w = 0;
static int g_hdri_sample_h = 0;

static inline void set_hdr_const(hdr16_rgb_t *dst, uint16_t v)
{
    dst->c[0] = v;
    dst->c[1] = v;
    dst->c[2] = v;
}

static inline void set_matcap_const(matcap_rgba_t *dst, uint8_t rgb, uint8_t a)
{
    dst->c[0] = rgb;
    dst->c[1] = rgb;
    dst->c[2] = rgb;
    dst->c[3] = a;
}

bool alloc_matcaps(MatcapSet *m)
{
    memset(m, 0, sizeof(*m));
    m->w = MATCAP_SIZE;
    m->h = MATCAP_SIZE;

    size_t texels = (size_t)m->w * (size_t)m->h;
    m->diffuse = malloc(sizeof(matcap_rgba_t) * texels);
    m->rough25 = malloc(sizeof(matcap_rgba_t) * texels);
    m->rough75 = malloc(sizeof(matcap_rgba_t) * texels);

    if (!m->diffuse || !m->rough25 || !m->rough75) {
        free_matcaps(m);
        return false;
    }
    return true;
}

void free_matcaps(MatcapSet *m)
{
    if (!m) return;
    free(m->diffuse);
    free(m->rough25);
    free(m->rough75);
    m->diffuse = NULL;
    m->rough25 = NULL;
    m->rough75 = NULL;
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

static inline float fast_acosf(float x)
{
    x = clampf_local(x, -1.0f, 1.0f);
    float res = -0.6981317f * x;
    res *= x;
    res -= 0.8726646f;
    res *= x;
    res += 1.5707963f;
    return res;
}

static inline void dir_to_equirect_uv(const fm_vec3_t *dir, float *u, float *v)
{
    *u = fm_atan2f(dir->z, dir->x) * HALF_INV_PI_F + 0.5f;
    *v = fast_acosf(dir->y) * INV_PI_F;
}

static inline void sample_equirect_nearest_u88(const hdr16_rgb_t *img, float u, float v, hdr16_rgb_t *out_rgb)
{
    if (!img || g_hdri_sample_w <= 0 || g_hdri_sample_h <= 0) {
        set_hdr_const(out_rgb, U88_ONE);
        return;
    }

    int ix = wrap_x((int)(u * (float)g_hdri_sample_w), g_hdri_sample_w);
    int iy = clamp_y((int)(v * (float)g_hdri_sample_h), g_hdri_sample_h);
    *out_rgb = img[(size_t)iy * (size_t)g_hdri_sample_w + (size_t)ix];
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
    float nx = ((float)x + 0.5f) * INV_MATCAP_SIZE * 2.0f - 1.0f;
    float ny = 1.0f - (((float)y + 0.5f) * INV_MATCAP_SIZE * 2.0f);
    float rr = nx * nx + ny * ny;
    if (rr > 1.0f) {
        *out = (fm_vec3_t){{0.0f, 0.0f, 1.0f}};
        return false;
    }
    *out = (fm_vec3_t){{nx, ny, sqrtf(1.0f - rr)}};
    return true;
}

static inline uint16_t quantize_u88_sat(float v)
{
    if (v <= 0.0f) return U88_ZERO;
    float s = v * (float)U88_ONE + 0.5f;
    if (s >= (float)U88_MAX) return U88_MAX;
    return (uint16_t)s;
}

static inline uint8_t u88_to_u26_sat(uint16_t v)
{
    uint16_t q = (uint16_t)((v + 2u) >> 2);
    if (q > 255u) q = 255u;
    return (uint8_t)q;
}

static inline uint8_t u88_to_u08_sat(uint16_t v)
{
    if (v >= U88_ONE) return 255u;
    return (uint8_t)((((uint32_t)v * 255u) + 128u) >> 8);
}

static inline float fresnel_schlick(float ndotv, float f0)
{
    ndotv = clampf_local(ndotv, 0.0f, 1.0f);
    float x = 1.0f - ndotv;
    float x2 = x * x;
    float x4 = x2 * x2;
    float x5 = x4 * x;
    return f0 + (1.0f - f0) * x5;
}

static void init_fresnel_masks(float f0)
{
    if (g_fresnel_masks_init) return;

    for (int y = 0; y < MATCAP_SIZE; y++) {
        for (int x = 0; x < MATCAP_SIZE; x++) {
            size_t idx = (size_t)y * (size_t)MATCAP_SIZE + (size_t)x;

            fm_vec3_t forward_view;
            bool valid = compute_forward_view(x, y, &forward_view);
            if (!valid) {
                g_fresnel_mask_f32[idx] = 0.0f;
                g_fresnel_mask_u88[idx] = 0;
                continue;
            }

            float fres = fresnel_schlick(forward_view.z, f0);
            g_fresnel_mask_f32[idx] = fres;
            g_fresnel_mask_u88[idx] = (uint16_t)(fres * (float)U88_ONE + 0.5f);
        }
    }

    g_fresnel_masks_init = true;
}

void build_camera_from_yaw(float yaw, CameraState *cam)
{
    fm_vec3_t f = {{fm_cosf(yaw), 0.0f, fm_sinf(yaw)}};
    fm_vec3_norm(&f, &f);
    cam->forward[0] = f.x;
    cam->forward[1] = f.y;
    cam->forward[2] = f.z;
}

// function to evaluate a GGX light intensity
// returns the vector of light intensity where each component is {roughness0, roughness1, diffuse}
// N - normal vector
// V - view vector
// L - light vector
// roughness - array of roughness values
// out - output vector
static void EvaluateGGX(const fm_vec3_t *N, const fm_vec3_t *V, const fm_vec3_t *L, const float roughness[2], fm_vec3_t *out)
{
    out->x = 0.0f;
    out->y = 0.0f;
    out->z = 0.0f;

    float NdotV = fm_vec3_dot(N, V);
    float NdotL = fm_vec3_dot(N, L);
    if (NdotV <= 0.0f || NdotL <= 0.0f) return;

    out->z = NdotL;

    fm_vec3_t H;
    fm_vec3_add(&H, V, L);
    float lenH2 = fm_vec3_dot(&H, &H);
    if (lenH2 <= 1e-8f) return;

    float invLenH = 1.0f / sqrtf(lenH2);
    float NdotH = clampf_local(fm_vec3_dot(N, &H) * invLenH, 0.0f, 1.0f);

    for (int i = 0; i < 2; i++) {
        float a = roughness[i] * roughness[i];
        float a2 = a * a;
        float NdotH2 = NdotH * NdotH;
        float denom = NdotH2 * (a2 - 1.0f) + 1.0f;
        float D = a2 / (PI_F * denom * denom + 1e-6f);

        float k = roughness[i] + 1.0f;
        k = (k * k) * 0.125f;

        float Gv = NdotV / (NdotV * (1.0f - k) + k);
        float Gl = NdotL / (NdotL * (1.0f - k) + k);
        float G = Gv * Gl;

        float brdf = (D * G) / (4.0f * NdotV * NdotL + 1e-6f);
        float lit = brdf * NdotL;
        if (i == 0) out->x = lit;
        else out->y = lit;
    }
}

void generate_matcaps(const CameraState *cam, const LightingState *lights, const HDRISet *hdri, MatcapSet *out)
{
    init_fresnel_masks(FRESNEL_F0_DEFAULT);
    g_hdri_sample_w = hdri ? hdri->w : 0;
    g_hdri_sample_h = hdri ? hdri->h : 0;

    float yaw = fm_atan2f(cam->forward[2], cam->forward[0]);
    float sin_yaw, cos_yaw;
    fm_sincosf(yaw, &sin_yaw, &cos_yaw);

    const fm_vec3_t V = {{0.0f, 0.0f, 1.0f}};
    const float roughness[2] = {0.25f, 0.75f};
    int light_count = lights ? lights->count : 0;
    if (light_count > 4) light_count = 4;

    for (int y = 0; y < MATCAP_SIZE; y++) {
        for (int x = 0; x < MATCAP_SIZE; x++) {
            size_t idx = (size_t)y * (size_t)MATCAP_SIZE + (size_t)x;

            fm_vec3_t forward_view;
            if (!compute_forward_view(x, y, &forward_view)) {
                set_matcap_const(&out->diffuse[idx], 0u, 0u);
                set_matcap_const(&out->rough25[idx], 0u, 0u);
                set_matcap_const(&out->rough75[idx], 0u, 0u);
                continue;
            }

            fm_vec3_t diffuse_dir_env = forward_view;
            rotate_y(&diffuse_dir_env, cos_yaw, sin_yaw);

            fm_vec3_t spec_dir_view = reflect_view_about_normal(&forward_view);
            fm_vec3_t spec_dir_env = spec_dir_view;
            rotate_y(&spec_dir_env, cos_yaw, sin_yaw);

            float u_diff, v_diff;
            float u_spec, v_spec;
            dir_to_equirect_uv(&diffuse_dir_env, &u_diff, &v_diff);
            dir_to_equirect_uv(&spec_dir_env, &u_spec, &v_spec);

            hdr16_rgb_t env_diff;
            hdr16_rgb_t env_s25;
            hdr16_rgb_t env_s75;
            sample_equirect_nearest_u88(hdri->diffuse, u_diff, v_diff, &env_diff);
            sample_equirect_nearest_u88(hdri->rough25, u_spec, v_spec, &env_s25);
            sample_equirect_nearest_u88(hdri->rough75, u_spec, v_spec, &env_s75);

            uint16_t diff_acc[3] = {env_diff.c[0], env_diff.c[1], env_diff.c[2]};
            uint16_t s25_acc[3] = {env_s25.c[0], env_s25.c[1], env_s25.c[2]};
            uint16_t s75_acc[3] = {env_s75.c[0], env_s75.c[1], env_s75.c[2]};

            for (int li = 0; li < light_count; li++) {
                fm_vec3_t L = {{
                    lights->dir[li][0],
                    lights->dir[li][1],
                    lights->dir[li][2]
                }};

                fm_vec3_t ggx;
                EvaluateGGX(&forward_view, &V, &L, roughness, &ggx);
                if (ggx.z <= 0.0f) continue;

                for (int c = 0; c < 3; c++) {
                    uint16_t d_add = quantize_u88_sat(lights->color[li][c] * ggx.z);
                    uint16_t s25_add = quantize_u88_sat(lights->color[li][c] * ggx.x);
                    uint16_t s75_add = quantize_u88_sat(lights->color[li][c] * ggx.y);
                    diff_acc[c] = u88_add_sat(diff_acc[c], d_add);
                    s25_acc[c] = u88_add_sat(s25_acc[c], s25_add);
                    s75_acc[c] = u88_add_sat(s75_acc[c], s75_add);
                }
            }

            uint16_t fres_u88 = g_fresnel_mask_u88[idx];
            uint8_t alpha_spec = u88_to_u08_sat(fres_u88);
            uint8_t alpha_diff = u88_to_u08_sat(u88_sub_sat(U88_ONE, fres_u88));

            for (int c = 0; c < 3; c++) {
                out->diffuse[idx].c[c] = u88_to_u26_sat(diff_acc[c]);
                out->rough25[idx].c[c] = u88_to_u26_sat(s25_acc[c]);
                out->rough75[idx].c[c] = u88_to_u26_sat(s75_acc[c]);
            }
            out->diffuse[idx].c[3] = alpha_diff;
            out->rough25[idx].c[3] = alpha_spec;
            out->rough75[idx].c[3] = alpha_spec;
        }
    }
}
