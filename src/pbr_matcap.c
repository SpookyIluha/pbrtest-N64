#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <libdragon.h>

#include "pbr_matcap.h"

#define PI_F 3.14159265358979323846f
#define FIXED_ONE 256.0f
#define INV_PI_F (1.0f / PI_F)
#define HALF_INV_PI_F (0.5f / PI_F)
#define INV_MATCAP_SIZE (1.0f / (float)MATCAP_SIZE)

typedef struct {
    bool valid;
    fm_vec3_t n;
} MatcapTexelCache;

static MatcapTexelCache g_texel_cache[MATCAP_SIZE * MATCAP_SIZE];
static bool g_texel_cache_init = false;
static inline uint16_t quantize_hdr(float v);

static inline void set_rgb_const(hdr16_rgb_t *dst, uint16_t v)
{
    for (int c = 0; c < 3; c++) dst->c[c] = v;
}

static inline void set_rgb_quantized(hdr16_rgb_t *dst, const float src[3])
{
    for (int c = 0; c < 3; c++) dst->c[c] = quantize_hdr(src[c]);
}

bool alloc_matcaps(MatcapSet *m)
{
    memset(m, 0, sizeof(*m));
    m->w = MATCAP_SIZE;
    m->h = MATCAP_SIZE;
    size_t texels = (size_t)m->w * (size_t)m->h;
    size_t chain_texels = 8u * texels;

    m->diffuse = malloc(sizeof(hdr16_rgb_t) * texels);
    if (!m->diffuse) return false;

    m->spec = malloc(sizeof(hdr16_rgb_t) * chain_texels);
    m->spec_mod = malloc(sizeof(hdr16_rgb_t) * chain_texels);
    m->spec_fres = malloc(sizeof(hdr16_rgb_t) * chain_texels);
    if (!m->spec || !m->spec_mod || !m->spec_fres) return false;

    return true;
}

void free_matcaps(MatcapSet *m)
{
    if (!m) return;
    free(m->diffuse);
    m->diffuse = NULL;
    free(m->spec);
    free(m->spec_mod);
    free(m->spec_fres);
    m->spec = NULL;
    m->spec_mod = NULL;
    m->spec_fres = NULL;
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

static inline void sample_equirect_nearest(const float *img, int w, int h, const fm_vec3_t *dir, float out_rgb[3])
{
    float u = fm_atan2f(dir->z, dir->x) * HALF_INV_PI_F + 0.5f;
    float v = fast_acosf(dir->y) * INV_PI_F;

    int ix = wrap_x((int)(u * (float)w), w);
    int iy = clamp_y((int)(v * (float)h), h);

    const float *s = &img[((size_t)iy * (size_t)w + (size_t)ix) * 3u];
    out_rgb[0] = s[0];
    out_rgb[1] = s[1];
    out_rgb[2] = s[2];
}

static inline float ggx_G1(float NdotX, float a)
{
    float a2 = a * a;
    float denom = NdotX + sqrtf(a2 + (1.0f - a2) * NdotX * NdotX);
    return (2.0f * NdotX) / (denom + 1e-6f);
}

static inline float fresnel_schlick_scalar(float VdotH, float F0)
{
    float f = 1.0f - clampf_local(VdotH, 0.0f, 1.0f);
    float f2 = f * f;
    float f4 = f2 * f2;
    return F0 + (1.0f - F0) * f4;
}

static inline uint16_t quantize_hdr(float v)
{
    float scaled = v * FIXED_ONE;
    if (scaled < 0.0f) return 0;
    if (scaled > 65535.0f) return 65535;
    return (uint16_t)(scaled);
}

static inline void rotate_y(fm_vec3_t *v, float cos_a, float sin_a)
{
    float x = v->x;
    float z = v->z;
    v->x = x * cos_a - z * sin_a;
    v->z = x * sin_a + z * cos_a;
}

static void init_texel_cache(void)
{
    if (g_texel_cache_init) return;

    for (int y = 0; y < MATCAP_SIZE; y++) {
        for (int x = 0; x < MATCAP_SIZE; x++) {
            int idx = y * MATCAP_SIZE + x;
            float nx = ((float)x + 0.5f) * INV_MATCAP_SIZE * 2.0f - 1.0f;
            float ny = 1.0f - (((float)y + 0.5f) * INV_MATCAP_SIZE * 2.0f);
            float rr = nx * nx + ny * ny;

            if (rr > 1.0f) {
                g_texel_cache[idx].valid = false;
                g_texel_cache[idx].n = (fm_vec3_t){{0.0f, 0.0f, 1.0f}};
            } else {
                g_texel_cache[idx].valid = true;
                g_texel_cache[idx].n = (fm_vec3_t){{nx, ny, sqrtf(1.0f - rr)}};
            }
        }
    }

    g_texel_cache_init = true;
}

void build_camera_from_yaw(float yaw, CameraState *cam)
{
    fm_vec3_t f = {{fm_cosf(yaw), 0.0f, fm_sinf(yaw)}};
    fm_vec3_norm(&f, &f);
    cam->forward[0] = f.x;
    cam->forward[1] = f.y;
    cam->forward[2] = f.z;
}

void generate_matcaps_ggx(const CameraState *cam, const LightingState *lights, const HDRISet *hdri, MatcapSet *out)
{
    init_texel_cache();

    float anchor_rough[4] = {0.12f, 0.22f, 0.52f, 0.75f};
    float fresnel_lut[MATCAP_SIZE * MATCAP_SIZE];
    float anchor_spec[4][MATCAP_SIZE * MATCAP_SIZE][3];
    fm_vec3_t light_dir[4];
    fm_vec3_t light_H[4];
    float light_vdoth[4];
    int light_count = lights->count;

    if (light_count > 4) light_count = 4;

    memset(anchor_spec, 0, sizeof(anchor_spec));
    memset(fresnel_lut, 0, sizeof(fresnel_lut));

    float yaw = fm_atan2f(cam->forward[2], cam->forward[0]);
    float sin_yaw, cos_yaw;
    fm_sincosf(yaw, &sin_yaw, &cos_yaw);

    for (int li = 0; li < light_count; li++) {
        light_dir[li] = (fm_vec3_t){{lights->dir[li][0], lights->dir[li][1], lights->dir[li][2]}};
        fm_vec3_norm(&light_dir[li], &light_dir[li]);

        fm_vec3_t view_vec = {{0.0f, 0.0f, 1.0f}};
        fm_vec3_add(&light_H[li], &light_dir[li], &view_vec);
        fm_vec3_norm(&light_H[li], &light_H[li]);
        light_vdoth[li] = clampf_local(light_H[li].z, 0.0f, 1.0f);
    }

    for (int idx = 0; idx < MATCAP_SIZE * MATCAP_SIZE; idx++) {
        if (!g_texel_cache[idx].valid) {
            set_rgb_const(&out->diffuse[idx], (uint16_t)FIXED_ONE);
            for (int r = 0; r < 8; r++) {
                int chain_idx = (r << 10) | idx;
                set_rgb_const(&out->spec[chain_idx], (uint16_t)FIXED_ONE);
                set_rgb_const(&out->spec_mod[chain_idx], (uint16_t)FIXED_ONE);
                set_rgb_const(&out->spec_fres[chain_idx], (uint16_t)FIXED_ONE);
            }
            continue;
        }

        const fm_vec3_t *N = &g_texel_cache[idx].n;
        float nx = N->x;
        float ny = N->y;
        float nz = N->z;
        float NdotV = nz;

        fm_vec3_t N_env = *N;
        rotate_y(&N_env, cos_yaw, sin_yaw);

        float env_d[3];
        sample_equirect_nearest(hdri->diffuse_irr ? hdri->diffuse_irr : hdri->rgb, hdri->w, hdri->h, &N_env, env_d);

        float diff[3] = {env_d[0], env_d[1], env_d[2]};
        float ndotl_lut[4] = {0};
        float ndoth_lut[4] = {0};

        for (int li = 0; li < light_count; li++) {
            float ndotl = fm_vec3_dot(N, &light_dir[li]);
            if (ndotl < 0.0f) ndotl = 0.0f;
            ndotl_lut[li] = ndotl;

            for (int c = 0; c < 3; c++) diff[c] += lights->color[li][c] * ndotl;

            float ndoth = fm_vec3_dot(N, &light_H[li]);
            if (ndoth < 0.0f) ndoth = 0.0f;
            ndoth_lut[li] = ndoth;
        }

        set_rgb_quantized(&out->diffuse[idx], diff);

        float fres = fresnel_schlick_scalar(NdotV, 0.04f);
        fresnel_lut[idx] = fres;

        fm_vec3_t R = {{2.0f * NdotV * nx, 2.0f * NdotV * ny, 2.0f * NdotV * NdotV - 1.0f}};
        rotate_y(&R, cos_yaw, sin_yaw);

        for (int a = 0; a < 4; a++) {
            float env[3];
            const float *src = hdri->prefilter[a] ? hdri->prefilter[a] : hdri->rgb;
            sample_equirect_nearest(src, hdri->w, hdri->h, &R, env);

            float rough = anchor_rough[a];
            float a2 = rough * rough;
            float gv = ggx_G1(NdotV, rough);
            float ggx_sum[3] = {0.0f, 0.0f, 0.0f};

            for (int li = 0; li < light_count; li++) {
                float NdotL = ndotl_lut[li];
                if (NdotL <= 0.0f) continue;

                float NdotH = ndoth_lut[li];
                float VdotH = light_vdoth[li];

                float dd = (NdotH * NdotH) * (a2 - 1.0f) + 1.0f;
                float D = a2 / (PI_F * dd * dd + 1e-6f);
                float G = gv * ggx_G1(NdotL, rough);
                float F = fresnel_schlick_scalar(VdotH, 0.04f);
                float spec = (D * G * F) / (4.0f * NdotV * NdotL + 1e-6f);

                for (int c = 0; c < 3; c++) ggx_sum[c] += lights->color[li][c] * spec * NdotL;
            }

            for (int c = 0; c < 3; c++) anchor_spec[a][idx][c] = env[c] + ggx_sum[c];
        }
    }

    for (int r = 0; r < 8; r++) {
        float rf = ((float)r / 7.0f) * 3.0f;
        int i0 = (int)fm_floorf(rf);
        int i1 = i0 + 1;
        if (i1 > 3) i1 = 3;
        float t = rf - (float)i0;

        for (int i = 0; i < MATCAP_SIZE * MATCAP_SIZE; i++) {
            float s0[3] = {
                anchor_spec[i0][i][0],
                anchor_spec[i0][i][1],
                anchor_spec[i0][i][2]
            };
            float s1[3] = {
                anchor_spec[i1][i][0],
                anchor_spec[i1][i][1],
                anchor_spec[i1][i][2]
            };
            float s[3] = {
                s0[0] + (s1[0] - s0[0]) * t,
                s0[1] + (s1[1] - s0[1]) * t,
                s0[2] + (s1[2] - s0[2]) * t
            };

            float fres = fresnel_lut[i];
            float inv_f = 1.0f - fres;
            int chain_idx = (r << 10) | i;

            for (int c = 0; c < 3; c++) {
                out->spec[chain_idx].c[c] = quantize_hdr(s[c]);
                out->spec_mod[chain_idx].c[c] = quantize_hdr(s[c] * inv_f);
                out->spec_fres[chain_idx].c[c] = quantize_hdr(s[c] * fres);
            }
        }
    }
}
