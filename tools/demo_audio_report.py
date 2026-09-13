#!/usr/bin/env python3
"""Find where the demo's music goes quiet, and what the 68000 sent just before.

Reads a tools/demo_audio_capture.py folder.  For every chapter it prints the
mean level and any stretch of silence longer than --gap seconds that begins
after the chapter's cross-fade, followed by the sound-latch bytes and YM
writes leading into that silence.  Exits nonzero with --strict when any
chapter goes silent, so it can gate a build.
"""
import argparse
import csv
import wave
from pathlib import Path

import numpy as np


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capture", type=Path)
    parser.add_argument("--threshold", type=float, default=0.003, help="RMS below this is silence")
    parser.add_argument("--gap", type=float, default=1.5, help="seconds of silence worth reporting")
    parser.add_argument("--settle", type=float, default=2.5, help="seconds after a chapter start to ignore")
    parser.add_argument("--strict", action="store_true")
    parser.add_argument("--quiet-chapters", default="1,23,26",
                        help="chapters allowed to be silent (boot, sound tour, credits)")
    args = parser.parse_args()
    allowed = {int(c) for c in args.quiet_chapters.split(",") if c}

    with wave.open(str(args.capture / "sound.wav"), "rb") as stream:
        rate, channels = stream.getframerate(), stream.getnchannels()
        pcm = np.frombuffer(stream.readframes(stream.getnframes()), "<i2").astype(np.float32) / 32768.0
    pcm = pcm.reshape(-1, channels).mean(axis=1)
    window = int(rate * 0.25)
    count = len(pcm) // window
    rms = np.sqrt((pcm[:count * window].reshape(count, window) ** 2).mean(axis=1))
    times = np.arange(count) * 0.25

    with (args.capture / "events.tsv").open(encoding="utf-8") as stream:
        events = list(csv.DictReader(stream, delimiter="\t"))
    chapters = [(float(e["time"]), int(e["value"])) for e in events if e["kind"] == "chapter"]
    others = [e for e in events if e["kind"] != "chapter"]

    failures = 0
    for index, (start, chapter) in enumerate(chapters):
        end = chapters[index + 1][0] if index + 1 < len(chapters) else times[-1]
        if chapter == 0:
            continue
        inside = (times >= start) & (times < end)
        level, stamp = rms[inside], times[inside]
        silent = level < args.threshold
        stretches, j = [], 0
        while j < len(silent):
            if not silent[j]:
                j += 1
                continue
            k = j
            while k < len(silent) and silent[k]:
                k += 1
            begin = max(stamp[j], start + args.settle)
            finish = stamp[k - 1] + 0.25
            if finish - begin >= args.gap:
                stretches.append((begin, finish))
            j = k
        text = " ".join(f"[{a - start:.1f}s..{b - start:.1f}s]" for a, b in stretches) or "-"
        flag = "" if not stretches or chapter in allowed else "  <-- SILENT"
        print(f"ch{chapter:02d} at {start:7.1f}s len {end - start:6.1f}s  level {level.mean():.4f}  silent: {text}{flag}")
        if stretches and chapter not in allowed:
            failures += 1
            for a, _b in stretches:
                print(f"    leading into the silence at {a:.1f}s:")
                for e in [e for e in others if a - 4.0 <= float(e["time"]) <= a + 0.5][-30:]:
                    print(f"      {float(e['time']):9.3f}  {e['kind']:4s} {e['value']:6s} {e.get('pc', '')}")
    if args.strict and failures:
        raise SystemExit(f"{failures} chapter(s) went silent")


if __name__ == "__main__":
    main()
