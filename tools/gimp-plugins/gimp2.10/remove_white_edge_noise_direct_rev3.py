#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# GIMP 2.10.x Python-Fu plug-in
# Remove White Edge Noise — Direct Rev3
#
# Menu:
#   Filters -> Enhance -> Remove White Edge Noise DIRECT Rev3...
#
# This version edits the ACTIVE LAYER directly. It creates NO duplicate layer.
# It is designed for RGBA character sprites on transparent backgrounds.
#
# Rev3 changes from Rev2:
#   - Rev2's "hidden RGB decontamination" pass only fixed pixels that were
#     ALREADY fully transparent (alpha <= transparent_alpha), and the
#     separate "visible erase" pass only reached near-white pixels within
#     edge_radius (2px default) of one of those fully transparent pixels.
#     A soft anti-aliasing ramp wider than that - which is common after
#     the art pipeline resizes or re-exports a sprite - left a ring of
#     semi-transparent, near-white pixels untouched by either pass.  Those
#     pixels keep their alpha, so nothing LOOKS wrong in GIMP, but once
#     the Neo Geo pipeline thresholds alpha down to the hardware's binary
#     transparent/opaque bit, any pixel above that cutoff becomes a fully
#     opaque near-white dot on the sprite's edge - which is what actually
#     shows up in-game.
#   - Added semi_transparent_ceiling: any near-white pixel with alpha
#     below this ceiling (not just fully transparent ones) now gets its
#     hidden RGB replaced by a nearby non-white opaque colour, regardless
#     of edge_radius reachability. This closes that gap directly instead
#     of relying on a wider search radius to (maybe) reach it.

from gimpfu import *

PLUGIN_PROC = "python_fu_remove_white_edge_noise_direct_rev3"


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


def _is_near_white(r, g, b, white_threshold, chroma_tolerance):
    mx = max(r, g, b)
    mn = min(r, g, b)
    luma = (30 * r + 59 * g + 11 * b) // 100

    if r >= white_threshold and g >= white_threshold and b >= white_threshold:
        return True

    if luma >= white_threshold and (mx - mn) <= chroma_tolerance:
        return True

    return False


def _alpha(rows, x, y, bpp):
    return rows[y][x * bpp + 3]


def _touches_transparent(rows, x, y, w, h, bpp, radius, transparent_alpha):
    y0 = max(0, y - radius)
    y1 = min(h - 1, y + radius)
    x0 = max(0, x - radius)
    x1 = min(w - 1, x + radius)

    yy = y0
    while yy <= y1:
        xx = x0
        while xx <= x1:
            if xx != x or yy != y:
                if _alpha(rows, xx, yy, bpp) <= transparent_alpha:
                    return True
            xx += 1
        yy += 1
    return False


def _nearest_good_rgb(rows, x, y, w, h, bpp,
                      white_threshold, chroma_tolerance,
                      opaque_alpha_floor):
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
                    a = rows[yy][p + 3]
                    if a >= opaque_alpha_floor:
                        r = rows[yy][p + 0]
                        g = rows[yy][p + 1]
                        b = rows[yy][p + 2]
                        if not _is_near_white(r, g, b, white_threshold, chroma_tolerance):
                            return r, g, b
                xx += 1
            yy += 1
        radius += 1

    return 0, 0, 0


def _parse_args(args):
    # For <Image>/ menu entries, GIMP automatically passes image and drawable.
    # Some cached old registrations can pass image/drawable twice, so always
    # take the last 8 values as user options.
    if len(args) < 10:
        raise RuntimeError("Internal argument error: expected at least 10 args, got %d" % len(args))

    image = args[0]
    drawable = args[1]
    opts = args[-8:]
    return (image, drawable,
            opts[0], opts[1], opts[2], opts[3], opts[4], opts[5], opts[6], opts[7])


def _process_layer_direct(layer,
                          white_threshold,
                          chroma_tolerance,
                          edge_radius,
                          transparent_alpha,
                          min_alpha_to_erase,
                          passes,
                          decontaminate_transparent_rgb,
                          semi_transparent_ceiling):
    w = layer.width
    h = layer.height
    bpp = layer.bpp

    if bpp != 4:
        raise RuntimeError("The active layer must be RGBA. Use Image > Mode > RGB and Layer > Transparency > Add Alpha Channel.")

    src = layer.get_pixel_rgn(0, 0, w, h, False, False)
    dst = layer.get_pixel_rgn(0, 0, w, h, True, True)

    rows = []
    y = 0
    while y < h:
        rows.append(bytearray(src[0:w, y]))
        y += 1

    removed_visible = 0
    cleaned_hidden = 0
    current_pass = 0

    while current_pass < passes:
        mask = []
        y = 0
        while y < h:
            mask.append(bytearray(w))
            y += 1

        pass_removed = 0
        gimp.progress_update(0.6 * float(current_pass) / float(max(1, passes)))

        y = 0
        while y < h:
            row = rows[y]
            x = 0
            while x < w:
                i = x * bpp
                r = row[i + 0]
                g = row[i + 1]
                b = row[i + 2]
                a = row[i + 3]

                # Visible or semi-visible white/gray edge pixels:
                # erase them only if they are near transparent background.
                if a >= min_alpha_to_erase:
                    if _is_near_white(r, g, b, white_threshold, chroma_tolerance):
                        if _touches_transparent(rows, x, y, w, h, bpp, edge_radius, transparent_alpha):
                            mask[y][x] = 1
                            pass_removed += 1
                x += 1
            y += 1

        if pass_removed == 0:
            # Still do hidden RGB decontamination below once; then stop.
            if current_pass > 0:
                break

        new_rows = []
        y = 0
        while y < h:
            new_rows.append(bytearray(rows[y]))
            y += 1

        y = 0
        while y < h:
            x = 0
            while x < w:
                if mask[y][x]:
                    i = x * bpp
                    rr, gg, bb = _nearest_good_rgb(rows, x, y, w, h, bpp,
                                                   white_threshold,
                                                   chroma_tolerance,
                                                   transparent_alpha)
                    new_rows[y][i + 0] = rr
                    new_rows[y][i + 1] = gg
                    new_rows[y][i + 2] = bb
                    new_rows[y][i + 3] = 0
                x += 1
            y += 1

        rows = new_rows
        removed_visible += pass_removed
        current_pass += 1

        if pass_removed == 0:
            break

    # Hidden RGB decontamination, fully-transparent pixels:
    # If fully/mostly transparent pixels still contain white RGB, replace their
    # RGB with a nearby real sprite colour. This may not look different now,
    # but prevents white bleed after scaling/exporting/conversion.
    if decontaminate_transparent_rgb:
        gimp.progress_update(0.65)
        y = 0
        while y < h:
            x = 0
            while x < w:
                i = x * bpp
                r = rows[y][i + 0]
                g = rows[y][i + 1]
                b = rows[y][i + 2]
                a = rows[y][i + 3]
                if a <= transparent_alpha and _is_near_white(r, g, b, white_threshold, chroma_tolerance):
                    rr, gg, bb = _nearest_good_rgb(rows, x, y, w, h, bpp,
                                                   white_threshold,
                                                   chroma_tolerance,
                                                   transparent_alpha)
                    rows[y][i + 0] = rr
                    rows[y][i + 1] = gg
                    rows[y][i + 2] = bb
                    cleaned_hidden += 1
                x += 1
            y += 1

    # Hidden RGB decontamination, semi-transparent anti-aliasing ramp:
    # Pixels here still have their real alpha (nothing is erased or made
    # transparent) - only the RGB that would otherwise read as a white
    # dot once the sprite's alpha gets thresholded down the pipeline is
    # replaced.  Independent of edge_radius, and independent of the
    # erase passes above.
    if decontaminate_transparent_rgb and semi_transparent_ceiling > transparent_alpha:
        gimp.progress_update(0.8)
        y = 0
        while y < h:
            x = 0
            while x < w:
                i = x * bpp
                a = rows[y][i + 3]
                if transparent_alpha < a < semi_transparent_ceiling:
                    r = rows[y][i + 0]
                    g = rows[y][i + 1]
                    b = rows[y][i + 2]
                    if _is_near_white(r, g, b, white_threshold, chroma_tolerance):
                        rr, gg, bb = _nearest_good_rgb(rows, x, y, w, h, bpp,
                                                       white_threshold,
                                                       chroma_tolerance,
                                                       semi_transparent_ceiling)
                        rows[y][i + 0] = rr
                        rows[y][i + 1] = gg
                        rows[y][i + 2] = bb
                        cleaned_hidden += 1
                x += 1
            y += 1

    y = 0
    while y < h:
        dst[0:w, y] = str(rows[y])
        y += 1

    layer.flush()
    layer.merge_shadow(True)
    layer.update(0, 0, w, h)

    return removed_visible, cleaned_hidden


def remove_white_edge_noise_direct_rev3(*args):
    (image, drawable, white_threshold, chroma_tolerance, edge_radius,
     transparent_alpha, min_alpha_to_erase, passes,
     decontaminate_transparent_rgb, semi_transparent_ceiling) = _parse_args(args)

    white_threshold = _clamp_int(white_threshold, 0, 255)
    chroma_tolerance = _clamp_int(chroma_tolerance, 0, 255)
    edge_radius = _clamp_int(edge_radius, 1, 8)
    transparent_alpha = _clamp_int(transparent_alpha, 0, 255)
    min_alpha_to_erase = _clamp_int(min_alpha_to_erase, 1, 255)
    passes = _clamp_int(passes, 1, 8)
    semi_transparent_ceiling = _clamp_int(semi_transparent_ceiling, 0, 255)

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

        gimp.progress_init("Removing white edge noise directly on active layer...")

        removed_visible, cleaned_hidden = _process_layer_direct(
            drawable,
            white_threshold,
            chroma_tolerance,
            edge_radius,
            transparent_alpha,
            min_alpha_to_erase,
            passes,
            decontaminate_transparent_rgb,
            semi_transparent_ceiling
        )

        pdb.gimp_displays_flush()

        # Message is intentionally short. The important part: no new layer is made.
        pdb.gimp_message("Direct clean done. Visible removed: %d. Hidden RGB cleaned: %d. No layer created." %
                         (removed_visible, cleaned_hidden))

    except Exception as e:
        pdb.gimp_message("Remove White Edge Noise DIRECT Rev3 failed: %s" % str(e))

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
    "Remove white / near-white edge noise directly on the active layer",
    "Directly edits the active RGBA layer. Removes visible semi-transparent white halo pixels near transparency and cleans hidden white RGB in both fully- and semi-transparent pixels.",
    "Eagle Software 777",
    "Eagle Software 777",
    "2026",
    "<Image>/Filters/Enhance/Remove White Edge Noise DIRECT Rev3...",
    "RGB*",
    [
        (PF_INT,  "white_threshold",              "White threshold, lower = stronger", 200),
        (PF_INT,  "chroma_tolerance",             "Near-white tolerance, higher = stronger", 80),
        (PF_INT,  "edge_radius",                  "Transparent edge search radius", 2),
        (PF_INT,  "transparent_alpha",            "Alpha treated as fully transparent", 32),
        (PF_INT,  "min_alpha_to_erase",           "Minimum visible alpha to erase", 1),
        (PF_INT,  "passes",                       "Passes", 3),
        (PF_BOOL, "decontaminate_transparent_rgb", "Also clean hidden RGB in transparent pixels", True),
        (PF_INT,  "semi_transparent_ceiling",      "Also decontaminate RGB below this alpha (anti-alias ramp)", 180),
    ],
    [],
    remove_white_edge_noise_direct_rev3
)

main()
