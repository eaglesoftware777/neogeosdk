#!/usr/bin/env python3
"""
Pack a game's artwork straight into Neo Geo C1/C2 and S1 ROMs and a C header.

A game's own art builder (its GAME_ART_BUILDER) describes its pictures and
hands them to an ArtPack:

    from ng_artpack import ArtPack
    pack = ArtPack(prefix="sw", game_id="782", title="SEVEN WONDERS")
    pack.sprite("cat_mimi", rgba, frame=(64, 64), banks=2)        # a sheet of frames
    pack.background("garden", rgba, banks=12)                     # 320 x 224 picture
    pack.font_glyphs(extra)                                      # optional FIX tiles
    pack.write(game_dir)

What it does, with the SDK's own codecs (artbox/palette_banks.py,
artbox/tile_codec.py) so the result decodes exactly as the hardware will:

  * fits each asset's colours to up to 16 palette banks of 15 colours (one
    bank chosen per 16x16 tile; bank 0 of an asset is its global fallback),
    numbered from `first_bank` upward, never past 255; refine=N adds up to N
    refit passes, for paintings whose smooth skies or roads would otherwise
    split into tile-sized blocks;
  * encodes the 4bpp tiles row by row (the order NGSpriteGroup reads them,
    tile = base + strip + row * strips) and checks every tile decodes back;
  * keeps the console system ROM's eye-catcher bank (0x49, tiles 0x4900-
    0x49FF) for a 240 x 64 title logo, and the three utility tiles the
    engine expects at 0xFFFD-0xFFFF; writes 4 MiB lanes, pair-swapped as
    the cartridge wants;
  * builds S1 from EagleBIOS's original font with the ASCII page at
    0xD00 (ng_fix_set_ascii_base(0xD00)), plus any extra 8x8 glyphs;
    text_edge=N rings that page's letters with colour N, so text stays
    readable over a busy picture (the system's own pages are left alone);
  * writes <prefix>_assets.h: an NGArtAsset per asset (tile base, size,
    first bank, per-tile bank map), its palettes as Neo Geo colour words,
    frame counts, and #defines for every name; and assets.json, a preview
    PNG per asset and the ROMs (artbox/ and roms/<game>/).

Fitting colours is the slow part of a build, so each fit is kept in a cache
keyed on the picture's pixels, the fit settings and the fitter's own source
(`~/.cache/neogeosdk/artpack`, or NG_ARTPACK_CACHE_DIR; NG_ARTPACK_CACHE=0
turns it off): a picture that has not changed is not fitted again.

Every picture in the pack should be original or legitimately licensed; the
caller records provenance (the pack only converts).
"""

import hashlib
import importlib.util
import json
import os
from pathlib import Path
import sys

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "artbox"))
from palette_banks import palette_words, quantize, reconstruct  # noqa: E402
from tile_codec import decode_image, encode_image, write_utility_tiles  # noqa: E402

CACHE = Path(os.environ.get("NG_ARTPACK_CACHE_DIR", Path.home() / ".cache/neogeosdk/artpack"))
USE_CACHE = os.environ.get("NG_ARTPACK_CACHE", "1") != "0"
FIT_SOURCE = hashlib.sha1((ROOT / "artbox/palette_banks.py").read_bytes()).digest()


def fit(rgba, banks, master, refine):
    """quantize(), through the cache: (indices, palettes, assignment)."""
    if not USE_CACHE:
        return quantize(rgba, bank_limit=banks, master=master, refine=refine)
    h = hashlib.sha1(FIT_SOURCE)
    h.update(repr((rgba.shape, banks, refine, master is not None)).encode())
    h.update(np.ascontiguousarray(rgba).tobytes())
    if master is not None:
        h.update(np.asarray(master, dtype=np.uint8).tobytes())
    path = CACHE / f"{h.hexdigest()}.npz"
    if path.is_file():
        with np.load(path) as z:
            return z["indices"], z["palettes"], z["assignment"]
    result = quantize(rgba, bank_limit=banks, master=master, refine=refine)
    CACHE.mkdir(parents=True, exist_ok=True)
    tmp = path.with_suffix(".tmp.npz")
    np.savez_compressed(tmp, indices=result[0], palettes=result[1], assignment=result[2])
    tmp.replace(path)
    return result


LANE = 0x400000
BOOT_BANK = 0x49
BOOT_START = BOOT_BANK * 256
BOOT_END = BOOT_START + 256
FIX_ASCII_BASE = 0xD00
FIRST_TILE = 0x100


def _font():
    spec = importlib.util.spec_from_file_location("eagle_sfix", ROOT / "bios/tools/gen_sfix.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


FONT = _font()


def ringed_glyph(ch, edge):
    """The system font's letter (ink 1, columns 1-5, rows 0-6 of the cell)
    with every empty pixel next to its ink set to `edge`: an outline, cut
    off only along the cell's top row."""
    rows = [int(r, 16) for r in FONT.GLYPHS.get(ch.upper(), FONT.GLYPHS["?"]).split()]
    ink = [[0] * 8 for _ in range(8)]
    for y, row in enumerate(rows):
        for x in range(5):
            ink[y][x + 1] = (row >> (4 - x)) & 1
    out = [[0] * 8 for _ in range(8)]
    for y in range(8):
        for x in range(8):
            if ink[y][x]:
                out[y][x] = 1
            elif any(ink[y + dy][x + dx] for dy in (-1, 0, 1) for dx in (-1, 0, 1)
                     if 0 <= y + dy < 8 and 0 <= x + dx < 8):
                out[y][x] = edge
    return out


def text_pixels(text, scale=1, ink=1):
    """The firmware font's glyphs as an indexed picture (8 x 8 per character)."""
    image = np.zeros((8 * scale, len(text) * 8 * scale), dtype=np.uint8)
    for i, char in enumerate(text):
        for y, row in enumerate(FONT.GLYPHS.get(char.upper(), "00 " * 7).split()):
            for x in range(5):
                if int(row, 16) & (1 << (4 - x)):
                    image[y * scale:(y + 1) * scale, (i * 8 + x + 1) * scale:(i * 8 + x + 2) * scale] = ink
    return image


def rgba_array(image):
    if isinstance(image, Image.Image):
        image = np.asarray(image.convert("RGBA"))
    image = np.asarray(image, dtype=np.uint8)
    if image.ndim != 3 or image.shape[2] != 4:
        raise ValueError("expected an RGBA picture")
    return image


def pad16(rgba):
    h, w = rgba.shape[:2]
    H, W = (h + 15) // 16 * 16, (w + 15) // 16 * 16
    if (H, W) == (h, w):
        return rgba
    out = np.zeros((H, W, 4), dtype=np.uint8)
    out[:h, :w] = rgba
    return out


class ArtPack:
    def __init__(self, prefix, game_id, title, first_bank=16, logo=None, text_edge=None):
        self.prefix = prefix
        self.text_edge = text_edge  # colour of a 1-pixel ring round the ASCII page's letters (None: none)
        self.game_id = str(game_id)
        self.title = title
        self.first_bank = first_bank
        self.next_bank = first_bank
        self.assets = []          # dicts
        self.extra_glyphs = []    # (tile, 8x8 indexed)
        self.logo = logo          # optional 240 x 64 indexed picture (values 0..15)
        self.logo_palette = None

    # ---- assets -----------------------------------------------------------
    def _add(self, name, kind, rgba, frame, banks, master=None, bank=None, refine=0):
        rgba = pad16(rgba_array(rgba))
        fw, fh = frame if frame else (rgba.shape[1], rgba.shape[0])
        if fw % 16 or fh % 16:
            raise ValueError(f"{name}: frames must be multiples of 16 pixels")
        if rgba.shape[1] % fw or rgba.shape[0] % fh:
            raise ValueError(f"{name}: the sheet is not a whole number of frames")
        indices, palettes, assignment = fit(rgba, banks, master, refine)
        nbanks = len(palettes)
        if bank is None:
            bank = self.next_bank
            self.next_bank += nbanks
        if bank + nbanks - 1 > 255:
            raise ValueError(f"{name}: out of palette banks ({bank}+{nbanks})")
        self.assets.append(dict(name=name, kind=kind, rgba=rgba, indices=indices, palettes=palettes,
                                assignment=assignment, frame=(fw, fh), bank=bank, banks=nbanks))
        return len(self.assets)

    def sprite(self, name, rgba, frame=None, banks=1, master=None, bank=None, refine=0):
        """A sprite sheet: frames of frame=(w, h) left to right, top to bottom."""
        return self._add(name, "sprite", rgba, frame, banks, master, bank, refine)

    def background(self, name, rgba, banks=16, bank=None, refine=0):
        """A screen-sized (or larger) picture drawn as one sprite group."""
        return self._add(name, "background", rgba, None, banks, None, bank, refine)

    def share(self, name, rgba, like, frame=None):
        """A sheet drawn with an earlier asset's first palette (recolours, extra frames)."""
        base = next(a for a in self.assets if a["name"] == like)
        return self._add(name, "sprite", rgba, frame, 1, master=base["palettes"][0][1:], bank=base["bank"])

    def font_glyphs(self, tile, glyph):
        """An extra 8 x 8 FIX glyph (indexed 0..15) at FIX tile `tile`."""
        self.extra_glyphs.append((tile, np.asarray(glyph, dtype=np.uint8)))

    # ---- output -----------------------------------------------------------
    def _fix_rom(self):
        rom = bytearray(FONT.build_rom())
        lo = FIX_ASCII_BASE * 32
        rom[lo:lo + 256 * 32] = rom[:256 * 32]
        rom[0xFF * 32:0x100 * 32] = bytes(32)
        if self.text_edge:
            for code in range(33, 127):
                rom[lo + code * 32:lo + (code + 1) * 32] = FONT.encode_tile(ringed_glyph(chr(code), self.text_edge))
        for tile, glyph in self.extra_glyphs:
            if glyph.shape != (8, 8):
                raise ValueError("FIX glyphs are 8 x 8")
            rom[tile * 32:(tile + 1) * 32] = FONT.encode_tile([[int(v) for v in row] for row in glyph])
        return bytes(rom)

    def _logo(self):
        if self.logo is not None:
            return np.asarray(self.logo, dtype=np.uint8)
        image = np.zeros((64, 240), dtype=np.uint8)
        text = text_pixels(self.title[:13], 2)
        x = max(0, (240 - text.shape[1]) // 2)
        image[16:32, x:x + text.shape[1]] = text[:, :240 - x]
        small = text_pixels("EAGLE SOFTWARE", 1, ink=2)
        sx = (240 - small.shape[1]) // 2
        image[40:48, sx:sx + small.shape[1]] = small
        image[52:54, 24:216] = 3
        image[:16, 224:] = 0           # the system ROM's map skips this column
        return image

    def write_fix(self, game_dir, roms_dir=None):
        """Only the S1 ROM (the build's sfix step): C-ROMs and the header stay as they are."""
        game_dir = Path(game_dir)
        roms = Path(roms_dir) if roms_dir else ROOT / "roms" / game_dir.name
        roms.mkdir(parents=True, exist_ok=True)
        fix = self._fix_rom()
        for dest in (game_dir / "artbox", roms):
            dest.mkdir(parents=True, exist_ok=True)
            (dest / f"{self.game_id}-s1.s1").write_bytes(fix)

    def write(self, game_dir, roms_dir=None, header_name=None):
        game_dir = Path(game_dir)
        art = game_dir / "artbox"
        out = art / "generated"
        out.mkdir(parents=True, exist_ok=True)
        roms = Path(roms_dir) if roms_dir else ROOT / "roms" / game_dir.name
        roms.mkdir(parents=True, exist_ok=True)
        lane1 = bytearray(LANE)
        lane2 = bytearray(LANE)
        tile = FIRST_TILE
        manifest, arrays, rows = [], [], []
        P = self.prefix
        for aid, a in enumerate(self.assets, 1):
            h, w = a["indices"].shape
            fw, fh = a["frame"]
            strips, trows = fw // 16, fh // 16
            frames = (w // fw) * (h // fh)
            per = strips * trows
            count = per * frames
            if tile < BOOT_END and tile + count > BOOT_START:
                tile = BOOT_END
            if tile + count > 0xFFFD:
                raise ValueError(f"{a['name']}: C-ROM full")
            # frame by frame, each frame's tiles row-major
            maps = []
            first = tile
            for fy in range(h // fh):
                for fx in range(w // fw):
                    idx = a["indices"][fy * fh:(fy + 1) * fh, fx * fw:(fx + 1) * fw]
                    c1, c2 = encode_image(idx, per)
                    if not np.array_equal(decode_image(c1, c2, fw, fh), idx):
                        raise ValueError(f"{a['name']}: tile round trip failed")
                    lane1[tile * 64:(tile + per) * 64] = c1
                    lane2[tile * 64:(tile + per) * 64] = c2
                    tile += per
                    maps.extend(int(v) + a["bank"] for v in
                                a["assignment"][fy * trows:(fy + 1) * trows, fx * strips:(fx + 1) * strips].reshape(-1))
            name = a["name"]
            cname = f"{P}_{name}"
            words = [v for p in a["palettes"] for v in palette_words(p)]
            arrays.append(f"static const uint16_t {cname}_pal[{len(words)}] = {{" + ",".join(f"0x{v:04X}u" for v in words) + "};")
            arrays.append(f"static const uint8_t {cname}_map[{len(maps)}] = {{" + ",".join(f"{v}u" for v in maps) + "};")
            vis = a["rgba"][:fh, :fw, 3] >= 128
            ys, xs = np.where(vis) if vis.any() else (np.array([0]), np.array([0]))
            ktype = 2 if a["kind"] == "background" else 1
            rows.append(f"    {{{aid}u,{ktype}u,{a['bank']}u,{first}u,{first + count - 1}u,{strips}u,{trows}u,{trows}u,"
                        f"{strips}u,{int(xs.min())},{int(ys.min())},{int(xs.max()) + 1 - int(xs.min())}u,"
                        f"{int(ys.max()) + 1 - int(ys.min())}u,{cname}_map}},")
            preview = np.dstack((reconstruct(a["indices"], a["palettes"], a["assignment"]),
                                 np.where(a["indices"] > 0, 255, 0))).astype(np.uint8)
            Image.fromarray(preview).save(out / f"{name}.png")
            manifest.append(dict(id=aid, name=name, kind=a["kind"], tile_base=first, tiles=count, frames=frames,
                                 frame_tiles=per, strips=strips, rows=trows, bank=a["bank"], banks=a["banks"],
                                 width=w, height=h))
        # the console system ROM's eye-catcher logo
        lo, hi = encode_image(self._logo(), 60)
        lo = lo[:14 * 64] + lo[15 * 64:]
        hi = hi[:14 * 64] + hi[15 * 64:]
        lane1[BOOT_START * 64:BOOT_START * 64 + len(lo)] = lo
        lane2[BOOT_START * 64:BOOT_START * 64 + len(hi)] = hi
        # the engine's utility tiles fill the lanes' last three tiles
        import io
        s1, s2 = io.BytesIO(bytes(lane1[:0xFFFD * 64])), io.BytesIO(bytes(lane2[:0xFFFD * 64]))
        s1.seek(0, 2)
        s2.seek(0, 2)
        write_utility_tiles(s1, s2)
        for stream, part in ((s1, "c1"), (s2, "c2")):
            raw = stream.getvalue()
            if len(raw) != LANE:
                raise ValueError(f"{part}: {len(raw)} bytes, expected {LANE}")
            final = np.frombuffer(raw, dtype=np.uint8).reshape(-1, 2)[:, ::-1].tobytes()
            for dest in (art, roms):
                (dest / f"{self.game_id}-{part}.{part}").write_bytes(final)
        fix = self._fix_rom()
        for dest in (art, roms):
            (dest / f"{self.game_id}-s1.s1").write_bytes(fix)
        guard = f"{P.upper()}_ASSETS_H"
        header = [f"/* Generated by tools/ng_artpack.py for {self.title}: do not edit. */", f"#ifndef {guard}",
                  f"#define {guard}", "#include <stdint.h>", '#include "sdk/2d_engine/ng_art_asset.h"', *arrays,
                  f"static const NGArtAsset {P}_art[] = {{", *rows, "};"]
        header += [f"static const uint16_t * const {P}_art_pal[] = {{"] + [f"    {P}_{m['name']}_pal," for m in manifest] + ["};"]
        header.append(f"static const uint8_t {P}_art_banks[] = {{" + ",".join(f"{m['banks']}u" for m in manifest) + "};")
        header.append(f"static const uint8_t {P}_art_frames[] = {{" + ",".join(f"{m['frames']}u" for m in manifest) + "};")
        header.append(f"#define {P.upper()}_ART_COUNT {len(manifest)}u")
        for m in manifest:
            header.append(f"#define {P.upper()}_ART_{m['name'].upper()} {m['id']}u")
        header.append(f"#define {P.upper()}_FIRST_FREE_BANK {self.next_bank}u")
        header += ["#endif", ""]
        (out / (header_name or f"{P}_assets.h")).write_text("\n".join(header), encoding="ascii")
        (out / "assets.json").write_text(json.dumps(dict(prefix=P, tiles_used=tile, banks_used=[self.first_bank, self.next_bank - 1],
                                                         assets=manifest), indent=1) + "\n", encoding="ascii")
        return manifest
