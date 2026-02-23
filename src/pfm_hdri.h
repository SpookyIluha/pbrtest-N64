#ifndef PFM_HDRI_H
#define PFM_HDRI_H

#include <stdbool.h>
#include "pbr_types.h"

bool load_pfm_hdri(const char *path, HDRISet *out);
void free_hdri(HDRISet *hdri);

#endif
