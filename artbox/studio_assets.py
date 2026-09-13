"""Read-only ROM reconstruction and asset authoring views for Artbox Studio."""

import io
import json
from pathlib import Path
import struct

import numpy as np
from PIL import Image
from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtGui import QColor, QImage, QPainter, QPixmap
from PyQt6.QtWidgets import (
    QAbstractItemView, QCheckBox, QFileDialog, QHBoxLayout, QLabel, QLineEdit,
    QListWidget, QListWidgetItem, QPlainTextEdit, QPushButton, QScrollArea,
    QSpinBox, QSplitter, QVBoxLayout, QWidget,
)

from tile_codec import decode_image
from studio_widgets import save_document


def ng_rgb(word):
    # Bit 15 is an inverted shared low bit, not a 50 percent dimmer.
    low = 1 - ((word >> 15) & 1)
    channels = [(((word >> shift) & 15) << 2) | (((word >> bit) & 1) << 1) | low
                for shift, bit in ((8, 14), (4, 13), (0, 12))]
    return tuple((c << 2) | (c >> 4) for c in channels)


def rgb_word(red, green, blue):
    rgb = (red, green, blue)
    best = None
    for low in (0, 1):
        values = [min(62 + low, max(low, round((c * 63 / 255 - low) / 2) * 2 + low)) for c in rgb]
        error = sum(w * (((v << 2) | (v >> 4)) - c) ** 2
                    for w, v, c in zip((0.299, 0.587, 0.114), values, rgb))
        if best is None or error < best[0]:
            best = (error, low, values)
    _, low, values = best
    r, g, b = [v >> 1 for v in values]
    return ((1 - low) << 15) | ((r & 1) << 14) | ((g & 1) << 13) | ((b & 1) << 12) | ((r >> 1) << 8) | ((g >> 1) << 4) | (b >> 1)


class PaletteBook(dict):
    def __init__(self, path=None, manifest=()):
        super().__init__()
        self.banks = {}
        self.manifest = list(manifest)
        self.tile_banks = {}
        if path and Path(path).exists():
            data = Path(path).read_bytes()
            if len(data) % 136:
                raise ValueError("Truncated neopal.bin: records must be 136 bytes")
            for offset in range(0, len(data), 136):
                # The packer uses i16Q: four bytes of alignment precede colors.
                values = struct.unpack_from("<I4x16Q", data, offset)
                self[values[0]] = [ng_rgb(word & 0xFFFF) for word in values[1:]]
        for asset in self.manifest:
            sid = asset.get("screen_id", asset["db_index"] + 1)
            bank = asset["palette_bank"]
            if sid in self:
                self.banks[bank] = self[sid]
            slots = asset.get("palette_slots", [bank])
            for slot, colors in zip(slots[1:], asset.get("extra_palettes", [])):
                self.banks[slot] = [ng_rgb(word) for word in colors]
            base = asset["tile_base"]
            count = asset.get("tile_reserved_count", 256)
            mapping = asset.get("tile_palette_banks")
            for i in range(count):
                self.tile_banks[base + i] = mapping[i] if mapping and i < len(mapping) else bank

    def for_tile(self, tile, fallback=0):
        return self.banks.get(self.tile_banks.get(tile), self.get(fallback, [(i * 17,) * 3 for i in range(16)]))


def frame_geometry(asset):
    stride = int(asset.get("canvas_width", 256)) // 16
    base = int(asset["tile_base"])
    col = int(asset.get("used_tile_col_start", 0))
    row = int(asset.get("used_tile_row_start", 0))
    strips = int(asset.get("sprite_strips", stride))
    rows = int(asset.get("sprite_active_rows", int(asset.get("canvas_height", 256)) // 16))
    return base + row * stride + col, strips, rows, stride


def reconstruct(asset, c1, c2, palettes):
    width, height = int(asset.get("canvas_width", 256)), int(asset.get("canvas_height", 256))
    if not (0 < width <= 4096 and 0 < height <= 4096 and width % 16 == height % 16 == 0):
        raise ValueError("Invalid 16-pixel canvas dimensions")
    start = int(asset["tile_base"]) * 64
    length = width * height // 4
    if start < 0 or start + length > min(len(c1), len(c2)):
        raise ValueError("Asset's tile range is outside the loaded C-ROM pair")
    if width * height // 256 > int(asset.get("tile_reserved_count", 256)):
        raise ValueError("Canvas exceeds its reserved tiles")
    indices = decode_image(c1[start:start + length], c2[start:start + length], width, height, swapped=True)
    rgba = np.zeros((height, width, 4), dtype=np.uint8)
    for y in range(0, height, 16):
        for x in range(0, width, 16):
            tile = asset["tile_base"] + (y // 16) * (width // 16) + x // 16
            bank = palettes.tile_banks.get(tile)
            if bank not in palettes.banks:
                raise ValueError(f"Missing palette bank {bank} for tile {tile}")
            block = indices[y:y + 16, x:x + 16]
            rgba[y:y + 16, x:x + 16, :3] = np.asarray(palettes.for_tile(tile), dtype=np.uint8)[block]
            rgba[y:y + 16, x:x + 16, 3] = np.where(block == 0, 0, 255)
    return Image.fromarray(rgba)


class ImageView(QWidget):
    def __init__(self):
        super().__init__()
        self.image = QImage()
        self.zoom = 1
        self.grid = False
        self.setMinimumSize(280, 240)

    def set_image(self, pil, zoom=1, grid=False):
        rgba = pil.convert("RGBA")
        self.image = QImage(rgba.tobytes(), rgba.width, rgba.height, rgba.width * 4, QImage.Format.Format_RGBA8888).copy()
        self.zoom, self.grid = zoom, grid
        self.setMinimumSize(max(280, rgba.width * zoom), max(240, rgba.height * zoom))
        self.update()

    def paintEvent(self, event):
        painter = QPainter(self)
        for y in range(event.rect().top() // 12 * 12, event.rect().bottom() + 1, 12):
            for x in range(event.rect().left() // 12 * 12, event.rect().right() + 1, 12):
                painter.fillRect(x, y, 12, 12, QColor("#35383b" if (x // 12 + y // 12) % 2 else "#45484b"))
        if self.image.isNull():
            return
        w, h = self.image.width() * self.zoom, self.image.height() * self.zoom
        x, y = (self.width() - w) // 2, (self.height() - h) // 2
        painter.drawImage(x, y, self.image.scaled(w, h, Qt.AspectRatioMode.IgnoreAspectRatio, Qt.TransformationMode.FastTransformation))
        if self.grid:
            painter.setPen(QColor(80, 210, 170, 130))
            for col in range(0, w + 1, 16 * self.zoom):
                painter.drawLine(x + col, y, x + col, y + h)
            for row in range(0, h + 1, 16 * self.zoom):
                painter.drawLine(x, y + row, x + w, y + row)


class AssetWorkbench(QWidget):
    """Source comparison and complete-frame animation from generated metadata."""

    def __init__(self, project, c1, c2, palettes, manifest):
        super().__init__()
        self.project, self.c1, self.c2 = project, c1, c2
        self.palettes, self.manifest = palettes, manifest
        self.compiled = None
        self.cache = {}
        self.frames = []
        self.frame_index = 0
        self.timer = QTimer(self)
        self.timer.timeout.connect(self._advance)
        layout = QVBoxLayout(self)
        toolbar = QHBoxLayout()
        self.search = QLineEdit()
        self.search.setPlaceholderText("Filter name or category")
        self.search.setClearButtonEnabled(True)
        self.search.textChanged.connect(self._filter)
        toolbar.addWidget(self.search, 1)
        self.zoom = QSpinBox()
        self.zoom.setRange(1, 8)
        self.zoom.setSuffix("x")
        self.zoom.valueChanged.connect(self._selection)
        toolbar.addWidget(self.zoom)
        self.grid = QCheckBox("Tile grid")
        self.grid.toggled.connect(self._selection)
        toolbar.addWidget(self.grid)
        self.crop = QCheckBox("Content bounds")
        self.crop.toggled.connect(self._selection)
        toolbar.addWidget(self.crop)
        layout.addLayout(toolbar)
        splitter = QSplitter()
        self.assets = QListWidget()
        self.assets.setSelectionMode(QAbstractItemView.SelectionMode.ExtendedSelection)
        for a in manifest:
            item = QListWidgetItem(f"{a.get('screen_id', 0):03d}  {a.get('subdir', '')}/{a['name']}")
            item.setData(Qt.ItemDataRole.UserRole, a)
            self.assets.addItem(item)
        self.assets.currentItemChanged.connect(self._selection)
        splitter.addWidget(self.assets)
        for title, attr in (("Source PNG", "source_view"), ("Encoded C-ROM / palette RAM", "rom_view")):
            pane = QWidget()
            box = QVBoxLayout(pane)
            box.addWidget(QLabel(title))
            view = ImageView()
            scroll = QScrollArea()
            scroll.setWidgetResizable(True)
            scroll.setWidget(view)
            box.addWidget(scroll)
            setattr(self, attr, view)
            splitter.addWidget(pane)
        splitter.setSizes([230, 440, 440])
        layout.addWidget(splitter, 1)
        controls = QHBoxLayout()
        self.play = QPushButton("Play selected frames")
        self.play.clicked.connect(self._play)
        controls.addWidget(self.play)
        self.fps = QSpinBox()
        self.fps.setRange(1, 30)
        self.fps.setValue(8)
        self.fps.setSuffix(" fps")
        self.fps.valueChanged.connect(lambda fps: self.timer.setInterval(round(1000 / fps)))
        controls.addWidget(self.fps)
        for label, callback in (("Export rendered PNG", self._export_png), ("Export frame metadata", self._export_frames)):
            button = QPushButton(label)
            button.clicked.connect(callback)
            controls.addWidget(button)
        controls.addStretch()
        layout.addLayout(controls)
        self.details = QPlainTextEdit()
        self.details.setReadOnly(True)
        self.details.setMaximumHeight(140)
        layout.addWidget(self.details)
        if manifest:
            self.assets.setCurrentRow(0)
        else:
            self.details.setPlainText(f"No generated asset manifest in {project.art_data}")

    def reload_assets(self, project, c1, c2, palettes, manifest):
        self.project, self.c1, self.c2 = project, c1, c2
        self.palettes, self.manifest = palettes, manifest
        self.compiled = None
        self.cache.clear()
        if self.timer.isActive():
            self.timer.stop()
            self.play.setText("Play selected frames")
        self.assets.blockSignals(True)
        self.assets.clear()
        for a in manifest:
            item = QListWidgetItem(f"{a.get('screen_id', 0):03d}  {a.get('subdir', '')}/{a['name']}")
            item.setData(Qt.ItemDataRole.UserRole, a)
            self.assets.addItem(item)
        self.assets.blockSignals(False)
        if manifest:
            self.assets.setCurrentRow(0)
            self._selection()
        else:
            self.source_view.image = QImage()
            self.source_view.update()
            self.rom_view.image = QImage()
            self.rom_view.update()
            self.details.setPlainText(f"No generated asset manifest in {project.art_data}")

    def _filter(self, query):
        for i in range(self.assets.count()):
            item = self.assets.item(i)
            item.setHidden(query.casefold() not in item.text().casefold())

    def _selection(self, *_):
        item = self.assets.currentItem()
        if item:
            self._show(item.data(Qt.ItemDataRole.UserRole))

    def _show(self, asset):
        try:
            key = asset["db_index"]
            if key not in self.cache:
                if len(self.cache) >= 32:
                    self.cache.pop(next(iter(self.cache)))
                self.cache[key] = reconstruct(asset, self.c1, self.c2, self.palettes)
            self.compiled = self.cache[key]
            # Recorded absolute Windows/WSL paths are not portable; use manifest names.
            source = self.project.art_source / "in" / asset.get("subdir", "") / asset["name"]
            if not source.exists():
                source = self.project.art_data / "in" / asset.get("subdir", "") / asset["name"]
            if source.exists():
                with Image.open(source) as image:
                    original = image.convert("RGBA")
            else:
                original = Image.new("RGBA", (self.compiled.width, self.compiled.height), (60, 60, 70, 255))
            image = self.compiled
            if self.crop.isChecked():
                x, y = asset.get("content_left", 0), asset.get("content_top", 0)
                image = image.crop((x, y, x + asset.get("content_width", image.width), y + asset.get("content_height", image.height)))
            self.source_view.set_image(original, self.zoom.value())
            self.rom_view.set_image(image, self.zoom.value(), self.grid.isChecked())
            tile, cols, rows, stride = frame_geometry(asset)
            alpha = np.asarray(self.compiled)[:, :, 3]
            used = int(np.count_nonzero(alpha))
            src_note = f"Source {original.width} x {original.height}" if source.exists() else "Source PNG not found (placeholder shown)"
            self.details.setPlainText(
                f"{asset.get('subdir', '')}/{asset['name']}  |  screen_id={asset.get('screen_id')}  |  {asset['mode']} / {asset['fit']}\n"
                f"{src_note}  |  Canvas {self.compiled.width} x {self.compiled.height}  |  {used} opaque pixels\n"
                f"Frame first tile {tile}  |  {cols} strips x {rows} rows  |  stride {stride}  |  reserved through {asset['tile_reserved_last']}\n"
                f"Palette slots {asset.get('palette_slots', [asset['palette_bank']])}  |  index 0 transparent  |  source and encoded dimensions shown independently"
            )
        except (OSError, ValueError, KeyError, IndexError) as exc:
            self.compiled = None
            self.rom_view.image = QImage()
            self.rom_view.update()
            self.details.setPlainText(str(exc))

    def _play(self):
        if self.timer.isActive():
            self.timer.stop()
            self.play.setText("Play selected frames")
            return
        self.frames = [item.data(Qt.ItemDataRole.UserRole) for item in self.assets.selectedItems() if not item.isHidden()]
        if self.frames:
            self.frame_index = 0
            self.play.setText("Stop animation")
            self.timer.start(round(1000 / self.fps.value()))
            self._show(self.frames[0])

    def _advance(self):
        self.frame_index = (self.frame_index + 1) % len(self.frames)
        self._show(self.frames[self.frame_index])

    def _export_png(self):
        if self.compiled is None:
            return
        filename, _ = QFileDialog.getSaveFileName(self, "Export decoded canvas", "rendered.png", "PNG (*.png)")
        if filename:
            data = io.BytesIO()
            self.compiled.save(data, format="PNG")
            save_document(self, filename, data.getvalue())

    def _export_frames(self):
        frames = [item.data(Qt.ItemDataRole.UserRole) for item in self.assets.selectedItems() if not item.isHidden()]
        filename, _ = QFileDialog.getSaveFileName(self, "Export selected frame metadata", "animation.json", "JSON (*.json)")
        if filename:
            data = {"format": "artbox-studio-frames", "version": 1, "game": self.project.game,
                    "fps": self.fps.value(), "frames": frames}
            save_document(self, filename, json.dumps(data, indent=2) + "\n")
