#include <libdragon.h>
#include <t3d/t3d.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "pbr_combine.h"
#include "pbr_matcap.h"
#include "pfm_hdri.h"

const resolution_t RESOLUTION_256x224 = {.width = 256, .height = 224, .interlaced = INTERLACE_OFF};

#define FB_COUNT 3

#define HDRI_PATH "rom:/textures/ferndale_studio_12_2k.pbm"
#define GBUFFER_ALBEDO_PATH "rom:/models/albedo2.rgba16.sprite"
#define GBUFFER_PACKED_PATH "rom:/models/packed2.rgba16.sprite"
#define DEBUG_DRAW_MATCAPS 0

static inline uint16_t pack_rgb5551_from_u88(uint16_t r88, uint16_t g88, uint16_t b88)
{
    if (r88 > 256u) r88 = 256u;
    if (g88 > 256u) g88 = 256u;
    if (b88 > 256u) b88 = 256u;

    uint16_t r5 = (uint16_t)((r88 * 31u + 128u) >> 8);
    uint16_t g5 = (uint16_t)((g88 * 31u + 128u) >> 8);
    uint16_t b5 = (uint16_t)((b88 * 31u + 128u) >> 8);
    return (uint16_t)((r5 << 11) | (g5 << 6) | (b5 << 1) | 1u);
}

static inline const hdr16_rgb_t *debug_matcap_texel(const MatcapSet *mats, int group, int roughness, int tex_idx)
{
    if (group == 0) {
        /* Diffuse has one map, repeat across roughness columns. */
        return &mats->diffuse[tex_idx];
    }
    if (group == 1) {
        return &mats->spec[(roughness << 10) | tex_idx];
    }
    if (group == 2) {
        return &mats->spec_mod[(roughness << 10) | tex_idx];
    }
    return &mats->spec_fres[(roughness << 10) | tex_idx];
}

static void draw_debug_matcaps(uint16_t *dst, int w, int h, const MatcapSet *mats)
{
    const int cols = 8;    /* roughness 0..7 */
    const int rows = 4;    /* diffuse/spec/spec_mod/spec_fres */
    const int tile = 30;   /* fit 8 columns into 256-wide frame */
    const int pad = 1;
    const int ox0 = 2;
    const int oy0 = 2;
    const int src_size = 32;

    for (int group = 0; group < rows; group++) {
        for (int roughness = 0; roughness < cols; roughness++) {
            int ox = ox0 + roughness * (tile + pad);
            int oy = oy0 + group * (tile + pad);

            for (int ty = 0; ty < tile; ty++) {
                int py = oy + ty;
                if ((unsigned)py >= (unsigned)h) continue;
                int sy = (ty * src_size) / tile;

                for (int tx = 0; tx < tile; tx++) {
                    int px = ox + tx;
                    if ((unsigned)px >= (unsigned)w) continue;
                    int sx = (tx * src_size) / tile;
                    int sidx = (sy << 5) | sx;
                    const hdr16_rgb_t *src = debug_matcap_texel(mats, group, roughness, sidx);
                    dst[(size_t)py * (size_t)w + (size_t)px] =
                        pack_rgb5551_from_u88(src->c[0], src->c[1], src->c[2]);
                }
            }
        }
    }
}

static bool load_gbuffers_from_sprites(const char *albedo_path, const char *packed_path, sprite_t **albedo, sprite_t **packed)
{
    *albedo = sprite_load(albedo_path);
    *packed = sprite_load(packed_path);

    if (!*albedo || !*packed) {
        debugf("sprite load failed: %s / %s\n", albedo_path, packed_path);
        return false;
    }

    return true;
}

static void setup_default_lighting(LightingState *ls)
{
    memset(ls, 0, sizeof(*ls));
    ls->count = 0;

    ls->dir[0][0] = 0.707f;
    ls->dir[0][1] = 0.577f;
    ls->dir[0][2] = 0.408f;
    ls->color[0][0] = 10.0f;
    ls->color[0][1] = 10.0f;
    ls->color[0][2] = 10.0f;

    /*ls->dir[1][0] = -0.4f;
    ls->dir[1][1] = 0.9f;
    ls->dir[1][2] = 0.2f;
    ls->color[1][0] = 10.5f;
    ls->color[1][1] = 8.0f;
    ls->color[1][2] = 8.0f;*/
}

int main(void)
{
    debug_init_isviewer();
    debug_init_usblog();
    asset_init_compression(2);

    dfs_init(DFS_DEFAULT_LOCATION);

    display_init(RESOLUTION_256x224, DEPTH_16_BPP, FB_COUNT, GAMMA_NONE, FILTERS_RESAMPLE_ANTIALIAS_DEDITHER);
    rdpq_init();
    t3d_init((T3DInitParams){});

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

    HDRISet hdri;
    if (!load_pfm_hdri(HDRI_PATH, &hdri)) {
        debugf("HDRI load failed\n");
        for (;;) {}
    }

    prefilter_hdri_roughness_4(&hdri, &hdri);

    MatcapSet mats;
    if (!alloc_matcaps(&mats)) {
        debugf("matcap alloc failed\n");
        for (;;) {}
    }

    uint16_t *final_rgba16 = malloc_uncached((size_t)w * (size_t)h * sizeof(uint16_t));
    if (!final_rgba16) {
        debugf("final buffer alloc failed\n");
        for (;;) {}
    }

    const uint16_t *albedo = sprite_pixels_u16(albedo_spr);
    const uint16_t *packed = sprite_pixels_u16(packed_spr);
    if (!albedo || !packed) {
        debugf("sprite pixel access failed\n");
        for (;;) {}
    }

    LightingState lights;
    setup_default_lighting(&lights);

    CameraState cam = {{0.0f, 0.0f, 1.0f}};
    float yaw = 0.0f;
    int frame = 0;

    for (;;) {
        uint64_t t_frame0 = get_ticks_us();

        yaw += 0.1f;
        build_camera_from_yaw(yaw, &cam);

        uint64_t t_mat0 = get_ticks_us();
        generate_matcaps_ggx(&cam, &lights, &hdri, &mats);
        uint64_t t_mat1 = get_ticks_us();

        uint64_t t_com0 = get_ticks_us();
        combine_deferred_cpu(albedo, packed, final_rgba16, w, h, &mats);
        uint64_t t_com1 = get_ticks_us();
#if DEBUG_DRAW_MATCAPS
        draw_debug_matcaps(final_rgba16, w, h, &mats);
#endif

        surface_t *disp = display_get();
        if (!disp) continue;

        if (disp->width != w || disp->height != h) {
            debugf("Display/sprite dimension mismatch: display=%dx%d sprite=%dx%d\n",
                   disp->width, disp->height, w, h);
            for (;;) {}
        }

        rdpq_attach(disp, display_get_zbuf());

        uint8_t *dst_base = (uint8_t *)disp->buffer;
        int dst_stride = disp->stride;
        for (int y = 0; y < h; y++) {
            memcpy(dst_base + y * dst_stride, &final_rgba16[(size_t)y * (size_t)w], (size_t)w * sizeof(uint16_t));
        }

        rdpq_detach_show();

        uint64_t t_frame1 = get_ticks_us();
        uint64_t mat_us = t_mat1 - t_mat0;
        uint64_t com_us = t_com1 - t_com0;
        uint64_t frame_us = t_frame1 - t_frame0;
        frame++;
        if ((frame % 30) == 0) {
            debugf("matcap_generate_us=%llu combine_us=%llu frame_us=%llu\n",
                   (unsigned long long)mat_us,
                   (unsigned long long)com_us,
                   (unsigned long long)frame_us);
        }
        (void)mat_us;
        (void)com_us;
        (void)frame_us;
    }

    free_matcaps(&mats);
    free_hdri(&hdri);
    sprite_free(albedo_spr);
    sprite_free(packed_spr);
    free_uncached(final_rgba16);

    t3d_destroy();
    return 0;
}
