"""Painted artwork: full-colour frames fitted to several hardware palettes.

assets/painted/painted.json lists families of frames that are shown in turn
on one sprite (a jet's bank poses, a boss's damage states). Each family is
fitted as one picture to its palette banks, so every member loads the same
palettes and changing frame never recolours a live sprite; each 16x16 tile
then picks whichever of those palettes suits it best.
"""
import json
from pathlib import Path

import numpy as np
from PIL import Image

from palette_banks import quantize

ROOT = Path(__file__).resolve().parents[1] / 'assets/painted'


def frame(path, full=False):
    """A painted frame; `full` (a full-screen picture) covers its whole canvas."""
    with Image.open(path) as im:
        rgba = np.array(im.convert('RGBA'))
    if rgba.shape[0] % 16 or rgba.shape[1] % 16:
        raise ValueError(f'{path}: canvas must be whole 16x16 tiles')
    alpha = rgba[:, :, 3]
    if ((alpha > 0) & (alpha < 255)).any():
        raise ValueError(f'{path}: painted frames need hard (0 or 255) alpha')
    if full:
        if not alpha.all():
            raise ValueError(f'{path}: a full-screen picture must be opaque')
        return rgba
    for edge in (alpha[0], alpha[-1], alpha[:, 0], alpha[:, -1]):
        if edge.any():
            raise ValueError(f'{path}: artwork touches the canvas edge')
    return rgba


def load():
    """{asset name: (RGBA image, (indices, palettes, tile banks), bank, pivot)}"""
    manifest = ROOT / 'painted.json'
    if not manifest.exists():
        return {}
    out = {}
    for family in json.loads(manifest.read_text()):
        names = list(family['members'])
        frames = [frame(ROOT / family['members'][n], family.get('full', False)) for n in names]
        width = max(f.shape[1] for f in frames)
        stack = np.zeros((sum(f.shape[0] for f in frames), width, 4), np.uint8)
        rows = []
        y = 0
        for f in frames:
            stack[y:y + f.shape[0], :f.shape[1]] = f
            rows.append((y, f.shape[0], f.shape[1]))
            y += f.shape[0]
        indices, palettes, tiles = quantize(stack, family['banks'])
        for name, f, (y, h, w) in zip(names, frames, rows):
            table = (indices[y:y + h, :w], palettes, tiles[y // 16:(y + h) // 16, :w // 16])
            pivot = family.get('pivot') or (w // 2, h // 2)
            out[name] = (Image.fromarray(f, 'RGBA'), table, family['bank'], tuple(pivot))
    return out
