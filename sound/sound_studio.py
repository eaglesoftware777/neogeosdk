#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
NeoGeo YM2610 Sound Studio
PyQt6 UI for FM patch editing, MML/SSG composition, ADPCM management,
and simulated YM2610 audio preview.

Usage: python3 sound/sound_studio.py   (from repo root)
"""

import os, sys, re, math, struct, wave
from pathlib import Path
import numpy as np

from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QTabWidget,
    QVBoxLayout, QHBoxLayout, QGridLayout, QFormLayout,
    QLabel, QComboBox, QSpinBox, QSlider, QPushButton,
    QTextEdit, QPlainTextEdit, QSplitter, QScrollArea,
    QListWidget, QListWidgetItem, QGroupBox, QCheckBox,
    QFileDialog, QMessageBox, QSizePolicy, QFrame,
    QAbstractItemView, QToolButton, QDoubleSpinBox,
)
from PyQt6.QtCore import Qt, QBuffer, QByteArray, QIODevice, QTimer, pyqtSignal
from PyQt6.QtGui import (
    QPainter, QColor, QPen, QFont, QFontMetrics, QPixmap,
    QSyntaxHighlighter, QTextCharFormat, QBrush,
)
from PyQt6.QtMultimedia import QAudioSink, QAudioFormat

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
REPO_ROOT = Path(__file__).resolve().parent.parent
SOUND_DIR  = REPO_ROOT / "sound"
FM_DIR     = SOUND_DIR / "fm"
SSG_DIR    = SOUND_DIR / "ssg"
MML_DIR    = SOUND_DIR / "fm"   # FM MML lives in sound/fm
TOOLS_DIR  = SOUND_DIR / "tools"
SAMPLES_DIR = SOUND_DIR / "samples" / "in_wav_a"
PATCHES_FILE = FM_DIR / "patches.fm"
SSG_CONFIG   = SSG_DIR / "config.ssg"

SAMPLE_RATE = 44100
ZOOM_DEFAULT = 2

# ---------------------------------------------------------------------------
# YM2610 note frequency table
# ---------------------------------------------------------------------------
def ym2610_note_freq(midi_note: int) -> float:
    """MIDI note 0-127 → frequency in Hz (A4=69=440Hz)."""
    return 440.0 * (2 ** ((midi_note - 69) / 12))

NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]

def midi_to_name(midi: int) -> str:
    return f"{NOTE_NAMES[midi % 12]}{midi // 12 - 1}"

# ---------------------------------------------------------------------------
# FM synthesis (YM2610 approximation)
# 4-operator FM with algorithm selection
# ---------------------------------------------------------------------------
ALG_DESCRIPTIONS = [
    "ALG0: Op1→2→3→4→OUT",
    "ALG1: (Op1+Op2)→3→4→OUT",
    "ALG2: (Op1,Op2→3)→4→OUT",
    "ALG3: Op1→2, (Op2+Op3)→4→OUT",
    "ALG4: Op1→2→OUT, Op3→4→OUT",
    "ALG5: Op1→(2,3,4)→OUT",
    "ALG6: Op1→2→OUT, Op3→OUT, Op4→OUT",
    "ALG7: Op1+Op2+Op3+Op4→OUT",
]

def _envelope(t, ar, dr, sl_level, sr, rr, total_dur, sr_rate=44100):
    """Very simplified ADSR-like envelope from YM2610 params."""
    env = np.ones_like(t)
    ar_time = max(0.001, (31 - ar) / 31 * 0.3)
    dr_time = max(0.001, (31 - dr) / 31 * 0.4)
    sl = sl_level / 15.0
    sr_time = max(0.001, (31 - sr) / 31 * 0.8)
    rr_time = max(0.001, rr / 15.0 * 0.5)

    for i, ti in enumerate(t):
        if ti < ar_time:
            env[i] = ti / ar_time
        elif ti < ar_time + dr_time:
            frac = (ti - ar_time) / dr_time
            env[i] = 1.0 - frac * (1.0 - sl)
        elif ti < total_dur - rr_time:
            decay = (ti - ar_time - dr_time) / max(0.001, total_dur - rr_time - ar_time - dr_time)
            env[i] = sl * (1.0 - decay * sr_time * 0.3)
        else:
            frac = (ti - (total_dur - rr_time)) / rr_time
            env[i] = max(0, sl * (1.0 - frac))
    return np.clip(env, 0, 1)

def synthesize_fm(patch: dict, midi_note: int, duration: float = 0.5) -> np.ndarray:
    """
    Synthesize ~YM2610 FM sound for a given patch and MIDI note.
    Returns float32 mono samples at SAMPLE_RATE.
    """
    freq = ym2610_note_freq(midi_note)
    t = np.linspace(0, duration, int(SAMPLE_RATE * duration), endpoint=False)

    alg = patch.get("alg", 7)
    fb  = patch.get("fb", 0)
    ops = patch.get("ops", {})

    def get_op(name):
        raw = ops.get(name, [0x01, 0x20, 0x1F, 0x08, 0x05, 0xAF, 0x00])
        if len(raw) < 7: raw += [0] * (7 - len(raw))
        dt_mul = raw[0]; tl = raw[1]; ar = raw[2]; dr = raw[3]
        sr_ = raw[4]; sl_rr = raw[5]; _env = raw[6]
        mul = dt_mul & 0x0F
        tl_lin = (127 - tl) / 127.0
        ar_v = (ar & 0x1F)
        dr_v = (dr & 0x1F)
        sr_v = sr_ & 0x1F
        sl = (sl_rr >> 4) & 0x0F
        rr = sl_rr & 0x0F
        mul_freq = max(0.5, mul) * freq
        env = _envelope(t, ar_v, dr_v, sl, sr_v, rr, duration)
        return tl_lin, mul_freq, env

    tl1, f1, e1 = get_op("op1"); tl2, f2, e2 = get_op("op2")
    tl3, f3, e3 = get_op("op3"); tl4, f4, e4 = get_op("op4")

    def wave(f, e, tl, mod=0.0):
        return tl * e * np.sin(2 * np.pi * f * t + mod)

    # Feedback on op1
    fb_scale = fb * 0.3
    op1_base = np.sin(2 * np.pi * f1 * t)
    op1 = wave(f1, e1, tl1, fb_scale * op1_base)

    if alg == 0:   # 1→2→3→4
        op2 = wave(f2, e2, tl2, op1 * np.pi)
        op3 = wave(f3, e3, tl3, op2 * np.pi)
        op4 = wave(f4, e4, tl4, op3 * np.pi)
        out = op4
    elif alg == 1: # (1+2)→3→4
        op2 = wave(f2, e2, tl2)
        op3 = wave(f3, e3, tl3, (op1 + op2) * np.pi)
        op4 = wave(f4, e4, tl4, op3 * np.pi)
        out = op4
    elif alg == 2: # (1,2→3)→4
        op2 = wave(f2, e2, tl2, op1 * np.pi)
        op3 = wave(f3, e3, tl3, op2 * np.pi)
        op4 = wave(f4, e4, tl4, op3 * np.pi)
        out = op4
    elif alg == 3: # 1→2, (2+3)→4
        op2 = wave(f2, e2, tl2, op1 * np.pi)
        op3 = wave(f3, e3, tl3)
        op4 = wave(f4, e4, tl4, (op2 + op3) * np.pi)
        out = op4
    elif alg == 4: # 1→2→out, 3→4→out
        op2 = wave(f2, e2, tl2, op1 * np.pi)
        op3 = wave(f3, e3, tl3)
        op4 = wave(f4, e4, tl4, op3 * np.pi)
        out = op2 + op4
    elif alg == 5: # 1→(2,3,4)
        op2 = wave(f2, e2, tl2, op1 * np.pi)
        op3 = wave(f3, e3, tl3, op1 * np.pi)
        op4 = wave(f4, e4, tl4, op1 * np.pi)
        out = op2 + op3 + op4
    elif alg == 6: # 1→2→out, 3→out, 4→out
        op2 = wave(f2, e2, tl2, op1 * np.pi)
        op3 = wave(f3, e3, tl3)
        op4 = wave(f4, e4, tl4)
        out = op2 + op3 + op4
    else:          # alg 7: additive
        op2 = wave(f2, e2, tl2)
        op3 = wave(f3, e3, tl3)
        op4 = wave(f4, e4, tl4)
        out = op1 + op2 + op3 + op4

    peak = np.max(np.abs(out))
    if peak > 0:
        out /= peak
    return out.astype(np.float32) * 0.7

def synthesize_ssg(preset: dict, midi_note: int, duration: float = 0.5) -> np.ndarray:
    """Synthesize SSG (square wave) note from preset dict."""
    freq = ym2610_note_freq(midi_note)
    t = np.linspace(0, duration, int(SAMPLE_RATE * duration), endpoint=False)
    vol_a = preset.get("vol_a", 10) / 15.0
    sq = np.sign(np.sin(2 * np.pi * freq * t)) * vol_a
    fade = np.linspace(1, 0, len(t)) ** 0.5
    return (sq * fade).astype(np.float32) * 0.5

_DEFAULT_PATCH = {"alg": 7, "fb": 2, "stereo": 0xC0, "lfo": 0,
                  "ops": {k: [0x01, 0x20, 0x1F, 0x08, 0x05, 0xAF, 0x00]
                          for k in ("op1", "op2", "op3", "op4")}}
_DEFAULT_SSG   = {"vol_a": 12, "vol_b": 0, "vol_c": 0, "tone_mask": 0x38}

def synthesize_track(events: list, patches: list, ssg_presets: list = None,
                     bpm: int = 120, mode: str = "fm") -> np.ndarray:
    """Render a full list of MML events to a float32 audio buffer."""
    if not events:
        return np.zeros(SAMPLE_RATE // 2, dtype=np.float32)
    TICKS_PER_BEAT = 48
    secs_per_tick = 60.0 / (bpm * TICKS_PER_BEAT)
    total_ticks = max(e[0] + e[2] for e in events)
    total_secs = total_ticks * secs_per_tick + 1.0
    buf = np.zeros(int(total_secs * SAMPLE_RATE), dtype=np.float32)
    for tick, midi, dur, vol, inst in events:
        t_start = int(tick * secs_per_tick * SAMPLE_RATE)
        note_dur = max(0.05, dur * secs_per_tick)
        if mode == "fm":
            patch = patches[inst % max(1, len(patches))] if patches else _DEFAULT_PATCH
            note = synthesize_fm(patch, midi, note_dur)
        else:
            preset = ssg_presets[inst % max(1, len(ssg_presets))] if ssg_presets else _DEFAULT_SSG
            note = synthesize_ssg(preset, midi, note_dur)
        note = note * (vol / 15.0)
        end = min(len(buf), t_start + len(note))
        buf[t_start:end] += note[:end - t_start]
    peak = np.max(np.abs(buf))
    if peak > 0:
        buf /= peak
    return buf.astype(np.float32) * 0.85

def synthesize_step_pattern(channels: list, grid: list, bpm: int,
                             patches: list, ssg_presets: list,
                             loops: int = 1) -> np.ndarray:
    """Render a step-sequencer grid to audio.
    channels: [{'type':'FM'/'SSG', 'note':midi, 'inst':n, 'vol':0-15}, ...]
    grid:     grid[ch][step] = bool
    """
    num_steps = len(grid[0]) if grid and grid[0] else 16
    step_dur = 60.0 / bpm / 2      # eighth-note steps
    note_dur = step_dur * 0.82
    total_samples = int(num_steps * step_dur * loops * SAMPLE_RATE) + SAMPLE_RATE
    buf = np.zeros(total_samples, dtype=np.float32)
    for loop in range(loops):
        loop_off = int(loop * num_steps * step_dur * SAMPLE_RATE)
        for step in range(num_steps):
            t_start = loop_off + int(step * step_dur * SAMPLE_RATE)
            for ch_idx, ch in enumerate(channels):
                if ch_idx >= len(grid) or step >= len(grid[ch_idx]):
                    continue
                if not grid[ch_idx][step]:
                    continue
                midi = ch["note"]; vol = ch["vol"] / 15.0
                inst = ch["inst"]
                if ch["type"] == "FM":
                    patch = patches[inst % max(1, len(patches))] if patches else _DEFAULT_PATCH
                    note = synthesize_fm(patch, midi, note_dur) * vol * 0.35
                else:
                    preset = ssg_presets[inst % max(1, len(ssg_presets))] if ssg_presets else _DEFAULT_SSG
                    note = synthesize_ssg(preset, midi, note_dur) * vol * 0.35
                end = min(total_samples, t_start + len(note))
                buf[t_start:end] += note[:end - t_start]
    peak = np.max(np.abs(buf))
    if peak > 0:
        buf /= peak
    return buf.astype(np.float32) * 0.85

# ---------------------------------------------------------------------------
# Audio playback via PyQt6
# ---------------------------------------------------------------------------
def play_samples(samples: np.ndarray):
    """Play float32 mono samples through QAudioSink."""
    fmt = QAudioFormat()
    fmt.setSampleRate(SAMPLE_RATE)
    fmt.setChannelCount(1)
    fmt.setSampleFormat(QAudioFormat.SampleFormat.Float)

    sink = QAudioSink(fmt)
    pcm_bytes = samples.tobytes()
    buf = QBuffer()
    buf.setData(QByteArray(pcm_bytes))
    buf.open(QIODevice.OpenModeFlag.ReadOnly)
    sink.start(buf)
    # Keep references alive until playback finishes
    sink._buf = buf
    return sink

# ---------------------------------------------------------------------------
# FM Patch parser/writer
# ---------------------------------------------------------------------------
def parse_patches(path: Path) -> list:
    patches = []
    current = None
    for line in path.read_text().splitlines():
        line = line.split(";", 1)[0].strip()
        if not line:
            continue
        m = re.match(r"\[patch\s+(\d+)\]", line, re.I)
        if m:
            if current is not None:
                patches.append(current)
            current = {"id": int(m.group(1)), "name": f"Patch {m.group(1)}",
                       "alg": 7, "fb": 0, "stereo": 0xC0, "lfo": 0x00, "ops": {}}
            continue
        if current is None:
            continue
        if "=" not in line:
            continue
        key, val = [x.strip() for x in line.split("=", 1)]
        key = key.lower()
        if key == "name":
            current["name"] = val
        elif key == "alg":
            current["alg"] = int(val, 0) & 7
        elif key == "fb":
            current["fb"] = int(val, 0) & 7
        elif key == "stereo":
            current["stereo"] = int(val, 16)
        elif key == "lfo":
            current["lfo"] = int(val, 16)
        elif key in ("op1", "op2", "op3", "op4"):
            current["ops"][key] = [int(x.strip(), 16) for x in val.split(",")]
    if current is not None:
        patches.append(current)
    return patches

def write_patches(patches: list, path: Path):
    lines = [
        "; ==========================================================",
        "; NeoGeo YM2610 FM Patch Bank",
        "; Patch format: op = DT/MUL, TL, AR, DR, SR, SL/RR, ENV",
        "; ==========================================================",
        "",
    ]
    for p in patches:
        lines.append(f"[patch {p['id']}]")
        lines.append(f"name={p['name']}")
        lines.append(f"alg={p['alg']}")
        lines.append(f"fb={p['fb']}")
        lines.append(f"stereo={p['stereo']:02X}")
        lines.append(f"lfo={p['lfo']:02X}")
        for op in ("op1", "op2", "op3", "op4"):
            vals = p["ops"].get(op, [0x01, 0x20, 0x1F, 0x08, 0x05, 0xAF, 0x00])
            lines.append(f"{op}={','.join(f'{v:02X}' for v in vals)}")
        lines.append("")
    path.write_text("\n".join(lines))

# ---------------------------------------------------------------------------
# SSG config parser/writer
# ---------------------------------------------------------------------------
def parse_ssg_presets(path: Path) -> list:
    presets = []
    current = None
    for line in path.read_text().splitlines():
        line = line.split(";", 1)[0].strip()
        if not line:
            continue
        m = re.match(r"\[preset\s+(\d+)\]", line, re.I)
        if m:
            if current is not None:
                presets.append(current)
            current = {"id": int(m.group(1)), "name": f"Preset {m.group(1)}",
                       "tone_mask": 0x38, "vol_a": 10, "vol_b": 6, "vol_c": 0,
                       "noise_freq": 0}
            continue
        if current is None:
            continue
        if "=" not in line:
            continue
        key, val = [x.strip() for x in line.split("=", 1)]
        key = key.lower()
        if key == "name":
            current["name"] = val
        elif key == "tone_mask":
            current["tone_mask"] = int(val, 0)
        elif key in ("vol_a", "vol_b", "vol_c"):
            current[key] = int(val, 16)
        elif key == "noise_freq":
            current["noise_freq"] = int(val, 16)
    if current is not None:
        presets.append(current)
    return presets

def write_ssg_presets(presets: list, path: Path):
    lines = ["; NeoGeo YM2610 SSG preset/config bank", ""]
    for p in presets:
        lines.append(f"[preset {p['id']}]")
        lines.append(f"name={p['name']}")
        lines.append(f"tone_mask=0x{p['tone_mask']:02X}")
        lines.append(f"vol_a={p['vol_a']:02X}")
        lines.append(f"vol_b={p['vol_b']:02X}")
        lines.append(f"vol_c={p['vol_c']:02X}")
        lines.append(f"noise_freq={p['noise_freq']:02X}")
        lines.append("")
    path.write_text("\n".join(lines))

# ---------------------------------------------------------------------------
# MML parser (for piano roll preview) — mirrors fm_compile.py logic
# ---------------------------------------------------------------------------
def parse_mml_events(text: str) -> list:
    """Return list of (tick, note_midi, duration_ticks, vol, instrument)."""
    def read_number(s, i, default):
        start = i
        while i < len(s) and s[i].isdigit():
            i += 1
        return (default, i) if start == i else (int(s[start:i]), i)

    def dur_ticks(length):
        return max(1, min(255, 48 // max(1, length)))

    NOTE_BASE = {"c":0,"d":2,"e":4,"f":5,"g":7,"a":9,"b":11}
    tempo = 120; octave = 4; length = 4; volume = 12; instrument = 0
    events = []; tick = 0
    s = ''.join(line.split(';',1)[0].lower() for line in text.splitlines())
    i = 0
    while i < len(s):
        c = s[i]
        if c in ' \t\n\r,':
            i += 1; continue
        if c == 't':
            i += 1; tempo, i = read_number(s, i, tempo); continue
        if c == 'v':
            i += 1; volume, i = read_number(s, i, volume); continue
        if c == 'i':
            i += 1; instrument, i = read_number(s, i, instrument); continue
        if c == 'l':
            i += 1; length, i = read_number(s, i, length); continue
        if c == 'o':
            i += 1; octave, i = read_number(s, i, octave); continue
        if c == '>':
            octave = min(8, octave + 1); i += 1; continue
        if c == '<':
            octave = max(0, octave - 1); i += 1; continue
        if c in NOTE_BASE:
            semitone = NOTE_BASE[c]; i += 1
            if i < len(s) and s[i] == '+':
                semitone += 1; i += 1
            elif i < len(s) and s[i] == '-':
                semitone -= 1; i += 1
            nl, i = read_number(s, i, length)
            dotted = i < len(s) and s[i] == '.'
            if dotted: i += 1
            dt = dur_ticks(nl)
            if dotted: dt = dt + dt // 2
            midi = octave * 12 + semitone + 12
            events.append((tick, midi, dt, volume, instrument))
            tick += dt
            continue
        if c == 'r':
            i += 1
            rl, i = read_number(s, i, length)
            tick += dur_ticks(rl)
            continue
        if c.isdigit():
            _, i = read_number(s, i, 0); continue
        i += 1
    return events

# ---------------------------------------------------------------------------
# Piano roll widget
# ---------------------------------------------------------------------------
class PianoRollWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.events = []          # list of (tick, midi, dur, vol, inst)
        self.total_ticks = 96
        self.px_per_tick = 4
        self.row_h = 6
        self.midi_min = 36
        self.midi_max = 84
        self.setMinimumHeight(300)
        self.setMinimumWidth(400)

    def set_events(self, events):
        self.events = events
        self.total_ticks = max(96, (max((e[0]+e[2] for e in events), default=96)))
        w = self.total_ticks * self.px_per_tick + 40
        h = (self.midi_max - self.midi_min + 1) * self.row_h + 20
        self.setMinimumWidth(w)
        self.setMinimumHeight(h)
        self.update()

    def paintEvent(self, event):
        p = QPainter(self)
        p.fillRect(self.rect(), QColor(20, 20, 30))
        rows = self.midi_max - self.midi_min + 1
        h = self.row_h
        x0 = 30

        for i in range(rows):
            midi = self.midi_max - i
            y = 10 + i * h
            # Shade black keys
            semi = midi % 12
            is_black = semi in (1, 3, 6, 8, 10)
            row_color = QColor(30, 30, 45) if is_black else QColor(25, 25, 38)
            p.fillRect(x0, y, 2000, h, row_color)
            if midi % 12 == 0:
                p.setPen(QColor(80, 80, 80))
                p.drawLine(x0, y, self.width(), y)
                p.setPen(QColor(120, 120, 120))
                p.setFont(QFont("Monospace", 5))
                p.drawText(2, y + h, f"C{midi//12-1}")

        COLORS = [QColor(80, 180, 255), QColor(255, 160, 60),
                  QColor(120, 220, 120), QColor(220, 100, 100)]
        for tick, midi, dur, vol, inst in self.events:
            if not (self.midi_min <= midi <= self.midi_max):
                continue
            row = self.midi_max - midi
            y = 10 + row * h + 1
            x = x0 + tick * self.px_per_tick
            w = max(2, dur * self.px_per_tick - 1)
            alpha = min(255, max(80, vol * 18))
            c = COLORS[inst % len(COLORS)]
            c.setAlpha(alpha)
            p.fillRect(x, y, w, h - 1, c)

        # Playhead-style bar guide every 48 ticks
        p.setPen(QPen(QColor(60, 60, 80), 1))
        for tick in range(0, self.total_ticks + 48, 48):
            x = x0 + tick * self.px_per_tick
            p.drawLine(x, 0, x, self.height())

# ---------------------------------------------------------------------------
# Piano keyboard widget for note preview
# ---------------------------------------------------------------------------
class PianoKeyboard(QWidget):
    note_pressed = pyqtSignal(int)   # emits MIDI note

    def __init__(self, parent=None):
        super().__init__(parent)
        self.octave = 4
        self.white_w = 22
        self.white_h = 80
        self.black_w = 14
        self.black_h = 50
        self.setFixedHeight(self.white_h + 4)
        self.setMinimumWidth(220)

    def _white_notes(self):
        return [n for n in range(self.octave*12, self.octave*12+13)
                if n % 12 not in (1,3,6,8,10)]

    def _black_notes(self):
        return [n for n in range(self.octave*12, self.octave*12+13)
                if n % 12 in (1,3,6,8,10)]

    def _white_x(self, note):
        whites = self._white_notes()
        try:
            return whites.index(note) * self.white_w + 2
        except ValueError:
            return -1

    def paintEvent(self, event):
        p = QPainter(self)
        p.fillRect(self.rect(), QColor(40, 40, 40))
        for note in self._white_notes():
            x = self._white_x(note)
            p.fillRect(x, 2, self.white_w - 2, self.white_h, QColor(230, 230, 230))
            p.setPen(QColor(0, 0, 0))
            p.drawRect(x, 2, self.white_w - 2, self.white_h)
            p.setFont(QFont("Monospace", 6))
            p.drawText(x + 2, self.white_h - 2, NOTE_NAMES[note % 12])
        # Black keys
        white_offsets = {0:0,2:1,4:2,5:3,7:4,9:5,11:6}
        for note in self._black_notes():
            semi = note % 12
            prev_white = {1:0,3:2,6:5,8:7,10:9}[semi]
            wi = [n for n in self._white_notes()].index(note - (semi - prev_white))
            x = 2 + wi * self.white_w + self.white_w - self.black_w // 2
            p.fillRect(x, 2, self.black_w, self.black_h, QColor(20, 20, 20))
            p.setPen(QColor(0, 0, 0))
            p.drawRect(x, 2, self.black_w, self.black_h)

    def mousePressEvent(self, event):
        x = event.position().x()
        y = event.position().y()
        # Check black keys first
        white_offsets = {1:0,3:2,6:5,8:7,10:9}
        for note in self._black_notes():
            semi = note % 12
            prev = white_offsets[semi]
            wi = [n for n in self._white_notes()].index(note - (semi - prev))
            kx = 2 + wi * self.white_w + self.white_w - self.black_w // 2
            if kx <= x <= kx + self.black_w and y <= self.black_h + 2:
                self.note_pressed.emit(note + 12)
                return
        # White keys
        for note in self._white_notes():
            kx = self._white_x(note)
            if kx <= x <= kx + self.white_w - 2:
                self.note_pressed.emit(note + 12)
                return

# ---------------------------------------------------------------------------
# Waveform display widget
# ---------------------------------------------------------------------------
class WaveformWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.samples = np.zeros(512, dtype=np.float32)
        self.setMinimumHeight(80)
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)

    def set_samples(self, samples: np.ndarray):
        # Downsample to ~1024 points for display
        n = len(samples)
        target = min(n, 1024)
        idx = np.linspace(0, n - 1, target, dtype=int)
        self.samples = samples[idx]
        self.update()

    def paintEvent(self, event):
        p = QPainter(self)
        p.fillRect(self.rect(), QColor(10, 15, 25))
        w = self.width(); h = self.height()
        mid = h // 2
        p.setPen(QPen(QColor(40, 40, 60), 1))
        p.drawLine(0, mid, w, mid)
        n = len(self.samples)
        if n < 2:
            return
        p.setPen(QPen(QColor(0, 200, 120), 1))
        for i in range(1, n):
            x0 = int((i - 1) * w / n)
            x1 = int(i * w / n)
            y0 = int(mid - self.samples[i-1] * (mid - 2))
            y1 = int(mid - self.samples[i] * (mid - 2))
            p.drawLine(x0, y0, x1, y1)

# ---------------------------------------------------------------------------
# FM Patch Editor Tab
# ---------------------------------------------------------------------------
class OperatorWidget(QGroupBox):
    """Single FM operator (op1..op4) editor."""
    changed = pyqtSignal()

    PARAMS = [
        ("DT/MUL", "dt_mul", 0x01, 0x00, 0xFF),
        ("TL",     "tl",     0x20, 0x00, 0x7F),
        ("AR",     "ar",     0x1F, 0x00, 0x1F),
        ("DR",     "dr",     0x08, 0x00, 0x1F),
        ("SR",     "sr",     0x05, 0x00, 0x1F),
        ("SL/RR",  "sl_rr",  0xAF, 0x00, 0xFF),
        ("ENV",    "env_ty", 0x00, 0x00, 0x01),
    ]

    def __init__(self, op_name: str, parent=None):
        super().__init__(op_name, parent)
        self.op_name = op_name
        self._spins = {}
        grid = QGridLayout()
        self.setLayout(grid)
        for row, (label, key, default, lo, hi) in enumerate(self.PARAMS):
            grid.addWidget(QLabel(label), row, 0)
            sb = QSpinBox()
            sb.setRange(0, hi)
            sb.setValue(default)
            sb.setDisplayIntegerBase(16)
            sb.setPrefix("0x")
            sb.valueChanged.connect(self.changed)
            grid.addWidget(sb, row, 1)
            self._spins[key] = sb
        self.setMaximumWidth(160)

    def get_values(self) -> list:
        keys = ["dt_mul","tl","ar","dr","sr","sl_rr","env_ty"]
        return [self._spins[k].value() for k in keys]

    def set_values(self, vals: list):
        keys = ["dt_mul","tl","ar","dr","sr","sl_rr","env_ty"]
        for k, v in zip(keys, vals):
            self._spins[k].blockSignals(True)
            self._spins[k].setValue(v)
            self._spins[k].blockSignals(False)

class AlgorithmDiagram(QWidget):
    """Draws the current FM algorithm block diagram."""
    def __init__(self, parent=None):
        super().__init__(parent)
        self.alg = 7
        self.setFixedSize(200, 120)

    def set_alg(self, alg: int):
        self.alg = alg & 7
        self.update()

    def paintEvent(self, event):
        p = QPainter(self)
        p.fillRect(self.rect(), QColor(20, 20, 30))
        p.setPen(QPen(QColor(180, 200, 255), 1))
        p.setFont(QFont("Monospace", 8))
        desc = ALG_DESCRIPTIONS[self.alg]
        # Draw simple text representation
        lines = desc.replace("→", "\n→ ").split("\n")
        y = 15
        for line in lines:
            p.drawText(5, y, line)
            y += 14

class FMPatchTab(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.patches = []
        self._current_idx = 0
        self._audio_sink = None
        self._build_ui()
        self._load_patches()

    def _build_ui(self):
        layout = QHBoxLayout(self)

        # Left: patch list
        left = QVBoxLayout()
        left.addWidget(QLabel("Patches"))
        self.patch_list = QListWidget()
        self.patch_list.setMaximumWidth(200)
        self.patch_list.currentRowChanged.connect(self._on_patch_selected)
        left.addWidget(self.patch_list)
        btn_add = QPushButton("+ New Patch")
        btn_add.clicked.connect(self._add_patch)
        left.addWidget(btn_add)
        btn_save = QPushButton("Save patches.fm")
        btn_save.clicked.connect(self._save_patches)
        left.addWidget(btn_save)
        layout.addLayout(left)

        # Center: patch parameters
        center = QVBoxLayout()

        # Name, alg, fb, stereo, lfo
        form = QFormLayout()
        self.name_edit = QTextEdit(); self.name_edit.setMaximumHeight(30)
        self.name_edit.textChanged.connect(self._on_param_changed)
        form.addRow("Name:", self.name_edit)
        self.alg_spin = QSpinBox(); self.alg_spin.setRange(0, 7)
        self.alg_spin.valueChanged.connect(self._on_alg_changed)
        form.addRow("ALG:", self.alg_spin)
        self.fb_spin = QSpinBox(); self.fb_spin.setRange(0, 7)
        self.fb_spin.valueChanged.connect(self._on_param_changed)
        form.addRow("FB:", self.fb_spin)
        self.stereo_spin = QSpinBox(); self.stereo_spin.setRange(0, 0xFF)
        self.stereo_spin.setDisplayIntegerBase(16); self.stereo_spin.setPrefix("0x")
        self.stereo_spin.valueChanged.connect(self._on_param_changed)
        form.addRow("Stereo:", self.stereo_spin)
        self.lfo_spin = QSpinBox(); self.lfo_spin.setRange(0, 0xFF)
        self.lfo_spin.setDisplayIntegerBase(16); self.lfo_spin.setPrefix("0x")
        self.lfo_spin.valueChanged.connect(self._on_param_changed)
        form.addRow("LFO:", self.lfo_spin)
        center.addLayout(form)

        # Algorithm diagram
        alg_row = QHBoxLayout()
        self.alg_diagram = AlgorithmDiagram()
        alg_row.addWidget(self.alg_diagram)
        alg_row.addStretch()
        center.addLayout(alg_row)

        # Operators
        op_row = QHBoxLayout()
        self.op_widgets = {}
        for name in ("op1", "op2", "op3", "op4"):
            w = OperatorWidget(name.upper())
            w.changed.connect(self._on_param_changed)
            self.op_widgets[name] = w
            op_row.addWidget(w)
        center.addLayout(op_row)

        layout.addLayout(center)

        # Right: preview
        right = QVBoxLayout()
        right.addWidget(QLabel("Preview"))
        self.alg_desc = QLabel("")
        self.alg_desc.setWordWrap(True)
        right.addWidget(self.alg_desc)

        oct_row = QHBoxLayout()
        oct_row.addWidget(QLabel("Octave:"))
        self.oct_spin = QSpinBox(); self.oct_spin.setRange(1, 7); self.oct_spin.setValue(4)
        self.oct_spin.valueChanged.connect(lambda v: setattr(self.keyboard, 'octave', v))
        oct_row.addWidget(self.oct_spin)
        right.addLayout(oct_row)

        self.keyboard = PianoKeyboard()
        self.keyboard.note_pressed.connect(self._play_note)
        right.addWidget(self.keyboard)

        self.waveform = WaveformWidget()
        right.addWidget(self.waveform)
        self.note_label = QLabel("Click a key to preview")
        self.note_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        right.addWidget(self.note_label)

        play_row = QHBoxLayout()
        btn_arp = QPushButton("▶ Arpeggio")
        btn_arp.setToolTip("Play C-E-G-C major arpeggio")
        btn_arp.clicked.connect(self._play_arpeggio)
        play_row.addWidget(btn_arp)
        btn_chord = QPushButton("▶ Chord")
        btn_chord.setToolTip("Play C+E+G major chord")
        btn_chord.clicked.connect(self._play_chord)
        play_row.addWidget(btn_chord)
        right.addLayout(play_row)
        right.addStretch()
        layout.addLayout(right)

    def _load_patches(self):
        if PATCHES_FILE.exists():
            self.patches = parse_patches(PATCHES_FILE)
        if not self.patches:
            self.patches = [{"id": 0, "name": "Default", "alg": 7, "fb": 2,
                             "stereo": 0xC0, "lfo": 0,
                             "ops": {k: [0x01,0x20,0x1F,0x08,0x05,0xAF,0x00]
                                     for k in ("op1","op2","op3","op4")}}]
        self.patch_list.clear()
        for p in self.patches:
            self.patch_list.addItem(f"{p['id']:2d}  {p['name']}")
        self.patch_list.setCurrentRow(0)

    def _on_patch_selected(self, row: int):
        if row < 0 or row >= len(self.patches):
            return
        self._current_idx = row
        p = self.patches[row]
        self.name_edit.blockSignals(True)
        self.name_edit.setText(p["name"])
        self.name_edit.blockSignals(False)
        self.alg_spin.blockSignals(True)
        self.alg_spin.setValue(p["alg"])
        self.alg_spin.blockSignals(False)
        self.fb_spin.blockSignals(True)
        self.fb_spin.setValue(p["fb"])
        self.fb_spin.blockSignals(False)
        self.stereo_spin.blockSignals(True)
        self.stereo_spin.setValue(p["stereo"])
        self.stereo_spin.blockSignals(False)
        self.lfo_spin.blockSignals(True)
        self.lfo_spin.setValue(p["lfo"])
        self.lfo_spin.blockSignals(False)
        for op_name, w in self.op_widgets.items():
            vals = p["ops"].get(op_name, [0x01,0x20,0x1F,0x08,0x05,0xAF,0x00])
            w.set_values(vals)
        self._update_alg_display(p["alg"])

    def _on_alg_changed(self, val):
        self._update_alg_display(val)
        self._on_param_changed()

    def _update_alg_display(self, alg: int):
        self.alg_diagram.set_alg(alg)
        self.alg_desc.setText(ALG_DESCRIPTIONS[alg])

    def _on_param_changed(self):
        if self._current_idx >= len(self.patches):
            return
        p = self.patches[self._current_idx]
        p["name"] = self.name_edit.toPlainText().strip()
        p["alg"] = self.alg_spin.value()
        p["fb"] = self.fb_spin.value()
        p["stereo"] = self.stereo_spin.value()
        p["lfo"] = self.lfo_spin.value()
        for op_name, w in self.op_widgets.items():
            p["ops"][op_name] = w.get_values()
        self.patch_list.item(self._current_idx).setText(
            f"{p['id']:2d}  {p['name']}")

    def _play_note(self, midi: int):
        if self._current_idx >= len(self.patches):
            return
        patch = self.patches[self._current_idx]
        samples = synthesize_fm(patch, midi)
        self.waveform.set_samples(samples)
        self.note_label.setText(f"Note: {midi_to_name(midi)}  (MIDI {midi})")
        self._audio_sink = play_samples(samples)

    def _play_arpeggio(self):
        if self._current_idx >= len(self.patches):
            return
        patch = self.patches[self._current_idx]
        base = self.oct_spin.value() * 12
        notes = [base, base + 4, base + 7, base + 12]
        dur = 0.18
        total = int(SAMPLE_RATE * (dur * len(notes) + 0.3))
        buf = np.zeros(total, dtype=np.float32)
        for i, midi in enumerate(notes):
            s = synthesize_fm(patch, midi, dur)
            off = int(i * dur * SAMPLE_RATE)
            end = min(total, off + len(s))
            buf[off:end] += s[:end - off]
        peak = np.max(np.abs(buf))
        if peak > 0: buf /= peak
        self.waveform.set_samples(buf)
        self.note_label.setText("Arpeggio: C-E-G-C")
        self._audio_sink = play_samples(buf * 0.85)

    def _play_chord(self):
        if self._current_idx >= len(self.patches):
            return
        patch = self.patches[self._current_idx]
        base = self.oct_spin.value() * 12
        notes = [base, base + 4, base + 7]
        dur = 0.6
        buf = np.zeros(int(SAMPLE_RATE * dur), dtype=np.float32)
        for midi in notes:
            buf += synthesize_fm(patch, midi, dur) * 0.4
        peak = np.max(np.abs(buf))
        if peak > 0: buf /= peak
        self.waveform.set_samples(buf)
        self.note_label.setText("Chord: C+E+G")
        self._audio_sink = play_samples(buf * 0.85)

    def _add_patch(self):
        new_id = max((p["id"] for p in self.patches), default=-1) + 1
        p = {"id": new_id, "name": f"Patch {new_id}", "alg": 7, "fb": 2,
             "stereo": 0xC0, "lfo": 0,
             "ops": {k: [0x01,0x20,0x1F,0x08,0x05,0xAF,0x00]
                     for k in ("op1","op2","op3","op4")}}
        self.patches.append(p)
        self.patch_list.addItem(f"{p['id']:2d}  {p['name']}")
        self.patch_list.setCurrentRow(len(self.patches) - 1)

    def _save_patches(self):
        write_patches(self.patches, PATCHES_FILE)
        QMessageBox.information(self, "Saved", f"Saved {len(self.patches)} patches to\n{PATCHES_FILE}")

# ---------------------------------------------------------------------------
# MML Composer Tab (FM and SSG)
# ---------------------------------------------------------------------------
class MmlHighlighter(QSyntaxHighlighter):
    def highlightBlock(self, text):
        comment_fmt = QTextCharFormat()
        comment_fmt.setForeground(QColor(100, 160, 100))
        cmd_fmt = QTextCharFormat()
        cmd_fmt.setForeground(QColor(100, 160, 255))
        note_fmt = QTextCharFormat()
        note_fmt.setForeground(QColor(255, 200, 80))

        for m in re.finditer(r";.*", text):
            self.setFormat(m.start(), m.end() - m.start(), comment_fmt)
        for m in re.finditer(r"\b[TtVvIiLlOo]\d+", text):
            self.setFormat(m.start(), m.end() - m.start(), cmd_fmt)
        for m in re.finditer(r"[a-gA-GrR][+\-]?\d*\.?", text):
            self.setFormat(m.start(), m.end() - m.start(), note_fmt)

class MmlComposerTab(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self._current_file = None
        self._current_mode = "fm"
        self._audio_sink = None
        self._patches = []
        self._ssg_presets = []
        self._build_ui()
        self._load_file_list()
        self._reload_patches()

    def _build_ui(self):
        layout = QHBoxLayout(self)

        # Left: file list
        left = QVBoxLayout()
        mode_row = QHBoxLayout()
        mode_row.addWidget(QLabel("Mode:"))
        self.mode_combo = QComboBox()
        self.mode_combo.addItems(["FM", "SSG"])
        self.mode_combo.currentTextChanged.connect(self._on_mode_changed)
        mode_row.addWidget(self.mode_combo)
        left.addLayout(mode_row)
        left.addWidget(QLabel("Tracks:"))
        self.file_list = QListWidget()
        self.file_list.setMaximumWidth(180)
        self.file_list.currentItemChanged.connect(self._on_file_selected)
        left.addWidget(self.file_list)
        btn_new = QPushButton("+ New Track")
        btn_new.clicked.connect(self._new_track)
        left.addWidget(btn_new)
        btn_save = QPushButton("Save Track")
        btn_save.clicked.connect(self._save_track)
        left.addWidget(btn_save)
        btn_compile = QPushButton("Compile All")
        btn_compile.clicked.connect(self._compile_all)
        left.addWidget(btn_compile)

        self.bpm_mml = QSpinBox(); self.bpm_mml.setRange(40, 240); self.bpm_mml.setValue(120)
        self.bpm_mml.setPrefix("BPM: ")
        left.addWidget(self.bpm_mml)

        self.btn_play_track = QPushButton("▶ Play Track")
        self.btn_play_track.clicked.connect(self._play_track)
        left.addWidget(self.btn_play_track)
        self.btn_stop_track = QPushButton("■ Stop")
        self.btn_stop_track.clicked.connect(self._stop_track)
        left.addWidget(self.btn_stop_track)
        left.addStretch()
        layout.addLayout(left)

        # Center: editor + roll
        center = QVBoxLayout()
        self.editor = QPlainTextEdit()
        self.editor.setFont(QFont("Monospace", 10))
        self.editor.textChanged.connect(self._on_text_changed)
        MmlHighlighter(self.editor.document())
        center.addWidget(self.editor, 2)

        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        self.roll = PianoRollWidget()
        scroll.setWidget(self.roll)
        scroll.setMinimumHeight(180)
        center.addWidget(scroll, 1)
        layout.addLayout(center, 2)

        # Right: preview
        right = QVBoxLayout()
        right.addWidget(QLabel("Preview"))
        oct_row = QHBoxLayout()
        oct_row.addWidget(QLabel("Octave:"))
        self.oct_spin = QSpinBox(); self.oct_spin.setRange(1,7); self.oct_spin.setValue(4)
        self.oct_spin.valueChanged.connect(lambda v: setattr(self.keyboard, 'octave', v))
        oct_row.addWidget(self.oct_spin)
        right.addLayout(oct_row)
        self.keyboard = PianoKeyboard()
        self.keyboard.note_pressed.connect(self._play_key_preview)
        right.addWidget(self.keyboard)
        self.waveform = WaveformWidget()
        right.addWidget(self.waveform)

        right.addWidget(QLabel("MML Quick Reference:"))
        ref = QPlainTextEdit()
        ref.setReadOnly(True)
        ref.setFont(QFont("Monospace", 8))
        ref.setMaximumHeight(200)
        ref.setPlainText(
            "T<n>  Tempo (BPM)\n"
            "V<n>  Volume (0-15)\n"
            "I<n>  Instrument/patch\n"
            "O<n>  Octave (0-8)\n"
            "L<n>  Default length (1,2,4,8,16)\n"
            ">/<   Octave up/down\n"
            "C-B   Notes (+ sharp, - flat)\n"
            "R     Rest\n"
            "Note. Dotted (x1.5 duration)\n\n"
            "Length: 1=whole, 2=half,\n"
            "        4=quarter, 8=eighth\n"
            "        16=sixteenth\n"
        )
        right.addWidget(ref)
        right.addStretch()
        layout.addLayout(right)

    def _load_file_list(self):
        mode_dir = FM_DIR if self._current_mode == "fm" else SSG_DIR
        self.file_list.clear()
        for f in sorted(mode_dir.glob("*.mml")):
            self.file_list.addItem(f.name)

    def _on_mode_changed(self, mode_text: str):
        self._current_mode = mode_text.lower()
        self._load_file_list()
        self._current_file = None
        self.editor.clear()

    def _on_file_selected(self, item, prev):
        if item is None:
            return
        mode_dir = FM_DIR if self._current_mode == "fm" else SSG_DIR
        path = mode_dir / item.text()
        self._current_file = path
        self.editor.blockSignals(True)
        self.editor.setPlainText(path.read_text())
        self.editor.blockSignals(False)
        self._refresh_roll()

    def _on_text_changed(self):
        self._refresh_roll()

    def _refresh_roll(self):
        text = self.editor.toPlainText()
        events = parse_mml_events(text)
        self.roll.set_events(events)

    def _play_key_preview(self, midi: int):
        if self._current_mode == "fm":
            patch = {"alg": 7, "fb": 2, "stereo": 0xC0, "lfo": 0,
                     "ops": {k: [0x01,0x20,0x1F,0x08,0x05,0xAF,0x00]
                             for k in ("op1","op2","op3","op4")}}
            samples = synthesize_fm(patch, midi)
        else:
            preset = {"vol_a": 12, "vol_b": 0, "vol_c": 0}
            samples = synthesize_ssg(preset, midi)
        self.waveform.set_samples(samples)
        self._audio_sink = play_samples(samples)

    def _reload_patches(self):
        if PATCHES_FILE.exists():
            self._patches = parse_patches(PATCHES_FILE)
        if SSG_CONFIG.exists():
            self._ssg_presets = parse_ssg_presets(SSG_CONFIG)

    def _play_track(self):
        self._stop_track()
        self._reload_patches()
        text = self.editor.toPlainText()
        events = parse_mml_events(text)
        if not events:
            return
        bpm = self.bpm_mml.value()
        buf = synthesize_track(events, self._patches, self._ssg_presets, bpm, self._current_mode)
        self.waveform.set_samples(buf)
        self._audio_sink = play_samples(buf)
        self.btn_play_track.setEnabled(False)
        dur_ms = int(len(buf) / SAMPLE_RATE * 1000) + 200
        QTimer.singleShot(dur_ms, lambda: self.btn_play_track.setEnabled(True))

    def _stop_track(self):
        if self._audio_sink:
            try: self._audio_sink.stop()
            except: pass
            self._audio_sink = None
        self.btn_play_track.setEnabled(True)

    def _new_track(self):
        mode_dir = FM_DIR if self._current_mode == "fm" else SSG_DIR
        count = len(list(mode_dir.glob("*.mml")))
        path = mode_dir / f"{count}_new_track.mml"
        path.write_text("; New track\nT120\nV12\nO4\nL8\nC E G\n")
        self._load_file_list()
        for i in range(self.file_list.count()):
            if self.file_list.item(i).text() == path.name:
                self.file_list.setCurrentRow(i)
                break

    def _save_track(self):
        if self._current_file is None:
            QMessageBox.warning(self, "No File", "Select or create a track first.")
            return
        self._current_file.write_text(self.editor.toPlainText())
        QMessageBox.information(self, "Saved", f"Saved {self._current_file.name}")

    def _compile_all(self):
        import subprocess
        mode = self._current_mode
        if mode == "fm":
            script = TOOLS_DIR / "fm_compile.py"
            glob = FM_DIR.glob("*.mml")
            out = SOUND_DIR / "driver" / "fm_data.inc"
        else:
            script = TOOLS_DIR / "ssg_compile.py"
            glob = SSG_DIR.glob("*.mml")
            out = SOUND_DIR / "driver" / "ssg_data.inc"
        files = sorted(str(f) for f in glob)
        if not files:
            QMessageBox.warning(self, "No Files", "No .mml files found.")
            return
        cmd = [sys.executable, str(script)] + files + ["-o", str(out)]
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
            if result.returncode == 0:
                QMessageBox.information(self, "Compiled", f"Written: {out.name}")
            else:
                QMessageBox.critical(self, "Error", result.stderr or "Compile failed.")
        except Exception as e:
            QMessageBox.critical(self, "Error", str(e))

# ---------------------------------------------------------------------------
# SSG Preset Editor Tab
# ---------------------------------------------------------------------------
class SSGPresetTab(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.presets = []
        self._current_idx = 0
        self._audio_sink = None
        self._build_ui()
        self._load_presets()

    def _build_ui(self):
        layout = QHBoxLayout(self)

        # Left: preset list
        left = QVBoxLayout()
        left.addWidget(QLabel("SSG Presets"))
        self.preset_list = QListWidget()
        self.preset_list.setMaximumWidth(200)
        self.preset_list.currentRowChanged.connect(self._on_preset_selected)
        left.addWidget(self.preset_list)
        btn_add = QPushButton("+ New Preset")
        btn_add.clicked.connect(self._add_preset)
        left.addWidget(btn_add)
        btn_save = QPushButton("Save config.ssg")
        btn_save.clicked.connect(self._save_presets)
        left.addWidget(btn_save)
        layout.addLayout(left)

        # Center: parameters
        center = QVBoxLayout()
        form = QFormLayout()
        self.name_edit = QTextEdit(); self.name_edit.setMaximumHeight(30)
        self.name_edit.textChanged.connect(self._on_param_changed)
        form.addRow("Name:", self.name_edit)

        self.tone_mask_spin = QSpinBox()
        self.tone_mask_spin.setRange(0, 0xFF)
        self.tone_mask_spin.setDisplayIntegerBase(16); self.tone_mask_spin.setPrefix("0x")
        self.tone_mask_spin.valueChanged.connect(self._on_param_changed)
        form.addRow("Tone Mask:", self.tone_mask_spin)

        for ch in ("a", "b", "c"):
            sb = QSpinBox(); sb.setRange(0, 0x0F)
            sb.setDisplayIntegerBase(16); sb.setPrefix("0x")
            sb.valueChanged.connect(self._on_param_changed)
            form.addRow(f"Vol {ch.upper()}:", sb)
            setattr(self, f"vol_{ch}_spin", sb)

        self.noise_spin = QSpinBox()
        self.noise_spin.setRange(0, 0x1F)
        self.noise_spin.setDisplayIntegerBase(16); self.noise_spin.setPrefix("0x")
        self.noise_spin.valueChanged.connect(self._on_param_changed)
        form.addRow("Noise Freq:", self.noise_spin)

        center.addLayout(form)
        center.addStretch()
        layout.addLayout(center)

        # Right: preview
        right = QVBoxLayout()
        right.addWidget(QLabel("SSG Channel Preview"))
        oct_row = QHBoxLayout()
        oct_row.addWidget(QLabel("Octave:"))
        self.oct_spin = QSpinBox(); self.oct_spin.setRange(1,7); self.oct_spin.setValue(4)
        self.oct_spin.valueChanged.connect(lambda v: setattr(self.keyboard, 'octave', v))
        oct_row.addWidget(self.oct_spin)
        right.addLayout(oct_row)
        self.keyboard = PianoKeyboard()
        self.keyboard.note_pressed.connect(self._play_note)
        right.addWidget(self.keyboard)
        self.waveform = WaveformWidget()
        right.addWidget(self.waveform)
        right.addWidget(QLabel(
            "SSG Channels A/B/C produce square\n"
            "waves at the selected frequency.\n"
            "Tone mask 0x38 = all tone on,\n"
            "noise disabled."
        ))
        right.addStretch()
        layout.addLayout(right)

    def _load_presets(self):
        if SSG_CONFIG.exists():
            self.presets = parse_ssg_presets(SSG_CONFIG)
        if not self.presets:
            self.presets = [{"id": 0, "name": "Default", "tone_mask": 0x38,
                             "vol_a": 10, "vol_b": 6, "vol_c": 0, "noise_freq": 0}]
        self.preset_list.clear()
        for p in self.presets:
            self.preset_list.addItem(f"{p['id']:2d}  {p['name']}")
        self.preset_list.setCurrentRow(0)

    def _on_preset_selected(self, row):
        if row < 0 or row >= len(self.presets):
            return
        self._current_idx = row
        p = self.presets[row]
        self.name_edit.blockSignals(True)
        self.name_edit.setText(p["name"])
        self.name_edit.blockSignals(False)
        for attr, key in [("tone_mask_spin","tone_mask"),("vol_a_spin","vol_a"),
                           ("vol_b_spin","vol_b"),("vol_c_spin","vol_c"),("noise_spin","noise_freq")]:
            w = getattr(self, attr)
            w.blockSignals(True); w.setValue(p[key]); w.blockSignals(False)

    def _on_param_changed(self):
        if self._current_idx >= len(self.presets):
            return
        p = self.presets[self._current_idx]
        p["name"] = self.name_edit.toPlainText().strip()
        p["tone_mask"] = self.tone_mask_spin.value()
        p["vol_a"] = self.vol_a_spin.value()
        p["vol_b"] = self.vol_b_spin.value()
        p["vol_c"] = self.vol_c_spin.value()
        p["noise_freq"] = self.noise_spin.value()
        self.preset_list.item(self._current_idx).setText(f"{p['id']:2d}  {p['name']}")

    def _play_note(self, midi):
        if self._current_idx >= len(self.presets):
            return
        preset = self.presets[self._current_idx]
        samples = synthesize_ssg(preset, midi)
        self.waveform.set_samples(samples)
        self._audio_sink = play_samples(samples)

    def _add_preset(self):
        new_id = max((p["id"] for p in self.presets), default=-1) + 1
        p = {"id": new_id, "name": f"Preset {new_id}", "tone_mask": 0x38,
             "vol_a": 10, "vol_b": 6, "vol_c": 0, "noise_freq": 0}
        self.presets.append(p)
        self.preset_list.addItem(f"{p['id']:2d}  {p['name']}")
        self.preset_list.setCurrentRow(len(self.presets) - 1)

    def _save_presets(self):
        write_ssg_presets(self.presets, SSG_CONFIG)
        QMessageBox.information(self, "Saved", f"Saved {len(self.presets)} presets to\n{SSG_CONFIG}")

# ---------------------------------------------------------------------------
# ADPCM Manager Tab
# ---------------------------------------------------------------------------
class ADPCMTab(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self._audio_sink = None
        self._current_samples = None
        self._build_ui()
        self._load_samples()

    def _build_ui(self):
        layout = QHBoxLayout(self)

        # Left: sample list
        left = QVBoxLayout()
        chan_row = QHBoxLayout()
        chan_row.addWidget(QLabel("Channel:"))
        self.chan_combo = QComboBox()
        self.chan_combo.addItems(["ADPCM-A (SFX)", "ADPCM-B (Music/BG)"])
        self.chan_combo.currentIndexChanged.connect(self._load_samples)
        chan_row.addWidget(self.chan_combo)
        left.addLayout(chan_row)
        left.addWidget(QLabel("WAV Samples:"))
        self.sample_list = QListWidget()
        self.sample_list.setMaximumWidth(200)
        self.sample_list.currentItemChanged.connect(self._on_sample_selected)
        left.addWidget(self.sample_list)
        left.addWidget(QLabel("Drop WAV files here or\nuse Add Sample button:"))
        btn_add = QPushButton("Add WAV Sample…")
        btn_add.clicked.connect(self._add_sample)
        left.addWidget(btn_add)
        left.addStretch()
        layout.addLayout(left)

        # Center: waveform + info
        center = QVBoxLayout()
        center.addWidget(QLabel("Waveform Preview"))
        self.waveform = WaveformWidget()
        self.waveform.setMinimumHeight(120)
        center.addWidget(self.waveform)

        self.info_label = QLabel("Select a sample")
        self.info_label.setAlignment(Qt.AlignmentFlag.AlignTop | Qt.AlignmentFlag.AlignLeft)
        self.info_label.setWordWrap(True)
        center.addWidget(self.info_label)

        btn_row = QHBoxLayout()
        self.btn_play = QPushButton("▶ Play")
        self.btn_play.clicked.connect(self._play_sample)
        btn_row.addWidget(self.btn_play)
        center.addLayout(btn_row)
        center.addStretch()
        layout.addLayout(center, 2)

        # Right: encoding info
        right = QVBoxLayout()
        right.addWidget(QLabel("Encoding Pipeline"))
        info = QPlainTextEdit()
        info.setReadOnly(True)
        info.setFont(QFont("Monospace", 8))
        info.setPlainText(
            "ADPCM-A (SFX, 6 channels)\n"
            "  in_wav_a/N.wav  →\n"
            "  out_16el_a/N.wav  (16kHz mono)\n"
            "  out_a/N.adpcma  (4-bit ADPCM)\n"
            "  → packed into 777-v1.v1\n\n"
            "ADPCM-B (streaming, 1 channel)\n"
            "  in_wav_b/N.wav  →\n"
            "  out_16el_b/N.wav  (16kHz mono)\n"
            "  out_b/N.adpcmb  (4-bit ADPCM)\n"
            "  → packed into 777-v1.v1\n\n"
            "Rebuild VROM:\n"
            "  make vrom  (Linux)\n"
            "  nmake -f MakefileWin32.mak vrom\n\n"
            "YM2610 ADPCM-A specs:\n"
            "  Sample rate: up to ~18.5 kHz\n"
            "  Max channels: 6 simultaneous\n"
            "  Format: OKI MSM6242B ADPCM\n\n"
            "YM2610 ADPCM-B specs:\n"
            "  Sample rate: variable\n"
            "  Max channels: 1\n"
            "  Format: same ADPCM encoding\n"
        )
        right.addWidget(info)
        layout.addLayout(right)

    def _samples_dir(self):
        ch = self.chan_combo.currentIndex()
        return SOUND_DIR / "samples" / ("in_wav_a" if ch == 0 else "in_wav_b")

    def _load_samples(self):
        d = self._samples_dir()
        self.sample_list.clear()
        if d.exists():
            for f in sorted(d.glob("*.wav")):
                self.sample_list.addItem(f.name)

    def _on_sample_selected(self, item, prev):
        if item is None:
            return
        path = self._samples_dir() / item.text()
        self._load_wav(path)

    def _load_wav(self, path: Path):
        try:
            with wave.open(str(path)) as wf:
                n = wf.getnframes()
                raw = wf.readframes(n)
                sw = wf.getsampwidth()
                nc = wf.getnchannels()
                sr = wf.getframerate()
            if sw == 2:
                data = np.frombuffer(raw, dtype=np.int16).astype(np.float32) / 32768.0
            elif sw == 1:
                data = (np.frombuffer(raw, dtype=np.uint8).astype(np.float32) - 128) / 128.0
            else:
                data = np.zeros(1024, dtype=np.float32)
            if nc > 1:
                data = data[::nc]
            self._current_samples = data
            self.waveform.set_samples(data)
            dur = n / sr
            self.info_label.setText(
                f"File: {path.name}\n"
                f"Sample rate: {sr} Hz\n"
                f"Channels: {nc}\n"
                f"Duration: {dur:.2f}s\n"
                f"Frames: {n}\n"
            )
        except Exception as e:
            self.info_label.setText(f"Error: {e}")
            self._current_samples = None

    def _play_sample(self):
        if self._current_samples is None:
            return
        sr_src = 44100  # assume
        samples = self._current_samples
        if len(samples) > SAMPLE_RATE * 10:
            samples = samples[:SAMPLE_RATE * 10]
        self._audio_sink = play_samples(samples.astype(np.float32))

    def _add_sample(self):
        paths, _ = QFileDialog.getOpenFileNames(
            self, "Add WAV Samples", str(self._samples_dir()), "WAV Files (*.wav)")
        for src in paths:
            dest = self._samples_dir() / Path(src).name
            if not dest.exists():
                import shutil
                shutil.copy(src, dest)
        self._load_samples()

# ---------------------------------------------------------------------------
# Step Sequencer grid widget
# ---------------------------------------------------------------------------
class StepSeqGrid(QWidget):
    """Clickable step-sequencer grid. Rows = channels, columns = time steps."""
    step_toggled = pyqtSignal(int, int, bool)

    CELL_W = 30
    CELL_H = 30
    LABEL_W = 68

    CH_COLORS = [
        QColor(60, 120, 220), QColor(40, 100, 200),
        QColor(30, 80, 180),  QColor(20, 60, 160),
        QColor(190, 110, 20), QColor(165, 85, 15),
        QColor(140, 62, 10),
    ]

    def __init__(self, num_channels: int = 7, num_steps: int = 16, parent=None):
        super().__init__(parent)
        self.num_channels = num_channels
        self.num_steps = num_steps
        self.grid = [[False] * num_steps for _ in range(num_channels)]
        self.current_step = -1
        self.channel_labels = ["FM 1","FM 2","FM 3","FM 4","SSG A","SSG B","SSG C"]
        self._resize()

    def _resize(self):
        w = self.LABEL_W + self.num_steps * self.CELL_W + 4
        h = self.num_channels * self.CELL_H + 4
        self.setFixedSize(w, h)

    def set_step_count(self, n: int):
        for ch in range(self.num_channels):
            cur = self.grid[ch]
            if len(cur) < n:
                cur += [False] * (n - len(cur))
            self.grid[ch] = cur[:n]
        self.num_steps = n
        self._resize()
        self.update()

    def set_playhead(self, step: int):
        self.current_step = step
        self.update()

    def clear_all(self):
        self.grid = [[False] * self.num_steps for _ in range(self.num_channels)]
        self.update()

    def paintEvent(self, event):
        p = QPainter(self)
        p.fillRect(self.rect(), QColor(20, 20, 30))
        for ch in range(self.num_channels):
            y = ch * self.CELL_H + 2
            p.setPen(QColor(200, 200, 220))
            p.setFont(QFont("Monospace", 8))
            label = self.channel_labels[ch] if ch < len(self.channel_labels) else f"CH{ch}"
            p.drawText(2, y + self.CELL_H - 8, label)
            col = self.CH_COLORS[ch % len(self.CH_COLORS)]
            for step in range(self.num_steps):
                x = self.LABEL_W + step * self.CELL_W + 2
                rx = x + 1; ry = y + 1
                rw = self.CELL_W - 2; rh = self.CELL_H - 2
                active = step < len(self.grid[ch]) and self.grid[ch][step]
                is_beat = step % 4 == 0
                if active:
                    p.fillRect(rx, ry, rw, rh, col)
                    p.setPen(QPen(col.lighter(160), 1))
                    p.drawLine(rx, ry, rx + rw, ry)
                elif is_beat:
                    p.fillRect(rx, ry, rw, rh, QColor(35, 35, 52))
                else:
                    p.fillRect(rx, ry, rw, rh, QColor(25, 25, 38))
                if step == self.current_step:
                    p.fillRect(rx, ry, rw, rh, QColor(255, 255, 100, 55))
                p.setPen(QPen(QColor(50, 50, 70), 1))
                p.drawRect(rx, ry, rw, rh)

    def mousePressEvent(self, event):
        x = int(event.position().x())
        y = int(event.position().y())
        ch   = (y - 2) // self.CELL_H
        step = (x - self.LABEL_W - 2) // self.CELL_W
        if 0 <= ch < self.num_channels and 0 <= step < self.num_steps:
            self.grid[ch][step] = not self.grid[ch][step]
            self.step_toggled.emit(ch, step, self.grid[ch][step])
            self.update()


# ---------------------------------------------------------------------------
# Step Sequencer / Composer Tab
# ---------------------------------------------------------------------------
class ComposerTab(QWidget):
    """Multi-channel step sequencer — FM 1-4 + SSG A-C, up to 32 steps."""

    NUM_CH   = 7
    CH_TYPES = ["FM","FM","FM","FM","SSG","SSG","SSG"]
    CH_NAMES = ["FM 1","FM 2","FM 3","FM 4","SSG A","SSG B","SSG C"]
    DEF_NOTES = [60, 64, 67, 72, 60, 64, 67]

    def __init__(self, parent=None):
        super().__init__(parent)
        self._patches = []
        self._ssg_presets = []
        self._sink = None
        self._play_step = 0
        self._timer = QTimer()
        self._timer.timeout.connect(self._tick)
        self._build_ui()
        self._reload_data()

    def _build_ui(self):
        main = QVBoxLayout(self)

        # Transport row
        tr = QHBoxLayout()
        self.bpm_spin = QSpinBox(); self.bpm_spin.setRange(40, 240); self.bpm_spin.setValue(120)
        self.bpm_spin.setPrefix("BPM: ")
        tr.addWidget(self.bpm_spin)

        self.steps_combo = QComboBox()
        self.steps_combo.addItems(["16 steps", "32 steps"])
        self.steps_combo.currentIndexChanged.connect(
            lambda i: self.seq_grid.set_step_count(32 if i else 16))
        tr.addWidget(self.steps_combo)

        self.loops_spin = QSpinBox(); self.loops_spin.setRange(1, 8); self.loops_spin.setValue(2)
        self.loops_spin.setPrefix("Loops: ")
        tr.addWidget(self.loops_spin)

        self.btn_play = QPushButton("▶ Play")
        self.btn_play.setFixedWidth(80)
        self.btn_play.clicked.connect(self._play)
        tr.addWidget(self.btn_play)

        self.btn_stop = QPushButton("■ Stop")
        self.btn_stop.setFixedWidth(80)
        self.btn_stop.clicked.connect(self._stop)
        tr.addWidget(self.btn_stop)

        btn_clear = QPushButton("Clear All")
        btn_clear.clicked.connect(lambda: self.seq_grid.clear_all())
        tr.addWidget(btn_clear)

        btn_export = QPushButton("Export MML…")
        btn_export.clicked.connect(self._export_mml)
        tr.addWidget(btn_export)
        tr.addStretch()
        main.addLayout(tr)

        # Grid in scroll area
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setMinimumHeight(self.NUM_CH * 32 + 20)
        inner = QWidget()
        il = QVBoxLayout(inner)
        self.seq_grid = StepSeqGrid(self.NUM_CH, 16)
        il.addWidget(self.seq_grid)
        il.addStretch()
        scroll.setWidget(inner)
        main.addWidget(scroll)

        # Per-channel settings
        main.addWidget(QLabel("<b>Channel settings:</b>"))
        ch_grid = QGridLayout()
        for col, h in enumerate(["Channel","Note","Instrument","Volume"]):
            ch_grid.addWidget(QLabel(f"<b>{h}</b>"), 0, col)

        self._ch_note = []; self._ch_inst = []; self._ch_vol = []
        for ch in range(self.NUM_CH):
            ch_grid.addWidget(QLabel(self.CH_NAMES[ch]), ch+1, 0)

            ns = QSpinBox(); ns.setRange(0, 127); ns.setValue(self.DEF_NOTES[ch])
            ns.setSuffix(f"  {midi_to_name(self.DEF_NOTES[ch])}")
            ns.valueChanged.connect(lambda v, s=ns: s.setSuffix(f"  {midi_to_name(v)}"))
            ch_grid.addWidget(ns, ch+1, 1); self._ch_note.append(ns)

            ins = QSpinBox(); ins.setRange(0, 15); ins.setValue(ch % 4)
            ch_grid.addWidget(ins, ch+1, 2); self._ch_inst.append(ins)

            vs = QSpinBox(); vs.setRange(0, 15); vs.setValue(12)
            ch_grid.addWidget(vs, ch+1, 3); self._ch_vol.append(vs)

        main.addLayout(ch_grid)

        self.waveform = WaveformWidget()
        self.waveform.setMinimumHeight(80)
        main.addWidget(self.waveform)
        self.status_lbl = QLabel("Click grid cells to toggle steps, then ▶ Play.")
        main.addWidget(self.status_lbl)

    def _reload_data(self):
        if PATCHES_FILE.exists():
            self._patches = parse_patches(PATCHES_FILE)
        if SSG_CONFIG.exists():
            self._ssg_presets = parse_ssg_presets(SSG_CONFIG)

    def _channels(self):
        return [{"type": self.CH_TYPES[ch], "note": self._ch_note[ch].value(),
                 "inst": self._ch_inst[ch].value(), "vol": self._ch_vol[ch].value()}
                for ch in range(self.NUM_CH)]

    def _play(self):
        self._stop()
        self._reload_data()
        bpm   = self.bpm_spin.value()
        loops = self.loops_spin.value()
        buf = synthesize_step_pattern(self._channels(), self.seq_grid.grid,
                                      bpm, self._patches, self._ssg_presets, loops)
        self.waveform.set_samples(buf)
        self._sink = play_samples(buf)
        num_steps = self.seq_grid.num_steps
        ms_per_step = max(1, int(60000 / bpm / 2))
        self._play_step = -1
        self._timer.start(ms_per_step)
        QTimer.singleShot(num_steps * loops * ms_per_step + 300, self._stop)
        self.status_lbl.setText(f"Playing {num_steps} steps × {loops} loops @ {bpm} BPM…")

    def _tick(self):
        self._play_step = (self._play_step + 1) % self.seq_grid.num_steps
        self.seq_grid.set_playhead(self._play_step)

    def _stop(self):
        self._timer.stop()
        if self._sink:
            try: self._sink.stop()
            except: pass
            self._sink = None
        self.seq_grid.set_playhead(-1)
        self.status_lbl.setText("Stopped.")

    def _export_mml(self):
        bpm = self.bpm_spin.value()
        channels = self._channels()
        num_steps = self.seq_grid.num_steps
        NS = ["C","C+","D","D+","E","F","F+","G","G+","A","A+","B"]
        lines = [f"; Step sequencer export — {bpm} BPM, {num_steps} steps", ""]
        for ch in range(self.NUM_CH):
            active_steps = [s for s in range(num_steps) if self.seq_grid.grid[ch][s]]
            if not active_steps:
                continue
            midi = channels[ch]["note"]
            note_name = NS[midi % 12]
            octave = midi // 12 - 1
            lines.append(f"; {self.CH_NAMES[ch]}")
            lines.append(f"T{bpm} V{channels[ch]['vol']} I{channels[ch]['inst']} O{octave} L8")
            lines.append("".join(note_name if self.seq_grid.grid[ch][s] else "R"
                                 for s in range(num_steps)))
            lines.append("")
        dlg = QMessageBox(self)
        dlg.setWindowTitle("MML Export")
        dlg.setText("Save to sound/fm/ or sound/ssg/ as a .mml file:")
        dlg.setDetailedText("\n".join(lines))
        dlg.exec()


# ---------------------------------------------------------------------------
# YM2610 Simulator Tab
# ---------------------------------------------------------------------------
class YM2610SimTab(QWidget):
    """Interactive YM2610 channel overview and multi-channel sequencer preview."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self._sinks = []
        self._patches = []
        self._ssg_presets = []
        self._build_ui()
        self._reload_data()

    def _build_ui(self):
        layout = QVBoxLayout(self)
        layout.addWidget(QLabel(
            "YM2610 Simulator — Preview all channels simultaneously. "
            "Click a row to audition."))

        # Channel matrix
        self.channel_grid = QGridLayout()
        headers = ["Channel", "Type", "Note", "Instrument", "Volume", "Play"]
        for col, h in enumerate(headers):
            lbl = QLabel(f"<b>{h}</b>")
            lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
            self.channel_grid.addWidget(lbl, 0, col)

        self._channel_rows = []
        channels = [
            ("FM 1", "FM",    60, 0, 12),
            ("FM 2", "FM",    64, 1, 12),
            ("FM 3", "FM",    67, 2, 10),
            ("FM 4", "FM",    72, 3, 10),
            ("SSG A","SSG",   60, 0, 12),
            ("SSG B","SSG",   64, 1, 10),
            ("SSG C","SSG",   67, 2,  8),
            ("ADPCM-A 1","ADPCM-A", 0, 0, 15),
            ("ADPCM-A 2","ADPCM-A", 0, 1, 15),
            ("ADPCM-B","ADPCM-B",   0, 0, 15),
        ]
        for row, (name, ch_type, note, inst, vol) in enumerate(channels, start=1):
            row_widgets = {}
            self.channel_grid.addWidget(QLabel(name), row, 0)
            self.channel_grid.addWidget(QLabel(ch_type), row, 1)
            note_spin = QSpinBox(); note_spin.setRange(0, 127); note_spin.setValue(note)
            note_spin.setPrefix("");
            note_spin.setSuffix(f"  {midi_to_name(note)}")
            note_spin.valueChanged.connect(lambda v, s=note_spin: s.setSuffix(f"  {midi_to_name(v)}"))
            self.channel_grid.addWidget(note_spin, row, 2)
            inst_spin = QSpinBox(); inst_spin.setRange(0, 15); inst_spin.setValue(inst)
            self.channel_grid.addWidget(inst_spin, row, 3)
            vol_spin = QSpinBox(); vol_spin.setRange(0, 15); vol_spin.setValue(vol)
            self.channel_grid.addWidget(vol_spin, row, 4)
            btn = QPushButton("▶")
            btn.setFixedWidth(30)
            ch_type_ref = ch_type
            btn.clicked.connect(lambda checked, r=row-1, t=ch_type_ref: self._play_channel(r, t))
            self.channel_grid.addWidget(btn, row, 5)
            row_widgets = {"note": note_spin, "inst": inst_spin, "vol": vol_spin, "type": ch_type}
            self._channel_rows.append(row_widgets)

        layout.addLayout(self.channel_grid)

        # Play all button
        btn_all = QPushButton("▶ Play All FM + SSG Channels")
        btn_all.clicked.connect(self._play_all)
        layout.addWidget(btn_all)

        # Waveform display
        layout.addWidget(QLabel("Mixed Output Preview:"))
        self.waveform = WaveformWidget()
        self.waveform.setMinimumHeight(120)
        layout.addWidget(self.waveform)

        # YM2610 specs
        layout.addWidget(QLabel(
            "YM2610 (OPN2) hardware channels: 4× FM (2-op carrier+mod), "
            "3× SSG (square wave), 6× ADPCM-A, 1× ADPCM-B"
        ))
        layout.addStretch()

    def _reload_data(self):
        if PATCHES_FILE.exists():
            self._patches = parse_patches(PATCHES_FILE)
        if SSG_CONFIG.exists():
            self._ssg_presets = parse_ssg_presets(SSG_CONFIG)

    def _play_channel(self, ch_idx: int, ch_type: str):
        if ch_idx >= len(self._channel_rows):
            return
        row = self._channel_rows[ch_idx]
        midi = row["note"].value()
        inst = row["inst"].value()
        vol  = row["vol"].value() / 15.0

        if ch_type == "FM":
            patch = self._patches[inst % max(1, len(self._patches))] if self._patches else \
                    {"alg":7,"fb":2,"stereo":0xC0,"lfo":0,
                     "ops":{k:[1,0x20,0x1F,8,5,0xAF,0] for k in ("op1","op2","op3","op4")}}
            samples = synthesize_fm(patch, midi) * vol
        elif ch_type == "SSG":
            preset = self._ssg_presets[inst % max(1, len(self._ssg_presets))] if self._ssg_presets else \
                     {"vol_a":12,"vol_b":0,"vol_c":0}
            samples = synthesize_ssg(preset, midi) * vol
        else:
            return  # ADPCM preview not implemented in sim

        self.waveform.set_samples(samples)
        sink = play_samples(samples)
        self._sinks.append(sink)
        if len(self._sinks) > 8:
            self._sinks = self._sinks[-8:]

    def _play_all(self):
        self._reload_data()
        mixed = np.zeros(int(SAMPLE_RATE * 0.6), dtype=np.float32)
        fm_channels = [(i, "FM") for i in range(4)]
        ssg_channels = [(i+4, "SSG") for i in range(3)]
        for ch_idx, ch_type in fm_channels + ssg_channels:
            if ch_idx >= len(self._channel_rows):
                continue
            row = self._channel_rows[ch_idx]
            midi = row["note"].value()
            inst = row["inst"].value()
            vol  = row["vol"].value() / 15.0
            if ch_type == "FM":
                patch = self._patches[inst % max(1, len(self._patches))] if self._patches else \
                        {"alg":7,"fb":2,"stereo":0xC0,"lfo":0,
                         "ops":{k:[1,0x20,0x1F,8,5,0xAF,0] for k in ("op1","op2","op3","op4")}}
                s = synthesize_fm(patch, midi, 0.6) * vol * 0.3
            else:
                preset = self._ssg_presets[inst % max(1, len(self._ssg_presets))] if self._ssg_presets else \
                         {"vol_a":12,"vol_b":0,"vol_c":0}
                s = synthesize_ssg(preset, midi, 0.6) * vol * 0.3
            n = min(len(mixed), len(s))
            mixed[:n] += s[:n]
        peak = np.max(np.abs(mixed))
        if peak > 0:
            mixed /= peak
        mixed *= 0.85
        self.waveform.set_samples(mixed)
        sink = play_samples(mixed)
        self._sinks.append(sink)

# ---------------------------------------------------------------------------
# Main window
# ---------------------------------------------------------------------------
class SoundStudio(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("NeoGeo YM2610 Sound Studio")
        self.resize(1200, 780)
        self._apply_theme()

        tabs = QTabWidget()
        self.fm_tab       = FMPatchTab()
        self.mml_tab      = MmlComposerTab()
        self.ssg_tab      = SSGPresetTab()
        self.adpcm_tab    = ADPCMTab()
        self.composer_tab = ComposerTab()
        self.sim_tab      = YM2610SimTab()

        tabs.addTab(self.fm_tab,       "FM Patches")
        tabs.addTab(self.mml_tab,      "MML Composer")
        tabs.addTab(self.ssg_tab,      "SSG Presets")
        tabs.addTab(self.adpcm_tab,    "ADPCM Samples")
        tabs.addTab(self.composer_tab, "Composer")
        tabs.addTab(self.sim_tab,      "YM2610 Simulator")
        self.setCentralWidget(tabs)

        self._build_menu()

    def _apply_theme(self):
        from PyQt6.QtWidgets import QApplication as _A
        _A.setStyle("Fusion")
        pal = self.palette()
        pal.setColor(pal.ColorRole.Window,          QColor(28, 28, 35))
        pal.setColor(pal.ColorRole.WindowText,       QColor(210, 215, 230))
        pal.setColor(pal.ColorRole.Base,             QColor(18, 18, 25))
        pal.setColor(pal.ColorRole.AlternateBase,    QColor(38, 38, 48))
        pal.setColor(pal.ColorRole.ToolTipBase,      QColor(40, 40, 55))
        pal.setColor(pal.ColorRole.ToolTipText,      QColor(210, 215, 230))
        pal.setColor(pal.ColorRole.Text,             QColor(210, 215, 230))
        pal.setColor(pal.ColorRole.Button,           QColor(50, 52, 68))
        pal.setColor(pal.ColorRole.ButtonText,       QColor(210, 215, 230))
        pal.setColor(pal.ColorRole.BrightText,       QColor(255, 80, 80))
        pal.setColor(pal.ColorRole.Link,             QColor(80, 160, 255))
        pal.setColor(pal.ColorRole.Highlight,        QColor(60, 120, 210))
        pal.setColor(pal.ColorRole.HighlightedText,  QColor(255, 255, 255))
        self.setPalette(pal)

    def _build_menu(self):
        menu = self.menuBar()
        file_menu = menu.addMenu("File")
        file_menu.addAction("Reload Patches", self.fm_tab._load_patches)
        file_menu.addAction("Reload SSG Presets", self.ssg_tab._load_presets)
        file_menu.addAction("Reload Samples", self.adpcm_tab._load_samples)
        file_menu.addSeparator()
        file_menu.addAction("Quit", self.close)

        help_menu = menu.addMenu("Help")
        help_menu.addAction("YM2610 Overview", self._show_ym2610_docs)

    def _show_ym2610_docs(self):
        msg = QMessageBox(self)
        msg.setWindowTitle("YM2610 (OPN2) Quick Reference")
        msg.setText("""<b>Yamaha YM2610 (OPN2) — NeoGeo Sound Chip</b><br><br>

<b>FM Synthesis (4 channels):</b><br>
• Each channel has 4 operators (Op1-Op4)<br>
• 8 algorithms define operator routing<br>
• Parameters: DT/MUL, TL, AR, DR, SR, SL/RR<br>
• ALG0: full chain 1→2→3→4 (complex)<br>
• ALG7: all operators add to output (rich)<br><br>

<b>SSG (3 channels):</b><br>
• Square wave generator (like AY-3-8910)<br>
• Channels A, B, C with independent vol<br>
• Noise generator sharable across channels<br><br>

<b>ADPCM-A (6 channels):</b><br>
• Short samples / SFX, 4-bit ADPCM<br>
• ~18.5 kHz max sample rate<br>
• All 6 can play simultaneously<br><br>

<b>ADPCM-B (1 channel):</b><br>
• Streaming audio, background music<br>
• Variable sample rate<br><br>

<b>FM Operator Parameters:</b><br>
• DT/MUL: Detune (bits 6-4) + Multiplier (bits 3-0)<br>
• TL: Total Level (attenuation, 0=loudest)<br>
• AR: Attack Rate (0-31)<br>
• DR: Decay Rate (0-31)<br>
• SR: Sustain Rate (0-31)<br>
• SL/RR: Sustain Level (bits 7-4) + Release Rate (bits 3-0)<br>
• ALG: Algorithm (0-7), FB: Feedback (0-7)
""")
        msg.exec()


def main():
    app = QApplication(sys.argv)
    win = SoundStudio()
    win.show()
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
