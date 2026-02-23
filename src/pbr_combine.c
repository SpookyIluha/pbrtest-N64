#include <libdragon.h>

#include <stdint.h>

#include "pbr_combine.h"
#include "pbr_u88.h"

const uint16_t *sprite_pixels_u16(const sprite_t *spr)
{
    return (const uint16_t *)spr->data;
}

bool sprite_dim_match(const sprite_t *spr, int w, int h)
{
    return spr && spr->width == w && spr->height == h;
}

static inline void unpack_rgba16_to_u16x3(uint16_t c, uint16_t out_rgb[3])
{
    uint16_t r5 = (c >> 11) & 0x1Fu;
    uint16_t g5 = (c >> 6)  & 0x1Fu;
    uint16_t b5 = (c >> 1)  & 0x1Fu;

    /* Expand 5-bit albedo to u8.8 fixed [0..256]. */
    out_rgb[0] = (uint16_t)((r5 * U88_ONE + 15u) >> 5);
    out_rgb[1] = (uint16_t)((g5 * U88_ONE + 15u) >> 5);
    out_rgb[2] = (uint16_t)((b5 * U88_ONE + 15u) >> 5);
}

static inline uint16_t pack_rgb5551_from_u88(uint16_t r88, uint16_t g88, uint16_t b88)
{
    /* Linear clamp to [0,1] in u8.8, then map to RGB555. */
    if (r88 > U88_ONE) r88 = U88_ONE;
    if (g88 > U88_ONE) g88 = U88_ONE;
    if (b88 > U88_ONE) b88 = U88_ONE;

    uint16_t r5 = (uint16_t)((r88 * 31u + 128u) >> 8);
    uint16_t g5 = (uint16_t)((g88 * 31u + 128u) >> 8);
    uint16_t b5 = (uint16_t)((b88 * 31u + 128u) >> 8);

    return (uint16_t)((r5 << 11) | (g5 << 6) | (b5 << 1) | 1u);
}

void combine_deferred_cpu(const uint16_t *albedo_rgba16,
                          const uint16_t *packed16,
                          uint16_t *out_rgba16,
                          int w,
                          int h,
                          const MatcapSet *mats)
{
    static const uint16_t ao_lut[4] = {
        U88_ONE,
        (uint16_t)(U88_ONE * 2 / 4),
        (uint16_t)(U88_ONE * 1 / 4),
        (uint16_t)(U88_ONE * 0 / 4)
    };

    size_t n = (size_t)w * (size_t)h;
    for (size_t i = 0; i < n; i++) {
        uint16_t a = albedo_rgba16[i];

        if ((a & 0x0001u) == 0) {
            out_rgba16[i] = a;
            continue;
        }

        uint16_t p = packed16[i];
        uint8_t ao = (uint8_t)((p >> 14) & 0x03u);
        uint8_t m = (uint8_t)(p & 0x01u);

        /*
         * Legacy packed layout is still RRRYYYYYXXXXX (32x32 source).
         * Downscale XY to MATCAP_SIZE and blend between roughness anchors.
         */
        uint16_t packed_idx = (uint16_t)((p >> 1) & 0x1FFFu);
        uint16_t tex_idx32 = (uint16_t)(packed_idx & 0x03FFu);
        uint8_t rough = (uint8_t)((packed_idx >> 10) & 0x07u);

        uint8_t sx32 = (uint8_t)(tex_idx32 & 0x1Fu);
        uint8_t sy32 = (uint8_t)((tex_idx32 >> 5) & 0x1Fu);
        uint8_t sx = (uint8_t)((sx32 * MATCAP_SIZE) >> 5);
        uint8_t sy = (uint8_t)((sy32 * MATCAP_SIZE) >> 5);
        uint16_t tex_idx = (uint16_t)((uint16_t)sy * MATCAP_SIZE + (uint16_t)sx);
        uint16_t rough_t = (uint16_t)((rough * U88_ONE + 3u) / 7u);

        uint16_t albedo16[3];
        unpack_rgba16_to_u16x3(a, albedo16);

        const hdr16_rgb_t *diff = &mats->diffuse[tex_idx];
        const hdr16_rgb_t *spec_lo = &mats->rough25[tex_idx];
        const hdr16_rgb_t *spec_hi = &mats->rough75[tex_idx];

        uint16_t ao_scale = ao_lut[ao];
        uint16_t out_c[3];
        for (int c = 0; c < 3; c++) {
            uint16_t spec = u88_lerp(spec_lo->c[c], spec_hi->c[c], rough_t);
            uint16_t shaded;
            if (m == 0) {
                uint16_t lambert = u88_mul(diff->c[c], albedo16[c]);
                shaded = u88_add_sat(lambert, spec);
            } else {
                shaded = u88_mul(spec, albedo16[c]);
            }

            out_c[c] = u88_mul(shaded, ao_scale);
        }

        out_rgba16[i] = pack_rgb5551_from_u88(out_c[0], out_c[1], out_c[2]);
    }
}
