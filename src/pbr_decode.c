#include <libdragon.h>

#include <stdint.h>

#include "pbr_decode.h"
#include "pbr_u88.h"
/*
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

    // Expand 5-bit albedo to u8.8 fixed [0..256]. 
    out_rgb[0] = (uint16_t)((r5 * U88_ONE + 15u) >> 5);
    out_rgb[1] = (uint16_t)((g5 * U88_ONE + 15u) >> 5);
    out_rgb[2] = (uint16_t)((b5 * U88_ONE + 15u) >> 5);
}

static inline uint16_t u26_to_u88(uint8_t v26)
{
    return (uint16_t)v26 << 2; // u2.6 -> u8.8 
}

static inline uint16_t u08_to_u88(uint8_t v08)
{
    return (uint16_t)((((uint32_t)v08 * U88_ONE) + 127u) / 255u);
}

static inline uint16_t pack_rgb5551_from_u88(uint16_t r88, uint16_t g88, uint16_t b88)
{
    // Linear clamp to [0,1] in u8.8, then map to RGB555. 
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
    size_t n = (size_t)w * (size_t)h;
    for (size_t i = 0; i < n; i++) {
        const uint16_t a = albedo_rgba16[i];

        if ((a & 0x0001u) == 0) {
            out_rgba16[i] = a;
            continue;
        }

        uint8_t albedo16[3];
        // albedo: RRRRRGGGGGBBBBBA
        albedo16[0] = (a >> 8) & 0xF8u;
        albedo16[1] = (a >> 3) & 0xF8u;
        albedo16[2] = (a << 2) & 0xF8u;

        const uint16_t p = packed16[i];
        // packed16: RRRRRYYYY0XXXX0M
        const uint8_t roughness = (uint8_t)((p >> 8) & 0xF8u);
        const uint8_t tex_idx = (uint8_t)(((p >> 2) & 0x0Fu) | ((p >> 3) & 0xF0u));
        const uint16_t m = (uint16_t)((p & 0x01u) << 8); // either 1.0 fixed or 0.0 fixed
        const uint16_t onem_roughness = U88_ONE - roughness;
        const uint16_t onem_m = U88_ONE - m;

        // sample matcaps
        const matcap_rgba_t *diff = &mats->diffuse[tex_idx];
        const matcap_rgba_t *spec_lo = &mats->rough25[tex_idx];
        const matcap_rgba_t *spec_hi = &mats->rough75[tex_idx];

        const uint16_t diff_rgb[3] = {
            (uint16_t)(diff->c[0]),
            (uint16_t)(diff->c[1]),
            (uint16_t)(diff->c[2])
        };
        const uint16_t spec_lo_rgb[3] = {
            (uint16_t)(spec_lo->c[0]),
            (uint16_t)(spec_lo->c[1]),
            (uint16_t)(spec_lo->c[2])
        };
        const uint16_t spec_hi_rgb[3] = {
            (uint16_t)(spec_hi->c[0]),
            (uint16_t)(spec_hi->c[1]),
            (uint16_t)(spec_hi->c[2])
        };
        const uint16_t fresnel = (uint16_t)(spec_lo->c[3]); // u0.8

        uint16_t out_c[3];
        for (int c = 0; c < 3; c++) {
            const uint16_t diff = (diff_rgb[c]) << 2;  // u2.6
            const uint16_t spec_lo = (spec_lo_rgb[c]) << 2;
            const uint16_t spec_hi = (spec_hi_rgb[c]) << 2;

            // u88 lerp using roughness as u0.8 factor.
            const uint16_t spec_high_fac = u88_mul(spec_hi, roughness);
            const uint16_t spec_low_fac = u88_mul(spec_lo, onem_roughness);
            const uint16_t specular = u88_add_sat(spec_low_fac, spec_high_fac);

            // fresnel factor
            const uint16_t specular_fres = u88_mul(fresnel, (uint16_t)specular);

            // metal factor 
            uint16_t metal = u88_mul(albedo16[c], (uint16_t)specular);
            metal = u88_mul(metal, m); // either 0 or 1 in u8.8
        
            // dielectric factor 
            uint16_t dielectic = u88_mul(albedo16[c], diff);
            dielectic = u88_mul(dielectic, onem_m); // either 0 or 1 in u8.8 

            // combine all shading 
            uint16_t shaded = u88_add_sat(dielectic, specular_fres);
            shaded = u88_add_sat(shaded, metal);

            out_c[c] = shaded;
        }

        out_rgba16[i] = pack_rgb5551_from_u88(out_c[0], out_c[1], out_c[2]);
    }
}

#define CACHE_OP_DIRTY ((0x3 << 2) | 0x1)
#define CACHE_OP_FLUSH ((0x5 << 2) | 0x1)

void decode_deferred_cpu(const uint16_t *packed16,
                          uint32_t *out_diffuse,
                          uint32_t *out_rough25,
                          uint32_t *out_rough75,
                          int w,
                          int h,
                          const MatcapSet *mats)
{
    register size_t n = (size_t)w * (size_t)h;
    register uint8_t *matcap_diffuse = (uint8_t *)mats->diffuse;
    register uint8_t *matcap_rough25 = (uint8_t *)mats->rough25;
    register uint8_t *matcap_rough75 = (uint8_t *)mats->rough75;

    // try to precache the matcaps into CPU's cache:
    for(int i = 0; i < MATCAP_SIZE*MATCAP_SIZE/2; i+=4)
    {
        register uint64_t d = matcap_diffuse[i];
        register uint64_t s25 = matcap_rough25[i];
        register uint64_t s75 = matcap_rough75[i];
        (void)d;
        (void)s25;
        (void)s75;
    }

    // decode the packed buffer into matcap buffers, so the RSP doesn't have to decode it
    for (register size_t i = 0; i < n; i++) {
        const register uint16_t a = packed16[i];        // ERRRRYYYY0XXXX0M
        register uint16_t index = 0;
        if(a & 0x8000) {
            const register uint16_t indexy = (a >> 1) & 0b0000001111000000; // 000000YYYY000000
            index = (uint16_t)((a & 0b0000000000111100) | indexy);
        }

        out_diffuse[i] = *(uint32_t*)&matcap_diffuse[index];
        out_rough25[i] = *(uint32_t*)&matcap_rough25[index];
        out_rough75[i] = *(uint32_t*)&matcap_rough75[index];
    }

}*/

#define CACHE_OP_DIRTY ((0x3 << 2) | 0x1)
#define CACHE_OP_FLUSH ((0x5 << 2) | 0x1)
void decode_packed_cpu(const uint16_t *packed16,
                          uint32_t *out_diffuse,
                          uint32_t *out_rough25,
                          uint32_t *out_rough75,
                          int w,
                          int h,
                          const MatcapSet *mats)
{
    register size_t n = (size_t)w * (size_t)h;

    // treat the matcaps as if they were 8-bit so we don't need to do a srl >> 2 in a loop
    register uint8_t *matcap_diffuse = (uint8_t *)mats->diffuse;
    register uint8_t *matcap_rough25 = (uint8_t *)mats->rough25;
    register uint8_t *matcap_rough75 = (uint8_t *)mats->rough75;
    // matcaps are 16x32 transposed(!) so it's faster to decode them, 
    // but the actual normals encoded in the packed buffer should be RRRRRXXXXXYYYY0M
    // and the matcaps themselves should be generated with normals swapped as well

    // try to precache the matcaps into CPU's cache:
    // 16x32 matcaps are 6KB which still fits into CPU's cache
    // doesn't seem to matter much here so commented out
    /*for(int i = 0; i < MATCAP_SIZE*MATCAP_SIZE*2*4; i+=8)
    {
        register uint64_t d = matcap_diffuse[i];
        register uint64_t s25 = matcap_rough25[i];
        register uint64_t s75 = matcap_rough75[i];
        (void)d;
        (void)s25;
        (void)s75;
    }*/

    // decode the packed buffer normals and matcaps into lighting buffers on CPU, 
    // so the RSP doesn't have to decode it itself (which it can't adequately) 
    // and just let it access linear buffers, and that's the fastest way to do it

    for (register size_t i = 0; i < n; i++) {
        const register uint16_t p = packed16[i]; // RRRRRYYYYYXXXX0M
        register uint16_t index = 0;

        // The emission 'bit' works like this: if roughness is set to 0, then use index = 0,
        // where the matcaps encode diffuse = 1.0 and specular = 0.0 making it a passthrough,
        // otherwise do the PBR workflow. 
		// 
        // If the packed buffer is initialized to 0 (as is the case when not drawn to it), 
		// then it will also be a passthrough
        if(p & 0b11110'00000'00000'0) {
            index = p & 0b00000'11111'11110'0; // 00000YYYYYXXXX00
        }

        // Create Dirty Exclusive for the lighting buffers to not read them to cache
        // TODO: doesn't work, just an uncached address is faster here
        /*if(i % 4 == 0) {
            asm ("\tcache %0,(%1)\n"::"i" (CACHE_OP_DIRTY), "r" (&out_diffuse[i]));
            asm ("\tcache %0,(%1)\n"::"i" (CACHE_OP_DIRTY), "r" (&out_rough25[i]));
            asm ("\tcache %0,(%1)\n"::"i" (CACHE_OP_DIRTY), "r" (&out_rough75[i]));
        }*/

        out_diffuse[i] = *(uint32_t*)&matcap_diffuse[index];
        out_rough25[i] = *(uint32_t*)&matcap_rough25[index];
        out_rough75[i] = *(uint32_t*)&matcap_rough75[index];
    }
    //assert(!packed16);
}

/*
// too slow because of cache misses
void decode_deferred_cpu_5bit(const uint16_t *packed16,
                              uint32_t *out_diffuse,
                              uint32_t *out_specular,
                              int w,
                              int h,
                              const uint32_t *matcap_diffuse,
                              const uint32_t *matcap_specular)
{
    register size_t n = (size_t)w * (size_t)h;

    // Treat inputs as u16 stream so masked indices can be used directly.
    // Diffuse: 32x32x1. Specular: 32x32x16.
    const uint16_t *matcap_diffuse_u16 = (const uint16_t *)matcap_diffuse;
    const uint16_t *matcap_specular_u16 = (const uint16_t *)matcap_specular;

    // decode the packed buffer into matcap buffers, so the RSP doesn't have to decode it
    for (register size_t i = 0; i < n; i++) {

        register uint16_t index_diffuse = 0;
        register uint16_t index_specular = 0;

        const register uint16_t p = packed16[i]; // ERRRRYYYYYXXXXXM

        // The emission bit works like this: if set to 0, then use index = 0,
        // where diffuse = 1.0 and specular = 0.0, otherwise work as usual
        if(p < 0x8000){
            index_specular = p & 0b0111111111111110;
            index_diffuse = p & 0b0000011111111110;
        }

        out_diffuse[i] = *(const uint32_t *)&matcap_diffuse_u16[index_diffuse];
        out_specular[i] = *(const uint32_t *)&matcap_specular_u16[index_specular];
    }

}
*/