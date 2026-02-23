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

static inline void set_rgb_const(hdr16_rgb_t *dst, uint16_t v)
{
    dst->c[0] = v;
    dst->c[1] = v;
    dst->c[2] = v;
}

bool alloc_matcaps(MatcapSet *m)
{
    memset(m, 0, sizeof(*m));
    m->w = MATCAP_SIZE;
    m->h = MATCAP_SIZE;

    size_t texels = (size_t)m->w * (size_t)m->h;
    m->diffuse = malloc(sizeof(hdr16_rgb_t) * texels);
    m->rough25 = malloc(sizeof(hdr16_rgb_t) * texels);
    m->rough75 = malloc(sizeof(hdr16_rgb_t) * texels);

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

static inline void sample_equirect_nearest_u88(const hdr16_rgb_t *img, int w, int h, const fm_vec3_t *dir, hdr16_rgb_t *out_rgb)
{
    if (!img || w <= 0 || h <= 0) {
        set_rgb_const(out_rgb, U88_ONE);
        return;
    }

    float u = fm_atan2f(dir->z, dir->x) * HALF_INV_PI_F + 0.5f;
    float v = fast_acosf(dir->y) * INV_PI_F;

    int ix = wrap_x((int)(u * (float)w), w);
    int iy = clamp_y((int)(v * (float)h), h);

    *out_rgb = img[(size_t)iy * (size_t)w + (size_t)ix];
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
    /* R = reflect(-V, N), with V=(0,0,1) in view space. */
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

// function to evaluate a GGX light
// returns the vector of light intensity where each component is {roughness0, roughness1, diffuse}
// N - normal vector
// V - view vector
// L - light vector
// roughness - array of roughness values
// out - output vector
static void EvaluateGGX(
    const fm_vec3_t* N,
    const fm_vec3_t* V,
    const fm_vec3_t* L,
    const float roughness[2],
    fm_vec3_t* out)
{
    out->x = 0.0f;
    out->y = 0.0f;
    out->z = 0.0f;

    float NdotV = fm_vec3_dot(N, V);
    float NdotL = fm_vec3_dot(N, L);

    if (NdotV <= 0.0f || NdotL <= 0.0f) return;

    out->z = NdotL;

    fm_vec3_t H; fm_vec3_add(&H, V, L);
    float lenH2 = fm_vec3_dot(&H, &H);
    if (lenH2 <= 1e-8f) return;
    float invLenH = 1.0f / sqrtf(lenH2);
    float NdotH = fm_vec3_dot(N, &H) * invLenH;
    NdotH = clampf_local(NdotH, 0.0f, 1.0f);

    for (int i = 0; i < 2; i++) {
        float a  = roughness[i] * roughness[i];
        float a2 = a * a;

        float NdotH2 = NdotH * NdotH;
        float denom = NdotH2 * (a2 - 1.0f) + 1.0f;
        float D = a2 / (PI_F * denom * denom);

        // fresnel is handled at RSP postprocess
        //float oneMinus = 1.0f - VdotH;
        //float oneMinus2 = oneMinus * oneMinus;
        //float oneMinus4 = oneMinus2 * oneMinus2;
        //fm_vec3_t F = F0 + (1.0f - F0) * oneMinus4; // approximated to just 4-th power for one less multiply

        float k = (roughness[i] + 1.0f);
        k = (k * k) * 0.125f;

        float Gv = NdotV / (NdotV * (1.0f - k) + k);
        float Gl = NdotL / (NdotL * (1.0f - k) + k);
        float G = Gv * Gl;

        float brdf = (D * G) / (4.0f * NdotV * NdotL + 1e-6f);
        float lit = brdf * NdotL; /* final light contribution fades to 0 at grazing NdotL */
        if (i == 0) out->x = lit;
        else out->y = lit;
    }
}

void generate_matcaps_ggx(const CameraState *cam, const LightingState *lights, const HDRISet *hdri, MatcapSet *out)
{
    init_fresnel_masks(FRESNEL_F0_DEFAULT);

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

            /* Shared per-pixel forward vector in view space for custom BRDF code. */
            fm_vec3_t forward_view;

            bool valid = compute_forward_view(x, y, &forward_view);
            if (!valid) {
                set_rgb_const(&out->diffuse[idx], U88_ZERO);
                set_rgb_const(&out->rough25[idx], U88_ZERO);
                set_rgb_const(&out->rough75[idx], U88_ZERO);
                continue;
            }

            fm_vec3_t diffuse_dir_env = forward_view;
            rotate_y(&diffuse_dir_env, cos_yaw, sin_yaw);

            fm_vec3_t spec_dir_view = reflect_view_about_normal(&forward_view);
            fm_vec3_t spec_dir_env = spec_dir_view;
            rotate_y(&spec_dir_env, cos_yaw, sin_yaw);

            /*
             * Precomputed Fresnel masks for the current texel:
             * - float mask for float-domain code
             * - u8.8 mask for fixed-point multiply
             */
            float fresnel_f = g_fresnel_mask_f32[idx];
            uint16_t fresnel_u88 = g_fresnel_mask_u88[idx];

            /*
             * Diffuse and specular use different sampling coordinates:
             * - diffuse: surface normal direction
             * - specular: reflected view vector
             */
            sample_equirect_nearest_u88(hdri->diffuse, hdri->w, hdri->h, &diffuse_dir_env, &out->diffuse[idx]);
            sample_equirect_nearest_u88(hdri->rough25, hdri->w, hdri->h, &spec_dir_env, &out->rough25[idx]);
            sample_equirect_nearest_u88(hdri->rough75, hdri->w, hdri->h, &spec_dir_env, &out->rough75[idx]);

            for (int li = 0; li < light_count; li++) {
                fm_vec3_t L = {{
                    lights->dir[li][0],
                    lights->dir[li][1],
                    lights->dir[li][2]
                }};
                fm_vec3_norm(&L, &L);

                fm_vec3_t ggx = {{0.0f, 0.0f, 0.0f}};
                EvaluateGGX(&forward_view, &V, &L, roughness, &ggx);
                if (ggx.z <= 0.0f) continue;

                float spec25 = ggx.x;
                float spec75 = ggx.y;
                float diff_l = ggx.z;

                for (int c = 0; c < 3; c++) {
                    uint16_t d_add = quantize_u88_sat(lights->color[li][c] * diff_l);
                    uint16_t s25_add = quantize_u88_sat(lights->color[li][c] * spec25);
                    uint16_t s75_add = quantize_u88_sat(lights->color[li][c] * spec75);

                    out->diffuse[idx].c[c] = u88_add_sat(out->diffuse[idx].c[c], d_add);
                    out->rough25[idx].c[c] = u88_add_sat(out->rough25[idx].c[c], s25_add);
                    out->rough75[idx].c[c] = u88_add_sat(out->rough75[idx].c[c], s75_add);
                }
            }

            (void)fresnel_f;
            (void)fresnel_u88;
        }
    }
}
