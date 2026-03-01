#ifndef PBR_BLEND_H
#define PBR_BLEND_H

#include <libdragon.h>
#include "pbr_types.h"

void rsp_pbr_blend_init(void);
void rsp_pbr_blend_set_gbuffer(const surface_t *albedo, const surface_t *packed, const surface_t *destination);
void rsp_pbr_blend_set_lighting_buffers(const surface_t *diffuse, const surface_t *rough25, const surface_t *rough75);
void rsp_pbr_blend_postprocess(void);

#endif // PBR_BLEND_H