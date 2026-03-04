#ifndef PBR_BLEND_H
#define PBR_BLEND_H

#include <libdragon.h>
#include "pbr_types.h"

void rsp_pbr_blend_init(void);
void rsp_pbr_blend_set_gbuffer(const surface_t *albedo, const surface_t *packed, const surface_t *destination);
void rsp_pbr_blend_set_lighting_buffer(const surface_t *lighting);
void rsp_pbr_blend_set_dither_matrix(void);
void rsp_pbr_blend_postprocess(void);

#endif // PBR_BLEND_H