"""A painted 16:9 key illustration -> the title card's full-colour source,
assets/authored/rebirth/cinema/sources/title-sdk177.png (352x224).

    python3 games/shadowleague/tools/import_title.py PAINTED.png [DROP]

The painting is scaled to the card's width and set DROP lines down (26 by
default; more for a painting whose heads sit high), so the faces clear the
logo that drops onto the top of the card. The gap above is the painting's
top line blurred sideways and darkening upward, blended into the sky mirrored
over its last BLEND lines (a mirror alone repeats raised arms and flags).
The bottom lines are darkened a little for the
PUSH START and copyright rows. rebirth_assets.py then packs the card into
sixteen palettes chosen tile by tile.
"""
import sys
from pathlib import Path

import numpy as np
from PIL import Image

GAME = Path(__file__).resolve().parents[1]
OUT = GAME / 'assets/authored/rebirth/cinema/sources/title-sdk177.png'
W, H = 352, 224
DROP = 26          # lines of mirrored sky above the painting
BLEND = 12         # lines over which the sky fill meets the painting
SHADE = 28         # lines darkened at the bottom, down to SHADE_MIN
SHADE_MIN = 0.55


def compose(path, drop=DROP):
    """The 352x224 RGBA card for a painting."""
    art = Image.open(path).convert('RGB')
    height = round(art.height * W / art.width)
    art = np.asarray(art.resize((W, height), Image.Resampling.LANCZOS)).astype(float)
    card = np.zeros((H, W, 3))
    card[drop:] = art[:H - drop]
    top = art[:4].mean(axis=0)
    kernel = np.ones(61) / 61
    top = np.stack([np.convolve(np.pad(top[:, c], 30, mode='edge'), kernel, 'valid') for c in range(3)], 1)
    fill = top[None] * np.linspace(0.45, 1.0, drop)[:, None, None]
    mix = np.clip((np.arange(drop) - (drop - BLEND)) / BLEND, 0, 1)[:, None, None]
    card[:drop] = fill * (1 - mix) + art[:drop][::-1] * mix
    ramp = np.linspace(1.0, SHADE_MIN, SHADE)[:, None, None]
    card[H - SHADE:] *= ramp
    return np.dstack([np.clip(card, 0, 255), np.full((H, W), 255.0)]).astype(np.uint8)


def main():
    drop = int(sys.argv[2]) if len(sys.argv) > 2 else DROP
    Image.fromarray(compose(sys.argv[1], drop), 'RGBA').save(OUT)
    print(OUT, (W, H))


if __name__ == '__main__':
    main()
