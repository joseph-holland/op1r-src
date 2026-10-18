#!/usr/bin/env python3
"""Build the separately pinned Gin bootstrap kit data from corresponding source."""
import argparse
from pathlib import Path
import subprocess

def build(root,out,cc='cc'):
    root,out=Path(root).resolve(),Path(out).resolve();out.mkdir(parents=True,exist_ok=False)
    includes=['src/tonic/presets','src/tonic/factory_bank','src/tonic/integer','src/tonic','third_party/libpo32/core/include','third_party/libpo32/core/src']
    sources=['tools/tonic/sdk-drum/generate_builtin.c','src/tonic/presets/gin_factory.c','src/tonic/factory_bank/gin_factory_bank.c','src/tonic/integer/gin_prepare.c','src/tonic/tonic_voice.c','third_party/libpo32/core/src/po32_synth.c']
    command=[cc,'-std=c99','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation']+['-I'+p for p in includes]+sources+['-lm','-o',str(out/'generator')]
    subprocess.run(command,cwd=root,check=True,timeout=60)
    subprocess.run([str(out/'generator')],cwd=out,check=True,timeout=30)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('root');p.add_argument('out');p.add_argument('--cc',default='cc');a=p.parse_args();build(a.root,a.out,a.cc)
