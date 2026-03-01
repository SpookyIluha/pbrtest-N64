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

/*void decode_deferred_cpu_5bit(const uint16_t *packed16,
                              uint32_t *out_diffuse,
                              uint32_t *out_specular,
                              int w,
                              int h,
                              const uint32_t *matcap_diffuse,
                              const uint32_t *matcap_specular);*/

#endif
