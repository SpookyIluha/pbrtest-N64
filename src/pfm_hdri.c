#include <libdragon.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pfm_hdri.h"

static inline uint32_t bswap32_local(uint32_t x)
{
    return ((x & 0x000000FFu) << 24) |
           ((x & 0x0000FF00u) << 8) |
           ((x & 0x00FF0000u) >> 8) |
           ((x & 0xFF000000u) >> 24);
}

static bool read_token(FILE *f, char *out, size_t out_len)
{
    int c;
    size_t i = 0;

    do {
        c = fgetc(f);
        if (c == '#') {
            while (c != '\n' && c != EOF) c = fgetc(f);
        }
    } while ((c == ' ' || c == '\n' || c == '\r' || c == '\t') && c != EOF);

    if (c == EOF) return false;

    while (c != EOF && c != ' ' && c != '\n' && c != '\r' && c != '\t') {
        if (i + 1 < out_len) out[i++] = (char)c;
        c = fgetc(f);
    }
    out[i] = '\0';
    return i > 0;
}

void free_hdri(HDRISet *hdri)
{
    if (!hdri) return;
    free(hdri->rgb);
    hdri->rgb = NULL;
    for (int i = 0; i < 4; i++) {
        free(hdri->prefilter[i]);
        hdri->prefilter[i] = NULL;
    }
    free(hdri->diffuse_irr);
    hdri->diffuse_irr = NULL;
    hdri->w = 0;
    hdri->h = 0;
}

bool load_pfm_hdri(const char *path, HDRISet *out)
{
    memset(out, 0, sizeof(*out));

    FILE *f = fopen(path, "rb");
    if (!f) {
        debugf("PFM open failed: %s\n", path);
        return false;
    }

    char tok[64] = {0};
    if (!read_token(f, tok, sizeof(tok)) || strcmp(tok, "PF") != 0) {
        debugf("PFM header is not PF\n");
        fclose(f);
        return false;
    }
    if (!read_token(f, tok, sizeof(tok))) {
        fclose(f);
        return false;
    }
    int w = atoi(tok);
    if (!read_token(f, tok, sizeof(tok))) {
        fclose(f);
        return false;
    }
    int h = atoi(tok);
    if (!read_token(f, tok, sizeof(tok))) {
        fclose(f);
        return false;
    }

    if (w <= 0 || h <= 0) {
        debugf("PFM invalid size %d x %d\n", w, h);
        fclose(f);
        return false;
    }

    float scale = (float)atof(tok);
    bool file_little_endian = (scale < 0.0f);
    float abs_scale = fabsf(scale);
    if (abs_scale <= 0.0f) abs_scale = 1.0f;

    size_t count = (size_t)w * (size_t)h * 3u;
    float *rgb = malloc(sizeof(float) * count);
    if (!rgb) {
        fclose(f);
        return false;
    }

    size_t got = fread(rgb, sizeof(float), count, f);
    fclose(f);

    if (got != count) {
        debugf("PFM short read (%lu / %lu)\n", (unsigned long)got, (unsigned long)count);
        free(rgb);
        return false;
    }

    uint16_t endian_test = 0x0102;
    bool host_little_endian = (*((uint8_t *)&endian_test) == 0x02);
    if (file_little_endian != host_little_endian) {
        for (size_t i = 0; i < count; i++) {
            uint32_t v;
            memcpy(&v, &rgb[i], sizeof(v));
            v = bswap32_local(v);
            memcpy(&rgb[i], &v, sizeof(v));
        }
    }

    for (size_t i = 0; i < count; i++) {
        rgb[i] *= abs_scale;
    }

    out->w = w;
    out->h = h;
    out->rgb = rgb;
    return true;
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

static inline float clampf_local(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static inline float dot3(const float a[3], const float b[3])
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static inline void cross3(const float a[3], const float b[3], float out[3])
{
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

static inline void normalize3(float v[3])
{
    float l2 = dot3(v, v);
    if (l2 <= 1e-20f) {
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 1.0f;
        return;
    }
    float inv = 1.0f / sqrtf(l2);
    v[0] *= inv;
    v[1] *= inv;
    v[2] *= inv;
}

static inline void build_basis(const float N[3], float T[3], float B[3])
{
    float up[3] = {0.0f, 1.0f, 0.0f};
    if (fabsf(N[1]) > 0.999f) {
        up[0] = 1.0f;
        up[1] = 0.0f;
        up[2] = 0.0f;
    }
    cross3(up, N, T);
    normalize3(T);
    cross3(N, T, B);
}

static inline void texel_to_dir(int x, int y, int w, int h, float dir[3])
{
    const float pi = 3.14159265358979323846f;
    float u = ((float)x + 0.5f) / (float)w;
    float v = ((float)y + 0.5f) / (float)h;
    float phi = (u - 0.5f) * (2.0f * pi);
    float theta = v * pi;
    float st = sinf(theta);

    dir[0] = cosf(phi) * st;
    dir[1] = cosf(theta);
    dir[2] = sinf(phi) * st;
}

static inline void sample_equirect_bilinear(const float *img, int w, int h, const float dir[3], float out_rgb[3])
{
    const float inv2pi = 0.15915494309189535f; /* 1/(2*pi) */
    const float invpi = 0.3183098861837907f;   /* 1/pi */

    float u = atan2f(dir[2], dir[0]) * inv2pi + 0.5f;
    float v = acosf(clampf_local(dir[1], -1.0f, 1.0f)) * invpi;

    float fx = u * (float)w - 0.5f;
    float fy = v * (float)h - 0.5f;
    int x0 = (int)floorf(fx);
    int y0 = (int)floorf(fy);
    int x1 = x0 + 1;
    int y1 = y0 + 1;
    float tx = fx - (float)x0;
    float ty = fy - (float)y0;

    x0 = wrap_x(x0, w);
    x1 = wrap_x(x1, w);
    y0 = clamp_y(y0, h);
    y1 = clamp_y(y1, h);

    const float *c00 = &img[((size_t)y0 * (size_t)w + (size_t)x0) * 3u];
    const float *c10 = &img[((size_t)y0 * (size_t)w + (size_t)x1) * 3u];
    const float *c01 = &img[((size_t)y1 * (size_t)w + (size_t)x0) * 3u];
    const float *c11 = &img[((size_t)y1 * (size_t)w + (size_t)x1) * 3u];

    for (int c = 0; c < 3; c++) {
        float a = c00[c] + (c10[c] - c00[c]) * tx;
        float b = c01[c] + (c11[c] - c01[c]) * tx;
        out_rgb[c] = a + (b - a) * ty;
    }
}

static inline float radical_inverse_vdc(uint32_t bits)
{
    bits = (bits << 16) | (bits >> 16);
    bits = ((bits & 0x55555555u) << 1) | ((bits & 0xAAAAAAAAu) >> 1);
    bits = ((bits & 0x33333333u) << 2) | ((bits & 0xCCCCCCCCu) >> 2);
    bits = ((bits & 0x0F0F0F0Fu) << 4) | ((bits & 0xF0F0F0F0u) >> 4);
    bits = ((bits & 0x00FF00FFu) << 8) | ((bits & 0xFF00FF00u) >> 8);
    return (float)bits * 2.3283064365386963e-10f;
}

static inline void hammersley_2d(uint32_t i, uint32_t n, float xi[2])
{
    xi[0] = (float)i / (float)n;
    xi[1] = radical_inverse_vdc(i);
}

static inline void sample_cosine_hemisphere(const float xi[2], float out[3])
{
    const float pi2 = 6.28318530717958647692f;
    float phi = pi2 * xi[0];
    float cos_theta = sqrtf(1.0f - xi[1]);
    float sin_theta = sqrtf(xi[1]);

    out[0] = cosf(phi) * sin_theta;
    out[1] = sinf(phi) * sin_theta;
    out[2] = cos_theta;
}

static inline void sample_ggx_half_vector(const float xi[2], float roughness, float out[3])
{
    const float pi2 = 6.28318530717958647692f;
    float a = roughness * roughness;
    if (a < 1e-3f) a = 1e-3f;
    float a2 = a * a;
    float phi = pi2 * xi[0];
    float cos_theta = sqrtf((1.0f - xi[1]) / (1.0f + (a2 - 1.0f) * xi[1]));
    float sin_theta = sqrtf(fmaxf(0.0f, 1.0f - cos_theta * cos_theta));

    out[0] = cosf(phi) * sin_theta;
    out[1] = sinf(phi) * sin_theta;
    out[2] = cos_theta;
}

static inline void tangent_to_world(const float local[3], const float T[3], const float B[3], const float N[3], float out[3])
{
    out[0] = T[0] * local[0] + B[0] * local[1] + N[0] * local[2];
    out[1] = T[1] * local[0] + B[1] * local[1] + N[1] * local[2];
    out[2] = T[2] * local[0] + B[2] * local[1] + N[2] * local[2];
    normalize3(out);
}

static inline void debug_progress_row(const char *stage_name, int y, int h, int *last_pct)
{
    int pct = ((y + 1) * 100) / h;
    if (pct != *last_pct && (pct == 1 || pct % 5 == 0 || pct == 100)) {
        debugf("%s: %d%%\n", stage_name, pct);
        *last_pct = pct;
    }
}

static void convolve_diffuse_irradiance(const float *src, float *dst, int w, int h, uint32_t samples, const char *stage_name)
{
    const float pi = 3.14159265358979323846f;
    int last_pct = -1;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float N[3], T[3], B[3];
            texel_to_dir(x, y, w, h, N);
            build_basis(N, T, B);

            float accum[3] = {0.0f, 0.0f, 0.0f};
            for (uint32_t s = 0; s < samples; s++) {
                float xi[2], L_local[3], L_world[3], sample_rgb[3];
                hammersley_2d(s, samples, xi);
                sample_cosine_hemisphere(xi, L_local);
                tangent_to_world(L_local, T, B, N, L_world);
                sample_equirect_bilinear(src, w, h, L_world, sample_rgb);
                accum[0] += sample_rgb[0];
                accum[1] += sample_rgb[1];
                accum[2] += sample_rgb[2];
            }

            float scale = pi / (float)samples;
            float *out = &dst[((size_t)y * (size_t)w + (size_t)x) * 3u];
            out[0] = accum[0] * scale;
            out[1] = accum[1] * scale;
            out[2] = accum[2] * scale;
        }
        debug_progress_row(stage_name, y, h, &last_pct);
    }
}

static void prefilter_specular_ggx(const float *src, float *dst, int w, int h, float roughness, uint32_t samples, const char *stage_name)
{
    int last_pct = -1;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float N[3], T[3], B[3];
            texel_to_dir(x, y, w, h, N);
            build_basis(N, T, B);

            float V[3] = {N[0], N[1], N[2]};
            float accum[3] = {0.0f, 0.0f, 0.0f};
            float weight = 0.0f;

            for (uint32_t s = 0; s < samples; s++) {
                float xi[2], H_local[3], H_world[3], sample_rgb[3];
                hammersley_2d(s, samples, xi);
                sample_ggx_half_vector(xi, roughness, H_local);
                tangent_to_world(H_local, T, B, N, H_world);

                float VdotH = dot3(V, H_world);
                if (VdotH <= 0.0f) continue;

                float L[3] = {
                    2.0f * VdotH * H_world[0] - V[0],
                    2.0f * VdotH * H_world[1] - V[1],
                    2.0f * VdotH * H_world[2] - V[2]
                };
                normalize3(L);

                float NdotL = dot3(N, L);
                if (NdotL <= 0.0f) continue;

                sample_equirect_bilinear(src, w, h, L, sample_rgb);
                accum[0] += sample_rgb[0] * NdotL;
                accum[1] += sample_rgb[1] * NdotL;
                accum[2] += sample_rgb[2] * NdotL;
                weight += NdotL;
            }

            float *out = &dst[((size_t)y * (size_t)w + (size_t)x) * 3u];
            if (weight > 1e-6f) {
                float inv = 1.0f / weight;
                out[0] = accum[0] * inv;
                out[1] = accum[1] * inv;
                out[2] = accum[2] * inv;
            } else {
                out[0] = 0.0f;
                out[1] = 0.0f;
                out[2] = 0.0f;
            }
        }
        debug_progress_row(stage_name, y, h, &last_pct);
    }
}

static void free_prefilter_buffers(HDRISet *io)
{
    for (int i = 0; i < 4; i++) {
        free(io->prefilter[i]);
        io->prefilter[i] = NULL;
    }
    free(io->diffuse_irr);
    io->diffuse_irr = NULL;
}

void prefilter_hdri_roughness_4(const HDRISet *in, HDRISet *io)
{
    (void)in;

    const float roughness_levels[4] = {0.12f, 0.22f, 0.52f, 0.75f};
    const uint32_t diffuse_samples = 64;
    const uint32_t spec_samples = 128;
    size_t size = sizeof(float) * (size_t)io->w * (size_t)io->h * 3u;

    free_prefilter_buffers(io);

    for (int i = 0; i < 4; i++) {
        char stage_name[64];
        snprintf(stage_name, sizeof(stage_name), "HDRI spec prefilter L%d r=%.2f", i, roughness_levels[i]);
        debugf("%s: 0%%\n", stage_name);

        io->prefilter[i] = malloc(size);
        if (!io->prefilter[i]) {
            debugf("prefilter alloc failed level %d\n", i);
            free_prefilter_buffers(io);
            return;
        }
        prefilter_specular_ggx(io->rgb, io->prefilter[i], io->w, io->h, roughness_levels[i], spec_samples, stage_name);
    }

    debugf("HDRI diffuse irradiance: 0%%\n");
    io->diffuse_irr = malloc(size);
    if (!io->diffuse_irr) {
        debugf("diffuse irr alloc failed\n");
        free_prefilter_buffers(io);
        return;
    }
    convolve_diffuse_irradiance(io->rgb, io->diffuse_irr, io->w, io->h, diffuse_samples, "HDRI diffuse irradiance");
}
