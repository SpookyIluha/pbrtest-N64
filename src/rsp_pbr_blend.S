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
  sw $a1, %lo(PACKED_SRC_RDRAM)($zero)
  sw $a3, %lo(SRC_PIXELS_ITERATIONS)($zero)
  sw $a0, %lo(ALBEDO_SRC_RDRAM)($zero)
  j RSPQ_Loop
  sw $a2, %lo(DEST_SRC_RDRAM)($zero)
PBRBlendCmd_SetLightingBuffers:
  j RSPQ_Loop
  sw $a0, %lo(LIGHTING_SRC_RDRAM)($zero)
PBRBlendCmd_SetDitherMatrix:
  ori $s6, $zero, %lo(DITHER_MATRIX_8X2_1)
  mtc0 $s6, COP0_DMA_SPADDR
  mtc0 $a0, COP0_DMA_RAMADDR
  addiu $s7, $zero, 31
  sw $a1, %lo(DITHER_LINE_IN_BUFFERS)($zero)
  mtc0 $s7, COP0_DMA_READ
  LABEL_PBRBlendCmd_SetDitherMatrix_0001:
  mfc0 $ra, COP0_DMA_BUSY
  bne $ra, $zero, LABEL_PBRBlendCmd_SetDitherMatrix_0001
  nop
  LABEL_PBRBlendCmd_SetDitherMatrix_0002:
  j RSPQ_Loop
  nop
PBRBlendCmd_PostProcess:
  ori $s3, $zero, %lo(ALBEDO_BUF1)
  mtc0 $s3, COP0_DMA_SPADDR
  lw $s6, %lo(ALBEDO_SRC_RDRAM + 0)
  ori $s2, $zero, %lo(PACKED_BUF1)
  addiu $sp, $zero, 191
  mtc0 $s6, COP0_DMA_RAMADDR
  mtc0 $sp, COP0_DMA_READ
  ori $s1, $zero, %lo(LIGHTING_BUF1)
  mtc0 $s2, COP0_DMA_SPADDR
  lw $s7, %lo(LIGHTING_SRC_RDRAM + 0)
  lw $s5, %lo(PACKED_SRC_RDRAM + 0)
  addiu $fp, $zero, 1
  mtc0 $s5, COP0_DMA_RAMADDR
  lw $s4, %lo(DEST_SRC_RDRAM + 0)
  addiu $k1, $zero, 1151
  ori $s0, $zero, %lo(DEST_BUF2)
  mtc0 $sp, COP0_DMA_READ
  LABEL_PBRBlendCmd_PostProcess_0003:
  mfc0 $ra, COP0_DMA_BUSY
  bne $ra, $zero, LABEL_PBRBlendCmd_PostProcess_0003
  nop
  LABEL_PBRBlendCmd_PostProcess_0004:
  addiu $at, $zero, 31744
  mtc2 $at, $v28.e0
  addiu $at, $zero, 32767
  mtc2 $at, $v28.e2
  mtc0 $s1, COP0_DMA_SPADDR
  vmov $v28.e1, $v30.e7
  mtc0 $s7, COP0_DMA_RAMADDR
  mtc0 $k1, COP0_DMA_READ
  vor $v27, $v00, $v28.e2
  vxor $v26, $v00, $v30.e5
  postprocess_loop:
  lw $k0, %lo(SRC_PIXELS_ITERATIONS + 0)
  beq $k0, $zero, LABEL_PBRBlendCmd_PostProcess_0013
  nop
  addiu $s7, $s7, 1152
  addiu $s5, $s5, 192
  ori $s2, $zero, %lo(PACKED_BUF2)
  ori $s0, $zero, %lo(DEST_BUF2)
  addiu $s6, $s6, 192
  ori $s1, $zero, %lo(LIGHTING_BUF2)
  ori $s3, $zero, %lo(ALBEDO_BUF2)
  LABEL_PBRBlendCmd_PostProcess_0006:
  mfc0 $ra, COP0_DMA_BUSY
  bne $ra, $zero, LABEL_PBRBlendCmd_PostProcess_0006
  nop
  LABEL_PBRBlendCmd_PostProcess_0007:
  mtc0 $s3, COP0_DMA_SPADDR
  mtc0 $s6, COP0_DMA_RAMADDR
  mtc0 $sp, COP0_DMA_READ
  mtc0 $s2, COP0_DMA_SPADDR
  mtc0 $s5, COP0_DMA_RAMADDR
  mtc0 $sp, COP0_DMA_READ
  LABEL_PBRBlendCmd_PostProcess_0008:
  mfc0 $ra, COP0_DMA_BUSY
  bne $ra, $zero, LABEL_PBRBlendCmd_PostProcess_0008
  nop
  LABEL_PBRBlendCmd_PostProcess_0009:
  mtc0 $s1, COP0_DMA_SPADDR
  mtc0 $s7, COP0_DMA_RAMADDR
  mtc0 $k1, COP0_DMA_READ
  bne $fp, $zero, LABEL_PBRBlendCmd_PostProcess_000A
  nop
  mtc0 $s0, COP0_DMA_SPADDR
  mtc0 $s4, COP0_DMA_RAMADDR
  addiu $s4, $s4, 192
  mtc0 $sp, COP0_DMA_WRITE
  LABEL_PBRBlendCmd_PostProcess_000A:
  ori $at, $zero, %lo(DITHER_MATRIX_8X2_1)
  ori $s0, $zero, %lo(DEST_BUF1)
  ori $k0, $zero, %lo(ALBEDO_BUF1)
  ori $t7, $zero, %lo(LIGHTING_BUF1)
  ori $s3, $zero, %lo(ALBEDO_BUF1)
  or $fp, $zero, $zero
  addiu $t7, $t7, 32
  ori $t8, $zero, %lo(LIGHTING_BUF1)
  ori $s2, $zero, %lo(PACKED_BUF1)
  ori $t4, $zero, %lo(LIGHTING_BUF1)
  addiu $t4, $t4, 3
  ori $t5, $zero, %lo(DEST_BUF1)
  ori $t9, $zero, %lo(PACKED_BUF1)
  ori $s1, $zero, %lo(LIGHTING_BUF1)
  ori $t6, $zero, %lo(LIGHTING_BUF1)
  lqv $v25, 0, 0, $at
  addiu $t6, $t6, 64
  addiu $t3, $zero, 96
  LABEL_PBRBlendCmd_PostProcess_000B:
  blez $t3, LABEL_PBRBlendCmd_PostProcess_000C
  nop
  lqv $v23, 0, 0, $t9
  lfv $v15, 0, 0, $t8
  lfv $v15, 8, 16, $t8
  lqv $v24, 0, 0, $k0
  addiu $t3, $t3, 65528
  vmudm $v19, $v23, $v31.e0
  vand $v18, $v23, $v28.e1
  addiu $t8, $t8, 1
  lfv $v23, 0, 0, $t4
  lfv $v12, 0, 0, $t7
  vmudm $v22, $v24, $v31.e0
  lfv $v12, 8, 16, $t7
  vmudn $v21, $v24, $v30.e3
  lfv $v09, 0, 0, $t6
  addiu $t7, $t7, 1
  vmudh $v18, $v18, $v27.v
  lfv $v09, 8, 16, $t6
  vand $v22, $v22, $v28.e0
  lfv $v14, 0, 0, $t8
  lfv $v14, 8, 16, $t8
  addiu $t6, $t6, 1
  vand $v19, $v19, $v28.e0
  lfv $v11, 0, 0, $t7
  addiu $t8, $t8, 1
  lfv $v11, 8, 16, $t7
  lfv $v13, 0, 0, $t8
  vmudn $v20, $v24, $v31.e6
  lfv $v13, 8, 16, $t8
  vmulf $v04, $v22, $v18.v
  addiu $t7, $t7, 1
  vsub $v17, $v27, $v19.v
  vsub $v16, $v27, $v18.v
  lfv $v10, 0, 0, $t7
  vand $v21, $v21, $v28.e0
  lfv $v08, 0, 0, $t6
  vand $v20, $v20, $v28.e0
  lfv $v23, 8, 16, $t4
  lfv $v08, 8, 16, $t6
  lfv $v10, 8, 16, $t7
  vmulf $v05, $v22, $v16.v
  addiu $t6, $t6, 1
  vmulf $v03, $v09, $v19.v
  lfv $v07, 0, 0, $t6
  vmacf $v03, $v12, $v17.v
  vmulf $v06, $v04, $v03.v
  lfv $v07, 8, 16, $t6
  vmacf $v06, $v05, $v15.v
  addiu $t7, $t7, 94
  vmacf $v06, $v03, $v23.v
  vmudh $v06, $v26, $v06.v
  addiu $k0, $k0, 16
  vmacf $v06, $v27, $v25.v
  vmulf $v04, $v21, $v16.v
  vmulf $v03, $v21, $v18.v
  vmulf $v02, $v08, $v19.v
  vmacf $v02, $v11, $v17.v
  vmulf $v05, $v03, $v02.v
  vmacf $v05, $v04, $v14.v
  vmacf $v05, $v02, $v23.v
  vmudh $v05, $v26, $v05.v
  vmacf $v05, $v27, $v25.v
  vmulf $v02, $v20, $v18.v
  addiu $t4, $t4, 96
  vmulf $v03, $v20, $v16.v
  vmulf $v01, $v07, $v19.v
  vmacf $v01, $v10, $v17.v
  vmulf $v04, $v02, $v01.v
  vmacf $v04, $v03, $v13.v
  vmacf $v04, $v01, $v23.v
  vmudh $v04, $v26, $v04.v
  vmacf $v04, $v27, $v25.v
  vand $v05, $v05, $v28.e0
  vand $v06, $v06, $v28.e0
  vand $v04, $v04, $v28.e0
  vmudm $v05, $v05, $v31.e3
  vmudn $v06, $v06, $v30.e6
  vmudm $v04, $v04, $v30.e0
  addiu $t8, $t8, 94
  vor $v03, $v06, $v05.v
  addiu $t6, $t6, 94
  vor $v03, $v03, $v04.v
  vor $v03, $v03, $v28.e1
  addiu $t9, $t9, 16
  sqv $v03, 0, 0, $t5
  j LABEL_PBRBlendCmd_PostProcess_000B
  addiu $t5, $t5, 16
  LABEL_PBRBlendCmd_PostProcess_000C:
  addiu $s6, $s6, 192
  addiu $s5, $s5, 192
  addiu $s7, $s7, 1152
  LABEL_PBRBlendCmd_PostProcess_000D:
  mfc0 $ra, COP0_DMA_BUSY
  bne $ra, $zero, LABEL_PBRBlendCmd_PostProcess_000D
  nop
  LABEL_PBRBlendCmd_PostProcess_000E:
  mtc0 $s3, COP0_DMA_SPADDR
  mtc0 $s6, COP0_DMA_RAMADDR
  mtc0 $sp, COP0_DMA_READ
  mtc0 $s2, COP0_DMA_SPADDR
  mtc0 $s5, COP0_DMA_RAMADDR
  mtc0 $sp, COP0_DMA_READ
  LABEL_PBRBlendCmd_PostProcess_000F:
  mfc0 $ra, COP0_DMA_BUSY
  bne $ra, $zero, LABEL_PBRBlendCmd_PostProcess_000F
  nop
  LABEL_PBRBlendCmd_PostProcess_0010:
  ori $t4, $zero, %lo(LIGHTING_BUF2)
  mtc0 $s1, COP0_DMA_SPADDR
  mtc0 $s7, COP0_DMA_RAMADDR
  ori $k0, $zero, %lo(ALBEDO_BUF2)
  ori $t9, $zero, %lo(PACKED_BUF2)
  mtc0 $k1, COP0_DMA_READ
  mtc0 $s0, COP0_DMA_SPADDR
  ori $t5, $zero, %lo(DEST_BUF2)
  ori $t6, $zero, %lo(LIGHTING_BUF2)
  ori $at, $zero, %lo(DITHER_MATRIX_8X2_2)
  ori $t8, $zero, %lo(LIGHTING_BUF2)
  ori $t7, $zero, %lo(LIGHTING_BUF2)
  mtc0 $s4, COP0_DMA_RAMADDR
  addiu $s4, $s4, 192
  addiu $t4, $t4, 3
  addiu $t7, $t7, 32
  lqv $v25, 0, 0, $at
  mtc0 $sp, COP0_DMA_WRITE
  addiu $t6, $t6, 64
  addiu $t3, $zero, 96
  LABEL_PBRBlendCmd_PostProcess_0011:
  blez $t3, LABEL_PBRBlendCmd_PostProcess_0012
  nop
  lqv $v24, 0, 0, $k0
  lqv $v23, 0, 0, $t9
  lfv $v15, 0, 0, $t8
  lfv $v15, 8, 16, $t8
  vmudn $v21, $v24, $v30.e3
  vmudm $v22, $v24, $v31.e0
  lfv $v12, 0, 0, $t7
  lfv $v12, 8, 16, $t7
  vmudm $v19, $v23, $v31.e0
  vand $v18, $v23, $v28.e1
  lfv $v09, 0, 0, $t6
  addiu $t8, $t8, 1
  lfv $v14, 0, 0, $t8
  lfv $v09, 8, 16, $t6
  vand $v19, $v19, $v28.e0
  lfv $v23, 0, 0, $t4
  lfv $v14, 8, 16, $t8
  vmudh $v18, $v18, $v27.v
  vand $v22, $v22, $v28.e0
  addiu $t7, $t7, 1
  vsub $v17, $v27, $v19.v
  addiu $t6, $t6, 1
  lfv $v11, 0, 0, $t7
  lfv $v11, 8, 16, $t7
  vmudn $v20, $v24, $v31.e6
  addiu $t7, $t7, 1
  addiu $t8, $t8, 1
  vsub $v16, $v27, $v18.v
  lfv $v13, 0, 0, $t8
  vand $v20, $v20, $v28.e0
  vand $v21, $v21, $v28.e0
  lfv $v13, 8, 16, $t8
  vmulf $v05, $v22, $v16.v
  lfv $v10, 0, 0, $t7
  vmulf $v04, $v22, $v18.v
  lfv $v08, 0, 0, $t6
  vmulf $v03, $v09, $v19.v
  lfv $v23, 8, 16, $t4
  lfv $v08, 8, 16, $t6
  vmacf $v03, $v12, $v17.v
  vmulf $v06, $v04, $v03.v
  vmacf $v06, $v05, $v15.v
  addiu $t6, $t6, 1
  vmacf $v06, $v03, $v23.v
  lfv $v10, 8, 16, $t7
  vmudh $v06, $v26, $v06.v
  lfv $v07, 0, 0, $t6
  vmacf $v06, $v27, $v25.v
  lfv $v07, 8, 16, $t6
  addiu $t3, $t3, 65528
  vmulf $v04, $v21, $v16.v
  vmulf $v03, $v21, $v18.v
  vmulf $v02, $v08, $v19.v
  vmacf $v02, $v11, $v17.v
  vmulf $v05, $v03, $v02.v
  vmacf $v05, $v04, $v14.v
  vmacf $v05, $v02, $v23.v
  vmudh $v05, $v26, $v05.v
  vmacf $v05, $v27, $v25.v
  vmulf $v02, $v20, $v18.v
  vmulf $v03, $v20, $v16.v
  vmulf $v01, $v07, $v19.v
  vmacf $v01, $v10, $v17.v
  vmulf $v04, $v02, $v01.v
  vmacf $v04, $v03, $v13.v
  vmacf $v04, $v01, $v23.v
  vmudh $v04, $v26, $v04.v
  vmacf $v04, $v27, $v25.v
  vand $v05, $v05, $v28.e0
  vand $v06, $v06, $v28.e0
  vand $v04, $v04, $v28.e0
  vmudm $v05, $v05, $v31.e3
  addiu $t9, $t9, 16
  vmudn $v06, $v06, $v30.e6
  addiu $t7, $t7, 94
  vmudm $v04, $v04, $v30.e0
  addiu $t6, $t6, 94
  vor $v03, $v06, $v05.v
  addiu $k0, $k0, 16
  addiu $t4, $t4, 96
  vor $v03, $v03, $v04.v
  vor $v03, $v03, $v28.e1
  addiu $t8, $t8, 94
  sqv $v03, 0, 0, $t5
  j LABEL_PBRBlendCmd_PostProcess_0011
  addiu $t5, $t5, 16
  LABEL_PBRBlendCmd_PostProcess_0012:
  lw $t2, %lo(SRC_PIXELS_ITERATIONS + 0)
  addiu $t2, $t2, 65534
  sw $t2, %lo(SRC_PIXELS_ITERATIONS)($zero)
  j postprocess_loop
  nop
  LABEL_PBRBlendCmd_PostProcess_0013:
  mfc0 $ra, COP0_DMA_BUSY
  bne $ra, $zero, LABEL_PBRBlendCmd_PostProcess_0013
  nop
  LABEL_PBRBlendCmd_PostProcess_0014:
  ori $s0, $zero, %lo(DEST_BUF2)
  mtc0 $s0, COP0_DMA_SPADDR
  mtc0 $s4, COP0_DMA_RAMADDR
  mtc0 $sp, COP0_DMA_WRITE
  LABEL_PBRBlendCmd_PostProcess_0015:
  mfc0 $ra, COP0_DMA_BUSY
  bne $ra, $zero, LABEL_PBRBlendCmd_PostProcess_0015
  nop
  LABEL_PBRBlendCmd_PostProcess_0016:
  j RSPQ_Loop
  nop

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