#!/usr/bin/env python3
"""Carry the playback rate alongside headerless PCM and ADPCM files."""

import argparse
import json
from pathlib import Path


def read_rate(path, fallback=16000):
    sidecar = Path(str(path) + ".json")
    rate = json.loads(sidecar.read_text())["sample_rate"] if sidecar.exists() else fallback
    if not isinstance(rate, int) or not 1 <= rate <= 55555:
        raise ValueError(f"Unsupported YM2610 sample rate: {rate}")
    return rate


def write_rate(path, rate):
    Path(str(path) + ".json").write_text(
        json.dumps({"sample_rate": rate}, indent=2) + "\n", encoding="ascii"
    )


def delta_n(rate):
    if not 1 <= rate <= 55555:
        raise ValueError(f"Unsupported YM2610 sample rate: {rate}")
    return max(1, min(65535, round(rate * 65536 * 144 / 8000000)))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path")
    parser.add_argument("--rate", type=int, required=True)
    args = parser.parse_args()
    delta_n(args.rate)
    write_rate(args.path, args.rate)
