#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "pbr_blend.h"
#include "pbr_decode.h"
#include "pbr_matcap.h"
#include "pfm_hdri.h"

#define FB_COUNT 3
#define PIPELINE_BUFFERS 1
#define BUFFER_W 320
#define BUFFER_H 240
#define EXPOSURE_MIN 0.0f
#define EXPOSURE_MAX 9999.0f
#define EXPOSURE_STEP 0.01f

//#define HDRI_PATH "rom:/textures/courtyard"
//#define HDRI_PATH "rom:/textures/ferndale_studio"
//#define HDRI_PATH "rom:/textures/sunset"
#define HDRI_PATH "rom:/textures/courtyard"
#define MODEL_ALBEDO_PATH "rom:/models/scene.t3dm"
#define MODEL_PACKED_PATH "rom:/models/scene_p.t3dm"
#define MEASURE_RSP_PERF 0

static void setup_default_lighting(LightingState *ls)
{
    memset(ls, 0, sizeof(*ls));
    ls->count = 1;

    ls->dir[0][0] = 0.707f;
    ls->dir[0][1] = 0.577f;
    ls->dir[0][2] = 0.408f;
    ls->color[0][0] = sqrt(10.0f);
    ls->color[0][1] = sqrt(6.0f);
    ls->color[0][2] = sqrt(4.0f);

    ls->exposure = 1.0f;
    ls->hdri_strength = 1.0f;
    ls->emission_strength = 1.0f;
}

static inline float clampf_local(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

int main(void)
{
    debug_init_isviewer();
    debug_init_usblog();
    asset_init_compression(2);

    dfs_init(DFS_DEFAULT_LOCATION);
    joypad_init();

    display_init(RESOLUTION_320x240, DEPTH_16_BPP, FB_COUNT, GAMMA_NONE, FILTERS_RESAMPLE_ANTIALIAS_DEDITHER);
    rdpq_init();
    t3d_init((T3DInitParams){});
    rsp_pbr_blend_init();

    int w = BUFFER_W;
    int h = BUFFER_H;

    rspq_block_t* albedoblock = NULL;
    rspq_block_t* packedblock = NULL;

    HDRISet hdri = {0};
    if (!load_pfm_hdri(HDRI_PATH, &hdri)) {
        assertf(0,"HDRI load failed\n");
    }

    T3DModel *model_albedo = t3d_model_load(MODEL_ALBEDO_PATH);
    T3DModel *model_packed = t3d_model_load(MODEL_PACKED_PATH);
    if (!model_albedo || !model_packed) {
        assertf(0,"Model load failed\n");
    }

    surface_t gbuffer_albedo;
    surface_t gbuffer_packed;
    surface_t gbuffer_depth = surface_alloc(FMT_RGBA16, w, h);
    if (!gbuffer_depth.buffer) {
        assertf(0,"GBuffer/depth alloc failed\n");
    }
    {
        gbuffer_albedo = surface_alloc(FMT_RGBA16, w, h);
        gbuffer_packed = surface_alloc(FMT_RGBA16, w, h);
        if (!gbuffer_albedo.buffer || !gbuffer_packed.buffer) {
            assertf(0,"GBuffer alloc failed\n");
        }
    }

    MatcapSet mats_db = {0};
    {
        if (!alloc_matcaps(&mats_db)) {
            assertf(0,"matcap alloc failed\n");
        }
    }

    surface_t decoded_lighting_surf;
    uint32_t *decoded_lighting;


    decoded_lighting_surf = surface_alloc(FMT_RGBA32, w, h * 3);
    decoded_lighting = (decoded_lighting_surf.buffer);

    if (!decoded_lighting) {
        assertf(0,"decode buffers alloc failed\n");
    }


    LightingState lights;
    setup_default_lighting(&lights);

    CameraState cam_history = {{0.0f, 0.0f, 1.0f}};

    T3DViewport viewport = t3d_viewport_create_buffered(FB_COUNT);

    T3DMat4 model_mat;
    t3d_mat4_identity(&model_mat);
    T3DMat4FP *model_mat_fp = malloc_uncached(sizeof(T3DMat4FP));
    t3d_mat4_to_fixed(model_mat_fp, &model_mat);

    T3DVec3 cam_pos = {{0.0f, 4.0f, 20.0f}};
    float cam_yaw = 0.0f;
    float cam_pitch = 0.0f;
    int frame = 0;
    uint64_t last_ticks = get_ticks_us();

    for (;;) {
        uint64_t t_frame0 = get_ticks_us();
        joypad_poll();
        joypad_inputs_t joypad = joypad_get_inputs(JOYPAD_PORT_1);

        if (joypad.stick_x < 10 && joypad.stick_x > -10) joypad.stick_x = 0;
        if (joypad.stick_y < 10 && joypad.stick_y > -10) joypad.stick_y = 0;

        uint64_t now_ticks = get_ticks_us();
        float dt = (float)(now_ticks - last_ticks) / 1000000.0f;
        last_ticks = now_ticks;

        float look_speed = .6f * dt;
        cam_yaw += (float)joypad.stick_x * look_speed * 0.01f;
        cam_pitch += (float)joypad.stick_y * look_speed * 0.01f;
        cam_pitch = clampf_local(cam_pitch, -1.4f, 1.4f);

        T3DVec3 cam_dir = {{
            fm_cosf(cam_yaw) * fm_cosf(cam_pitch),
            fm_sinf(cam_pitch),
            fm_sinf(cam_yaw) * fm_cosf(cam_pitch)
        }};
        t3d_vec3_norm(&cam_dir);

        float move_speed = 90.0f * dt;
        T3DVec3 cam_fwd_xz = {{cam_dir.v[0], 0.0f, cam_dir.v[2]}};
        t3d_vec3_norm(&cam_fwd_xz);
        T3DVec3 cam_right = {{cam_fwd_xz.v[2], 0.0f, -cam_fwd_xz.v[0]}};

        {
            if (joypad.btn.d_up) {
                cam_pos.v[0] += cam_dir.v[0] * move_speed;
                cam_pos.v[1] += cam_dir.v[1] * move_speed;
                cam_pos.v[2] += cam_dir.v[2] * move_speed;
            }
            if (joypad.btn.d_down) {
                cam_pos.v[0] -= cam_dir.v[0] * move_speed;
                cam_pos.v[1] -= cam_dir.v[1] * move_speed;
                cam_pos.v[2] -= cam_dir.v[2] * move_speed;
            }
        }
        if (joypad.btn.d_right) {
            cam_pos.v[0] -= cam_right.v[0] * move_speed;
            cam_pos.v[2] -= cam_right.v[2] * move_speed;
        }
        if (joypad.btn.d_left) {
            cam_pos.v[0] += cam_right.v[0] * move_speed;
            cam_pos.v[2] += cam_right.v[2] * move_speed;
        }

        T3DVec3 cam_target = {{
            cam_pos.v[0] + cam_dir.v[0],
            cam_pos.v[1] + cam_dir.v[1],
            cam_pos.v[2] + cam_dir.v[2]
        }};
        cam_history.forward[0] = cam_dir.x;
        cam_history.forward[1] = cam_dir.y;
        cam_history.forward[2] = cam_dir.z;

        surface_t* displaysurf = display_get();

        t3d_viewport_set_projection(&viewport, T3D_DEG_TO_RAD(85.0f), 20.0f, 500.0f);
        t3d_viewport_look_at(&viewport, &cam_pos, &cam_target, &(T3DVec3){{0,1,0}});

        // draw gbuffers
        rdpq_attach_clear(&gbuffer_albedo, &gbuffer_depth);
        t3d_frame_start();
        t3d_viewport_attach(&viewport);
        t3d_light_set_ambient((uint8_t[4]){0xFF, 0xFF, 0xFF, 0xFF});
        t3d_light_set_count(0);
        t3d_matrix_push(model_mat_fp);
        if(!albedoblock){
            rspq_block_begin();
            T3DModelIter it = t3d_model_iter_create(model_albedo, T3D_CHUNK_TYPE_OBJECT);
            while(t3d_model_iter_next(&it)) {
                // (Apply materials here before the draw)
                t3d_model_draw_material(it.object->material, NULL);
                t3d_state_set_vertex_fx(T3D_VERTEX_FX_NONE, 0,0);
                rdpq_set_prim_color(RGBA32(0,127,127,32));
                t3d_state_set_vertex_fx_scale(0b00000000'00000100);
                t3d_model_draw_object(it.object, NULL);
            }
            albedoblock = rspq_block_end();
        } rspq_block_run(albedoblock);

        t3d_matrix_pop(1);
        rdpq_detach();

        rdpq_attach(&gbuffer_packed, &gbuffer_depth);
        rdpq_clear(RGBA32(0, 0, 0, 0xFF));
        t3d_light_set_ambient((uint8_t[4]){0xFF, 0xFF, 0xFF, 0xFF});
        t3d_light_set_count(0);
        t3d_matrix_push(model_mat_fp);
        if(!packedblock){
            rspq_block_begin();
            T3DModelIter it = t3d_model_iter_create(model_packed, T3D_CHUNK_TYPE_OBJECT);
            while(t3d_model_iter_next(&it)) {
                t3d_model_draw_material(it.object->material, NULL);
                rdpq_mode_zmode(ZMODE_DECAL);
                rdpq_mode_zbuf(true, false);
                t3d_state_set_vertex_fx(T3D_VERTEX_FX_SPHERICAL_VERTEXCOLOR, 0,0);
                t3d_state_set_vertex_fx_scale(0b00000000'00010000);
                t3d_model_draw_object(it.object, NULL);
            }
            packedblock = rspq_block_end();
        } rspq_block_run(packedblock);
        t3d_matrix_pop(1);
        rdpq_detach();

        uint64_t t_mat0 = get_ticks_us();
        uint64_t t_dec0 = t_mat0;

        // generate matcaps for this frame
        generate_matcaps(&cam_history, &lights, &hdri, &mats_db);
        uint64_t t_mat1 = get_ticks_us();

        // wait for the gbuffers to be drawn (not parallelized at this stage)
        rspq_wait();
        t_dec0 = t_mat1;
        // matcaps -> lighting buffers
        cpu_decode_packed_to_interleaved_lighting(
            (const uint16_t *)gbuffer_packed.buffer,
            (uint32_t*)decoded_lighting,
            w,
            h,
            &mats_db);
        
        uint64_t t_dec1 = get_ticks_us();

        uint64_t t_rsp0 = get_ticks_us();

        rsp_pbr_blend_set_gbuffer(&gbuffer_albedo, &gbuffer_packed, displaysurf);
        rsp_pbr_blend_set_lighting_buffers(&decoded_lighting_surf);
        rsp_pbr_blend_set_dither_matrix();
        rsp_pbr_blend_postprocess();
        rdpq_attach(displaysurf, NULL);

        rdpq_detach_show();
        uint64_t t_rsp1 = get_ticks_us();

        uint64_t t_frame1 = get_ticks_us();
        uint64_t mat_us = (t_dec0 - t_mat0);
        uint64_t dec_us = (t_dec1 - t_dec0);
        uint64_t rsp_submit_us = (t_rsp1 - t_rsp0);
        uint64_t frame_us = t_frame1 - t_frame0;
        frame++;
        if ((frame % 30) == 0) {
            debugf("matcap_generate_us=%llu decode_us=%llu rsp_submit_us=%llu frame_us=%llu exposure=%.3f\n",
                   (unsigned long long)mat_us,
                   (unsigned long long)dec_us,
                   (unsigned long long)rsp_submit_us,
                   (unsigned long long)frame_us,
                   lights.exposure);
        }
    }

    {
        surface_free(&decoded_lighting_surf);
        free_matcaps(&mats_db);
        surface_free(&gbuffer_albedo);
        surface_free(&gbuffer_packed);
    }
    surface_free(&gbuffer_depth);
    free_uncached(model_mat_fp);

    t3d_destroy();
    return 0;
}
