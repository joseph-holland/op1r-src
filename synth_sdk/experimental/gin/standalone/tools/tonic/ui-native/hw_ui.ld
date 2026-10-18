/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */

OUTPUT_FORMAT("elf32-bfin")
ENTRY(_gin_fw_entry_database_open)
EXTERN(_gin_fw_entry_render _gin_fw_entry_trigger _gin_fw_entry_status _gin_fw_entry_release)
_gin_fw_stock_render_entry = 0x01970ee8;
_gin_fw_stock_trigger_entry = 0x01970de6;
_gin_fw_stock_status_entry = 0x01970e5c;
_gin_fw_stock_release_entry = 0x01970e7e;
SECTIONS {
 .patch_type 0x019B9810 : { KEEP(*(.patch_type)) }
 .patch_database 0x0196F6B4 : { KEEP(*(.patch_database)) }
 .patch_name 0x019B975C : { KEEP(*(.patch_name)) }
 .patch_factory 0x01967984 : { KEEP(*(.patch_factory)) }
 .patch_profile_audio 0x0191AB52 : { KEEP(*(.patch_profile_audio)) }
 .patch_ui_ctor 0x01A3BD58 : { KEEP(*(.patch_ui_ctor)) }
 .patch_ui_load 0x019F07D2 : { KEEP(*(.patch_ui_load)) }
 .patch_ui_key 0x019C2D3E : { KEEP(*(.patch_ui_key)) }
 .patch_ui_knob 0x019C2E94 : { KEEP(*(.patch_ui_knob)) }
 .patch_ui_select 0x01A3BFEE : { KEEP(*(.patch_ui_select)) }
 .patch_preset_load 0x01A00630 : { KEEP(*(.patch_preset_load)) }
 .patch_preset_save 0x019FFFBA : { KEEP(*(.patch_preset_save)) }
 .patch_preset_catalog 0x01A0502A : { KEEP(*(.patch_preset_catalog)) }
 .patch_preset_select 0x0193F5C2 : { KEEP(*(.patch_preset_select)) }
 .patch_working_slot_tick 0x019F0A3C : { KEEP(*(.patch_working_slot_tick)) }
 .patch_snapshot_read 0x019AD554 : { KEEP(*(.patch_snapshot_read)) }
 .patch_snapshot_slot_save 0x019AD776 : { KEEP(*(.patch_snapshot_slot_save)) }
 .patch_snapshot_normalize 0x01A037A8 : { KEEP(*(.patch_snapshot_normalize)) }
 . = 0x01E00000;
 .text : { *(.text*) }
 .rodata : { KEEP(*(.rodata.gin_contract)) *(.rodata*) }
 .data : { *(.data*) }
 ASSERT(. <= 0x01E40000, "Gin UI code exceeds pinned SDK namespace")
 . = 0x01EC0000;
 .bss (NOLOAD) : { KEEP(*(.bss.gin_observation)) *(.bss*) *(COMMON) }
 ASSERT(. <= 0x01EF0000, "Gin UI BSS exceeds pinned SDK namespace")
 /DISCARD/ : { *(.comment) *(.note*) }
}
