#!/usr/bin/env python3
"""Bounded host checks for the draft drum-synth interface. No firmware needed."""
import ast
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

SDK = Path(__file__).resolve().parents[1]


def main():
    cc = shutil.which('clang') or shutil.which('cc')
    if not cc:
        raise SystemExit('A C compiler is required')
    with tempfile.TemporaryDirectory(prefix='op1-drumsynth-') as tmp:
        tmp = Path(tmp)
        common = [cc, '-std=c99', '-Wall', '-Wextra', '-Werror', '-Wno-unknown-pragmas',
                  '-UNDEBUG', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-g']
        for name, files in [
            ('registry-envelope', ['harness/drumsynth_test.c', 'drumsynth/ds_registry.c', 'drumsynth/ds_envelope.c']),
            ('file-parser', ['harness/drumsynth_file_test.c', 'plugload/op1p_load.c']),
            ('choke-groups', ['harness/drumsynth_choke_test.c', 'drumsynth/ds_choke.c']),
        ]:
            exe = tmp / name
            subprocess.run(common + [str(SDK / f) for f in files] + ['-o', str(exe)], check=True, timeout=30)
            subprocess.run([str(exe)], check=True, timeout=30)

        # Execute the actual builder's default-denial path without importing
        # optional ELF tooling or starting Docker. Removing the guard reaches
        # unavailable build machinery and fails this check.
        tree = ast.parse((SDK / 'make_plugin.py').read_text())
        build = next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == 'build')
        scope = {'OUT': str(tmp / 'no-build'), 'KINDS': {'drum_synth': 6},
                 'find': lambda name: (6, {})}
        exec(compile(ast.Module(body=[build], type_ignores=[]), 'make_plugin.py', 'exec'), scope)
        try:
            scope['build']('draft')
        except SystemExit as e:
            if 'host-only draft' not in str(e):
                raise
        else:
            raise RuntimeError('Draft plugin build was not refused')
        if (tmp / 'no-build').exists():
            raise RuntimeError('Refused builder created output')

        spec = importlib.util.spec_from_file_location('ds_discovery', SDK / 'plugins.py')
        discovery = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(discovery)
        folder = tmp / 'plugins'
        for name, kind in [('tone', 'synth'), ('echo', 'fx'), ('pads', 'drum'), ('kicklab', 'drum_synth')]:
            d = folder / name
            d.mkdir(parents=True)
            (d / 'plugin.json').write_text(json.dumps({'name': name, 'kind': kind, 'symbol': 'TEST', 'sources': ['test.c']}))
        discovery.DIR = str(folder)
        engines, effects = discovery.load()
        if [e['name'] for e in engines] != ['tone'] or [e['name'] for e in effects] != ['echo']:
            raise RuntimeError('Legacy two-tuple discovery changed')
        if [e['name'] for e in discovery.load_seqs('drum')] != ['pads']:
            raise RuntimeError('Sample drum discovery changed')
        if [e['name'] for e in discovery.load_seqs('drum_synth')] != ['kicklab']:
            raise RuntimeError('Synthesised drum discovery failed')
    print('discovery compatibility and default builder refusal: PASS')


if __name__ == '__main__':
    main()
