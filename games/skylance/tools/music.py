"""Render and encode the game's reproducible, independently composed music bank."""
import hashlib
import json
from pathlib import Path
import sys
import wave

import numpy as np

GAME = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(GAME.parents[1] / "sound/tools"))
from adpcm_enc import encode_adpcmb, adpcmb_decode_nibble
from pcm_metadata import write_rate

RATE = 16000
TRACKS = (
    ("squadron_signal", 132, 50, (0, 2, 3, 5, 7, 9, 10), (0, 4, 6, 4, 2, 3, 5, 4, 1, 3, 4, 6, 5, 3, 2, 0)),
    ("mountain_dawn", 138, 52, (0, 2, 3, 5, 7, 8, 10), (0, 2, 4, 5, 4, 2, 1, 3, 4, 6, 5, 3, 2, 4, 1, 0)),
    ("harbor_blockade", 144, 45, (0, 2, 3, 5, 7, 8, 10), (4, 3, 1, 0, 2, 4, 5, 3, 6, 4, 2, 1, 3, 5, 4, 0)),
    ("convoy_pursuit", 148, 48, (0, 2, 3, 5, 7, 9, 10), (0, 0, 4, 2, 5, 4, 1, 3, 2, 6, 4, 5, 3, 2, 1, 0)),
    ("reef_intercept", 136, 53, (0, 2, 4, 5, 7, 9, 10), (2, 4, 5, 6, 4, 2, 3, 1, 0, 3, 5, 4, 6, 3, 1, 0)),
    ("forest_relay", 140, 47, (0, 2, 3, 5, 7, 9, 10), (0, 3, 2, 4, 6, 5, 3, 1, 2, 5, 4, 6, 3, 4, 1, 0)),
    ("armored_front", 146, 43, (0, 1, 3, 5, 7, 8, 10), (0, 1, 4, 3, 2, 4, 6, 5, 4, 2, 1, 3, 5, 4, 1, 0)),
    ("crimson_array", 150, 46, (0, 1, 3, 5, 7, 8, 10), (4, 6, 5, 3, 1, 2, 4, 0, 3, 5, 6, 4, 2, 3, 1, 0)),
    ("return_to_base", 120, 50, (0, 2, 4, 5, 7, 9, 11), (0, 2, 4, 6, 5, 4, 2, 1, 3, 5, 4, 2, 6, 4, 1, 0)),
)


def frequency(note):
    return 440.0 * 2 ** ((note - 69) / 12)


def render(index):
    _, bpm, root, scale, theme = TRACKS[index]
    beat = 60 / bpm
    output = np.zeros(round(64 * beat * RATE), dtype=np.float64)
    rng = np.random.default_rng(779500 + index)

    def add(start, duration, signal, gain):
        offset = round(start * RATE)
        count = min(round(duration * RATE), len(output) - offset)
        if count <= 0:
            return
        t = np.arange(count) / RATE
        env = np.minimum(t / .010, 1) * np.minimum((duration - t) / .040, 1)
        output[offset:offset+count] += signal(t) * np.maximum(env, 0) * gain

    def note(start, length, pitch, gain, bass=False):
        f = frequency(pitch)
        def signal(t):
            phase = 2*np.pi*f*t
            if bass:
                # Varied bass waveforms: FM slap, sub pulse, or growl
                mod = 0.6 if index in (0, 3) else 0.35 if index in (1, 4) else 0.8
                return (np.sin(phase + mod*np.sin(phase*2)*np.exp(-t*6)) +
                        0.15*np.sin(phase*3)) * np.exp(-t*2.2)
            else:
                # Varied lead waveforms: brass, square, detuned saws, bells
                if index in (1, 5): # Sharp arcade square/saw
                    return (0.6*np.sin(phase) + 0.3*np.sin(phase*3) + 0.15*np.sin(phase*5)) * np.exp(-t*1.2)
                elif index in (2, 6): # Heavy detuned saw
                    return (0.5*np.sin(phase*1.006) + 0.5*np.sin(phase*0.994) + 0.2*np.sin(phase*2)) * np.exp(-t*1.5)
                elif index in (4, 8): # Bell/chime lead
                    return (0.7*np.sin(phase) + 0.3*np.sin(phase*4)*np.exp(-t*8)) * np.exp(-t*1.8)
                else: # Classic FM brass
                    return (np.sin(phase + 0.8*np.sin(phase*2)*np.exp(-t*4)) + 0.18*np.sin(phase*3)) * np.exp(-t*1.4)
        add(start, length, signal, gain)

    progression = (0, 5, 3, 4) if index % 2 == 0 else (0, 3, 5, 4)
    if index == 6: progression = (0, 1, 4, 5) # Dark march
    elif index == 7: progression = (0, 3, 6, 5) # Intense chromatic

    for bar in range(16):
        chord = progression[(bar // 2) % 4]
        base = root + scale[chord]
        for step in range(8):
            start = (bar*4 + step*.5)*beat
            # Bassline patterns
            bass_len = beat * (0.28 if index == 3 else 0.42)
            bass_oct = 12 if (index == 3 and step in (1, 3, 5, 7)) or (step in (3, 7)) else 0
            note(start, bass_len, base - 12 + bass_oct, 0.17, True)

            # Melody
            if step not in ((3, 7) if index % 3 == 0 else (5,)):
                degree = theme[(bar*3 + step) % 16]
                pitch = root + 12 + scale[degree] + (12 if bar in (7, 15) and step >= 6 else 0)
                note(start, beat*(.72 if step == 6 else .43), pitch, 0.14)

            # Harmony / arpeggio stabs
            if step % 2 == 0:
                f_harm = frequency(base + (7 if step == 2 else 12))
                add(start, beat*.85, lambda t, f=f_harm: np.sin(2*np.pi*f*t)*np.exp(-t*3.5), .045)

            # Hi-hat / cymbal rhythm
            hat_gain = 0.035 if index in (1, 3, 7) else 0.024
            add(start, .048, lambda t: rng.uniform(-1, 1, len(t))*np.exp(-t*90), hat_gain)

            # Kick drum: 4-on-the-floor for trance/techno, syncopated for others
            kick = (step in (0, 4)) or (index == 3 and step in (2, 6)) or (index in (2, 6, 7) and step == 7)
            if kick:
                add(start, .16, lambda t: np.sin(2*np.pi*(52*t + 8*(1-np.exp(-t*35))))*np.exp(-t*22), .24)

            # Snare drum
            snare = (step in (2, 6)) or (index == 6 and step in (3, 7)) # Military rolls on 6
            if snare:
                add(start, .13, lambda t: (rng.uniform(-1, 1, len(t))*.75 +
                    np.sin(2*np.pi*180*t)*.25)*np.exp(-t*26), .11)

    output -= output.mean()
    output *= .66/max(float(np.abs(output).max()), 1e-9)
    fade = round(.008*RATE)
    output[:fade] *= np.linspace(0, 1, fade)
    output[-fade:] *= np.linspace(1, 0, fade)
    return np.rint(output*32767).astype(np.int16)


def save_wav(path, pcm):
    with wave.open(str(path), "wb") as stream:
        stream.setnchannels(1)
        stream.setsampwidth(2)
        stream.setframerate(RATE)
        stream.writeframes(np.asarray(pcm, dtype="<i2").tobytes())


def main():
    source = GAME / "sound/samples/in_wav_b"
    bank = GAME / "sound/samples/out_b"
    diagnostic = GAME / "sound/diagnostics/music"
    for folder in (source, bank, diagnostic):
        folder.mkdir(parents=True, exist_ok=True)
    report = []
    for index, (name, bpm, _, _, _) in enumerate(TRACKS):
        pcm = render(index)
        wav = source / f"{index+1}.wav"
        save_wav(wav, pcm)
        encoded, _ = encode_adpcmb(pcm.tolist())
        target = bank / f"{index+1}.adpcmb"
        target.write_bytes(encoded)
        write_rate(target, RATE)
        state, decoded = [0, 127], []
        for value in encoded:
            for nibble in (value >> 4, value & 15):
                adpcmb_decode_nibble(state, nibble)
                decoded.append(state[0])
        decoded = np.asarray(decoded, dtype=np.int16)
        save_wav(diagnostic / f"{index+1}_{name}.wav", decoded)
        report.append(dict(id=index+1, name=name, bpm=bpm, creator="Eagle Software",
            method="Original score rendered by tools/music.py", sample_rate=RATE,
            seconds=len(pcm)/RATE, bytes=len(encoded),
            source_sha256=hashlib.sha256(wav.read_bytes()).hexdigest(),
            encoded_sha256=hashlib.sha256(encoded).hexdigest(),
            peak=int(np.abs(pcm.astype(np.int32)).max()),
            rms=float(np.sqrt(np.mean(pcm.astype(float)**2))),
            decoded_peak=int(np.abs(decoded.astype(np.int32)).max()),
            clipped_samples=int((np.abs(decoded.astype(np.int32)) >= 32767).sum()),
            decoded_boundary_delta=int(decoded[-1])-int(decoded[0])))
        print(f"{index+1}: {name}, {bpm} BPM, {len(encoded)} bytes", flush=True)
    (GAME / "sound/diagnostics/music.json").write_text(json.dumps(report, indent=2)+"\n")


if __name__ == "__main__":
    main()
