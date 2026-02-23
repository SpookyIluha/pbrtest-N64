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
        debugf("PFM open failed: %s\n", path);
        return false;
    }

    char tok[64] = {0};
    if (!read_token(f, tok, sizeof(tok)) || strcmp(tok, "PF") != 0) {
        debugf("PFM header is not PF: %s\n", path);
        fclose(f);
        return false;
    }
    if (!read_token(f, tok, sizeof(tok))) { fclose(f); return false; }
    int w = atoi(tok);
    if (!read_token(f, tok, sizeof(tok))) { fclose(f); return false; }
    int h = atoi(tok);
    if (!read_token(f, tok, sizeof(tok))) { fclose(f); return false; }

    if (w <= 0 || h <= 0) {
        debugf("PFM invalid size %d x %d: %s\n", w, h, path);
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
        debugf("PFM short read (%lu/%lu): %s\n", (unsigned long)got, (unsigned long)count, path);
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
        debugf("HDRI path too long: base=%s suffix=%s\n", base, suffix);
        return false;
    }
    return load_pfm_file(path, w, h, rgb);
}

static inline uint16_t quantize_hdr_u88(float v)
{
    float scaled = v * FIXED_ONE;
    if (scaled < 0.0f) return 0;
    if (scaled > 65535.0f) return 65535;
    return (uint16_t)scaled;
}

static hdr16_rgb_t *convert_rgbf_to_u88(const float *rgb, int w, int h)
{
    size_t texels = (size_t)w * (size_t)h;
    hdr16_rgb_t *out = malloc(sizeof(hdr16_rgb_t) * texels);
    if (!out) return NULL;

    for (size_t i = 0; i < texels; i++) {
        out[i].c[0] = quantize_hdr_u88(rgb[i * 3u + 0u]);
        out[i].c[1] = quantize_hdr_u88(rgb[i * 3u + 1u]);
        out[i].c[2] = quantize_hdr_u88(rgb[i * 3u + 2u]);
    }
    return out;
}

void free_hdri(HDRISet *hdri)
{
    if (!hdri) return;

    free(hdri->diffuse);
    free(hdri->rough25);
    free(hdri->rough75);
    hdri->diffuse = NULL;
    hdri->rough25 = NULL;
    hdri->rough75 = NULL;
    hdri->w = 0;
    hdri->h = 0;
}

bool load_pfm_hdri(const char *path, HDRISet *out)
{
    char base[HDRI_BASE_PATH_MAX];

    int dw = 0, dh = 0, lw = 0, lh = 0, hw = 0, hh = 0;
    float *diff_f = NULL;
    float *low_f = NULL;
    float *high_f = NULL;

    hdr16_rgb_t *diff_u88 = NULL;
    hdr16_rgb_t *low_u88 = NULL;
    hdr16_rgb_t *high_u88 = NULL;

    normalize_base_path(path, base, sizeof(base));

    if (!load_pfm_suffixed(base, "_d", &dw, &dh, &diff_f)) goto fail;
    if (!load_pfm_suffixed(base, "_s25", &lw, &lh, &low_f)) goto fail;
    if (!load_pfm_suffixed(base, "_s75", &hw, &hh, &high_f)) goto fail;

    if (dw != lw || dh != lh || dw != hw || dh != hh) {
        debugf("HDRI size mismatch: _d=%dx%d _s25=%dx%d _s75=%dx%d\n", dw, dh, lw, lh, hw, hh);
        goto fail;
    }

    diff_u88 = convert_rgbf_to_u88(diff_f, dw, dh);
    low_u88 = convert_rgbf_to_u88(low_f, lw, lh);
    high_u88 = convert_rgbf_to_u88(high_f, hw, hh);
    if (!diff_u88 || !low_u88 || !high_u88) goto fail;

    free(diff_f);
    free(low_f);
    free(high_f);
    diff_f = NULL;
    low_f = NULL;
    high_f = NULL;

    free_hdri(out);
    memset(out, 0, sizeof(*out));
    out->w = dw;
    out->h = dh;
    out->diffuse = diff_u88;
    out->rough25 = low_u88;
    out->rough75 = high_u88;

    snprintf(g_hdri_base_path, sizeof(g_hdri_base_path), "%s", base);
    debugf("Loaded HDRI triplet (u8.8): %s_d/_s25/_s75.pbm (%dx%d)\n", base, dw, dh);
    return true;

fail:
    free(diff_f);
    free(low_f);
    free(high_f);
    free(diff_u88);
    free(low_u88);
    free(high_u88);
    return false;
}