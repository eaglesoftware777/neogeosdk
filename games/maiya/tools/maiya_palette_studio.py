#!/usr/bin/env python3
"""Maiya Palette Studio -- edit the per-valley and boss recolour tints that
games/maiya/tools/build_commercial_assets.py bakes into the C-ROM, without
touching that script.

Reads and writes games/maiya/artbox/palette_config.json (auto-created with
the shipped defaults the first time either this tool or a normal art build
runs). Each row is a hue shift (-180..180 degrees), a saturation multiplier,
and a value/brightness multiplier applied on top of the source artwork.

Usage:
    python3 games/maiya/tools/maiya_palette_studio.py
"""

import json
import os
import queue
import subprocess
import sys
import threading
from pathlib import Path

try:
    import tkinter as tk
    from tkinter import ttk, messagebox
except ImportError:
    sys.exit(
        "tkinter is not installed for this Python.\n"
        "Debian/Ubuntu: sudo apt-get install python3-tk\n"
        "Fedora:        sudo dnf install python3-tkinter\n"
        "Windows/macOS python.org installers include it already."
    )

GAME_DIR = Path(__file__).resolve().parent.parent
REPO_ROOT = GAME_DIR.parent.parent
sys.path.insert(0, str(Path(__file__).resolve().parent))
import build_commercial_assets as bca  # noqa: E402

IS_WINDOWS = os.name == "nt"
BOSS_SOURCES = ("beetle", "toad", "vulture", "jackal", "owl", "leviathan", "smoggar")


def make_argv(*targets_and_vars):
    if IS_WINDOWS:
        return ["make", "-f", "MakefileWin32.mak", *targets_and_vars]
    return ["make", *targets_and_vars]


class FloatEntry(ttk.Entry):
    def __init__(self, master, value, width=8):
        self.var = tk.StringVar(value=f"{value:g}")
        super().__init__(master, textvariable=self.var, width=width)

    def get_float(self):
        return float(self.var.get())


class ValleyRow:
    def __init__(self, table, row, data):
        self.name_var = tk.StringVar(value=data["name"])
        ttk.Entry(table, textvariable=self.name_var, width=18).grid(row=row, column=0, padx=2, pady=1)
        self.hue = FloatEntry(table, data["hue"])
        self.hue.grid(row=row, column=1, padx=2)
        self.sat = FloatEntry(table, data["sat"])
        self.sat.grid(row=row, column=2, padx=2)
        self.val = FloatEntry(table, data["val"])
        self.val.grid(row=row, column=3, padx=2)

    def to_dict(self):
        return {
            "name": self.name_var.get().strip() or "Valley",
            "hue": self.hue.get_float(),
            "sat": self.sat.get_float(),
            "val": self.val.get_float(),
        }


class BossRow:
    def __init__(self, table, row, data):
        self.name_var = tk.StringVar(value=data["name"])
        ttk.Entry(table, textvariable=self.name_var, width=14).grid(row=row, column=0, padx=2, pady=1)
        self.source_var = tk.StringVar(value=data["source"])
        ttk.Combobox(
            table, textvariable=self.source_var, values=BOSS_SOURCES, width=12, state="readonly"
        ).grid(row=row, column=1, padx=2)
        self.hue = FloatEntry(table, data["hue"])
        self.hue.grid(row=row, column=2, padx=2)
        self.sat = FloatEntry(table, data["sat"])
        self.sat.grid(row=row, column=3, padx=2)
        self.val = FloatEntry(table, data["val"])
        self.val.grid(row=row, column=4, padx=2)

    def to_dict(self):
        return {
            "name": self.name_var.get().strip() or "boss",
            "source": self.source_var.get(),
            "hue": self.hue.get_float(),
            "sat": self.sat.get_float(),
            "val": self.val.get_float(),
        }


class PaletteStudio(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Maiya Palette Studio")
        self.geometry("640x680")
        self.minsize(560, 520)
        self.output_queue = queue.Queue()

        valley_cfg, boss_cfg = bca.load_palette_config()
        self.valley_rows = []
        self.boss_rows = []

        self._build_widgets(valley_cfg, boss_cfg)
        self.after(100, self._drain_queue)

    def _build_widgets(self, valley_cfg, boss_cfg):
        notice = ttk.Label(
            self,
            text="Hue is a shift in degrees (-180..180); Sat/Val are multipliers on the\n"
                 "source artwork's saturation and brightness (1.0 = unchanged).",
            justify="left",
        )
        notice.pack(anchor="w", padx=10, pady=(10, 4))

        ttk.Label(self, text="Per-valley enemy recolour (applies to slime/beetle/crow/goblin/worm/robot)",
                  font=("", 10, "bold")).pack(anchor="w", padx=10, pady=(8, 2))
        valley_frame = ttk.Frame(self, padding=(10, 0))
        valley_frame.pack(fill="x")
        for col, label in enumerate(("Valley name", "Hue", "Sat", "Val")):
            ttk.Label(valley_frame, text=label).grid(row=0, column=col, padx=2)
        for i, entry in enumerate(valley_cfg, start=1):
            self.valley_rows.append(ValleyRow(valley_frame, i, entry))

        ttk.Label(self, text="Boss recolours (new boss name reusing another boss's art)",
                  font=("", 10, "bold")).pack(anchor="w", padx=10, pady=(14, 2))
        boss_frame = ttk.Frame(self, padding=(10, 0))
        boss_frame.pack(fill="x")
        for col, label in enumerate(("New name", "Source boss", "Hue", "Sat", "Val")):
            ttk.Label(boss_frame, text=label).grid(row=0, column=col, padx=2)
        self.boss_frame = boss_frame
        for i, entry in enumerate(boss_cfg, start=1):
            self.boss_rows.append(BossRow(boss_frame, i, entry))

        ttk.Button(self, text="+ Add boss recolour", command=self._add_boss_row).pack(anchor="w", padx=10, pady=(4, 0))

        button_row = ttk.Frame(self, padding=10)
        button_row.pack(fill="x")
        ttk.Button(button_row, text="Save", command=self._save).pack(side="left")
        ttk.Button(button_row, text="Save + Rebuild Art", command=self._save_and_rebuild).pack(side="left", padx=6)

        ttk.Label(self, text="Build output:").pack(anchor="w", padx=10)
        self.log = tk.Text(self, height=12, wrap="word", state="disabled", bg="#111", fg="#ddd")
        self.log.pack(fill="both", expand=True, padx=10, pady=(0, 10))

    def _add_boss_row(self):
        row = len(self.boss_rows) + 1
        self.boss_rows.append(
            BossRow(self.boss_frame, row, {"name": "newboss", "source": BOSS_SOURCES[0], "hue": 0.0, "sat": 1.0, "val": 1.0})
        )

    def _collect(self):
        try:
            valley_cfg = [row.to_dict() for row in self.valley_rows]
            boss_cfg = [row.to_dict() for row in self.boss_rows]
        except ValueError:
            messagebox.showerror("Invalid value", "Hue/Sat/Val fields must be numbers.")
            return None
        return valley_cfg, boss_cfg

    def _save(self):
        collected = self._collect()
        if collected is None:
            return False
        valley_cfg, boss_cfg = collected
        bca.PALETTE_CONFIG_PATH.parent.mkdir(parents=True, exist_ok=True)
        bca.PALETTE_CONFIG_PATH.write_text(
            json.dumps({"valley_tint": valley_cfg, "reused_boss_tint": boss_cfg}, indent=2) + "\n",
            encoding="utf-8",
        )
        self._log(f"Saved {bca.PALETTE_CONFIG_PATH}\n")
        return True

    def _log(self, text):
        self.log.configure(state="normal")
        self.log.insert("end", text)
        self.log.see("end")
        self.log.configure(state="disabled")

    def _drain_queue(self):
        try:
            while True:
                self._log(self.output_queue.get_nowait())
        except queue.Empty:
            pass
        self.after(100, self._drain_queue)

    def _save_and_rebuild(self):
        if not self._save():
            return
        argv = make_argv("art", "GAME=maiya")
        self._log("\n$ " + " ".join(argv) + "\n")

        def worker():
            process = subprocess.Popen(
                argv, cwd=REPO_ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1
            )
            for line in process.stdout:
                self.output_queue.put(line)
            process.wait()
            self.output_queue.put(f"\n(exit code {process.returncode})\n")

        threading.Thread(target=worker, daemon=True).start()


def main():
    app = PaletteStudio()
    app.mainloop()


if __name__ == "__main__":
    main()
