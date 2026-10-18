"""Read-only exact-#246 archive inspection and application analysis mapping.
No device I/O. No bootloader/OTP writes. No update repacking or patching.
"""
from __future__ import annotations
from dataclasses import dataclass
from pathlib import Path
import hashlib, io, lzma, struct, tarfile, zlib

PACKAGE_SHA256 = 'c5315218f825f143b415ca554516541898abee843d3a236df0b54c04e1fb13a9'
APP_SHA256 = 'f79d299f6f9019c998013a4176223a6a1bc999a2d8c54451346861f6ea455c74'
BOOT_SHA256 = 'f6233b974bdb6c92f39e73401f206721924ef3ce52abd66b642a5350cebcad93'

def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def load_package(path: Path):
    data = path.read_bytes()
    if sha256(data) != PACKAGE_SHA256:
        raise ValueError('Refusing addresses from an image other than exact stock #246')
    crc, = struct.unpack_from('<I', data)
    if crc != zlib.crc32(data[4:]):
        raise ValueError('Package CRC mismatch')
    raw = lzma.decompress(data[4:], format=lzma.FORMAT_ALONE)
    with tarfile.open(fileobj=io.BytesIO(raw), mode='r:') as tf:
        members = tf.getmembers()
        selected = {}
        for name, expected in [('OP1_vdk.ldr', APP_SHA256), ('te-boot.ldr', BOOT_SHA256)]:
            matches = [m for m in members if m.isfile() and Path(m.name).name == name]
            if len(matches) != 1:
                raise ValueError(f'Ambiguous archive member: {name}')
            stream = tf.extractfile(matches[0])
            if stream is None:
                raise ValueError(f'Unreadable archive member: {name}')
            content = stream.read()
            if sha256(content) != expected:
                raise ValueError(f'Hash mismatch: {name}')
            selected[name] = content
    # The bootloader is hashed in memory and is not written to a file.
    info = {'package_sha256': sha256(data), 'package_bytes': len(data),
            'crc32': f'0x{crc:08x}', 'tar_bytes': len(raw), 'archive_members': len(members),
            'application_sha256': sha256(selected['OP1_vdk.ldr']),
            'bootloader_sha256': sha256(selected['te-boot.ldr']),
            'bootloader_written': False,
            'source_files': [m.name for m in members if Path(m.name).suffix.lower() in ('.c','.cpp','.h')]}
    return Image(selected['OP1_vdk.ldr']), info

@dataclass(frozen=True)
class Block:
    index: int
    offset: int
    flags: int
    address: int
    size: int
    argument: int
    data_offset: int
    stored: int

class Image:
    def __init__(self, data: bytes):
        self.data = data
        self.blocks = []
        off = 0
        while off < len(data):
            raw = data[off:off+16]
            if len(raw) != 16:
                raise ValueError(f'Truncated loader header at {off:x}')
            flags, checksum, magic, address, size, argument = struct.unpack('<HBBIII', raw)
            xor = 0
            for x in raw:
                xor ^= x
            if magic != 0xad or xor:
                raise ValueError(f'Invalid loader header at {off:x}')
            stored = 0 if flags & 0x100 else size
            if off + 16 + stored > len(data):
                raise ValueError('Truncated loader payload')
            self.blocks.append(Block(len(self.blocks),off,flags,address,size,argument,off+16,stored))
            off += 16 + stored

    def owner(self, address: int, size: int = 1) -> Block:
        candidates = [b for b in self.blocks if b.size and not b.flags & 0x1000
                      and b.address <= address and address + size <= b.address + b.size]
        if size < 0 or len(candidates) != 1:
            raise ValueError(f'Non-unique/unmapped range 0x{address:08x}+0x{size:x}')
        return candidates[0]

    def read(self, address: int, size: int) -> bytes:
        b = self.owner(address, size)
        rel = address - b.address
        if b.flags & 0x100:
            pattern = struct.pack('<I', b.argument)
            return bytes(pattern[(rel+i) % 4] for i in range(size))
        return self.data[b.data_offset+rel:b.data_offset+rel+size]

    def u32(self, address: int) -> int:
        return struct.unpack('<I', self.read(address,4))[0]

    def provenance(self, address: int, size: int) -> dict:
        b = self.owner(address,size)
        return {'address': f'0x{address:08x}', 'bytes': size, 'block_index': b.index,
                'stock_flags': f'0x{b.flags:04x}',
                'ldr_offset': b.data_offset + address-b.address if b.stored else None,
                'protected_payload': b.flags & 0x2400 == 0x2400,
                'zero_fill': bool(b.flags & 0x100)}

    def cstring(self, address: int) -> str:
        b = self.owner(address)
        s = self.read(address,min(512,b.address+b.size-address)).split(b'\0',1)[0]
        return s.decode('ascii',errors='backslashreplace')
