#include <libdragon.h>

#include <stdint.h>
#include <string.h>

#include "pbr_decode.h"
#include "pbr_u88.h"

#define CACHE_OP_DIRTY ((0x3 << 2) | 0x1)
#define CACHE_OP_FLUSH ((0x5 << 2) | 0x1)

static inline uint16_t packed_to_matcap_byte_index(const uint16_t p)
{
    // Packed layout: MRRRRXXXXXYYYY0E
    // Transposed matcap: normal.x(5) -> Y, normal.y(4) -> X
    return p & 0b00000'11111'11110'0; // 00000YYYYYXXXX00
}

static inline uint32_t load_matcap_u32(const uint8_t *base, const uint16_t byte_index)
{
    return *(const uint32_t *)&base[byte_index];
}

// Instruction cache defines.
#define CACHE_INST_FLAG                 (0)
#define CACHE_INST_SIZE                 (16 * 1024)
#define CACHE_INST_LINESIZE             (32)
// Data cache defines.
#define CACHE_DATA_FLAG                 (1)
#define CACHE_DATA_SIZE                 (8 * 1024)
#define CACHE_DATA_LINESIZE             (16)
// Cache ops described in VR4300 manual on page 404.
#define INDEX_INVALIDATE                (0)
#define INDEX_LOAD_TAG                  (1)
#define INDEX_STORE_TAG                 (2)
#define INDEX_CREATE_DIRTY              (3)
#define HIT_INVALIDATE                  (4)
#define HIT_WRITEBACK_INVALIDATE        (5)
#define HIT_WRITEBACK                   (6)

#define CACHE_OP_DIRTY ((0x3 << 2) | 0x1)
#define CACHE_OP_FLUSH ((0x5 << 2) | 0x1)

/**
 * @brief Helper macro to perform cache refresh operations
 *
 * @param[in] op
 *            Operation to perform
 * @param[in] linesize
 *            Size of a cacheline in bytes
 */
#define cache_op(op, linesize) ({ \
    if (length) { \
        void *cur = (void*)((unsigned long)addr & ~(linesize-1)); \
        int count = (int)length + (addr-cur); \
        for (int i = 0; i < count; i += linesize) \
            asm ("\tcache %0,(%1)\n"::"i" (op), "r" (cur+i)); \
    } \
})

/**
 * @brief Helper macro to create the opcode for a cache operation.
 * Operation is encoded in 5 bits where bits 4..2 contain the operation type
 * and bits 0..1 determine the cache that is to be operated on. 
 * 
 * @param[in] op
 *            Operation type to perform.
 * @param[in] cache
 *            Specific cache to perform the operation on.
 */
#define build_opcode(op, cache)             (((op) << 2) | (cache))

void inline data_cache_prepare_for_write_local(volatile void *addr, unsigned long length)
{
    cache_op(build_opcode(INDEX_CREATE_DIRTY, CACHE_DATA_FLAG), CACHE_DATA_LINESIZE);
}

void cpu_decode_packed_to_interleaved_lighting(const uint16_t *packed16,
                                             uint32_t *out_lighting_interleaved,
                                             int w,
                                             int h,
                                             const MatcapSet *mats)
{
    const size_t n = (size_t)w * (size_t)h;
    assertf((n % DECODE_INTERLEAVED8_PIXELS) == 0,
            "decode_packed_to_interleaved_lighting: pixel count must be multiple of 8");

    // Matcaps are stored as 16x32 transposed:
    // They're transposed so that there's more horizontal resolution, than vertical
    // packed normal.x (5-bit) -> matcap Y, packed normal.y (4-bit) -> matcap X.
    // Packed layout here is: RRRRRXXXXXYYYY0M

    // try to precache the matcaps into CPU's cache:
    // 16x32 matcaps are 6KB which still fits into CPU's cache
    // doesn't seem to matter much here so commented out
    /*for(int i = 0; i < MATCAP_SIZE*MATCAP_SIZE*2*4; i+=8)
    {
        register uint64_t d = matcap_diffuse[i];
        register uint64_t s25 = matcap_spec25[i];
        register uint64_t s75 = matcap_spec75[i];
        (void)d;
        (void)s25;
        (void)s75;
    }*/

    const uint8_t *matcap_diffuse = (const uint8_t *)mats->diffuse;
    const uint8_t *matcap_spec25 = (const uint8_t *)mats->spec25;
    const uint8_t *matcap_spec75 = (const uint8_t *)mats->spec75;

    const uint16_t *p_ptr = packed16;
    register uint32_t *dst = out_lighting_interleaved;
    register size_t blocks = n / DECODE_INTERLEAVED8_PIXELS;

    // decode the packed buffer normals and matcaps into lighting buffers on CPU, 
    // so the RSP doesn't have to decode it itself (which it can't adequately) 
    // and just let it access linear buffers
    while (blocks--) {
        uint32_t offsets[DECODE_INTERLEAVED8_PIXELS];
        // AND with a mask
        for(int i = 0; i < DECODE_INTERLEAVED8_PIXELS; i++) {
            offsets[i] = packed_to_matcap_byte_index(p_ptr[i]);
        }

        // Create Dirty Exclusive for the lighting buffers to not read them to cache
        // TODO: doesn't work, cached address is faster here for some reason

        // access via the offsets
        for(int i = 0; i < DECODE_INTERLEAVED8_PIXELS; i++){
            dst[i] = load_matcap_u32(matcap_diffuse, offsets[i]);
        }
        dst+=8;

        for(int i = 0; i < DECODE_INTERLEAVED8_PIXELS; i++){
            dst[i] = load_matcap_u32(matcap_spec25, offsets[i]);
        }
        dst+=8;

        for(int i = 0; i < DECODE_INTERLEAVED8_PIXELS; i++){
            dst[i] = load_matcap_u32(matcap_spec75, offsets[i]);
        }
        dst+=8;

        p_ptr+=DECODE_INTERLEAVED8_PIXELS;
    }
}
