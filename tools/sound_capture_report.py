#!/usr/bin/env python3
"""Check sound_capture output against clock, mixer and sample contracts."""

import argparse
import csv
import json
from pathlib import Path
import wave

import numpy as np


def inspect(directory):
    with (directory / "commands.tsv").open() as stream:
        commands = list(csv.DictReader(stream, delimiter="\t"))
    starts, ends = {}, {}
    for command in commands:
        starts.setdefault(command["event"], float(command["time"]))
        ends[command["event"]] = float(command["time"])
    with (directory / "ym.tsv").open() as stream:
        writes = [(float(row["time"]), int(row["port"]),
                   int(row["register"], 16), int(row["value"], 16))
                  for row in csv.DictReader(stream, delimiter="\t")]
    with wave.open(str(directory / "sound.wav")) as wav:
        rate = wav.getframerate()
        if wav.getsampwidth() != 2:
            raise ValueError("expected a 16-bit MAME WAV capture")
        pcm = np.frombuffer(wav.readframes(wav.getnframes()), dtype="<i2")
        pcm = pcm.reshape(-1, wav.getnchannels()).astype(np.float64) / 32768

    errors, report = [], {}

    def check(condition, message):
        if not condition:
            errors.append(message)

    def between(first, last, port, reg):
        return [(t, v) for t, p, r, v in writes
                if p == port and r == reg and starts[first] + 0.2 < t < starts[last]]

    ticks = [t for t, _ in between("fm_120", "fm_90", 0, 0x27)]
    irq_period = float(np.median(np.diff(ticks))) if len(ticks) > 1 else 0
    report["timer_hz"] = 1 / irq_period if irq_period else 0
    check(abs(report["timer_hz"] - 124.0079365) < 0.1, "wrong Timer-B frequency")

    for name, next_name, bpm in (("fm_120", "fm_90", 120),
                                ("fm_90", "fm_150", 90),
                                ("fm_150", "lfo_off", 150)):
        keys = [t for t, value in between(name, next_name, 0, 0x28) if value == 0xF1]
        check(len(keys) >= 10, f"too few notes in {name}")
        intervals = np.diff(keys)
        if len(intervals):
            sixteenth = 60 / bpm / 4
            error = np.abs(intervals - np.round(intervals / sixteenth) * sixteenth)
            report[name + "_max_grid_error_ms"] = float(error.max() * 1000)
            check(error.max() < 0.015, f"notes drift off the tempo grid in {name}")
            if bpm == 120:
                check(np.max(np.abs(intervals - 0.25)) < 0.012,
                      "the opening eighth notes are not 120 BPM")

    for name, next_name, value in (("lfo_off", "lfo_0", 0), ("lfo_0", "lfo_4", 8),
                                   ("lfo_4", "lfo_7", 12), ("lfo_7", "ssg_120", 15)):
        values = [v for t, p, r, v in writes if p == 0 and r == 0x22
                  and ends[name] <= t < starts[next_name]]
        check(values and values[-1] == value and all(v == value for v in values),
              f"LFO override lost in {name}")
    check(any(p == 0 and r == 0xB5 and v & 7 for t, p, r, v in writes
              if starts["lfo_off"] < t < starts["ssg_120"]), "LFO patch has no pitch sensitivity")

    # Isolate sustained fundamentals, excluding attack/release. Register writes
    # alone cannot prove that the rendered pitch is modulated.
    notes, frequency_regs = [], {0xA1: 0, 0xA5: 0}
    for t, p, r, v in writes:
        if p != 0:
            continue
        if r in frequency_regs:
            frequency_regs[r] = v
        if r == 0x28 and v == 0xF1:
            high = frequency_regs[0xA5]
            fnum = frequency_regs[0xA1] | ((high & 7) << 8)
            hz = fnum * 8000000 / (144 * 2 ** (21 - (high >> 3)))
            notes.append((t, hz))
    pitch_spread = {}
    for name, next_name in (("lfo_off", "lfo_0"), ("lfo_0", "lfo_4"),
                            ("lfo_4", "lfo_7"), ("lfo_7", "ssg_120")):
        measures = []
        for (start, hz), (stop, _) in zip(notes, notes[1:]):
            stop = min(stop, starts[next_name])
            if start < ends[name] + 0.15 or stop - start < 0.72 or hz <= 0:
                continue
            signal = pcm[int(start * rate):int(stop * rate)].mean(axis=1)
            frequencies = np.fft.fftfreq(len(signal), 1 / rate)
            distance = np.abs(frequencies / hz - 1)
            # Keep only the positive-frequency fundamental and its sidebands.
            # Taper the band edge to reduce ringing from finite note windows.
            weight = np.clip((0.35 - distance) / 0.1, 0, 1)
            weight = (1 - np.cos(np.pi * weight)) * (frequencies > 0)
            analytic = np.fft.ifft(np.fft.fft(signal) * weight)
            phase = np.unwrap(np.angle(analytic))
            pitch = np.diff(phase) * rate / (2 * np.pi)
            pitch = pitch[int(0.15 * rate):-int(0.15 * rate)]
            cents = 1200 * np.log2(np.maximum(pitch, 1) / hz)
            measures.append(float(np.std(cents)))
        check(bool(measures), f"no sustained note for waveform verification in {name}")
        spread = float(np.median(measures)) if measures else 0
        pitch_spread[name] = spread
        check(spread < 3 if name == "lfo_off" else spread > 6,
              f"unexpected waveform pitch modulation in {name}")
    report["lfo_pitch_spread_cents"] = pitch_spread

    rates = {r: v for t, p, r, v in writes if p == 0 and r in (0x19, 0x1A)
             and starts["adpcmb_32k"] < t < starts["ssg_formant"]}
    check(rates == {0x19: 0x75, 0x1A: 0x93}, "wrong 32 kHz ADPCM-B Delta-N")
    voice_controls = [(t, v) for t, p, r, v in writes if p == 1 and r == 0
                      and starts["six_voices"] + 0.03 < t < starts["final_stop"]]
    check([v for _, v in voice_controls if v < 0x80] == [4, 8, 16, 32, 1, 2],
          "ADPCM-A round-robin voices did not use all six channels")
    check(all(v != 0xBF for _, v in voice_controls), "ADPCM-A voices stopped prematurely")

    for name, next_name, control in (("b_loop", "b_fade", 0x90),
                                      ("b_one_shot", "end_stop", 0x80)):
        regs = [(t, r, v) for t, p, r, v in writes if p == 0
                and starts[name] <= t < starts[next_name]]
        triggers = [(t, v) for t, r, v in regs if r == 0x10 and v & 0x80]
        check(len(triggers) == 1 and triggers[0][1] == control,
              f"unexpected ADPCM-B restart or repeat mode in {name}")
        if not triggers:
            continue
        address = {r: v for _, r, v in regs if r in (0x12, 0x13, 0x14, 0x15)}
        first_page = address[0x12] | (address[0x13] << 8)
        last_page = address[0x14] | (address[0x15] << 8)
        duration = (last_page - first_page + 1) * 512 / (0x9375 * 8000000 / (65536 * 144))
        start = triggers[0][0] + duration + 0.2
        tail = pcm[int(start * rate):int((start + 0.5) * rate)]
        rms = float(np.sqrt(np.mean(tail ** 2)))
        check(rms > 0.005 if control == 0x90 else rms < 0.0001,
              f"wrong playback after the first sample end in {name}")
        report[name + "_after_end_rms"] = rms
    fade = between("b_fade", "b_one_shot", 0, 0x1B)
    check(fade and fade[-1][1] == 0, "ADPCM-B fade did not reach silence")
    check(all(b[1] <= a[1] for a, b in zip(fade, fade[1:])), "ADPCM-B fade is not monotonic")

    # The mixed example shares a seven-bar phrase and a 112 BPM note grid.
    tones = [t for t, _ in between("mml_mix", "mml_inline_b", 0, 0)]
    check(len(tones) >= 20, "MML stopped after an inline ADPCM-A event")
    if len(tones) > 1:
        beat = 60 / 112
        intervals = np.diff(tones)
        grid_error = np.abs(intervals - np.round(intervals / beat) * beat)
        report["mml_max_grid_error_ms"] = float(grid_error.max() * 1000)
        check(grid_error.max() < 0.015, "MML notes include an extra wait tick")
    drums = [t for t, v in between("mml_mix", "mml_inline_b", 1, 0) if v < 0x80]
    check(len(drums) >= 2, "MML did not reach the drum cue in two loops")
    if len(drums) > 1:
        report["mml_loop_seconds"] = float(drums[1] - drums[0])
        check(abs(drums[1] - drums[0] - 28 * 60 / 112) < 0.02,
              "the mixed phrase does not repeat after 28 beats")
    inline_b_tones = between("mml_inline_b", "mml_stop", 0, 0)
    check(len(inline_b_tones) >= 5, "MML cursor lost after an inline ADPCM-B event")

    levels = {}
    for name, length in (("fm_120", 4), ("lfo_off", 2.5), ("ssg_120", 4),
                         ("ssg_mute", 1), ("coin", 0.8), ("voice", 1.5),
                         ("adpcmb_32k", 6), ("ssg_formant", 2), ("stop", 0.3),
                         ("fm_mute", 1), ("six_voices", 1), ("final_stop", 0.3),
                         ("mml_mix", 6), ("mml_stop", 0.3)):
        start = starts[name] + 0.4
        segment = pcm[int(start * rate):int((start + length) * rate)]
        if not segment.size:
            raise ValueError(f"missing WAV segment: {name}")
        rms, peak = float(np.sqrt(np.mean(segment ** 2))), float(np.max(np.abs(segment)))
        levels[name] = {"rms": rms, "peak": peak}
        check(peak < 0.99, f"clipping in {name}")
        if "mute" in name or "stop" in name:
            check(rms < 0.0001, f"residual audio in {name}")
        else:
            check(rms > 0.005, f"missing or near-silent playback in {name}")
    report["levels"] = levels
    report["errors"] = errors
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    report = inspect(args.directory)
    (args.directory / "checks.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"Timer B: {report['timer_hz']:.3f} Hz")
    for name, level in report["levels"].items():
        print(f"{name:16s} RMS {level['rms']:.5f}  peak {level['peak']:.5f}")
    if report["errors"]:
        raise SystemExit("\n".join(report["errors"]))
    print("Tempo, LFO, sample rate, six voices, repeat, fades, MML and mute: PASS")


if __name__ == "__main__":
    main()
