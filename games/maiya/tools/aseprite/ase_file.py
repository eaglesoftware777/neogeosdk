"""Read and write Maiya's walk/run source (.aseprite) without Aseprite.

Covers what build_walkrun.lua and export_walkrun.lua use: an indexed 80x64
sprite with one 16-colour palette (slot 0 transparent), the hidden 'ref'
guide layer and her six limb layers, one full-canvas cel per layer and
frame, frame durations and the walk/run/stop tags. The chunks are the ones
Aseprite 1.3 writes for that document (file format: Aseprite's
docs/ase-file-specs.md), so the file opens in Aseprite as if it had saved
it, and a file Aseprite saved reads back here.

pipeline.py uses this when ASEPRITE=builtin.
"""
import struct
import zlib

import numpy as np

MAGIC, FRAME_MAGIC = 0xA5E0, 0xF1FA
LAYER, CEL, PROFILE, TAGS, PALETTE, USER = 0x2004, 0x2005, 0x2007, 0x2018, 0x2019, 0x2020
VISIBLE, EDITABLE = 1, 2


def _string(text):
    raw = text.encode()
    return struct.pack('<H', len(raw)) + raw


def _chunk(kind, body):
    return struct.pack('<IH', 6 + len(body), kind) + body


def write(path, width, height, colors, layers, frames, tags):
    """colors: 16 [r, g, b]; layers: [(name, visible, editable)];
    frames: [(duration_ms, [index image per layer])]; tags: [(name, first, last)]."""
    body = bytearray()
    for f, (duration, images) in enumerate(frames):
        chunks = []
        if f == 0:
            chunks.append(_chunk(PROFILE, struct.pack('<HHI8x', 1, 0, 0)))
            entries = b''.join(struct.pack('<HBBBB', 0, *(colors[i] if i < len(colors) else (0, 0, 0)),
                                           0 if i == 0 else 255) for i in range(256))
            chunks.append(_chunk(PALETTE, struct.pack('<III8x', 256, 0, 255) + entries))
            tag_body = struct.pack('<H8x', len(tags))
            for name, first, last in tags:
                tag_body += struct.pack('<HHBH6x3sB', first, last, 0, 0, b'\0\0\0', 0) + _string(name)
            chunks.append(_chunk(TAGS, tag_body))
            for _ in tags:      # each tag's colour, as user data
                chunks.append(_chunk(USER, struct.pack('<I4B', 2, 0, 0, 0, 255)))
            for name, visible, editable in layers:
                flags = (VISIBLE if visible else 0) | (EDITABLE if editable else 0)
                chunks.append(_chunk(LAYER, struct.pack('<6HB3x', flags, 0, 0, 0, 0, 0, 255) + _string(name)))
        for index, image in enumerate(images):
            image = np.asarray(image, np.uint8)
            assert image.shape == (height, width)
            chunks.append(_chunk(CEL, struct.pack('<HhhBHh5xHH', index, 0, 0, 255, 2, 0, width, height)
                                 + zlib.compress(image.tobytes())))
        payload = b''.join(chunks)
        body += struct.pack('<IHHH2xI', 16 + len(payload), FRAME_MAGIC, min(len(chunks), 0xFFFF),
                            duration, len(chunks)) + payload
    header = struct.pack('<IHHHHHIHII', 128 + len(body), MAGIC, len(frames), width, height, 8, 1,
                         frames[0][0], 0, 0)
    header += struct.pack('<B3xHBBhhHH', 0, 256, 1, 1, 0, 0, 16, 16)
    header += bytes(128 - len(header))
    with open(path, 'wb') as out:
        out.write(header + bytes(body))


def read(path):
    """The document as a dict: width, height, transparent, colors (rgba),
    layers [{name, visible, editable}], tags [{name, from, to}] (0-based),
    frames [{duration, cels: {layer index: index image}}]."""
    data = open(path, 'rb').read()
    size, magic, count, width, height, depth = struct.unpack_from('<IHHHHH', data, 0)
    if magic != MAGIC or size != len(data):
        raise ValueError(f'{path}: not an Aseprite file')
    if depth != 8:
        raise ValueError(f'{path}: not an indexed sprite')
    doc = dict(width=width, height=height, transparent=data[28], colors=[], layers=[], tags=[], frames=[])
    offset = 128
    for _ in range(count):
        length, magic, old, duration, new = struct.unpack_from('<IHHH2xI', data, offset)
        if magic != FRAME_MAGIC:
            raise ValueError(f'{path}: bad frame header')
        frame = dict(duration=duration, cels={})
        p = offset + 16
        for _ in range(new or old):
            chunk_size, kind = struct.unpack_from('<IH', data, p)
            chunk = data[p + 6:p + chunk_size]
            if kind == LAYER:
                flags, layer_type = struct.unpack_from('<HH', chunk, 0)
                n, = struct.unpack_from('<H', chunk, 16)
                doc['layers'].append(dict(name=chunk[18:18 + n].decode(), visible=bool(flags & VISIBLE),
                                          editable=bool(flags & EDITABLE), type=layer_type,
                                          opacity=chunk[12], blend=struct.unpack_from('<H', chunk, 10)[0]))
            elif kind == CEL:
                layer, x, y, opacity, cel_type = struct.unpack_from('<HhhBH', chunk, 0)
                if cel_type != 2:
                    raise ValueError(f'{path}: unsupported cel type {cel_type}')
                w, h = struct.unpack_from('<HH', chunk, 16)
                image = np.zeros((height, width), np.uint8)
                image[y:y + h, x:x + w] = np.frombuffer(zlib.decompress(chunk[20:]), np.uint8).reshape(h, w)
                frame['cels'][layer] = dict(image=image, opacity=opacity)
            elif kind == PALETTE:
                total, first, last = struct.unpack_from('<III', chunk, 0)
                q = 20
                colors = doc['colors'] or [[0, 0, 0, 0]] * total
                for i in range(first, last + 1):
                    flags, r, g, b, a = struct.unpack_from('<HBBBB', chunk, q)
                    q += 6
                    if flags & 1:
                        q += 2 + struct.unpack_from('<H', chunk, q)[0]
                    colors[i] = [r, g, b, a]
                doc['colors'] = colors
            elif kind == TAGS:
                n, = struct.unpack_from('<H', chunk, 0)
                q = 10
                for _ in range(n):
                    first, last = struct.unpack_from('<HH', chunk, q)
                    q += 17
                    length_, = struct.unpack_from('<H', chunk, q)
                    doc['tags'].append({'name': chunk[q + 2:q + 2 + length_].decode(), 'from': first, 'to': last})
                    q += 2 + length_
            p += chunk_size
        doc['frames'].append(frame)
        offset += length
    return doc


def flatten(doc, frame):
    """What Aseprite draws for a frame: the visible layers bottom to top,
    index 0 transparent (all layers normal blend, full opacity)."""
    out = np.zeros((doc['height'], doc['width']), np.uint8)
    for index, layer in enumerate(doc['layers']):
        cel = doc['frames'][frame]['cels'].get(index)
        if layer['visible'] and cel is not None:
            if layer['blend'] != 0 or layer['opacity'] != 255 or cel['opacity'] != 255:
                raise ValueError('flatten: only normal, opaque layers are supported')
            image = cel['image']
            out[image != 0] = image[image != 0]
    return out
