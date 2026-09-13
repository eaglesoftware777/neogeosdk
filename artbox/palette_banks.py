"""Source-faithful, budgeted palettes for 4bpp Neo Geo artwork.

Bank zero is fitted to the whole image. Additional banks address the tiles
with the greatest remaining error, without replacing that global fallback.
Tile boundaries change only the bank selection, never the source colors.
"""

import numpy as np

from img2neo import lab_to_rgb, rgb_to_lab
from img2neo_tile import ng_snap


def nearest(samples, centers):
    samples = np.asarray(samples, dtype=np.float32).reshape(-1, 3)
    centers = np.asarray(centers, dtype=np.float32).reshape(-1, 3)
    labels = np.empty(len(samples), dtype=np.int32)
    errors = np.empty(len(samples), dtype=np.float32)
    for start in range(0, len(samples), 4096):
        block = samples[start:start + 4096]
        distances = np.sum((block[:, None] - centers[None]) ** 2, axis=2)
        chosen = distances.argmin(axis=1)
        labels[start:start + len(block)] = chosen
        errors[start:start + len(block)] = distances[np.arange(len(block)), chosen]
    return labels, errors


def fit_palette(pixels, colors=15):
    """Fit original pixel frequencies, then refine on the hardware lattice."""
    pixels = np.asarray(pixels, dtype=np.uint8).reshape(-1, 3)
    if not len(pixels):
        return np.zeros((colors, 3), dtype=np.uint8)
    unique, counts = np.unique(pixels, axis=0, return_counts=True)
    snapped = np.unique(ng_snap(unique), axis=0)
    if len(snapped) <= colors:
        return np.concatenate((snapped, np.repeat(snapped[-1:], colors - len(snapped), axis=0)))

    # Bound training cost without favoring one corner or one animation frame.
    if len(unique) > 8192:
        rng = np.random.default_rng(0)
        pixels = pixels[rng.choice(len(pixels), min(len(pixels), 32768), replace=False)]
        unique, counts = np.unique(pixels, axis=0, return_counts=True)
    lab = rgb_to_lab(unique).reshape(-1, 3).astype(np.float32)
    weights = counts.astype(np.float64)
    centers = [lab[weights.argmax()]]
    distances = np.full(len(lab), np.inf)
    for _ in range(1, colors):
        distances = np.minimum(distances, np.sum((lab - centers[-1]) ** 2, axis=1))
        centers.append(lab[np.argmax(distances * weights)])
    centers = np.asarray(centers)
    best_error = np.inf
    best = None
    for _ in range(18):
        palette = ng_snap(lab_to_rgb(centers).reshape(-1, 3))
        labels, errors = nearest(lab, rgb_to_lab(palette))
        total = float(np.dot(errors, weights))
        if total >= best_error - 1e-6:
            break
        best, best_error = palette, total
        for i in range(colors):
            selected = labels == i
            if selected.any():
                centers[i] = np.average(lab[selected], axis=0, weights=weights[selected])
            else:
                centers[i] = lab[np.argmax(errors * weights)]
    return best


def palette_words(palette):
    """Pack D/R0/G0/B0 and the split channel bits, with transparent slot 0."""
    rgb6 = ng_snap(np.asarray(palette, dtype=np.uint8)).astype(np.uint16) >> 2
    words = []
    for red, green, blue in rgb6:
        dark = 1 - (int(red) & 1)
        r, g, b = int(red) >> 1, int(green) >> 1, int(blue) >> 1
        words.append((dark << 15) | ((r & 1) << 14) | ((g & 1) << 13)
                     | ((b & 1) << 12) | ((r >> 1) << 8) | ((g >> 1) << 4) | (b >> 1))
    words[0] = 0
    return words


def training_mask(rgba):
    visible = rgba[:, :, 3] >= 128
    core = rgba[:, :, 3] >= 250
    # Tiny antialiased craft and glows may have only a few solid pixels.
    # Those highlights cannot stand in for the colors of the entire object.
    if int(core.sum()) < max(16, int(visible.sum()) // 8):
        return visible
    return core


def quantize(rgba, bank_limit=1, master=None, dither="none"):
    """Return indices, RGB banks (including index zero), tile-bank offsets.

    Semi-transparent borders do not train palettes when a solid core exists.
    Alpha below 128 stays transparent. More banks cannot increase the fitted
    image's nearest-color Lab error: the global fallback is never discarded.
    """
    rgba = np.asarray(rgba, dtype=np.uint8)
    height, width = rgba.shape[:2]
    if height % 16 or width % 16 or height == 0 or width == 0:
        raise ValueError("Palette canvases must be positive multiples of 16")
    if not 1 <= bank_limit <= 16:
        raise ValueError("palette_banks must be between 1 and 16")
    rgb = rgba[:, :, :3]
    visible = rgba[:, :, 3] >= 128
    core = training_mask(rgba)
    training = rgb[core]
    first = fit_palette(training) if master is None else np.asarray(master, dtype=np.uint8)
    palettes = [first]
    lab = rgb_to_lab(rgb).reshape(-1, 3)
    tile_count = (height // 16) * (width // 16)
    tile_ids = np.arange(tile_count).reshape(height // 16, width // 16).repeat(16, 0).repeat(16, 1)
    pixels_by_tile = tile_ids.reshape(-1)
    assignment = np.zeros(tile_count, dtype=np.uint8)

    def evaluate(palette):
        labels, errors = nearest(lab, rgb_to_lab(palette))
        errors[~visible.reshape(-1)] = 0
        totals = np.bincount(pixels_by_tile, weights=errors, minlength=tile_count)
        return labels, totals

    labels, best_errors = evaluate(first)
    index_planes = [labels]
    budget = bank_limit if master is None else 1
    # A seed is only tried once, which both bounds the loop and stops it
    # returning to a tile that has already failed to earn a bank.
    tried = np.zeros(tile_count, dtype=bool)

    while len(palettes) < budget:
        # Take the worst tile that can still seed a palette.  Skipping to
        # the next one matters: a tile whose pixels are all soft edges has
        # nothing to fit a palette from, and abandoning the whole
        # allocation there left large sprites using a fraction of the
        # banks they were given - the budget was spent by whichever tile
        # happened to be worst, not by the picture.
        seed = -1
        for candidate_tile in np.argsort(-best_errors):
            if tried[candidate_tile] or best_errors[candidate_tile] < 1e-4:
                continue
            if ((tile_ids == candidate_tile) & core).any():
                seed = int(candidate_tile)
                break
        if seed < 0:
            break
        tried[seed] = True

        candidate = fit_palette(rgb[(tile_ids == seed) & core])
        new_labels, new_errors = evaluate(candidate)
        selected = new_errors < best_errors
        # Fit the entire region that benefits, not isolated tile averages.
        region = selected[tile_ids] & core
        if region.any():
            refit = fit_palette(rgb[region])
            refit_labels, refit_errors = evaluate(refit)
            if np.minimum(refit_errors, best_errors).sum() < np.minimum(new_errors, best_errors).sum():
                candidate, new_labels, new_errors = refit, refit_labels, refit_errors
                selected = new_errors < best_errors
        if not selected.any():
            # This seed buys nothing; the next worst tile may still.
            continue
        assignment[selected] = len(palettes)
        best_errors = np.minimum(best_errors, new_errors)
        palettes.append(candidate)
        index_planes.append(new_labels)

    banks = np.zeros((len(palettes), 16, 3), dtype=np.uint8)
    banks[:, 1:] = palettes
    chosen = assignment[pixels_by_tile]
    indices = np.stack(index_planes)[chosen, np.arange(height * width)].reshape(height, width) + 1

    if dither == "ordered":
        # Low-amplitude, image-aligned dither: no error wave crosses a silhouette
        # or accumulates at a tile boundary. Flat pixel-art colors stay exact.
        matrix = (np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]) - 7.5) / 4
        perturbation = np.tile(matrix, (height // 4, width // 4))
        dither_rgb = np.clip(rgb.astype(np.float32) + perturbation[:, :, None], 0, 255).astype(np.uint8)
        dither_lab = rgb_to_lab(dither_rgb).reshape(-1, 3)
        for bank, palette in enumerate(palettes):
            mask = (chosen == bank) & visible.reshape(-1)
            ids, _ = nearest(dither_lab[mask], rgb_to_lab(palette))
            indices.reshape(-1)[mask] = ids + 1
    indices[~visible] = 0
    return indices.astype(np.uint8), banks, assignment.reshape(height // 16, width // 16)


def reconstruct(indices, banks, tile_banks):
    selected = tile_banks.repeat(16, 0).repeat(16, 1)
    return banks[selected, indices]
