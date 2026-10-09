"""Build Sky Lance directly into native C1/C2 and S1 ROMs and C tables."""

import argparse
import io
import json
from pathlib import Path
import sys

import numpy as np
from PIL import Image, ImageEnhance, ImageFilter, ImageDraw

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
sys.path.insert(0, str(ROOT / "artbox"))
from palette_banks import fit_palette, palette_words, quantize, reconstruct
from tile_codec import encode_image, decode_image, write_utility_tiles
from boot_assets import BOOT_C_BANK, fix_rom, font, logo_pixels, text_pixels

ART = GAME / "artbox"
OUT = ART / "generated"
SOURCES = ART / "in"
ROMS = ROOT / "roms/skylance"
EXPLOSION_COLORS = np.asarray([
    (34, 36, 42), (56, 56, 63), (81, 74, 78), (109, 92, 83),
    (137, 72, 44), (174, 63, 28), (211, 77, 25), (239, 110, 26),
    (255, 146, 39), (255, 181, 59), (255, 210, 84), (255, 234, 132),
    (255, 248, 203), (207, 147, 80), (153, 109, 69),
], dtype=np.uint8)

# IDs 1..29 keep the original game's roster stable. Terrain palettes are
# shared only between mutually exclusive scenes; craft/enemy banks coexist.
ASSETS = [
    ("coast", "backgrounds/background_coast_city.png", 16, 16),
    ("mountain", "backgrounds/background_sky_mountains.png", 16, 16),
    ("p1_pilot", "characters/sprite_p1_pilot.png", 32, 4),
    ("p1_plane", "characters/sprite_p1_plane.png", 44, 4),
    ("p2_pilot", "characters/sprite_p2_pilot.png", 36, 4),
    ("p2_plane", "characters/sprite_p2_plane.png", 48, 4),
    ("p3_pilot", "characters/sprite_p3_pilot.png", 40, 4),
    ("p3_plane", "characters/sprite_p3_plane.png", 52, 4),
    ("shot_player", "characters/sprite_shot_player.png", 93, 1),
    ("bomber", "npcs/opponent_bomber_olive.png", 72, 3),
    ("cathedral", "npcs/opponent_boss_crimson_cathedral.png", 56, 16),
    ("gold_core", "npcs/opponent_boss_gold_core.png", 56, 16),
    ("heli_carrier", "npcs/opponent_boss_heli_carrier.png", 56, 16),
    ("battleship", "npcs/opponent_boss_navy_battleship.png", 56, 16),
    ("red_fortress", "npcs/opponent_boss_red_fortress.png", 56, 16),
    ("stealth", "npcs/opponent_boss_stealth_bomber.png", 56, 16),
    ("tank_fort", "npcs/opponent_boss_tank_fortress.png", 56, 16),
    ("drone", "npcs/opponent_drone_red.png", 75, 3),
    ("fighter", "npcs/opponent_fighter_grayred.png", 78, 3),
    ("heli", "npcs/opponent_heli_gunship.png", 81, 3),
    ("interceptor", "npcs/opponent_interceptor_blackgold.png", 84, 3),
    ("missile_boat", "npcs/opponent_missile_boat.png", 87, 3),
    ("shot_orb", "npcs/opponent_shot_orb.png", 94, 1),
    ("shot_ring", "npcs/opponent_shot_ring.png", 95, 1),
    ("tank", "npcs/opponent_tank_camo.png", 90, 3),
    ("item_life", "zzzz_pickups/item_life.png", 96, 1),
    ("item_missile", "zzzz_pickups/item_missile.png", 97, 1),
    ("item_speed", "zzzz_pickups/item_speed.png", 98, 1),
    ("open_sea", "zzzz_terrain/background_open_sea.png", 16, 16),
]


def sprite(path, box):
    source = Image.open(path).convert("RGBA")
    alpha = np.asarray(source.getchannel("A")) >= 128
    ys, xs = np.where(alpha)
    if not len(xs):
        raise ValueError(f"Empty sprite: {path}")
    source = source.crop((int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1))
    ratio = min((box[0] - 4) / source.width, (box[1] - 4) / source.height)
    size = (max(1, round(source.width * ratio)), max(1, round(source.height * ratio)))
    source = source.resize(size, Image.Resampling.LANCZOS)
    source = source.filter(ImageFilter.UnsharpMask(radius=0.55, percent=145, threshold=2))
    canvas = Image.new("RGBA", box)
    canvas.paste(source, ((box[0] - size[0]) // 2, (box[1] - size[1]) // 2))
    pixels = np.asarray(canvas).copy()
    pixels[:, :, 3] = np.where(pixels[:, :, 3] >= 128, 255, 0)
    # Fill only enclosed one-to-three-pixel pinholes. Outside transparency
    # and intentional large gaps between wings/limbs remain untouched.
    visible = pixels[:, :, 3] > 0
    seen = np.zeros(visible.shape, dtype=bool)
    for y, x in zip(*np.where(~visible)):
        if seen[y, x]:
            continue
        pending, component, border = [(y, x)], [], False
        seen[y, x] = True
        while pending:
            py, px = pending.pop()
            component.append((py, px))
            border |= py in (0, box[1]-1) or px in (0, box[0]-1)
            for ny, nx in ((py-1, px), (py+1, px), (py, px-1), (py, px+1)):
                if 0 <= ny < box[1] and 0 <= nx < box[0] and not visible[ny, nx] and not seen[ny, nx]:
                    seen[ny, nx] = True
                    pending.append((ny, nx))
        if border or len(component) > 3:
            continue
        for py, px in component:
            neighbours = [pixels[ny, nx, :3] for ny in range(py-1, py+2)
                          for nx in range(px-1, px+2)
                          if 0 <= ny < box[1] and 0 <= nx < box[0] and visible[ny, nx]]
            if neighbours:
                pixels[py, px, :3] = np.median(neighbours, axis=0).astype(np.uint8)
                pixels[py, px, 3] = 255
    return Image.fromarray(pixels)


def terrain(path, x=96, tint=(1.0, 1.0, 1.0), structures=False):
    image = Image.open(path).convert("RGB").resize((512, 512), Image.Resampling.LANCZOS)
    x = min(x, 160)
    image = image.crop((x, 0, x + 352, 512))
    image = ImageEnhance.Contrast(image).enhance(1.06)
    pixels = np.asarray(image).astype(np.float32) * np.asarray(tint)
    image = Image.fromarray(np.clip(pixels, 0, 255).astype(np.uint8)).convert("RGBA")
    if structures:
        draw = ImageDraw.Draw(image)
        for y, bx in ((96, 46), (256, 245), (384, 72)):
            draw.rectangle((bx + 3, y + 3, bx + 37, y + 26), fill="#152329")
            draw.rectangle((bx, y, bx + 33, y + 23), fill="#76807b", outline="#263d3c")
            draw.rectangle((bx+8, y+7, bx+26, y+17), fill="#35494d", outline="#9daea1")
            draw.rectangle((bx+11, y+9, bx+23, y+13), fill="#7ea7ad")
            draw.line((bx-2, y+27, bx+39, y+27), fill="#a6aa86", width=2)
            draw.line((bx + 2, y + 2, bx + 31, y + 2), fill="#bac4b7")
            for k in range(5, 30, 6):
                draw.line((bx + k, y + 4, bx + k, y + 19), fill="#465956")
    # A symmetric junction strip makes both edges meet without reflecting
    # the entire world. Quantisation also gives those rows one bank map.
    pixels = np.asarray(image).copy()
    junction = ((pixels[0].astype(np.uint16) + pixels[-1]) // 2).astype(np.uint8)
    for row in range(24):
        mix = (24 - row) / 24
        for target in (row, 511 - row):
            pixels[target] = np.rint(pixels[target] * (1 - mix) + junction * mix).astype(np.uint8)
    return Image.fromarray(pixels)


def title(craft):
    image = terrain(SOURCES / ASSETS[1][1]).crop((16, 80, 336, 304))
    image = ImageEnhance.Brightness(image).enhance(0.58)
    draw = ImageDraw.Draw(image)
    for y in (32, 83):
        draw.line((19, y, 301, y), fill="#26414a", width=3)
        draw.line((22, y, 297, y), fill="#92c7cc")
    # Pixel-stepped italic lettering, bevel, extrusion and metal bands.
    raw = text_pixels("SKY LANCE", 4)
    slanted = np.zeros((raw.shape[0], raw.shape[1]+8), dtype=np.uint8)
    for y in range(raw.shape[0]):
        offset = (raw.shape[0]-1-y)//4
        slanted[y, offset:offset+raw.shape[1]] = raw[y]*255
    mask = Image.fromarray(slanted)
    outline = mask.filter(ImageFilter.MaxFilter(5))
    image.paste("#06151d", (14, 47), outline)
    image.paste("#b98532", (12, 43), outline)
    image.paste("#f9e3a8", (12, 40), mask.filter(ImageFilter.MaxFilter(3)))
    metal = Image.new("RGBA", mask.size)
    ink = ImageDraw.Draw(metal)
    bands = ("#fff5d3", "#f6d172", "#b97f29", "#fdf7db", "#74d7df", "#277caa", "#153e63", "#aadce5")
    for y in range(mask.height):
        ink.line((0, y, mask.width, y), fill=bands[y//4])
    image.paste(metal, (12, 40), mask)
    tagline = Image.fromarray(text_pixels("SKY LANCE SQUADRON") * 255)
    image.paste("#e4edf0", (92, 90), tagline)
    for x in (87, 230):
        draw.polygon(((x, 157), (x+6, 157), (160, 94)), fill="#87bac3")
    for i, plane in enumerate(craft):
        if i != 1:
            image.alpha_composite(plane, (41 if i == 0 else 229, 131))
    lead = sprite(SOURCES / ASSETS[5][1], (80, 80))
    image.alpha_composite(lead, (120, 103))
    draw.rectangle((0, 186, 319, 223), fill="#0b1923")
    draw.line((30, 185, 289, 185), fill="#779f9f")
    copyright = "(C)1996 (+30) EAGLE SOFTWARE"
    mask = Image.fromarray(text_pixels(copyright) * 255)
    image.paste("#ead89a", ((320-mask.width)//2, 210), mask)
    return image


def clouds():
    image = Image.new("RGBA", (352, 512))
    draw = ImageDraw.Draw(image)
    for x, y in ((32, 54), (240, 197), (83, 357)):
        for ox, oy, rx, ry in ((0, 0, 24, 7), (15, -5, 15, 9), (-13, -3, 12, 6)):
            draw.ellipse((x+ox-rx, y+oy-ry, x+ox+rx, y+oy+ry), fill="#668e9d")
        draw.ellipse((x-22, y-6, x+15, y+2), fill="#adc2c6")
        draw.ellipse((x+5, y-10, x+24, y-1), fill="#d5ded8")
    return image


def emit_array(name, values, ctype="uint16_t"):
    body = ",".join(str(int(v)) for v in np.asarray(values).flatten())
    return f"static const {ctype} {name}[] = {{{body}}};"


def explosion(frame):
    image = Image.new("RGBA", (32, 32))
    draw = ImageDraw.Draw(image)
    radius = (4, 8, 13, 12, 10, 7)[frame]
    color = lambda i: tuple(int(v) for v in EXPLOSION_COLORS[i]) + (255,)
    # Uneven pixel clusters replace a smooth, perfectly circular flash.
    for ox, oy, scale in ((0, 0, 1), (-6, -4, 0.45), (5, -5, 0.5), (-4, 5, 0.55), (6, 4, 0.4)):
        cx, cy = 16 + ox, 16 + oy
        r = max(2, round(radius * scale))
        draw.ellipse((cx-r, cy-r, cx+r, cy+r), fill=color(1 if frame > 3 else 5))
        r = max(1, r - 2)
        draw.ellipse((cx-r, cy-r, cx+r, cy+r), fill=color(3 if frame > 3 else 8))
        if frame < 4:
            r = max(1, r - 2)
            draw.ellipse((cx-r, cy-r, cx+r, cy+r), fill=color(12 - frame))
    if frame >= 3:
        draw.rectangle((13, 13, 18, 18), fill=(0, 0, 0, 0))
    if frame in (1, 2, 3):
        for x, y in ((3, 5), (28, 8), (5, 27), (27, 26)):
            draw.rectangle((x, y, x+1, y+2), fill=color(10))
    return image


def build_fix():
    rom = fix_rom()
    names = ["1P", "2P", "3P", "HI", "SCORE"] + [f"digit_{i}" for i in range(10)]
    names += ["energy", "energy_empty", "life"]
    rows, palettes = [], []
    tile = 0xE00
    for name in names:
        image = np.asarray(Image.open(ART / "infix" / f"fix_{name}.png").convert("RGBA"))
        h, w = image.shape[:2]
        if w % 8 or h % 8:
            raise ValueError("FIX art must be aligned to 8 pixels")
        visible = image[:, :, 3] >= 128
        palette = np.zeros((16, 3), dtype=np.uint8)
        palette[1:] = fit_palette(image[:, :, :3][visible])
        distance = ((image[:, :, :3].astype(np.int32)[:, :, None] - palette[None, None, 1:].astype(np.int32)) ** 2).sum(axis=3)
        indices = np.where(visible, distance.argmin(axis=2) + 1, 0).astype(np.uint8)
        rows.append(f"    {{{tile}u,{w // 8}u,{h // 8}u,0u}},")
        palettes.append(palette_words(palette))
        for y in range(0, h, 8):
            for x in range(0, w, 8):
                rom[tile * 32:(tile + 1) * 32] = font.encode_tile(indices[y:y + 8, x:x + 8])
                tile += 1
    if tile > 0x1000:
        raise ValueError("HUD exceeds private FIX banks")
    header = ["/* Generated by Sky Lance's direct art builder. */", "#ifndef INFIX_PALETTES_H", "#define INFIX_PALETTES_H", "#include <stdint.h>", f"#define INFIX_IMAGE_COUNT {len(names)}u", "typedef struct { uint16_t tile_base; uint8_t cols, rows, pal_bank; } InfixImage;", "static const InfixImage INFIX_IMAGES[] = {", *rows, "};", "static const uint16_t INFIX_PALETTES[][16] = {"]
    header += ["    {" + ",".join(str(v) for v in pal) + "}," for pal in palettes]
    header += ["};", "#endif", ""]
    (ART / "infix_palettes.h").write_text("\n".join(header), encoding="ascii")
    for destination in (ART, ROMS):
        (destination / "779-s1.s1").write_bytes(rom)


def build():
    OUT.mkdir(parents=True, exist_ok=True)
    ROMS.mkdir(parents=True, exist_ok=True)
    streams = [io.BytesIO(), io.BytesIO()]
    arrays, assets, metadata, manifest = [], [], [], []
    boot_start = BOOT_C_BANK * 256 * 64
    boot_end = boot_start + 256 * 64
    images = []
    for name, path, bank, limit in ASSETS:
        if name in ("coast", "mountain", "open_sea"):
            image = terrain(SOURCES / path)
        else:
            box = (80, 96) if "pilot" in name else (112, 112) if bank == 56 else (16, 16) if name.startswith(("shot", "item")) else (48, 48)
            # The impact ring keeps enough pixels for its expanding outline.
            if name == "shot_ring":
                box = (32, 32)
            image = sprite(SOURCES / path, box)
        images.append((name, image, bank, limit))
    images += [
        ("convoy", terrain(SOURCES / ASSETS[1][1], 0, (1.05, 0.94, 0.82), True), 16, 16),
        ("outpost", terrain(SOURCES / ASSETS[1][1], 172, (0.83, 1.03, 0.92), True), 16, 16),
        ("armored", terrain(SOURCES / ASSETS[1][1], 12, (0.98, 0.91, 0.80), True), 16, 16),
        ("citadel", terrain(SOURCES / ASSETS[0][1], 170, (0.88, 0.85, 1.06), True), 16, 16),
        ("title", title([images[i][1] for i in (3, 5, 7)]), 16, 16),
    ]
    for i in (3, 5, 7):
        name, plane, bank, limit = images[i]
        for direction, factor in (("left", 0.78), ("right", 0.78)):
            narrow = plane.resize((round(48 * factor), 48), Image.Resampling.LANCZOS)
            banked = Image.new("RGBA", (48, 48))
            banked.paste(narrow, (2 if direction == "left" else 48 - narrow.width - 2, 0))
            images.append((name + "_" + direction, banked, bank, limit))
    images += [(f"explosion_{i}", explosion(i), 99, 1) for i in range(6)]
    images.append(("clouds", clouds(), 100, 2))

    for aid, (name, image, bank, limit) in enumerate(images, 1):
        print(f"[{aid}/{len(images)}] {name} {image.size}", flush=True)
        rgba = np.asarray(image).copy()
        indices, palettes, assignment = quantize(
            rgba, limit, master=EXPLOSION_COLORS if name.startswith("explosion_") else None,
            dither="none")
        if image.height == 512 and name != "clouds":
            # Use the same banks on both edge strips, not merely the last
            # tile. Reindex all pixels affected by the bank-map change.
            bank_map = assignment.reshape(image.height // 16, image.width // 16)
            for col in range(image.width // 16):
                selected = bank_map[0, col]
                colors = np.asarray(palettes[selected][1:], dtype=np.int32)
                area = rgba[-16:, col * 16:(col + 1) * 16, :3].astype(np.int32)
                distance = ((area[:, :, None, :] - colors) ** 2).sum(axis=3)
                indices[-16:, col * 16:(col + 1) * 16] = distance.argmin(axis=2) + 1
                bank_map[-1, col] = selected
            indices[-1] = indices[0]
        count = image.width * image.height // 256
        if streams[0].tell() < boot_end and streams[0].tell() + count * 64 > boot_start:
            for stream in streams:
                stream.write(bytes(boot_end - stream.tell()))
        tile = streams[0].tell() // 64
        lanes = encode_image(indices, count)
        for stream, lane in zip(streams, lanes):
            stream.write(lane)
        if not np.array_equal(decode_image(*lanes, image.width, image.height), indices):
            raise ValueError(f"Tile round trip failed: {name}")
        prefix = f"sky_asset_{aid}"
        arrays.append(emit_array(prefix + "_pal", [v for p in palettes for v in palette_words(p)]))
        arrays.append(emit_array(prefix + "_map", assignment + bank, "uint8_t"))
        cols, rows = image.width // 16, image.height // 16
        bg = image.height == 512
        ys, xs = np.where(rgba[:, :, 3] >= 128)
        x0, y0 = int(xs.min()), int(ys.min())
        cw, ch = int(xs.max()) + 1 - x0, int(ys.max()) + 1 - y0
        assets.append(f"    {{{aid}u,{2 if bg else 1}u,{bank}u,{tile}u,{tile + count - 1}u,{cols}u,{rows}u,{rows}u,{cols}u,0,0,{cw}u,{ch}u,{prefix}_map}},")
        metadata.append(f"    {{{tile}u,{bank}u,{cols}u,{rows}u,0u,0u,{x0}u,{y0}u,{cw}u,{ch}u,{0 if bg else 1}u,0u,{cols}u}},")
        preview = np.dstack((reconstruct(indices, palettes, assignment), np.where(indices > 0, 255, 0))).astype(np.uint8)
        Image.fromarray(preview).save(OUT / f"{name}.png")
        manifest.append(dict(id=aid, name=name, tile_base=tile, tile_last=tile + count - 1, width=image.width, height=image.height, bank_base=bank, palette_count=len(palettes)))
    end = streams[0].tell()
    lo, hi = encode_image(logo_pixels(), 60)
    # Wire the BIOS's missing first-row column exactly like Maiya's logo.
    for stream, lane in zip(streams, (lo, hi)):
        if stream.tell() < boot_start:
            stream.write(bytes(boot_start - stream.tell()))
        stream.seek(boot_start)
        lane = lane[:14 * 64] + lane[15 * 64:]
        stream.write(lane.ljust(256 * 64, b"\0"))
        stream.seek(max(end, boot_end))
    write_utility_tiles(*streams)
    for stream, part in zip(streams, ("c1", "c2")):
        final = np.frombuffer(stream.getvalue(), dtype=np.uint8).reshape(-1, 2)[:, ::-1].tobytes()
        if len(final) != 0x400000 or final[0xFFFF * 64:] != bytes(64):
            raise ValueError("Invalid hardware tile reservation")
        for destination in (OUT, ART, ROMS):
            (destination / f"779-{part}.{part}").write_bytes(final)
    header = ["/* Generated by games/skylance/tools/art.py. */", "#ifndef SKY_ASSETS_H", "#define SKY_ASSETS_H", "#include <stdint.h>", '#include "sdk/2d_engine/ng_art_asset.h"', *arrays, "static const NGArtAsset sky_art_assets[] = {", *assets, "};", "static const uint16_t * const sky_art_palettes[] = {"]
    header += [f"    sky_asset_{i}_pal," for i in range(1, len(images) + 1)]
    header += ["};", emit_array("sky_art_bank_counts", [m["palette_count"] for m in manifest], "uint8_t"), f"#define SKY_ART_COUNT {len(images)}u", "#endif", ""]
    (OUT / "sky_assets.h").write_text("\n".join(header), encoding="ascii")
    meta = ["/* Generated by Sky Lance's direct art builder. */", "#ifndef ARTBOX_SPRITE_META_H", "#define ARTBOX_SPRITE_META_H", "#include <stdint.h>", "typedef struct { uint16_t tile_base; uint8_t palette_bank, strips, active_rows, tile_col_start, tile_row_start, x_pad, y_pad; uint16_t content_width, content_height; uint8_t mode, category, tile_stride; } NGSpriteAssetMeta;", f"#define NG_ASSET_META_COUNT {len(images)}u", "static const NGSpriteAssetMeta g_ng_asset_meta[] = {", *metadata, "};", "#endif", ""]
    (ART / "sprite_meta.h").write_text("\n".join(meta), encoding="ascii")
    (OUT / "assets.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="ascii")
    print(f"Direct C-ROMs: {len(images)} assets, 4 MiB per lane", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fix-only", action="store_true")
    args = parser.parse_args()
    ROMS.mkdir(parents=True, exist_ok=True)
    if not args.fix_only:
        build()
    build_fix()


if __name__ == "__main__":
    main()
