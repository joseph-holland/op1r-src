#!/bin/bash
set -euo pipefail
cd /out
export PATH=/workspace/.toolchain/bin:$PATH
flags='-DGIN_FACTORY_BANK -DGIN_FAST_FIXED=1 -DGIN_NATIVE_SNAPSHOTS -DGIN_AUDIO_OBSERVATION=0 -std=c99 -O2 -mcpu=bf524 -mno-fdpic -ffreestanding -fno-builtin -nostdlib -fno-common -fno-tree-loop-distribute-patterns -ffunction-sections -fdata-sections -Wall -Wextra -Werror -Wno-misleading-indentation -fstack-usage'
working_slots=${GIN_WORKING_SLOTS:-1}
[[ $working_slots == 0 || $working_slots == 1 ]]
flags="$flags -DGIN_WORKING_SLOTS=$working_slots"
profile=${GIN_PROFILE_ENABLED:-0}
[[ $profile == 0 || $profile == 1 ]]
flags="$flags -DGIN_PROFILE_ENABLED=$profile"
profile_object=()
profile_asm=()
if [[ $profile == 1 ]]; then
 flags="$flags -DGIN_PROFILE_DIAGNOSTIC"
 profile_object=(gin_profile.o)
 profile_asm=(--defsym GIN_CPU_PROFILE=1)
else
 [[ ${GIN_LEVEL_TRACE:-0} == 0 && ${GIN_SERVICE_TRACE:-0} == 0 && ${GIN_STOCK_PROTECTED_CONTROL:-0} == 0 ]]
fi
flags="$flags -DGIN_PARAMS_ONLY=${GIN_PARAMS_ONLY:?variant required}"
flags="$flags -DGIN_FW_OUTPUT_SHIFT=${GIN_FW_OUTPUT_SHIFT:-14}"
flags="$flags -DGIN_SINE_QUARTER=${GIN_SINE_QUARTER:-0}"
flags="$flags -DGIN_LEVEL_TRACE=${GIN_LEVEL_TRACE:-0}"
flags="$flags -DGIN_SERVICE_TRACE=${GIN_SERVICE_TRACE:-0}"
flags="$flags -DGIN_STOCK_PROTECTED_CONTROL=${GIN_STOCK_PROTECTED_CONTROL:-0}"
flags="$flags -DGIN_SYNTH_VOICE_LIMIT=${GIN_SYNTH_VOICE_LIMIT:-4}"
flags="$flags -DGIN_RATE_2X=${GIN_RATE_2X:-0} -DGIN_BLOCK_STATE=${GIN_BLOCK_STATE:-0} -DGIN_FACTORY_PACKED_INDEX=${GIN_FACTORY_PACKED_INDEX:-0}"
inc='-I/scene/freestanding -I/source/synth_sdk -I/source/synth_sdk/plugins/gin -I/owned -I/ui -I/scene -I/integer -I/tonic -I/presets -I/po/include -I/po/src -I/font'
for n in gin_adapter gin_dsp;do bfin-elf-gcc $flags $inc -c /source/synth_sdk/plugins/gin/$n.c -o $n.o;done
for n in drum_dispatcher drum_module_table drum_name_bridge drum_ui drum_screen_table;do bfin-elf-gcc $flags $inc -c /source/synth_sdk/$n.c -o $n.o;done
bfin-elf-gcc $flags -O3 $inc -c /owned/gin_resample.c -o gin_resample.o
bfin-elf-gcc $flags $inc -c /owned/gin_fw.c -o gin_fw.o
bfin-elf-gcc $flags $inc -c /owned/gin_fw_presets.c -o gin_fw_presets.o
bfin-elf-gcc $flags -Os $inc -c /owned/gin_fw_snapshot.c -o gin_fw_snapshot.o
bfin-elf-gcc $flags -Os $inc -c /presets/gin_snapshot_text.c -o gin_snapshot_text.o
bfin-elf-gcc $flags $inc -c /tonic/presets/gin_factory.c -o gin_factory.o
bfin-elf-gcc $flags $inc -c /tonic/factory_bank/gin_factory_bank.c -o gin_factory_bank.o
if [[ $profile == 1 ]]; then bfin-elf-gcc $flags $inc -c /owned/gin_profile.c -o gin_profile.o; fi
# The expanded readable GUI is size-optimised; audio/preparation flags stay unchanged.
bfin-elf-gcc $flags -Os $inc -c /scene/gin_screen.c -o gin_screen.o
for n in gin_text gin_controls gin_target_lib;do bfin-elf-gcc $flags $inc -c /scene/$n.c -o $n.o;done
for n in gin_ui gin_name;do bfin-elf-gcc $flags $inc -c /ui/$n.c -o $n.o;done
bfin-elf-gcc $flags $inc -c /integer/gin_prepare.c -o gin_prepare.o
bfin-elf-gcc $flags $inc -c /tonic/tonic_voice.c -o tonic_voice.o
bfin-elf-gcc $flags $inc -c /po/src/po32_synth.c -o po32_synth.o
cat /owned/gin_fw_entries.S /scene/gin_ui_entries.S /owned/gin_preset_entries.S > all_entries.S
bfin-elf-as -mcpu=bf524 -I /font --defsym GIN_WORKING_SLOTS=$working_slots "${profile_asm[@]}" --defsym GIN_STOCK_PROTECTED_CONTROL=${GIN_STOCK_PROTECTED_CONTROL:-0} all_entries.S -o gin_fw_entries.o
cat > builtin.S <<'ASM'
.section .rodata
.balign 4
.global _gin_fw_builtin_kit
_gin_fw_builtin_kit:
.incbin "/data/builtin.kit.bin"
.balign 4
.global _gin_fw_builtin_prepared
_gin_fw_builtin_prepared:
.incbin "/data/builtin.prepared.bin"
.balign 4
.global _gin_fw_builtin_patch
_gin_fw_builtin_patch:
.incbin "/data/builtin.patch.bin"
.balign 4
.global _gin_factory_prepared_dictionary
_gin_factory_prepared_dictionary:
.incbin "/factory-data/factory.dictionary.bin"
.balign 2
.global _gin_factory_prepared_index
_gin_factory_prepared_index:
.incbin "/factory-data/factory.index.bin"
ASM
if [[ ${GIN_FACTORY_PACKED_INDEX:-0} == 1 ]]; then
 test "$(wc -c < /factory-data/factory.dictionary.bin)" -le 16384
 test "$(wc -c < /factory-data/factory.index12.bin)" -eq "$(( $(wc -c < /factory-data/factory.index.bin) * 3 / 4 ))"
 sed -i s/factory.index.bin/factory.index12.bin/ builtin.S
fi
bfin-elf-as -mcpu=bf524 builtin.S -o builtin.o
libgcc=$(bfin-elf-gcc -print-libgcc-file-name)
sha256sum "$libgcc" > libgcc.sha256
bfin-elf-ld --gc-sections -Map=payload.map -T /ui-tools/hw_ui.ld drum_dispatcher.o drum_module_table.o drum_name_bridge.o drum_ui.o drum_screen_table.o gin_adapter.o gin_dsp.o gin_resample.o gin_fw.o gin_fw_presets.o gin_fw_snapshot.o gin_snapshot_text.o gin_factory.o gin_factory_bank.o "${profile_object[@]}" gin_fw_entries.o gin_screen.o gin_text.o gin_controls.o gin_target_lib.o gin_ui.o gin_name.o gin_prepare.o tonic_voice.o po32_synth.o builtin.o "$libgcc" -o gin_fw_candidate.elf
bfin-elf-nm -u gin_fw_candidate.elf > unresolved.txt
test ! -s unresolved.txt
bfin-elf-nm -S -n gin_fw_candidate.elf > symbols.txt
bfin-elf-readelf -h -l -S gin_fw_candidate.elf > layout.txt
bfin-elf-objdump -dr gin_fw_candidate.elf > disassembly.txt
bfin-elf-size -A gin_fw_candidate.elf > sections.txt
sha256sum *.o gin_fw_candidate.elf > objects.sha256
