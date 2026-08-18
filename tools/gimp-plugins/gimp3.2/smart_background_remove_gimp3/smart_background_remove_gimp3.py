#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# GIMP 3.0+ Python-Fu plug-in (GObject-Introspection API)
# Smart Background Remove — GIMP 3
#
# Menu:
#   Filters -> Enhance -> Smart Background Remove (GIMP 3)...
#
# Port of the GIMP 2.10 "Smart Background Remove DIRECT Rev2" plug-in.
# Samples the border/corners, flood-fills only background connected to
# the image edge, expands into near-background edge leftovers, then
# removes it - defringing/decontaminating the RGB of any foreground
# pixel whose alpha is being softened (feather or defringe), same as
# the 2.10 Rev2 fix, so a partly-transparent edge never keeps a hidden
# light/white RGB underneath it.
#
# Pure Python, no third-party dependencies - see neogeo_hd_pixel_enhance_gimp3
# for why. The flood fill uses an explicit pixel queue (same shape as the
# GIMP 2.10 version), not a numpy dilation.
#
# Installation: this file must live at
#   <GIMP profile>/plug-ins/smart_background_remove_gimp3/smart_background_remove_gimp3.py

import sys
import gi

gi.require_version('Gimp', '3.0')
gi.require_version('GimpUi', '3.0')
gi.require_version('Gegl', '0.4')
from gi.repository import Gimp, GimpUi, Gegl, GObject, GLib, Gio  # noqa: E402

PROC_NAME = "smart-background-remove-gimp3"


def _rgb_dist2(a, b):
    dr = a[0] - b[0]
    dg = a[1] - b[1]
    db = a[2] - b[2]
    return (3 * dr * dr + 4 * dg * dg + 2 * db * db) // 9


def _is_bg_like_rgb(rgb, samples, tol2):
    for s in samples:
        if _rgb_dist2(rgb, s) <= tol2:
            return True
    return False


def _pixel_rgb(buf, x, y, w):
    p = (y * w + x) * 4
    return buf[p], buf[p + 1], buf[p + 2]


def _pixel_alpha(buf, x, y, w):
    return buf[(y * w + x) * 4 + 3]


def _touches_mask(mask, x, y, w, h, radius):
    y0 = max(0, y - radius)
    y1 = min(h - 1, y + radius)
    x0 = max(0, x - radius)
    x1 = min(w - 1, x + radius)
    yy = y0
    while yy <= y1:
        xx = x0
        while xx <= x1:
            if (xx != x or yy != y) and mask[yy * w + xx]:
                return True
            xx += 1
        yy += 1
    return False


def _is_safe_foreground_pixel(mask, buf, x, y, w, h, transparent_alpha):
    if mask[y * w + x]:
        return False
    if _pixel_alpha(buf, x, y, w) <= transparent_alpha:
        return False
    if _touches_mask(mask, x, y, w, h, 1):
        return False
    return True


def _nearest_safe_foreground_rgb(mask, buf, x, y, w, h, transparent_alpha):
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
                    if _is_safe_foreground_pixel(mask, buf, xx, yy, w, h, transparent_alpha):
                        return _pixel_rgb(buf, xx, yy, w)
                xx += 1
            yy += 1
        radius += 1
    return _pixel_rgb(buf, x, y, w)


def _collect_background_samples(buf, w, h, corner_size, border_step, transparent_alpha, max_samples):
    raw = []

    def add_sample(x, y):
        if x < 0 or x >= w or y < 0 or y >= h:
            return
        if _pixel_alpha(buf, x, y, w) <= transparent_alpha:
            return
        raw.append(_pixel_rgb(buf, x, y, w))

    cs = max(1, min(corner_size, w, h))
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

    step = max(1, border_step)
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


def _flood_background(buf, w, h, samples, tolerance, transparent_alpha):
    tol2 = tolerance * tolerance
    mask = bytearray(w * h)
    q = []
    head = 0

    def try_add(x, y):
        if x < 0 or x >= w or y < 0 or y >= h:
            return
        idx = y * w + x
        if mask[idx]:
            return
        if _pixel_alpha(buf, x, y, w) <= transparent_alpha:
            mask[idx] = 1
            q.append((x, y))
            return
        rgb = _pixel_rgb(buf, x, y, w)
        if _is_bg_like_rgb(rgb, samples, tol2):
            mask[idx] = 1
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
            Gimp.progress_update(0.2 + 0.5 * (float(head) / float(max(1, len(q)))))

    return mask


def _expand_edge_cleanup(buf, mask, w, h, samples, tolerance, edge_extra_tolerance, passes, transparent_alpha):
    removed = 0
    tol = max(0, tolerance + edge_extra_tolerance)
    tol2 = tol * tol

    p = 0
    while p < passes:
        add = bytearray(w * h)
        pass_removed = 0
        y = 0
        while y < h:
            x = 0
            while x < w:
                idx = y * w + x
                if not mask[idx] and _touches_mask(mask, x, y, w, h, 1):
                    if _pixel_alpha(buf, x, y, w) <= transparent_alpha:
                        add[idx] = 1
                        pass_removed += 1
                    else:
                        rgb = _pixel_rgb(buf, x, y, w)
                        if _is_bg_like_rgb(rgb, samples, tol2):
                            add[idx] = 1
                            pass_removed += 1
                x += 1
            y += 1

        if pass_removed == 0:
            break

        y = 0
        while y < h:
            x = 0
            while x < w:
                idx = y * w + x
                if add[idx]:
                    mask[idx] = 1
                x += 1
            y += 1

        removed += pass_removed
        p += 1

    return removed


def _apply_mask(buf, mask, w, h, transparent_alpha, feather_radius, defringe_edges):
    removed = 0
    softened = 0
    defringed = 0

    y = 0
    while y < h:
        x = 0
        while x < w:
            idx = y * w + x
            if mask[idx]:
                p = idx * 4
                if buf[p + 3] != 0:
                    removed += 1
                buf[p + 3] = 0
            x += 1
        y += 1

    touch_radius = max(1, feather_radius)
    if defringe_edges or feather_radius > 0:
        targets = []
        y = 0
        while y < h:
            x = 0
            while x < w:
                idx = y * w + x
                if not mask[idx] and _pixel_alpha(buf, x, y, w) > transparent_alpha:
                    if _touches_mask(mask, x, y, w, h, touch_radius):
                        targets.append((x, y))
                x += 1
            y += 1

        for (x, y) in targets:
            rr, gg, bb = _nearest_safe_foreground_rgb(mask, buf, x, y, w, h, transparent_alpha)
            p = (y * w + x) * 4
            buf[p], buf[p + 1], buf[p + 2] = rr, gg, bb
            defringed += 1

    if feather_radius > 0:
        y = 0
        while y < h:
            x = 0
            while x < w:
                idx = y * w + x
                if not mask[idx] and _pixel_alpha(buf, x, y, w) > transparent_alpha:
                    if _touches_mask(mask, x, y, w, h, feather_radius):
                        p = idx * 4
                        a = buf[p + 3]
                        na = int(a * 0.85)
                        if na < a:
                            buf[p + 3] = na
                            softened += 1
                x += 1
            y += 1

    return removed, softened, defringed


def _get_layer_rgba(drawable):
    buf = drawable.get_buffer()
    rect = buf.get_extent()
    raw = buf.get(rect, 1.0, "R'G'B'A u8", Gegl.AbyssPolicy.CLAMP)
    return bytearray(raw), rect


def _set_layer_rgba(drawable, rgba, rect):
    buf = drawable.get_buffer()
    buf.set(rect, "R'G'B'A u8", bytes(rgba))
    buf.flush()
    drawable.update(rect.x, rect.y, rect.width, rect.height)


class SmartBackgroundRemoveGimp3(Gimp.PlugIn):
    def do_query_procedures(self):
        return [PROC_NAME]

    def do_create_procedure(self, name):
        procedure = Gimp.ImageProcedure.new(
            self, name, Gimp.PDBProcType.PLUGIN, self.run, None)
        procedure.set_image_types("RGB*")
        procedure.set_menu_label("Smart Background Remove (GIMP 3)...")
        procedure.add_menu_path("<Image>/Filters/Enhance")
        procedure.set_documentation(
            "Smart connected background remover with conservative edge cleanup",
            "Samples the border/corners, flood-fills only connected "
            "background, removes edge leftovers, and defringes/"
            "decontaminates any foreground edge whose alpha is being "
            "softened. GIMP 3 port of the 2.10 DIRECT Rev2 plug-in.",
            name)
        procedure.set_attribution("Eagle Software 777", "Eagle Software 777", "2026")

        procedure.add_int_argument("tolerance", "Tolerance", "Background colour tolerance, higher = removes more", 0, 255, 42, GObject.ParamFlags.READWRITE)
        procedure.add_int_argument("corner-sample-size", "Corner sample size", "Corner sample size", 1, 64, 10, GObject.ParamFlags.READWRITE)
        procedure.add_int_argument("border-sample-step", "Border sample step", "Border sample step", 1, 64, 8, GObject.ParamFlags.READWRITE)
        procedure.add_int_argument("edge-extra-tolerance", "Edge extra tolerance", "Extra edge cleanup tolerance", 0, 120, 18, GObject.ParamFlags.READWRITE)
        procedure.add_int_argument("edge-cleanup-passes", "Edge cleanup passes", "Edge cleanup passes", 0, 8, 2, GObject.ParamFlags.READWRITE)
        procedure.add_int_argument("feather-radius", "Feather radius", "0 = hard clean edge", 0, 4, 0, GObject.ParamFlags.READWRITE)
        procedure.add_int_argument("transparent-alpha", "Transparent alpha", "Alpha treated as transparent", 0, 255, 8, GObject.ParamFlags.READWRITE)
        procedure.add_boolean_argument("defringe-edges", "Defringe edges", "Defringe remaining foreground edge RGB", True, GObject.ParamFlags.READWRITE)
        procedure.add_int_argument("max-samples", "Max samples", "Max background samples", 8, 256, 96, GObject.ParamFlags.READWRITE)

        return procedure

    def run(self, procedure, run_mode, image, drawables, config, run_data):
        if not drawables:
            return procedure.new_return_values(
                Gimp.PDBStatusType.CALLING_ERROR,
                GLib.Error("No active drawable/layer."))

        drawable = drawables[0]

        tolerance = config.get_property("tolerance")
        corner_sample_size = config.get_property("corner-sample-size")
        border_sample_step = config.get_property("border-sample-step")
        edge_extra_tolerance = config.get_property("edge-extra-tolerance")
        edge_cleanup_passes = config.get_property("edge-cleanup-passes")
        feather_radius = config.get_property("feather-radius")
        transparent_alpha = config.get_property("transparent-alpha")
        defringe_edges = config.get_property("defringe-edges")
        max_samples = config.get_property("max-samples")

        Gimp.context_push()
        image.undo_group_start()

        try:
            if not drawable.has_alpha():
                drawable.add_alpha()

            Gimp.progress_init("Smart background remove: sampling border...")

            rgba, rect = _get_layer_rgba(drawable)
            w, h = rect.width, rect.height

            samples = _collect_background_samples(
                rgba, w, h, corner_sample_size, border_sample_step,
                transparent_alpha, max_samples)

            Gimp.progress_update(0.10)
            mask = _flood_background(rgba, w, h, samples, tolerance, transparent_alpha)

            Gimp.progress_update(0.75)
            edge_removed = _expand_edge_cleanup(
                rgba, mask, w, h, samples, tolerance, edge_extra_tolerance,
                edge_cleanup_passes, transparent_alpha)

            Gimp.progress_update(0.90)
            removed, softened, defringed = _apply_mask(
                rgba, mask, w, h, transparent_alpha, feather_radius, defringe_edges)

            _set_layer_rgba(drawable, rgba, rect)

            Gimp.displays_flush()
            Gimp.message(
                "Smart BG remove done. Removed: %d px. Edge cleanup: %d. Defringed: %d. No layer created." %
                (removed, edge_removed, defringed))

        except Exception as e:
            Gimp.message("Smart Background Remove (GIMP 3) failed: %s" % str(e))
        finally:
            image.undo_group_end()
            Gimp.context_pop()

        return procedure.new_return_values(Gimp.PDBStatusType.SUCCESS, GLib.Error())


Gimp.main(SmartBackgroundRemoveGimp3.__gtype__, sys.argv)
