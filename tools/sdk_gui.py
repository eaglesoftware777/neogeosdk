#!/usr/bin/env python3
"""Desktop control panel for building, packaging, installing, and testing
every NeoGeo SDK game (current and future) without touching the command
line.

Wraps the same `make` targets and tools/mame_launcher.py helpers as
tools/game_menu.py.  The game list is read from games/*/game.mk at
startup and on demand, so a newly added game shows up with no changes
here.

Usage:
    python3 tools/sdk_gui.py
"""

import os
import queue
import subprocess
import sys
import threading
from pathlib import Path

try:
    import tkinter as tk
    from tkinter import ttk, filedialog, messagebox
except ImportError:
    sys.exit(
        "tkinter is not installed for this Python.\n"
        "Debian/Ubuntu: sudo apt-get install python3-tk\n"
        "Fedora:        sudo dnf install python3-tkinter\n"
        "Windows/macOS python.org installers include it already."
    )

sys.path.insert(0, str(Path(__file__).resolve().parent))
import mame_launcher  # noqa: E402

REPO_ROOT = Path(__file__).resolve().parent.parent
IS_WINDOWS = os.name == "nt"


def make_argv(*targets_and_vars):
    if IS_WINDOWS:
        return ["make", "-f", "MakefileWin32.mak", *targets_and_vars]
    return ["make", *targets_and_vars]


class SdkGui(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("NeoGeo SDK - Build & Test Control Panel")
        self.geometry("820x560")
        self.minsize(700, 480)

        self.output_queue = queue.Queue()
        self.busy = False

        self.game_var = tk.StringVar()
        self.platform_var = tk.StringVar(value="mvs")
        self.mame_path_var = tk.StringVar(value=mame_launcher.load_config().get("mame_path", ""))

        self._build_widgets()
        self._refresh_games()
        self.after(100, self._drain_queue)

    # ------------------------------------------------------------------
    # Layout
    # ------------------------------------------------------------------
    def _build_widgets(self):
        top = ttk.Frame(self, padding=8)
        top.pack(fill="x")

        ttk.Label(top, text="Game:").grid(row=0, column=0, sticky="w")
        self.game_combo = ttk.Combobox(top, textvariable=self.game_var, state="readonly", width=24)
        self.game_combo.grid(row=0, column=1, padx=(4, 12))
        ttk.Button(top, text="Refresh", command=self._refresh_games).grid(row=0, column=2)

        ttk.Label(top, text="Platform:").grid(row=0, column=3, padx=(16, 0))
        ttk.Radiobutton(top, text="MVS", value="mvs", variable=self.platform_var).grid(row=0, column=4)
        ttk.Radiobutton(top, text="AES", value="aes", variable=self.platform_var).grid(row=0, column=5)

        path_row = ttk.Frame(self, padding=(8, 0, 8, 8))
        path_row.pack(fill="x")
        ttk.Label(path_row, text="MAME path:").pack(side="left")
        ttk.Entry(path_row, textvariable=self.mame_path_var).pack(side="left", fill="x", expand=True, padx=6)
        ttk.Button(path_row, text="Browse...", command=self._browse_mame_path).pack(side="left")
        ttk.Button(path_row, text="Save", command=self._save_mame_path).pack(side="left", padx=(6, 0))

        actions = ttk.Frame(self, padding=8)
        actions.pack(fill="x")
        buttons = [
            ("Build game", self._build_game),
            ("Build ALL games", self._build_all),
            ("Package game (dist)", self._package_game),
            ("Package ALL games", self._package_all),
            ("Install to MAME", self._install_game),
            ("Run in MAME", self._run_game),
            ("Full pipeline", self._full_pipeline),
            ("Clean build output", self._clean),
        ]
        for i, (label, handler) in enumerate(buttons):
            ttk.Button(actions, text=label, command=handler).grid(
                row=i // 4, column=i % 4, padx=4, pady=4, sticky="ew"
            )
        for col in range(4):
            actions.columnconfigure(col, weight=1)

        log_frame = ttk.Frame(self, padding=(8, 0, 8, 8))
        log_frame.pack(fill="both", expand=True)
        self.log = tk.Text(log_frame, wrap="word", state="disabled", bg="#111", fg="#ddd")
        scrollbar = ttk.Scrollbar(log_frame, command=self.log.yview)
        self.log.configure(yscrollcommand=scrollbar.set)
        self.log.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")

        self.status_var = tk.StringVar(value="Ready.")
        ttk.Label(self, textvariable=self.status_var, anchor="w", relief="sunken").pack(fill="x", side="bottom")

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------
    def _refresh_games(self):
        games = mame_launcher.discover_games(REPO_ROOT)
        self.game_combo["values"] = games
        if games and self.game_var.get() not in games:
            self.game_var.set(games[0])

    def _browse_mame_path(self):
        chosen = filedialog.askdirectory(title="Select MAME installation folder")
        if chosen:
            self.mame_path_var.set(chosen)

    def _save_mame_path(self):
        path = self.mame_path_var.get().strip()
        if not path or not Path(path).is_dir():
            messagebox.showerror("Invalid path", f"'{path}' is not a directory.")
            return
        config = mame_launcher.load_config()
        config["mame_path"] = path
        mame_launcher.save_config(config)
        self._log(f"Saved MAME path: {path}\n")

    def _log(self, text):
        self.log.configure(state="normal")
        self.log.insert("end", text)
        self.log.see("end")
        self.log.configure(state="disabled")

    def _set_busy(self, busy, status=""):
        self.busy = busy
        self.status_var.set(status or ("Working..." if busy else "Ready."))

    def _selected_game(self):
        game = self.game_var.get()
        if not game:
            messagebox.showerror("No game selected", "Pick a game first.")
            return None
        return game

    def _mame_path_or_none(self, prompt_if_missing=True):
        path = self.mame_path_var.get().strip()
        if path and Path(path).is_dir():
            return path
        if prompt_if_missing:
            self._browse_mame_path()
            path = self.mame_path_var.get().strip()
            if path and Path(path).is_dir():
                self._save_mame_path()
                return path
        messagebox.showerror("MAME path required", "Set a valid MAME installation folder first.")
        return None

    # ------------------------------------------------------------------
    # Background task runner
    # ------------------------------------------------------------------
    def _run_in_background(self, description, func):
        if self.busy:
            messagebox.showinfo("Busy", "A task is already running; wait for it to finish.")
            return
        self._set_busy(True, description)
        self._log(f"\n=== {description} ===\n")

        def worker():
            try:
                func()
                self.output_queue.put(("done", f"{description}: finished."))
            except SystemExit as exc:
                self.output_queue.put(("done", f"{description}: {exc}"))
            except Exception as exc:  # noqa: BLE001 - surface any failure in the log
                self.output_queue.put(("done", f"{description}: ERROR: {exc}"))

        threading.Thread(target=worker, daemon=True).start()

    def _drain_queue(self):
        try:
            while True:
                kind, payload = self.output_queue.get_nowait()
                if kind == "line":
                    self._log(payload)
                elif kind == "done":
                    self._log(payload + "\n")
                    self._set_busy(False)
        except queue.Empty:
            pass
        self.after(100, self._drain_queue)

    def _stream_subprocess(self, argv):
        self.output_queue.put(("line", "$ " + " ".join(argv) + "\n"))
        process = subprocess.Popen(
            argv, cwd=REPO_ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1
        )
        for line in process.stdout:
            self.output_queue.put(("line", line))
        process.wait()
        if process.returncode != 0:
            raise RuntimeError(f"command exited with status {process.returncode}")

    # ------------------------------------------------------------------
    # Button actions
    # ------------------------------------------------------------------
    def _build_game(self):
        game = self._selected_game()
        if not game:
            return
        self._run_in_background(f"Build {game}", lambda: self._stream_subprocess(make_argv("all", f"GAME={game}")))

    def _build_all(self):
        self._run_in_background("Build ALL games", lambda: self._stream_subprocess(make_argv("all-games")))

    def _package_game(self):
        game = self._selected_game()
        if not game:
            return
        self._run_in_background(
            f"Package {game}", lambda: self._stream_subprocess(make_argv("bios-package", f"GAME={game}"))
        )

    def _package_all(self):
        self._run_in_background("Package ALL games", lambda: self._stream_subprocess(make_argv("dist-all")))

    def _install_game(self):
        game = self._selected_game()
        if not game:
            return
        mame_path = self._mame_path_or_none()
        if not mame_path:
            return

        def task():
            mame_launcher.install_game(game, mame_path)

        self._run_in_background(f"Install {game} into MAME", task)

    def _run_game(self):
        game = self._selected_game()
        if not game:
            return
        mame_path = self._mame_path_or_none()
        if not mame_path:
            return
        platform = self.platform_var.get()

        def task():
            mame_launcher.run_game(game, mame_path, platform)

        self._run_in_background(f"Run {game} ({platform}) in MAME", task)

    def _full_pipeline(self):
        game = self._selected_game()
        if not game:
            return
        mame_path = self._mame_path_or_none()
        if not mame_path:
            return
        platform = self.platform_var.get()

        def task():
            self._stream_subprocess(make_argv("all", f"GAME={game}"))
            self._stream_subprocess(make_argv("bios-package", f"GAME={game}"))
            mame_launcher.install_game(game, mame_path)
            mame_launcher.run_game(game, mame_path, platform)

        self._run_in_background(f"Full pipeline: {game}", task)

    def _clean(self):
        game = self._selected_game()
        if not game:
            return
        self._run_in_background(f"Clean {game}", lambda: self._stream_subprocess(make_argv("clean", f"GAME={game}")))


def main():
    app = SdkGui()
    app.mainloop()


if __name__ == "__main__":
    main()
