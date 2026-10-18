#!/usr/bin/env python3
"""Check the draft voice adapter against a caller-supplied GIN source tree.

No firmware, private fonts or emulator profile is read. GIN DSP/kit sources are
not bundled with this SDK draft. --gin-tree must contain src/tonic and the MIT
libpo32 subset under third_party/libpo32/core.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import time

SDK = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--gin-tree', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    tree = args.gin_tree.resolve(strict=True)
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    g = tree / 'src/tonic'
    po = tree / 'third_party/libpo32/core'
    cc = shutil.which('clang') or shutil.which('cc')
    if not cc:
        raise SystemExit('A C compiler is required')
    sources = [SDK/'harness/gin_voice_test.c', SDK/'experimental/gin/gin_voice.c',
               SDK/'drumsynth/ds_envelope.c', SDK/'drumsynth/ds_choke.c',
               g/'integer/gin_dsp.c', g/'integer/gin_prepare.c',
               g/'sdk_drum/gin_resample.c', g/'tonic_voice.c',
               g/'presets/gin_factory.c', g/'factory_bank/gin_factory_bank.c',
               po/'src/po32_synth.c']
    for source in sources:
        if not source.is_file():
            raise SystemExit(f'Missing source: {source.name}')
    flags = ['-std=c99', '-O1', '-g', '-UNDEBUG', '-DGIN_FAST_FIXED=1',
             '-DGIN_BLOCK_STATE=1', '-DGIN_SINE_QUARTER=0', '-Wall', '-Wextra',
             '-Werror', '-Wno-misleading-indentation', '-fsanitize=address,undefined',
             '-fno-omit-frame-pointer']
    includes = [SDK/'experimental/gin', g/'integer', g/'sdk_drum', g/'presets',
                g, po/'include', po/'src']
    command = [cc, *flags, *[f'-I{p}' for p in includes], *map(str, sources),
               '-lm', '-o', str(out/'gin-voice-test')]
    started = time.monotonic()
    built = subprocess.run(command, capture_output=True, text=True, timeout=40)
    result = {'scope': 'Host adapter checks; no native loading, emulator or hardware evidence',
              'compile_flags': flags, 'compiler': subprocess.check_output([cc, '--version'], text=True).splitlines()[0],
              'build_exit': built.returncode, 'build_stderr': built.stderr,
              'sources_sha256': {str(p.relative_to(SDK) if p.is_relative_to(SDK) else p.relative_to(tree)):
                                 hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}}
    if not built.returncode:
        run = subprocess.run([str(out/'gin-voice-test')], capture_output=True, text=True, timeout=50)
        result.update(test_exit=run.returncode, test_stdout=run.stdout, test_stderr=run.stderr,
                      executable_sha256=hashlib.sha256((out/'gin-voice-test').read_bytes()).hexdigest())
    result['elapsed_seconds'] = time.monotonic() - started
    (out/'RESULT.json').write_text(json.dumps(result, indent=2)+'\n')
    print(built.stderr, end='')
    print(result.get('test_stdout', ''), end='')
    print(result.get('test_stderr', ''), end='')
    raise SystemExit(built.returncode or result.get('test_exit', 1))

if __name__ == '__main__':
    main()
