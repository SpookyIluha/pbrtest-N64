#ifndef PBR_COMBINE_H
#define PBR_COMBINE_H

#include <stdbool.h>
#include <stdint.h>
#include <libdragon.h>

#include "pbr_types.h"

/*
const uint16_t *sprite_pixels_u16(const sprite_t *spr);
bool sprite_dim_match(const sprite_t *spr, int w, int h);

void combine_deferred_cpu(const uint16_t *albedo_rgba16,
                          const uint16_t *packed16,
                          uint16_t *out_rgba16,
                          int w,
                          int h,
                          const MatcapSet *mats);

void decode_deferred_cpu(const uint16_t *packed16,
                         uint32_t *out_diffuse,
                         uint32_t *out_rough25,
                         uint32_t *out_rough75,
                         int w,
                         int h,
                         const MatcapSet *mats);*/

// Decode the packed buffer and matcap sets into a set of linear lighting buffers
void decode_packed_cpu(const uint16_t *packed16,
                         uint32_t *out_diffuse,
                         uint32_t *out_rough25,
                         uint32_t *out_rough75,
                         int w,
                         int h,
                         const MatcapSet *mats);

// Interleaved block layout for 8 pixels:
// [8x albedo16][8x packed16][8x diffuse32][8x rough25_32][8x rough75_32]
#define DECODE_INTERLEAVED8_PIXELS 8u
#define DECODE_INTERLEAVED8_ALBEDO_BYTES (DECODE_INTERLEAVED8_PIXELS * sizeof(uint16_t))
#define DECODE_INTERLEAVED8_PACKED_BYTES (DECODE_INTERLEAVED8_PIXELS * sizeof(uint16_t))
#define DECODE_INTERLEAVED8_LIGHTING_BYTES (DECODE_INTERLEAVED8_PIXELS * sizeof(uint32_t))
#define DECODE_INTERLEAVED8_BLOCK_BYTES (DECODE_INTERLEAVED8_ALBEDO_BYTES + DECODE_INTERLEAVED8_PACKED_BYTES + (3u * DECODE_INTERLEAVED8_LIGHTING_BYTES))
#define DECODE_INTERLEAVED8_OFF_ALBEDO 0u
#define DECODE_INTERLEAVED8_OFF_PACKED (DECODE_INTERLEAVED8_OFF_ALBEDO + DECODE_INTERLEAVED8_ALBEDO_BYTES)
#define DECODE_INTERLEAVED8_OFF_DIFFUSE (DECODE_INTERLEAVED8_OFF_PACKED + DECODE_INTERLEAVED8_PACKED_BYTES)
#define DECODE_INTERLEAVED8_OFF_ROUGH25 (DECODE_INTERLEAVED8_OFF_DIFFUSE + DECODE_INTERLEAVED8_LIGHTING_BYTES)
#define DECODE_INTERLEAVED8_OFF_ROUGH75 (DECODE_INTERLEAVED8_OFF_ROUGH25 + DECODE_INTERLEAVED8_LIGHTING_BYTES)

// Decode + pack everything into 8-pixel interleaved blocks for low-DMA RSP paths.
void decode_packed_cpu_interleaved8(const uint16_t *albedo16,
                                    const uint16_t *packed16,
                                    uint8_t *out_interleaved,
                                    int w,
                                    int h,
                                    const MatcapSet *mats);

// Lighting-only interleaved block layout for 8 pixels:
// [8x diffuse32][8x rough25_32][8x rough75_32]
#define DECODE_LIGHTING_INTERLEAVED8_BLOCK_BYTES (3u * DECODE_INTERLEAVED8_LIGHTING_BYTES)
#define DECODE_LIGHTING_INTERLEAVED8_OFF_DIFFUSE 0u
#define DECODE_LIGHTING_INTERLEAVED8_OFF_ROUGH25 (DECODE_LIGHTING_INTERLEAVED8_OFF_DIFFUSE + DECODE_INTERLEAVED8_LIGHTING_BYTES)
#define DECODE_LIGHTING_INTERLEAVED8_OFF_ROUGH75 (DECODE_LIGHTING_INTERLEAVED8_OFF_ROUGH25 + DECODE_INTERLEAVED8_LIGHTING_BYTES)

void decode_packed_cpu_lighting_interleaved8(const uint16_t *packed16,
                                             uint8_t *out_lighting_interleaved,
                                             int w,
                                             int h,
                                             const MatcapSet *mats);

/*void decode_deferred_cpu_5bit(const uint16_t *packed16,
                              uint32_t *out_diffuse,
                              uint32_t *out_specular,
                              int w,
                              int h,
                              const uint32_t *matcap_diffuse,
                              const uint32_t *matcap_specular);*/

#endif
