## Auto-generated file, transpiled with RSPL
#define RGBA32_CHANNELS 4
#define RGBA32_BYTES 4
#define RGBA16_BYTES 2
#define BUFFER_PIXELS 96
#define BUFFER_SIZE_GBUFFER 192
#define BUFFER_SIZE_LIGHTING 384
#define BUFFER_SIZE_GBUFFER_MINUSONE 191
#define BUFFER_SIZE_LIGHTING_MINUSONE 383
#define BUFFER_ITERATION_SIZE 12
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
    RSPQ_DefineCommand PBRBlendCmd_SetLightingBuffers, 12
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
    DIFFUSE_SRC_RDRAM: .ds.b 4
    .align 2
    ROUGH25_SRC_RDRAM: .ds.b 4
    .align 2
    ROUGH75_SRC_RDRAM: .ds.b 4
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
    DIFFUSE_BUF1: .ds.b 384
    .align 4
    ROUGH25_BUF1: .ds.b 384
    .align 4
    ROUGH75_BUF1: .ds.b 384
    .align 4
    DIFFUSE_BUF2: .ds.b 384
    .align 4
    ROUGH25_BUF2: .ds.b 384
    .align 4
    ROUGH75_BUF2: .ds.b 384
    STATE_MEM_END:
  RSPQ_EndSavedState

.text
OVERLAY_CODE_START:

PBRBlendCmd_SetGBuffer:
  sw $a0, %lo(ALBEDO_SRC_RDRAM)($zero)
  sw $a2, %lo(DEST_SRC_RDRAM)($zero)
  sw $a1, %lo(PACKED_SRC_RDRAM)($zero)
  j RSPQ_Loop
  sw $a3, %lo(SRC_PIXELS_ITERATIONS)($zero)
PBRBlendCmd_SetLightingBuffers:
  sw $a2, %lo(ROUGH75_SRC_RDRAM)($zero)
  sw $a1, %lo(ROUGH25_SRC_RDRAM)($zero)
  j RSPQ_Loop
  sw $a0, %lo(DIFFUSE_SRC_RDRAM)($zero)
PBRBlendCmd_PostProcess:
  vxor $v27, $v00, $v31.e0
  lw $s7, %lo(DIFFUSE_SRC_RDRAM + 0)
  lw $s3, %lo(PACKED_SRC_RDRAM + 0)
  ori $s1, $zero, %lo(ALBEDO_BUF1)
  lw $s4, %lo(ALBEDO_SRC_RDRAM + 0)
  lw $s5, %lo(ROUGH75_SRC_RDRAM + 0)
  lw $s2, %lo(DEST_SRC_RDRAM + 0)
  mtc0 $s1, COP0_DMA_SPADDR
  mtc0 $s4, COP0_DMA_RAMADDR
  lw $s6, %lo(ROUGH25_SRC_RDRAM + 0)
  ori $s0, $zero, %lo(PACKED_BUF1)
  ori $sp, $zero, %lo(ROUGH25_BUF1)
  addiu $k0, $zero, 191
  mtc0 $k0, COP0_DMA_READ
  mtc0 $s0, COP0_DMA_SPADDR
  addiu $t9, $zero, 383
  ori $s1, $zero, %lo(ALBEDO_BUF2)
  mtc0 $s3, COP0_DMA_RAMADDR
  mtc0 $k0, COP0_DMA_READ
  ori $fp, $zero, %lo(DIFFUSE_BUF1)
  ori $s0, $zero, %lo(PACKED_BUF2)
  mtc0 $fp, COP0_DMA_SPADDR
  mtc0 $s7, COP0_DMA_RAMADDR
  addiu $at, $zero, 31744
  ori $k1, $zero, %lo(ROUGH75_BUF1)
  mtc0 $t9, COP0_DMA_READ
  mtc0 $sp, COP0_DMA_SPADDR
  mtc0 $s6, COP0_DMA_RAMADDR
  mtc0 $t9, COP0_DMA_READ
  mtc2 $at, $v28.e0
  mtc0 $k1, COP0_DMA_SPADDR
  ori $sp, $zero, %lo(ROUGH25_BUF2)
  ori $fp, $zero, %lo(DIFFUSE_BUF2)
  mtc0 $s5, COP0_DMA_RAMADDR
  ori $k1, $zero, %lo(ROUGH75_BUF2)
  mtc0 $t9, COP0_DMA_READ
  vmov $v28.e1, $v30.e7
  LABEL_PBRBlendCmd_PostProcess_0001:
  mfc0 $ra, COP0_DMA_BUSY
  bne $ra, $zero, LABEL_PBRBlendCmd_PostProcess_0001
  nop
  postprocess_loop:
  lw $t8, %lo(SRC_PIXELS_ITERATIONS + 0)
  beq $t8, $zero, LABEL_PBRBlendCmd_PostProcess_000C
  nop
  addiu $s4, $s4, 192
  addiu $s5, $s5, 384
  mtc0 $s1, COP0_DMA_SPADDR
  mtc0 $s4, COP0_DMA_RAMADDR
  ori $t5, $zero, %lo(ROUGH25_BUF1)
  addiu $s3, $s3, 192
  ori $t3, $zero, %lo(DEST_BUF1)
  mtc0 $k0, COP0_DMA_READ
  ori $t7, $zero, %lo(PACKED_BUF1)
  ori $t4, $zero, %lo(ROUGH75_BUF1)
  mtc0 $s0, COP0_DMA_SPADDR
  mtc0 $s3, COP0_DMA_RAMADDR
  addiu $s6, $s6, 384
  ori $t8, $zero, %lo(ALBEDO_BUF1)
  ori $t6, $zero, %lo(DIFFUSE_BUF1)
  mtc0 $k0, COP0_DMA_READ
  mtc0 $fp, COP0_DMA_SPADDR
  addiu $s7, $s7, 384
  mtc0 $s7, COP0_DMA_RAMADDR
  mtc0 $t9, COP0_DMA_READ
  mtc0 $sp, COP0_DMA_SPADDR
  mtc0 $s6, COP0_DMA_RAMADDR
  ori $t2, $zero, %lo(ROUGH25_BUF1)
  ori $s0, $zero, %lo(PACKED_BUF1)
  ori $s1, $zero, %lo(ALBEDO_BUF1)
  mtc0 $t9, COP0_DMA_READ
  mtc0 $k1, COP0_DMA_SPADDR
  ori $fp, $zero, %lo(DIFFUSE_BUF1)
  addiu $t2, $t2, 3
  mtc0 $s5, COP0_DMA_RAMADDR
  ori $sp, $zero, %lo(ROUGH25_BUF1)
  ori $k1, $zero, %lo(ROUGH75_BUF1)
  addiu $t1, $zero, 96
  mtc0 $t9, COP0_DMA_READ
  LABEL_PBRBlendCmd_PostProcess_0004:
  blez $t1, LABEL_PBRBlendCmd_PostProcess_0005
  nop
  lqv $v26, 0, 0, $t8
  lqv $v25, 0, 0, $t7
  lfv $v17, 0, 0, $t2
  lfv $v16, 0, 0, $t6
  vmudm $v24, $v26, $v31.e0
  vmudm $v21, $v25, $v31.e0
  lfv $v16, 8, 16, $t6
  lfv $v17, 8, 16, $t2
  vand $v20, $v25, $v28.e1
  vmudn $v23, $v26, $v30.e3
  vmudn $v22, $v26, $v31.e6
  lfv $v13, 0, 0, $t5
  vand $v24, $v24, $v28.e0
  addiu $t6, $t6, 1
  vmudn $v20, $v20, $v31.e0
  lfv $v13, 8, 16, $t5
  addiu $t5, $t5, 1
  vand $v21, $v21, $v28.e0
  lfv $v10, 0, 0, $t4
  lfv $v10, 8, 16, $t4
  lfv $v15, 0, 0, $t6
  vand $v23, $v23, $v28.e0
  lfv $v15, 8, 16, $t6
  vsubc $v18, $v27, $v20.v
  vsubc $v19, $v27, $v21.v
  addiu $t4, $t4, 1
  addiu $t6, $t6, 1
  lfv $v12, 0, 0, $t5
  lfv $v12, 8, 16, $t5
  vmulu $v06, $v24, $v18.v
  lfv $v09, 0, 0, $t4
  vmulu $v05, $v24, $v20.v
  lfv $v14, 0, 0, $t6
  lfv $v14, 8, 16, $t6
  vand $v22, $v22, $v28.e0
  addiu $t5, $t5, 1
  vmulu $v04, $v10, $v21.v
  vmacu $v04, $v13, $v19.v
  lfv $v11, 0, 0, $t5
  lfv $v11, 8, 16, $t5
  vmulu $v07, $v05, $v04.v
  vmacu $v07, $v06, $v16.v
  addiu $t2, $t2, 32
  vmacu $v07, $v04, $v17.v
  addiu $t7, $t7, 16
  vmulu $v04, $v23, $v20.v
  lfv $v09, 8, 16, $t4
  addiu $t4, $t4, 1
  vmulu $v05, $v23, $v18.v
  vmulf $v07, $v07, $v30.e4
  lfv $v08, 0, 0, $t4
  vmulu $v03, $v09, $v21.v
  lfv $v08, 8, 16, $t4
  vmacu $v03, $v12, $v19.v
  vmulu $v06, $v04, $v03.v
  vmacu $v06, $v05, $v15.v
  vmacu $v06, $v03, $v17.v
  vmulu $v04, $v22, $v18.v
  vmulu $v03, $v22, $v20.v
  vmulf $v06, $v06, $v30.e4
  vmulu $v02, $v08, $v21.v
  addiu $t8, $t8, 16
  vmacu $v02, $v11, $v19.v
  vmulu $v05, $v03, $v02.v
  vmacu $v05, $v04, $v14.v
  vmacu $v05, $v02, $v17.v
  vand $v07, $v07, $v28.e0
  vand $v06, $v06, $v28.e0
  vmulf $v05, $v05, $v30.e4
  addiu $t4, $t4, 42
  vmudm $v06, $v06, $v31.e3
  vand $v05, $v05, $v28.e0
  vmudn $v07, $v07, $v30.e6
  vmudm $v05, $v05, $v30.e0
  addiu $t5, $t5, 42
  vor $v04, $v07, $v06.v
  addiu $t6, $t6, 42
  addiu $t1, $t1, 65528
  vor $v04, $v04, $v05.v
  sqv $v04, 0, 0, $t3
  j LABEL_PBRBlendCmd_PostProcess_0004
  addiu $t3, $t3, 16
  LABEL_PBRBlendCmd_PostProcess_0005:
  addiu $s4, $s4, 192
  addiu $s5, $s5, 384
  addiu $s7, $s7, 384
  addiu $s6, $s6, 384
  addiu $s3, $s3, 192
  LABEL_PBRBlendCmd_PostProcess_0006:
  mfc0 $ra, COP0_DMA_BUSY
  bne $ra, $zero, LABEL_PBRBlendCmd_PostProcess_0006
  nop
  LABEL_PBRBlendCmd_PostProcess_0007:
  mtc0 $s1, COP0_DMA_SPADDR
  mtc0 $s4, COP0_DMA_RAMADDR
  mtc0 $k0, COP0_DMA_READ
  ori $t5, $zero, %lo(ROUGH25_BUF2)
  mtc0 $s0, COP0_DMA_SPADDR
  mtc0 $s3, COP0_DMA_RAMADDR
  mtc0 $k0, COP0_DMA_READ
  mtc0 $fp, COP0_DMA_SPADDR
  ori $t2, $zero, %lo(ROUGH25_BUF2)
  mtc0 $s7, COP0_DMA_RAMADDR
  mtc0 $t9, COP0_DMA_READ
  addiu $t2, $t2, 3
  ori $t7, $zero, %lo(PACKED_BUF2)
  mtc0 $sp, COP0_DMA_SPADDR
  mtc0 $s6, COP0_DMA_RAMADDR
  mtc0 $t9, COP0_DMA_READ
  mtc0 $k1, COP0_DMA_SPADDR
  addiu $t1, $zero, 96
  ori $t3, $zero, %lo(DEST_BUF2)
  mtc0 $s5, COP0_DMA_RAMADDR
  mtc0 $t9, COP0_DMA_READ
  ori $t0, $zero, %lo(DEST_BUF1)
  ori $t4, $zero, %lo(ROUGH75_BUF2)
  mtc0 $t0, COP0_DMA_SPADDR
  mtc0 $s2, COP0_DMA_RAMADDR
  ori $t8, $zero, %lo(ALBEDO_BUF2)
  addiu $s2, $s2, 192
  mtc0 $k0, COP0_DMA_WRITE
  ori $t6, $zero, %lo(DIFFUSE_BUF2)
  LABEL_PBRBlendCmd_PostProcess_0008:
  blez $t1, LABEL_PBRBlendCmd_PostProcess_0009
  nop
  lqv $v26, 0, 0, $t8
  lqv $v25, 0, 0, $t7
  lfv $v15, 0, 0, $t6
  vmudn $v23, $v26, $v30.e3
  lfv $v17, 0, 0, $t2
  lfv $v17, 8, 16, $t2
  vmudm $v24, $v26, $v31.e0
  vmudn $v22, $v26, $v31.e6
  vand $v20, $v25, $v28.e1
  lfv $v15, 8, 16, $t6
  addiu $t6, $t6, 1
  lfv $v14, 0, 0, $t6
  vand $v24, $v24, $v28.e0
  lfv $v12, 0, 0, $t5
  lfv $v12, 8, 16, $t5
  lfv $v14, 8, 16, $t6
  addiu $t5, $t5, 1
  vand $v22, $v22, $v28.e0
  addiu $t6, $t6, 1
  vmudn $v20, $v20, $v31.e0
  lfv $v09, 0, 0, $t4
  vmudm $v21, $v25, $v31.e0
  lfv $v11, 0, 0, $t5
  lfv $v11, 8, 16, $t5
  lfv $v09, 8, 16, $t4
  addiu $t4, $t4, 1
  vand $v21, $v21, $v28.e0
  vand $v23, $v23, $v28.e0
  lfv $v13, 0, 0, $t6
  vsubc $v18, $v27, $v20.v
  lfv $v13, 8, 16, $t6
  lfv $v08, 0, 0, $t4
  vmulu $v04, $v24, $v20.v
  vsubc $v19, $v27, $v21.v
  addiu $t5, $t5, 1
  vsubc $v16, $v27, $v17.v
  lfv $v10, 0, 0, $t5
  vmulu $v05, $v24, $v18.v
  lfv $v08, 8, 16, $t4
  vmulu $v03, $v09, $v21.v
  addiu $t4, $t4, 1
  vmacu $v03, $v12, $v19.v
  lfv $v10, 8, 16, $t5
  vmulu $v06, $v04, $v03.v
  lfv $v07, 0, 0, $t4
  vmacu $v06, $v05, $v15.v
  addiu $t7, $t7, 16
  vmacu $v06, $v03, $v17.v
  addiu $t5, $t5, 42
  vmulu $v04, $v23, $v18.v
  vmulu $v03, $v23, $v20.v
  vmulf $v06, $v06, $v30.e4
  vmulu $v02, $v08, $v21.v
  lfv $v07, 8, 16, $t4
  vmacu $v02, $v11, $v19.v
  vmulu $v05, $v03, $v02.v
  addiu $t4, $t4, 42
  vmacu $v05, $v04, $v14.v
  vmacu $v05, $v02, $v17.v
  vmulu $v02, $v22, $v20.v
  vmulu $v03, $v22, $v18.v
  vmulf $v05, $v05, $v30.e4
  vmulu $v01, $v07, $v21.v
  vmacu $v01, $v10, $v19.v
  vmulu $v04, $v02, $v01.v
  vmacu $v04, $v03, $v13.v
  vmacu $v04, $v01, $v17.v
  vand $v05, $v05, $v28.e0
  vand $v06, $v06, $v28.e0
  vmulf $v04, $v04, $v30.e4
  vmudm $v05, $v05, $v31.e3
  vand $v04, $v04, $v28.e0
  vmudn $v06, $v06, $v30.e6
  vmudm $v04, $v04, $v30.e0
  addiu $t2, $t2, 32
  vor $v03, $v06, $v05.v
  addiu $t8, $t8, 16
  vor $v03, $v03, $v04.v
  addiu $t1, $t1, 65528
  addiu $t6, $t6, 42
  sqv $v03, 0, 0, $t3
  addiu $t3, $t3, 16
  j LABEL_PBRBlendCmd_PostProcess_0008
  nop
  LABEL_PBRBlendCmd_PostProcess_0009:
  ori $k1, $zero, %lo(ROUGH75_BUF2)
  ori $fp, $zero, %lo(DIFFUSE_BUF2)
  ori $sp, $zero, %lo(ROUGH25_BUF2)
  lw $t0, %lo(SRC_PIXELS_ITERATIONS + 0)
  ori $s0, $zero, %lo(PACKED_BUF2)
  ori $s1, $zero, %lo(ALBEDO_BUF2)
  addiu $t0, $t0, 65535
  sw $t0, %lo(SRC_PIXELS_ITERATIONS)($zero)
  LABEL_PBRBlendCmd_PostProcess_000A:
  mfc0 $ra, COP0_DMA_BUSY
  bne $ra, $zero, LABEL_PBRBlendCmd_PostProcess_000A
  nop
  LABEL_PBRBlendCmd_PostProcess_000B:
  ori $t0, $zero, %lo(DEST_BUF2)
  mtc0 $t0, COP0_DMA_SPADDR
  mtc0 $s2, COP0_DMA_RAMADDR
  mtc0 $k0, COP0_DMA_WRITE
  j postprocess_loop
  addiu $s2, $s2, 192
  LABEL_PBRBlendCmd_PostProcess_000C:
  mfc0 $ra, COP0_DMA_BUSY
  bne $ra, $zero, LABEL_PBRBlendCmd_PostProcess_000C
  nop
  LABEL_PBRBlendCmd_PostProcess_000D:
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