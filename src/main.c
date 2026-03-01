#include <libdragon.h>
#include <t3d/t3d.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "pbr_decode.h"
#include "pbr_matcap.h"
#include "pfm_hdri.h"

const resolution_t RESOLUTION_256x224 = {.width = 256, .height = 224, .interlaced = INTERLACE_OFF};

#define FB_COUNT 3

#define HDRI_PATH "rom:/textures/ferndale_studio"
#define GBUFFER_ALBEDO_PATH "rom:/models/albedo.rgba16.sprite"
#define GBUFFER_PACKED_PATH "rom:/models/packed.rgba16.sprite"
#define DEBUG_DRAW_MATCAPS 0
#define DRAW_FRAME 1
#define DUMMY_MATCAP_W 16
#define DUMMY_MATCAP_H 32
#define DUMMY_MATCAP_TEXELS ((size_t)DUMMY_MATCAP_W * (size_t)DUMMY_MATCAP_H)

static matcap_rgba_t dummy_diffuse_matcap[DUMMY_MATCAP_TEXELS];
static matcap_rgba_t dummy_rough25_matcap[DUMMY_MATCAP_TEXELS];
static matcap_rgba_t dummy_rough75_matcap[DUMMY_MATCAP_TEXELS];

static void init_dummy_matcaps_16x32(MatcapSet *out)
{
    for (int y = 0; y < DUMMY_MATCAP_H; y++) {
        for (int x = 0; x < DUMMY_MATCAP_W; x++) {
            size_t idx = (size_t)y * (size_t)DUMMY_MATCAP_W + (size_t)x;
            uint8_t u = (uint8_t)((x * 255) / (DUMMY_MATCAP_W - 1));
            uint8_t v = (uint8_t)((y * 255) / (DUMMY_MATCAP_H - 1));
            dummy_diffuse_matcap[idx].c[0] = u;
            dummy_diffuse_matcap[idx].c[1] = v;
            dummy_diffuse_matcap[idx].c[2] = 192u;
            dummy_diffuse_matcap[idx].c[3] = 255u;

            dummy_rough25_matcap[idx].c[0] = 48u;
            dummy_rough25_matcap[idx].c[1] = u;
            dummy_rough25_matcap[idx].c[2] = v;
            dummy_rough25_matcap[idx].c[3] = 255u;

            dummy_rough75_matcap[idx].c[0] = 192u;
            dummy_rough75_matcap[idx].c[1] = v;
            dummy_rough75_matcap[idx].c[2] = u;
            dummy_rough75_matcap[idx].c[3] = 255u;
        }
    }

    /* Emission fallback texel: diffuse=white, specular=black. */
    memset(&dummy_diffuse_matcap[0], 0xFF, sizeof(dummy_diffuse_matcap[0]));
    memset(&dummy_rough25_matcap[0], 0x00, sizeof(dummy_rough25_matcap[0]));
    memset(&dummy_rough75_matcap[0], 0x00, sizeof(dummy_rough75_matcap[0]));
    dummy_rough25_matcap[0].c[3] = 255u;
    dummy_rough75_matcap[0].c[3] = 255u;

    out->w = DUMMY_MATCAP_W;
    out->h = DUMMY_MATCAP_H;
    out->diffuse = dummy_diffuse_matcap;
    out->rough25 = dummy_rough25_matcap;
    out->rough75 = dummy_rough75_matcap;
}

const uint16_t *sprite_pixels_u16(const sprite_t *spr)
{
    return (const uint16_t *)spr->data;
}

bool sprite_dim_match(const sprite_t *spr, int w, int h)
{
    return spr && spr->width == w && spr->height == h;
}

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

#if DEBUG_DRAW_MATCAPS
static inline const matcap_rgba_t *debug_matcap_texel(const MatcapSet *mats, int slot, int tex_idx)
{
    if (slot == 0) {
        return &mats->diffuse[tex_idx];
    }
    if (slot == 1) {
        return &mats->rough25[tex_idx];
    }
    return &mats->rough75[tex_idx];
}

static void draw_debug_matcaps(uint16_t *dst, int w, int h, const MatcapSet *mats)
{
    const int cols = 3;    /* diffuse / rough25 / rough75 */
    const int rows = 1;
    const int tile = 80;
    const int pad = 1;
    const int ox0 = 2;
    const int oy0 = 2;
    const int src_size = MATCAP_SIZE;

    for (int yslot = 0; yslot < rows; yslot++) {
        for (int xslot = 0; xslot < cols; xslot++) {
            int ox = ox0 + xslot * (tile + pad);
            int oy = oy0 + yslot * (tile + pad);
            int slot = xslot;

            for (int ty = 0; ty < tile; ty++) {
                int py = oy + ty;
                if ((unsigned)py >= (unsigned)h) continue;
                int sy = (ty * src_size) / tile;

                for (int tx = 0; tx < tile; tx++) {
                    int px = ox + tx;
                    if ((unsigned)px >= (unsigned)w) continue;
                    int sx = (tx * src_size) / tile;
                    int sidx = sy * src_size + sx;
                    const matcap_rgba_t *src = debug_matcap_texel(mats, slot, sidx);
                    dst[(size_t)py * (size_t)w + (size_t)px] =
                        pack_rgb5551_from_u88((uint16_t)src->c[0] << 2,
                                              (uint16_t)src->c[1] << 2,
                                              (uint16_t)src->c[2] << 2);
                }
            }
        }
    }
}
#endif

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
    ls->count = 2;

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

    HDRISet hdri = {0};
    if (!load_pfm_hdri(HDRI_PATH, &hdri)) {
        debugf("HDRI load failed\n");
        for (;;) {}
    }

    MatcapSet mats;
    if (!alloc_matcaps(&mats)) {
        debugf("matcap alloc failed\n");
        for (;;) {}
    }

    uint16_t *final_rgba16 = malloc((size_t)w * (size_t)h * sizeof(uint16_t));
    if (!final_rgba16) {
        debugf("final buffer alloc failed\n");
        for (;;) {}
    }
    
    surface_t decoded_diffuse_surf = surface_alloc(FMT_RGBA32, w, h);
    uint32_t *decoded_diffuse = UncachedAddr(decoded_diffuse_surf.buffer);
    surface_t decoded_rough25_surf = surface_alloc(FMT_RGBA32, w, h);
    uint32_t *decoded_rough25 = UncachedAddr(decoded_rough25_surf.buffer);
    surface_t decoded_rough75_surf = surface_alloc(FMT_RGBA32, w, h);
    uint32_t *decoded_rough75 = UncachedAddr(decoded_rough75_surf.buffer);

    //surface_t decoded_diffuse_surf = surface_alloc(FMT_RGBA32, w, h);
    //uint32_t *decoded_diffuse = malloc_uncached((size_t)w * (size_t)h * sizeof(uint32_t));
    //surface_t decoded_rough25_surf = surface_alloc(FMT_RGBA32, w, h);
    //uint32_t *decoded_rough25 = malloc_uncached((size_t)w * (size_t)h * sizeof(uint32_t));
    //surface_t decoded_rough75_surf = surface_alloc(FMT_RGBA32, w, h);
    //uint32_t *decoded_rough75 = malloc_uncached((size_t)w * (size_t)h * sizeof(uint32_t));

    if (!decoded_diffuse || !decoded_rough25 || !decoded_rough75) {
        debugf("decode buffers alloc failed\n");
        for (;;) {}
    }
    memset(final_rgba16, 0, (size_t)w * (size_t)h * sizeof(uint16_t));

    const uint16_t *albedo = sprite_pixels_u16(albedo_spr);
    const uint16_t *packed = sprite_pixels_u16(packed_spr);
    if (!albedo || !packed) {
        debugf("sprite pixel access failed\n");
        for (;;) {}
    }

    (void)albedo;
    MatcapSet dummy_mats = {0};
    init_dummy_matcaps_16x32(&dummy_mats);

    LightingState lights;
    setup_default_lighting(&lights);

    CameraState cam = {{0.0f, 0.0f, 1.0f}};
    float yaw = 0.0f;
    int frame = 0;

    for (;;) {
        uint64_t t_frame0 = get_ticks_us();

        yaw += 0.03f;
        build_camera_from_yaw(yaw, &cam);

        uint64_t t_mat0 = get_ticks_us();
        generate_matcaps(&cam, &lights, &hdri, &mats);
        uint64_t t_mat1 = get_ticks_us();

        uint64_t t_com0 = get_ticks_us();
#if DRAW_FRAME
        decode_packed_cpu(packed,
                                  decoded_diffuse,
                                  decoded_rough25,
                                  decoded_rough75,
                                  w,
                                  h,
                                  &dummy_mats);
#endif
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
            debugf("matcap_generate_us=%llu decode_16x32_us=%llu frame_us=%llu\n",
                   (unsigned long long)mat_us,
                   (unsigned long long)com_us,
                   (unsigned long long)frame_us);
        }
        (void)mat_us;
        (void)com_us;
        (void)frame_us;
    }

    sprite_free(albedo_spr);
    sprite_free(packed_spr);
    free(final_rgba16);
    //surface_free(&decoded_diffuse_surf);
    //surface_free(&decoded_rough25_surf);
    //surface_free(&decoded_rough75_surf);

    t3d_destroy();
    return 0;
}
