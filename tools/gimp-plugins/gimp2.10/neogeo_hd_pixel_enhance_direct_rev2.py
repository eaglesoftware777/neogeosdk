#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# GIMP 2.10.x Python-Fu plug-in
# NeoGeo HD Pixel Enhance — Direct Rev2
#
# Menu:
#   Filters -> Enhance -> NeoGeo HD Pixel Enhance DIRECT Rev2...
#
# Purpose:
#   Directly enhance the ACTIVE LAYER for Neo Geo / Artbox style assets:
#   - stronger arcade contrast
#   - controlled saturation
#   - optional gamma correction
#   - optional small-radius sharpening
#   - optional Neo Geo 5-bit/channel colour snap
#   - optional ordered dithering before 5-bit snap, tuned for a chosen
#     target display (CRT arcade monitor vs a modern LCD/emulator)
#   - optional transparent RGB decontamination
#
# It creates NO duplicate layer. It edits the selected layer directly.
#
# Rev2 changes from Rev1:
#   - The ordered dither used a DIFFERENT Bayer cell per channel (R at
#     (x,y), G at (x+1,y), B at (x,y+1)).  That decorrelates the three
#     channels, so at some pixels R, G and B independently round toward
#     their bright extreme at once even over a flat mid-tone area -
#     a visible bright/white speck that has nothing to do with the
#     source art.  Rev2 samples ONE cell per pixel and applies the same
#     offset to all three channels, which dithers luminance while
#     preserving hue - the standard approach.
#   - The offset formula could swing a channel by multiple full 5-bit
#     steps (up to ~60 of 255 at strength=100, vs. the ~4 that a single
#     half-step of dither should ever move a channel).  That alone was
#     enough to blow a mid-tone pixel out to full white after rounding.
#     Rev2 scales the offset to a fraction of one real 5-bit step
#     (255/31 ~= 8.23), so strength=100 tops out at a proper half-step.
#   - Added target_display: CRT keeps the classic 4x4 Bayer pattern
#     (a CRT's own blur/bloom smears the dither into a fine texture);
#     LCD/emulator switches to an 8x8 Bayer at a lower default strength,
#     since every dithered pixel stays a hard, fully visible dot with
#     no blur to hide the 4x4 pattern's repeat.
#
# Target:
#   GIMP 2.10.38 Python-Fu / Python 2.7 style API.

from gimpfu import *

PLUGIN_PROC = "python_fu_neogeo_hd_pixel_enhance_direct_rev2"

TARGET_CRT = 0
TARGET_LCD = 1

# One Neo Geo 5-bit/channel quantisation step, in 8-bit units.
NEO5_STEP = 255.0 / 31.0

BAYER4 = (
    (0,  8,  2, 10),
    (12, 4, 14,  6),
    (3, 11,  1,  9),
    (15, 7, 13,  5),
)

# Standard 8x8 ordered-dither matrix. Larger period than the 4x4 table,
# so its repeat is far less visible as a regular checker on a crisp,
# blur-free LCD panel.
BAYER8 = (
    (0,  48, 12, 60,  3, 51, 15, 63),
    (32, 16, 44, 28, 35, 19, 47, 31),
    (8,  56,  4, 52, 11, 59,  7, 55),
    (40, 24, 36, 20, 43, 27, 39, 23),
    (2,  50, 14, 62,  1, 49, 13, 61),
    (34, 18, 46, 30, 33, 17, 45, 29),
    (10, 58,  6, 54,  9, 57,  5, 53),
    (42, 26, 38, 22, 41, 25, 37, 21),
)


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


def _u8(v):
    if v < 0:
        return 0
    if v > 255:
        return 255
    return int(v)


def _rgb_to_luma(r, g, b):
    return (30 * r + 59 * g + 11 * b) // 100


def _apply_gamma(c, gamma_x100):
    gamma = float(gamma_x100) / 100.0
    if gamma <= 0.01:
        gamma = 1.0
    return _u8((pow(float(c) / 255.0, gamma) * 255.0) + 0.5)


def _apply_contrast(c, contrast_percent):
    # contrast_percent: -100..100, 0 unchanged.
    factor = (259.0 * (contrast_percent + 255.0)) / (255.0 * (259.0 - contrast_percent))
    return _u8((factor * (float(c) - 128.0)) + 128.0)


def _apply_saturation(r, g, b, saturation_percent):
    # saturation_percent: -100..100, 0 unchanged.
    sat = 1.0 + (float(saturation_percent) / 100.0)
    y = float(_rgb_to_luma(r, g, b))
    rr = y + (float(r) - y) * sat
    gg = y + (float(g) - y) * sat
    bb = y + (float(b) - y) * sat
    return _u8(rr), _u8(gg), _u8(bb)


def _neo5(v):
    # Snap 8-bit component to Neo Geo 5-bit/channel grid, returned as 8-bit preview.
    q = int((int(v) * 31 + 127) // 255)
    return int((q * 255 + 15) // 31)


def _dither_offset(matrix, size, x, y, strength_pct):
    # One cell, shared by all three channels of this pixel, so the
    # dither perturbs luminance without shifting hue.  frac spans
    # about -0.5..+0.5 across the matrix; strength=100 caps the swing
    # at one half of a real 5-bit step.
    if strength_pct <= 0:
        return 0.0
    cell = matrix[y % size][x % size]
    n = size * size
    frac = (cell - (n - 1) / 2.0) / (n - 1)
    return frac * NEO5_STEP * (float(strength_pct) / 100.0)


def _is_near_white(r, g, b, white_threshold, chroma_tolerance):
    mx = max(r, g, b)
    mn = min(r, g, b)
    y = _rgb_to_luma(r, g, b)
    if r >= white_threshold and g >= white_threshold and b >= white_threshold:
        return True
    if y >= white_threshold and (mx - mn) <= chroma_tolerance:
        return True
    return False


def _nearest_opaque_rgb(rows, x, y, w, h, bpp, transparent_alpha):
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
                    p = xx * bpp
                    if rows[yy][p + 3] > transparent_alpha:
                        return rows[yy][p + 0], rows[yy][p + 1], rows[yy][p + 2]
                xx += 1
            yy += 1
        radius += 1
    return 0, 0, 0


def _parse_args(args):
    # Correct <Image>/ registration passes:
    # image, drawable, 11 user parameters = 13 args.
    # If GIMP has stale cached calls, keep using the final 11 parameters.
    if len(args) < 13:
        raise RuntimeError("Internal argument error: expected at least 13 args, got %d." % len(args))
    image = args[0]
    drawable = args[1]
    opts = args[-11:]
    return (image, drawable,
            opts[0], opts[1], opts[2], opts[3], opts[4], opts[5],
            opts[6], opts[7], opts[8], opts[9], opts[10])


def _sharpen_rows(rows, w, h, bpp, amount_percent, alpha_min):
    amount = float(amount_percent) / 100.0
    if amount <= 0.0:
        return rows

    out = []
    y = 0
    while y < h:
        out.append(bytearray(rows[y]))
        y += 1

    y = 0
    while y < h:
        x = 0
        while x < w:
            i = x * bpp
            a = rows[y][i + 3] if bpp == 4 else 255

            if a >= alpha_min:
                # 4-neighbour unsharp, conservative for sprite/pixel art.
                acc = [0, 0, 0]
                count = 0
                if x > 0:
                    p = (x - 1) * bpp
                    acc[0] += rows[y][p + 0]; acc[1] += rows[y][p + 1]; acc[2] += rows[y][p + 2]; count += 1
                if x < w - 1:
                    p = (x + 1) * bpp
                    acc[0] += rows[y][p + 0]; acc[1] += rows[y][p + 1]; acc[2] += rows[y][p + 2]; count += 1
                if y > 0:
                    p = x * bpp
                    acc[0] += rows[y - 1][p + 0]; acc[1] += rows[y - 1][p + 1]; acc[2] += rows[y - 1][p + 2]; count += 1
                if y < h - 1:
                    p = x * bpp
                    acc[0] += rows[y + 1][p + 0]; acc[1] += rows[y + 1][p + 1]; acc[2] += rows[y + 1][p + 2]; count += 1

                if count > 0:
                    r = rows[y][i + 0]
                    g = rows[y][i + 1]
                    b = rows[y][i + 2]
                    br = acc[0] / count
                    bg = acc[1] / count
                    bb = acc[2] / count
                    out[y][i + 0] = _u8(float(r) + (float(r) - float(br)) * amount)
                    out[y][i + 1] = _u8(float(g) + (float(g) - float(bg)) * amount)
                    out[y][i + 2] = _u8(float(b) + (float(b) - float(bb)) * amount)
            x += 1
        y += 1

    return out


def _process_direct(layer,
                    contrast_percent,
                    saturation_percent,
                    gamma_x100,
                    sharpen_percent,
                    snap_to_neo_5bit,
                    ordered_dither,
                    dither_strength,
                    target_display,
                    clean_transparent_rgb,
                    transparent_alpha,
                    alpha_min):
    w = layer.width
    h = layer.height
    bpp = layer.bpp

    if bpp not in (3, 4):
        raise RuntimeError("The active layer must be RGB or RGBA.")

    matrix, msize = (BAYER8, 8) if target_display == TARGET_LCD else (BAYER4, 4)

    src = layer.get_pixel_rgn(0, 0, w, h, False, False)
    dst = layer.get_pixel_rgn(0, 0, w, h, True, True)

    rows = []
    y = 0
    while y < h:
        rows.append(bytearray(src[0:w, y]))
        y += 1

    changed = 0
    hidden_cleaned = 0

    # Main colour enhancement pass.
    y = 0
    while y < h:
        row = rows[y]
        x = 0
        while x < w:
            i = x * bpp

            if bpp == 4:
                a = row[i + 3]
            else:
                a = 255

            if a >= alpha_min:
                old_r = row[i + 0]
                old_g = row[i + 1]
                old_b = row[i + 2]

                r = _apply_gamma(old_r, gamma_x100)
                g = _apply_gamma(old_g, gamma_x100)
                b = _apply_gamma(old_b, gamma_x100)

                r = _apply_contrast(r, contrast_percent)
                g = _apply_contrast(g, contrast_percent)
                b = _apply_contrast(b, contrast_percent)

                r, g, b = _apply_saturation(r, g, b, saturation_percent)

                if ordered_dither:
                    off = _dither_offset(matrix, msize, x, y, dither_strength)
                    if off != 0.0:
                        r = _u8(r + off)
                        g = _u8(g + off)
                        b = _u8(b + off)

                if snap_to_neo_5bit:
                    r = _neo5(r)
                    g = _neo5(g)
                    b = _neo5(b)

                if r != old_r or g != old_g or b != old_b:
                    row[i + 0] = r
                    row[i + 1] = g
                    row[i + 2] = b
                    changed += 1

            x += 1
        y += 1

    # Conservative sharpen after colour shaping.
    if sharpen_percent > 0:
        rows = _sharpen_rows(rows, w, h, bpp, sharpen_percent, alpha_min)

        # Re-snap after sharpening if target is Neo Geo preview.
        if snap_to_neo_5bit:
            y = 0
            while y < h:
                x = 0
                while x < w:
                    i = x * bpp
                    a = rows[y][i + 3] if bpp == 4 else 255
                    if a >= alpha_min:
                        rows[y][i + 0] = _neo5(rows[y][i + 0])
                        rows[y][i + 1] = _neo5(rows[y][i + 1])
                        rows[y][i + 2] = _neo5(rows[y][i + 2])
                    x += 1
                y += 1

    # Transparent RGB cleanup helps resize/export/conversion avoid halos.
    if bpp == 4 and clean_transparent_rgb:
        y = 0
        while y < h:
            x = 0
            while x < w:
                i = x * bpp
                a = rows[y][i + 3]
                if a <= transparent_alpha:
                    r = rows[y][i + 0]
                    g = rows[y][i + 1]
                    b = rows[y][i + 2]
                    if _is_near_white(r, g, b, 210, 80):
                        rr, gg, bb = _nearest_opaque_rgb(rows, x, y, w, h, bpp, transparent_alpha)
                        rows[y][i + 0] = rr
                        rows[y][i + 1] = gg
                        rows[y][i + 2] = bb
                        hidden_cleaned += 1
                x += 1
            y += 1

    y = 0
    while y < h:
        dst[0:w, y] = str(rows[y])
        y += 1

    layer.flush()
    layer.merge_shadow(True)
    layer.update(0, 0, w, h)

    return changed, hidden_cleaned


def neogeo_hd_pixel_enhance_direct_rev2(*args):
    (image, drawable, contrast_percent, saturation_percent, gamma_x100,
     sharpen_percent, snap_to_neo_5bit, ordered_dither, dither_strength,
     target_display, clean_transparent_rgb, transparent_alpha, alpha_min) = _parse_args(args)

    contrast_percent = _clamp_int(contrast_percent, -80, 80)
    saturation_percent = _clamp_int(saturation_percent, -80, 80)
    gamma_x100 = _clamp_int(gamma_x100, 50, 180)
    sharpen_percent = _clamp_int(sharpen_percent, 0, 160)
    dither_strength = _clamp_int(dither_strength, 0, 100)
    target_display = _clamp_int(target_display, 0, 1)
    transparent_alpha = _clamp_int(transparent_alpha, 0, 255)
    alpha_min = _clamp_int(alpha_min, 1, 255)

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

        gimp.progress_init("NeoGeo HD pixel enhancement on active layer...")

        changed, hidden_cleaned = _process_direct(
            drawable,
            contrast_percent,
            saturation_percent,
            gamma_x100,
            sharpen_percent,
            snap_to_neo_5bit,
            ordered_dither,
            dither_strength,
            target_display,
            clean_transparent_rgb,
            transparent_alpha,
            alpha_min
        )

        pdb.gimp_displays_flush()
        pdb.gimp_message("NeoGeo HD enhance done. Pixels changed: %d. Hidden RGB cleaned: %d. No layer created." %
                         (changed, hidden_cleaned))

    except Exception as e:
        pdb.gimp_message("NeoGeo HD Pixel Enhance failed: %s" % str(e))

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
    "Enhance image for Neo Geo / Artbox pixel quality directly on active layer",
    "Arcade contrast, saturation, gamma, conservative sharpening, optional Neo Geo 5-bit colour snap with hue-preserving ordered dither tuned for CRT or LCD, and transparent RGB cleanup.",
    "Eagle Software 777",
    "Eagle Software 777",
    "2026",
    "<Image>/Filters/Enhance/NeoGeo HD Pixel Enhance DIRECT Rev2...",
    "RGB*",
    [
        (PF_INT,    "contrast_percent",     "Contrast percent (-80..80)", 12),
        (PF_INT,    "saturation_percent",   "Saturation percent (-80..80)", 8),
        (PF_INT,    "gamma_x100",           "Gamma x100: 95 brighter, 105 darker", 95),
        (PF_INT,    "sharpen_percent",      "Pixel sharpen percent (0..160)", 45),
        (PF_BOOL,   "snap_to_neo_5bit",     "Snap colours to Neo Geo 5-bit/channel preview", True),
        (PF_BOOL,   "ordered_dither",       "Ordered dither before 5-bit snap", True),
        (PF_INT,    "dither_strength",      "Dither strength (0..100)", 20),
        (PF_OPTION, "target_display",       "Target display", 0, ["CRT (arcade monitor)", "LCD / emulator (modern flat panel)"]),
        (PF_BOOL,   "clean_transparent_rgb", "Clean hidden white RGB in transparent pixels", True),
        (PF_INT,    "transparent_alpha",    "Alpha treated as transparent", 24),
        (PF_INT,    "alpha_min",            "Minimum alpha to enhance", 1),
    ],
    [],
    neogeo_hd_pixel_enhance_direct_rev2
)

main()
