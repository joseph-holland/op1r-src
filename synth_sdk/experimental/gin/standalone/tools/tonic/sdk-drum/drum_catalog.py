"""SDK-style factory catalog registration for separately bound Drum plugins.

These eight words are native Drum working defaults. Full plugin sound state
belongs to the plugin's kit format, independently of this browser metadata.
"""
import json
import re
import sqlite3
from contextlib import closing

BUILTIN = {'fm', 'digital', 'drwave', 'string', 'cluster', 'pulse', 'sampler',
           'phase', 'drum', 'dbox', 'ls', 'dna', 'dsynth', 'voltage', 'iter'}


def register_drums(connection, plugins):
    """Add missing browser rows; preserve existing rows and every other table.

    The caller owns the transaction. DB IDs are allocated from the catalog,
    independently of constructor families, plugin order and logical sound IDs.
    """
    entries = []
    for plugin in plugins:
        name = plugin['name']
        defaults = plugin['drum_defaults']
        if (not isinstance(name, str) or not re.fullmatch(r'[a-z][a-z0-9_]{0,14}', name)
                or name in BUILTIN or name in {n for n, _ in entries}):
            raise ValueError('Invalid or duplicate Drum plugin name')
        if (not isinstance(defaults, list) or len(defaults) != 8
                or any(type(v) is not int or not -32768 <= v <= 32767 for v in defaults)):
            raise ValueError('Drum defaults require eight signed 16-bit native words')
        entries.append((name, json.dumps(defaults, separators=(',', ':'))))
    columns = [r[1] for r in connection.execute('PRAGMA table_info(drum_types)')]
    if columns != ['id', 'type', 'default_params']:
        raise ValueError('Unsupported Drum catalog schema')
    before = connection.execute('SELECT id,type,default_params FROM drum_types ORDER BY id').fetchall()
    added = []
    for name, defaults in entries:
        if connection.execute('SELECT 1 FROM drum_types WHERE type=?', (name,)).fetchone():
            continue
        row_id = connection.execute('SELECT COALESCE(MAX(id),-1)+1 FROM drum_types').fetchone()[0]
        connection.execute('INSERT INTO drum_types(id,type,default_params) VALUES(?,?,?)',
                           (row_id, name, defaults))
        added.append((row_id, name, defaults))
    after = connection.execute('SELECT id,type,default_params FROM drum_types ORDER BY id').fetchall()
    if after != sorted(before + added):
        raise ValueError('Unexpected catalog mutation')
    return added


def patch_factory_bytes(data, plugins):
    """Modify an isolated serialized database; never open a device or stock file."""
    with closing(sqlite3.connect(':memory:')) as connection:
        connection.deserialize(data)
        if connection.execute('PRAGMA integrity_check').fetchone() != ('ok',):
            raise ValueError('Factory database integrity')
        before = list(connection.iterdump())
        added = register_drums(connection, plugins)
        connection.commit()
        after = list(connection.iterdump())
        unrelated = lambda rows: [s for s in rows if not s.startswith('INSERT INTO "drum_types"')]
        if unrelated(before) != unrelated(after):
            raise ValueError('Factory changes outside Drum catalog rows')
        if connection.execute('PRAGMA integrity_check').fetchone() != ('ok',):
            raise ValueError('Patched factory database integrity')
        return connection.serialize(), added


def patch_gin_presets(data, bank):
    """Own GIN category; immutable bank reference, not compressed sound state.

    Native firmware expands each reference into its complete owned 24-key kit.
    The existing native common defaults come from a synthesized Drum template.
    No sample resource or third-party sound parameters are copied into Gin.
    """
    kits = bank['kits']
    if (not 1 <= len(kits) <= 34 or [k['order'] for k in kits] != list(range(len(kits)))
            or len({k['name'] for k in kits}) != len(kits)):
        raise ValueError('Gin bank identity/order')
    with closing(sqlite3.connect(':memory:')) as connection:
        connection.deserialize(data)
        before = list(connection.iterdump())
        original = connection.execute('SELECT id,patch,folder FROM drum_presets ORDER BY id').fetchall()
        if len(original) != 24:
            raise ValueError('Expected exact-stock 24 factory presets')
        templates = [json.loads(p) for _, p, _ in original if json.loads(p).get('type') == 'dbox']
        if len(templates) != 10 or any(f == 'gin' for _, _, f in original):
            raise ValueError('Unexpected factory composition/category')
        common = ('octave', 'synth_version', 'drum_version', 'dyna_env',
                  'fx_active', 'fx_type', 'fx_params', 'lfo_active', 'lfo_type', 'lfo_params')
        template = {k: templates[0][k] for k in common if k in templates[0]}
        if not all(k in template for k in ('dyna_env', 'fx_active', 'fx_type', 'fx_params',
                                          'lfo_active', 'lfo_type', 'lfo_params', 'octave')):
            raise ValueError('Native common defaults missing')
        defaults = json.loads(connection.execute("SELECT default_params FROM drum_types WHERE type='dbox'").fetchone()[0])
        if len(defaults) != 8 or any(type(v) is not int for v in defaults):
            raise ValueError('Native synthesized Drum defaults')
        defaults[0] = 0  # Native global pitch stays neutral.
        added = []
        next_id = max(r[0] for r in original) + 1
        for i, kit in enumerate(kits):
            if (not isinstance(kit['name'], str) or not 1 <= len(kit['name']) <= 23
                    or any(not 32 <= ord(c) <= 126 for c in kit['name'])
                    or len(kit['sounds']) != 24 or len(kit['key_map']) != 24):
                raise ValueError('Gin kit shape/name')
            words = defaults.copy()
            words[7] = 4096 + i
            # Native opfont lowercase IDs draw letters; uppercase IDs are icons.
            patch = dict(template, type='gin', name=kit['name'].lower(), dbox_data=[words.copy() for _ in range(24)])
            encoded = json.dumps(patch, separators=(',', ':'))
            connection.execute('INSERT INTO drum_presets(id,patch,folder) VALUES(?,?,?)',
                               (next_id + i, encoded, 'gin'))
            added.append((next_id + i, kit['name'], i))
        connection.commit()
        after = list(connection.iterdump())
        unrelated = lambda rows: [s for s in rows if not s.startswith('INSERT INTO "drum_presets"')]
        if unrelated(before) != unrelated(after):
            raise ValueError('Changes outside Gin preset rows')
        if connection.execute('SELECT id,patch,folder FROM drum_presets WHERE id<? ORDER BY id',
                              (next_id,)).fetchall() != original:
            raise ValueError('Stock preset mutation')
        if connection.execute('PRAGMA integrity_check').fetchone() != ('ok',):
            raise ValueError('Factory database integrity')
        return connection.serialize(), added
