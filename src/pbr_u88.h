#ifndef PBR_U88_H
#define PBR_U88_H

#include <stdint.h>

#define U88_ZERO 0u
#define U88_ONE 256u
#define U88_HALF 128u
#define U88_MAX 65535u

static inline uint16_t u88_clamp_u32(uint32_t v)
{
    if (v > U88_MAX) return U88_MAX;
    return (uint16_t)v;
}

static inline uint16_t u88_add_sat(uint16_t a, uint16_t b)
{
    return u88_clamp_u32((uint32_t)a + (uint32_t)b);
}

static inline uint16_t u88_sub_sat(uint16_t a, uint16_t b)
{
    if (a <= b) return 0;
    return (uint16_t)(a - b);
}

static inline uint16_t u88_mul(uint16_t a, uint16_t b)
{
    uint32_t v = ((uint32_t)a * (uint32_t)b + U88_HALF) >> 8;
    return (v);
}

static inline uint16_t u88_lerp(uint16_t a, uint16_t b, uint16_t t)
{
    int32_t d = (int32_t)b - (int32_t)a;
    int64_t p = (int64_t)d * (int64_t)t;
    if (p >= 0) p += U88_HALF;
    else p -= U88_HALF;

    int32_t v = (int32_t)a + (int32_t)(p >> 8);
    if (v < 0) return 0;
    if (v > (int32_t)U88_MAX) return U88_MAX;
    return (uint16_t)v;
}

#endif
