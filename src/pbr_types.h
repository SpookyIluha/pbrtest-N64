#ifndef PBR_TYPES_H
#define PBR_TYPES_H

#include <stdint.h>

#define MATCAP_SIZE 32

typedef struct {
    /* u8.8 fixed-point RGB channels (1.0 == 256). */
    uint16_t c[3];
} hdr16_rgb_t;

typedef struct {
    int w;
    int h;
    hdr16_rgb_t *diffuse;
    hdr16_rgb_t *spec;
    hdr16_rgb_t *spec_mod;
    hdr16_rgb_t *spec_fres;
} MatcapSet;

typedef struct {
    int w;
    int h;
    float *rgb;
    float *prefilter[4];
    float *diffuse_irr;
} HDRISet;

typedef struct {
    float forward[3];
} CameraState;

typedef struct {
    float dir[4][3];
    float color[4][3];
    int count;
} LightingState;

#endif
