#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# GIMP 2.10.x Python-Fu plug-in
# Smart Background Remove — Direct Rev2
#
# Menu:
#   Filters -> Enhance -> Smart Background Remove DIRECT Rev2...
#
# Removes the connected background from the active layer by sampling the image
# borders/corners, flood-filling only background connected to the image edge,
# and doing conservative edge cleanup so no extra pixels remain.
#
# Direct edit: creates NO duplicate layer.
#
# Rev2 changes from Rev1:
#   - Feathering (feather_radius > 0) only reduced the alpha of foreground
#     pixels next to the removed background; it never touched their RGB.
#     If defringe_edges was turned off, or a pixel sat just past the 1px
#     ring the defringe pass covers, its original RGB (often blended
#     toward the background colour by anti-aliasing, i.e. light/white)
#     stayed in place under the now-partially-transparent alpha - a
#     light halo/dot right on the silhouette edge once composited or
#     alpha-thresholded. Rev2 always decontaminates the RGB of any pixel
#     whose alpha it is about to soften, independent of defringe_edges.

from gimpfu import *

PLUGIN_PROC = "python_fu_smart_background_remove_direct_rev2"


def _clamp_int(v, lo, hi):
    try:
        v = int(v)
    except Exception:
        v = lo
    if v < lo:
        return lo
    if v > hi:
        return hi
    return v


def _rgb_dist2(a, b):
    dr = int(a[0]) - int(b[0])
    dg = int(a[1]) - int(b[1])
    db = int(a[2]) - int(b[2])
    return (3 * dr * dr + 4 * dg * dg + 2 * db * db) // 9


def _is_bg_like_rgb(rgb, samples, tol2):
    i = 0
    n = len(samples)
    while i < n:
        if _rgb_dist2(rgb, samples[i]) <= tol2:
            return True
        i += 1
    return False


def _pixel_rgb(rows, x, y, bpp):
    p = x * bpp
    return rows[y][p + 0], rows[y][p + 1], rows[y][p + 2]


def _pixel_alpha(rows, x, y, bpp):
    if bpp == 4:
        return rows[y][x * bpp + 3]
    return 255


def _touches_mask(mask, x, y, w, h, radius):
    y0 = max(0, y - radius)
    y1 = min(h - 1, y + radius)
    x0 = max(0, x - radius)
    x1 = min(w - 1, x + radius)

    yy = y0
    while yy <= y1:
        xx = x0
        while xx <= x1:
            if xx != x or yy != y:
                if mask[yy][xx]:
                    return True
            xx += 1
        yy += 1
    return False


def _is_safe_foreground_pixel(mask, rows, x, y, w, h, bpp, transparent_alpha):
    if mask[y][x]:
        return False
    if _pixel_alpha(rows, x, y, bpp) <= transparent_alpha:
        return False
    if _touches_mask(mask, x, y, w, h, 1):
        return False
    return True


def _nearest_safe_foreground_rgb(mask, rows, x, y, w, h, bpp, transparent_alpha):
    radius = 1
    while radius <= 8:
        y0 = max(0, y - radius)
        y1 = min(h - 1, y + radius)
        x0 = max(0, x - radius)
        x1 = min(w - 1, x + radius)

        yy = y0
        while yy <= y1:
            xx = x0
            while xx <= x1:
                if xx != x or yy != y:
                    if _is_safe_foreground_pixel(mask, rows, xx, yy, w, h, bpp, transparent_alpha):
                        return _pixel_rgb(rows, xx, yy, bpp)
                xx += 1
            yy += 1

        radius += 1

    return _pixel_rgb(rows, x, y, bpp)


def _collect_background_samples(rows, w, h, bpp, corner_size, border_step, transparent_alpha, max_samples):
    raw = []

    def add_sample(x, y):
        if x < 0 or x >= w or y < 0 or y >= h:
            return
        if _pixel_alpha(rows, x, y, bpp) <= transparent_alpha:
            return
        raw.append(_pixel_rgb(rows, x, y, bpp))

    cs = corner_size
    if cs < 1:
        cs = 1
    if cs > w:
        cs = w
    if cs > h:
        cs = h

    y = 0
    while y < cs:
        x = 0
        while x < cs:
            add_sample(x, y)
            add_sample(w - 1 - x, y)
            add_sample(x, h - 1 - y)
            add_sample(w - 1 - x, h - 1 - y)
            x += 1
        y += 1

    step = border_step
    if step < 1:
        step = 1

    x = 0
    while x < w:
        add_sample(x, 0)
        add_sample(x, h - 1)
        x += step

    y = 0
    while y < h:
        add_sample(0, y)
        add_sample(w - 1, y)
        y += step

    if not raw:
        raw.append((255, 255, 255))

    if len(raw) <= max_samples:
        return raw

    out = []
    stride = max(1, len(raw) // max_samples)
    i = 0
    while i < len(raw) and len(out) < max_samples:
        out.append(raw[i])
        i += stride

    return out


def _flood_background(rows, w, h, bpp, samples, tolerance, transparent_alpha):
    tol2 = tolerance * tolerance
    mask = []
    y = 0
    while y < h:
        mask.append(bytearray(w))
        y += 1

    q = []
    head = 0

    def try_add(x, y):
        if x < 0 or x >= w or y < 0 or y >= h:
            return
        if mask[y][x]:
            return

        if _pixel_alpha(rows, x, y, bpp) <= transparent_alpha:
            mask[y][x] = 1
            q.append((x, y))
            return

        rgb = _pixel_rgb(rows, x, y, bpp)
        if _is_bg_like_rgb(rgb, samples, tol2):
            mask[y][x] = 1
            q.append((x, y))

    x = 0
    while x < w:
        try_add(x, 0)
        try_add(x, h - 1)
        x += 1

    y = 1
    while y < h - 1:
        try_add(0, y)
        try_add(w - 1, y)
        y += 1

    while head < len(q):
        x, y = q[head]
        head += 1
        try_add(x + 1, y)
        try_add(x - 1, y)
        try_add(x, y + 1)
        try_add(x, y - 1)

        if (head & 4095) == 0:
            gimp.progress_update(0.2 + 0.5 * (float(head) / float(max(1, len(q)))))

    return mask


def _expand_edge_cleanup(rows, mask, w, h, bpp, samples, tolerance, edge_extra_tolerance, passes, transparent_alpha):
    removed = 0
    tol = tolerance + edge_extra_tolerance
    if tol < 0:
        tol = 0
    tol2 = tol * tol

    p = 0
    while p < passes:
        add = []
        y = 0
        while y < h:
            add.append(bytearray(w))
            y += 1

        pass_removed = 0
        y = 0
        while y < h:
            x = 0
            while x < w:
                if not mask[y][x]:
                    if _touches_mask(mask, x, y, w, h, 1):
                        if _pixel_alpha(rows, x, y, bpp) <= transparent_alpha:
                            add[y][x] = 1
                            pass_removed += 1
                        else:
                            rgb = _pixel_rgb(rows, x, y, bpp)
                            if _is_bg_like_rgb(rgb, samples, tol2):
                                add[y][x] = 1
                                pass_removed += 1
                x += 1
            y += 1

        if pass_removed == 0:
            break

        y = 0
        while y < h:
            x = 0
            while x < w:
                if add[y][x]:
                    mask[y][x] = 1
                x += 1
            y += 1

        removed += pass_removed
        p += 1

    return removed


def _apply_mask(rows, mask, w, h, bpp, transparent_alpha, feather_radius, defringe_edges):
    removed = 0
    softened = 0
    defringed = 0

    y = 0
    while y < h:
        x = 0
        while x < w:
            if mask[y][x]:
                p = x * bpp
                if bpp == 4:
                    if rows[y][p + 3] != 0:
                        removed += 1
                    rows[y][p + 3] = 0
            x += 1
        y += 1

    # Pixels that will have their alpha softened by feathering need their
    # RGB decontaminated too, or the leftover edge blend colour (usually
    # bled toward the background, i.e. light/white) shows through the
    # new partial transparency. Build that pixel set up front so both
    # the defringe pass and the feather pass agree on it, instead of
    # defringe silently being the only thing standing between a feathered
    # pixel and a visible fringe.
    needs_rgb_fix = None
    if bpp == 4 and (defringe_edges or feather_radius > 0):
        needs_rgb_fix = []
        y = 0
        while y < h:
            row = []
            x = 0
            while x < w:
                touches = (not mask[y][x] and _pixel_alpha(rows, x, y, bpp) > transparent_alpha
                           and _touches_mask(mask, x, y, w, h, max(1, feather_radius)))
                row.append(touches)
                x += 1
            needs_rgb_fix.append(row)
            y += 1

        y = 0
        while y < h:
            x = 0
            while x < w:
                if needs_rgb_fix[y][x]:
                    rr, gg, bb = _nearest_safe_foreground_rgb(mask, rows, x, y, w, h, bpp, transparent_alpha)
                    p = x * bpp
                    rows[y][p + 0] = rr
                    rows[y][p + 1] = gg
                    rows[y][p + 2] = bb
                    defringed += 1
                x += 1
            y += 1

    if bpp == 4 and feather_radius > 0:
        y = 0
        while y < h:
            x = 0
            while x < w:
                if not mask[y][x] and _pixel_alpha(rows, x, y, bpp) > transparent_alpha:
                    if _touches_mask(mask, x, y, w, h, feather_radius):
                        p = x * bpp
                        a = rows[y][p + 3]
                        na = int(float(a) * 0.85)
                        if na < a:
                            rows[y][p + 3] = na
                            softened += 1
                x += 1
            y += 1

    return removed, softened, defringed


def _parse_args(args):
    if len(args) < 11:
        raise RuntimeError("Internal argument error: expected at least 11 args, got %d." % len(args))

    image = args[0]
    drawable = args[1]
    opts = args[-9:]

    return (image, drawable,
            opts[0], opts[1], opts[2], opts[3], opts[4],
            opts[5], opts[6], opts[7], opts[8])


def smart_background_remove_direct_rev2(*args):
    image, drawable, tolerance, corner_sample_size, border_sample_step, edge_extra_tolerance, edge_cleanup_passes, feather_radius, transparent_alpha, defringe_edges, max_samples = _parse_args(args)

    tolerance = _clamp_int(tolerance, 0, 255)
    corner_sample_size = _clamp_int(corner_sample_size, 1, 64)
    border_sample_step = _clamp_int(border_sample_step, 1, 64)
    edge_extra_tolerance = _clamp_int(edge_extra_tolerance, 0, 120)
    edge_cleanup_passes = _clamp_int(edge_cleanup_passes, 0, 8)
    feather_radius = _clamp_int(feather_radius, 0, 4)
    transparent_alpha = _clamp_int(transparent_alpha, 0, 255)
    max_samples = _clamp_int(max_samples, 8, 256)

    undo_started = False
    gimp.context_push()

    try:
        if image is None:
            raise RuntimeError("No active image.")
        if drawable is None:
            raise RuntimeError("No active drawable/layer.")

        pdb.gimp_image_undo_group_start(image)
        undo_started = True

        if not pdb.gimp_drawable_is_rgb(drawable):
            raise RuntimeError("Use Image > Mode > RGB first.")

        if not pdb.gimp_drawable_has_alpha(drawable):
            pdb.gimp_layer_add_alpha(drawable)

        w = drawable.width
        h = drawable.height
        bpp = drawable.bpp

        if bpp != 4:
            raise RuntimeError("The active layer must be RGBA after adding alpha.")

        src = drawable.get_pixel_rgn(0, 0, w, h, False, False)
        dst = drawable.get_pixel_rgn(0, 0, w, h, True, True)

        rows = []
        y = 0
        while y < h:
            rows.append(bytearray(src[0:w, y]))
            y += 1

        gimp.progress_init("Smart background remove: sampling border...")
        samples = _collect_background_samples(rows, w, h, bpp,
                                              corner_sample_size,
                                              border_sample_step,
                                              transparent_alpha,
                                              max_samples)

        gimp.progress_update(0.10)
        mask = _flood_background(rows, w, h, bpp, samples, tolerance, transparent_alpha)

        gimp.progress_update(0.75)
        edge_removed = _expand_edge_cleanup(rows, mask, w, h, bpp, samples,
                                            tolerance,
                                            edge_extra_tolerance,
                                            edge_cleanup_passes,
                                            transparent_alpha)

        gimp.progress_update(0.90)
        removed, softened, defringed = _apply_mask(rows, mask, w, h, bpp,
                                                   transparent_alpha,
                                                   feather_radius,
                                                   defringe_edges)

        y = 0
        while y < h:
            dst[0:w, y] = str(rows[y])
            y += 1

        drawable.flush()
        drawable.merge_shadow(True)
        drawable.update(0, 0, w, h)

        pdb.gimp_displays_flush()
        pdb.gimp_message("Smart BG remove done. Removed: %d px. Edge cleanup: %d. Defringed: %d. No layer created." %
                         (removed, edge_removed, defringed))

    except Exception as e:
        pdb.gimp_message("Smart Background Remove failed: %s" % str(e))

    finally:
        if undo_started:
            try:
                pdb.gimp_image_undo_group_end(image)
            except Exception:
                pass
        try:
            gimp.context_pop()
        except Exception:
            pass


register(
    PLUGIN_PROC,
    "Smart connected background remover with conservative edge cleanup",
    "Samples the border/corners, flood-fills only connected background, removes edge leftovers, and defringes/decontaminates any foreground edge whose alpha is being softened. Direct active-layer edit; no new layer.",
    "Eagle Software 777",
    "Eagle Software 777",
    "2026",
    "<Image>/Filters/Enhance/Smart Background Remove DIRECT Rev2...",
    "RGB*",
    [
        (PF_INT,  "tolerance",             "Background colour tolerance, higher = removes more", 42),
        (PF_INT,  "corner_sample_size",    "Corner sample size", 10),
        (PF_INT,  "border_sample_step",    "Border sample step", 8),
        (PF_INT,  "edge_extra_tolerance",  "Extra edge cleanup tolerance", 18),
        (PF_INT,  "edge_cleanup_passes",   "Edge cleanup passes", 2),
        (PF_INT,  "feather_radius",        "Feather radius, 0 = hard clean edge", 0),
        (PF_INT,  "transparent_alpha",     "Alpha treated as transparent", 8),
        (PF_BOOL, "defringe_edges",        "Defringe remaining foreground edge RGB", True),
        (PF_INT,  "max_samples",           "Max background samples", 96),
    ],
    [],
    smart_background_remove_direct_rev2
)

main()
