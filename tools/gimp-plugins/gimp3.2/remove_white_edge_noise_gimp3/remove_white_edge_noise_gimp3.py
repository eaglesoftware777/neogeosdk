#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# GIMP 3.0+ Python-Fu plug-in (GObject-Introspection API)
# Remove White Edge Noise — GIMP 3
#
# Menu:
#   Filters -> Enhance -> Remove White Edge Noise (GIMP 3)...
#
# Port of the GIMP 2.10 "Remove White Edge Noise DIRECT Rev3" plug-in.
# Same algorithm and the same Rev3 fix: hidden white RGB is decontaminated
# both in fully transparent pixels AND in the semi-transparent anti-alias
# ramp (below semi_transparent_ceiling), not only within edge_radius of a
# fully transparent neighbour - the ramp case is what leaves a white dot
# once the Neo Geo pipeline thresholds alpha down to a binary bit.
#
# Pure Python, no third-party dependencies - see neogeo_hd_pixel_enhance_gimp3
# for why (no numpy wheel/build toolchain available for GIMP 3.2's bundled
# interpreter at the time this was written). Reads/writes the layer as a
# flat bytearray through the GEGL buffer.
#
# Installation: this file must live at
#   <GIMP profile>/plug-ins/remove_white_edge_noise_gimp3/remove_white_edge_noise_gimp3.py

import sys
import gi

gi.require_version('Gimp', '3.0')
gi.require_version('GimpUi', '3.0')
gi.require_version('Gegl', '0.4')
from gi.repository import Gimp, GimpUi, Gegl, GObject, GLib, Gio  # noqa: E402

PROC_NAME = "remove-white-edge-noise-gimp3"


def _is_near_white(r, g, b, white_threshold, chroma_tolerance):
    mx = max(r, g, b)
    mn = min(r, g, b)
    luma = (30 * r + 59 * g + 11 * b) // 100
    if r >= white_threshold and g >= white_threshold and b >= white_threshold:
        return True
    if luma >= white_threshold and (mx - mn) <= chroma_tolerance:
        return True
    return False


def _alpha(buf, x, y, w):
    return buf[(y * w + x) * 4 + 3]


def _touches_transparent(buf, x, y, w, h, radius, transparent_alpha):
    y0 = max(0, y - radius)
    y1 = min(h - 1, y + radius)
    x0 = max(0, x - radius)
    x1 = min(w - 1, x + radius)
    yy = y0
    while yy <= y1:
        xx = x0
        while xx <= x1:
            if xx != x or yy != y:
                if _alpha(buf, xx, yy, w) <= transparent_alpha:
                    return True
            xx += 1
        yy += 1
    return False


def _nearest_good_rgb(buf, x, y, w, h, white_threshold, chroma_tolerance, opaque_alpha_floor):
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
                    if buf[p + 3] >= opaque_alpha_floor:
                        r, g, b = buf[p], buf[p + 1], buf[p + 2]
                        if not _is_near_white(r, g, b, white_threshold, chroma_tolerance):
                            return r, g, b
                xx += 1
            yy += 1
        radius += 1
    return 0, 0, 0


def _process_buffer(buf, w, h, white_threshold, chroma_tolerance, edge_radius,
                    transparent_alpha, min_alpha_to_erase, passes,
                    decontaminate_transparent_rgb, semi_transparent_ceiling):
    removed_visible = 0
    cleaned_hidden = 0
    current_pass = 0

    while current_pass < passes:
        mask = bytearray(w * h)
        pass_removed = 0

        y = 0
        while y < h:
            row = y * w
            x = 0
            while x < w:
                i = (row + x) * 4
                r, g, b, a = buf[i], buf[i + 1], buf[i + 2], buf[i + 3]
                if a >= min_alpha_to_erase and _is_near_white(r, g, b, white_threshold, chroma_tolerance):
                    if _touches_transparent(buf, x, y, w, h, edge_radius, transparent_alpha):
                        mask[row + x] = 1
                        pass_removed += 1
                x += 1
            y += 1

        if pass_removed == 0:
            break

        y = 0
        while y < h:
            row = y * w
            x = 0
            while x < w:
                if mask[row + x]:
                    rr, gg, bb = _nearest_good_rgb(buf, x, y, w, h, white_threshold, chroma_tolerance, transparent_alpha + 1)
                    i = (row + x) * 4
                    buf[i], buf[i + 1], buf[i + 2], buf[i + 3] = rr, gg, bb, 0
                x += 1
            y += 1

        removed_visible += pass_removed
        current_pass += 1

    if decontaminate_transparent_rgb:
        y = 0
        while y < h:
            row = y * w
            x = 0
            while x < w:
                i = (row + x) * 4
                a = buf[i + 3]
                if a <= transparent_alpha:
                    r, g, b = buf[i], buf[i + 1], buf[i + 2]
                    if _is_near_white(r, g, b, white_threshold, chroma_tolerance):
                        rr, gg, bb = _nearest_good_rgb(buf, x, y, w, h, white_threshold, chroma_tolerance, transparent_alpha + 1)
                        buf[i], buf[i + 1], buf[i + 2] = rr, gg, bb
                        cleaned_hidden += 1
                x += 1
            y += 1

        if semi_transparent_ceiling > transparent_alpha:
            y = 0
            while y < h:
                row = y * w
                x = 0
                while x < w:
                    i = (row + x) * 4
                    a = buf[i + 3]
                    if transparent_alpha < a < semi_transparent_ceiling:
                        r, g, b = buf[i], buf[i + 1], buf[i + 2]
                        if _is_near_white(r, g, b, white_threshold, chroma_tolerance):
                            rr, gg, bb = _nearest_good_rgb(buf, x, y, w, h, white_threshold, chroma_tolerance, semi_transparent_ceiling)
                            buf[i], buf[i + 1], buf[i + 2] = rr, gg, bb
                            cleaned_hidden += 1
                    x += 1
                y += 1

    return removed_visible, cleaned_hidden


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


class RemoveWhiteEdgeNoiseGimp3(Gimp.PlugIn):
    def do_query_procedures(self):
        return [PROC_NAME]

    def do_create_procedure(self, name):
        procedure = Gimp.ImageProcedure.new(
            self, name, Gimp.PDBProcType.PLUGIN, self.run, None)
        procedure.set_image_types("RGBA")
        procedure.set_menu_label("Remove White Edge Noise (GIMP 3)...")
        procedure.add_menu_path("<Image>/Filters/Enhance")
        procedure.set_documentation(
            "Remove white / near-white edge noise on the active layer",
            "Removes visible semi-transparent white halo pixels near "
            "transparency and cleans hidden white RGB in both fully- and "
            "semi-transparent pixels. GIMP 3 port of the 2.10 DIRECT Rev3 "
            "plug-in.",
            name)
        procedure.set_attribution("Eagle Software 777", "Eagle Software 777", "2026")

        procedure.add_int_argument("white-threshold", "White threshold", "Lower = stronger", 0, 255, 200, GObject.ParamFlags.READWRITE)
        procedure.add_int_argument("chroma-tolerance", "Chroma tolerance", "Higher = stronger", 0, 255, 80, GObject.ParamFlags.READWRITE)
        procedure.add_int_argument("edge-radius", "Edge radius", "Transparent edge search radius", 1, 8, 2, GObject.ParamFlags.READWRITE)
        procedure.add_int_argument("transparent-alpha", "Transparent alpha", "Alpha treated as fully transparent", 0, 255, 32, GObject.ParamFlags.READWRITE)
        procedure.add_int_argument("min-alpha-to-erase", "Min alpha to erase", "Minimum visible alpha to erase", 1, 255, 1, GObject.ParamFlags.READWRITE)
        procedure.add_int_argument("passes", "Passes", "Passes", 1, 8, 3, GObject.ParamFlags.READWRITE)
        procedure.add_boolean_argument("decontaminate-transparent-rgb", "Decontaminate hidden RGB", "Also clean hidden RGB in transparent pixels", True, GObject.ParamFlags.READWRITE)
        procedure.add_int_argument("semi-transparent-ceiling", "Semi-transparent ceiling", "Also decontaminate RGB below this alpha (anti-alias ramp)", 0, 255, 180, GObject.ParamFlags.READWRITE)

        return procedure

    def run(self, procedure, run_mode, image, drawables, config, run_data):
        if not drawables:
            return procedure.new_return_values(
                Gimp.PDBStatusType.CALLING_ERROR,
                GLib.Error("No active drawable/layer."))

        drawable = drawables[0]

        white_threshold = config.get_property("white-threshold")
        chroma_tolerance = config.get_property("chroma-tolerance")
        edge_radius = config.get_property("edge-radius")
        transparent_alpha = config.get_property("transparent-alpha")
        min_alpha_to_erase = config.get_property("min-alpha-to-erase")
        passes = config.get_property("passes")
        decontaminate_transparent_rgb = config.get_property("decontaminate-transparent-rgb")
        semi_transparent_ceiling = config.get_property("semi-transparent-ceiling")

        Gimp.context_push()
        image.undo_group_start()

        try:
            if not drawable.has_alpha():
                drawable.add_alpha()

            Gimp.progress_init("Removing white edge noise directly on active layer...")

            rgba, rect = _get_layer_rgba(drawable)
            removed_visible, cleaned_hidden = _process_buffer(
                rgba, rect.width, rect.height,
                white_threshold, chroma_tolerance, edge_radius,
                transparent_alpha, min_alpha_to_erase, passes,
                decontaminate_transparent_rgb, semi_transparent_ceiling)
            _set_layer_rgba(drawable, rgba, rect)

            Gimp.displays_flush()
            Gimp.message(
                "Direct clean done. Visible removed: %d. Hidden RGB cleaned: %d. No layer created." %
                (removed_visible, cleaned_hidden))

        except Exception as e:
            Gimp.message("Remove White Edge Noise (GIMP 3) failed: %s" % str(e))
        finally:
            image.undo_group_end()
            Gimp.context_pop()

        return procedure.new_return_values(Gimp.PDBStatusType.SUCCESS, GLib.Error())


Gimp.main(RemoveWhiteEdgeNoiseGimp3.__gtype__, sys.argv)
