#!/usr/bin/env python3
"""
Artbox Studio — NeoGeoSDK visual tool
Tabs: Tile Grid Viewer | Sprite Designer | Hitbox Editor | Pixel Paint
Run from the artbox/ directory.
"""

import json
import math
import os
import struct
import sys

import numpy as np
from PyQt6.QtCore import (QPoint, QRect, QSize, Qt, QTimer, pyqtSignal)
from PyQt6.QtGui import (QColor, QCursor, QFont, QIcon, QImage, QPainter,
                          QPen, QPixmap)
from PyQt6.QtWidgets import (QApplication, QComboBox, QDockWidget,
                              QFileDialog, QFrame, QGridLayout, QGroupBox,
                              QHBoxLayout, QLabel, QLineEdit, QListWidget,
                              QListWidgetItem, QMainWindow, QMessageBox,
                              QPushButton, QScrollArea, QSizePolicy,
                              QSlider, QSpinBox, QSplitter, QStatusBar,
                              QTabWidget, QToolBar, QTreeWidget,
                              QTreeWidgetItem, QVBoxLayout, QWidget,
                              QDialog, QDialogButtonBox, QPlainTextEdit)

# ---------------------------------------------------------------------------
# NeoGeo C-ROM tile codec
# ---------------------------------------------------------------------------

TILE_PX = 16        # tile size in pixels
ZOOM_DEFAULT = 3    # display zoom factor

def ng_color_to_rgb(ng_word: int):
    """Convert a 16-bit NeoGeo palette word to (r, g, b) each 0-255."""
    dark      = (ng_word >> 15) & 1
    red_lsb   = (ng_word >> 14) & 1
    green_lsb = (ng_word >> 13) & 1
    blue_lsb  = (ng_word >> 12) & 1
    red_0     = (ng_word >> 11) & 1
    red_1     = (ng_word >> 10) & 1
    red_2     = (ng_word >>  9) & 1
    red_3     = (ng_word >>  8) & 1
    green_0   = (ng_word >>  7) & 1
    green_1   = (ng_word >>  6) & 1
    green_2   = (ng_word >>  5) & 1
    green_3   = (ng_word >>  4) & 1
    blue_0    = (ng_word >>  3) & 1
    blue_1    = (ng_word >>  2) & 1
    blue_2    = (ng_word >>  1) & 1
    blue_3    =  ng_word        & 1

    r5 = (red_0   << 4) | (red_1   << 3) | (red_2   << 2) | (red_3   << 1) | red_lsb
    g5 = (green_0 << 4) | (green_1 << 3) | (green_2 << 2) | (green_3 << 1) | green_lsb
    b5 = (blue_0  << 4) | (blue_1  << 3) | (blue_2  << 2) | (blue_3  << 1) | blue_lsb

    r = (r5 << 3) | (r5 >> 2)
    g = (g5 << 3) | (g5 >> 2)
    b = (b5 << 3) | (b5 >> 2)

    if dark:
        r >>= 1
        g >>= 1
        b >>= 1

    return (r & 0xFF, g & 0xFF, b & 0xFF)


def rgb_to_ng_color(r: int, g: int, b: int) -> int:
    """Convert (r, g, b) 8-bit each to 16-bit NeoGeo palette word."""
    r5 = r >> 3
    g5 = g >> 3
    b5 = b >> 3
    red_lsb   = r5 & 1
    red_0     = (r5 >> 4) & 1
    red_1     = (r5 >> 3) & 1
    red_2     = (r5 >> 2) & 1
    red_3     = (r5 >> 1) & 1
    green_lsb = g5 & 1
    green_0   = (g5 >> 4) & 1
    green_1   = (g5 >> 3) & 1
    green_2   = (g5 >> 2) & 1
    green_3   = (g5 >> 1) & 1
    blue_lsb  = b5 & 1
    blue_0    = (b5 >> 4) & 1
    blue_1    = (b5 >> 3) & 1
    blue_2    = (b5 >> 2) & 1
    blue_3    = (b5 >> 1) & 1
    return (
        (red_lsb << 14) | (green_lsb << 13) | (blue_lsb << 12) |
        (red_0 << 11) | (red_1 << 10) | (red_2 << 9) | (red_3 << 8) |
        (green_0 << 7) | (green_1 << 6) | (green_2 << 5) | (green_3 << 4) |
        (blue_0 << 3) | (blue_1 << 2) | (blue_2 << 1) | blue_3
    )


def decode_block(c1: bytes, c2: bytes, offset: int):
    """Decode one 8x8 block from C-ROMs at byte offset. Returns 8x8 np array of color indices."""
    pixels = np.zeros((8, 8), dtype=np.uint8)
    for row in range(8):
        base = offset + row * 2
        # romtiles.py wrote [plane_b, plane_a] in c1; after /f byte-flip → [plane_a, plane_b]
        plane_a = c1[base]
        plane_b = c1[base + 1]
        plane_c = c2[base]
        plane_d = c2[base + 1]
        for p in range(8):
            bit_a = (plane_a >> p) & 1
            bit_b = (plane_b >> p) & 1
            bit_c = (plane_c >> p) & 1
            bit_d = (plane_d >> p) & 1
            idx = (bit_d << 3) | (bit_c << 2) | (bit_b << 1) | bit_a
            pixels[row, p] = idx
    return pixels


def decode_tile(c1: bytes, c2: bytes, tile_index: int):
    """Decode a 16x16 tile (4 blocks) from C-ROMs. Returns 16x16 colour-index array.
    Each tile occupies 64 bytes in each C-ROM file (4 blocks × 8 rows × 2 bytes).
    """
    BLOCK_BYTES = 16    # bytes per 8x8 block in each C-ROM
    TILE_BYTES  = 64    # 4 blocks × 16 bytes

    base = tile_index * TILE_BYTES
    if base + TILE_BYTES > len(c1) or base + TILE_BYTES > len(c2):
        return np.zeros((16, 16), dtype=np.uint8)

    # block order written by romtiles: block1(top-right), block2(bot-right),
    #                                  block3(top-left),  block4(bot-left)
    blk1 = decode_block(c1, c2, base + 0 * BLOCK_BYTES)   # top-right
    blk2 = decode_block(c1, c2, base + 1 * BLOCK_BYTES)   # bot-right
    blk3 = decode_block(c1, c2, base + 2 * BLOCK_BYTES)   # top-left
    blk4 = decode_block(c1, c2, base + 3 * BLOCK_BYTES)   # bot-left

    tile = np.zeros((16, 16), dtype=np.uint8)
    tile[0:8,  8:16] = blk1   # top-right quadrant
    tile[8:16, 8:16] = blk2   # bot-right quadrant
    tile[0:8,  0:8]  = blk3   # top-left quadrant
    tile[8:16, 0:8]  = blk4   # bot-left quadrant
    return tile


def tile_to_pixmap(tile_idx: np.ndarray, palette_rgb: list, zoom=1,
                   bg_color=(80, 80, 80)):
    """Convert 16x16 colour-index array + palette list → QPixmap."""
    h, w = tile_idx.shape
    img = QImage(w * zoom, h * zoom, QImage.Format.Format_RGB32)
    for y in range(h):
        for x in range(w):
            ci = int(tile_idx[y, x])
            if ci == 0:
                r, g, b = bg_color
            else:
                r, g, b = palette_rgb[ci] if ci < len(palette_rgb) else (255, 0, 255)
            for dy in range(zoom):
                for dx in range(zoom):
                    img.setPixel(x * zoom + dx, y * zoom + dy,
                                 QColor(r, g, b).rgb())
    return QPixmap.fromImage(img)


# ---------------------------------------------------------------------------
# Palette / neopal.bin helpers
# ---------------------------------------------------------------------------
PALETTE_RECORD_SIZE = 136  # 4 (index) + 16*8 (uint64 colours) in neopal.bin

def load_neopal(path="neopal.bin"):
    """Return dict[image_index] → list of 16 (r,g,b) tuples."""
    result = {}
    if not os.path.exists(path):
        return result
    data = open(path, "rb").read()
    n = len(data) // PALETTE_RECORD_SIZE
    for i in range(n):
        off = i * PALETTE_RECORD_SIZE
        img_idx = struct.unpack_from("i", data, off)[0]
        colors = []
        for c in range(16):
            ng = struct.unpack_from("Q", data, off + 4 + c * 8)[0]
            colors.append(ng_color_to_rgb(ng & 0xFFFF))
        result[img_idx] = colors
    return result


def load_manifest(path="assets_manifest.json"):
    if not os.path.exists(path):
        return []
    with open(path) as f:
        return json.load(f)


# ---------------------------------------------------------------------------
# Tile Grid Widget
# ---------------------------------------------------------------------------
class TileGridWidget(QWidget):
    tileSelected = pyqtSignal(int)  # tile index

    def __init__(self, c1: bytes, c2: bytes, palettes: dict,
                 manifest: list, parent=None):
        super().__init__(parent)
        self.c1 = c1
        self.c2 = c2
        self.palettes = palettes
        self.manifest = manifest
        self.total_tiles = len(c1) // 64
        self.zoom = ZOOM_DEFAULT
        self.cols = 16              # tiles per row in the grid view
        self.selected_tile = -1
        self.pal_index = 0          # which palette to use (image index in neopal)
        self._tiles_cache = {}
        self._compute_size()
        self.setMouseTracking(True)
        self.setCursor(QCursor(Qt.CursorShape.CrossCursor))

    def _compute_size(self):
        tw = TILE_PX * self.zoom
        rows = math.ceil(self.total_tiles / self.cols)
        self.setMinimumSize(self.cols * tw + 2, rows * tw + 2)

    def set_zoom(self, z: int):
        self.zoom = z
        self._tiles_cache.clear()
        self._compute_size()
        self.update()

    def set_palette(self, img_idx: int):
        self.pal_index = img_idx
        self._tiles_cache.clear()
        self.update()

    def _get_palette(self):
        return self.palettes.get(self.pal_index, [(i*16, i*16, i*16) for i in range(16)])

    def _render_tile(self, t: int) -> QPixmap:
        key = (t, self.pal_index, self.zoom)
        if key not in self._tiles_cache:
            idx_arr = decode_tile(self.c1, self.c2, t)
            pal = self._get_palette()
            px = tile_to_pixmap(idx_arr, pal, self.zoom)
            self._tiles_cache[key] = px
        return self._tiles_cache[key]

    def paintEvent(self, event):
        p = QPainter(self)
        tw = TILE_PX * self.zoom
        rect = event.rect()
        col0 = max(0, rect.x() // tw)
        col1 = min(self.cols, rect.right() // tw + 1)
        row0 = max(0, rect.y() // tw)
        row1 = min(math.ceil(self.total_tiles / self.cols), rect.bottom() // tw + 1)

        for row in range(row0, row1):
            for col in range(col0, col1):
                t = row * self.cols + col
                if t >= self.total_tiles:
                    break
                px = self._render_tile(t)
                x, y = col * tw, row * tw
                p.drawPixmap(x, y, px)
                if t == self.selected_tile:
                    p.setPen(QPen(QColor(255, 255, 0), 2))
                    p.drawRect(x + 1, y + 1, tw - 2, tw - 2)

    def mousePressEvent(self, ev):
        tw = TILE_PX * self.zoom
        col = ev.position().x() // tw
        row = ev.position().y() // tw
        t = int(row) * self.cols + int(col)
        if 0 <= t < self.total_tiles:
            self.selected_tile = t
            self.update()
            self.tileSelected.emit(t)

    def mouseMoveEvent(self, ev):
        tw = TILE_PX * self.zoom
        col = int(ev.position().x() // tw)
        row = int(ev.position().y() // tw)
        t = row * self.cols + col
        if 0 <= t < self.total_tiles:
            # find which image owns this tile from manifest
            name = "?"
            for entry in self.manifest:
                if entry.get("tile_base", -1) <= t <= entry.get("tile_reserved_last", -1):
                    name = entry.get("name", "?")
                    break
            self.setToolTip(f"Tile {t}  ({name})")


# ---------------------------------------------------------------------------
# Sprite Designer Widget
# ---------------------------------------------------------------------------
class SpritePreviewWidget(QWidget):
    """Renders a multi-strip sprite group from tile grid data."""

    def __init__(self, c1, c2, palettes, parent=None):
        super().__init__(parent)
        self.c1 = c1
        self.c2 = c2
        self.palettes = palettes
        self.tile_base = 0
        self.strips = 3
        self.rows = 3
        self.stride = 16
        self.pal_index = 0
        self.zoom = 4
        self.bg = QColor(64, 64, 80)
        self.setMinimumSize(320, 320)

    def set_sprite(self, tile_base, strips, rows, stride, pal_index):
        self.tile_base = tile_base
        self.strips = strips
        self.rows = rows
        self.stride = stride
        self.pal_index = pal_index
        self.update()

    def paintEvent(self, event):
        p = QPainter(self)
        p.fillRect(self.rect(), self.bg)
        tw = TILE_PX * self.zoom
        pal = self.palettes.get(self.pal_index, [(i*16,)*3 for i in range(16)])
        ox = (self.width()  - self.strips * tw) // 2
        oy = (self.height() - self.rows   * tw) // 2
        for strip in range(self.strips):
            for row in range(self.rows):
                t = self.tile_base + strip + row * self.stride
                if t >= len(self.c1) // 64:
                    continue
                idx_arr = decode_tile(self.c1, self.c2, t)
                px = tile_to_pixmap(idx_arr, pal, self.zoom, bg_color=(64, 64, 80))
                p.drawPixmap(ox + strip * tw, oy + row * tw, px)
        # draw grid overlay
        p.setPen(QPen(QColor(0, 200, 0, 80), 1))
        for s in range(self.strips + 1):
            p.drawLine(ox + s * tw, oy, ox + s * tw, oy + self.rows * tw)
        for r in range(self.rows + 1):
            p.drawLine(ox, oy + r * tw, ox + self.strips * tw, oy + r * tw)


class SpriteDesignerTab(QWidget):
    def __init__(self, c1, c2, palettes, manifest, parent=None):
        super().__init__(parent)
        self.c1 = c1
        self.c2 = c2
        self.palettes = palettes
        self.manifest = manifest
        self._build_ui()

    def _build_ui(self):
        root = QHBoxLayout(self)

        # Left: controls
        ctrl = QVBoxLayout()
        ctrl.setSpacing(6)

        # Asset picker
        g_asset = QGroupBox("Asset")
        gl = QVBoxLayout(g_asset)
        self.asset_combo = QComboBox()
        for entry in self.manifest:
            self.asset_combo.addItem(
                f"{entry['screen_id']:3d} {entry['name']}",
                entry
            )
        self.asset_combo.currentIndexChanged.connect(self._asset_changed)
        gl.addWidget(self.asset_combo)
        ctrl.addWidget(g_asset)

        # Manual tile/strips/rows
        g_man = QGroupBox("Manual overrides")
        gm = QGridLayout(g_man)
        gm.addWidget(QLabel("Tile base"), 0, 0)
        self.spin_tile = QSpinBox(); self.spin_tile.setRange(0, 999999)
        gm.addWidget(self.spin_tile, 0, 1)
        gm.addWidget(QLabel("Strips"), 1, 0)
        self.spin_strips = QSpinBox(); self.spin_strips.setRange(1, 16)
        self.spin_strips.setValue(3)
        gm.addWidget(self.spin_strips, 1, 1)
        gm.addWidget(QLabel("Rows"), 2, 0)
        self.spin_rows = QSpinBox(); self.spin_rows.setRange(1, 16)
        self.spin_rows.setValue(3)
        gm.addWidget(self.spin_rows, 2, 1)
        gm.addWidget(QLabel("Stride"), 3, 0)
        self.spin_stride = QSpinBox(); self.spin_stride.setRange(1, 32)
        self.spin_stride.setValue(16)
        gm.addWidget(self.spin_stride, 3, 1)
        gm.addWidget(QLabel("Palette img"), 4, 0)
        self.spin_pal = QSpinBox(); self.spin_pal.setRange(0, 255)
        gm.addWidget(self.spin_pal, 4, 1)
        gm.addWidget(QLabel("Zoom"), 5, 0)
        self.spin_zoom = QSpinBox(); self.spin_zoom.setRange(1, 8)
        self.spin_zoom.setValue(4)
        gm.addWidget(self.spin_zoom, 5, 1)
        for spin in (self.spin_tile, self.spin_strips, self.spin_rows,
                     self.spin_stride, self.spin_pal, self.spin_zoom):
            spin.valueChanged.connect(self._refresh)
        ctrl.addWidget(g_man)

        # Export C snippet
        btn_export = QPushButton("Export C snippet")
        btn_export.clicked.connect(self._export_c)
        ctrl.addWidget(btn_export)
        ctrl.addStretch()

        # Right: preview
        self.preview = SpritePreviewWidget(self.c1, self.c2, self.palettes)
        scroll = QScrollArea()
        scroll.setWidget(self.preview)
        scroll.setWidgetResizable(True)

        root.addLayout(ctrl, 0)
        root.addWidget(scroll, 1)

        # Init
        if self.manifest:
            self._asset_changed(0)

    def _asset_changed(self, idx):
        entry = self.asset_combo.itemData(idx)
        if not entry:
            return
        self.spin_tile.setValue(entry.get("tile_base", 0))
        self.spin_strips.setValue(entry.get("sprite_strips", 3))
        self.spin_rows.setValue(entry.get("sprite_active_rows", 3))
        self.spin_stride.setValue(16)
        self.spin_pal.setValue(entry.get("palette_bank", 0) - 1)
        self._refresh()

    def _refresh(self):
        z = self.spin_zoom.value()
        self.preview.zoom = z
        w = self.spin_strips.value() * TILE_PX * z + 40
        h = self.spin_rows.value()   * TILE_PX * z + 40
        self.preview.setMinimumSize(max(w, 200), max(h, 200))
        self.preview.set_sprite(
            self.spin_tile.value(),
            self.spin_strips.value(),
            self.spin_rows.value(),
            self.spin_stride.value(),
            self.spin_pal.value()
        )

    def _export_c(self):
        tile = self.spin_tile.value()
        strips = self.spin_strips.value()
        rows = self.spin_rows.value()
        stride = self.spin_stride.value()
        pal = self.spin_pal.value() + 1  # palette_bank is 1-indexed
        c = (f"/* Sprite: tile_base={tile}, {strips} strips, {rows} rows, "
             f"stride={stride}, palette_bank={pal} */\n"
             f"char_set_sprite(c, firstSprite, {strips}, {rows}, {tile}, {pal});\n"
             f"c->sprite_stride = {stride};\n"
             f"c->sprite_active_rows = {rows};\n")
        dlg = QDialog(self)
        dlg.setWindowTitle("C Snippet")
        lay = QVBoxLayout(dlg)
        txt = QPlainTextEdit(c)
        txt.setFont(QFont("Courier", 10))
        txt.setReadOnly(True)
        lay.addWidget(txt)
        bb = QDialogButtonBox(QDialogButtonBox.StandardButton.Ok)
        bb.accepted.connect(dlg.accept)
        lay.addWidget(bb)
        dlg.exec()


# ---------------------------------------------------------------------------
# Hitbox Editor Widget
# ---------------------------------------------------------------------------
class HitboxPreviewWidget(QWidget):
    def __init__(self, c1, c2, palettes, parent=None):
        super().__init__(parent)
        self.c1 = c1
        self.c2 = c2
        self.palettes = palettes
        self.tile_base = 0
        self.strips = 3
        self.rows = 3
        self.stride = 16
        self.pal_index = 0
        self.zoom = 4
        self.body_rect = QRect(4, 8, 40, 40)
        self.hit_rect  = QRect(8, 4, 32, 32)
        self.dragging = None   # 'body' or 'hit'
        self.drag_corner = None
        self._drag_start = None
        self.setMouseTracking(True)
        self.setMinimumSize(320, 320)

    def _sprite_origin(self):
        tw = TILE_PX * self.zoom
        ox = (self.width()  - self.strips * tw) // 2
        oy = (self.height() - self.rows   * tw) // 2
        return ox, oy

    def paintEvent(self, event):
        p = QPainter(self)
        p.fillRect(self.rect(), QColor(48, 48, 60))
        tw = TILE_PX * self.zoom
        pal = self.palettes.get(self.pal_index, [(i*16,)*3 for i in range(16)])
        ox, oy = self._sprite_origin()
        for strip in range(self.strips):
            for row in range(self.rows):
                t = self.tile_base + strip + row * self.stride
                if t >= len(self.c1) // 64:
                    continue
                idx_arr = decode_tile(self.c1, self.c2, t)
                px = tile_to_pixmap(idx_arr, pal, self.zoom, bg_color=(48, 48, 60))
                p.drawPixmap(ox + strip * tw, oy + row * tw, px)
        # body rect (green)
        p.setPen(QPen(QColor(0, 255, 0), 2))
        p.drawRect(ox + self.body_rect.x() * self.zoom,
                   oy + self.body_rect.y() * self.zoom,
                   self.body_rect.width()  * self.zoom,
                   self.body_rect.height() * self.zoom)
        # hit rect (red)
        p.setPen(QPen(QColor(255, 80, 80), 2))
        p.drawRect(ox + self.hit_rect.x() * self.zoom,
                   oy + self.hit_rect.y() * self.zoom,
                   self.hit_rect.width()  * self.zoom,
                   self.hit_rect.height() * self.zoom)
        # legend
        p.setPen(QColor(255, 255, 255))
        p.drawText(4, 14, "Green = body  Red = hit")

    def mousePressEvent(self, ev):
        ox, oy = self._sprite_origin()
        gx = (ev.position().x() - ox) / self.zoom
        gy = (ev.position().y() - oy) / self.zoom
        if self.body_rect.contains(int(gx), int(gy)):
            self.dragging = 'body'
        elif self.hit_rect.contains(int(gx), int(gy)):
            self.dragging = 'hit'
        else:
            self.dragging = None
        self._drag_start = (gx, gy)

    def mouseMoveEvent(self, ev):
        if not self.dragging or not self._drag_start:
            return
        ox, oy = self._sprite_origin()
        gx = (ev.position().x() - ox) / self.zoom
        gy = (ev.position().y() - oy) / self.zoom
        dx = int(gx - self._drag_start[0])
        dy = int(gy - self._drag_start[1])
        r = self.body_rect if self.dragging == 'body' else self.hit_rect
        r.translate(dx, dy)
        self._drag_start = (gx, gy)
        self.update()
        self.parentWidget().parentWidget().parentWidget()._update_readout()

    def mouseReleaseEvent(self, ev):
        self.dragging = None


class HitboxEditorTab(QWidget):
    def __init__(self, c1, c2, palettes, manifest, parent=None):
        super().__init__(parent)
        self.c1 = c1
        self.c2 = c2
        self.palettes = palettes
        self.manifest = manifest
        self._build_ui()

    def _build_ui(self):
        root = QHBoxLayout(self)
        ctrl = QVBoxLayout()
        ctrl.setSpacing(6)

        g_asset = QGroupBox("Asset")
        gl = QVBoxLayout(g_asset)
        self.asset_combo = QComboBox()
        for entry in self.manifest:
            self.asset_combo.addItem(f"{entry['screen_id']:3d} {entry['name']}", entry)
        self.asset_combo.currentIndexChanged.connect(self._asset_changed)
        gl.addWidget(self.asset_combo)
        ctrl.addWidget(g_asset)

        g_box = QGroupBox("Body rect (green)")
        gb = QGridLayout(g_box)
        self.body_spins = {}
        for i, name in enumerate(("x", "y", "w", "h")):
            gb.addWidget(QLabel(name), i, 0)
            s = QSpinBox(); s.setRange(-128, 512)
            if name in ("w", "h"): s.setValue(40)
            else: s.setValue(4)
            s.valueChanged.connect(self._spin_changed)
            gb.addWidget(s, i, 1)
            self.body_spins[name] = s
        ctrl.addWidget(g_box)

        g_hit = QGroupBox("Hit rect (red)")
        gh = QGridLayout(g_hit)
        self.hit_spins = {}
        for i, name in enumerate(("x", "y", "w", "h")):
            gh.addWidget(QLabel(name), i, 0)
            s = QSpinBox(); s.setRange(-128, 512)
            if name in ("w", "h"): s.setValue(32)
            else: s.setValue(8)
            s.valueChanged.connect(self._spin_changed)
            gh.addWidget(s, i, 1)
            self.hit_spins[name] = s
        ctrl.addWidget(g_hit)

        g_zoom = QGroupBox("Zoom")
        gz = QHBoxLayout(g_zoom)
        self.zoom_spin = QSpinBox(); self.zoom_spin.setRange(1, 8)
        self.zoom_spin.setValue(4)
        self.zoom_spin.valueChanged.connect(self._zoom_changed)
        gz.addWidget(self.zoom_spin)
        ctrl.addWidget(g_zoom)

        btn_export = QPushButton("Export C struct")
        btn_export.clicked.connect(self._export_c)
        ctrl.addWidget(btn_export)
        ctrl.addStretch()

        self.preview = HitboxPreviewWidget(self.c1, self.c2, self.palettes, self)
        scroll = QScrollArea()
        scroll.setWidget(self.preview)
        scroll.setWidgetResizable(True)
        root.addLayout(ctrl, 0)
        root.addWidget(scroll, 1)

        if self.manifest:
            self._asset_changed(0)

    def _asset_changed(self, idx):
        entry = self.asset_combo.itemData(idx)
        if not entry:
            return
        self.preview.tile_base = entry.get("tile_base", 0)
        self.preview.strips    = entry.get("sprite_strips", 3)
        self.preview.rows      = entry.get("sprite_active_rows", 3)
        self.preview.stride    = 16
        self.preview.pal_index = entry.get("palette_bank", 1) - 1
        self.preview.update()

    def _spin_changed(self):
        self.preview.body_rect = QRect(
            self.body_spins["x"].value(), self.body_spins["y"].value(),
            self.body_spins["w"].value(), self.body_spins["h"].value())
        self.preview.hit_rect = QRect(
            self.hit_spins["x"].value(),  self.hit_spins["y"].value(),
            self.hit_spins["w"].value(),  self.hit_spins["h"].value())
        self.preview.update()

    def _zoom_changed(self):
        self.preview.zoom = self.zoom_spin.value()
        self.preview.update()

    def _update_readout(self):
        r = self.preview.body_rect
        self.body_spins["x"].setValue(r.x())
        self.body_spins["y"].setValue(r.y())
        self.body_spins["w"].setValue(r.width())
        self.body_spins["h"].setValue(r.height())
        r = self.preview.hit_rect
        self.hit_spins["x"].setValue(r.x())
        self.hit_spins["y"].setValue(r.y())
        self.hit_spins["w"].setValue(r.width())
        self.hit_spins["h"].setValue(r.height())

    def _export_c(self):
        br = self.preview.body_rect
        hr = self.preview.hit_rect
        c = (f"char_set_body(c, {br.x()}, {br.y()}, {br.width()}, {br.height()});\n"
             f"char_set_hit(c,  {hr.x()}, {hr.y()}, {hr.width()}, {hr.height()});\n")
        dlg = QDialog(self)
        dlg.setWindowTitle("C Struct")
        lay = QVBoxLayout(dlg)
        txt = QPlainTextEdit(c)
        txt.setFont(QFont("Courier", 10))
        txt.setReadOnly(True)
        lay.addWidget(txt)
        bb = QDialogButtonBox(QDialogButtonBox.StandardButton.Ok)
        bb.accepted.connect(dlg.accept)
        lay.addWidget(bb)
        dlg.exec()


# ---------------------------------------------------------------------------
# Pixel Art Painter Widget
# ---------------------------------------------------------------------------
class PaintCanvas(QWidget):
    """Edit a single 16x16 tile pixel by pixel."""
    tileEdited = pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.pixels = np.zeros((16, 16), dtype=np.uint8)
        self.palette_rgb = [(i * 16, i * 16, i * 16) for i in range(16)]
        self.palette_ng  = [0] * 16
        self.current_color = 1
        self.zoom = 20
        self.grid = True
        self.setMinimumSize(16 * self.zoom, 16 * self.zoom)
        self.setMouseTracking(True)
        self._drawing = False

    def load_tile(self, tile_idx: np.ndarray, pal_rgb: list, pal_ng: list):
        self.pixels = tile_idx.copy()
        self.palette_rgb = pal_rgb[:]
        self.palette_ng  = pal_ng[:]
        self.update()

    def set_color(self, ci: int):
        self.current_color = ci

    def paintEvent(self, event):
        p = QPainter(self)
        z = self.zoom
        for y in range(16):
            for x in range(16):
                ci = int(self.pixels[y, x])
                r, g, b = self.palette_rgb[ci] if ci < len(self.palette_rgb) else (0, 0, 0)
                p.fillRect(x * z, y * z, z, z, QColor(r, g, b))
        if self.grid:
            p.setPen(QPen(QColor(80, 80, 80, 160), 1))
            for i in range(17):
                p.drawLine(i * z, 0, i * z, 16 * z)
                p.drawLine(0, i * z, 16 * z, i * z)

    def _paint_at(self, ev):
        z = self.zoom
        x = int(ev.position().x() // z)
        y = int(ev.position().y() // z)
        if 0 <= x < 16 and 0 <= y < 16:
            btn = ev.buttons()
            if btn & Qt.MouseButton.LeftButton:
                ci = self.current_color
            elif btn & Qt.MouseButton.RightButton:
                ci = 0
            else:
                return
            if self.pixels[y, x] != ci:
                self.pixels[y, x] = ci
                self.update()
                self.tileEdited.emit()

    def mousePressEvent(self, ev):
        self._drawing = True
        self._paint_at(ev)

    def mouseMoveEvent(self, ev):
        if self._drawing:
            self._paint_at(ev)

    def mouseReleaseEvent(self, ev):
        self._drawing = False


class PaletteSwatchWidget(QWidget):
    colorSelected = pyqtSignal(int)

    def __init__(self, palette_rgb: list, parent=None):
        super().__init__(parent)
        self.palette = palette_rgb
        self.selected = 1
        self.setMinimumSize(16 * 20, 20)
        self.setMaximumHeight(24)

    def set_palette(self, pal: list):
        self.palette = pal
        self.update()

    def paintEvent(self, event):
        p = QPainter(self)
        sz = self.height()
        for i, (r, g, b) in enumerate(self.palette):
            p.fillRect(i * sz, 0, sz, sz, QColor(r, g, b))
            if i == self.selected:
                p.setPen(QPen(QColor(255, 255, 0), 2))
                p.drawRect(i * sz + 1, 1, sz - 2, sz - 2)

    def mousePressEvent(self, ev):
        sz = self.height()
        ci = int(ev.position().x() // sz)
        if 0 <= ci < len(self.palette):
            self.selected = ci
            self.update()
            self.colorSelected.emit(ci)


class PixelPaintTab(QWidget):
    def __init__(self, c1: bytearray, c2: bytearray,
                 palettes: dict, manifest: list, parent=None):
        super().__init__(parent)
        self.c1 = c1
        self.c2 = c2
        self.palettes = palettes
        self.manifest = manifest
        self.current_tile = 0
        self.current_pal_idx = 0
        self._build_ui()

    def _build_ui(self):
        root = QVBoxLayout(self)

        # Top toolbar
        toolbar = QHBoxLayout()
        toolbar.addWidget(QLabel("Asset:"))
        self.asset_combo = QComboBox()
        for e in self.manifest:
            self.asset_combo.addItem(f"{e['screen_id']:3d} {e['name']}", e)
        self.asset_combo.currentIndexChanged.connect(self._asset_changed)
        toolbar.addWidget(self.asset_combo, 1)

        toolbar.addWidget(QLabel("Tile offset:"))
        self.tile_off = QSpinBox(); self.tile_off.setRange(0, 255)
        self.tile_off.valueChanged.connect(self._tile_changed)
        toolbar.addWidget(self.tile_off)

        self.btn_save = QPushButton("Save tile to ROM")
        self.btn_save.clicked.connect(self._save_tile)
        toolbar.addWidget(self.btn_save)

        self.btn_export_png = QPushButton("Export PNG")
        self.btn_export_png.clicked.connect(self._export_png)
        toolbar.addWidget(self.btn_export_png)
        root.addLayout(toolbar)

        # Palette swatch
        pal = [(i * 16, i * 16, i * 16) for i in range(16)]
        self.swatch = PaletteSwatchWidget(pal)
        self.swatch.colorSelected.connect(self._color_selected)
        root.addWidget(self.swatch)

        # Canvas
        self.canvas = PaintCanvas()
        self.canvas.tileEdited.connect(self._on_edit)
        scroll = QScrollArea()
        scroll.setWidget(self.canvas)
        scroll.setWidgetResizable(False)
        root.addWidget(scroll, 1)

        # Status
        self.status_lbl = QLabel("Select an asset to start painting")
        root.addWidget(self.status_lbl)

        if self.manifest:
            self._asset_changed(0)

    def _asset_changed(self, idx):
        entry = self.asset_combo.itemData(idx)
        if not entry:
            return
        self.current_tile = entry.get("tile_base", 0)
        self.current_pal_idx = entry.get("palette_bank", 1) - 1
        self.tile_off.setValue(0)
        self._load_tile()

    def _tile_changed(self):
        e = self.asset_combo.currentData()
        if e:
            self.current_tile = e.get("tile_base", 0) + self.tile_off.value()
        self._load_tile()

    def _load_tile(self):
        pal_rgb = self.palettes.get(self.current_pal_idx,
                                    [(i*16,)*3 for i in range(16)])
        pal_ng  = [0] * 16  # placeholder (not used for display)
        tile_arr = decode_tile(self.c1, self.c2, self.current_tile)
        self.canvas.load_tile(tile_arr, pal_rgb, pal_ng)
        self.swatch.set_palette(pal_rgb)
        self.status_lbl.setText(
            f"Tile {self.current_tile}  palette_img={self.current_pal_idx}")

    def _color_selected(self, ci):
        self.canvas.set_color(ci)

    def _on_edit(self):
        self.status_lbl.setText(
            f"Tile {self.current_tile} — edited (unsaved)")

    def _save_tile(self):
        """Write edited pixels back to C-ROM byte arrays."""
        pixels = self.canvas.pixels
        t = self.current_tile
        base = t * 128

        def encode_block_from_pixels(block, c1_out, c2_out, off):
            for row in range(8):
                colors = [int(block[row, col]) for col in range(8)]
                plane_a = sum(((colors[p] >> 0) & 1) << p for p in range(8))
                plane_b = sum(((colors[p] >> 1) & 1) << p for p in range(8))
                plane_c = sum(((colors[p] >> 2) & 1) << p for p in range(8))
                plane_d = sum(((colors[p] >> 3) & 1) << p for p in range(8))
                # 052-c1.c1 is byte-flipped: order is [plane_a, plane_b]
                c1_out[off + row * 2]     = plane_a
                c1_out[off + row * 2 + 1] = plane_b
                c2_out[off + row * 2]     = plane_c
                c2_out[off + row * 2 + 1] = plane_d

        blk1 = pixels[0:8,  8:16]   # top-right
        blk2 = pixels[8:16, 8:16]   # bot-right
        blk3 = pixels[0:8,  0:8]    # top-left
        blk4 = pixels[8:16, 0:8]    # bot-left
        encode_block_from_pixels(blk1, self.c1, self.c2, base +  0)
        encode_block_from_pixels(blk2, self.c1, self.c2, base + 16)
        encode_block_from_pixels(blk3, self.c1, self.c2, base + 32)
        encode_block_from_pixels(blk4, self.c1, self.c2, base + 48)
        self.status_lbl.setText(f"Tile {t} saved to ROM buffer. Use File > Save ROMs to write to disk.")

    def _export_png(self):
        pal_rgb = self.palettes.get(self.current_pal_idx,
                                    [(i*16,)*3 for i in range(16)])
        px = tile_to_pixmap(self.canvas.pixels, pal_rgb, zoom=4,
                            bg_color=(0, 0, 0))
        path, _ = QFileDialog.getSaveFileName(
            self, "Export tile PNG", f"tile_{self.current_tile}.png",
            "PNG (*.png)")
        if path:
            px.save(path)


# ---------------------------------------------------------------------------
# Tile Grid Tab wrapper
# ---------------------------------------------------------------------------
class TileGridTab(QWidget):
    tileSelected = pyqtSignal(int)

    def __init__(self, c1, c2, palettes, manifest, parent=None):
        super().__init__(parent)
        self.c1 = c1
        self.c2 = c2
        self.palettes = palettes
        self.manifest = manifest
        self._build_ui()

    def _build_ui(self):
        root = QVBoxLayout(self)

        # Toolbar
        tb = QHBoxLayout()
        tb.addWidget(QLabel("Zoom:"))
        self.zoom_spin = QSpinBox()
        self.zoom_spin.setRange(1, 8)
        self.zoom_spin.setValue(ZOOM_DEFAULT)
        self.zoom_spin.valueChanged.connect(self._zoom_changed)
        tb.addWidget(self.zoom_spin)

        tb.addWidget(QLabel("Palette img:"))
        self.pal_spin = QSpinBox()
        self.pal_spin.setRange(0, 255)
        self.pal_spin.valueChanged.connect(self._pal_changed)
        tb.addWidget(self.pal_spin)

        tb.addWidget(QLabel("Asset:"))
        self.asset_combo = QComboBox()
        self.asset_combo.addItem("(all tiles)", -1)
        for e in self.manifest:
            self.asset_combo.addItem(f"{e['screen_id']:3d} {e['name']}", e)
        self.asset_combo.currentIndexChanged.connect(self._jump_to_asset)
        tb.addWidget(self.asset_combo, 1)

        self.info_lbl = QLabel("Click a tile to select")
        tb.addWidget(self.info_lbl)
        tb.addStretch()
        root.addLayout(tb)

        # Grid
        self.grid = TileGridWidget(self.c1, self.c2, self.palettes, self.manifest)
        self.grid.tileSelected.connect(self._on_tile_selected)
        scroll = QScrollArea()
        scroll.setWidget(self.grid)
        scroll.setWidgetResizable(False)
        root.addWidget(scroll, 1)

    def _zoom_changed(self, z):
        self.grid.set_zoom(z)

    def _pal_changed(self, v):
        self.grid.set_palette(v)

    def _jump_to_asset(self, idx):
        e = self.asset_combo.itemData(idx)
        if e and isinstance(e, dict):
            self.pal_spin.setValue(e.get("palette_bank", 1) - 1)

    def _on_tile_selected(self, t):
        self.info_lbl.setText(f"Tile {t}")
        self.tileSelected.emit(t)


# ---------------------------------------------------------------------------
# Main Window
# ---------------------------------------------------------------------------
class ArtboxStudio(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Artbox Studio — NeoGeoSDK")
        self.resize(1200, 800)

        self.c1: bytearray = bytearray()
        self.c2: bytearray = bytearray()
        self.palettes: dict = {}
        self.manifest: list = []

        self._load_data()
        self._build_ui()
        self._build_menu()

    def _load_data(self):
        # Load C-ROMs (use 052-c1.c1 / 052-c2.c2 in artbox directory)
        for fname, attr in (("052-c1.c1", "c1"), ("052-c2.c2", "c2")):
            if os.path.exists(fname):
                with open(fname, "rb") as f:
                    setattr(self, attr, bytearray(f.read()))
        self.palettes = load_neopal("neopal.bin")
        self.manifest = load_manifest("assets_manifest.json")

    def _build_ui(self):
        tabs = QTabWidget()
        tabs.setDocumentMode(True)
        self.setCentralWidget(tabs)

        self.tab_grid = TileGridTab(self.c1, self.c2, self.palettes, self.manifest)
        tabs.addTab(self.tab_grid, "Tile Grid")

        self.tab_sprite = SpriteDesignerTab(self.c1, self.c2, self.palettes, self.manifest)
        tabs.addTab(self.tab_sprite, "Sprite Designer")

        self.tab_hitbox = HitboxEditorTab(self.c1, self.c2, self.palettes, self.manifest)
        tabs.addTab(self.tab_hitbox, "Hitbox Editor")

        self.tab_paint = PixelPaintTab(self.c1, self.c2, self.palettes, self.manifest)
        tabs.addTab(self.tab_paint, "Pixel Paint")

        self.setStatusBar(QStatusBar())
        total = len(self.c1) // 64
        self.statusBar().showMessage(
            f"Loaded {total} tiles from C-ROMs  |  {len(self.manifest)} assets  |  "
            f"{len(self.palettes)} palettes")

    def _build_menu(self):
        mb = self.menuBar()
        fm = mb.addMenu("File")

        act_reload = fm.addAction("Reload ROMs")
        act_reload.triggered.connect(self._reload)

        act_save = fm.addAction("Save ROMs (write painted tiles to disk)")
        act_save.triggered.connect(self._save_roms)

        fm.addSeparator()
        act_quit = fm.addAction("Quit")
        act_quit.triggered.connect(self.close)

        vm = mb.addMenu("View")
        act_dark = vm.addAction("Toggle dark/light background")
        act_dark.triggered.connect(self._toggle_bg)
        self._dark_bg = True

    def _reload(self):
        self._load_data()
        # refresh tabs by rebuilding
        self._build_ui()
        self.statusBar().showMessage("ROMs reloaded.")

    def _save_roms(self):
        for fname, attr in (("052-c1.c1", "c1"), ("052-c2.c2", "c2")):
            with open(fname, "wb") as f:
                f.write(getattr(self, attr))
        self.statusBar().showMessage("C-ROMs saved to disk.")

    def _toggle_bg(self):
        self._dark_bg = not self._dark_bg
        bg = QColor(48, 48, 60) if self._dark_bg else QColor(200, 200, 200)
        if hasattr(self.tab_sprite, "preview"):
            self.tab_sprite.preview.bg = bg
            self.tab_sprite.preview.update()


def main():
    # Run from artbox directory
    artbox_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(artbox_dir)

    app = QApplication(sys.argv)
    app.setApplicationName("Artbox Studio")
    app.setStyle("Fusion")

    # Dark palette
    from PyQt6.QtGui import QPalette
    pal = app.palette()
    pal.setColor(QPalette.ColorRole.Window,          QColor(45, 45, 48))
    pal.setColor(QPalette.ColorRole.WindowText,      QColor(220, 220, 220))
    pal.setColor(QPalette.ColorRole.Base,            QColor(30, 30, 30))
    pal.setColor(QPalette.ColorRole.AlternateBase,   QColor(55, 55, 55))
    pal.setColor(QPalette.ColorRole.Text,            QColor(220, 220, 220))
    pal.setColor(QPalette.ColorRole.Button,          QColor(55, 55, 55))
    pal.setColor(QPalette.ColorRole.ButtonText,      QColor(220, 220, 220))
    pal.setColor(QPalette.ColorRole.Highlight,       QColor(42, 130, 218))
    pal.setColor(QPalette.ColorRole.HighlightedText, QColor(0, 0, 0))
    app.setPalette(pal)

    win = ArtboxStudio()
    win.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
