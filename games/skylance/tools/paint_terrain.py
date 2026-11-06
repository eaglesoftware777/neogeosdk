"""Stage 1's mountain route from painted panels.

assets/painted/sources/terrain_mountain_<k>.png are full-bleed paintings in
flight order: panel 0 is where the sortie starts (the bottom of the screen),
each next panel continues above the last. They are scaled to the route's
352-pixel width, stacked and cross-faded at the seams; the valley road
(world X=92) and the three target pads are painted over the result; the
whole map is fitted to the sixteen terrain palette banks, one per 16x16
tile; and its 256-row pages are laid out in the order the game streams
them (page 0, at the top of the route image, is shown first, at the bottom
of the screen, and each later page is drawn above the one before).

The route's last page is its boss's arena: sources/arena_<theme>.png, an
open painting with nothing standing in it (all water for the warship, an
empty desert for the tank fortress), laid over the top 256 rows of the
screen-order map and cross-faded into the route below. The road ends there.

The result, assets/painted/routes/stage_1.png, is what mountain_route.route()
returns when it exists.
"""
from pathlib import Path

import numpy as np
from PIL import Image
from scipy import ndimage as ndi

from palette_banks import fit_palette, quantize, reconstruct

ROOT = Path(__file__).resolve().parents[1] / 'assets/painted'
W, PAGE = 352, 256
SEAM = 48                                  # rows cross-faded between panels


def unmatte(rgb):
    """Fill any flat-magenta matte left at a painting's edge from the nearest painted pixel."""
    r, g, b = (rgb[:, :, i].astype(int) for i in range(3))
    matte = (r - g > 40) & (b - g > 40) & (abs(r - b) < 110)
    matte = ndi.binary_dilation(matte, iterations=3)       # and the blended fringe around it
    if not matte.any():
        return rgb
    _, (iy, ix) = ndi.distance_transform_edt(matte, return_indices=True)
    return rgb[iy, ix]


def panels(prefix):
    found = []
    k = 0
    while (ROOT / 'sources' / f'{prefix}_{k}.png').exists():
        with Image.open(ROOT / 'sources' / f'{prefix}_{k}.png') as im:
            rgb = unmatte(np.asarray(im.convert('RGB')))
            im = Image.fromarray(rgb)
            found.append(np.asarray(im.resize((W, round(im.height * W / im.width)), Image.Resampling.LANCZOS)).astype(np.float32))
        k += 1
    return found


def stack(parts, H):
    """Screen order, top to bottom: the last panel first. Seams cross-fade."""
    screen = parts[0]
    for upper in parts[1:]:
        fade = np.linspace(0, 1, SEAM)[:, None, None]      # 0 at the upper panel's last row
        blend = upper[-SEAM:] * (1 - fade) + screen[:SEAM] * fade
        screen = np.concatenate([upper[:-SEAM], blend, screen[SEAM:]], axis=0)
    if screen.shape[0] < H * 0.97:
        raise ValueError(f'panels cover {screen.shape[0]} rows; the route needs {H}')
    if screen.shape[0] < H:                                 # a few rows short: stretch, unseen
        screen = np.asarray(Image.fromarray(np.clip(screen, 0, 255).astype(np.uint8)).resize(
            (W, H), Image.Resampling.LANCZOS)).astype(np.float32)
    return screen[-H:]                                      # the sortie starts at the bottom


def arena(screen, name):
    """Lay the boss arena over the last page (the top PAGE rows, screen order),
    cross-fading it into the route over the SEAM rows below."""
    with Image.open(ROOT / 'sources' / f'{name}.png') as im:
        rgb = Image.fromarray(unmatte(np.asarray(im.convert('RGB'))))
        art = np.asarray(rgb.resize((W, round(rgb.height * W / rgb.width)), Image.Resampling.LANCZOS)).astype(np.float32)
    if art.shape[0] < PAGE + SEAM:
        raise ValueError(f'{name}: {art.shape[0]} rows; the arena needs {PAGE + SEAM}')
    out = screen.copy()
    out[:PAGE] = art[:PAGE]
    fade = np.linspace(0, 1, SEAM)[:, None, None]
    out[PAGE:PAGE + SEAM] = art[PAGE:PAGE + SEAM] * (1 - fade) + screen[PAGE:PAGE + SEAM] * fade
    return out


def road(screen, arena_rows=0):
    """The valley road: soil verge, asphalt with worn edges, a dashed centre line.
    With an arena it stops short of it, fading out over the arena's seam."""
    plain = screen
    out = screen.copy()
    H = out.shape[0]
    rng = np.random.default_rng(92)
    verge = np.array([138, 122, 92], np.float32)
    asphalt = np.array([70, 80, 88], np.float32)
    dark = np.array([46, 54, 64], np.float32)
    line = np.array([214, 206, 172], np.float32)
    for x in range(72, 113):
        edge = min(x - 72, 112 - x)
        if edge < 6:                                        # soft verge into the painting
            a = 0.35 + 0.1 * edge
            out[:, x] = out[:, x] * (1 - a) + verge * a
    noise = rng.normal(0, 4, (H, 25, 1)).astype(np.float32)
    out[:, 80:105] = asphalt + noise
    out[:, 80:82] = dark
    out[:, 103:105] = dark
    for y in range(8, H, 24):
        out[y:y + 11, 91:93] = line
    if arena_rows:
        keep = np.clip((np.arange(H) - arena_rows) / SEAM, 0, 1)[:, None, None]
        out = plain * (1 - keep) + out * keep
    return out


def pads(screen):
    """Concrete pads where the command targets stand (route pages 2, 4, 6) on land routes."""
    out = screen.copy()
    H = out.shape[0]
    for page in (2, 4, 6):
        y = H - (page + 1) * PAGE + 96                      # route row page*256+96, in screen order
        out[y - 34:y + 33, 118:171] = np.array([150, 138, 108], np.float32)
        out[y - 26:y + 25, 124:165] = np.array([96, 104, 108], np.float32)
        out[y - 25:y + 24, 125:164] = np.array([112, 120, 124], np.float32)
        for yy in (y - 19, y + 17):
            out[yy, 128:161] = np.array([214, 206, 172], np.float32)
        out[y + 9:y + 18, 105:124] = np.array([70, 80, 88], np.float32)
    return out


def refine(rgb, palettes, rounds=4):
    """Rounds of: every tile takes the palette that draws it best, then each palette is
    refit to its own tiles. The fitter places tiles as it adds palettes and never revisits
    them, which leaves tiles of dark water drawn as flat squares."""
    H, W = rgb.shape[:2]
    tiles = rgb.reshape(H // 16, 16, W // 16, 16, 3).transpose(0, 2, 1, 3, 4).reshape(-1, 256, 3).astype(np.float32)
    weight = np.array([0.30, 0.59, 0.11], np.float32)
    pals = np.asarray(palettes, np.float32)[:, 1:]              # slot 0 is transparency
    for _ in range(rounds):
        cost = np.zeros((len(tiles), len(pals)), np.float32)
        for k, pal in enumerate(pals):
            for start in range(0, len(tiles), 512):
                t = tiles[start:start + 512]
                d = (((t[:, :, None, :] - pal[None, None]) ** 2) * weight).sum(3)
                cost[start:start + 512, k] = d.min(2).sum(1)
        choice = cost.argmin(1)
        for k in range(len(pals)):
            mine = tiles[choice == k].reshape(-1, 3)
            if len(mine):
                pals[k] = fit_palette(mine.astype(np.uint8))
    indices = np.zeros((len(tiles), 256), np.uint8)
    for k, pal in enumerate(pals):
        sel = choice == k
        if sel.any():
            d = (((tiles[sel][:, :, None, :] - pal[None, None]) ** 2) * weight).sum(3)
            indices[sel] = d.argmin(2) + 1
    out_pals = np.zeros((len(pals), 16, 3), np.uint8)
    out_pals[:, 1:] = np.clip(np.round(pals), 0, 255)
    indices = indices.reshape(H // 16, W // 16, 16, 16).transpose(0, 2, 1, 3).reshape(H, W)
    return indices, out_pals, choice.reshape(H // 16, W // 16).astype(np.uint8)


def despeckle(indices, tiles):
    """A pixel unlike all four neighbours, three of which agree (in its own tile,
    so on its palette), takes their colour: single-pixel specks left by the
    reduction become clean clusters, as hand-placed pixels would be."""
    out = indices.copy()
    tile = np.repeat(np.repeat(tiles, 16, 0), 16, 1)
    p, t = np.pad(out, 1, mode='edge'), np.pad(tile, 1, mode='edge')
    nb = np.stack([p[:-2, 1:-1], p[2:, 1:-1], p[1:-1, :-2], p[1:-1, 2:]])
    same_tile = np.stack([t[:-2, 1:-1], t[2:, 1:-1], t[1:-1, :-2], t[1:-1, 2:]]) == tile[None]
    lone = (nb != out[None]).all(0) & same_tile.all(0)
    for y, x in zip(*np.nonzero(lone)):
        v, n = np.unique(nb[:, y, x], return_counts=True)
        if n.max() >= 3:
            out[y, x] = v[n.argmax()]
    return out


def route_order(screen):
    H = screen.shape[0]
    pages = [screen[H - (k + 1) * PAGE:H - k * PAGE] for k in range(H // PAGE)]
    return np.concatenate(pages, axis=0)


def sea_lanes(screen):
    """Boats and submarines sail at world X=80 and 192: both columns must be open water,
    allowing reef patches and the end of a pier (85%)."""
    for x in (80, 192):
        col = screen[:, x - 4:x + 5].reshape(-1, 3)
        blue = (col[:, 2] > col[:, 0] + 10).mean()
        if blue < 0.85:
            raise ValueError(f'sea lane x={x} is only {blue:.0%} water')
    return screen


# stage number: (panel prefix, route rows, road, target pads, sea lanes)
STAGES = {1: ('terrain_mountain', 4096, True, True, False), 2: ('coast', 2048, False, False, True),
          3: ('convoy', 2048, True, True, False), 4: ('open_sea', 2048, False, False, True),
          5: ('outpost', 2048, True, True, False), 6: ('armored', 2048, True, True, False),
          7: ('citadel', 2048, True, True, False)}
# each boss's arena; the open-sea route already ends in open ocean
ARENAS = {1: 'arena_mountain', 2: 'arena_coast', 3: 'arena_convoy', 5: 'arena_outpost',
          6: 'arena_armored', 7: 'arena_citadel'}


def build(stage=1):
    prefix, H, valley, targets, naval = STAGES[stage]
    parts = panels(prefix)
    if not parts:
        return None
    screen = stack(parts, H)
    open_ground = ARENAS.get(stage)
    if open_ground and (ROOT / 'sources' / f'{open_ground}.png').exists():
        screen = arena(screen, open_ground)
    else:
        open_ground = None
    if valley:
        screen = road(screen, PAGE if open_ground else 0)
    if targets:
        screen = pads(screen)
    if naval:
        screen = sea_lanes(screen)
    rgba = np.dstack([np.clip(route_order(screen), 0, 255), np.full((H, W), 255)]).astype(np.uint8)
    indices, palettes, tiles = quantize(rgba, 16)
    indices, palettes, tiles = refine(rgba[:, :, :3], palettes)
    indices = despeckle(indices, tiles)
    exact = reconstruct(indices, palettes, tiles).astype(np.uint8)
    out = ROOT / 'routes'
    out.mkdir(parents=True, exist_ok=True)
    Image.fromarray(np.dstack([exact, np.full((H, W), 255, np.uint8)]), 'RGBA').save(out / f'stage_{stage}.png')
    print(f'Painted route, stage {stage}: {len(parts)} panels, {len(palettes)} banks')
    return out / f'stage_{stage}.png'


if __name__ == '__main__':
    import sys
    sys.path.insert(0, str(Path(__file__).resolve().parents[3] / 'artbox'))
    for stage in (sys.argv[1:] or ['1']):
        build(int(stage))
