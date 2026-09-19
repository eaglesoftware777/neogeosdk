#!/usr/bin/env python3
"""Test EagleBIOS in MAME with headless simulation, frame capture, and audio analysis."""

import os
import sys
import wave
import struct
import argparse
import subprocess
from pathlib import Path

def analyze_audio(wav_path: Path):
    if not wav_path.exists():
        print(f"Audio file {wav_path} not found.")
        return

    with wave.open(str(wav_path), "rb") as w:
        nframes = w.getnframes()
        rate = w.getframerate()
        nch = w.getnchannels()
        raw = w.readframes(nframes)

    samples = struct.unpack(f"<{nframes * nch}h", raw)
    peak = max(abs(s) for s in samples) if samples else 0
    non_zero = sum(1 for s in samples if abs(s) > 100) if samples else 0
    duration = nframes / rate if rate > 0 else 0.0
    pct = (non_zero / len(samples) * 100.0) if samples else 0.0

    print("--- Audio Analysis ---")
    print(f"  Duration:       {duration:.2f}s ({nframes} frames @ {rate} Hz, {nch} ch)")
    print(f"  Peak Amplitude: {peak} / 32767 ({peak / 32767.0 * 100.0:.1f}%)")
    print(f"  Active Samples: {non_zero} / {len(samples)} ({pct:.1f}%)")
    if peak > 500 and non_zero > 1000:
        print("  Status:         AUDIO ACTIVE (playback verified)")
    else:
        print("  Status:         SILENT / INACTIVE")

def main():
    root = Path(__file__).resolve().parents[1]
    bios_dir = root / "bios"
    test_roms = bios_dir / "test_roms" / "neogeo"
    test_roms.mkdir(parents=True, exist_ok=True)

    # 1. Copy generated EagleBIOS sp-s2.sp1 and sm1.sm1 into test_roms
    sp1_src = bios_dir / "sp-s2.sp1"
    if not sp1_src.exists():
        print(f"Error: {sp1_src} not found. Run 'make -C bios' first.")
        sys.exit(1)
    sp1_dst = test_roms / "sp-s2.sp1"
    sp1_dst.write_bytes(sp1_src.read_bytes())

    sm1_src = bios_dir / "sm1.sm1"
    if not sm1_src.exists():
        print(f"Error: {sm1_src} not found. Run 'make -C bios' first.")
        sys.exit(1)
    sm1_dst = test_roms / "sm1.sm1"
    sm1_dst.write_bytes(sm1_src.read_bytes())

    # Copy required support ROMs (000-lo.lo, sfix.sfix)
    for sup in ["000-lo.lo", "sfix.sfix"]:
        src = root / "roms" / "neogeo" / sup
        dst = test_roms / sup
        if src.exists() and not dst.exists():
            dst.write_bytes(src.read_bytes())

    # 2. Output directory
    capture_dir = bios_dir / "test_captures"
    capture_dir.mkdir(parents=True, exist_ok=True)

    # 3. Environment
    env = os.environ.copy()
    env.update(
        DISPLAY="",
        SDL_VIDEODRIVER="dummy",
        SDL_AUDIODRIVER="dummy",
        GAME_CAPTURE_DIR=str(capture_dir),
        GAME_CAPTURE_EVERY="1.0",
        GAME_CAPTURE_PLAY="1"
    )

    parser = argparse.ArgumentParser(description="Test EagleBIOS with MAME")
    parser.add_argument("--game", default="maiya", choices=["maiya", "demo"], help="Game to test")
    parser.add_argument("--seconds", type=int, default=12, help="Seconds to run")
    parser.add_argument("--no-audio", action="store_true", help="Disable audio synthesis and analysis")
    args = parser.parse_args()

    game = args.game
    seconds = str(args.seconds)
    audio_enabled = not args.no_audio
    wav_path = capture_dir / f"{game}_test_audio.wav"

    # 4. MAME Command
    rompath = f"{bios_dir / 'test_roms'};{root / 'roms'}"
    if game == "demo":
        hashpath = f"{root / 'hash_eagle' / 'demo'};{root / 'hash_eagle'};{root / 'hash'}"
    else:
        hashpath = f"{root / 'hash_eagle' / 'maiya'};{root / 'hash_eagle'};{root / 'hash'}"

    cmd = [
        "mame", "neogeo",
        "-noreadconfig",
        "-rompath", rompath,
        "-hashpath", hashpath,
        "-bios", "euro",
        "-cart1", game,
        "-video", "none",
        "-nothrottle",
        "-seconds_to_run", seconds,
        "-skip_gameinfo",
        "-nonvram_save",
        "-cfg_directory", str(capture_dir / "cfg"),
        "-nvram_directory", str(capture_dir / "nvram"),
        "-snapshot_directory", str(capture_dir),
        "-autoboot_delay", "0",
        "-autoboot_script", str(root / "tools" / "game_capture.lua")
    ]

    if audio_enabled:
        cmd.extend(["-wavwrite", str(wav_path)])
    else:
        cmd.extend(["-sound", "none"])

    print(f"Running MAME with EagleBIOS on {game} ({seconds}s)...")
    log_file = capture_dir / "mame.log"
    with open(log_file, "w", encoding="utf-8") as log:
        res = subprocess.run(cmd, env=env, cwd=root, stdout=log, stderr=subprocess.STDOUT)

    print(f"MAME finished with exit code {res.returncode}")
    pngs = sorted(capture_dir.rglob("*.png"))
    print(f"Captured {len(pngs)} frames in {capture_dir}:")
    for p in pngs:
        print(f"  {p.name} ({p.stat().st_size} bytes)")

    if audio_enabled:
        analyze_audio(wav_path)

    if res.returncode != 0:
        print("MAME log tail:")
        lines = log_file.read_text(encoding="utf-8", errors="replace").splitlines()
        for line in lines[-20:]:
            print(" ", line)

if __name__ == "__main__":
    main()
