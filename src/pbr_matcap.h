#ifndef PBR_MATCAP_H
#define PBR_MATCAP_H

#include <stdbool.h>
#include "pbr_types.h"

bool alloc_matcaps(MatcapSet *m);
void free_matcaps(MatcapSet *m);
void build_camera_from_yaw(float yaw, CameraState *cam);
void generate_matcaps_ggx(const CameraState *cam, const LightingState *lights, const HDRISet *hdri, MatcapSet *out);

#endif
