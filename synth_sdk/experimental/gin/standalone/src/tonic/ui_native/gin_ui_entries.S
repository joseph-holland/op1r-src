/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */


.text
.align 2
.macro UI_CALL name,target,after
.global \name
\name:
 SAVE
 R0 = [SP + 168];
 R1 = [SP + 164];
 R2 = [SP + 160];
 CALL \target;
.global \after
\after:
 RESTORE
 RTS;
.endm
UI_CALL _gin_ui_entry_ctor,_gin_fw_ui_ctor,_gin_ui_after_ctor
UI_CALL _gin_ui_entry_load,_gin_fw_ui_load,_gin_ui_after_load
UI_CALL _gin_ui_entry_key,_gin_fw_ui_key,_gin_ui_after_key
UI_CALL _gin_ui_entry_knob,_gin_fw_ui_knob,_gin_ui_after_knob
UI_CALL _gin_ui_entry_update,_gin_screen_update,_gin_ui_after_update
.global _gin_ui_entry_select
_gin_ui_entry_select:
 SAVE
 CALL _gin_fw_ui_select;
.global _gin_ui_after_select
_gin_ui_after_select:
 [SP + 168] = R0;
 RESTORE

 P0 = [FP + 4];
 UNLINK;
 JUMP (P0);
.section .patch_ui_ctor,"ax"
 CALL _gin_ui_entry_ctor;
.section .patch_ui_load,"ax"
 CALL _gin_ui_entry_load;
.section .patch_ui_key,"ax"
 CALL _gin_ui_entry_key;
.section .patch_ui_knob,"ax"
 CALL _gin_ui_entry_knob;
.section .patch_ui_select,"ax"
 JUMP.L _gin_ui_entry_select;
 NOP;
