/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */

.ifndef GIN_STOCK_PROTECTED_CONTROL
.set GIN_STOCK_PROTECTED_CONTROL, 0
.endif
.if GIN_STOCK_PROTECTED_CONTROL != 0 && GIN_STOCK_PROTECTED_CONTROL != 1
.error "GIN_STOCK_PROTECTED_CONTROL must be 0 or 1"
.endif
.text
.align 2
.macro SAVE
 [--SP] = ASTAT;
 [--SP] = RETS;
 [--SP] = FP;
 [--SP] = (R7:0, P5:0);
 [--SP] = A0.w;
 [--SP] = A0.x;
 [--SP] = A1.w;
 [--SP] = A1.x;
 [--SP] = I0;
 [--SP] = I1;
 [--SP] = I2;
 [--SP] = I3;
 [--SP] = M0;
 [--SP] = M1;
 [--SP] = M2;
 [--SP] = M3;
 [--SP] = B0;
 [--SP] = B1;
 [--SP] = B2;
 [--SP] = B3;
 [--SP] = L0;
 [--SP] = L1;
 [--SP] = L2;
 [--SP] = L3;
 [--SP] = LC0;
 [--SP] = LC1;
 [--SP] = LT0;
 [--SP] = LT1;
 [--SP] = LB0;
 [--SP] = LB1;

 R0 = 0;
 L0 = R0;
 L1 = R0;
 L2 = R0;
 L3 = R0;
 LC0 = R0;
 LC1 = R0;
 SP += -12;
.endm
.macro RESTORE
 SP += 12;
 LB1 = [SP++];
 LB0 = [SP++];
 LT1 = [SP++];
 LT0 = [SP++];
 LC1 = [SP++];
 LC0 = [SP++];
 L3 = [SP++];
 L2 = [SP++];
 L1 = [SP++];
 L0 = [SP++];
 B3 = [SP++];
 B2 = [SP++];
 B1 = [SP++];
 B0 = [SP++];
 M3 = [SP++];
 M2 = [SP++];
 M1 = [SP++];
 M0 = [SP++];
 I3 = [SP++];
 I2 = [SP++];
 I1 = [SP++];
 I0 = [SP++];
 A1.x = [SP++];
 A1.w = [SP++];
 A0.x = [SP++];
 A0.w = [SP++];
 (R7:0, P5:0) = [SP++];
 FP = [SP++];
 RETS = [SP++];
 ASTAT = [SP++];
.endm




.macro OWN_ENTER op
 [SP] = P0;
 [SP + 4] = R0;
.ifdef GIN_CPU_PROFILE

 R0 = CYCLES;
 [SP + 8] = R0;
.endif
 P0.L = _gin_fw_stack_top_\op;
 P0.H = _gin_fw_stack_top_\op;
 R0 = SP;
 [P0] = R0;
 R0 = [SP];
 [P0 + 4] = R0;
 CLI R0;
 [P0 + 8] = R0;
 R0 = [SP + 4];
 SP = P0;
.global _gin_fw_stack_entered_\op
_gin_fw_stack_entered_\op:
.endm
.macro OWN_LEAVE tag, lane
 P0 = [SP];
 [P0 + 4] = R0;
.ifdef GIN_CPU_PROFILE


 [--SP] = ASTAT;
 [--SP] = R1;
 R1 = CYCLES;
 R0 = [P0 + 8];
 R0 = R1 - R0;
 P0.L = _gin_profile_wrapper_raw + 12*\lane;
 P0.H = _gin_profile_wrapper_raw + 12*\lane;
 R1 = [P0];
 R1 = R1 + R0;
 [P0] = R1;
 R1 = [P0 + 4];
 R1 += 1;
 [P0 + 4] = R1;
 R1 = [P0 + 8];
 CC = R0 <= R1 (IU);
 IF CC JUMP .Lprofile_span_done_\@;
 [P0 + 8] = R0;
.Lprofile_span_done_\@:
 R1 = [SP++];
 ASTAT = [SP++];
 P0 = [SP];
.endif
 R0 = [SP + 8];
 SP = P0;
.global _gin_fw_stack_left_\tag
_gin_fw_stack_left_\tag:
 P0 = [SP];
 STI R0;
 R0 = [SP + 4];
.global _gin_fw_stack_left_end_\tag
_gin_fw_stack_left_end_\tag:
.endm

.global _gin_fw_entry_render
_gin_fw_entry_render:
 OWN_ENTER render
 SAVE
 R0 = [SP + 168];
 CALL _gin_fw_route;
 CC = R0 == 0;
 IF CC JUMP .Lstock_render;
 R0 = [SP + 168];
 R1 = [SP + 164];
 R2 = [SP + 160];
 CALL _gin_fw_render;
.global _gin_fw_after_render
_gin_fw_after_render:
 [SP + 168] = R0;
 RESTORE
 OWN_LEAVE render, 3
 RTS;
.Lstock_render:
.if GIN_STOCK_PROTECTED_CONTROL



 R0 = [SP + 168];
 R1 = [SP + 164];
 R2 = [SP + 160];
 CALL _gin_fw_profile_stock_render;
 JUMP _gin_fw_after_render;
.else
 RESTORE
 OWN_LEAVE stock_render, 3

 JUMP.L _gin_fw_profile_stock_render;
.endif
.global _gin_fw_entry_trigger
_gin_fw_entry_trigger:
 OWN_ENTER trigger
 SAVE
 R0 = [SP + 168];
 CALL _gin_fw_route;
 CC = R0 == 0;
 IF CC JUMP .Lstock_trigger;
 R0 = [SP + 168];
 R1 = [SP + 164];
 R2 = [SP + 160];
 CALL _gin_fw_trigger;
.global _gin_fw_after_trigger
_gin_fw_after_trigger:
 [SP + 168] = R0;
 RESTORE
 OWN_LEAVE trigger, 0
 RTS;
.Lstock_trigger:
 RESTORE
 OWN_LEAVE stock_trigger, 0

 P0.L = 0xde6;
 P0.H = 0x197;
 JUMP (P0);
.global _gin_fw_entry_status
_gin_fw_entry_status:
 OWN_ENTER status
 SAVE
 R0 = [SP + 168];
 CALL _gin_fw_route;
 CC = R0 == 0;
 IF CC JUMP .Lstock_status;
 R0 = [SP + 168];
 R1 = [SP + 164];
 R2 = [SP + 160];
 CALL _gin_fw_status;
.global _gin_fw_after_status
_gin_fw_after_status:
 [SP + 168] = R0;
 RESTORE
 OWN_LEAVE status, 1
 RTS;
.Lstock_status:
 RESTORE
 OWN_LEAVE stock_status, 1

 P0.L = 0xe5c;
 P0.H = 0x197;
 JUMP (P0);
.global _gin_fw_entry_release
_gin_fw_entry_release:
 OWN_ENTER release
 SAVE
 R0 = [SP + 168];
 CALL _gin_fw_route;
 CC = R0 == 0;
 IF CC JUMP .Lstock_release;
 R0 = [SP + 168];
 R1 = [SP + 164];
 R2 = [SP + 160];
 CALL _gin_fw_release;
.global _gin_fw_after_release
_gin_fw_after_release:
 [SP + 168] = R0;
 RESTORE
 OWN_LEAVE release, 2
 RTS;
.Lstock_release:
 RESTORE
 OWN_LEAVE stock_release, 2

 P0.L = 0xe7e;
 P0.H = 0x197;
 JUMP (P0);

.global _gin_fw_entry_type
_gin_fw_entry_type:
 P0 = [SP++];
 SAVE
 R0 = [SP + 168];
 R1 = [SP + 164];
 R2 = [SP + 160];
 R1 = [SP + 176];
 R2 = P5;
 CALL _gin_fw_type_entry;
 [SP + 168] = R0;
 RESTORE
 RTS;

.global _gin_fw_entry_factory_parent
_gin_fw_entry_factory_parent:
 P0 = [SP++];
 SAVE
 R0 = [SP + 168];
 R1 = [SP + 164];
 R2 = [SP + 160];

 CALL _gin_fw_factory_parent;
 [SP + 168] = R0;
 RESTORE
 RTS;

.global _gin_fw_entry_database_open
_gin_fw_entry_database_open:
 P0 = [SP++];
 SAVE
 R0 = [SP + 168];
 R1 = [SP + 164];
 R2 = [SP + 160];

 CALL _gin_fw_database_open;
 [SP + 168] = R0;
 RESTORE
 RTS;

.global _gin_fw_entry_name_compare
_gin_fw_entry_name_compare:
 P0 = [SP++];
 SAVE
 R0 = [SP + 168];
 R1 = [SP + 164];
 R2 = [SP + 160];

 CALL _gin_fw_name_compare;
 [SP + 168] = R0;
 RESTORE
 RTS;



.global _gin_fw_original_type
_gin_fw_original_type:
.include "gin_stock_type.private.inc"
 P0.L = 0x981e;
 P0.H = 0x019b;
 JUMP (P0);
.global _gin_fw_original_database_open
_gin_fw_original_database_open:
.include "gin_stock_database.private.inc"
 P0.L = 0xf6c4;
 P0.H = 0x0196;
 JUMP (P0);

.section .patch_type,"ax"
 [--SP] = P0;
 P0.L = _gin_fw_entry_type;
 P0.H = _gin_fw_entry_type;
 JUMP (P0);
 NOP;

.ifdef GIN_CPU_PROFILE
.section .patch_profile_audio,"ax"
 R1.L = _gin_profile_audio_cb;
 R1.H = _gin_profile_audio_cb;
.endif

.section .patch_database,"ax"
 [--SP] = P0;
 P0.L = _gin_fw_entry_database_open;
 P0.H = _gin_fw_entry_database_open;
 JUMP (P0);
 NOP;
 NOP;

.section .patch_name,"ax"
 [--SP] = P0;
 P0.L = _gin_fw_entry_name_compare;
 P0.H = _gin_fw_entry_name_compare;
 JUMP (P0);

.section .patch_factory,"ax"
 [--SP] = P0;
 P0.L = _gin_fw_entry_factory_parent;
 P0.H = _gin_fw_entry_factory_parent;
 JUMP (P0);
 NOP;


.bss
.align 4
.global _gin_fw_stack_low_trigger
_gin_fw_stack_low_trigger:
 .space 4096
.global _gin_fw_stack_top_trigger
_gin_fw_stack_top_trigger:
 .space 12
.global _gin_fw_stack_low_render
_gin_fw_stack_low_render:
 .space 4096
.global _gin_fw_stack_top_render
_gin_fw_stack_top_render:
 .space 12
.global _gin_fw_stack_low_release
_gin_fw_stack_low_release:
 .space 4096
.global _gin_fw_stack_top_release
_gin_fw_stack_top_release:
 .space 12
.global _gin_fw_stack_low_status
_gin_fw_stack_low_status:
 .space 4096
.global _gin_fw_stack_top_status
_gin_fw_stack_top_status:
 .space 12


.global _gin_fw_stack_bytes
.set _gin_fw_stack_bytes,4096
