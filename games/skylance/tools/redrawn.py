"""Import the versioned art masters into stable, native sprite canvases."""
from pathlib import Path

import numpy as np
from PIL import Image

MASTER = Path(__file__).resolve().parents[1] / "artbox/redrawn"


def cells(filename, columns, rows):
    image = Image.open(MASTER / filename).convert("RGBA")
    alpha = np.asarray(image)[:, :, 3] >= 128
    bounds = []
    for axis, count in ((0, columns), (1, rows)):
        length = image.width if axis == 0 else image.height
        projection = alpha.sum(axis=0 if axis == 0 else 1)
        cuts = [0]
        for n in range(1, count):
            center = round(length * n / count)
            radius = max(1, round(length / count * .06))
            region = projection[center-radius:center+radius+1]
            minima = np.flatnonzero(region == region.min()) + center-radius
            cuts.append(int(minima[np.abs(minima-center).argmin()]))
        bounds.append(cuts + [length])
    result = []
    for row in range(rows):
        for col in range(columns):
            crop = image.crop((bounds[0][col], bounds[1][row],
                               bounds[0][col+1], bounds[1][row+1]))
            mask = np.asarray(crop)[:, :, 3] >= 128
            yy, xx = np.where(mask)
            result.append(crop.crop((xx.min(), yy.min(), xx.max()+1, yy.max()+1))
                          if len(xx) else None)
    return result


def native(source, box, ratio=None, centerline=False):
    if source is None:
        raise ValueError("An art cell is empty")
    if ratio is None:
        ratio = min((box[0]-4)/source.width, (box[1]-4)/source.height)
    rx, ry = ratio if isinstance(ratio, tuple) else (ratio, ratio)
    size = (max(1, round(source.width*rx)), max(1, round(source.height*ry)))
    resized = source.resize(size, Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", box)
    x = (box[0]-size[0])//2
    if centerline:
        mask = np.asarray(source)[:max(1, source.height//5), :, 3] >= 160
        _, nose = np.where(mask)
        if not len(nose):
            raise ValueError("Missing aircraft nose centerline")
        x = round(box[0]/2-float(np.median(nose))*rx)
        if x < 0 or x+size[0] > box[0]:
            raise ValueError("Roll frame exceeds its fixed fuselage canvas")
    canvas.paste(resized, (x, (box[1]-size[1])//2))
    pixels = np.asarray(canvas).copy()
    pixels[:, :, 3] = np.where(pixels[:, :, 3] >= 160, 255, 0)
    pixels[pixels[:, :, 3] == 0, :3] = 0
    return Image.fromarray(pixels)


def load():
    planes = cells("aircraft-roll-v2.png", 7, 3)
    poses = []
    for pilot in range(3):
        family = planes[pilot*7:(pilot+1)*7]
        # Miniature lengths and spans retain the real families' proportions.
        length = (38, 40, 44)[pilot]
        span = (25, 27, 29)[pilot]
        ratio = (span/family[3].width, length/max(p.height for p in family))
        # Coordinated lateral flight changes wing projection, not heading.
        # Keep every nose on the same vertical fuselage axis.
        poses.append([native(p, (48, 48), ratio, centerline=True) for p in family])
    portraits = [native(p, (80, 96)) for p in cells("pilots.png", 3, 1)]
    combat = cells("combat-v2.png", 4, 5)
    names = ("drone", "fighter", "interceptor", "heli", "missile_boat", "tank", "bomber")
    artwork = {name: native(p, (48, 32) if name == "bomber" else (32, 32))
               for name, p in zip(names, combat)}
    names = ("sam", "radar_truck", "submarine", "aa_bunker", "radar_site")
    artwork.update({name: native(p, (32, 32)) for name, p in zip(names, combat[7:12])})
    artwork.update({f"hostile_{i}": native(p, (16, 16))
                    for i, p in enumerate(combat[12:20])})
    artwork.update({f"cockpit_{i}": native(p, (64, 112))
                    for i, p in enumerate(cells("cockpits-v2.png", 3, 1))})
    maneuvers = cells("aircraft-maneuvers-v2.png", 4, 3)
    for pilot in range(3):
        family = maneuvers[pilot*4:pilot*4+4]
        length = (38, 40, 44)[pilot]
        span = (25, 27, 29)[pilot]
        ratio = (span/family[1].width, length/max(p.height for p in family))
        artwork.update({f"maneuver_{pilot}_{i}": native(p, (48, 48), ratio, centerline=True)
                        for i, p in enumerate(family)})
    names = ("castle", "gold_core", "heli_carrier", "battleship",
             "red_fortress", "stealth", "tank_fort")
    artwork.update({name: native(p, (112, 112))
                    for name, p in zip(names, cells("bosses.png", 4, 2))})
    for pilot in range(3):
        artwork[f"p{pilot+1}_pilot"] = portraits[pilot]
        artwork[f"p{pilot+1}_plane"] = poses[pilot][3]
    artwork["title"] = Image.open(MASTER / "title-v2.png").convert("RGBA").resize(
        (320, 224), Image.Resampling.LANCZOS)
    return artwork, poses
