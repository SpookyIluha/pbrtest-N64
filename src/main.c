#include <libdragon.h>
#include <t3d/t3d.h>

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "pbr_blend.h"
#include "pbr_decode.h"
#include "pbr_matcap.h"
#include "pfm_hdri.h"

#define FB_COUNT 3
#define PIPELINE_BUFFERS 2
#define BUFFER_W 320
#define BUFFER_H 240

#define HDRI_PATH "rom:/textures/courtyard"
#define GBUFFER_ALBEDO_PATH "rom:/models/spheres_albedo.rgba16.sprite"
#define GBUFFER_PACKED_PATH "rom:/models/spheres_packed.rgba16.sprite"

#define MEASURE_RSP_PERF 1

const uint16_t *sprite_pixels_u16(const sprite_t *spr)
{
    return (const uint16_t *)spr->data;
}

bool sprite_dim_match(const sprite_t *spr, int w, int h)
{
    return spr && spr->width == w && spr->height == h;
}

static bool load_gbuffers_from_sprites(const char *albedo_path, const char *packed_path, sprite_t **albedo, sprite_t **packed)
{
    sprite_t* _albedo = sprite_load(albedo_path);
    sprite_t* _packed = sprite_load(packed_path);

    if (!_albedo || !_packed) {
        debugf("sprite load failed: %s / %s\n", albedo_path, packed_path);
        return false;
    }
    *albedo = _albedo;
    *packed = _packed;

    return true;
}

static void setup_default_lighting(LightingState *ls)
{
    memset(ls, 0, sizeof(*ls));
    ls->count = 0;

    ls->dir[0][0] = 0.707f;
    ls->dir[0][1] = 0.577f;
    ls->dir[0][2] = 0.408f;
    ls->color[0][0] = 1.0f;
    ls->color[0][1] = 1.0f;
    ls->color[0][2] = 1.0f;

    ls->dir[1][0] = -0.4f;
    ls->dir[1][1] = 0.9f;
    ls->dir[1][2] = 0.2f;
    ls->color[1][0] = 1.5f;
    ls->color[1][1] = 1.0f;
    ls->color[1][2] = 1.0f;
}

static inline float clampf_local(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// Packed RGBA16 layout is RRRRRGGGGGBBBBBA:
// R(5)=roughness, G(5)=X, B(5)=Y, A(1)=metalness.
// Clamp XY so ||(x,y)|| <= 1 before decoding.
static void preprocess_packed_rgba16_normals(const uint16_t *src, uint16_t *dst, int w, int h)
{
    const size_t n = (size_t)w * (size_t)h;
    for (size_t i = 0; i < n; i++) {
        const uint16_t p = src[i];

        const uint16_t rough5 = (p >> 11) & 0x1Fu;
        const uint16_t x5 = (p >> 6) & 0x1Fu;
        const uint16_t y5 = (p >> 1) & 0x1Fu;
        const uint16_t m1 = p & 0x01u;

        float x = ((float)x5 * (2.0f / 31.0f)) - 1.0f;
        float y = ((float)y5 * (2.0f / 31.0f)) - 1.0f;
        float len2 = x * x + y * y;
        if (len2 > 1.0f) {
            float inv_len = 1.0f / sqrtf(len2);
            x *= inv_len;
            y *= inv_len;
        }

        uint16_t x5_out = (uint16_t)(clampf_local((x + 1.0f) * 0.5f, 0.0f, 1.0f) * 31.0f + 0.5f);
        uint16_t y5_out = (uint16_t)(clampf_local((y + 1.0f) * 0.5f, 0.0f, 1.0f) * 31.0f + 0.5f);

        dst[i] = (uint16_t)((rough5 << 11) | (x5_out << 6) | (y5_out << 1) | m1);
    }
}

int main(void)
{
    debug_init_isviewer();
    debug_init_usblog();
    asset_init_compression(2);

    dfs_init(DFS_DEFAULT_LOCATION);

    display_init(RESOLUTION_320x240, DEPTH_16_BPP, FB_COUNT, GAMMA_CORRECT_DITHER, FILTERS_RESAMPLE_ANTIALIAS_DEDITHER);
    rdpq_init();
    t3d_init((T3DInitParams){});
    rsp_pbr_blend_init();

    sprite_t *albedo_spr = NULL;
    sprite_t *packed_spr = NULL;
    if (!load_gbuffers_from_sprites(GBUFFER_ALBEDO_PATH, GBUFFER_PACKED_PATH, &albedo_spr, &packed_spr)) {
        debugf("GBuffer load failed\n");
        for (;;) {}
    }

    int w = albedo_spr->width;
    int h = albedo_spr->height;

    if (!sprite_dim_match(packed_spr, w, h)) {
        debugf("GBuffer size mismatch\n");
        for (;;) {}
    }
    if (w != BUFFER_W || h != BUFFER_H) {
        debugf("Expected %dx%d buffers, got %dx%d\n", BUFFER_W, BUFFER_H, w, h);
        for (;;) {}
    }

    HDRISet hdri = {0};
    if (!load_pfm_hdri(HDRI_PATH, &hdri)) {
        debugf("HDRI load failed\n");
        for (;;) {}
    }

    MatcapSet mats_db[PIPELINE_BUFFERS] = {0};
    for (int i = 0; i < PIPELINE_BUFFERS; i++) {
        if (!alloc_matcaps(&mats_db[i])) {
            debugf("matcap alloc failed\n");
            for (;;) {}
        }
    }

    surface_t decoded_lighting_surf[PIPELINE_BUFFERS];
    uint32_t *decoded_lighting[PIPELINE_BUFFERS];

    for (int i = 0; i < PIPELINE_BUFFERS; i++) {
        decoded_lighting_surf[i] = surface_alloc(FMT_RGBA32, w, h * 3);

        decoded_lighting[i] = UncachedAddr(decoded_lighting_surf[i].buffer);

        if (!decoded_lighting[i]) {
            debugf("decode buffers alloc failed\n");
            for (;;) {}
        }
    }

    const uint16_t *albedo = sprite_pixels_u16(albedo_spr);
    const uint16_t *packed_src = sprite_pixels_u16(packed_spr);
    uint16_t *packed_preprocessed = malloc((size_t)w * (size_t)h * sizeof(uint16_t));
    const uint16_t *packed = packed_preprocessed;
    if (!albedo || !packed_src || !packed_preprocessed) {
        debugf("sprite pixel access failed\n");
        for (;;) {}
    }
    preprocess_packed_rgba16_normals(packed_src, packed_preprocessed, w, h);

    surface_t albedo_surf = surface_make_linear((void *)albedo, FMT_RGBA16, (uint16_t)w, (uint16_t)h);
    surface_t packed_surf = surface_make_linear((void *)packed, FMT_RGBA16, (uint16_t)w, (uint16_t)h);

    LightingState lights;
    setup_default_lighting(&lights);

    CameraState cam = {{0.0f, 0.0f, 1.0f}};
    float yaw = 0.0f;
    int frame = 0;

    surface_t *pending_disp = NULL;
    rspq_syncpoint_t pending_sync = 0;
    bool pending_show = false;

    for (;;) {
        uint64_t t_frame0 = get_ticks_us();
        int slot = frame & (PIPELINE_BUFFERS - 1);

        yaw += 0.06f;
        build_camera_from_yaw(yaw, &cam);

        uint64_t t_mat0 = get_ticks_us();
        generate_matcaps(&cam, &lights, &hdri, &mats_db[slot]);
        uint64_t t_mat1 = get_ticks_us();

        uint64_t t_dec0 = get_ticks_us();
        decode_packed_cpu_lighting_interleaved8(packed,
                          (uint8_t*)decoded_lighting[slot],
                          w,
                          h,
                          &mats_db[slot]);
        uint64_t t_dec1 = get_ticks_us();

        if (pending_show) {
            //rspq_syncpoint_wait(pending_sync);
            rdpq_attach(pending_disp, NULL);
            rdpq_detach_show();
            pending_show = false;
            pending_disp = NULL;
        }

        surface_t *disp = display_get();
        if (!disp) {
            continue;
        }
        if (disp->width != w || disp->height != h) {
            debugf("Display/sprite dimension mismatch: display=%dx%d sprite=%dx%d\n",
                   disp->width, disp->height, w, h);
            for (;;) {}
        }
        
        uint64_t t_rsp0 = get_ticks_us();
        if (MEASURE_RSP_PERF) {
            rspq_wait();
            rspq_highpri_begin();
        }

        rsp_pbr_blend_set_gbuffer(&albedo_surf, &packed_surf, disp);
        rsp_pbr_blend_set_lighting_buffer(&decoded_lighting_surf[slot]);
        rsp_pbr_blend_set_dither_matrix();
        rsp_pbr_blend_postprocess();
        //pending_sync = rspq_syncpoint_new();
        pending_disp = disp;
        pending_show = true;

        if (MEASURE_RSP_PERF) {
            rspq_highpri_end();
            rspq_flush();
            rspq_highpri_sync();
        }

        uint64_t t_rsp1 = get_ticks_us();

        uint64_t t_frame1 = get_ticks_us();
        uint64_t mat_us = t_mat1 - t_mat0;
        uint64_t dec_us = t_dec1 - t_dec0;
        uint64_t rsp_submit_us = t_rsp1 - t_rsp0;
        uint64_t frame_us = t_frame1 - t_frame0;
        frame++;
        if ((frame % 30) == 0) {
            debugf("matcap_generate_us=%llu decode_us=%llu rsp_submit_us=%llu frame_us=%llu\n",
                   (unsigned long long)mat_us,
                   (unsigned long long)dec_us,
                   (unsigned long long)rsp_submit_us,
                   (unsigned long long)frame_us);
        }
    }

    sprite_free(albedo_spr);
    sprite_free(packed_spr);

    for (int i = 0; i < PIPELINE_BUFFERS; i++) {
        surface_free(&decoded_lighting_surf[i]);
        free_matcaps(&mats_db[i]);
    }

    t3d_destroy();
    return 0;
}
