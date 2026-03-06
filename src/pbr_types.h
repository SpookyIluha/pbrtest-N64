#ifndef PBR_TYPES_H
#define PBR_TYPES_H

#include <stdint.h>

#define MATCAP_W 16
#define MATCAP_H 32
#define MATCAP_TEXELS ((MATCAP_W) * (MATCAP_H))

typedef struct {
    /* u8.8 fixed-point RGB channels (1.0 == 256). */
    uint16_t c[3];
} hdri_color_t;

typedef struct{
    float c[3];
} float_color_t;

typedef struct {
    uint8_t c[4];
} matcap_color_t;

typedef struct {
    int w;
    int h;
    matcap_color_t *diffuse;
    matcap_color_t *rough25;
    matcap_color_t *rough75;
} MatcapSet;

typedef struct {
    /* Keep legacy layout first for object compatibility. */
    int w;
    int h;
    hdri_color_t *diffuse;
    hdri_color_t *rough25;
    hdri_color_t *rough75;
    int diffuse_w;
    int diffuse_h;
    int rough25_w;
    int rough25_h;
    int rough75_w;
    int rough75_h;
} HDRISet;

typedef struct {
    float forward[3];
} CameraState;

typedef struct {
    int count;
    float dir[4][3];
    float color[4][3];
    float hdri_brightness;
    
    float exposure;
} LightingState;

#endif
