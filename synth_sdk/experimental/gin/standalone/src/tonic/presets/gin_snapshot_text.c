/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */
#include "gin_snapshot_text.h"
#define SOUND_BYTES 115u
static const char alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static void copy(void *out, const void *in, unsigned n) {
  unsigned char *a = out;
  const unsigned char *b = in;
  while (n--)
    *a++ = *b++;
}
static uint32_t bits(const float *p) {
  uint32_t v;
  copy(&v, p, 4);
  return v;
}
static uint32_t get32(const unsigned char *p) {
  return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 |
         (uint32_t)p[3] << 24;
}
static uint32_t get16(const unsigned char *p) {
  return (uint32_t)p[0] | (uint32_t)p[1] << 8;
}
static void put32(unsigned char *p, uint32_t v) {
  unsigned i;
  for (i = 0; i < 4; ++i)
    p[i] = (unsigned char)(v >> (i * 8));
}
static void put16(unsigned char *p, uint32_t v) {
  p[0] = (unsigned char)v;
  p[1] = (unsigned char)(v >> 8);
}
static uint32_t crc(const unsigned char *p, unsigned n) {
  uint32_t v = ~0u;
  while (n--) {
    unsigned b;
    v ^= *p++;
    for (b = 0; b < 8; ++b)
      v = (v >> 1) ^ ((0u - (v & 1u)) & 0xedb88320u);
  }
  return ~v;
}

static int overlaps(const void *left, uint32_t ln, const void *right,
                    uint32_t rn) {
  uintptr_t a = (uintptr_t)left, b = (uintptr_t)right;
  return a <= b ? b - a < ln : a - b < rn;
}
static int name(const unsigned char *p) {
  unsigned i = 0, ink = 0;
  if (!p[0])
    return 0;
  for (; i < 24 && p[i]; ++i) {
    if (p[i] < 32 || p[i] > 126)
      return 0;
    if (p[i] != 32)
      ink = 1;
  }
  if (i == 24 || !ink)
    return 0;
  for (; i < 24; ++i)
    if (p[i])
      return 0;
  return 1;
}
static int unit(uint32_t v) { return v <= 0x3f800000u || v == 0x80000000u; }
static int valid_kit(const gin_preset_owned_t *k) {
  unsigned i, j, s, p;
  if (!k || k->count != 24 || !name((const unsigned char *)k->name))
    return 0;
  for (i = 0; i < 24; ++i) {
    const gin_preset_sound_t *a = &k->sounds[i];
    if (!a->id || !name((const unsigned char *)a->name) || a->morph > 1000 ||
        a->edit_side > 1)
      return 0;
    for (j = 0; j < i; ++j)
      if (k->sounds[j].id == a->id)
        return 0;
    for (s = 0; s < 2; ++s)
      for (p = 0; p < 21; ++p)
        if (!unit(bits(&a->endpoint[s][p])))
          return 0;
    for (j = 0; j < i; ++j)
      if (k->key_map[j] == k->key_map[i])
        return 0;
    for (j = 0; j < 24 && k->sounds[j].id != k->key_map[i]; ++j) {
    }
    if (j == 24)
      return 0;
  }
  return 1;
}



static uint32_t quantize(uint32_t v, unsigned parameter) {
  uint32_t q = 0, e = (v >> 23) & 255u;
  if (v == 0x80000000u)
    v = 0;
  if (e >= 110u) {
    unsigned shift = 150u - e;
    uint64_t product = (uint64_t)((v & 0x7fffffu) | 0x800000u) * 65535u;
    q = (uint32_t)((product + ((uint64_t)1 << (shift - 1))) >> shift);
  }


  if (parameter == 0 || parameter == 4 || parameter == 7) {
    if (v < 0x3eaaaaabu) {
      if (q >= 21845u)
        q = 21844u;
    } else if (v < 0x3f2aaaabu) {
      if (q < 21845u)
        q = 21845u;
      if (q >= 43690u)
        q = 43689u;
    } else if (q < 43690u)
      q = 43690u;
  }
  if (parameter == 10) {
    if (v > 0x3f2aaaabu) {
      if (q <= 43690u)
        q = 43691u;
    } else if (v > 0x3eaaaaabu) {
      if (q <= 21845u)
        q = 21846u;
      if (q > 43690u)
        q = 43690u;
    } else if (q > 21845u)
      q = 21845u;
  }
  return q;
}
static int sextet(unsigned char c);
typedef struct {
  const unsigned char *bytes;
  unsigned encoded;
} snapshot_reader_t;


static unsigned char read_byte(const snapshot_reader_t *r, unsigned offset) {
  unsigned n, part, a, b;
  if (!r->encoded)
    return r->bytes[offset];
  n = offset / 3u * 4u;
  part = offset % 3u;
  a = (unsigned)sextet(r->bytes[n + part]);
  b = (unsigned)sextet(r->bytes[n + part + 1u]);
  return (unsigned char)(part == 0
                             ? (a << 2) | (b >> 4)
                             : part == 1 ? (a << 4) | (b >> 2) : (a << 6) | b);
}
static uint32_t read16(const snapshot_reader_t *r, unsigned offset) {
  return (uint32_t)read_byte(r, offset) | (uint32_t)read_byte(r, offset + 1)
                                              << 8;
}
static uint32_t read32(const snapshot_reader_t *r, unsigned offset) {
  return read16(r, offset) | read16(r, offset + 2) << 16;
}
static int read_name(const snapshot_reader_t *r, unsigned offset) {
  unsigned i = 0, ink = 0, c;
  if (!read_byte(r, offset))
    return 0;
  for (; i < 24 && (c = read_byte(r, offset + i)) != 0; ++i) {
    if (c < 32 || c > 126)
      return 0;
    if (c != 32)
      ink = 1;
  }
  if (i == 24 || !ink)
    return 0;
  for (; i < 24; ++i)
    if (read_byte(r, offset + i))
      return 0;
  return 1;
}
static uint32_t read_crc(const snapshot_reader_t *r) {
  uint32_t v = ~0u;
  unsigned i, b;
  for (i = 12; i < GIN_SNAPSHOT_BINARY_BYTES; ++i) {
    v ^= read_byte(r, i);
    for (b = 0; b < 8; ++b)
      v = (v >> 1) ^ ((0u - (v & 1u)) & 0xedb88320u);
  }
  return ~v;
}
static int valid_binary(const snapshot_reader_t *in) {
  unsigned i, j;
  unsigned p, map;
  if (read_byte(in, 0) != 'G' || read_byte(in, 1) != 'N' ||
      read_byte(in, 2) != 'S' || read_byte(in, 3) != '1' ||
      read16(in, 4) != 1 || read16(in, 6) != 24 ||
      read32(in, 8) != read_crc(in))
    return 0;
  map = 12 + 24 * SOUND_BYTES;
  if (!read_name(in, map + 24))
    return 0;
  for (i = 0; i < 24; ++i) {
    p = 12 + i * SOUND_BYTES;
    if (!read32(in, p) || !read_name(in, p + 4) || read16(in, p + 112) > 1000 ||
        read_byte(in, p + 114) > 1)
      return 0;
    for (j = 0; j < i; ++j)
      if (read32(in, 12 + j * SOUND_BYTES) == read32(in, p))
        return 0;
    if (read_byte(in, map + i) >= 24)
      return 0;
    for (j = 0; j < i; ++j)
      if (read_byte(in, map + j) == read_byte(in, map + i))
        return 0;
  }
  return 1;
}
int gin_snapshot_text_encode(const gin_preset_owned_t *kit, char *buffer,
                             uint32_t capacity) {
  uint32_t i, j, s, input = GIN_SNAPSHOT_BINARY_BYTES,
                    output = GIN_SNAPSHOT_TEXT_BYTES;
  unsigned char *bytes = (unsigned char *)buffer, *p;
  if (!kit || !buffer || capacity < GIN_SNAPSHOT_TEXT_CAPACITY ||
      overlaps(kit, sizeof(*kit), buffer, GIN_SNAPSHOT_TEXT_CAPACITY) ||
      !valid_kit(kit))
    return -1;
  bytes[0] = 'G';
  bytes[1] = 'N';
  bytes[2] = 'S';
  bytes[3] = '1';
  put16(bytes + 4, 1);
  put16(bytes + 6, 24);
  p = bytes + 12;
  for (i = 0; i < 24; ++i) {
    const gin_preset_sound_t *a = &kit->sounds[i];
    put32(p, a->id);
    copy(p + 4, a->name, 24);
    for (s = 0; s < 2; ++s)
      for (j = 0; j < 21; ++j)
        put16(p + 28 + 2 * (s * 21 + j), quantize(bits(&a->endpoint[s][j]), j));
    put16(p + 112, a->morph);
    p[114] = a->edit_side;
    p += SOUND_BYTES;
  }


  for (i = 0; i < 24; ++i) {
    for (j = 0; j < 24 && kit->sounds[j].id != kit->key_map[i]; ++j) {
    }
    p[i] = (unsigned char)j;
  }
  copy(p + 24, kit->name, 24);
  put32(bytes + 8, crc(bytes + 12, GIN_SNAPSHOT_BINARY_BYTES - 12));


  while (input) {
    uint32_t v;
    input -= 3;
    output -= 4;
    v = (uint32_t)bytes[input] << 16 | (uint32_t)bytes[input + 1] << 8 |
        bytes[input + 2];
    buffer[output] = alphabet[(v >> 18) & 63u];
    buffer[output + 1] = alphabet[(v >> 12) & 63u];
    buffer[output + 2] = alphabet[(v >> 6) & 63u];
    buffer[output + 3] = alphabet[v & 63u];
  }
  buffer[GIN_SNAPSHOT_TEXT_BYTES] = 0;
  return 0;
}
static int sextet(unsigned char c) {
  if (c >= 'A' && c <= 'Z')
    return c - 'A';
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 26;
  if (c >= '0' && c <= '9')
    return c - '0' + 52;
  if (c == '+')
    return 62;
  if (c == '/')
    return 63;
  return -1;
}
int gin_snapshot_text_validate(const char *text, uint32_t length,
                               unsigned char *scratch, uint32_t capacity) {
  uint32_t input, output = 0;
  if (!text || !scratch || length != GIN_SNAPSHOT_TEXT_BYTES ||
      capacity < GIN_SNAPSHOT_BINARY_BYTES ||
      overlaps(text, length, scratch, GIN_SNAPSHOT_BINARY_BYTES))
    return -1;
  for (input = 0; input < length; input += 4) {
    int a = sextet((unsigned char)text[input]),
        b = sextet((unsigned char)text[input + 1]),
        c = sextet((unsigned char)text[input + 2]),
        d = sextet((unsigned char)text[input + 3]);
    uint32_t v;
    if (a < 0 || b < 0 || c < 0 || d < 0)
      return -1;
    v = (uint32_t)a << 18 | (uint32_t)b << 12 | (uint32_t)c << 6 | (uint32_t)d;
    scratch[output++] = (unsigned char)(v >> 16);
    scratch[output++] = (unsigned char)(v >> 8);
    scratch[output++] = (unsigned char)v;
  }
  {
    snapshot_reader_t reader = {scratch, 0};
    return valid_binary(&reader) ? 0 : -1;
  }
}
int gin_snapshot_text_validate_readonly(const char *text, uint32_t length) {
  unsigned i;
  snapshot_reader_t reader;
  if (!text || length != GIN_SNAPSHOT_TEXT_BYTES)
    return -1;
  for (i = 0; i < length; ++i)
    if (sextet((unsigned char)text[i]) < 0)
      return -1;
  reader.bytes = (const unsigned char *)text;
  reader.encoded = 1;
  return valid_binary(&reader) ? 0 : -1;
}
int gin_snapshot_text_decode(const char *text, uint32_t length,
                             unsigned char *scratch, uint32_t capacity,
                             gin_preset_owned_t *out) {
  unsigned i, j, s;
  const unsigned char *p, *map;
  unsigned char *zero;
  if (!out || !text || !scratch || length != GIN_SNAPSHOT_TEXT_BYTES ||
      capacity < GIN_SNAPSHOT_BINARY_BYTES ||
      overlaps(text, length, out, sizeof(*out)) ||
      overlaps(scratch, GIN_SNAPSHOT_BINARY_BYTES, out, sizeof(*out)))
    return -1;
  if (gin_snapshot_text_validate(text, length, scratch, capacity))
    return -1;
  zero = (unsigned char *)out;
  for (i = 0; i < sizeof(*out); ++i)
    zero[i] = 0;
  out->count = 24;
  map = scratch + 12 + 24 * SOUND_BYTES;
  for (i = 0; i < 24; ++i) {
    gin_preset_sound_t *a = &out->sounds[i];
    p = scratch + 12 + i * SOUND_BYTES;
    a->id = get32(p);
    copy(a->name, p + 4, 24);
    for (s = 0; s < 2; ++s)
      for (j = 0; j < 21; ++j)
        a->endpoint[s][j] = (float)get16(p + 28 + 2 * (s * 21 + j)) / 65535.f;
    a->morph = (uint16_t)get16(p + 112);
    a->edit_side = p[114];
    out->key_map[i] = get32(scratch + 12 + map[i] * SOUND_BYTES);
  }
  copy(out->name, map + 24, 24);
  return 0;
}
