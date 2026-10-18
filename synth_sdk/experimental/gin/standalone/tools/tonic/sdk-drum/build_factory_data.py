"""Build and losslessly pack all active Gin factory coefficient sets."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
from pack_factory import pack

ROOT = Path(__file__).resolve().parents[3]


def build(out, cc='cc'):
    out = Path(out).resolve()
    out.mkdir(parents=True, exist_ok=False)
    sources = ['tools/tonic/sdk-drum/generate_factory.c', 'src/tonic/presets/gin_factory.c',
               'src/tonic/factory_bank/gin_factory_bank.c', 'src/tonic/integer/gin_prepare.c',
               'src/tonic/tonic_voice.c', 'third_party/libpo32/core/src/po32_synth.c']
    includes = ['src/tonic/presets', 'src/tonic/integer', 'src/tonic',
                'third_party/libpo32/core/include', 'third_party/libpo32/core/src']
    command = [cc, '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror', '-Wno-misleading-indentation']
    command += ['-I'+p for p in includes] + sources + ['-lm', '-o', str(out/'generator')]
    compilation = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=60)
    execution = None
    if compilation.returncode == 0:
        execution = subprocess.run([str(out/'generator')], cwd=out, capture_output=True, text=True, timeout=30)
    result = dict(command=command, build=dict(code=compilation.returncode, stdout=compilation.stdout,
                  stderr=compilation.stderr), execution=None if execution is None else dict(
                  code=execution.returncode, stdout=execution.stdout, stderr=execution.stderr),
                  source_sha256={p: hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in sources})
    (out/'GENERATION.json').write_text(json.dumps(result, indent=2)+'\n')
    if compilation.returncode or execution.returncode:
        raise RuntimeError('Factory data generation failed; see GENERATION.json')
    return pack(out/'factory.prepared.bin', out)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', required=True)
    parser.add_argument('--cc', default='cc')
    args = parser.parse_args()
    print(json.dumps(build(args.out, args.cc), indent=2))
