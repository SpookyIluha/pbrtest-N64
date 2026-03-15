#include <libdragon.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pfm_hdri.h"

#define HDRI_BASE_PATH_MAX 512
#define HDRI_PATH_MAX 640
#define FIXED_ONE 256.0f

static char g_hdri_base_path[HDRI_BASE_PATH_MAX];

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

static void normalize_base_path(const char *in, char *out, size_t out_len)
{
    size_t n = strlen(in);
    if (n + 1 > out_len) n = out_len - 1;
    memcpy(out, in, n);
    out[n] = '\0';

    if (n >= 4) {
        const char *ext = &out[n - 4];
        if (strcmp(ext, ".pbm") == 0 || strcmp(ext, ".PBM") == 0) {
            out[n - 4] = '\0';
        }
    }
}

static bool build_suffixed_path(char *out, size_t out_len, const char *base, const char *suffix)
{
    int rc = snprintf(out, out_len, "%s%s.pbm", base, suffix);
    return rc > 0 && (size_t)rc < out_len;
}

static bool load_pfm_file(const char *path, int *w_out, int *h_out, float **rgb_out)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        assertf(0,"PFM open failed: %s\n", path);
        return false;
    }

    char tok[64] = {0};
    if (!read_token(f, tok, sizeof(tok)) || strcmp(tok, "PF") != 0) {
        assertf(0,"PFM header is not PF: %s\n", path);
        fclose(f);
        return false;
    }
    if (!read_token(f, tok, sizeof(tok))) { fclose(f); return false; }
    int w = atoi(tok);
    if (!read_token(f, tok, sizeof(tok))) { fclose(f); return false; }
    int h = atoi(tok);
    if (!read_token(f, tok, sizeof(tok))) { fclose(f); return false; }

    if (w <= 0 || h <= 0) {
        assertf(0,"PFM invalid size %d x %d: %s\n", w, h, path);
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
        assertf(0,"PFM short read (%lu/%lu): %s\n", (unsigned long)got, (unsigned long)count, path);
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

    *w_out = w;
    *h_out = h;
    *rgb_out = rgb;
    return true;
}

static bool load_pfm_suffixed(const char *base, const char *suffix, int *w, int *h, float **rgb)
{
    char path[HDRI_PATH_MAX];
    if (!build_suffixed_path(path, sizeof(path), base, suffix)) {
        assertf(0,"HDRI path too long: base=%s suffix=%s\n", base, suffix);
        return false;
    }
    return load_pfm_file(path, w, h, rgb);
}

static inline uint16_t quantize_hdr_u88(float v)
{
    // Linear to gamma conversion thspec sqrt(), because VI's own GAMMA_CORRECT is very prone to banding artifacts 
    // on 16bpp at low-light levels, so we do math in gamma-space instead and use GAMMA_NONE
    // that also means that the additional GGX light colors need to be set in gamma space as well (!)
    //
    // Technically this is not correct because the color blending on RSP will be wrong
    //
    // However the HDR->SDR tonemapper fixes it just a bit, since it has its own exponential curve, 
    // so it looks kind of similar to the fully linear result
    //
    // A proper way would be to just use 32bpp VI output with GAMMA_CORRECT, but that's slower to render to
    float scaled = sqrt(v) * FIXED_ONE;
    if (scaled < 0.0f) return 0;
    if (scaled > 65535.0f) return 65535;
    return (uint16_t)scaled;
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

static inline void dual_paraboloid_atlas_inverse_uv(float u, float v, bool front, float *x, float *y, float *z)
{
    float local_u = front ? (u * 2.0f) : ((u - 0.5f) * 2.0f);
    float a = 2.0f * local_u - 1.0f;
    float b = 2.0f * v - 1.0f;
    float r2 = a * a + b * b;
    float inv = 1.0f / (1.0f + r2);

    if (front) {
        *x = 2.0f * a * inv;
        *y = 2.0f * b * inv;
        *z = (1.0f - r2) * inv;
    } else {
        *x = -2.0f * a * inv;
        *y = -2.0f * b * inv;
        *z = (r2 - 1.0f) * inv;
    }
}

static inline float fast_acosf(float x)
{
    float res = -0.6981317f * x;
    res *= x;
    res -= 0.8726646f;
    res *= x;
    res += 1.5707963f;
    return res;
}

static inline void dir_to_equirect_uv(float x, float y, float z, float *u, float *v)
{
    const float pi = FM_PI;
    *u = fm_atan2f(z, x) * (0.5f / pi) + 0.5f;
    *v = fast_acosf(clampf_local(y, -1.0f, 1.0f)) * (1.0f / pi);
}

static inline float lerpf_local(float a, float b, float t)
{
    return a + (b - a) * t;
}

static inline void sample_equirect_rgbf_bilinear(const float *rgb, int w, int h, float u, float v, float out_rgb[3])
{
    float px = u * (float)w - 0.5f;
    float py = v * (float)h - 0.5f;

    int x0 = wrap_x((int)floorf(px), w);
    int y0 = clamp_y((int)floorf(py), h);
    int x1 = wrap_x(x0 + 1, w);
    int y1 = clamp_y(y0 + 1, h);

    float tx = px - floorf(px);
    float ty = py - floorf(py);

    size_t i00 = ((size_t)y0 * (size_t)w + (size_t)x0) * 3u;
    size_t i10 = ((size_t)y0 * (size_t)w + (size_t)x1) * 3u;
    size_t i01 = ((size_t)y1 * (size_t)w + (size_t)x0) * 3u;
    size_t i11 = ((size_t)y1 * (size_t)w + (size_t)x1) * 3u;

    out_rgb[0] = lerpf_local(lerpf_local(rgb[i00 + 0u], rgb[i10 + 0u], tx),
                             lerpf_local(rgb[i01 + 0u], rgb[i11 + 0u], tx), ty);
    out_rgb[1] = lerpf_local(lerpf_local(rgb[i00 + 1u], rgb[i10 + 1u], tx),
                             lerpf_local(rgb[i01 + 1u], rgb[i11 + 1u], tx), ty);
    out_rgb[2] = lerpf_local(lerpf_local(rgb[i00 + 2u], rgb[i10 + 2u], tx),
                             lerpf_local(rgb[i01 + 2u], rgb[i11 + 2u], tx), ty);
}

static hdri_color_t *reproject_rgbf_equirect_to_dual_paraboloid_u88(const float *src_rgb,
                                                                     int src_w,
                                                                     int src_h,
                                                                     int *dst_w_out,
                                                                     int *dst_h_out)
{
    int dst_w = src_w * 2;
    int dst_h = src_h;
    size_t texels = (size_t)dst_w * (size_t)dst_h;
    hdri_color_t *out = malloc(sizeof(hdri_color_t) * texels);
    if (!out) return NULL;

    for (int y = 0; y < dst_h; y++) {
        float v_hemi = ((float)y + 0.5f) / (float)dst_h;

        for (int x = 0; x < dst_w; x++) {
            bool front = x < src_w;
            float u_hemi = ((float)x + 0.5f) / (float)dst_w;
            float dx, dy, dz;
            float src_sample[3];
            float src_u, src_v;
            size_t dst_i = (size_t)y * (size_t)dst_w + (size_t)x;

            // HDRI reprojection from equirectangular to dual-parabaloid, since it's cheaper to sample from
            dual_paraboloid_atlas_inverse_uv(u_hemi, v_hemi, front, &dx, &dy, &dz);
            dir_to_equirect_uv(dx, dy, dz, &src_u, &src_v);
            sample_equirect_rgbf_bilinear(src_rgb, src_w, src_h, src_u, src_v, src_sample);

            out[dst_i].c[0] = quantize_hdr_u88(src_sample[0]);
            out[dst_i].c[1] = quantize_hdr_u88(src_sample[1]);
            out[dst_i].c[2] = quantize_hdr_u88(src_sample[2]);
        }
    }

    *dst_w_out = dst_w;
    *dst_h_out = dst_h;
    return out;
}

static hdri_color_t *reproject_specular_pair_to_interleaved_dual_paraboloid_u88(const float *low_rgb,
                                                                                  const float *high_rgb,
                                                                                  int src_w,
                                                                                  int src_h,
                                                                                  int *dst_w_out,
                                                                                  int *dst_h_out)
{
    int dst_w = src_w * 2;
    int dst_h = src_h;
    size_t texels = (size_t)dst_w * (size_t)dst_h;
    hdri_color_t *out = malloc(sizeof(hdri_color_t) * texels * 2u);
    if (!out) return NULL;

    for (int y = 0; y < dst_h; y++) {
        float v_hemi = ((float)y + 0.5f) / (float)dst_h;

        for (int x = 0; x < dst_w; x++) {
            bool front = x < src_w;
            float u_hemi = ((float)x + 0.5f) / (float)dst_w;
            float dx, dy, dz;
            float s25[3];
            float s75[3];
            float src_u, src_v;
            size_t dst_i = ((size_t)y * (size_t)dst_w + (size_t)x) * 2u;
            
            // HDRI reprojection from equirectangular to dual-parabaloid, since it's cheaper to sample from
            dual_paraboloid_atlas_inverse_uv(u_hemi, v_hemi, front, &dx, &dy, &dz);
            dir_to_equirect_uv(dx, dy, dz, &src_u, &src_v);
            sample_equirect_rgbf_bilinear(low_rgb, src_w, src_h, src_u, src_v, s25);
            sample_equirect_rgbf_bilinear(high_rgb, src_w, src_h, src_u, src_v, s75);

            out[dst_i + 0u].c[0] = quantize_hdr_u88(s25[0]);
            out[dst_i + 0u].c[1] = quantize_hdr_u88(s25[1]);
            out[dst_i + 0u].c[2] = quantize_hdr_u88(s25[2]);

            out[dst_i + 1u].c[0] = quantize_hdr_u88(s75[0]);
            out[dst_i + 1u].c[1] = quantize_hdr_u88(s75[1]);
            out[dst_i + 1u].c[2] = quantize_hdr_u88(s75[2]);
        }
    }

    *dst_w_out = dst_w;
    *dst_h_out = dst_h;
    return out;
}

void free_hdri(HDRISet *hdri)
{
    if (!hdri) return;

    free(hdri->diffuse);
    free(hdri->spec25);
    free(hdri->spec75);
    free(hdri->specular_interleaved);
    hdri->diffuse = NULL;
    hdri->spec25 = NULL;
    hdri->spec75 = NULL;
    hdri->specular_interleaved = NULL;
    hdri->diffuse_w = 0;
    hdri->diffuse_h = 0;
    hdri->spec25_w = 0;
    hdri->spec25_h = 0;
    hdri->spec75_w = 0;
    hdri->spec75_h = 0;
    hdri->specular_w = 0;
    hdri->specular_h = 0;
}

bool load_pfm_hdri(const char *path, HDRISet *out)
{
    char base[HDRI_BASE_PATH_MAX];

    int dw = 0, dh = 0, lw = 0, lh = 0, hw = 0, hh = 0;
    float *diff_f = NULL;
    float *low_f = NULL;
    float *high_f = NULL;

    hdri_color_t *diff_u88 = NULL;
    hdri_color_t *spec_i_u88 = NULL;
    int diff_dp_w = 0, diff_dp_h = 0;
    int spec_dp_w = 0, spec_dp_h = 0;

    normalize_base_path(path, base, sizeof(base));

    if (!load_pfm_suffixed(base, "_d", &dw, &dh, &diff_f)) goto fail;
    if (!load_pfm_suffixed(base, "_s25", &lw, &lh, &low_f)) goto fail;
    if (!load_pfm_suffixed(base, "_s75", &hw, &hh, &high_f)) goto fail;
    if (lw != hw || lh != hh) {
        assertf(0,"HDRI specular size mismatch: _s25=%dx%d _s75=%dx%d\n", lw, lh, hw, hh);
        goto fail;
    }

    // preprocess the HDRI into a more feasible format to be used in matcaps and HDR lighting
    // equirectangular mapping is replaced with a dual parabaloid, presampled bilinearly
    // float RGB is replaced by u8.8 fixed point, halfs the memory cost
    diff_u88 = reproject_rgbf_equirect_to_dual_paraboloid_u88(diff_f, dw, dh, &diff_dp_w, &diff_dp_h);
    spec_i_u88 = reproject_specular_pair_to_interleaved_dual_paraboloid_u88(low_f, high_f, lw, lh, &spec_dp_w, &spec_dp_h);
    if (!diff_u88 || !spec_i_u88) goto fail;

    free(diff_f);
    free(low_f);
    free(high_f);
    diff_f = NULL;
    low_f = NULL;
    high_f = NULL;

    free_hdri(out);
    memset(out, 0, sizeof(*out));
    out->diffuse_w = diff_dp_w;
    out->diffuse_h = diff_dp_h;
    out->spec25_w = spec_dp_w;
    out->spec25_h = spec_dp_h;
    out->spec75_w = spec_dp_w;
    out->spec75_h = spec_dp_h;
    out->diffuse = diff_u88;
    out->spec25 = NULL;
    out->spec75 = NULL;
    out->specular_interleaved = spec_i_u88;
    out->specular_w = spec_dp_w;
    out->specular_h = spec_dp_h;

    snprintf(g_hdri_base_path, sizeof(g_hdri_base_path), "%s", base);
    debugf("Loaded HDRI triplet + DP reprojection (u8.8): %s_d=%dx%d -> %dx%d _s25=%dx%d _s75=%dx%d -> %dx%d\n",
           base, dw, dh, diff_dp_w, diff_dp_h, lw, lh, hw, hh, spec_dp_w, spec_dp_h);
    return true;

fail:
    free(diff_f);
    free(low_f);
    free(high_f);
    free(diff_u88);
    free(spec_i_u88);
    return false;
}
