#include <libdragon.h>

#include <stdint.h>

#include "pbr_combine.h"

#define FIXED_ONE 256u
#define FIXED_MAX 65535u

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
    out_rgb[0] = (uint16_t)((r5 * FIXED_ONE + 15u) >> 5);
    out_rgb[1] = (uint16_t)((g5 * FIXED_ONE + 15u) >> 5);
    out_rgb[2] = (uint16_t)((b5 * FIXED_ONE + 15u) >> 5);
}

static inline uint16_t pack_rgb5551_from_u88(uint16_t r88, uint16_t g88, uint16_t b88)
{
    /* Linear clamp to [0,1] in u8.8, then map to RGB555. */
    if (r88 > FIXED_ONE) r88 = FIXED_ONE;
    if (g88 > FIXED_ONE) g88 = FIXED_ONE;
    if (b88 > FIXED_ONE) b88 = FIXED_ONE;

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
        FIXED_ONE,
        (uint16_t)(FIXED_ONE * 2 / 4),
        (uint16_t)(FIXED_ONE * 1 / 4),
        (uint16_t)(FIXED_ONE * 0 / 4)
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

        /* Directly use RRRYYYYYXXXXX as packed spec index. */
        uint16_t spec_idx = (uint16_t)((p >> 1) & 0x1FFFu);
        uint16_t tex_idx = (uint16_t)(spec_idx & 0x03FFu);    /* YYYYYXXXXX */

        uint16_t albedo16[3];
        unpack_rgba16_to_u16x3(a, albedo16);

        const hdr16_rgb_t *left = NULL;
        const hdr16_rgb_t *right = NULL;
        if (m == 0) {
            left = &mats->diffuse[tex_idx];
            right = &mats->spec[spec_idx];
        } else {
            left = &mats->spec_mod[spec_idx];
            right = &mats->spec_fres[spec_idx];
        }

        uint16_t ao_scale = ao_lut[ao];
        uint16_t out_c[3];
        for (int c = 0; c < 3; c++) {
            uint32_t mul = ((uint32_t)left->c[c] * (uint32_t)albedo16[c]) >> 8;
            uint32_t sum = mul + (uint32_t)right->c[c];
            //if (sum > FIXED_MAX) sum = FIXED_MAX;
            uint32_t ao_applied = (sum * (uint32_t)ao_scale) >> 8;
            //if (ao_applied > FIXED_MAX) ao_applied = FIXED_MAX;
            out_c[c] = (uint16_t)ao_applied;
        }

        out_rgba16[i] = pack_rgb5551_from_u88(out_c[0], out_c[1], out_c[2]);
    }
}
