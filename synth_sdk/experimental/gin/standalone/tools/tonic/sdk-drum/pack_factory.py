"""Lossless word dictionary for immutable prepared factory records (host only)."""
from pathlib import Path
import struct
import hashlib
import json


def pack(source, destination):
    raw = Path(source).read_bytes()
    if not raw or len(raw) % (24 * 308) or len(raw) > 34 * 24 * 308:
        raise ValueError('Factory prepared geometry')
    words = struct.unpack('<' + 'I' * (len(raw)//4), raw)
    dictionary, lookup, indices = [], {}, []
    for word in words:
        if word not in lookup:
            lookup[word] = len(dictionary)
            dictionary.append(word)
        indices.append(lookup[word])
    if len(dictionary) > 65535:
        raise ValueError('Factory dictionary overflow')
    restored = struct.pack('<' + 'I'*len(words), *(dictionary[i] for i in indices))
    if restored != raw:
        raise ValueError('Factory reconstruction mismatch')
    destination = Path(destination)
    destination.mkdir(exist_ok=True)
    (destination/'factory.dictionary.bin').write_bytes(struct.pack('<'+'I'*len(dictionary), *dictionary))
    (destination/'factory.index.bin').write_bytes(struct.pack('<'+'H'*len(indices), *indices))
    # Current bank has fewer than4096 distinct words. Pack pairs of12-bit
    # dictionary coordinates losslessly; decoding occurs only at kit load.
    packed12 = bytearray()
    if len(dictionary) <= 4096:
        if len(indices) % 2:
            raise ValueError('Factory12 index parity')
        for a, b in zip(indices[::2], indices[1::2]):
            packed12.extend((a & 255, (a >> 8) | ((b & 15) << 4), b >> 4))
        decoded = []
        for a, b, c in zip(packed12[::3], packed12[1::3], packed12[2::3]):
            decoded.extend((a | ((b & 15) << 8), (b >> 4) | (c << 4)))
        if decoded != indices:
            raise ValueError('Factory12 reconstruction mismatch')
        (destination/'factory.index12.bin').write_bytes(packed12)
    receipt = dict(kits=len(raw)//(24*308), sounds=len(raw)//308, prepared_bytes=308, words_per_sound=77,
                   index12_bytes=len(packed12), dictionary_words=len(dictionary), packed_bytes=len(dictionary)*4+len(indices)*2,
                   source_sha256=hashlib.sha256(raw).hexdigest(), lossless=True)
    (destination/'FACTORY_PACK.json').write_text(json.dumps(receipt, indent=2)+'\n')
    return receipt


if __name__ == '__main__':
    import sys
    print(json.dumps(pack(sys.argv[1], sys.argv[2]), indent=2))
