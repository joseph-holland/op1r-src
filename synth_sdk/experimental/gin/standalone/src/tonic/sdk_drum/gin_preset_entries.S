/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */


.section .patch_preset_load,"ax"
 CALL _gin_fw_json_dbox;
.section .patch_preset_save,"ax"
 CALL _gin_fw_json_dbox;
.section .patch_preset_catalog,"ax"
 CALL _gin_fw_factory_preset_query;
.section .patch_preset_select,"ax"
 CALL _gin_fw_browser_preset_copy;


.section .patch_snapshot_read,"ax"
 CALL _gin_fw_snapshot_read_entry;
.section .patch_snapshot_slot_save,"ax"
 CALL _gin_fw_snapshot_slot_save;

.section .patch_snapshot_normalize,"ax"
 JUMP.L _gin_fw_normalize;
.text
.align 2
.global _gin_fw_original_normalize
_gin_fw_original_normalize:
.include "gin_stock_normalize.private.inc"
 P0.L = 0x37ac;
 P0.H = 0x01a0;
 JUMP (P0);

.text
.align 2
.global _gin_fw_snapshot_read_abort
.set _gin_fw_snapshot_read_abort,0x019ad8f4
.global _gin_fw_snapshot_read_entry
_gin_fw_snapshot_read_entry:
 [--SP] = RETS;
 SP += -12;
 CALL _gin_fw_snapshot_read;
 SP += 12;
 RETS = [SP++];
 CC = R0 == 0;
 IF CC JUMP .Lgin_snapshot_read_ok;


 P0.L = 0xd8f4;
 P0.H = 0x019a;
 JUMP (P0);
.Lgin_snapshot_read_ok:
 RTS;



.ifdef GIN_WORKING_SLOTS
.if GIN_WORKING_SLOTS
.section .patch_working_slot_tick,"ax"
 CALL _gin_fw_working_tick;
.endif
.endif
