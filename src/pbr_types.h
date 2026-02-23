#ifndef PBR_TYPES_H
#define PBR_TYPES_H

#include <stdint.h>

#define MATCAP_SIZE 16

typedef struct {
    /* u8.8 fixed-point RGB channels (1.0 == 256). */
    uint16_t c[3];
} hdr16_rgb_t;

typedef struct {
    int w;
    int h;
    hdr16_rgb_t *diffuse;
    hdr16_rgb_t *rough25;
    hdr16_rgb_t *rough75;
} MatcapSet;

typedef struct {
    int w;
    int h;
    hdr16_rgb_t *diffuse;
    hdr16_rgb_t *rough25;
    hdr16_rgb_t *rough75;
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
