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
                # 777-c1.c1 is byte-flipped: order is [plane_a, plane_b]
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
# ROM Address Grid Tab
# ---------------------------------------------------------------------------
class RomAddressGridTab(QWidget):
    """Browse all assets with their NeoGeo C-ROM addresses, tile bases,
    palette bank registers, and character addresses."""

    def __init__(self, manifest: list, parent=None):
        super().__init__(parent)
        self.manifest = manifest
        self._build_ui()
        self._populate()

    def _build_ui(self):
        layout = QVBoxLayout(self)

        # Filter row
        frow = QHBoxLayout()
        frow.addWidget(QLabel("Filter:"))
        self.filter_edit = QLineEdit(); self.filter_edit.setPlaceholderText("name, category…")
        self.filter_edit.textChanged.connect(self._apply_filter)
        frow.addWidget(self.filter_edit)
        frow.addWidget(QLabel("Category:"))
        self.cat_combo = QComboBox()
        self.cat_combo.addItem("All")
        self.cat_combo.currentTextChanged.connect(self._apply_filter)
        frow.addWidget(self.cat_combo)
        frow.addStretch()
        layout.addLayout(frow)

        # Tree view
        self.tree = QTreeWidget()
        cols = [
            "Screen ID", "Name", "Category",
            "Tile Base", "Tile Base (hex)",
            "C1 Byte Offset", "C1 Addr (hex)",
            "Palette Bank", "PAL Register (hex)",
            "Strips", "Active Rows",
            "Col Start", "Row Start",
            "Width px", "Height px",
        ]
        self.tree.setColumnCount(len(cols))
        self.tree.setHeaderLabels(cols)
        self.tree.setAlternatingRowColors(True)
        self.tree.setSortingEnabled(True)
        layout.addWidget(self.tree)

        # Detail panel
        self.detail = QPlainTextEdit()
        self.detail.setReadOnly(True)
        self.detail.setMaximumHeight(120)
        self.detail.setFont(QFont("Monospace", 9))
        layout.addWidget(self.detail)
        self.tree.currentItemChanged.connect(self._on_selection)

    def _populate(self):
        cats = set()
        self.tree.clear()
        for e in self.manifest:
            cats.add(e.get("category","?"))
            tile_base = e.get("tile_base", 0)
            c1_off = tile_base * 64
            pal_bank = e.get("palette_bank", 0)
            # NeoGeo palette RAM register: PALETTES base + palette_bank * PALOFFSET
            # PALOFFSET = 0x200 (palette slot stride in VRAM words)
            pal_reg = 0x400000 + pal_bank * 0x200 * 2
            item = QTreeWidgetItem([
                str(e.get("screen_id", "")),
                e.get("name", ""),
                e.get("category", ""),
                str(tile_base),
                f"0x{tile_base:04X}",
                str(c1_off),
                f"0x{c1_off:06X}",
                str(pal_bank),
                f"0x{pal_reg:06X}",
                str(e.get("sprite_strips", "")),
                str(e.get("sprite_active_rows", "")),
                str(e.get("used_tile_col_start", "")),
                str(e.get("used_tile_row_start", "")),
                str(e.get("content_width", "")),
                str(e.get("content_height", "")),
            ])
            item.setData(0, Qt.ItemDataRole.UserRole, e)
            self.tree.addTopLevelItem(item)
        for i in range(self.tree.columnCount()):
            self.tree.resizeColumnToContents(i)
        cats_sorted = sorted(cats)
        for c in cats_sorted:
            self.cat_combo.addItem(c)

    def _apply_filter(self):
        text = self.filter_edit.text().lower()
        cat  = self.cat_combo.currentText()
        for i in range(self.tree.topLevelItemCount()):
            item = self.tree.topLevelItem(i)
            e = item.data(0, Qt.ItemDataRole.UserRole) or {}
            name_match = text in item.text(1).lower() if text else True
            cat_match  = cat == "All" or e.get("category","") == cat
            item.setHidden(not (name_match and cat_match))

    def _on_selection(self, item, _prev):
        if item is None:
            return
        e = item.data(0, Qt.ItemDataRole.UserRole) or {}
        tile_base = e.get("tile_base", 0)
        pal_bank  = e.get("palette_bank", 0)
        strips    = e.get("sprite_strips", 1)
        rows      = e.get("sprite_active_rows", 1)
        c1_off    = tile_base * 64
        c2_off    = tile_base * 64
        # NeoGeo char_set_sprite C snippet
        snippet = (
            f"/* {e.get('name','')} */\n"
            f"char_set_sprite(c, x, y, {strips}, {rows},\n"
            f"                {tile_base}, {pal_bank});\n"
            f"/* C1 byte offset: 0x{c1_off:06X}  C2 byte offset: 0x{c2_off:06X} */\n"
            f"/* Palette VRAM addr: 0x{0x400000 + pal_bank*0x400:06X} */"
        )
        self.detail.setPlainText(snippet)


# ---------------------------------------------------------------------------
# Palette Manager Tab
# ---------------------------------------------------------------------------
class PaletteSwatchGrid(QWidget):
    """Display up to 16 color swatches for one palette bank."""
    color_clicked = pyqtSignal(int)   # slot index 0-15

    SWATCH_SIZE = 28

    def __init__(self, parent=None):
        super().__init__(parent)
        self.colors = [(0,0,0)] * 16
        self.selected = 0
        self.setFixedSize(self.SWATCH_SIZE * 8, self.SWATCH_SIZE * 2 + 4)

    def set_palette(self, colors: list):
        self.colors = list(colors[:16]) + [(0,0,0)] * (16 - len(colors))
        self.update()

    def paintEvent(self, event):
        p = QPainter(self)
        p.fillRect(self.rect(), QColor(30, 30, 40))
        for i, (r, g, b) in enumerate(self.colors):
            col = i % 8; row = i // 8
            x = col * self.SWATCH_SIZE + 1
            y = row * self.SWATCH_SIZE + 2
            sw = self.SWATCH_SIZE - 2
            p.fillRect(x, y, sw, sw, QColor(r, g, b))
            if i == self.selected:
                p.setPen(QPen(QColor(255, 220, 0), 2))
                p.drawRect(x, y, sw, sw)
            else:
                p.setPen(QPen(QColor(60, 60, 70), 1))
                p.drawRect(x, y, sw, sw)
            # Slot number
            p.setPen(QColor(200, 200, 200) if (r+g+b) < 300 else QColor(30, 30, 30))
            p.setFont(QFont("Monospace", 6))
            p.drawText(x + 2, y + sw - 2, str(i))

    def mousePressEvent(self, event):
        x = int(event.position().x()); y = int(event.position().y())
        col = x // self.SWATCH_SIZE; row = y // self.SWATCH_SIZE
        idx = row * 8 + col
        if 0 <= idx < 16:
            self.selected = idx
            self.color_clicked.emit(idx)
            self.update()


class PaletteManagerTab(QWidget):
    """Browse all palette banks loaded from neopal.bin, view swatches,
    copy NeoGeo word values, and export the palette data."""

    def __init__(self, palettes: dict, manifest: list, parent=None):
        super().__init__(parent)
        self.palettes = palettes
        self.manifest = manifest
        self._build_ui()
        self._populate()

    def _build_ui(self):
        layout = QHBoxLayout(self)

        # Left: bank list
        left = QVBoxLayout()
        left.addWidget(QLabel("Palette Banks (by asset):"))
        self.bank_list = QListWidget()
        self.bank_list.setMaximumWidth(220)
        self.bank_list.currentRowChanged.connect(self._on_bank_selected)
        left.addWidget(self.bank_list)
        layout.addLayout(left)

        # Center: swatches + slot detail
        center = QVBoxLayout()
        center.addWidget(QLabel("Colors (click to select slot):"))
        self.swatch_grid = PaletteSwatchGrid()
        self.swatch_grid.color_clicked.connect(self._on_color_clicked)
        center.addWidget(self.swatch_grid)

        self.slot_label = QLabel("Slot 0")
        self.slot_label.setFont(QFont("Monospace", 9))
        center.addWidget(self.slot_label)

        # NeoGeo word table
        center.addWidget(QLabel("All 16 NeoGeo palette words (for C code):"))
        self.ng_words = QPlainTextEdit()
        self.ng_words.setReadOnly(True)
        self.ng_words.setFont(QFont("Monospace", 9))
        self.ng_words.setMaximumHeight(140)
        center.addWidget(self.ng_words)

        # C setpal snippet
        center.addWidget(QLabel("C setpal() snippet:"))
        self.c_snippet = QPlainTextEdit()
        self.c_snippet.setReadOnly(True)
        self.c_snippet.setFont(QFont("Monospace", 9))
        self.c_snippet.setMaximumHeight(120)
        center.addWidget(self.c_snippet)
        center.addStretch()
        layout.addLayout(center, 2)

        # Right: info
        right = QVBoxLayout()
        right.addWidget(QLabel("Palette info:"))
        self.info_label = QLabel("")
        self.info_label.setWordWrap(True)
        self.info_label.setFont(QFont("Monospace", 9))
        right.addWidget(self.info_label)
        right.addStretch()
        layout.addLayout(right)

    def _populate(self):
        self.bank_list.clear()
        # Map palette_bank → asset name from manifest
        bank_names = {}
        for e in self.manifest:
            pb = e.get("palette_bank", 0)
            bank_names[pb] = e.get("name", f"bank {pb}")
        for img_idx in sorted(self.palettes.keys()):
            name = bank_names.get(img_idx, f"bank {img_idx}")
            self.bank_list.addItem(f"[{img_idx:3d}]  {name}")
        if self.bank_list.count():
            self.bank_list.setCurrentRow(0)

    def _on_bank_selected(self, row):
        if row < 0:
            return
        keys = sorted(self.palettes.keys())
        if row >= len(keys):
            return
        img_idx = keys[row]
        colors = self.palettes[img_idx]
        self.swatch_grid.set_palette(colors)
        self._refresh_words(img_idx, colors)

    def _refresh_words(self, img_idx, colors):
        ng_lines = []
        for i, (r, g, b) in enumerate(colors):
            ng = rgb_to_ng_color(r, g, b)
            ng_lines.append(f"  [{i:2d}] 0x{ng:04X}   rgb({r:3d},{g:3d},{b:3d})")
        self.ng_words.setPlainText("\n".join(ng_lines))

        # C setpal snippet
        vals = []
        for r, g, b in colors:
            ng = rgb_to_ng_color(r, g, b)
            vals.append(f"0x{ng:04X}")
        snippet = (f"uint16_t pal[16];\n"
                   f"setpal(pal, 0,\n"
                   f"  {', '.join(vals[:8])},\n"
                   f"  {', '.join(vals[8:])});\n"
                   f"load_palettes(pal, PALETTES + PALOFFSET * {img_idx});")
        self.c_snippet.setPlainText(snippet)
        self.info_label.setText(
            f"Bank index: {img_idx}\n"
            f"Palette bank reg: 0x{0x400000 + img_idx*0x400:06X}\n"
            f"Colors: {len(colors)}"
        )

    def _on_color_clicked(self, slot):
        row = self.bank_list.currentRow()
        if row < 0:
            return
        keys = sorted(self.palettes.keys())
        img_idx = keys[row]
        colors = self.palettes[img_idx]
        if slot >= len(colors):
            return
        r, g, b = colors[slot]
        ng = rgb_to_ng_color(r, g, b)
        self.slot_label.setText(
            f"Slot {slot}:  rgb({r},{g},{b})  NeoGeo word: 0x{ng:04X}"
        )


# ---------------------------------------------------------------------------
# Manual Palette Editor Tab
# ---------------------------------------------------------------------------
class ColorBlockWidget(QWidget):
    """Single large color block — click to open color picker."""
    from PyQt6.QtCore import pyqtSignal as _sig
    color_changed = _sig(int, int, int)   # r, g, b

    def __init__(self, r=0, g=0, b=0, parent=None):
        super().__init__(parent)
        self.r = r; self.g = g; self.b = b
        self.setFixedSize(40, 40)
        self.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))

    def set_color(self, r, g, b):
        self.r = r; self.g = g; self.b = b
        self.update()

    def paintEvent(self, event):
        p = QPainter(self)
        p.fillRect(self.rect(), QColor(self.r, self.g, self.b))
        p.setPen(QPen(QColor(120,120,120), 1))
        p.drawRect(0, 0, self.width()-1, self.height()-1)

    def mousePressEvent(self, event):
        from PyQt6.QtWidgets import QColorDialog
        c = QColorDialog.getColor(QColor(self.r, self.g, self.b), self)
        if c.isValid():
            self.r = c.red(); self.g = c.green(); self.b = c.blue()
            self.update()
            self.color_changed.emit(self.r, self.g, self.b)


class ManualPaletteEditorTab(QWidget):
    """Manually build a 16-color NeoGeo palette, preview it, and export
    a C setpal() call and raw NeoGeo words."""

    def __init__(self, palettes: dict, parent=None):
        super().__init__(parent)
        self.palettes = palettes
        self._colors = [(0,0,0)] * 16    # current working palette
        self._build_ui()
        self._refresh_output()

    def _build_ui(self):
        layout = QHBoxLayout(self)

        # Left: import from bank
        left = QVBoxLayout()
        left.addWidget(QLabel("Import from bank:"))
        self.import_spin = QSpinBox(); self.import_spin.setRange(0, 255)
        left.addWidget(self.import_spin)
        btn_import = QPushButton("Import →")
        btn_import.clicked.connect(self._import_bank)
        left.addWidget(btn_import)
        left.addWidget(QLabel("Bank name:"))
        self.name_edit = QLineEdit(); self.name_edit.setText("My Palette")
        left.addWidget(self.name_edit)
        left.addWidget(QLabel("Palette slot (for export):"))
        self.slot_spin = QSpinBox(); self.slot_spin.setRange(0, 255)
        left.addWidget(self.slot_spin)
        left.addStretch()
        layout.addLayout(left)

        # Center: 16 color blocks in a 8×2 grid
        center = QVBoxLayout()
        center.addWidget(QLabel("Click any swatch to pick a color:"))
        grid_w = QWidget()
        grid = QGridLayout(grid_w)
        grid.setSpacing(4)
        self._blocks = []
        for i in range(16):
            col = i % 8; row = i // 8
            block = ColorBlockWidget()
            block.color_changed.connect(lambda r, g, b, idx=i: self._on_color_changed(idx, r, g, b))
            grid.addWidget(block, row, col)
            self._blocks.append(block)
        center.addWidget(grid_w)

        # RGB spinboxes for fine control
        rgb_row = QHBoxLayout()
        self.sel_label = QLabel("Slot 0:")
        rgb_row.addWidget(self.sel_label)
        self.r_spin = QSpinBox(); self.r_spin.setRange(0,255); self.r_spin.setPrefix("R:")
        self.g_spin = QSpinBox(); self.g_spin.setRange(0,255); self.g_spin.setPrefix("G:")
        self.b_spin = QSpinBox(); self.b_spin.setRange(0,255); self.b_spin.setPrefix("B:")
        for w in (self.r_spin, self.g_spin, self.b_spin):
            rgb_row.addWidget(w)
        btn_apply = QPushButton("Apply")
        btn_apply.clicked.connect(self._apply_rgb)
        rgb_row.addWidget(btn_apply)
        self._selected_slot = 0
        for block in self._blocks:
            block.mousePressEvent  # wire selection below
        center.addLayout(rgb_row)

        # Wire block click → select slot
        for i, block in enumerate(self._blocks):
            block.mousePressEvent = self._make_slot_click(i, block)

        center.addStretch()
        layout.addLayout(center, 2)

        # Right: output
        right = QVBoxLayout()
        right.addWidget(QLabel("NeoGeo palette words:"))
        self.ng_out = QPlainTextEdit()
        self.ng_out.setReadOnly(True)
        self.ng_out.setFont(QFont("Monospace", 9))
        self.ng_out.setMaximumHeight(180)
        right.addWidget(self.ng_out)

        right.addWidget(QLabel("C setpal() snippet:"))
        self.c_out = QPlainTextEdit()
        self.c_out.setReadOnly(True)
        self.c_out.setFont(QFont("Monospace", 9))
        self.c_out.setMaximumHeight(140)
        right.addWidget(self.c_out)

        btn_row = QHBoxLayout()
        btn_copy = QPushButton("Copy C snippet")
        btn_copy.clicked.connect(lambda: QApplication.clipboard().setText(self.c_out.toPlainText()))
        btn_row.addWidget(btn_copy)
        right.addLayout(btn_row)
        right.addStretch()
        layout.addLayout(right)

    def _make_slot_click(self, idx, block):
        orig = ColorBlockWidget.mousePressEvent
        def handler(event):
            self._selected_slot = idx
            self.sel_label.setText(f"Slot {idx}:")
            r, g, b = self._colors[idx]
            self.r_spin.setValue(r); self.g_spin.setValue(g); self.b_spin.setValue(b)
            orig(block, event)
        return handler

    def _on_color_changed(self, slot, r, g, b):
        self._colors[slot] = (r, g, b)
        self._refresh_output()

    def _apply_rgb(self):
        slot = self._selected_slot
        r = self.r_spin.value(); g = self.g_spin.value(); b = self.b_spin.value()
        self._colors[slot] = (r, g, b)
        self._blocks[slot].set_color(r, g, b)
        self._refresh_output()

    def _import_bank(self):
        bank = self.import_spin.value()
        if bank in self.palettes:
            for i, (r, g, b) in enumerate(self.palettes[bank][:16]):
                self._colors[i] = (r, g, b)
                self._blocks[i].set_color(r, g, b)
            self._refresh_output()
        else:
            QMessageBox.warning(self, "Not Found", f"Palette bank {bank} not loaded.")

    def _refresh_output(self):
        slot = self.slot_spin.value()
        lines = []
        vals  = []
        for i, (r, g, b) in enumerate(self._colors):
            ng = rgb_to_ng_color(r, g, b)
            lines.append(f"  [{i:2d}] 0x{ng:04X}   rgb({r:3d},{g:3d},{b:3d})")
            vals.append(f"0x{ng:04X}")
        self.ng_out.setPlainText("\n".join(lines))
        c_code = (f"/* {self.name_edit.text()} */\n"
                  f"uint16_t pal[16];\n"
                  f"setpal(pal, 0,\n"
                  f"  {', '.join(vals[:8])},\n"
                  f"  {', '.join(vals[8:])});\n"
                  f"load_palettes(pal, PALETTES + PALOFFSET * {slot});")
        self.c_out.setPlainText(c_code)


# ---------------------------------------------------------------------------
# Main Window
# ---------------------------------------------------------------------------
###############################################################################
#  Pipeline Runner Tab
#  ---------------------------------------------------------------------------
#  Run every artbox-pipeline step from inside the studio.  Each step has a
#  Run button, a status pill, and a shared scrollback log pane.
#
#  Steps wired up:
#    1. img2neo          (PNGs → indexed NeoGeo palettes / dithered)
#    2. img2neo --hd     (HD pipeline: bilateral + CLAHE + blue-noise)
#    3. genscreens       (generate showScreenN bodies)
#    4. gen_sprite_meta  (sprite metadata table)
#    5. fixtiles         (FIX-layer tiles → S1 ROM)
#    6. fixtiles --hd    (per-tile palette + sharp-text)
#    7. romdbimgimport   (PNG → C-ROM blocks)
#    8. romtiles         (C1/C2 ROM packer)
#    9. romdbfiximport   (FIX ROM packer)
#   10. createromdb      (regenerate ROM database)
#   11. ALL (art)        (full pipeline via `make art`)
###############################################################################
import subprocess as _ax_sub
import shlex      as _ax_shlex
from pathlib       import Path as _AxPath
from PyQt6.QtCore    import QProcess, QProcessEnvironment

# Repository root (one level up from artbox/)
_AX_REPO_ROOT = _AxPath(__file__).resolve().parent.parent
_AX_ARTBOX    = _AX_REPO_ROOT / "artbox"

PIPELINE_STEPS = [
    ("img2neo",         ["python3", str(_AX_ARTBOX / "img2neo.py"), "--batch"],
                        "Convert source PNGs → NeoGeo palettes (Floyd-Steinberg)"),
    ("img2neo HD",      ["python3", str(_AX_ARTBOX / "img2neo_hd.py")],
                        "HD photo pipeline (bilateral + CLAHE + blue-noise)"),
    ("genscreens",      ["python3", str(_AX_ARTBOX / "genscreens.py")],
                        "Generate showScreenN bodies + screens.c"),
    ("gen_sprite_meta", ["python3", str(_AX_ARTBOX / "gen_sprite_meta.py")],
                        "Build sprite metadata table (sprite_meta.h)"),
    ("fixtiles",        ["python3", str(_AX_ARTBOX / "fixtiles.py")],
                        "Pack FIX-layer 8x8 tiles → S1 ROM"),
    ("fixtiles HD",     ["python3", str(_AX_ARTBOX / "fixtiles_hd.py")],
                        "HD FIX pipeline (per-tile palette + sharp-text)"),
    ("romdbimgimport",  ["python3", str(_AX_ARTBOX / "romdbimgimport.py")],
                        "PNG → C-ROM blocks (sprite image data)"),
    ("romtiles",        ["python3", str(_AX_ARTBOX / "romtiles.py")],
                        "Pack C1/C2 ROM blocks into final C ROMs"),
    ("romdbfiximport",  ["python3", str(_AX_ARTBOX / "romdbfiximport.py")],
                        "Import FIX tiles into ROM database"),
    ("createromdb",     ["python3", str(_AX_ARTBOX / "createromdb.py")],
                        "Regenerate the ROM database (neorom.db)"),
    ("ALL (art)",       ["make", "art"],
                        "Full art pipeline via the top-level Makefile"),
]


class _ArtStatusPill(QLabel):
    """Tiny coloured status indicator (IDLE / RUNNING / OK / FAIL)."""
    def __init__(self):
        super().__init__("IDLE")
        self.setFixedWidth(72)
        self.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.set_state("idle")

    def set_state(self, state: str):
        styles = {
            "idle":    ("#3a3a48", "#bababa", "IDLE"),
            "running": ("#0066aa", "#ffffff", "RUNNING"),
            "ok":      ("#00aa55", "#ffffff", "OK"),
            "fail":    ("#aa3333", "#ffffff", "FAIL"),
            "skip":    ("#665533", "#ffcc66", "MISSING"),
        }
        bg, fg, txt = styles.get(state, styles["idle"])
        self.setText(txt)
        self.setStyleSheet(
            f"QLabel {{ background:{bg}; color:{fg}; padding:2px 6px;"
            f" border-radius:6px; font-weight:bold; font-size:10px; }}")


class PipelineRunnerTab(QWidget):
    """
    Tab that runs each artbox pipeline step independently or all in
    sequence.  Pipes the live output of every step into a single
    scrollback so the user can see exactly what tool failed and where.
    """
    def __init__(self):
        super().__init__()
        self._proc      = None
        self._queue     = []
        self._step_rows = []

        title = QLabel("<b>Artbox Pipeline Runner</b>")
        intro = QLabel(
            "Run any single step or chain the lot.  Stops on the first "
            "failure to surface the error.  GAME selection is honoured "
            "from games/$(CURRENT_GAME)/."
        )
        intro.setWordWrap(True)

        grid_box = QGroupBox("Pipeline Steps")
        grid     = QGridLayout(grid_box)
        grid.addWidget(QLabel("<b>Step</b>"),    0, 0)
        grid.addWidget(QLabel("<b>Status</b>"),  0, 1)
        grid.addWidget(QLabel("<b>Description</b>"), 0, 2)
        grid.addWidget(QLabel("<b>Action</b>"),  0, 3)

        for row, (name, cmd, desc) in enumerate(PIPELINE_STEPS, start=1):
            grid.addWidget(QLabel(name), row, 0)
            pill = _ArtStatusPill()
            # Auto-mark missing scripts so the user sees what's installed
            if cmd[0] == "python3" and not os.path.exists(cmd[1]):
                pill.set_state("skip")
            grid.addWidget(pill, row, 1)
            grid.addWidget(QLabel(desc), row, 2)
            btn  = QPushButton("Run")
            btn.clicked.connect(lambda _, n=name, c=cmd: self._run_one(n, c))
            grid.addWidget(btn, row, 3)
            self._step_rows.append((name, btn, pill, cmd))

        action_row = QHBoxLayout()
        self.btn_all   = QPushButton("Run Full Pipeline (sequential)")
        self.btn_clear = QPushButton("Clear Log")
        self.btn_stop  = QPushButton("Stop")
        self.btn_all  .clicked.connect(self._run_all)
        self.btn_clear.clicked.connect(lambda: self.log.clear())
        self.btn_stop .clicked.connect(self._stop)
        self.btn_stop.setEnabled(False)
        action_row.addWidget(self.btn_all)
        action_row.addWidget(self.btn_clear)
        action_row.addWidget(self.btn_stop)
        action_row.addStretch()

        self.log = QPlainTextEdit()
        self.log.setReadOnly(True)
        self.log.setStyleSheet(
            "QPlainTextEdit { background:#0c0c10; color:#cfcf80;"
            " font-family:'Courier New',monospace; }")

        layout = QVBoxLayout(self)
        layout.addWidget(title)
        layout.addWidget(intro)
        layout.addWidget(grid_box)
        layout.addLayout(action_row)
        layout.addWidget(self.log, 1)

    def _log(self, text: str):
        self.log.appendPlainText(text)

    def _set_pill(self, name: str, state: str):
        for n, _btn, pill, _cmd in self._step_rows:
            if n == name:
                pill.set_state(state)
                return

    def _set_buttons_enabled(self, enabled: bool):
        for _n, btn, _pill, _cmd in self._step_rows:
            btn.setEnabled(enabled)
        self.btn_all.setEnabled(enabled)
        self.btn_stop.setEnabled(not enabled)

    def _run_one(self, name: str, cmd_list: list):
        if self._proc is not None:
            self._log("(busy — finish or stop the current step first)")
            return
        # Skip if a Python tool doesn't exist on disk
        if cmd_list[0] == "python3" and not os.path.exists(cmd_list[1]):
            self._set_pill(name, "skip")
            self._log(f"--- {name} SKIPPED (script not found: {cmd_list[1]}) ---")
            if self._queue:
                next_name, next_cmd = self._queue.pop(0)
                self._run_one(next_name, next_cmd)
            return
        self._set_pill(name, "running")
        self._set_buttons_enabled(False)
        self._current_name = name
        self._log(f"=== {name} ===")
        self._log("$ " + " ".join(_ax_shlex.quote(c) for c in cmd_list))

        proc = QProcess(self)
        proc.setWorkingDirectory(str(_AX_REPO_ROOT))
        proc.setProcessChannelMode(QProcess.ProcessChannelMode.MergedChannels)
        proc.readyReadStandardOutput.connect(self._on_stdout)
        proc.finished.connect(self._on_finished)
        proc.setProcessEnvironment(QProcessEnvironment.systemEnvironment())
        self._proc = proc
        proc.start(cmd_list[0], cmd_list[1:])

    def _on_stdout(self):
        if self._proc is None:
            return
        data = self._proc.readAllStandardOutput().data().decode(errors="replace")
        for line in data.splitlines():
            self._log(line)

    def _on_finished(self, code, _status):
        name = getattr(self, "_current_name", "?")
        if code == 0:
            self._set_pill(name, "ok")
            self._log(f"--- {name} OK ---")
        else:
            self._set_pill(name, "fail")
            self._log(f"--- {name} FAILED (exit {code}) ---")
            self._queue = []
        self._proc = None
        if self._queue:
            next_name, next_cmd = self._queue.pop(0)
            self._run_one(next_name, next_cmd)
        else:
            self._set_buttons_enabled(True)

    def _run_all(self):
        if self._proc is not None:
            return
        for n, _btn, pill, _cmd in self._step_rows:
            pill.set_state("idle")
        # Exclude HD variants (they're explicit opt-ins) and the
        # trailing "ALL (art)" alias from the chained sequence.
        self._queue = [(n, c) for n, _b, _p, c in self._step_rows
                       if "HD" not in n and not n.startswith("ALL")]
        if not self._queue:
            return
        name, cmd = self._queue.pop(0)
        self._run_one(name, cmd)

    def _stop(self):
        if self._proc is None:
            return
        self._proc.kill()
        self._queue = []


class AssetRulesTab(QWidget):
    """Browse / edit artbox/assets.cfg — the file that drives per-category
    fit and category bucketing for the pipeline."""
    def __init__(self):
        super().__init__()
        layout = QVBoxLayout(self)
        layout.addWidget(QLabel("<b>artbox/assets.cfg</b>"))

        self.editor = QPlainTextEdit()
        self.editor.setStyleSheet(
            "QPlainTextEdit { background:#0c0c10; color:#cfcf80;"
            " font-family:'Courier New',monospace; }")
        self._path = _AX_ARTBOX / "assets.cfg"
        try:
            self.editor.setPlainText(self._path.read_text(errors="replace"))
        except Exception as e:
            self.editor.setPlainText(f"(could not read: {e})")
        layout.addWidget(self.editor, 1)

        btn_row = QHBoxLayout()
        btn_save = QPushButton("Save")
        btn_reload = QPushButton("Reload from Disk")
        btn_save.clicked.connect(self._save)
        btn_reload.clicked.connect(self._reload)
        btn_row.addWidget(btn_save)
        btn_row.addWidget(btn_reload)
        btn_row.addStretch()
        layout.addLayout(btn_row)

    def _save(self):
        try:
            self._path.write_text(self.editor.toPlainText())
        except Exception as e:
            QMessageBox.warning(self, "Save Failed", str(e))

    def _reload(self):
        try:
            self.editor.setPlainText(self._path.read_text(errors="replace"))
        except Exception as e:
            QMessageBox.warning(self, "Reload Failed", str(e))


###############################################################################
#  Asset Browser — recursively list every PNG in artbox/in/* with thumbnail
###############################################################################
class AssetBrowserTab(QWidget):
    def __init__(self):
        super().__init__()
        from PyQt6.QtWidgets import QTreeWidget, QTreeWidgetItem
        layout = QVBoxLayout(self)
        layout.addWidget(QLabel("<b>Source Asset Browser</b>"))
        layout.addWidget(QLabel(
            "Every PNG under artbox/in/ + artbox/infix/.  Click a file "
            "to preview at native size."))

        splitter = QSplitter(Qt.Orientation.Horizontal)

        self.tree = QTreeWidget()
        self.tree.setHeaderLabels(["Asset", "Size", "Dim"])
        self.tree.setColumnWidth(0, 240)
        self.tree.itemSelectionChanged.connect(self._on_select)
        splitter.addWidget(self.tree)

        right = QWidget()
        rl = QVBoxLayout(right)
        rl.addWidget(QLabel("<b>Preview</b>"))
        self.preview = QLabel()
        self.preview.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.preview.setStyleSheet("background:#1a1a1a; border:1px solid #333;")
        self.preview.setMinimumSize(256, 256)
        rl.addWidget(self.preview, 1)

        self.info = QPlainTextEdit()
        self.info.setReadOnly(True)
        self.info.setMaximumHeight(120)
        self.info.setStyleSheet(
            "QPlainTextEdit { background:#0c0c10; color:#a0c0e0;"
            " font-family:'Courier New',monospace; }")
        rl.addWidget(self.info)

        btn = QPushButton("Refresh")
        btn.clicked.connect(self._populate)
        rl.addWidget(btn)

        splitter.addWidget(right)
        splitter.setSizes([320, 600])
        layout.addWidget(splitter, 1)

        self._populate()

    def _populate(self):
        from PyQt6.QtWidgets import QTreeWidgetItem
        self.tree.clear()
        for label, folder in [
            ("in/backgrounds",         _AX_ARTBOX / "in" / "backgrounds"),
            ("in/characters",          _AX_ARTBOX / "in" / "characters"),
            ("in/effects",             _AX_ARTBOX / "in" / "effects"),
            ("in/eyecatcher",          _AX_ARTBOX / "in" / "eyecatcher"),
            ("in/npcs",                _AX_ARTBOX / "in" / "npcs"),
            ("in/screens",             _AX_ARTBOX / "in" / "screens"),
            ("in/titles",              _AX_ARTBOX / "in" / "titles"),
            ("infix (FIX-layer art)",  _AX_ARTBOX / "infix"),
        ]:
            root = QTreeWidgetItem([label, "", ""])
            self.tree.addTopLevelItem(root)
            if not folder.exists():
                root.setText(1, "missing")
                continue
            try:
                for entry in sorted(folder.iterdir()):
                    if entry.is_file() and entry.suffix.lower() == ".png":
                        sz = entry.stat().st_size
                        dim = self._png_dim(entry)
                        item = QTreeWidgetItem(
                            [entry.name, f"{sz}", dim])
                        item.setData(
                            0, Qt.ItemDataRole.UserRole, str(entry))
                        root.addChild(item)
            except Exception as e:
                root.setText(1, f"err: {e}")
            root.setExpanded(False)

    def _png_dim(self, path):
        try:
            img = QImage(str(path))
            if img.isNull():
                return "?"
            return f"{img.width()}x{img.height()}"
        except Exception:
            return "?"

    def _on_select(self):
        items = self.tree.selectedItems()
        if not items:
            return
        p_str = items[0].data(0, Qt.ItemDataRole.UserRole)
        if not p_str:
            self.preview.clear()
            self.info.clear()
            return
        path = _AxPath(p_str)
        img = QImage(p_str)
        if img.isNull():
            self.preview.setText("(failed to load)")
            self.info.setPlainText(f"{path}\n(invalid PNG)")
            return
        # Scale preview to fit
        from PyQt6.QtGui import QPixmap
        pix = QPixmap.fromImage(img)
        max_size = self.preview.size()
        if pix.width() > max_size.width() or pix.height() > max_size.height():
            pix = pix.scaled(max_size,
                             Qt.AspectRatioMode.KeepAspectRatio,
                             Qt.TransformationMode.SmoothTransformation)
        self.preview.setPixmap(pix)
        self.info.setPlainText(
            f"Path:   {path}\n"
            f"Dim:    {img.width()}x{img.height()}\n"
            f"Size:   {path.stat().st_size} bytes\n"
            f"Format: {img.format()}\n"
            f"Colors: {'paletted' if img.format() == QImage.Format.Format_Indexed8 else 'RGB(A)'}"
        )


###############################################################################
#  HD Compare — convert any PNG through standard AND HD pipelines, A/B view
###############################################################################
class HdCompareTab(QWidget):
    def __init__(self):
        super().__init__()
        layout = QVBoxLayout(self)
        layout.addWidget(QLabel("<b>HD Conversion Compare</b>"))
        layout.addWidget(QLabel(
            "Convert a PNG with BOTH the standard and HD pipelines, "
            "then view the indexed outputs side by side.  Originals "
            "untouched on disk — outputs land in artbox/out/_hd_cmp/."))

        # Source picker row
        pick_row = QHBoxLayout()
        self.src_line = QLineEdit()
        self.src_line.setPlaceholderText(
            "Pick a PNG (e.g. artbox/in/characters/sprite_001_*.png)")
        btn_pick = QPushButton("Browse…")
        btn_pick.clicked.connect(self._pick_src)
        btn_run  = QPushButton("Convert (both pipelines)")
        btn_run.clicked.connect(self._convert)
        pick_row.addWidget(self.src_line, 1)
        pick_row.addWidget(btn_pick)
        pick_row.addWidget(btn_run)
        layout.addLayout(pick_row)

        # Side-by-side previews
        previews = QHBoxLayout()
        self.std_label = QLabel("(standard output)")
        self.hd_label  = QLabel("(HD output)")
        for lbl in (self.std_label, self.hd_label):
            lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
            lbl.setStyleSheet(
                "background:#1a1a1a; border:1px solid #333; min-height:280px;")
        std_box = QGroupBox("Standard (img2neo.py)")
        hd_box  = QGroupBox("HD (img2neo_hd.py)")
        for box, lbl in [(std_box, self.std_label), (hd_box, self.hd_label)]:
            bl = QVBoxLayout(box)
            bl.addWidget(lbl, 1)
        previews.addWidget(std_box, 1)
        previews.addWidget(hd_box, 1)
        layout.addLayout(previews, 1)

        # Log
        self.log = QPlainTextEdit()
        self.log.setReadOnly(True)
        self.log.setMaximumHeight(150)
        self.log.setStyleSheet(
            "QPlainTextEdit { background:#0c0c10; color:#cfcf80;"
            " font-family:'Courier New',monospace; }")
        layout.addWidget(self.log)

    def _pick_src(self):
        path, _ = QFileDialog.getOpenFileName(
            self, "Pick source PNG",
            str(_AX_ARTBOX / "in"), "PNG (*.png);;All Files (*)")
        if path:
            self.src_line.setText(path)

    def _convert(self):
        src = self.src_line.text().strip()
        if not src or not os.path.exists(src):
            self.log.appendPlainText("(no source selected)")
            return
        out_dir = _AX_ARTBOX / "out" / "_hd_cmp"
        out_dir.mkdir(parents=True, exist_ok=True)
        base = os.path.splitext(os.path.basename(src))[0]
        std_out = out_dir / f"{base}_std.png"
        hd_out  = out_dir / f"{base}_hd.png"
        for label, script, out in [
            ("standard", _AX_ARTBOX / "img2neo.py", std_out),
            ("HD",       _AX_ARTBOX / "img2neo_hd.py", hd_out),
        ]:
            if not script.exists():
                self.log.appendPlainText(f"--- {label} SKIPPED ({script} missing) ---")
                continue
            cmd = ["python3", str(script), src, str(out)]
            self.log.appendPlainText(f"$ {' '.join(cmd)}")
            try:
                r = _ax_sub.run(cmd, capture_output=True, text=True, timeout=120)
                self.log.appendPlainText(r.stdout)
                if r.returncode != 0:
                    self.log.appendPlainText(f"!! {label} failed: {r.stderr}")
                    continue
            except Exception as e:
                self.log.appendPlainText(f"!! {label} error: {e}")
                continue
            # Load result image into the preview
            if out.exists():
                img = QImage(str(out))
                if not img.isNull():
                    from PyQt6.QtGui import QPixmap
                    pix = QPixmap.fromImage(img)
                    target = self.std_label if label == "standard" else self.hd_label
                    sz = target.size()
                    pix = pix.scaled(sz,
                                     Qt.AspectRatioMode.KeepAspectRatio,
                                     Qt.TransformationMode.FastTransformation)
                    target.setPixmap(pix)


###############################################################################
#  ROM Inspector — show built ROM file inventory
###############################################################################
class RomInventoryTab(QWidget):
    def __init__(self):
        super().__init__()
        layout = QVBoxLayout(self)
        layout.addWidget(QLabel("<b>Built ROM Inventory</b>"))
        layout.addWidget(QLabel(
            "Every ROM kind (p1 / m1 / s1 / v1 / c1 / c2) for every game "
            "under roms/.  Refresh to pick up new builds."))

        from PyQt6.QtWidgets import QTableWidget, QTableWidgetItem, QHeaderView
        self.table = QTableWidget(0, 5)
        self.table.setHorizontalHeaderLabels(
            ["Game", "File", "Size (B)", "Modified", "Present"])
        self.table.horizontalHeader().setSectionResizeMode(
            QHeaderView.ResizeMode.ResizeToContents)
        layout.addWidget(self.table, 1)

        btn = QPushButton("Refresh")
        btn.clicked.connect(self._refresh)
        layout.addWidget(btn)
        self._refresh()

    def _refresh(self):
        from PyQt6.QtWidgets import QTableWidgetItem
        roms_dir = _AX_REPO_ROOT / "roms"
        rows = []
        if roms_dir.exists():
            for game_dir in sorted(roms_dir.iterdir()):
                if not game_dir.is_dir():
                    continue
                game = game_dir.name
                game_id = "???"
                mk = _AX_REPO_ROOT / "games" / game / "game.mk"
                if mk.exists():
                    for line in mk.read_text(errors="replace").splitlines():
                        if line.strip().startswith("GAME_ID"):
                            parts = line.split("=")
                            if len(parts) > 1:
                                game_id = parts[1].strip()
                                break
                for kind in ("p1", "m1", "s1", "v1", "c1", "c2"):
                    fname = f"{game_id}-{kind}.{kind}"
                    fpath = game_dir / fname
                    if fpath.exists():
                        st = fpath.stat()
                        import datetime
                        mtime = datetime.datetime.fromtimestamp(
                            st.st_mtime).strftime("%Y-%m-%d %H:%M:%S")
                        rows.append((game, fname, str(st.st_size), mtime, "yes"))
                    else:
                        rows.append((game, fname, "—", "—", "no"))
        self.table.setRowCount(len(rows))
        for r, row_data in enumerate(rows):
            for c, val in enumerate(row_data):
                item = QTableWidgetItem(val)
                if c == 4 and val == "no":
                    item.setForeground(QColor("#aa6666"))
                elif c == 4 and val == "yes":
                    item.setForeground(QColor("#66aa66"))
                self.table.setItem(r, c, item)


###############################################################################
#  Hex Sprite Inspector — view a tile's raw C-ROM bytes + decoded pixels +
#  let the user swap palette and instantly recolour
###############################################################################
class HexSpriteInspectorTab(QWidget):
    def __init__(self, c1, c2, palettes):
        super().__init__()
        self.c1 = c1
        self.c2 = c2
        self.palettes = palettes
        self._tile_idx = 0
        self._zoom = 8

        layout = QVBoxLayout(self)
        layout.addWidget(QLabel("<b>Hex Sprite Inspector</b>"))
        layout.addWidget(QLabel(
            "Pick a tile index, view its raw bytes from C1/C2, the "
            "decoded 16×16 pixel grid, and swap palettes live."))

        # Control row
        ctrl = QHBoxLayout()
        ctrl.addWidget(QLabel("Tile #"))
        max_tiles = max(1, len(self.c1) // 64)
        self.sp_tile = QSpinBox()
        self.sp_tile.setRange(0, max_tiles - 1)
        self.sp_tile.valueChanged.connect(self._on_tile)
        ctrl.addWidget(self.sp_tile)
        ctrl.addWidget(QLabel("Zoom"))
        self.sp_zoom = QSpinBox()
        self.sp_zoom.setRange(1, 24)
        self.sp_zoom.setValue(self._zoom)
        self.sp_zoom.valueChanged.connect(self._on_zoom)
        ctrl.addWidget(self.sp_zoom)
        ctrl.addWidget(QLabel("Palette"))
        self.cb_pal = QComboBox()
        for k in sorted(self.palettes.keys()):
            self.cb_pal.addItem(f"{k}", k)
        self.cb_pal.currentIndexChanged.connect(lambda _: self._refresh())
        ctrl.addWidget(self.cb_pal, 1)
        layout.addLayout(ctrl)

        # Big preview + hex panel splitter
        splitter = QSplitter(Qt.Orientation.Horizontal)

        # LEFT: preview + palette strip
        left = QWidget()
        left_l = QVBoxLayout(left)
        left_l.addWidget(QLabel("<b>Decoded pixels</b>"))
        self.preview = QLabel()
        self.preview.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.preview.setStyleSheet(
            "background:#1a1a1a; border:1px solid #333;")
        self.preview.setMinimumSize(256, 256)
        left_l.addWidget(self.preview, 1)
        left_l.addWidget(QLabel("<b>Palette (current)</b>"))
        self.pal_strip = QLabel()
        self.pal_strip.setFixedHeight(28)
        self.pal_strip.setStyleSheet("background:#1a1a1a; border:1px solid #333;")
        left_l.addWidget(self.pal_strip)
        splitter.addWidget(left)

        # RIGHT: hex dumps + tile palette indices
        right = QWidget()
        right_l = QVBoxLayout(right)
        right_l.addWidget(QLabel("<b>C1 ROM bytes (64 B / tile)</b>"))
        self.hex_c1 = QPlainTextEdit()
        self.hex_c1.setReadOnly(True)
        self.hex_c1.setMaximumHeight(140)
        self.hex_c1.setStyleSheet(
            "QPlainTextEdit { background:#0c0c10; color:#a0e0a0;"
            " font-family:'Courier New',monospace; }")
        right_l.addWidget(self.hex_c1)
        right_l.addWidget(QLabel("<b>C2 ROM bytes (64 B / tile)</b>"))
        self.hex_c2 = QPlainTextEdit()
        self.hex_c2.setReadOnly(True)
        self.hex_c2.setMaximumHeight(140)
        self.hex_c2.setStyleSheet(
            "QPlainTextEdit { background:#0c0c10; color:#a0c0e0;"
            " font-family:'Courier New',monospace; }")
        right_l.addWidget(self.hex_c2)
        right_l.addWidget(QLabel("<b>Pixel palette indices (16×16)</b>"))
        self.idx_grid = QPlainTextEdit()
        self.idx_grid.setReadOnly(True)
        self.idx_grid.setStyleSheet(
            "QPlainTextEdit { background:#0c0c10; color:#cfcf80;"
            " font-family:'Courier New',monospace; font-size:11px; }")
        right_l.addWidget(self.idx_grid, 1)
        splitter.addWidget(right)
        splitter.setSizes([400, 600])

        layout.addWidget(splitter, 1)
        self._refresh()

    def _on_tile(self, v):
        self._tile_idx = v
        self._refresh()

    def _on_zoom(self, v):
        self._zoom = v
        self._refresh()

    def _refresh(self):
        if not self.c1 or not self.c2:
            self.preview.setText("(no C-ROM loaded)")
            return
        tile = decode_tile(self.c1, self.c2, self._tile_idx)
        # Palette
        pal_key = self.cb_pal.currentData()
        pal_rgb = self.palettes.get(pal_key, [(0, 0, 0)] * 16)
        if len(pal_rgb) < 16:
            pal_rgb = list(pal_rgb) + [(0, 0, 0)] * (16 - len(pal_rgb))

        # Render preview
        pix = tile_to_pixmap(tile, pal_rgb, zoom=self._zoom)
        self.preview.setPixmap(pix)

        # Render palette strip (16 swatches)
        strip = QImage(16 * 16, 24, QImage.Format.Format_RGB32)
        strip.fill(QColor(20, 20, 30))
        painter = QPainter(strip)
        for i, (r, g, b) in enumerate(pal_rgb[:16]):
            painter.fillRect(i * 16, 0, 16, 24, QColor(r, g, b))
            painter.setPen(QColor(60, 60, 60))
            painter.drawRect(i * 16, 0, 15, 23)
        painter.end()
        from PyQt6.QtGui import QPixmap
        self.pal_strip.setPixmap(QPixmap.fromImage(strip))

        # Hex dump
        base = self._tile_idx * 64
        def hex_block(buf):
            out = []
            for r in range(4):
                row = buf[base + r * 16:base + (r + 1) * 16]
                out.append(" ".join(f"{b:02X}" for b in row))
            return "\n".join(out)
        self.hex_c1.setPlainText(hex_block(self.c1))
        self.hex_c2.setPlainText(hex_block(self.c2))

        # Pixel grid as palette indices
        rows = []
        for y in range(16):
            rows.append(" ".join(f"{tile[y, x]:X}" for x in range(16)))
        self.idx_grid.setPlainText("\n".join(rows))


###############################################################################
#  Movement / Animation Designer
#  ---------------------------------------------------------------------------
#  Build a frame sequence (a list of tile-group indices each with a duration)
#  and watch it loop in the preview at the chosen FPS.
###############################################################################
class MovementDesignerTab(QWidget):
    def __init__(self, c1, c2, palettes, manifest):
        super().__init__()
        self.c1 = c1
        self.c2 = c2
        self.palettes = palettes
        self.manifest = manifest
        self._frames = []     # list of dicts {"tile": int, "dur_ms": int}
        self._cursor = 0
        self._elapsed = 0
        self._anim_running = False
        self._zoom = 6

        layout = QVBoxLayout(self)
        layout.addWidget(QLabel("<b>Movement / Animation Designer</b>"))
        layout.addWidget(QLabel(
            "Build a frame sequence — tile index + duration per frame — "
            "and preview it looping.  Export prints C arrays you can "
            "paste into a scene."))

        # Top row: frame controls
        ctrl = QHBoxLayout()
        ctrl.addWidget(QLabel("Tile"))
        self.sp_tile = QSpinBox()
        max_tiles = max(1, len(self.c1) // 64)
        self.sp_tile.setRange(0, max_tiles - 1)
        ctrl.addWidget(self.sp_tile)
        ctrl.addWidget(QLabel("Dur (ms)"))
        self.sp_dur = QSpinBox()
        self.sp_dur.setRange(16, 4000)
        self.sp_dur.setSingleStep(33)
        self.sp_dur.setValue(100)
        ctrl.addWidget(self.sp_dur)
        ctrl.addWidget(QLabel("Palette"))
        self.cb_pal = QComboBox()
        for k in sorted(self.palettes.keys()):
            self.cb_pal.addItem(str(k), k)
        ctrl.addWidget(self.cb_pal, 1)
        btn_add = QPushButton("+ Add Frame")
        btn_add.clicked.connect(self._add_frame)
        ctrl.addWidget(btn_add)
        btn_del = QPushButton("- Remove Sel")
        btn_del.clicked.connect(self._del_frame)
        ctrl.addWidget(btn_del)
        layout.addLayout(ctrl)

        # Splitter: frame list + preview
        splitter = QSplitter(Qt.Orientation.Horizontal)

        # LEFT: frame list
        left = QWidget()
        ll = QVBoxLayout(left)
        ll.addWidget(QLabel("<b>Frame sequence</b>"))
        self.lst = QListWidget()
        self.lst.setSelectionMode(QListWidget.SelectionMode.SingleSelection)
        ll.addWidget(self.lst, 1)
        ll_btn = QHBoxLayout()
        for label, fn in [("↑ Up", self._move_up),
                          ("↓ Down", self._move_down),
                          ("Clear", self._clear_all)]:
            b = QPushButton(label)
            b.clicked.connect(fn)
            ll_btn.addWidget(b)
        ll.addLayout(ll_btn)
        splitter.addWidget(left)

        # RIGHT: animation preview + export
        right = QWidget()
        rl = QVBoxLayout(right)
        rl.addWidget(QLabel("<b>Loop preview</b>"))
        self.preview = QLabel()
        self.preview.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.preview.setStyleSheet(
            "background:#1a1a1a; border:1px solid #333; min-height:200px;")
        rl.addWidget(self.preview, 1)
        play_row = QHBoxLayout()
        self.btn_play = QPushButton("▶ Play")
        self.btn_play.clicked.connect(self._toggle)
        self.btn_step = QPushButton("Step →")
        self.btn_step.clicked.connect(self._step_one)
        play_row.addWidget(self.btn_play)
        play_row.addWidget(self.btn_step)
        play_row.addStretch()
        rl.addLayout(play_row)
        rl.addWidget(QLabel("<b>Export (C array)</b>"))
        self.exp = QPlainTextEdit()
        self.exp.setReadOnly(True)
        self.exp.setMaximumHeight(120)
        self.exp.setStyleSheet(
            "QPlainTextEdit { background:#0c0c10; color:#a0c0e0;"
            " font-family:'Courier New',monospace; font-size:11px; }")
        rl.addWidget(self.exp)
        splitter.addWidget(right)
        splitter.setSizes([280, 600])

        layout.addWidget(splitter, 1)

        self.timer = QTimer(self)
        self.timer.setInterval(33)
        self.timer.timeout.connect(self._tick)

    def _add_frame(self):
        f = {"tile": self.sp_tile.value(),
             "dur_ms": self.sp_dur.value(),
             "pal": self.cb_pal.currentData()}
        self._frames.append(f)
        self.lst.addItem(f"#{len(self._frames)-1}  tile={f['tile']}  {f['dur_ms']}ms  pal={f['pal']}")
        self._update_export()

    def _del_frame(self):
        row = self.lst.currentRow()
        if row < 0 or row >= len(self._frames):
            return
        del self._frames[row]
        self.lst.takeItem(row)
        self._update_export()

    def _clear_all(self):
        self._frames.clear()
        self.lst.clear()
        self._update_export()

    def _move_up(self):
        row = self.lst.currentRow()
        if row <= 0:
            return
        self._frames[row], self._frames[row-1] = self._frames[row-1], self._frames[row]
        self._rebuild_list()
        self.lst.setCurrentRow(row - 1)

    def _move_down(self):
        row = self.lst.currentRow()
        if row < 0 or row >= len(self._frames) - 1:
            return
        self._frames[row], self._frames[row+1] = self._frames[row+1], self._frames[row]
        self._rebuild_list()
        self.lst.setCurrentRow(row + 1)

    def _rebuild_list(self):
        self.lst.clear()
        for i, f in enumerate(self._frames):
            self.lst.addItem(
                f"#{i}  tile={f['tile']}  {f['dur_ms']}ms  pal={f['pal']}")
        self._update_export()

    def _toggle(self):
        if self._anim_running:
            self.timer.stop()
            self._anim_running = False
            self.btn_play.setText("▶ Play")
        elif self._frames:
            self._cursor = 0
            self._elapsed = 0
            self.timer.start()
            self._anim_running = True
            self.btn_play.setText("⏸ Pause")
            self._draw_frame()

    def _step_one(self):
        if not self._frames:
            return
        self._cursor = (self._cursor + 1) % len(self._frames)
        self._draw_frame()

    def _tick(self):
        if not self._frames:
            self.timer.stop()
            self._anim_running = False
            return
        self._elapsed += 33
        cur = self._frames[self._cursor]
        if self._elapsed >= cur["dur_ms"]:
            self._cursor = (self._cursor + 1) % len(self._frames)
            self._elapsed = 0
            self._draw_frame()

    def _draw_frame(self):
        if not self._frames:
            return
        f = self._frames[self._cursor]
        if not self.c1 or not self.c2:
            return
        tile = decode_tile(self.c1, self.c2, f["tile"])
        pal = self.palettes.get(f["pal"], [(0, 0, 0)] * 16)
        if len(pal) < 16:
            pal = list(pal) + [(0, 0, 0)] * (16 - len(pal))
        self.preview.setPixmap(tile_to_pixmap(tile, pal, zoom=self._zoom))

    def _update_export(self):
        if not self._frames:
            self.exp.setPlainText("/* (empty — add frames first) */")
            return
        lines = [
            "/* Generated by Artbox Studio — Movement Designer */",
            f"static const uint16_t anim_frame_tile[{len(self._frames)}] = {{",
            "    " + ", ".join(f"{f['tile']}u" for f in self._frames),
            "};",
            f"static const uint8_t  anim_frame_dur[{len(self._frames)}] = {{",
            "    " + ", ".join(f"{max(1, f['dur_ms']//16)}u" for f in self._frames)
                + "  /* duration in vblank frames (1/60s) */",
            "};",
        ]
        self.exp.setPlainText("\n".join(lines))


###############################################################################
#  Level Designer — paint a tilemap, export as a C array of tile indices
###############################################################################
class LevelDesignerTab(QWidget):
    def __init__(self, c1, c2, palettes):
        super().__init__()
        self.c1 = c1
        self.c2 = c2
        self.palettes = palettes
        self._cols = 20
        self._rows = 14
        self._cell_px = 24
        self._brush_tile = 0
        self._brush_pal = 0
        self._map = [[0] * self._cols for _ in range(self._rows)]
        self._pal_idx = [[0] * self._cols for _ in range(self._rows)]

        layout = QVBoxLayout(self)
        layout.addWidget(QLabel("<b>Level / Tilemap Designer</b>"))
        layout.addWidget(QLabel(
            f"Paint a {self._cols}×{self._rows} tilemap.  Click to paint, "
            "right-click to erase.  Export gives you a ready-to-paste C array."))

        # Control row
        ctrl = QHBoxLayout()
        ctrl.addWidget(QLabel("Brush tile"))
        self.sp_tile = QSpinBox()
        self.sp_tile.setRange(0, max(0, (len(self.c1) // 64) - 1))
        self.sp_tile.valueChanged.connect(
            lambda v: setattr(self, "_brush_tile", v))
        ctrl.addWidget(self.sp_tile)
        ctrl.addWidget(QLabel("Brush pal"))
        self.cb_pal = QComboBox()
        for k in sorted(self.palettes.keys()):
            self.cb_pal.addItem(str(k), k)
        self.cb_pal.currentIndexChanged.connect(
            lambda _: setattr(self, "_brush_pal",
                              self.cb_pal.currentData()))
        ctrl.addWidget(self.cb_pal, 1)
        btn_clear = QPushButton("Clear Map")
        btn_clear.clicked.connect(self._clear)
        ctrl.addWidget(btn_clear)
        btn_export = QPushButton("Update Export")
        btn_export.clicked.connect(self._refresh_export)
        ctrl.addWidget(btn_export)
        layout.addLayout(ctrl)

        # Splitter: canvas + export
        splitter = QSplitter(Qt.Orientation.Vertical)

        # Canvas
        self.canvas = QLabel()
        self.canvas.setStyleSheet("background:#0c0c10; border:1px solid #333;")
        self.canvas.setFixedSize(self._cols * self._cell_px,
                                 self._rows * self._cell_px)
        self.canvas.setAlignment(Qt.AlignmentFlag.AlignTop |
                                 Qt.AlignmentFlag.AlignLeft)
        self.canvas.mousePressEvent  = self._on_mouse
        self.canvas.mouseMoveEvent   = self._on_mouse
        splitter.addWidget(self.canvas)

        self.exp = QPlainTextEdit()
        self.exp.setReadOnly(True)
        self.exp.setStyleSheet(
            "QPlainTextEdit { background:#0c0c10; color:#a0e0a0;"
            " font-family:'Courier New',monospace; font-size:11px; }")
        splitter.addWidget(self.exp)
        splitter.setSizes([400, 200])
        layout.addWidget(splitter, 1)

        self._render()
        self._refresh_export()

    def _on_mouse(self, evt):
        x = int(evt.position().x()) // self._cell_px
        y = int(evt.position().y()) // self._cell_px
        if x < 0 or x >= self._cols or y < 0 or y >= self._rows:
            return
        if evt.buttons() & Qt.MouseButton.LeftButton:
            self._map[y][x] = self._brush_tile
            self._pal_idx[y][x] = self._brush_pal
        elif evt.buttons() & Qt.MouseButton.RightButton:
            self._map[y][x] = 0
            self._pal_idx[y][x] = 0
        self._render()

    def _clear(self):
        for y in range(self._rows):
            for x in range(self._cols):
                self._map[y][x] = 0
                self._pal_idx[y][x] = 0
        self._render()
        self._refresh_export()

    def _render(self):
        canvas = QImage(self._cols * self._cell_px,
                        self._rows * self._cell_px,
                        QImage.Format.Format_RGB32)
        canvas.fill(QColor(12, 12, 16))
        painter = QPainter(canvas)
        pen = QPen(QColor(40, 40, 50), 1)
        painter.setPen(pen)
        # grid + tile thumbnails
        for y in range(self._rows):
            for x in range(self._cols):
                rect_x = x * self._cell_px
                rect_y = y * self._cell_px
                t = self._map[y][x]
                if t > 0 and self.c1 and self.c2:
                    pal_key = self._pal_idx[y][x]
                    pal_rgb = self.palettes.get(pal_key, [(0, 0, 0)] * 16)
                    if len(pal_rgb) < 16:
                        pal_rgb = list(pal_rgb) + [(0, 0, 0)] * (16 - len(pal_rgb))
                    tile = decode_tile(self.c1, self.c2, t)
                    pix = tile_to_pixmap(tile, pal_rgb, zoom=1)
                    if pix and not pix.isNull():
                        scaled = pix.toImage().scaled(
                            self._cell_px, self._cell_px,
                            Qt.AspectRatioMode.IgnoreAspectRatio,
                            Qt.TransformationMode.FastTransformation)
                        painter.drawImage(rect_x, rect_y, scaled)
                painter.drawRect(rect_x, rect_y,
                                 self._cell_px - 1, self._cell_px - 1)
        painter.end()
        from PyQt6.QtGui import QPixmap
        self.canvas.setPixmap(QPixmap.fromImage(canvas))

    def _refresh_export(self):
        lines = [
            "/* Generated by Artbox Studio — Level Designer */",
            f"#define LEVEL_W {self._cols}",
            f"#define LEVEL_H {self._rows}",
            "",
            f"static const uint16_t level_tile[LEVEL_H][LEVEL_W] = {{"
        ]
        for y in range(self._rows):
            row = ", ".join(f"{t:3d}u" for t in self._map[y])
            lines.append(f"    {{ {row} }}{',' if y < self._rows-1 else ''}")
        lines.append("};")
        self.exp.setPlainText("\n".join(lines))


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
        # Load C-ROMs (use 777-c1.c1 / 777-c2.c2 in artbox directory)
        for fname, attr in (("777-c1.c1", "c1"), ("777-c2.c2", "c2")):
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

        self.tab_addr = RomAddressGridTab(self.manifest)
        tabs.addTab(self.tab_addr, "ROM Addresses")

        self.tab_palman = PaletteManagerTab(self.palettes, self.manifest)
        tabs.addTab(self.tab_palman, "Palette Manager")

        self.tab_paled = ManualPaletteEditorTab(self.palettes)
        tabs.addTab(self.tab_paled, "Palette Editor")

        self.tab_browser = AssetBrowserTab()
        tabs.addTab(self.tab_browser, "Asset Browser")

        self.tab_hex = HexSpriteInspectorTab(self.c1, self.c2, self.palettes)
        tabs.addTab(self.tab_hex, "Hex Sprite Inspector")

        self.tab_move = MovementDesignerTab(
            self.c1, self.c2, self.palettes, self.manifest)
        tabs.addTab(self.tab_move, "Movement Designer")

        self.tab_level = LevelDesignerTab(self.c1, self.c2, self.palettes)
        tabs.addTab(self.tab_level, "Level Designer")

        self.tab_pipeline = PipelineRunnerTab()
        tabs.addTab(self.tab_pipeline, "Pipeline")

        self.tab_hd = HdCompareTab()
        tabs.addTab(self.tab_hd, "HD Compare")

        self.tab_inv = RomInventoryTab()
        tabs.addTab(self.tab_inv, "ROM Inventory")

        self.tab_rules = AssetRulesTab()
        tabs.addTab(self.tab_rules, "Asset Rules")

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
        for fname, attr in (("777-c1.c1", "c1"), ("777-c2.c2", "c2")):
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
