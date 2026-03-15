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
    // u3.5 HDR RGB, 1.0 == 64
    uint8_t c[4];
} matcap_color_t;

typedef struct {
    int w;
    int h;
    matcap_color_t *diffuse;
    matcap_color_t *spec25;
    matcap_color_t *spec75;
} MatcapSet;

typedef struct {

    hdri_color_t *diffuse;
    hdri_color_t *spec25;
    hdri_color_t *spec75;
    int diffuse_w;
    int diffuse_h;
    int spec25_w;
    int spec25_h;
    int spec75_w;
    int spec75_h;
    /* Interleaved specular texels: [s25_rgb, s75_rgb, s25_rgb, s75_rgb, ...]. */
    hdri_color_t *specular_interleaved;
    int specular_w;
    int specular_h;
} HDRISet;

typedef struct {
    float forward[3];
} CameraState;

typedef struct {
    int count;
    float dir[4][3];
    float color[4][3];

    float hdri_strength;
    float emission_strength;
    float exposure;
} LightingState;

#endif
