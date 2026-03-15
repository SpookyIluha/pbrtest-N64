/**
 * Pixelshader - example of using RSP to manipulate pixels
 * 
 * This example shows how to achieve additive alpha blending using RSP.
 * It is meant just as an example of doing per-pixel effects with RSP. The
 * name "pixel shader" is catchy but the technique cannot be used as a real
 * pixel shader: it is only possible to preprocess a texture/sprite using
 * RSP before using RDP to draw it.
 */

#include <libdragon.h>
#include "pbr_types.h"
#include "rsp_pbr_blend_constants.inc"

static uint32_t ovl_id;
static void rsp_pbr_blend_assert_handler(rsp_snapshot_t *state, uint16_t code);

enum {
    // Overlay commands. This must match the command table in the RSP code
    RSP_PBR_BLEND_CMD_SET_GBUFFER = 0x0,
    RSP_PBR_BLEND_CMD_SET_LIGHTING_BUFFERS = 0x1,
    RSP_PBR_BLEND_CMD_SET_DITHER_MATRIX = 0x2,
    RSP_PBR_BLEND_CMD_POSTPROCESS = 0x3
};

#define DITHER_MULTIPLIER 64

static const uint16_t bayer8x2_dither_signed[16] = {
    // Row 0
    (uint16_t)(( 5) * DITHER_MULTIPLIER),
    (uint16_t)((12) * DITHER_MULTIPLIER),
    (uint16_t)(( 3) * DITHER_MULTIPLIER),
    (uint16_t)((10) * DITHER_MULTIPLIER),
    (uint16_t)((4) * DITHER_MULTIPLIER),
    (uint16_t)((9) * DITHER_MULTIPLIER),
    (uint16_t)((11) * DITHER_MULTIPLIER),
    (uint16_t)((14) * DITHER_MULTIPLIER),

    // Row 1
    (uint16_t)((11) * DITHER_MULTIPLIER),
    (uint16_t)((3) * DITHER_MULTIPLIER),
    (uint16_t)((15) * DITHER_MULTIPLIER),
    (uint16_t)((8) * DITHER_MULTIPLIER),
    (uint16_t)((12) * DITHER_MULTIPLIER),
    (uint16_t)((8) * DITHER_MULTIPLIER),
    (uint16_t)((13) * DITHER_MULTIPLIER),
    (uint16_t)((9) * DITHER_MULTIPLIER),
};

// Overlay definition
DEFINE_RSP_UCODE(rsp_pbr_blend,
    .assert_handler = rsp_pbr_blend_assert_handler);

void rsp_pbr_blend_init(void) {
    // Initialize if rspq (if it isn't already). It's best practice to let all overlays
    // always call rspq_init(), so that they can be themselves initialized in any order
    // by the user.
    rspq_init();
    ovl_id = rspq_overlay_register(&rsp_pbr_blend);
}

void rsp_pbr_blend_assert_handler(rsp_snapshot_t *state, uint16_t code) {
    assertf(0, "PBR Blend crashed with code %i", code);
}

void rsp_pbr_blend_set_gbuffer(const surface_t *albedo, const surface_t *packed, const surface_t* destination) {
    assert(albedo && packed && destination);
    assertf(surface_get_format(albedo) == FMT_RGBA16 
    && surface_get_format(packed) == FMT_RGBA16 
    && surface_get_format(destination) == FMT_RGBA16,
        "rsp_pbr_blend only handles RGB5551 surfaces");
    
    assertf((packed->width == albedo->width) && (packed->height == albedo->height), "Mismatched PBR buffer sizes");
    assertf((destination->width == albedo->width) && (destination->height == albedo->height), "Mismatched output buffer sizes");
    size_t pixels = albedo->width * albedo->height;
    assertf(pixels % (BUFFER_PIXELS*2) == 0, "Surface buffer's pixel count must be multiple of %i", (BUFFER_PIXELS*2));

    rspq_write(ovl_id, RSP_PBR_BLEND_CMD_SET_GBUFFER, PhysicalAddr(albedo->buffer), PhysicalAddr(packed->buffer),
        PhysicalAddr(destination->buffer), (uint32_t)(pixels / BUFFER_PIXELS));
}

void rsp_pbr_blend_set_lighting_buffers(const surface_t *lighting) {
    assertf(lighting != NULL, "Nullptr Lighting buffer");
    assertf(surface_get_format(lighting) == FMT_RGBA32, "rsp_pbr_blend lighting buffers must be RGBA32");

    rspq_write(ovl_id, RSP_PBR_BLEND_CMD_SET_LIGHTING_BUFFERS, 
        PhysicalAddr(lighting->buffer));
}

void rsp_pbr_blend_set_dither_matrix() {
    rspq_write(ovl_id, RSP_PBR_BLEND_CMD_SET_DITHER_MATRIX, 
        PhysicalAddr(bayer8x2_dither_signed), 0);
}

void rsp_pbr_blend_postprocess() {
    rspq_write(ovl_id, RSP_PBR_BLEND_CMD_POSTPROCESS);
}
