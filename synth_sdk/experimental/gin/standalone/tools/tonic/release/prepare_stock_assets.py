#!/usr/bin/env python3
"""Generate required stock-derived build inputs locally from exact OP-1 #246.

No stock bytes are included in this tool. Generated outputs are private inputs,
not release assets. Requires Node.js, playwright and its Chromium browser.
Does not patch firmware or communicate with hardware.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path
import struct
import subprocess
import tarfile
import lzma
import xml.etree.ElementTree as ET
import zlib
from font_geometry import generate

PACKAGE_SHA = 'c5315218f825f143b415ca554516541898abee843d3a236df0b54c04e1fb13a9'
APP_SHA = 'f79d299f6f9019c998013a4176223a6a1bc999a2d8c54451346861f6ea455c74'
FONT_SHA = '6969944280492f8a6f35ac1f19c9d011ee858768838b6f71b832c6a6dbd214f2'
HEADER_SHA = 'ebfa0a7125a206cbfc281b1b742d355864e5e8a921d683b5824157cc35b54241'
PREFIX_INCLUDE_SHA = {'type':'4a313e5608cebdc162326842e03fbdfab8ee3d6e8210d3fd9f5547fbf7586781','database':'b55faf62fecbf25e0cd0f8d55e7829d8ff40e40a5884fbfe058e484d7d895ee2','normalize':'2ec4d6d99c1fafbeb22037a06a79ef45f7f94905a330065e1c1f8b4e0d9ee37f'}
PREFIXES = {'type': (0x019b9810,14), 'database': (0x0196f6b4,16), 'normalize': (0x01a037a8,4)}
sha = lambda b: hashlib.sha256(b).hexdigest()

def qualified_package(path):
    data = Path(path).read_bytes()
    if sha(data) != PACKAGE_SHA: raise ValueError('Exact stock #246 package required')
    if struct.unpack_from('<I',data)[0] != zlib.crc32(data[4:]): raise ValueError('Package CRC')
    archive = tarfile.open(fileobj=io.BytesIO(lzma.decompress(data[4:],format=lzma.FORMAT_ALONE)))
    def member(name):
        found = [m for m in archive.getmembers() if m.isfile() and m.name == name]
        if len(found) != 1: raise ValueError('Missing or ambiguous '+name)
        return archive.extractfile(found[0]).read()
    font, app = member('content/display/opfont.svg'), member('OP1_vdk.ldr')
    if sha(font) != FONT_SHA or sha(app) != APP_SHA: raise ValueError('Resource identity')
    return font, app

def image_read(app,address,size):
    off = 0; found = []
    while off < len(app):
        header = app[off:off+16]
        if len(header) != 16: raise ValueError('LDR header')
        flags, _, magic, base, count, argument = struct.unpack('<HBBIII', header)
        xor = 0
        for byte in header: xor ^= byte
        stored = 0 if flags & 0x100 else count
        if magic != 0xad or xor or off+16+stored>len(app): raise ValueError('LDR geometry')
        if not flags & 0x1000 and base <= address and address+size <= base+count:
            if flags & 0x100: raise ValueError('Prefix in fill block')
            found.append(app[off+16+address-base:off+16+address-base+size])
        off += 16+stored
    if len(found) != 1: raise ValueError('Prefix ownership')
    return found[0]

def glyph_nodes(font, boxes):
    root = ET.fromstring(font); nodes = {n.get('id'):n for n in root if n.get('id')}
    attrs = {'d','points','x','y','width','height','cx','cy','r','rx','ry','x1','x2','y1','y2','fill','stroke','stroke-width','stroke-linecap','stroke-linejoin'}
    def convert(n):
        tag = n.tag.rsplit('}',1)[-1]
        if tag not in {'g','path','line','polyline','polygon','circle','ellipse','rect'} or n.get('transform') is not None:
            raise ValueError('Unsupported glyph node')
        return {'t':tag,'a':{k:v for k,v in n.attrib.items() if k in attrs},'children':[convert(c) for c in n if c.get('stroke')!='#FFFF00' and c.get('fill')!='#FFFF00']}
    glyphs = {}
    for char in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+-%.:':
        key = char.lower() if char.isalpha() else f'_x{ord(char):02X}_'
        if key not in nodes: key = char
        glyphs[char] = {'box':boxes[key], 'node':convert(nodes[key])}
    return {'glyphs':glyphs}

def build(stock, out, node='node', chromium=None):
    font,app = qualified_package(stock)
    out = Path(out); out.mkdir(parents=True,exist_ok=False)
    svg = out/'opfont.private.svg'; svg.write_bytes(font)
    command = [node,str(Path(__file__).with_name('font_bounds.cjs')),str(svg),str(out/'bounds.private.json')]
    if chromium: command.append(chromium)
    subprocess.run(command,check=True,timeout=45)
    header = generate(glyph_nodes(font,json.loads((out/'bounds.private.json').read_text())))
    if sha(header) != HEADER_SHA: raise ValueError('Font output does not match accepted header')
    (out/'gin_font.private.inc').write_bytes(header)
    receipts = {'gin_font.private.inc':sha(header)}
    for name,(address,size) in PREFIXES.items():
        raw = image_read(app,address,size)
        include = '.byte '+','.join('0x%02x'%b for b in raw)+'\n'
        if sha(include.encode()) != PREFIX_INCLUDE_SHA[name]: raise ValueError('Stock prefix identity')
        target = out/('gin_stock_'+name+'.private.inc'); target.write_text(include)
        receipts[target.name] = sha(target.read_bytes())
    (out/'ASSETS.private.json').write_text(json.dumps({'package_sha256':PACKAGE_SHA,'generated_sha256':receipts},indent=2)+'\n')
    return receipts

if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('stock');p.add_argument('out');p.add_argument('--node',default='node');p.add_argument('--chromium');a=p.parse_args()
    print(json.dumps(build(a.stock,a.out,a.node,a.chromium),indent=2))
