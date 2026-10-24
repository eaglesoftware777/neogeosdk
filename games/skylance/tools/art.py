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
from boot_assets import BOOT_C_BANK, GAME_FIX_BASE, fix_rom, font, logo_pixels, text_pixels
from alpha_audit import inspect as inspect_alpha
from redrawn import load as load_redrawn
from redrawn import MASTER
from native_art import load as load_native
from painted import load as load_painted
from pixel_scenery import route as pixel_route, cockpit as pixel_cockpit, chaff, radar, miniboss, indexed_route, cloud

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
    ("castle", "npcs/opponent_boss_crimson_castle.png", 56, 16),
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
    # The orb, ring and three old item drawings keep their IDs but the game
    # never shows them: they share bank 94, leaving 95-99 to the instruments
    # and the explosions.
    ("shot_orb", "npcs/opponent_shot_orb.png", 94, 1),
    ("shot_ring", "npcs/opponent_shot_ring.png", 94, 1),
    ("tank", "npcs/opponent_tank_camo.png", 90, 3),
    ("item_life", "zzzz_pickups/item_life.png", 94, 1),
    ("item_missile", "zzzz_pickups/item_missile.png", 94, 1),
    ("item_speed", "zzzz_pickups/item_speed.png", 94, 1),
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
    # Preserve mechanical openings unless a defect has an explicit repair
    # decision. The audit tool writes candidates without modifying sources.
    result, _ = inspect_alpha(Image.fromarray(pixels), repair=False)
    return result


PICKUP_COLORS = ("#63baff", "#ffa456", "#9fe8f0", "#83e65c", "#f1da61",
                 "#cfa6ff", "#ff8da7", "#ffe77b", "#f4faff")
PICKUP_OUTLINE, PICKUP_WHITE = (16, 22, 38), (244, 250, 255)


def pickup_shades(index):
    """A badge's two shades: its colour lightened for the top half, deepened below."""
    base = np.array([int(PICKUP_COLORS[index][k:k + 2], 16) for k in (1, 3, 5)], float)
    return tuple(int(v) for v in base * 0.6 + 255 * 0.4), tuple(int(v) for v in base * 0.62)


def pickup(index):
    """A glossy octagonal badge: two shades of its colour, a white highlight and a
    white symbol in the HUD face with a dark shadow, all inside a dark outline."""
    from hud_art import glyph
    light, dark = pickup_shades(index)
    image = Image.new("RGBA", (16, 16))
    draw = ImageDraw.Draw(image)
    octagon = ((4, 1), (11, 1), (14, 4), (14, 11), (11, 14), (4, 14), (1, 11), (1, 4))
    draw.polygon(octagon, fill=dark, outline=PICKUP_OUTLINE)
    upper = Image.new("RGBA", (16, 16))
    ImageDraw.Draw(upper).polygon(((4, 2), (11, 2), (13, 4), (13, 7), (2, 7), (2, 4)), fill=light)
    image.alpha_composite(upper)
    draw.line((5, 3, 8, 3), fill=PICKUP_WHITE)
    a = np.array(image)
    if index == 5:
        mark = np.zeros((7, 5), bool)
        for y, row in enumerate(["..#..", ".###.", "#####", "#####", ".###.", "..#..", "..#.."]):
            mark[y] = [c == "#" for c in row]
    elif index == 7:
        mark = np.array([[c == "#" for c in r] for r in [".....", "..#..", ".###.", "#####", ".###.", ".#.#.", "#...#"]])
    else:
        mark = glyph("PMSFTXLXH"[index])
    y0, x0 = 5, 5
    ink, shade = PICKUP_WHITE, PICKUP_OUTLINE
    if sum(light) > 690:                 # a pale badge takes a dark symbol
        ink, shade = PICKUP_OUTLINE, light
    for y in range(7):
        for x in range(5):
            if mark[y, x]:
                if not mark[min(y + 1, 6), min(x + 1, 4)] or y == 6 or x == 4:
                    a[y0 + y + 1, x0 + x + 1, :3] = shade
                a[y0 + y, x0 + x, :3] = ink
    return Image.fromarray(a, "RGBA")


def weapon(pilot, power):
    image = Image.new("RGBA", (16, 16))
    draw = ImageDraw.Draw(image)
    color = ("#62b9ff", "#77ef9a", "#ffae56")[pilot]
    half = 1 + power // 2
    draw.polygon(((8, 0), (8 + half, 4), (8 + half, 12), (8, 15),
                  (8 - half, 12), (8 - half, 4)), fill=color)
    draw.line((8, 3, 8, 11), fill="#faffff")
    if power >= 1:
        draw.point((8 - half - 1, 8), fill=color)
        draw.point((8 + half + 1, 8), fill=color)
    if power >= 3:
        draw.line((4, 5, 4, 10), fill=color)
        draw.line((12, 5, 12, 10), fill=color)
    if power == 4:
        draw.line((6, 1, 10, 1), fill="#faffff")
    return image


def attitude(bank):
    """Native blue/brown artificial horizon with a fixed aircraft reference."""
    image = Image.new("RGBA", (16, 16))
    draw = ImageDraw.Draw(image)
    slope = (-1.0, -.577, -.268, 0, .268, .577, 1.0)[bank]
    for y in range(2, 14):
        for x in range(2, 14):
            if (x-8)**2 + (y-8)**2 <= 5**2:
                horizon = 8 + (x-8)*slope
                draw.point((x, y), fill="#3265a9" if y < horizon else "#785235")
                if abs(y-horizon) < .65:
                    draw.point((x, y), fill="#eaf2ed")
    draw.ellipse((1, 1, 15, 15), outline="#8797a1")
    draw.line((3, 8, 6, 8, 8, 10, 10, 8, 13, 8), fill="#f4cd65")
    draw.point((8, 2), fill="#f4cd65")
    return image


def civilian(index):
    image = Image.new("RGBA", (16, 16))
    draw = ImageDraw.Draw(image)
    draw.rectangle((5, 2, 10, 13), fill="#cdd9de" if not index else "#ba8057", outline="#172738")
    draw.rectangle((6, 4, 9, 6), fill="#356a8e")
    draw.rectangle((6, 10, 9, 11), fill="#356a8e")
    for x in (4, 11):
        draw.line((x, 4, x, 6), fill="#172738")
        draw.line((x, 10, x, 12), fill="#172738")
    return image


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


def clouds(sprite_image):
    image = Image.new("RGBA", (352, 512))
    for x, y in ((28, 54), (225, 197), (77, 357)):
        image.paste(sprite_image, (x, y))
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
    # Sky Lance's own text face on its ASCII page (the BIOS keeps the low one),
    # and its counter digits, labels, cells and life pip redrawn to match.
    from hud_art import build as build_hud_art, font_tiles
    for code, glyph in font_tiles().items():
        tile_id = GAME_FIX_BASE + code
        rom[tile_id * 32:(tile_id + 1) * 32] = font.encode_tile(glyph)
    build_hud_art()
    # Compact hull and boss meters occupy unused tiles below the text font:
    # one segment per cell with a gap after it. A full segment is lit on top
    # (ink entry 1), darker below (3) with a shadow foot (2); an empty one is
    # its hollow outline in the shadow colour.
    for filled in (False, True):
        glyph = np.zeros((8, 8), dtype=np.uint8)
        if filled:
            glyph[1:3, 0:7] = 1
            glyph[3:6, 0:7] = 3
            glyph[6, 0:7] = 2
        else:
            glyph[1, 0:7] = glyph[6, 0:7] = 2
            glyph[1:7, 0] = glyph[1:7, 6] = 2
        tile_id = 0xCE0 + int(filled)
        rom[tile_id * 32:(tile_id + 1) * 32] = font.encode_tile(glyph)
    names = ["1P", "2P", "3P", "HI", "SCORE"] + [f"digit_{i}" for i in range(10)]
    names += ["energy", "energy_empty", "life"]
    names += ["hud_" + n for n in ("power", "missile", "shield", "fuel", "core", "turbo")]
    # The six equipment badges share one FIX palette: outline, white, and two
    # shades for each badge's colour.
    shades = [c for i in (0, 1, 2, 3, 4, 5) for c in pickup_shades(i)]
    hud_palette = np.asarray([(0, 0, 0), PICKUP_OUTLINE, PICKUP_WHITE, *shades,
                              (128, 140, 156)], dtype=np.uint8)
    rows, palettes = [], []
    tile = 0xE00
    for name in names:
        hud = name.startswith("hud_")
        if hud:
            index = {"power": 0, "missile": 1, "shield": 2, "fuel": 3,
                     "core": 5, "turbo": 4}[name[4:]]
            image = np.asarray(pickup(index))
        else:
            image = np.asarray(Image.open(ART / "infix" / f"fix_{name}.png").convert("RGBA"))
        h, w = image.shape[:2]
        if w % 8 or h % 8:
            raise ValueError("FIX art must be aligned to 8 pixels")
        visible = image[:, :, 3] >= 128
        palette = hud_palette.copy() if hud else np.zeros((16, 3), dtype=np.uint8)
        if not hud:
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
    # Reopen indexed Aseprite sources on every art build.
    from aseprite.aircraft_pipeline import run as check_aircraft
    check_aircraft()
    from aseprite.production_pipeline import run as check_production
    check_production()
    native_overrides, native_extra, native_pivots, provenance, native_banks = load_native()
    # Painted full-colour frames replace native drawings of the same name.
    painted = load_painted()
    for name, (_, _, _, pivot) in painted.items():
        native_pivots[name] = pivot
        provenance.pop(name, None)
    redrawn, bank_poses = load_redrawn()
    route_names = ("mountain", "coast", "convoy", "open_sea", "outpost", "armored", "citadel")
    routes = {}
    for stage, name in enumerate(route_names):
        if stage == 0:
            from mountain_route import route as mountain_campaign
            routes[name] = mountain_campaign()
        else:
            routes[name] = pixel_route(stage)
        routes[name].save(OUT / f"authored_route_{stage}.png")
        redrawn[name] = routes[name].crop((0, 0, 352, 512))
    for pilot in range(3):
        redrawn[f"cockpit_{pilot}"] = pixel_cockpit(pilot)
    for name, path, bank, limit in ASSETS:
        if name in redrawn:
            image = redrawn[name]
        elif name in ("coast", "mountain", "open_sea"):
            image = terrain(SOURCES / path)
        else:
            box = (80, 96) if "pilot" in name else (112, 112) if bank == 56 else (16, 16) if name.startswith(("shot", "item")) else (48, 48)
            # The impact ring keeps enough pixels for its expanding outline.
            if name == "shot_ring":
                box = (32, 32)
            image = sprite(SOURCES / path, box)
        images.append((name, image, bank, limit))
    images += [
        (name, redrawn[name], 16, 16) for name in ("convoy", "outpost", "armored", "citadel")
    ] + [
        ("title", redrawn["title"], 16, 16),
    ]
    for pilot, i in enumerate((3, 5, 7)):
        name, plane, bank, limit = images[i]
        for direction, pose in (("left", 0), ("right", 6)):
            images.append((name + "_" + direction, bank_poses[pilot][pose], bank, limit))
    images += [(f"explosion_{i}", explosion(i), 99, 1) for i in range(6)]
    from redrawn import cells, native
    atmosphere = cells("atmosphere-smoke.png", 4, 3)
    cloud_frames = [clouds(cloud(i)) for i in range(7)]
    images.append(("clouds", cloud_frames[0], 100, 2))
    images += [(f"pickup_{name}", pickup(i), 105 + i, 1)
               for i, name in enumerate(("power", "missile", "shield", "fuel", "turbo",
                                         "core", "life", "star", "repair"))]
    images += [(f"weapon_{pilot}_{power}", weapon(pilot, power), 102 + pilot, 1)
               for pilot in range(3) for power in range(5)]
    for pilot in range(3):
        for suffix, pose in (("left_mid", 1), ("left_soft", 2),
                             ("right_soft", 4), ("right_mid", 5)):
            images.append((f"p{pilot+1}_plane_{suffix}", bank_poses[pilot][pose],
                           44 + pilot*4, 4))
    images += [(f"smoke_{i}", native(p, (32, 32)), 114, 1)
               for i, p in enumerate(atmosphere[7:11])]
    images += [(f"clouds_{i}", cloud_frames[i], 100, 2) for i in range(1, 7)]
    special_frames = [native(p, (64, 64)) for p in cells("strikes-v2.png", 4, 3)]
    images += [(f"special_{pilot}_{frame}", special_frames[pilot*4+frame],
                115+pilot, 1) for pilot in range(3) for frame in range(4)]
    special_palettes = [fit_palette(np.concatenate([
        np.asarray(p)[:, :, :3][np.asarray(p)[:, :, 3] > 0]
        for p in special_frames[pilot*4:pilot*4+4]])) for pilot in range(3)]
    images += [(name, redrawn[name], 118+i, 1) for i, name in enumerate(
        ("sam", "radar_truck", "submarine", "aa_bunker", "radar_site"))]
    images += [(f"hostile_{i}", redrawn[f"hostile_{i}"], 123, 1) for i in range(8)]
    images += [(f"cockpit_{i}", redrawn[f"cockpit_{i}"], 124+i, 1) for i in range(3)]
    images += [(f"attitude_{i}", attitude(i), 127, 1) for i in range(7)]
    images += [(f"maneuver_{pilot}_{i}", redrawn[f"maneuver_{pilot}_{i}"], 44+pilot*4, 4)
               for pilot in range(3) for i in range(4)]
    images += [(f"route_{stage}_{part}", routes[name].crop((0, part*512, 352, (part+1)*512)), 16, 16)
               for stage, name in enumerate(route_names) for part in range(1, 4)]
    images += [(f"civilian_{i}", civilian(i), 123, 1) for i in range(2)]
    images += [("chaff", chaff(), 127, 1)]
    images += [(f"radar_zone_{i}", radar(i), 123, 1) for i in range(2)]
    images += [(f"miniboss_{i}", miniboss(i), 75+i*3, 3) for i in range(3)]
    images += [(f"console_{pilot}_{g}_{boost}", pixel_cockpit(pilot,g,boost),124+pilot,1)
               for pilot in range(3) for g in range(4) for boost in range(2)]
    # Append new IDs after the complete legacy 1..193 roster.
    images += native_extra
    images += [(f"route_{stage}_{part}", routes[name].crop((0,part*512,352,(part+1)*512)),16,16)
               for stage,name in enumerate(route_names) for part in range(4,routes[name].height//512)]
    # Painted frames with new names (a strip's later frames) come last, so
    # no existing ID moves.
    known = {row[0] for row in images}
    images += [(name, image, bank, 1) for name, (image, _, bank, _) in painted.items() if name not in known]
    # A native drawing brings its own palette, so it also names its bank.
    images = [(name, painted[name][0], painted[name][2], limit) if name in painted
              else (name, native_overrides[name][0], native_banks[name], limit) if name in native_overrides
              else (name, image, bank, limit) for name, image, bank, limit in images]
    cockpit_palettes = [fit_palette(np.concatenate([
        np.asarray(pixel_cockpit(i,g,b))[:,:,:3][np.asarray(pixel_cockpit(i,g,b))[:,:,3]>0]
        for g in range(4) for b in range(2)])) for i in range(3)]
    hostile_palette = fit_palette(np.concatenate([
        np.asarray(p)[:, :, :3][np.asarray(p)[:, :, 3] > 0]
        for p in [redrawn[f"hostile_{i}"] for i in range(8)] + [civilian(i) for i in range(2)] + [radar(i) for i in range(2)]]))
    attitude_palette = fit_palette(np.concatenate([
        np.asarray(p)[:, :, :3][np.asarray(p)[:, :, 3] > 0] for p in [attitude(i) for i in range(7)] + [chaff()]]))
    enemy_palettes = {}
    for i, name in enumerate(("drone", "fighter", "heli")):
        sources = [redrawn[name], miniboss(i)]
        palette = fit_palette(np.concatenate([
            np.asarray(p)[:, :, :3][np.asarray(p)[:, :, 3] > 0] for p in sources]))
        enemy_palettes[name] = enemy_palettes[f"miniboss_{i}"] = palette
    route_tables = {}
    for stage, name in enumerate(route_names):
        indexed, colors, mapping = indexed_route(routes[name])
        if stage == 0:
            from aseprite.route_pipeline import roundtrip as check_route_document
            from mountain_route import SECTORS
            indexed, colors, mapping = check_route_document("stage_1_mountain",routes[name],indexed,colors,mapping,SECTORS)
        base = next(i for i, row in enumerate(images, 1) if row[0] == name)
        for part in range(routes[name].height//512):
            aid = base if not part else next(i for i,row in enumerate(images,1) if row[0]==f"route_{stage}_{part}")
            route_tables[aid] = (indexed[part*512:(part+1)*512], colors,
                                mapping[part*32:(part+1)*32])

    # Banking frames share one fitted bank set. Updating a pose must not
    # recolor live strips using another pose's palette RAM.
    pose_tables = {}
    for pilot, neutral in enumerate((3, 5, 7)):
        members = (neutral, 34 + pilot * 2, 35 + pilot * 2,
                   *range(71 + pilot * 4, 75 + pilot * 4),
                   *range(128 + pilot * 4, 132 + pilot * 4))
        combined = np.concatenate([np.asarray(images[i][1]) for i in members], axis=0)
        indexed, colors, mapping = quantize(combined, 4, dither="none")
        for row, member in enumerate(members):
            pose_tables[member + 1] = (indexed[row * 48:(row + 1) * 48],
                                      colors, mapping[row * 3:(row + 1) * 3])
    weapon_palettes = [fit_palette(np.concatenate([
        np.asarray(weapon(pilot, power))[:, :, :3][np.asarray(weapon(pilot, power))[:, :, 3] > 0]
        for power in range(5)])) for pilot in range(3)]

    for aid, (name, image, bank, limit) in enumerate(images, 1):
        print(f"[{aid}/{len(images)}] {name} {image.size}", flush=True)
        rgba = np.asarray(image).copy()
        if name in painted:
            indices, palettes, assignment = painted[name][1]
        elif name in native_overrides:
            indices, palettes, assignment = native_overrides[name][1]
        elif aid in route_tables:
            indices, palettes, assignment = route_tables[aid]
        elif aid in pose_tables:
            indices, palettes, assignment = pose_tables[aid]
        else:
            master = (np.array([[v, v, v] for v in range(20, 231, 15)],
                               dtype=np.uint8) if name.startswith("smoke_") else
                      EXPLOSION_COLORS if name.startswith("explosion_") else (
                weapon_palettes[int(name.split("_")[1])] if name.startswith("weapon_") else
                special_palettes[int(name.split("_")[1])] if name.startswith("special_") else
                hostile_palette if name.startswith(("hostile_", "civilian_", "radar_zone_")) else
                attitude_palette if name.startswith("attitude_") or name == "chaff" else None)
                      )
            if name in enemy_palettes:
                master = enemy_palettes[name]
            if name.startswith(("cockpit_", "console_")):
                master = cockpit_palettes[int(name.split('_')[1])]
            indices, palettes, assignment = quantize(rgba, limit, master=master, dither="none")
        if image.height == 512 and aid not in route_tables and not name.startswith("clouds"):
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
        manifest.append(dict(id=aid, name=name, tile_base=tile, tile_last=tile + count - 1, width=image.width, height=image.height, bank_base=bank, palette_count=len(palettes),
                             native_source=provenance.get(name), pivot=native_pivots.get(name)))
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
    meta[-2:-2] = ["static const int16_t g_ng_asset_pivots[][2] = {",
                    *[f"    {{{native_pivots.get(row[0], (-1,-1))[0]}, {native_pivots.get(row[0], (-1,-1))[1]}}}," for row in images], "};"]
    native_header = ["/* Native indexed Aseprite production assets. */", "#ifndef SKY_NATIVE_ASSETS_H", "#define SKY_NATIVE_ASSETS_H"]
    native_header += [f"#define SKY_{m['name'].upper()} {m['id']}u" for m in manifest if m['id'] > 193]
    native_header += ["static const uint16_t sky_route_asset_ids[7][8] = {"]
    for stage,name in enumerate(route_names):
        family=[next(i for i,row in enumerate(images,1) if row[0]==(name if part==0 else f"route_{stage}_{part}")) for part in range(routes[name].height//512)]
        native_header.append("    {"+",".join(str(i)+"u" for i in family+[0]*(8-len(family)))+"},")
    native_header += ["};", "static const uint8_t sky_route_page_counts[7] = {"+",".join(str(routes[n].height//256)+"u" for n in route_names)+"};"]
    # Open water on the two naval routes, a bit per 16x16 cell (bit c = column c):
    # boats and submarines spawn only where their hull and path are water.
    native_header += ["static const uint32_t sky_route_water[2][128] = {"]
    for name in ("coast", "open_sea"):
        a = np.asarray(routes[name])[:2048, :, :3].astype(int)
        wet = (a[:, :, 2] > a[:, :, 0] + 10).reshape(128, 16, 22, 16).mean(axis=(1, 3)) >= 0.9
        native_header.append("    {" + ",".join(f"0x{sum(1 << c for c in range(22) if wet[r, c]):06x}u" for r in range(128)) + "},")
    native_header += ["};", "#endif", ""]
    (OUT / "sky_native_assets.h").write_text("\n".join(native_header), encoding="ascii")
    (ART / "sprite_meta.h").write_text("\n".join(meta), encoding="ascii")
    (OUT / "assets.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="ascii")
    import subprocess
    subprocess.run([sys.executable,str(ROOT / "tools/check_skylance_native_art.py")],check=True)
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
