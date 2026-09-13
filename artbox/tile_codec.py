"""C-ROM planar encoding with stable, row-major asset reservations."""

import numpy as np

HALF_SOLID_TILE = 0xFFFD
SOLID_TILE = 0xFFFE
BLANK_TILE = 0xFFFF


def write_utility_tiles(c1_file, c2_file):
    """Reserve opaque/half-height HUD tiles and the engine's transparent tile."""
    start = HALF_SOLID_TILE * 64
    if c1_file.tell() != c2_file.tell() or c1_file.tell() > start:
        raise ValueError("C-ROM assets overlap reserved utility tiles FFFD-FFFF")
    gap = bytes(start - c1_file.tell())
    c1_file.write(gap)
    c2_file.write(gap)
    pixels = np.zeros((16, 48), dtype=np.uint8)
    pixels[:8, :16] = 1
    pixels[:, 16:32] = 1
    c1, c2 = encode_image(pixels, 3)
    c1_file.write(c1)
    c2_file.write(c2)


def encode_image(indexed, reserved_tiles=256):
    indexed = np.asarray(indexed)
    if indexed.ndim != 2:
        raise ValueError("Indexed art must have two dimensions")
    height, width = indexed.shape
    if not height or not width or height % 16 or width % 16:
        raise ValueError("C-ROM art dimensions must be multiples of 16")
    count = height * width // 256
    if count > reserved_tiles or np.any(indexed < 0) or np.any(indexed > 15):
        raise ValueError("Asset exceeds its tile reservation or 4bpp palette")
    c1, c2 = bytearray(), bytearray()
    for y in range(0, height, 16):
        for x in range(0, width, 16):
            tile = indexed[y:y + 16, x:x + 16]
            for bx, by in ((8, 0), (8, 8), (0, 0), (0, 8)):
                for row in tile[by:by + 8, bx:bx + 8]:
                    planes = [sum(((int(pixel) >> bit) & 1) << i
                                  for i, pixel in enumerate(row)) for bit in range(4)]
                    # romts swaps each pair before writing the final C-ROMs.
                    c1.extend((planes[1], planes[0]))
                    c2.extend((planes[3], planes[2]))
    padding = bytes((reserved_tiles - count) * 64)
    return bytes(c1) + padding, bytes(c2) + padding


def decode_image(c1, c2, width, height, swapped=False):
    """Decode packer or final ROM bytes for round-trip and visual checks."""
    indexed = np.zeros((height, width), dtype=np.uint8)
    pos = 0
    for y in range(0, height, 16):
        for x in range(0, width, 16):
            for bx, by in ((8, 0), (8, 8), (0, 0), (0, 8)):
                for row in range(8):
                    lo, hi = (0, 1) if swapped else (1, 0)
                    planes = (c1[pos + lo], c1[pos + hi], c2[pos + lo], c2[pos + hi])
                    for i in range(8):
                        indexed[y + by + row, x + bx + i] = sum(
                            ((plane >> i) & 1) << bit for bit, plane in enumerate(planes)
                        )
                    pos += 2
    return indexed
