#!/usr/bin/env python3
"""
Seven Wonders title logo, a cat-eared O and a Japanese ribbon, and the
cheering words that pop up in play, all in candy letters.

    python3 games/sevenwonders/tools/logo.py --fonts DIR [--out-dir DIR]

DIR holds the M PLUS Rounded 1c fonts (SIL Open Font License):
MPLUSRounded1c-Black.ttf and MPLUSRounded1c-ExtraBold.ttf. The pictures are
written to artbox/painted/logo.png (304 x 112) and words.png (six 160 x 32
frames: iine, sugoi, suteki, nyandafuru, kansei, rakkii), which tools/art.py
packs in place of its plain ones. Only the rendered pictures go in the ROM.

Each letter is cut out on its own, tilted a little and stacked as a
sticker: a deep plum outline and drop shadow under a white rim, then the
letter in a two-tone candy gradient with a shine across its top. SEVEN is
strawberry pink; the seven letters of WONDERS take the colours of the
seven wonders, and the O is a cat's head. Under them a ribbon
reads "nyanko pazuru" (kitty puzzle) in rounded kana.
"""

import argparse
import math
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

GAME = Path(__file__).resolve().parents[1]
SS = 4                       # supersampling
W, H = 304, 112

PLUM = (66, 18, 72)
SHADOW = (44, 10, 58)
WHITE = (255, 255, 255)
# (top, bottom) per letter
SEVEN = [((255, 170, 214), (240, 64, 150))] * 5
WONDERS = [
    ((255, 150, 160), (236, 56, 92)),     # rose
    ((255, 206, 120), (246, 128, 32)),    # amber: the cat's head
    ((255, 240, 140), (240, 188, 20)),    # sunstone
    ((170, 248, 170), (40, 180, 90)),     # garden green
    ((170, 240, 255), (30, 160, 220)),    # canal aqua
    ((190, 200, 255), (90, 100, 240)),    # moonlight blue
    ((230, 180, 255), (156, 70, 226)),    # starlight violet
]


def S(v):
    return int(round(v * SS))


def glyph_mask(font, ch):
    """A letter's mask on its own canvas, with room to tilt and outline."""
    l, t, r, b = font.getbbox(ch)
    pad = S(10)
    img = Image.new("L", (r - l + 2 * pad, b - t + 2 * pad), 0)
    ImageDraw.Draw(img).text((pad - l, pad - t), ch, font=font, fill=255)
    return img


def dilate(mask, r):
    """Round dilation by r supersampled pixels."""
    if r <= 0:
        return mask
    return mask.filter(ImageFilter.GaussianBlur(r * 0.5)).point(lambda v: 255 if v > 8 else 0) \
        if r < 3 else mask.filter(ImageFilter.MaxFilter(2 * (r // 2) + 1)).filter(ImageFilter.GaussianBlur(r * 0.25)).point(lambda v: 255 if v > 100 else 0)


def gradient_fill(size, top, bottom, y0, y1):
    w, h = size
    t = np.clip((np.arange(h)[:, None] - y0) / max(1, (y1 - y0)), 0, 1)
    t = np.repeat(t, w, axis=1)[..., None]
    g = np.asarray(top)[None, None, :] * (1 - t) + np.asarray(bottom)[None, None, :] * t
    return g.astype(np.uint8)


class Layers:
    """Shadow, plum outline, white rim and fill, each a full-size canvas."""

    def __init__(self, w=W, h=H):
        size = (S(w), S(h))
        self.shadow = Image.new("L", size, 0)
        self.outline = Image.new("L", size, 0)
        self.rim = Image.new("L", size, 0)
        self.fill = Image.new("RGBA", size, (0, 0, 0, 0))

    def letter(self, mask, x, y, angle, colors, shine=True):
        """Place a letter mask with its top-left at (x, y), tilted by angle degrees."""
        m = mask.rotate(angle, resample=Image.BICUBIC, expand=False).point(lambda v: 255 if v > 127 else 0)
        rim = dilate(m, S(2.6))
        out = dilate(m, S(4.6))
        for layer, piece, dx, dy in ((self.shadow, out, S(2), S(2.5)), (self.outline, out, 0, 0), (self.rim, rim, 0, 0)):
            layer.paste(255, (x + dx, y + dy), piece)
        box = m.getbbox() or (0, 0, 1, 1)
        top, bottom = colors
        fill = gradient_fill(m.size, top, bottom, box[1], box[3])
        rgba = np.dstack((fill, np.asarray(m)))
        if shine:
            # a shine across the top third, inset from the edges
            inner = np.asarray(m.filter(ImageFilter.MinFilter(2 * S(1.5) + 1))) > 0
            yy = np.arange(m.size[1])[:, None]
            band = (yy > box[1] + (box[3] - box[1]) * 0.08) & (yy < box[1] + (box[3] - box[1]) * 0.32)
            sel = inner & band
            rgba[sel, :3] = (rgba[sel, :3].astype(int) * 0.35 + 255 * 0.65).astype(np.uint8)
        piece = Image.fromarray(rgba, "RGBA")
        self.fill.alpha_composite(piece, (x, y))

    def compose(self):
        size = self.fill.size
        img = Image.new("RGBA", size, (0, 0, 0, 0))
        img.paste(SHADOW + (255,), (0, 0), self.shadow)
        img.paste(PLUM + (255,), (0, 0), self.outline)
        img.paste(WHITE + (255,), (0, 0), self.rim)
        img.alpha_composite(self.fill)
        return img


def cat_head(font):
    """The O as a cat's head: a round face with two ears, on the O's own canvas."""
    base = glyph_mask(font, "O")
    w, h = base.size
    pad = S(10)
    bw, bh = w - 2 * pad, h - 2 * pad
    m = Image.new("L", (w, h), 0)
    d = ImageDraw.Draw(m)
    top = pad + bh * 0.10
    d.ellipse([pad - S(1.5), top, pad + bw + S(1.5), pad + bh + S(0.5)], fill=255)
    for side in (-1, 1):
        cx = pad + bw / 2 + side * bw * 0.30
        d.polygon([(cx - bw * 0.22, top + bh * 0.22), (cx + side * bw * 0.06, top - S(7.5)),
                   (cx + bw * 0.22, top + bh * 0.22)], fill=255)
    return m


def word(layers, font, text, colors, cx, baseline, spacing, tilts, bounce, custom=None):
    masks = [custom[i](font) if custom and i in custom else glyph_mask(font, ch) for i, ch in enumerate(text)]
    widths = [m.size[0] - 2 * S(10) for m in masks]
    total = sum(widths) + spacing * (len(text) - 1)
    x = cx - total // 2
    asc = font.getbbox("E")[1]
    placed = []
    for i, (ch, m) in enumerate(zip(text, masks)):
        l, t, r, b = font.getbbox(ch)
        top = baseline - (font.getbbox("E")[3] - t) + bounce[i]
        layers.letter(m, x - S(10), top - S(10), tilts[i], colors[i], shine=not (custom and i in custom))
        placed.append((x, top, x + widths[i], top + (b - t)))
        x += widths[i] + spacing
    return placed


def cat_face(layers, box):
    """Inner ears, eyes, blush, nose and mouth on the cat's head."""
    x0, y0, x1, y1 = box
    w, h = x1 - x0, y1 - y0
    cx = (x0 + x1) / 2
    d = ImageDraw.Draw(layers.fill)
    top = y0 + h * 0.10
    for side in (-1, 1):
        ex = cx + side * w * 0.30
        d.polygon([(ex - w * 0.11, top + h * 0.08), (ex + side * w * 0.05, top - S(4.5)),
                   (ex + w * 0.11, top + h * 0.08)], fill=(255, 150, 190, 255))
    ey = y0 + h * 0.55
    for side in (-1, 1):
        ex = cx + side * w * 0.22
        d.ellipse([ex - S(2.2), ey - S(3.4), ex + S(2.2), ey + S(2.6)], fill=PLUM + (255,))
        d.ellipse([ex - S(1.3), ey - S(2.6), ex + S(0.1), ey - S(1.1)], fill=WHITE + (255,))
        bx = cx + side * w * 0.36
        d.ellipse([bx - S(2.6), ey + S(3.2), bx + S(2.6), ey + S(5.2)], fill=(255, 120, 160, 255))
    d.polygon([(cx - S(1.4), ey + S(2.2)), (cx + S(1.4), ey + S(2.2)), (cx, ey + S(3.6))], fill=(230, 80, 120, 255))
    for side in (-1, 1):
        d.arc([cx + (side - 1) * S(1.6), ey + S(2.6), cx + (side + 1) * S(1.6), ey + S(6.0)], 20, 160,
              fill=PLUM + (255,), width=S(0.9))


def sparkle(d, x, y, r, color=(255, 255, 255, 255)):
    pts = []
    for i in range(8):
        a = i * math.pi / 4
        rr = r if i % 2 == 0 else r * 0.28
        pts.append((x + math.cos(a) * rr, y + math.sin(a) * rr))
    d.polygon(pts, fill=color)


def ribbon(img, font, text, cx, cy, w, h):
    """A pink ribbon with folded tails and white kana."""
    d = ImageDraw.Draw(img)
    x0, x1, y0, y1 = cx - w // 2, cx + w // 2, cy - h // 2, cy + h // 2
    tail = S(12)
    fold = S(5)
    pink, deep, light = (255, 112, 170), (196, 52, 120), (255, 176, 210)
    for sx in (-1, 1):
        ex = x0 if sx < 0 else x1
        pts = [(ex + sx * S(4), y0 + S(5)), (ex + sx * (tail + S(10)), y0 + S(5)), (ex + sx * (S(8) + tail - S(6)), cy + S(3)),
               (ex + sx * (tail + S(10)), y1 + S(5)), (ex + sx * S(4), y1 + S(5))]
        d.polygon([(px, py) for px, py in pts], fill=PLUM + (255,))
        inner = [(ex + sx * S(5), y0 + S(7)), (ex + sx * (tail + S(6)), y0 + S(7)), (ex + sx * (S(8) + tail - S(9)), cy + S(3)),
                 (ex + sx * (tail + S(6)), y1 + S(3)), (ex + sx * S(5), y1 + S(3))]
        d.polygon(inner, fill=deep + (255,))
        d.polygon([(ex, y1), (ex + sx * fold, y1 + S(5)), (ex + sx * fold, y1)], fill=PLUM + (255,))
    d.rounded_rectangle([x0 - S(2), y0 - S(2), x1 + S(2), y1 + S(2)], radius=S(5), fill=PLUM + (255,))
    d.rounded_rectangle([x0, y0, x1, y1], radius=S(4), fill=pink + (255,))
    d.rounded_rectangle([x0 + S(2), y0 + S(1.5), x1 - S(2), y0 + S(4)], radius=S(1.5), fill=light + (255,))
    # dotted stitching
    for k in range(int((x1 - x0) / S(5))):
        px = x0 + S(3) + k * S(5)
        for py in (y0 + S(2.2), y1 - S(2.2)):
            if px < x1 - S(2):
                d.ellipse([px - S(0.6), py - S(0.6), px + S(0.6), py + S(0.6)], fill=(255, 220, 236, 255))
    l, t, r, b = font.getbbox(text)
    tx, ty = cx - (r - l) // 2 - l, cy - (b - t) // 2 - t
    m = Image.new("L", img.size, 0)
    ImageDraw.Draw(m).text((tx, ty), text, font=font, fill=255)
    edge = dilate(m, S(1.6))
    img.paste(deep + (255,), (S(0.5), S(1)), edge)
    img.paste((255, 255, 255, 255), (0, 0), m)


def reduce(img, w=W, h=H):
    small = img.resize((w, h), Image.LANCZOS)
    a = np.asarray(small).copy()
    solid = a[:, :, 3] >= 100
    a[:, :, 3] = np.where(solid, 255, 0)
    # undo the dark fringe that premultiplied resampling leaves on edges
    return Image.fromarray(a, "RGBA")


def build(font_dir):
    black = ImageFont.truetype(str(font_dir / "MPLUSRounded1c-Black.ttf"), S(43))
    bold = ImageFont.truetype(str(font_dir / "MPLUSRounded1c-ExtraBold.ttf"), S(16))
    layers = Layers()
    word(layers, black, "SEVEN", SEVEN, S(W / 2), S(43), S(1), [-6, 4, -3, 5, -5], [S(1), S(-2), S(1), S(-1), S(2)])
    placed = word(layers, black, "WONDERS", WONDERS, S(W / 2), S(82), S(0), [5, 0, 3, -5, 4, -3, 6],
                  [S(-1), S(0), S(-2), S(1), S(-1), S(2), S(0)], custom={1: cat_head})
    cat_face(layers, placed[1])
    img = layers.compose()
    d = ImageDraw.Draw(img)
    for x, y, r in ((34, 18, 6), (272, 20, 5.5), (266, 62, 4), (36, 68, 4), (150, 6, 3.5), (270, 90, 3)):
        sparkle(d, S(x), S(y), S(r) + S(1.5), PLUM + (255,))
        sparkle(d, S(x), S(y), S(r), (255, 250, 200, 255))
    ribbon(img, bold, "にゃんこパズル", S(W / 2), S(97), S(100), S(18))
    return reduce(img)


# The words that pop up: (text, (top, bottom) candy colours).
WORDS = [
    ("いいね！", ((255, 170, 214), (240, 64, 150))),            # nice
    ("すごい！", ((255, 214, 130), (246, 120, 30))),            # amazing
    ("すてき！", ((180, 250, 190), (40, 180, 100))),            # lovely
    ("にゃんだふる！", ((220, 190, 255), (140, 70, 230))),      # nyan-derful
    ("かんせい！", ((255, 244, 150), (236, 170, 10))),          # finished!
    ("ラッキー！", ((255, 160, 170), (230, 40, 70))),           # lucky!
]
WORD_W, WORD_H = 160, 32


def word_frame(font, text, colors):
    """One word, centred in a 160 x 32 frame, its letters bouncing a little."""
    layers = Layers(WORD_W, WORD_H)
    n = len(text)
    tilts = [(-5, 4, -3, 5, -4, 3, -5)[i % 7] for i in range(n)]
    bounce = [S((1, -1, 1, 0, -1, 1, 0)[i % 7]) for i in range(n)]
    word(layers, font, text, [colors] * n, S(WORD_W / 2), S(25), S(-1), tilts, bounce)
    img = layers.compose()
    return reduce(img, WORD_W, WORD_H)


def build_words(font_dir):
    sizes = {7: 19, 6: 21, 5: 23}
    frames = []
    for text, colors in WORDS:
        font = ImageFont.truetype(str(font_dir / "MPLUSRounded1c-Black.ttf"), S(sizes.get(len(text), 23)))
        frames.append(np.asarray(word_frame(font, text, colors)))
    return Image.fromarray(np.concatenate(frames, axis=1), "RGBA")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--fonts", required=True, type=Path)
    ap.add_argument("--out-dir", type=Path, default=GAME / "artbox/painted")
    args = ap.parse_args()
    args.out_dir.mkdir(parents=True, exist_ok=True)
    for name, img in (("logo", build(args.fonts)), ("words", build_words(args.fonts))):
        img.save(args.out_dir / f"{name}.png")
        print(f"logo: {args.out_dir / name}.png {img.size}")


if __name__ == "__main__":
    main()
