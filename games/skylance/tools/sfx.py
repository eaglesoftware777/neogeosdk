"""Author and encode deterministic aviation effects without borrowed recordings."""
import hashlib
import json
from pathlib import Path
import sys
import wave

import numpy as np

GAME = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(GAME.parents[1] / "sound/tools"))
from adpcm_enc import encode_adpcma, adpcma_decode_nibble

RATE = 18519
NAMES = ("pickup", "armor_hit", "confirm", "rook_cannon", "aircraft_explosion",
         "ricochet", "missile_launch", "shield_break", "ground_destruction",
         "super_release", "stage_clear", "radar_warning", "boss_critical",
         "kira_cannon", "blaze_cannon", "upgrade")


def tone(t, start, end):
    duration = max(t[-1], 1 / RATE)
    return np.sin(2 * np.pi * (start * t + (end - start) * t * t / (2 * duration)))


def synth(index):
    duration = (0.19, 0.12, 0.17, 0.055, 0.42, 0.10, 0.25, 0.18,
                0.33, 0.46, 0.40, 0.26, 0.23, 0.045, 0.065, 0.28)[index]
    t = np.arange(round(duration * RATE)) / RATE
    rng = np.random.default_rng(779000 + index)
    noise = rng.uniform(-1, 1, len(t))
    smoothed = np.convolve(noise, np.ones(9) / 9, mode="same")
    if index in (0, 2, 10, 15):
        frequencies = (660, 880, 1100) if index != 10 else (523.25, 659.25, 783.99)
        segment = np.minimum(2, (t * 3 / duration).astype(int))
        frequency = np.take(frequencies, segment)
        signal = np.sin(2 * np.pi * np.cumsum(frequency) / RATE) * 0.7
        signal += np.sin(4 * np.pi * np.cumsum(frequency) / RATE) * 0.12
    elif index in (3, 13, 14):
        start, end = ((980, 180), (1900, 650), (450, 70))[(3, 13, 14).index(index)]
        signal = tone(t, start, end) * 0.65 + noise * 0.18 + smoothed * 0.30
        signal *= np.exp(-t * 36)
    elif index in (4, 8, 9, 12):
        signal = smoothed * 1.3 + noise * 0.17 + tone(t, 105, 35) * 0.48
        signal *= np.exp(-t * (7 if index == 9 else 12))
        if index == 9:
            signal += tone(t, 220, 1500) * 0.22 * np.sin(np.pi * t / duration)
    elif index == 6:
        signal = smoothed * 1.2 + tone(t, 160, 820) * 0.35
        signal *= np.sin(np.pi * t / duration) ** 2
    elif index == 11:
        signal = tone(t, 740, 740) * 0.6 * (0.5 + 0.5 * np.sin(2 * np.pi * 12 * t))
    else:
        signal = tone(t, 2300, 350) * 0.45 + noise * 0.28
        signal *= np.exp(-t * 24)
    signal -= signal.mean()
    fade = min(round(0.004 * RATE), len(t) // 2)
    signal[:fade] *= np.linspace(0, 1, fade)
    signal[-fade:] *= np.linspace(1, 0, fade)
    signal *= 0.68 / max(np.abs(signal).max(), 1e-9)
    return np.rint(signal * 32767).astype(np.int16)


def wav(path, samples):
    with wave.open(str(path), "wb") as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(RATE)
        output.writeframes(np.asarray(samples, dtype="<i2").tobytes())


def main():
    sources = GAME / "sound/samples/in_wav_a"
    encoded = GAME / "sound/samples/out_a"
    decoded = GAME / "sound/diagnostics/decoded"
    for folder in (sources, encoded, decoded):
        folder.mkdir(parents=True, exist_ok=True)
    report = []
    for index, name in enumerate(NAMES, 1):
        pcm = synth(index - 1)
        source = sources / f"{index}.wav"
        target = encoded / f"{index}.adpcma"
        wav(source, pcm)
        data, _ = encode_adpcma(pcm.tolist())
        assert len(data) % 256 == 0 and len(data) <= 0x100000
        target.write_bytes(data)
        state, restored = [0, 0], []
        for value in data:
            for nibble in (value >> 4, value & 15):
                adpcma_decode_nibble(state, nibble)
                restored.append(state[0] * 16)
        restored = np.array(restored, dtype=np.int16)
        wav(decoded / f"{index}_{name}.wav", restored)
        report.append({"id": index, "name": name, "source": str(source.relative_to(GAME)),
                       "creator": "Eagle Software", "method": "Procedural synthesis in tools/sfx.py",
                       "sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
                       "encoded_sha256": hashlib.sha256(data).hexdigest(), "bytes": len(data),
                       "peak": int(np.abs(pcm.astype(np.int32)).max()),
                       "rms": float(np.sqrt(np.mean(pcm.astype(float) ** 2))),
                       "dc": float(pcm.mean()), "clipped_samples": int((np.abs(pcm.astype(np.int32)) >= 32767).sum()),
                       "decoded_peak": int(np.abs(restored.astype(np.int32)).max()),
                       "decoded_rms": float(np.sqrt(np.mean(restored.astype(float) ** 2))),
                       "decoded_end": int(restored[-1])})
    (GAME / "sound/diagnostics/effects.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"Encoded {len(report)} original effects: {sum(r['bytes'] for r in report)} bytes")


if __name__ == "__main__":
    main()
