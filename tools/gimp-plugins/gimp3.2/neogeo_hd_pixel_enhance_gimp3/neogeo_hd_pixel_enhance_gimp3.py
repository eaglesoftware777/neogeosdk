#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# GIMP 3.0+ Python-Fu plug-in (GObject-Introspection API)
# NeoGeo HD Pixel Enhance — GIMP 3
#
# Menu:
#   Filters -> Enhance -> NeoGeo HD Pixel Enhance (GIMP 3)...
#
# Port of the GIMP 2.10 "NeoGeo HD Pixel Enhance DIRECT Rev2" plug-in to
# GIMP 3's Gimp.PlugIn / GEGL buffer API. Same algorithm, same Rev2 dither
# fix (one Bayer cell shared by R/G/B, offset capped to a real 5-bit
# step, CRT vs LCD target matrix).
#
# Pure Python, no third-party dependencies: GIMP 3.2's bundled Python
# interpreter (3.14 at the time this was written) has no numpy wheel
# available yet and no working C build toolchain for one, so this reads
# and writes the layer as a flat bytearray through the GEGL buffer
# instead of vectorising with numpy - same per-pixel approach as the
# GIMP 2.10 plug-in, just against GIMP 3's buffer API.
#
# Installation: this file must live at
#   <GIMP profile>/plug-ins/neogeo_hd_pixel_enhance_gimp3/neogeo_hd_pixel_enhance_gimp3.py
# GIMP 3's plug-in browser only finds an executable that shares its
# parent directory's name - a bare .py file directly in plug-ins/ (the
# old 2.10 layout) will not be picked up.

import sys
import gi

gi.require_version('Gimp', '3.0')
gi.require_version('GimpUi', '3.0')
gi.require_version('Gegl', '0.4')
from gi.repository import Gimp, GimpUi, Gegl, GObject, GLib, Gio  # noqa: E402

PROC_NAME = "neogeo-hd-pixel-enhance-gimp3"

TARGET_CRT = 0
TARGET_LCD = 1

NEO5_STEP = 255.0 / 31.0

BAYER4 = (
    (0,  8,  2, 10),
    (12, 4, 14,  6),
    (3, 11,  1,  9),
    (15, 7, 13,  5),
)

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


def _u8(v):
    if v < 0:
        return 0
    if v > 255:
        return 255
    return int(v)


def _rgb_to_luma(r, g, b):
    return (30 * r + 59 * g + 11 * b) // 100


def _apply_gamma(c, gamma_x100):
    gamma = max(0.01, gamma_x100 / 100.0)
    return _u8((float(c) / 255.0) ** gamma * 255.0 + 0.5)


def _apply_contrast(c, contrast_percent):
    factor = (259.0 * (contrast_percent + 255.0)) / (255.0 * (259.0 - contrast_percent))
    return _u8(factor * (float(c) - 128.0) + 128.0)


def _apply_saturation(r, g, b, saturation_percent):
    sat = 1.0 + (saturation_percent / 100.0)
    y = float(_rgb_to_luma(r, g, b))
    return (_u8(y + (r - y) * sat), _u8(y + (g - y) * sat), _u8(y + (b - y) * sat))


def _neo5(v):
    q = int((v * 31 + 127) // 255)
    return int((q * 255 + 15) // 31)


def _dither_offset(matrix, size, x, y, strength_pct):
    if strength_pct <= 0:
        return 0.0
    cell = matrix[y % size][x % size]
    n = size * size
    frac = (cell - (n - 1) / 2.0) / (n - 1)
    return frac * NEO5_STEP * (strength_pct / 100.0)


def _is_near_white(r, g, b, white_threshold, chroma_tolerance):
    mx = max(r, g, b)
    mn = min(r, g, b)
    y = _rgb_to_luma(r, g, b)
    if r >= white_threshold and g >= white_threshold and b >= white_threshold:
        return True
    if y >= white_threshold and (mx - mn) <= chroma_tolerance:
        return True
    return False


def _nearest_opaque_rgb(buf, x, y, w, h, transparent_alpha):
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
                    p = (yy * w + xx) * 4
                    if buf[p + 3] > transparent_alpha:
                        return buf[p], buf[p + 1], buf[p + 2]
                xx += 1
            yy += 1
        radius += 1
    return 0, 0, 0


def _process_buffer(buf, w, h,
                    contrast_percent, saturation_percent, gamma_x100,
                    sharpen_percent, snap_to_neo_5bit, ordered_dither,
                    dither_strength, target_display, clean_transparent_rgb,
                    transparent_alpha, alpha_min):
    matrix, msize = (BAYER8, 8) if target_display == TARGET_LCD else (BAYER4, 4)
    changed = 0
    hidden_cleaned = 0

    y = 0
    while y < h:
        row = y * w * 4
        x = 0
        while x < w:
            i = row + x * 4
            a = buf[i + 3]

            if a >= alpha_min:
                old_r, old_g, old_b = buf[i], buf[i + 1], buf[i + 2]

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
                    buf[i], buf[i + 1], buf[i + 2] = r, g, b
                    changed += 1
            x += 1
        y += 1

    if sharpen_percent > 0:
        amount = sharpen_percent / 100.0
        src = bytes(buf)
        y = 0
        while y < h:
            row = y * w * 4
            x = 0
            while x < w:
                i = row + x * 4
                a = src[i + 3]
                if a >= alpha_min:
                    acc_r = acc_g = acc_b = 0
                    count = 0
                    if x > 0:
                        p = i - 4
                        acc_r += src[p]; acc_g += src[p + 1]; acc_b += src[p + 2]; count += 1
                    if x < w - 1:
                        p = i + 4
                        acc_r += src[p]; acc_g += src[p + 1]; acc_b += src[p + 2]; count += 1
                    if y > 0:
                        p = i - w * 4
                        acc_r += src[p]; acc_g += src[p + 1]; acc_b += src[p + 2]; count += 1
                    if y < h - 1:
                        p = i + w * 4
                        acc_r += src[p]; acc_g += src[p + 1]; acc_b += src[p + 2]; count += 1
                    if count > 0:
                        r0, g0, b0 = src[i], src[i + 1], src[i + 2]
                        br = acc_r / count
                        bg = acc_g / count
                        bb = acc_b / count
                        r1 = _u8(r0 + (r0 - br) * amount)
                        g1 = _u8(g0 + (g0 - bg) * amount)
                        b1 = _u8(b0 + (b0 - bb) * amount)
                        if snap_to_neo_5bit:
                            r1 = _neo5(r1); g1 = _neo5(g1); b1 = _neo5(b1)
                        buf[i], buf[i + 1], buf[i + 2] = r1, g1, b1
                x += 1
            y += 1

    if clean_transparent_rgb:
        y = 0
        while y < h:
            row = y * w * 4
            x = 0
            while x < w:
                i = row + x * 4
                a = buf[i + 3]
                if a <= transparent_alpha:
                    r, g, b = buf[i], buf[i + 1], buf[i + 2]
                    if _is_near_white(r, g, b, 210, 80):
                        rr, gg, bb = _nearest_opaque_rgb(buf, x, y, w, h, transparent_alpha)
                        buf[i], buf[i + 1], buf[i + 2] = rr, gg, bb
                        hidden_cleaned += 1
                x += 1
            y += 1

    return changed, hidden_cleaned


def _get_layer_rgba(drawable):
    buf = drawable.get_buffer()
    rect = buf.get_extent()
    has_alpha = drawable.has_alpha()
    fmt = "R'G'B'A u8" if has_alpha else "R'G'B' u8"
    raw = buf.get(rect, 1.0, fmt, Gegl.AbyssPolicy.CLAMP)
    raw = bytearray(raw)
    if not has_alpha:
        w, h = rect.width, rect.height
        rgba = bytearray(w * h * 4)
        n = w * h
        i3 = 0
        i4 = 0
        for _ in range(n):
            rgba[i4] = raw[i3]
            rgba[i4 + 1] = raw[i3 + 1]
            rgba[i4 + 2] = raw[i3 + 2]
            rgba[i4 + 3] = 255
            i3 += 3
            i4 += 4
        raw = rgba
    return raw, rect, has_alpha


def _set_layer_rgba(drawable, rgba, rect, has_alpha):
    buf = drawable.get_buffer()
    if has_alpha:
        fmt = "R'G'B'A u8"
        data = bytes(rgba)
    else:
        fmt = "R'G'B' u8"
        w, h = rect.width, rect.height
        rgb = bytearray(w * h * 3)
        n = w * h
        i3 = 0
        i4 = 0
        for _ in range(n):
            rgb[i3] = rgba[i4]
            rgb[i3 + 1] = rgba[i4 + 1]
            rgb[i3 + 2] = rgba[i4 + 2]
            i3 += 3
            i4 += 4
        data = bytes(rgb)
    buf.set(rect, fmt, data)
    buf.flush()
    drawable.update(rect.x, rect.y, rect.width, rect.height)


class NeogeoHdPixelEnhanceGimp3(Gimp.PlugIn):
    def do_query_procedures(self):
        return [PROC_NAME]

    def do_create_procedure(self, name):
        procedure = Gimp.ImageProcedure.new(
            self, name, Gimp.PDBProcType.PLUGIN, self.run, None)
        procedure.set_image_types("RGB*")
        procedure.set_menu_label("NeoGeo HD Pixel Enhance (GIMP 3)...")
        procedure.add_menu_path("<Image>/Filters/Enhance")
        procedure.set_documentation(
            "Enhance the active layer for Neo Geo / Artbox pixel quality",
            "Arcade contrast, saturation, gamma, conservative sharpening, "
            "optional Neo Geo 5-bit colour snap with hue-preserving ordered "
            "dither tuned for CRT or LCD, and transparent RGB cleanup. "
            "GIMP 3 port of the 2.10 DIRECT Rev2 plug-in.",
            name)
        procedure.set_attribution("Eagle Software 777", "Eagle Software 777", "2026")

        procedure.add_int_argument("contrast-percent", "Contrast %", "Contrast percent (-80..80)", -80, 80, 12, GObject.ParamFlags.READWRITE)
        procedure.add_int_argument("saturation-percent", "Saturation %", "Saturation percent (-80..80)", -80, 80, 8, GObject.ParamFlags.READWRITE)
        procedure.add_int_argument("gamma-x100", "Gamma x100", "95 brighter, 105 darker", 50, 180, 95, GObject.ParamFlags.READWRITE)
        procedure.add_int_argument("sharpen-percent", "Sharpen %", "Pixel sharpen percent (0..160)", 0, 160, 45, GObject.ParamFlags.READWRITE)
        procedure.add_boolean_argument("snap-to-neo-5bit", "Snap to Neo 5-bit", "Snap colours to Neo Geo 5-bit/channel preview", True, GObject.ParamFlags.READWRITE)
        procedure.add_boolean_argument("ordered-dither", "Ordered dither", "Ordered dither before 5-bit snap", True, GObject.ParamFlags.READWRITE)
        procedure.add_int_argument("dither-strength", "Dither strength", "Dither strength (0..100)", 0, 100, 20, GObject.ParamFlags.READWRITE)

        target_choice = Gimp.Choice.new()
        target_choice.add("crt", TARGET_CRT, "CRT (arcade monitor)", "")
        target_choice.add("lcd", TARGET_LCD, "LCD / emulator (modern flat panel)", "")
        procedure.add_choice_argument("target-display", "Target display", "Target display", target_choice, "crt", GObject.ParamFlags.READWRITE)

        procedure.add_boolean_argument("clean-transparent-rgb", "Clean hidden RGB", "Clean hidden white RGB in transparent pixels", True, GObject.ParamFlags.READWRITE)
        procedure.add_int_argument("transparent-alpha", "Transparent alpha", "Alpha treated as transparent", 0, 255, 24, GObject.ParamFlags.READWRITE)
        procedure.add_int_argument("alpha-min", "Min alpha", "Minimum alpha to enhance", 1, 255, 1, GObject.ParamFlags.READWRITE)

        return procedure

    def run(self, procedure, run_mode, image, drawables, config, run_data):
        if not drawables:
            return procedure.new_return_values(
                Gimp.PDBStatusType.CALLING_ERROR,
                GLib.Error("No active drawable/layer."))

        drawable = drawables[0]

        contrast_percent = config.get_property("contrast-percent")
        saturation_percent = config.get_property("saturation-percent")
        gamma_x100 = config.get_property("gamma-x100")
        sharpen_percent = config.get_property("sharpen-percent")
        snap_to_neo_5bit = config.get_property("snap-to-neo-5bit")
        ordered_dither = config.get_property("ordered-dither")
        dither_strength = config.get_property("dither-strength")
        target_display = config.get_property("target-display")
        clean_transparent_rgb = config.get_property("clean-transparent-rgb")
        transparent_alpha = config.get_property("transparent-alpha")
        alpha_min = config.get_property("alpha-min")

        try:
            target_display = int(target_display)
        except (TypeError, ValueError):
            target_display = TARGET_CRT

        Gimp.context_push()
        image.undo_group_start()

        try:
            Gimp.progress_init("NeoGeo HD pixel enhancement on active layer...")

            rgba, rect, has_alpha = _get_layer_rgba(drawable)
            changed, hidden_cleaned = _process_buffer(
                rgba, rect.width, rect.height,
                contrast_percent, saturation_percent, gamma_x100,
                sharpen_percent, snap_to_neo_5bit, ordered_dither,
                dither_strength, target_display, clean_transparent_rgb,
                transparent_alpha, alpha_min)
            _set_layer_rgba(drawable, rgba, rect, has_alpha)

            Gimp.displays_flush()
            Gimp.message(
                "NeoGeo HD enhance done. Pixels changed: %d. Hidden RGB cleaned: %d. No layer created." %
                (changed, hidden_cleaned))

        except Exception as e:
            Gimp.message("NeoGeo HD Pixel Enhance (GIMP 3) failed: %s" % str(e))
        finally:
            image.undo_group_end()
            Gimp.context_pop()

        return procedure.new_return_values(Gimp.PDBStatusType.SUCCESS, GLib.Error())


Gimp.main(NeogeoHdPixelEnhanceGimp3.__gtype__, sys.argv)
