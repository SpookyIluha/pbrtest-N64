## Auto-generated file, transpiled with RSPL
#define RGBA32_CHANNELS 4
#define RGBA32_BYTES 4
#define RGBA16_BYTES 2
#define BUFFER_PIXELS 96
#define BUFFER_LIGHTING_PIXELS 288
#define BUFFER_SIZE_GBUFFER 192
#define BUFFER_SIZE_LIGHTING_THREE 1152
#define BUFFER_SIZE_GBUFFER_MINUSONE 191
#define BUFFER_SIZE_LIGHTING_THREE_MINUSONE 1151
#define GBUFFER_8PIXEL_OFFSET 16
#define LIGHTING_8PIXEL_OFFSET 32
#define LIGHTING_16PIXEL_OFFSET 64
#include <rsp_queue.inc>

.set noreorder
.set noat
.set nomacro

#undef zero
#undef at
#undef v0
#undef v1
#undef a0
#undef a1
#undef a2
#undef a3
#undef t0
#undef t1
#undef t2
#undef t3
#undef t4
#undef t5
#undef t6
#undef t7
#undef s0
#undef s1
#undef s2
#undef s3
#undef s4
#undef s5
#undef s6
#undef s7
#undef t8
#undef t9
#undef k0
#undef k1
#undef gp
#undef sp
#undef fp
#undef ra
.equ hex.$zero, 0
.equ hex.$at, 1
.equ hex.$v0, 2
.equ hex.$v1, 3
.equ hex.$a0, 4
.equ hex.$a1, 5
.equ hex.$a2, 6
.equ hex.$a3, 7
.equ hex.$t0, 8
.equ hex.$t1, 9
.equ hex.$t2, 10
.equ hex.$t3, 11
.equ hex.$t4, 12
.equ hex.$t5, 13
.equ hex.$t6, 14
.equ hex.$t7, 15
.equ hex.$s0, 16
.equ hex.$s1, 17
.equ hex.$s2, 18
.equ hex.$s3, 19
.equ hex.$s4, 20
.equ hex.$s5, 21
.equ hex.$s6, 22
.equ hex.$s7, 23
.equ hex.$t8, 24
.equ hex.$t9, 25
.equ hex.$k0, 26
.equ hex.$k1, 27
.equ hex.$gp, 28
.equ hex.$sp, 29
.equ hex.$fp, 30
.equ hex.$ra, 31
#define vco 0
#define vcc 1
#define vce 2

.data
  RSPQ_BeginOverlayHeader
    RSPQ_DefineCommand PBRBlendCmd_SetGBuffer, 16
    RSPQ_DefineCommand PBRBlendCmd_SetLightingBuffers, 4
    RSPQ_DefineCommand PBRBlendCmd_SetDitherMatrix, 8
    RSPQ_DefineCommand PBRBlendCmd_PostProcess, 4
  RSPQ_EndOverlayHeader

  RSPQ_BeginSavedState
    STATE_MEM_START:
    .align 2
    ALBEDO_SRC_RDRAM: .ds.b 4
    .align 2
    PACKED_SRC_RDRAM: .ds.b 4
    .align 2
    DEST_SRC_RDRAM: .ds.b 4
    .align 2
    SRC_PIXELS_ITERATIONS: .ds.b 4
    .align 2
    LIGHTING_SRC_RDRAM: .ds.b 4
    .align 4
    ALBEDO_BUF1: .ds.b 192
    .align 4
    PACKED_BUF1: .ds.b 192
    .align 4
    DEST_BUF1: .ds.b 192
    .align 4
    ALBEDO_BUF2: .ds.b 192
    .align 4
    PACKED_BUF2: .ds.b 192
    .align 4
    DEST_BUF2: .ds.b 192
    .align 4
    LIGHTING_BUF1: .ds.b 1152
    .align 4
    LIGHTING_BUF2: .ds.b 1152
    .align 4
    DITHER_MATRIX_8X2_1: .ds.b 16
    .align 4
    DITHER_MATRIX_8X2_2: .ds.b 16
    .align 1
    DITHER_LINE_IN_BUFFERS: .ds.b 2
    STATE_MEM_END:
  RSPQ_EndSavedState

.text
OVERLAY_CODE_START:

PBRBlendCmd_SetGBuffer:
  sw $a0, %lo(ALBEDO_SRC_RDRAM)($zero)               ## L:99   |      ^ | store(albedoptr, ALBEDO_SRC_RDRAM);
  sw $a1, %lo(PACKED_SRC_RDRAM)($zero)               ## L:100  |      2 | store(packedptr, PACKED_SRC_RDRAM);
  sw $a2, %lo(DEST_SRC_RDRAM)($zero)                 ## L:101  |      3 | store(destptr, DEST_SRC_RDRAM);
  j RSPQ_Loop                                        ## L:103  |      4 | }
  sw $a3, %lo(SRC_PIXELS_ITERATIONS)($zero)          ## L:102  |     *6 | store(iterations, SRC_PIXELS_ITERATIONS);
PBRBlendCmd_SetLightingBuffers:
  j RSPQ_Loop                                        ## L:109  |      ^ | }
  sw $a0, %lo(LIGHTING_SRC_RDRAM)($zero)             ## L:108  |     *3 | store(lightingptr, LIGHTING_SRC_RDRAM);
PBRBlendCmd_SetDitherMatrix:
  sw $a1, %lo(DITHER_LINE_IN_BUFFERS)($zero)         ## L:113  |      ^ | store(iterations_per_line, DITHER_LINE_IN_BUFFERS);
  ori $s6, $zero, %lo(DITHER_MATRIX_8X2_1)           ## L:115  |      2 | u32 matrixDMEM = DITHER_MATRIX_8X2_1;
  mtc0 $s6, COP0_DMA_SPADDR                          ## L:76   |      3 | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x1
  mtc0 $a0, COP0_DMA_RAMADDR                         ## L:77   |      4 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x1
  addiu $s7, $zero, 31                               ## L:114  |      5 | u32 size = 31;
  mtc0 $s7, COP0_DMA_READ                            ## L:78   |     *7 | @Barrier("DMA") set_dma_read(size); ## Barrier: 0x1
  LABEL_PBRBlendCmd_SetDitherMatrix_0001:
  mfc0 $ra, COP0_DMA_BUSY                            ## L:92   |      8 | RA = get_dma_busy();
  bne $ra, $zero, LABEL_PBRBlendCmd_SetDitherMatrix_0001 ## L:92   |   **11 | RA = get_dma_busy();
  nop                                                ## L:92   |    *13 | RA = get_dma_busy();
  LABEL_PBRBlendCmd_SetDitherMatrix_0002:
  j RSPQ_Loop                                        ## L:93   |     14 | 
  nop                                                ## L:93   |    *16 | 
PBRBlendCmd_PostProcess:
  addiu $k1, $zero, 1151                             ## L:136  |      ^ | const u32 lightingbufferSizeMinusOne = 1151;
  lw $s6, %lo(ALBEDO_SRC_RDRAM + 0)                  ## L:124  |      2 | u32 albedoRDRAM = load(ALBEDO_SRC_RDRAM);
  addiu $fp, $zero, 1                                ## L:132  |      3 | u32 firstLoop = 1;
  ori $s3, $zero, %lo(ALBEDO_BUF1)                   ## L:128  |      4 | u32 albedoDMEM = ALBEDO_BUF1;
  mtc0 $s3, COP0_DMA_SPADDR                          ## L:76   |      5 | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x1
  addiu $sp, $zero, 191                              ## L:135  |      6 | const u32 gbufferSizeMinusOne = 191;
  lw $s4, %lo(DEST_SRC_RDRAM + 0)                    ## L:126  |      7 | u32 destRDRAM = load(DEST_SRC_RDRAM);
  mtc0 $s6, COP0_DMA_RAMADDR                         ## L:77   |      8 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x1
  ori $s0, $zero, %lo(DEST_BUF2)                     ## L:131  |      9 | u32 destDMEM = DEST_BUF2;
  lw $s7, %lo(LIGHTING_SRC_RDRAM + 0)                ## L:122  |     10 | u32 lightingRDRAM = load(LIGHTING_SRC_RDRAM);
  mtc0 $sp, COP0_DMA_READ                            ## L:78   |     11 | @Barrier("DMA") set_dma_read(size); ## Barrier: 0x1
  lw $s5, %lo(PACKED_SRC_RDRAM + 0)                  ## L:125  |     12 | u32 packedRDRAM = load(PACKED_SRC_RDRAM);
  ori $s2, $zero, %lo(PACKED_BUF1)                   ## L:129  |     13 | u32 packedDMEM = PACKED_BUF1;
  mtc0 $s2, COP0_DMA_SPADDR                          ## L:76   |    *15 | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x1
  mtc0 $s5, COP0_DMA_RAMADDR                         ## L:77   |     16 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x1
  ori $s1, $zero, %lo(LIGHTING_BUF1)                 ## L:130  |     17 | u32 lightingDMEM = LIGHTING_BUF1;
  mtc0 $sp, COP0_DMA_READ                            ## L:78   |    *19 | @Barrier("DMA") set_dma_read(size); ## Barrier: 0x1
  LABEL_PBRBlendCmd_PostProcess_0003:
  mfc0 $ra, COP0_DMA_BUSY                            ## L:92   |     20 | RA = get_dma_busy();
  bne $ra, $zero, LABEL_PBRBlendCmd_PostProcess_0003 ## L:92   |   **23 | RA = get_dma_busy();
  nop                                                ## L:92   |    *25 | RA = get_dma_busy();
  LABEL_PBRBlendCmd_PostProcess_0004:
  vmov $v28.e4, $v30.e7                              ## L:148  |     26 | VCONSTS.X = 1;
  addiu $at, $zero, 31744                            ## L:144  |      ^ | VCONSTS.x = 0b01111100'00000000;
  vmov $v28.e5, $v31.e0                              ## L:149  |     27 | VCONSTS.Y = 0b10000000'00000000;
  mtc0 $s1, COP0_DMA_SPADDR                          ## L:76   |      ^ | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x1
  mtc2 $at, $v28.e0                                  ## L:144  |     28 | VCONSTS.x = 0b01111100'00000000;
  vmov $v28.e1, $v30.e7                              ## L:145  |     29 | VCONSTS.y = 0b00000000'00000001;
  addiu $at, $zero, 32512                            ## L:146  |      ^ | VCONSTS.z = 0b01111111'00000000;
  mtc2 $at, $v28.e2                                  ## L:146  |    *31 | VCONSTS.z = 0b01111111'00000000;
  mtc0 $s7, COP0_DMA_RAMADDR                         ## L:77   |     32 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x1
  addiu $at, $zero, 15360                            ## L:147  |   **35 | VCONSTS.w = 0b00111100'00000000;
  vor $v27, $v00, $v28.e2                            ## L:150  |      ^ | const vec16 VONE = VCONSTS.z;
  mtc0 $k1, COP0_DMA_READ                            ## L:78   |     36 | @Barrier("DMA") set_dma_read(size); ## Barrier: 0x1
  vxor $v26, $v00, $v30.e6                           ## L:151  |      ^ | const vec16 V_HDR_MULTIPLIER = 2;
  mtc2 $at, $v28.e3                                  ## L:147  |     37 | VCONSTS.w = 0b00111100'00000000;
  postprocess_loop:
  lw $k0, %lo(SRC_PIXELS_ITERATIONS + 0)             ## L:154  |     38 | u32 bufferscounter = load(SRC_PIXELS_ITERATIONS);
  beq $k0, $zero, LABEL_PBRBlendCmd_PostProcess_0013 ## L:155  |   **41 | if(bufferscounter == 0) goto postprocess_end;
  nop                                                ## L:155  |    *43 | if(bufferscounter == 0) goto postprocess_end;
  ori $s2, $zero, %lo(PACKED_BUF2)                   ## L:166  |     44 | packedDMEM = PACKED_BUF2;
  addiu $s7, $s7, 1152                               ## L:163  |     45 | lightingRDRAM += 1152;
  ori $s1, $zero, %lo(LIGHTING_BUF2)                 ## L:167  |     46 | lightingDMEM = LIGHTING_BUF2;
  addiu $s6, $s6, 192                                ## L:161  |     47 | albedoRDRAM += 192;
  ori $s3, $zero, %lo(ALBEDO_BUF2)                   ## L:165  |     48 | albedoDMEM = ALBEDO_BUF2;
  addiu $s5, $s5, 192                                ## L:162  |     49 | packedRDRAM += 192;
  ori $s0, $zero, %lo(DEST_BUF2)                     ## L:168  |     50 | destDMEM = DEST_BUF2;
  LABEL_PBRBlendCmd_PostProcess_0006:
  mfc0 $ra, COP0_DMA_BUSY                            ## L:92   |     51 | RA = get_dma_busy();
  bne $ra, $zero, LABEL_PBRBlendCmd_PostProcess_0006 ## L:92   |   **54 | RA = get_dma_busy();
  nop                                                ## L:92   |    *56 | RA = get_dma_busy();
  LABEL_PBRBlendCmd_PostProcess_0007:
  mtc0 $s3, COP0_DMA_SPADDR                          ## L:76   |     57 | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x1
  mtc0 $s6, COP0_DMA_RAMADDR                         ## L:77   |     58 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x1
  mtc0 $sp, COP0_DMA_READ                            ## L:78   |   **61 | @Barrier("DMA") set_dma_read(size); ## Barrier: 0x1
  mtc0 $s2, COP0_DMA_SPADDR                          ## L:76   |     62 | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x1
  mtc0 $s5, COP0_DMA_RAMADDR                         ## L:77   |   **65 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x1
  mtc0 $sp, COP0_DMA_READ                            ## L:78   |     66 | @Barrier("DMA") set_dma_read(size); ## Barrier: 0x1
  LABEL_PBRBlendCmd_PostProcess_0008:
  mfc0 $ra, COP0_DMA_BUSY                            ## L:92   |   **69 | RA = get_dma_busy();
  bne $ra, $zero, LABEL_PBRBlendCmd_PostProcess_0008 ## L:92   |   **72 | RA = get_dma_busy();
  nop                                                ## L:92   |    *74 | RA = get_dma_busy();
  LABEL_PBRBlendCmd_PostProcess_0009:
  mtc0 $s1, COP0_DMA_SPADDR                          ## L:76   |     75 | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x1
  mtc0 $s7, COP0_DMA_RAMADDR                         ## L:77   |     76 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x1
  bne $fp, $zero, LABEL_PBRBlendCmd_PostProcess_000A ## L:175  |     77 | if(firstLoop == 0){
  mtc0 $k1, COP0_DMA_READ                            ## L:78   |   **80 | @Barrier("DMA") set_dma_read(size); ## Barrier: 0x1
  mtc0 $s0, COP0_DMA_SPADDR                          ## L:84   |    *82 | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x1
  mtc0 $s4, COP0_DMA_RAMADDR                         ## L:85   |     83 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x1
  addiu $s4, $s4, 192                                ## L:177  |     84 | destRDRAM += 192;
  mtc0 $sp, COP0_DMA_WRITE                           ## L:86   |    *86 | @Barrier("DMA") set_dma_write(size); ## Barrier: 0x1
  LABEL_PBRBlendCmd_PostProcess_000A:
  ori $t4, $zero, %lo(LIGHTING_BUF1)                 ## L:195  |     87 | u32 fresnelPixels = LIGHTING_BUF1; fresnelPixels += 3;
  addiu $t4, $t4, 3                                  ## L:195  |     88 | u32 fresnelPixels = LIGHTING_BUF1; fresnelPixels += 3;
  ori $t6, $zero, %lo(LIGHTING_BUF1)                 ## L:193  |     89 | u32 rough75Pixels = LIGHTING_BUF1; rough75Pixels += 64;
  addiu $t6, $t6, 64                                 ## L:193  |     90 | u32 rough75Pixels = LIGHTING_BUF1; rough75Pixels += 64;
  lfv $v18, 0, 0, $t6                                ## L:252  |     91 | asm_op("lfv", rough75_r, 0,  0, rough75Pixels);
  lfv $v11, 0, 0, $t4                                ## L:237  |     92 | asm_op("lfv", fresnel, 0,  0, fresnelPixels);
  ori $t5, $zero, %lo(DEST_BUF1)                     ## L:194  |     93 | u32 destPixels = DEST_BUF1;
  ori $t9, $zero, %lo(PACKED_BUF1)                   ## L:190  |     94 | u32 packedPixels = PACKED_BUF1;
  ori $t8, $zero, %lo(LIGHTING_BUF1)                 ## L:191  |     95 | u32 diffusePixels = LIGHTING_BUF1;
  ori $t7, $zero, %lo(LIGHTING_BUF1)                 ## L:192  |     96 | u32 rough25Pixels = LIGHTING_BUF1; rough25Pixels += 32;
  lfv $v24, 0, 0, $t8                                ## L:244  |     97 | asm_op("lfv", diffuse_r, 0,  0, diffusePixels);
  lfv $v11, 8, 16, $t4                               ## L:239  |     98 | asm_op("lfv", fresnel, 8, 16, fresnelPixels);
  addiu $t7, $t7, 32                                 ## L:192  |     99 | u32 rough25Pixels = LIGHTING_BUF1; rough25Pixels += 32;
  lfv $v24, 8, 16, $t8                               ## L:245  |    100 | asm_op("lfv", diffuse_r, 8, 16, diffusePixels);
  lfv $v21, 0, 0, $t7                                ## L:248  |    101 | asm_op("lfv", rough25_r, 0,  0, rough25Pixels);
  addiu $t8, $t8, 1                                  ## L:246  |    102 | diffusePixels += 1;
  lfv $v18, 8, 16, $t6                               ## L:253  |    103 | asm_op("lfv", rough75_r, 8, 16, rough75Pixels);
  lfv $v23, 0, 0, $t8                                ## L:258  |    104 | asm_op("lfv", diffuse_g, 0,  0, diffusePixels);
  addiu $t6, $t6, 1                                  ## L:254  |    105 | rough75Pixels += 1;
  lfv $v23, 8, 16, $t8                               ## L:259  |    106 | asm_op("lfv", diffuse_g, 8, 16, diffusePixels);
  ori $at, $zero, %lo(DITHER_MATRIX_8X2_1)           ## L:187  |    107 | vec16 dither = load(DITHER_MATRIX_8X2_1);
  ori $k0, $zero, %lo(ALBEDO_BUF1)                   ## L:189  |    108 | u32 albedoPixels = ALBEDO_BUF1;
  lfv $v17, 0, 0, $t6                                ## L:266  |    109 | asm_op("lfv", rough75_g, 0,  0, rough75Pixels);
  lfv $v17, 8, 16, $t6                               ## L:267  |    110 | asm_op("lfv", rough75_g, 8, 16, rough75Pixels);
  addiu $t8, $t8, 1                                  ## L:260  |    111 | diffusePixels += 1;
  lfv $v21, 8, 16, $t7                               ## L:249  |    112 | asm_op("lfv", rough25_r, 8, 16, rough25Pixels);
  addiu $t6, $t6, 1                                  ## L:268  |    113 | rough75Pixels += 1;
  lfv $v22, 0, 0, $t8                                ## L:270  |    114 | asm_op("lfv", diffuse_b, 0,  0, diffusePixels);
  addiu $t4, $t4, 96                                 ## L:240  |    115 | fresnelPixels += 96;
  addiu $t7, $t7, 1                                  ## L:250  |    116 | rough25Pixels += 1;
  lfv $v20, 0, 0, $t7                                ## L:262  |    117 | asm_op("lfv", rough25_g, 0,  0, rough25Pixels);
  lfv $v22, 8, 16, $t8                               ## L:271  |    118 | asm_op("lfv", diffuse_b, 8, 16, diffusePixels);
  or $fp, $zero, $zero                               ## L:179  |    119 | firstLoop = 0;
  ori $s3, $zero, %lo(ALBEDO_BUF1)                   ## L:181  |    120 | albedoDMEM = ALBEDO_BUF1;
  lfv $v20, 8, 16, $t7                               ## L:263  |    121 | asm_op("lfv", rough25_g, 8, 16, rough25Pixels);
  addiu $t8, $t8, 94                                 ## L:272  |    122 | diffusePixels += 94;
  addiu $t7, $t7, 1                                  ## L:264  |    123 | rough25Pixels += 1;
  lfv $v19, 0, 0, $t7                                ## L:274  |    124 | asm_op("lfv", rough25_b, 0,  0, rough25Pixels);
  lfv $v16, 0, 0, $t6                                ## L:278  |    125 | asm_op("lfv", rough75_b, 0,  0, rough75Pixels);
  lfv $v16, 8, 16, $t6                               ## L:279  |    126 | asm_op("lfv", rough75_b, 8, 16, rough75Pixels);
  lfv $v19, 8, 16, $t7                               ## L:275  |    127 | asm_op("lfv", rough25_b, 8, 16, rough25Pixels);
  ori $s0, $zero, %lo(DEST_BUF1)                     ## L:184  |    128 | destDMEM = DEST_BUF1;
  lqv $v25, 0, 0, $at                                ## L:187  |    129 | vec16 dither = load(DITHER_MATRIX_8X2_1);
  addiu $t3, $zero, 96                               ## L:198  |    130 | u16 pixelcounter = 96;
  addiu $t7, $t7, 94                                 ## L:276  |    131 | rough25Pixels += 94;
  ori $s2, $zero, %lo(PACKED_BUF1)                   ## L:182  |    132 | packedDMEM = PACKED_BUF1;
  addiu $t6, $t6, 94                                 ## L:280  |    133 | rough75Pixels += 94;
  ori $s1, $zero, %lo(LIGHTING_BUF1)                 ## L:183  |    134 | lightingDMEM = LIGHTING_BUF1;
  LABEL_PBRBlendCmd_PostProcess_000B:
  blez $t3, LABEL_PBRBlendCmd_PostProcess_000C       ## L:282  |    135 | while(pixelcounter > 0){
  nop                                                ## L:282  |   *137 | while(pixelcounter > 0){
  lqv $v09, 0, 0, $t9                                ## L:285  |    138 | vec16 packed = load(packedPixels);
  lqv $v10, 0, 0, $k0                                ## L:284  |    139 | vec16 albedo = load(albedoPixels);
  addiu $t3, $t3, 65528                              ## L:461  |  **142 | pixelcounter -= 8;
  vand $v12, $v09, $v28.e3                           ## L:295  |      ^ | roughness = packed & VCONSTS.w;
  addiu $k0, $k0, 16                                 ## L:288  |    143 | albedoPixels += 16;
  vmudn $v14, $v10, $v30.e3                          ## L:306  |      ^ | albedo_g = albedo << 4;
  vmudm $v15, $v10, $v31.e0                          ## L:305  |    144 | albedo_r = albedo >> 1;
  vmudn $v13, $v10, $v31.e6                          ## L:307  |    145 | albedo_b = albedo << 9;
  vand $v10, $v09, $v28.e5                           ## L:313  |    146 | vec16 metalness = packed & VCONSTS.Y;
  addiu $t9, $t9, 16                                 ## L:289  |      ^ | packedPixels += 16;
  veq $v09, $v10, $v00.e0                            ## L:315  | ***150 | vec16 metalnessselect = metalness == 0;
  vsub $v10, $v27, $v12.v                            ## L:321  |    151 | vec16 oneminusroughness:sfract = VONE:sfract - roughness:sfract;
  vmulf $v09, $v21, $v12.v                           ## L:372  |    152 | specular_r:sfract = rough25_r:sfract * roughness:sfract;
  lfv $v21, 0, 0, $t7                                ## L:419  |      ^ | asm_op("lfv", rough25_r, 0,  0, rough25Pixels);
  vmacf $v09, $v18, $v10.v                           ## L:373  |  **155 | specular_r:sfract = rough75_r:sfract +* oneminusroughness:sfract;
  vmulf $v08, $v20, $v12.v                           ## L:374  |    156 | specular_g:sfract = rough25_g:sfract * roughness:sfract;
  lfv $v18, 0, 0, $t6                                ## L:428  |      ^ | asm_op("lfv", rough75_r, 0,  0, rough75Pixels);
  lfv $v18, 8, 16, $t6                               ## L:429  |    157 | asm_op("lfv", rough75_r, 8, 16, rough75Pixels);
  vmacf $v08, $v17, $v10.v                           ## L:375  |      ^ | specular_g:sfract = rough75_g:sfract +* oneminusroughness:sfract;
  vmulf $v07, $v19, $v12.v                           ## L:376  |    158 | specular_b:sfract = rough25_b:sfract * roughness:sfract;
  vmacf $v07, $v16, $v10.v                           ## L:377  |    159 | specular_b:sfract = rough75_b:sfract +* oneminusroughness:sfract;
  vmrg $v06, $v24, $v09                              ## L:383  |    160 | albedomultiplier_r = select(diffuse_r, specular_r);
  lfv $v21, 8, 16, $t7                               ## L:423  |      ^ | asm_op("lfv", rough25_r, 8, 16, rough25Pixels);
  vand $v15, $v15, $v28.e0                           ## L:378  |    161 | albedo_r &= VCONSTS.x;
  addiu $t7, $t7, 1                                  ## L:424  |      ^ | rough25Pixels += 1;
  vmrg $v05, $v23, $v08                              ## L:384  |    162 | albedomultiplier_g = select(diffuse_g, specular_g);
  lfv $v20, 0, 0, $t7                                ## L:437  |      ^ | asm_op("lfv", rough25_g, 0,  0, rough25Pixels);
  vand $v14, $v14, $v28.e0                           ## L:385  |    163 | albedo_g &= VCONSTS.x;
  lfv $v24, 0, 0, $t8                                ## L:410  |      ^ | asm_op("lfv", diffuse_r, 0,  0, diffusePixels);
  vmrg $v04, $v22, $v07                              ## L:386  |    164 | albedomultiplier_b = select(diffuse_b, specular_b);
  lfv $v20, 8, 16, $t7                               ## L:439  |      ^ | asm_op("lfv", rough25_g, 8, 16, rough25Pixels);
  lfv $v24, 8, 16, $t8                               ## L:414  |    165 | asm_op("lfv", diffuse_r, 8, 16, diffusePixels);
  vmulf $v03, $v09, $v11.v                           ## L:388  |      ^ | result_r:sfract = specular_r:sfract * fresnel:sfract;
  addiu $t8, $t8, 1                                  ## L:415  |    166 | diffusePixels += 1;
  vmacf $v03, $v15, $v06.v                           ## L:389  |      ^ | result_r:sfract = albedo_r:sfract +* albedomultiplier_r:sfract;
  vand $v13, $v13, $v28.e0                           ## L:391  |    167 | albedo_b &= VCONSTS.x;
  addiu $t6, $t6, 1                                  ## L:430  |      ^ | rough75Pixels += 1;
  vmudh $v03, $v26, $v03.v                           ## L:390  |  **170 | result_r:sfract = V_HDR_MULTIPLIER:sint * result_r:sfract;
  lfv $v17, 0, 0, $t6                                ## L:441  |      ^ | asm_op("lfv", rough75_g, 0,  0, rough75Pixels);
  addiu $t7, $t7, 1                                  ## L:440  |    171 | rough25Pixels += 1;
  vmulf $v02, $v08, $v11.v                           ## L:392  |      ^ | result_g:sfract = specular_g:sfract * fresnel:sfract;
  vmacf $v02, $v14, $v05.v                           ## L:393  |    172 | result_g:sfract = albedo_g:sfract +* albedomultiplier_g:sfract;
  lfv $v23, 0, 0, $t8                                ## L:434  |      ^ | asm_op("lfv", diffuse_g, 0,  0, diffusePixels);
  vsub $v03, $v27, $v03.v                            ## L:395  |   *174 | result_r:sfract = VONE:sfract - result_r:sfract;
  lfv $v19, 0, 0, $t7                                ## L:449  |      ^ | asm_op("lfv", rough25_b, 0,  0, rough25Pixels);
  lfv $v19, 8, 16, $t7                               ## L:450  |   *176 | asm_op("lfv", rough25_b, 8, 16, rough25Pixels);
  vmudh $v02, $v26, $v02.v                           ## L:394  |      ^ | result_g:sfract = V_HDR_MULTIPLIER:sint * result_g:sfract;
  vmulf $v01, $v07, $v11.v                           ## L:396  |    177 | result_b:sfract = specular_b:sfract * fresnel:sfract;
  lfv $v23, 8, 16, $t8                               ## L:435  |      ^ | asm_op("lfv", diffuse_g, 8, 16, diffusePixels);
  addiu $t8, $t8, 1                                  ## L:436  |    178 | diffusePixels += 1;
  vmacf $v01, $v13, $v04.v                           ## L:397  |      ^ | result_b:sfract = albedo_b:sfract +* albedomultiplier_b:sfract;
  vsub $v02, $v27, $v02.v                            ## L:399  |   *180 | result_g:sfract = VONE:sfract - result_g:sfract;
  lfv $v17, 8, 16, $t6                               ## L:442  |      ^ | asm_op("lfv", rough75_g, 8, 16, rough75Pixels);
  vmulf $v03, $v03, $v03.v                           ## L:400  |    181 | result_r:sfract = result_r:sfract * result_r:sfract;
  addiu $t6, $t6, 1                                  ## L:443  |      ^ | rough75Pixels += 1;
  lfv $v22, 0, 0, $t8                                ## L:445  |    182 | asm_op("lfv", diffuse_b, 0,  0, diffusePixels);
  vmudh $v01, $v26, $v01.v                           ## L:398  |      ^ | result_b:sfract = V_HDR_MULTIPLIER:sint * result_b:sfract;
  vmulf $v02, $v02, $v02.v                           ## L:403  |   *184 | result_g:sfract = result_g:sfract * result_g:sfract;
  lfv $v16, 0, 0, $t6                                ## L:453  |      ^ | asm_op("lfv", rough75_b, 0,  0, rough75Pixels);
  addiu $t7, $t7, 94                                 ## L:451  |    185 | rough25Pixels += 94;
  vmulf $v03, $v03, $v03.v                           ## L:404  |      ^ | result_r:sfract = result_r:sfract * result_r:sfract;
  vsub $v01, $v27, $v01.v                            ## L:402  |    186 | result_b:sfract = VONE:sfract - result_b:sfract;
  lfv $v22, 8, 16, $t8                               ## L:446  |      ^ | asm_op("lfv", diffuse_b, 8, 16, diffusePixels);
  lfv $v11, 0, 0, $t4                                ## L:401  |   *188 | asm_op("lfv", fresnel, 0,  0, fresnelPixels);
  vmulf $v02, $v02, $v02.v                           ## L:408  |      ^ | result_g:sfract = result_g:sfract * result_g:sfract;
  lfv $v16, 8, 16, $t6                               ## L:454  |   *190 | asm_op("lfv", rough75_b, 8, 16, rough75Pixels);
  vmulf $v01, $v01, $v01.v                           ## L:407  |      ^ | result_b:sfract = result_b:sfract * result_b:sfract;
  vsub $v03, $v27, $v03.v                            ## L:409  |    191 | result_r:sfract = VONE:sfract - result_r:sfract;
  addiu $t6, $t6, 94                                 ## L:455  |      ^ | rough75Pixels += 94;
  vmulf $v01, $v01, $v01.v                           ## L:411  |  **194 | result_b:sfract = result_b:sfract * result_b:sfract;
  lfv $v11, 8, 16, $t4                               ## L:405  |      ^ | asm_op("lfv", fresnel, 8, 16, fresnelPixels);
  vsub $v02, $v27, $v02.v                            ## L:412  |    195 | result_g:sfract = VONE:sfract - result_g:sfract;
  vadd $v03, $v03, $v25.v                            ## L:413  |    196 | result_r:sfract = result_r:sfract + dither:sfract;
  vsub $v01, $v27, $v01.v                            ## L:416  |   *198 | result_b:sfract = VONE:sfract - result_b:sfract;
  vadd $v02, $v02, $v25.v                            ## L:417  |    199 | result_g:sfract = result_g:sfract + dither:sfract;
  vand $v03, $v03, $v28.e0                           ## L:418  |    200 | result_r &= VCONSTS.x;
  vadd $v01, $v01, $v25.v                            ## L:420  |   *202 | result_b:sfract = result_b:sfract + dither:sfract;
  vand $v02, $v02, $v28.e0                           ## L:421  |    203 | result_g &= VCONSTS.x;
  vmudn $v03, $v03, $v30.e6                          ## L:422  |    204 | result_r <<= 1;
  vand $v01, $v01, $v28.e0                           ## L:425  |   *206 | result_b &= VCONSTS.x;
  vmudm $v02, $v02, $v31.e3                          ## L:427  |    207 | result_g >>= 4;
  addiu $t4, $t4, 96                                 ## L:406  |      ^ | fresnelPixels += 96;
  vor $v03, $v03, $v28.e1                            ## L:426  |    208 | result_r |= VCONSTS.y;
  vmudm $v01, $v01, $v30.e0                          ## L:431  |   *210 | result_b >>= 9;
  vor $v10, $v03, $v02.v                             ## L:433  |   *212 | result16bpp = result_r | result_g;
  addiu $t8, $t8, 94                                 ## L:447  |      ^ | diffusePixels += 94;
  vor $v10, $v10, $v01.v                             ## L:438  | ***216 | result16bpp |= result_b;
  sqv $v10, 0, 0, $t5                                ## L:457  | ***220 | store(result16bpp, destPixels);
  j LABEL_PBRBlendCmd_PostProcess_000B               ## L:461  |    221 | pixelcounter -= 8;
  addiu $t5, $t5, 16                                 ## L:460  |   *223 | destPixels += 16;
  LABEL_PBRBlendCmd_PostProcess_000C:
  addiu $s5, $s5, 192                                ## L:466  |    224 | packedRDRAM += 192;
  addiu $s6, $s6, 192                                ## L:465  |    225 | albedoRDRAM += 192;
  addiu $s7, $s7, 1152                               ## L:467  |    226 | lightingRDRAM += 1152;
  LABEL_PBRBlendCmd_PostProcess_000D:
  mfc0 $ra, COP0_DMA_BUSY                            ## L:92   |    227 | RA = get_dma_busy();
  bne $ra, $zero, LABEL_PBRBlendCmd_PostProcess_000D ## L:92   |  **230 | RA = get_dma_busy();
  nop                                                ## L:92   |   *232 | RA = get_dma_busy();
  LABEL_PBRBlendCmd_PostProcess_000E:
  mtc0 $s3, COP0_DMA_SPADDR                          ## L:76   |    233 | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x1
  mtc0 $s6, COP0_DMA_RAMADDR                         ## L:77   |    234 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x1
  mtc0 $sp, COP0_DMA_READ                            ## L:78   |  **237 | @Barrier("DMA") set_dma_read(size); ## Barrier: 0x1
  mtc0 $s2, COP0_DMA_SPADDR                          ## L:76   |    238 | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x1
  mtc0 $s5, COP0_DMA_RAMADDR                         ## L:77   |  **241 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x1
  mtc0 $sp, COP0_DMA_READ                            ## L:78   |    242 | @Barrier("DMA") set_dma_read(size); ## Barrier: 0x1
  LABEL_PBRBlendCmd_PostProcess_000F:
  mfc0 $ra, COP0_DMA_BUSY                            ## L:92   |  **245 | RA = get_dma_busy();
  bne $ra, $zero, LABEL_PBRBlendCmd_PostProcess_000F ## L:92   |  **248 | RA = get_dma_busy();
  nop                                                ## L:92   |   *250 | RA = get_dma_busy();
  LABEL_PBRBlendCmd_PostProcess_0010:
  ori $t7, $zero, %lo(LIGHTING_BUF2)                 ## L:483  |    251 | rough25Pixels = LIGHTING_BUF2; rough25Pixels += 32;
  ori $t4, $zero, %lo(LIGHTING_BUF2)                 ## L:486  |    252 | fresnelPixels = LIGHTING_BUF2; fresnelPixels += 3;
  ori $t6, $zero, %lo(LIGHTING_BUF2)                 ## L:484  |    253 | rough75Pixels = LIGHTING_BUF2; rough75Pixels += 64;
  ori $at, $zero, %lo(DITHER_MATRIX_8X2_2)           ## L:478  |    254 | dither = load(DITHER_MATRIX_8X2_2);
  mtc0 $s1, COP0_DMA_SPADDR                          ## L:76   |    255 | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x1
  mtc0 $s7, COP0_DMA_RAMADDR                         ## L:77   |    256 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x1
  addiu $t6, $t6, 64                                 ## L:484  |    257 | rough75Pixels = LIGHTING_BUF2; rough75Pixels += 64;
  addiu $t7, $t7, 32                                 ## L:483  |    258 | rough25Pixels = LIGHTING_BUF2; rough25Pixels += 32;
  lfv $v21, 0, 0, $t7                                ## L:544  |    259 | asm_op("lfv", rough25_r, 0,  0, rough25Pixels);
  mtc0 $k1, COP0_DMA_READ                            ## L:78   |    260 | @Barrier("DMA") set_dma_read(size); ## Barrier: 0x1
  lfv $v18, 0, 0, $t6                                ## L:550  |    261 | asm_op("lfv", rough75_r, 0,  0, rough75Pixels);
  addiu $t3, $zero, 96                               ## L:489  |    262 | pixelcounter = 96;
  lfv $v18, 8, 16, $t6                               ## L:552  |    263 | asm_op("lfv", rough75_r, 8, 16, rough75Pixels);
  mtc0 $s0, COP0_DMA_SPADDR                          ## L:84   |    264 | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x1
  addiu $t6, $t6, 1                                  ## L:553  |    265 | rough75Pixels += 1;
  ori $t5, $zero, %lo(DEST_BUF2)                     ## L:485  |    266 | destPixels = DEST_BUF2;
  lfv $v21, 8, 16, $t7                               ## L:546  |    267 | asm_op("lfv", rough25_r, 8, 16, rough25Pixels);
  mtc0 $s4, COP0_DMA_RAMADDR                         ## L:85   |    268 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x1
  addiu $t7, $t7, 1                                  ## L:547  |    269 | rough25Pixels += 1;
  ori $t8, $zero, %lo(LIGHTING_BUF2)                 ## L:482  |    270 | diffusePixels = LIGHTING_BUF2;
  lfv $v17, 0, 0, $t6                                ## L:570  |    271 | asm_op("lfv", rough75_g, 0,  0, rough75Pixels);
  addiu $t4, $t4, 3                                  ## L:486  |    272 | fresnelPixels = LIGHTING_BUF2; fresnelPixels += 3;
  lfv $v24, 0, 0, $t8                                ## L:538  |    273 | asm_op("lfv", diffuse_r, 0,  0, diffusePixels);
  lfv $v20, 0, 0, $t7                                ## L:564  |    274 | asm_op("lfv", rough25_g, 0,  0, rough25Pixels);
  lqv $v25, 0, 0, $at                                ## L:478  |    275 | dither = load(DITHER_MATRIX_8X2_2);
  ori $k0, $zero, %lo(ALBEDO_BUF2)                   ## L:480  |    276 | albedoPixels = ALBEDO_BUF2;
  addiu $s4, $s4, 192                                ## L:476  |    277 | destRDRAM += 192;
  ori $t9, $zero, %lo(PACKED_BUF2)                   ## L:481  |    278 | packedPixels = PACKED_BUF2;
  lfv $v24, 8, 16, $t8                               ## L:540  |    279 | asm_op("lfv", diffuse_r, 8, 16, diffusePixels);
  mtc0 $sp, COP0_DMA_WRITE                           ## L:86   |    280 | @Barrier("DMA") set_dma_write(size); ## Barrier: 0x1
  lfv $v20, 8, 16, $t7                               ## L:566  |    281 | asm_op("lfv", rough25_g, 8, 16, rough25Pixels);
  addiu $t8, $t8, 1                                  ## L:541  |    282 | diffusePixels += 1;
  addiu $t7, $t7, 1                                  ## L:567  |    283 | rough25Pixels += 1;
  lfv $v23, 0, 0, $t8                                ## L:558  |    284 | asm_op("lfv", diffuse_g, 0,  0, diffusePixels);
  lfv $v17, 8, 16, $t6                               ## L:572  |    285 | asm_op("lfv", rough75_g, 8, 16, rough75Pixels);
  lfv $v19, 0, 0, $t7                                ## L:582  |    286 | asm_op("lfv", rough25_b, 0,  0, rough25Pixels);
  lfv $v23, 8, 16, $t8                               ## L:560  |    287 | asm_op("lfv", diffuse_g, 8, 16, diffusePixels);
  lfv $v19, 8, 16, $t7                               ## L:584  |    288 | asm_op("lfv", rough25_b, 8, 16, rough25Pixels);
  addiu $t8, $t8, 1                                  ## L:561  |    289 | diffusePixels += 1;
  lfv $v22, 0, 0, $t8                                ## L:576  |    290 | asm_op("lfv", diffuse_b, 0,  0, diffusePixels);
  addiu $t6, $t6, 1                                  ## L:573  |    291 | rough75Pixels += 1;
  lfv $v16, 0, 0, $t6                                ## L:588  |    292 | asm_op("lfv", rough75_b, 0,  0, rough75Pixels);
  lfv $v11, 0, 0, $t4                                ## L:530  |    293 | asm_op("lfv", fresnel, 0,  0, fresnelPixels);
  lfv $v16, 8, 16, $t6                               ## L:590  |    294 | asm_op("lfv", rough75_b, 8, 16, rough75Pixels);
  lfv $v11, 8, 16, $t4                               ## L:532  |    295 | asm_op("lfv", fresnel, 8, 16, fresnelPixels);
  lfv $v22, 8, 16, $t8                               ## L:578  |    296 | asm_op("lfv", diffuse_b, 8, 16, diffusePixels);
  addiu $t8, $t8, 94                                 ## L:579  |    297 | diffusePixels += 94;
  addiu $t4, $t4, 96                                 ## L:533  |    298 | fresnelPixels += 96;
  addiu $t7, $t7, 94                                 ## L:585  |    299 | rough25Pixels += 94;
  addiu $t6, $t6, 94                                 ## L:591  |    300 | rough75Pixels += 94;
  LABEL_PBRBlendCmd_PostProcess_0011:
  blez $t3, LABEL_PBRBlendCmd_PostProcess_0012       ## L:593  |    301 | while(pixelcounter > 0){
  nop                                                ## L:593  |   *303 | while(pixelcounter > 0){
  lqv $v09, 0, 0, $t9                                ## L:596  |    304 | vec16 packed = load(packedPixels);
  lqv $v10, 0, 0, $k0                                ## L:595  |    305 | vec16 albedo = load(albedoPixels);
  addiu $k0, $k0, 16                                 ## L:599  |  **308 | albedoPixels += 16;
  vand $v12, $v09, $v28.e3                           ## L:606  |      ^ | roughness = packed & VCONSTS.w;
  addiu $t9, $t9, 16                                 ## L:600  |    309 | packedPixels += 16;
  vmudm $v15, $v10, $v31.e0                          ## L:616  |      ^ | albedo_r = albedo >> 1;
  vmudn $v14, $v10, $v30.e3                          ## L:617  |    310 | albedo_g = albedo << 4;
  vmudn $v13, $v10, $v31.e6                          ## L:618  |    311 | albedo_b = albedo << 9;
  vand $v10, $v09, $v28.e5                           ## L:624  |    312 | vec16 metalness = packed & VCONSTS.Y;
  veq $v09, $v10, $v00.e0                            ## L:626  | ***316 | vec16 metalnessselect = metalness == 0;
  vsub $v10, $v27, $v12.v                            ## L:632  |    317 | vec16 oneminusroughness:sfract = VONE:sfract - roughness:sfract;
  vmulf $v09, $v21, $v12.v                           ## L:686  |    318 | specular_r:sfract = rough25_r:sfract * roughness:sfract;
  vmacf $v09, $v18, $v10.v                           ## L:687  |  **321 | specular_r:sfract = rough75_r:sfract +* oneminusroughness:sfract;
  lfv $v21, 8, 16, $t7                               ## L:743  |      ^ | asm_op("lfv", rough25_r, 8, 16, rough25Pixels);
  lfv $v21, 0, 0, $t7                                ## L:738  |    322 | asm_op("lfv", rough25_r, 0,  0, rough25Pixels);
  vmulf $v08, $v20, $v12.v                           ## L:688  |      ^ | specular_g:sfract = rough25_g:sfract * roughness:sfract;
  vmacf $v08, $v17, $v10.v                           ## L:689  |    323 | specular_g:sfract = rough75_g:sfract +* oneminusroughness:sfract;
  lfv $v18, 0, 0, $t6                                ## L:749  |      ^ | asm_op("lfv", rough75_r, 0,  0, rough75Pixels);
  vmulf $v07, $v19, $v12.v                           ## L:690  |    324 | specular_b:sfract = rough25_b:sfract * roughness:sfract;
  lfv $v18, 8, 16, $t6                               ## L:751  |      ^ | asm_op("lfv", rough75_r, 8, 16, rough75Pixels);
  addiu $t6, $t6, 1                                  ## L:752  |    325 | rough75Pixels += 1;
  vmacf $v07, $v16, $v10.v                           ## L:691  |      ^ | specular_b:sfract = rough75_b:sfract +* oneminusroughness:sfract;
  vmrg $v06, $v24, $v09                              ## L:697  |    326 | albedomultiplier_r = select(diffuse_r, specular_r);
  lfv $v24, 0, 0, $t8                                ## L:727  |      ^ | asm_op("lfv", diffuse_r, 0,  0, diffusePixels);
  vmrg $v05, $v23, $v08                              ## L:698  |    327 | albedomultiplier_g = select(diffuse_g, specular_g);
  addiu $t7, $t7, 1                                  ## L:744  |      ^ | rough25Pixels += 1;
  lfv $v20, 8, 16, $t7                               ## L:765  |    328 | asm_op("lfv", rough25_g, 8, 16, rough25Pixels);
  vand $v15, $v15, $v28.e0                           ## L:692  |      ^ | albedo_r &= VCONSTS.x;
  vmrg $v04, $v22, $v07                              ## L:700  |    329 | albedomultiplier_b = select(diffuse_b, specular_b);
  vand $v14, $v14, $v28.e0                           ## L:699  |    330 | albedo_g &= VCONSTS.x;
  vmulf $v03, $v09, $v11.v                           ## L:702  |    331 | result_r:sfract = specular_r:sfract * fresnel:sfract;
  vmacf $v03, $v15, $v06.v                           ## L:703  |    332 | result_r:sfract = albedo_r:sfract +* albedomultiplier_r:sfract;
  lfv $v20, 0, 0, $t7                                ## L:762  |      ^ | asm_op("lfv", rough25_g, 0,  0, rough25Pixels);
  vand $v13, $v13, $v28.e0                           ## L:705  |    333 | albedo_b &= VCONSTS.x;
  lfv $v24, 8, 16, $t8                               ## L:732  |      ^ | asm_op("lfv", diffuse_r, 8, 16, diffusePixels);
  vmudh $v03, $v26, $v03.v                           ## L:704  |  **336 | result_r:sfract = V_HDR_MULTIPLIER:sint * result_r:sfract;
  addiu $t8, $t8, 1                                  ## L:733  |      ^ | diffusePixels += 1;
  lfv $v23, 8, 16, $t8                               ## L:759  |    337 | asm_op("lfv", diffuse_g, 8, 16, diffusePixels);
  vmulf $v02, $v08, $v11.v                           ## L:706  |      ^ | result_g:sfract = specular_g:sfract * fresnel:sfract;
  vmacf $v02, $v14, $v05.v                           ## L:707  |    338 | result_g:sfract = albedo_g:sfract +* albedomultiplier_g:sfract;
  addiu $t7, $t7, 1                                  ## L:766  |      ^ | rough25Pixels += 1;
  vsub $v03, $v27, $v03.v                            ## L:709  |   *340 | result_r:sfract = VONE:sfract - result_r:sfract;
  lfv $v17, 8, 16, $t6                               ## L:770  |      ^ | asm_op("lfv", rough75_g, 8, 16, rough75Pixels);
  vmudh $v02, $v26, $v02.v                           ## L:708  |   *342 | result_g:sfract = V_HDR_MULTIPLIER:sint * result_g:sfract;
  vmulf $v01, $v07, $v11.v                           ## L:710  |    343 | result_b:sfract = specular_b:sfract * fresnel:sfract;
  vmacf $v01, $v13, $v04.v                           ## L:711  |    344 | result_b:sfract = albedo_b:sfract +* albedomultiplier_b:sfract;
  lfv $v19, 0, 0, $t7                                ## L:780  |      ^ | asm_op("lfv", rough25_b, 0,  0, rough25Pixels);
  vsub $v02, $v27, $v02.v                            ## L:713  |   *346 | result_g:sfract = VONE:sfract - result_g:sfract;
  lfv $v11, 0, 0, $t4                                ## L:716  |      ^ | asm_op("lfv", fresnel, 0,  0, fresnelPixels);
  lfv $v19, 8, 16, $t7                               ## L:782  |    347 | asm_op("lfv", rough25_b, 8, 16, rough25Pixels);
  vmulf $v03, $v03, $v03.v                           ## L:714  |      ^ | result_r:sfract = result_r:sfract * result_r:sfract;
  vmudh $v01, $v26, $v01.v                           ## L:712  |    348 | result_b:sfract = V_HDR_MULTIPLIER:sint * result_b:sfract;
  lfv $v17, 0, 0, $t6                                ## L:768  |      ^ | asm_op("lfv", rough75_g, 0,  0, rough75Pixels);
  vsub $v01, $v27, $v01.v                            ## L:717  | ***352 | result_b:sfract = VONE:sfract - result_b:sfract;
  addiu $t6, $t6, 1                                  ## L:771  |      ^ | rough75Pixels += 1;
  vmulf $v02, $v02, $v02.v                           ## L:718  |    353 | result_g:sfract = result_g:sfract * result_g:sfract;
  vmulf $v03, $v03, $v03.v                           ## L:719  |    354 | result_r:sfract = result_r:sfract * result_r:sfract;
  lfv $v16, 0, 0, $t6                                ## L:786  |      ^ | asm_op("lfv", rough75_b, 0,  0, rough75Pixels);
  vmulf $v01, $v01, $v01.v                           ## L:723  |   *356 | result_b:sfract = result_b:sfract * result_b:sfract;
  lfv $v16, 8, 16, $t6                               ## L:788  |      ^ | asm_op("lfv", rough75_b, 8, 16, rough75Pixels);
  vmulf $v02, $v02, $v02.v                           ## L:724  |    357 | result_g:sfract = result_g:sfract * result_g:sfract;
  lfv $v11, 8, 16, $t4                               ## L:721  |      ^ | asm_op("lfv", fresnel, 8, 16, fresnelPixels);
  vsub $v03, $v27, $v03.v                            ## L:725  |    358 | result_r:sfract = VONE:sfract - result_r:sfract;
  vmulf $v01, $v01, $v01.v                           ## L:728  |   *360 | result_b:sfract = result_b:sfract * result_b:sfract;
  vsub $v02, $v27, $v02.v                            ## L:729  |    361 | result_g:sfract = VONE:sfract - result_g:sfract;
  addiu $t7, $t7, 94                                 ## L:783  |      ^ | rough25Pixels += 94;
  vadd $v03, $v03, $v25.v                            ## L:730  |    362 | result_r:sfract = result_r:sfract + dither:sfract;
  vsub $v01, $v27, $v01.v                            ## L:734  |   *364 | result_b:sfract = VONE:sfract - result_b:sfract;
  vadd $v02, $v02, $v25.v                            ## L:735  |    365 | result_g:sfract = result_g:sfract + dither:sfract;
  lfv $v23, 0, 0, $t8                                ## L:757  |      ^ | asm_op("lfv", diffuse_g, 0,  0, diffusePixels);
  addiu $t8, $t8, 1                                  ## L:760  |    366 | diffusePixels += 1;
  vand $v03, $v03, $v28.e0                           ## L:736  |      ^ | result_r &= VCONSTS.x;
  lfv $v22, 8, 16, $t8                               ## L:776  |   *368 | asm_op("lfv", diffuse_b, 8, 16, diffusePixels);
  vadd $v01, $v01, $v25.v                            ## L:739  |      ^ | result_b:sfract = result_b:sfract + dither:sfract;
  vand $v02, $v02, $v28.e0                           ## L:740  |    369 | result_g &= VCONSTS.x;
  addiu $t3, $t3, 65528                              ## L:795  |      ^ | pixelcounter -= 8;
  vmudn $v03, $v03, $v30.e6                          ## L:741  |    370 | result_r <<= 1;
  vand $v01, $v01, $v28.e0                           ## L:745  |   *372 | result_b &= VCONSTS.x;
  vmudm $v02, $v02, $v31.e3                          ## L:747  |    373 | result_g >>= 4;
  vor $v03, $v03, $v28.e1                            ## L:746  |    374 | result_r |= VCONSTS.y;
  vmudm $v01, $v01, $v30.e0                          ## L:753  |   *376 | result_b >>= 9;
  addiu $t6, $t6, 94                                 ## L:789  |      ^ | rough75Pixels += 94;
  lfv $v22, 0, 0, $t8                                ## L:774  |   *378 | asm_op("lfv", diffuse_b, 0,  0, diffusePixels);
  vor $v10, $v03, $v02.v                             ## L:755  |      ^ | result16bpp = result_r | result_g;
  addiu $t8, $t8, 94                                 ## L:777  |    379 | diffusePixels += 94;
  addiu $t4, $t4, 96                                 ## L:722  |  **382 | fresnelPixels += 96;
  vor $v10, $v10, $v01.v                             ## L:763  |      ^ | result16bpp |= result_b;
  sqv $v10, 0, 0, $t5                                ## L:791  | ***386 | store(result16bpp, destPixels);
  j LABEL_PBRBlendCmd_PostProcess_0011               ## L:795  |    387 | pixelcounter -= 8;
  addiu $t5, $t5, 16                                 ## L:794  |   *389 | destPixels += 16;
  LABEL_PBRBlendCmd_PostProcess_0012:
  lw $t2, %lo(SRC_PIXELS_ITERATIONS + 0)             ## L:799  |    390 | u32 bufferscounter = load(SRC_PIXELS_ITERATIONS);
  addiu $t2, $t2, 65534                              ## L:800  |  **393 | bufferscounter -= 2;
  j postprocess_loop                                 ## L:804  |    394 | goto postprocess_loop;
  sw $t2, %lo(SRC_PIXELS_ITERATIONS)($zero)          ## L:801  |   *396 | store(bufferscounter, SRC_PIXELS_ITERATIONS);
  LABEL_PBRBlendCmd_PostProcess_0013:
  mfc0 $ra, COP0_DMA_BUSY                            ## L:92   |    397 | RA = get_dma_busy();
  bne $ra, $zero, LABEL_PBRBlendCmd_PostProcess_0013 ## L:92   |  **400 | RA = get_dma_busy();
  nop                                                ## L:92   |   *402 | RA = get_dma_busy();
  LABEL_PBRBlendCmd_PostProcess_0014:
  ori $s0, $zero, %lo(DEST_BUF2)                     ## L:808  |    403 | destDMEM = DEST_BUF2;
  mtc0 $s0, COP0_DMA_SPADDR                          ## L:84   |    404 | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x1
  mtc0 $s4, COP0_DMA_RAMADDR                         ## L:85   |    405 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x1
  mtc0 $sp, COP0_DMA_WRITE                           ## L:86   |  **408 | @Barrier("DMA") set_dma_write(size); ## Barrier: 0x1
  LABEL_PBRBlendCmd_PostProcess_0015:
  mfc0 $ra, COP0_DMA_BUSY                            ## L:92   |    409 | RA = get_dma_busy();
  bne $ra, $zero, LABEL_PBRBlendCmd_PostProcess_0015 ## L:92   |  **412 | RA = get_dma_busy();
  nop                                                ## L:92   |   *414 | RA = get_dma_busy();
  LABEL_PBRBlendCmd_PostProcess_0016:
  j RSPQ_Loop                                        ## L:93   |    415 | 
  nop                                                ## L:93   |   *417 | 

OVERLAY_CODE_END:

#define zero $0
#define v0 $2
#define v1 $3
#define a0 $4
#define a1 $5
#define a2 $6
#define a3 $7
#define t0 $8
#define t1 $9
#define t2 $10
#define t3 $11
#define t4 $12
#define t5 $13
#define t6 $14
#define t7 $15
#define s0 $16
#define s1 $17
#define s2 $18
#define s3 $19
#define s4 $20
#define s5 $21
#define s6 $22
#define s7 $23
#define t8 $24
#define t9 $25
#define k0 $26
#define k1 $27
#define gp $28
#define sp $29
#define fp $30
#define ra $31

.set at
.set macro