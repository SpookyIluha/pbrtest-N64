#ifndef PFM_HDRI_H
#define PFM_HDRI_H

#include <stdbool.h>
#include "pbr_types.h"

bool load_pfm_hdri(const char *path, HDRISet *out);
void prefilter_hdri_roughness_4(const HDRISet *in, HDRISet *io);
void free_hdri(HDRISet *hdri);

#endif
