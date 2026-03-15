#ifndef PBR_COMBINE_H
#define PBR_COMBINE_H

#include <stdbool.h>
#include <stdint.h>
#include <libdragon.h>

#include "pbr_types.h"

// Interleaved block layout for 8 pixels:
// [8x diffuse RGBA32][8x spec25 RGBA32][8x spec75 RGBA32]
#define DECODE_INTERLEAVED8_PIXELS 8u

void cpu_decode_packed_to_interleaved_lighting(const uint16_t *packed16,
                                             uint32_t *out_lighting_interleaved,
                                             int w,
                                             int h,
                                             const MatcapSet *mats);


#endif
