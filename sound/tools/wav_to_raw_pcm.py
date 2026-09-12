#!/usr/bin/env python3

import argparse
import math
import pathlib
import wave

import numpy as np
from pcm_metadata import write_rate


def decode_pcm_frames(frames, sample_width):
    if sample_width == 1:
        samples = np.frombuffer(frames, dtype=np.uint8).astype(np.float32)
        return (samples - 128.0) / 128.0
    if sample_width == 2:
        samples = np.frombuffer(frames, dtype="<i2").astype(np.float32)
        return samples / 32768.0
    if sample_width == 3:
        raw = np.frombuffer(frames, dtype=np.uint8).reshape(-1, 3)
        samples = (
            raw[:, 0].astype(np.int32)
            | (raw[:, 1].astype(np.int32) << 8)
            | (raw[:, 2].astype(np.int32) << 16)
        )
        sign = samples & 0x800000
        samples = samples - (sign << 1)
        return samples.astype(np.float32) / 8388608.0
    if sample_width == 4:
        samples = np.frombuffer(frames, dtype="<i4").astype(np.float32)
        return samples / 2147483648.0
    raise ValueError(f"unsupported WAV sample width: {sample_width} bytes")


def fold_to_mono(samples, channels):
    if channels == 1:
        return samples
    return samples.reshape(-1, channels).mean(axis=1)


def resample_linear(samples, source_rate, target_rate):
    if source_rate == target_rate or len(samples) == 0:
        return samples

    out_count = max(1, int(round(len(samples) * target_rate / source_rate)))
    source_positions = np.arange(len(samples), dtype=np.float64)
    target_positions = np.arange(out_count, dtype=np.float64) * (source_rate / target_rate)
    target_positions = np.clip(target_positions, 0.0, max(len(samples) - 1, 0))
    return np.interp(target_positions, source_positions, samples).astype(np.float32)


def design_lowpass(taps, cutoff, beta=8.6):
    """Linear-phase windowed-sinc low-pass, unity gain at DC.

    `cutoff` is relative to Nyquist.  The Kaiser window at beta 8.6 gives
    about 90 dB of stop-band rejection, which is far below anything a 4-bit
    ADPCM stream can reproduce, so aliasing from the rate change is never
    the audible artefact.
    """
    m = np.arange(taps, dtype=np.float64) - 0.5 * (taps - 1)
    h = cutoff * np.sinc(cutoff * m) * np.kaiser(taps, beta)
    return h / h.sum()


def resample_bandlimited(samples, source_rate, target_rate, chunk=1 << 16):
    """Polyphase rational resampling with a band-limiting FIR.

    Every output sample is a dot product of the input with one phase of a
    long windowed sinc, evaluated directly - the zero-stuffed signal a
    textbook rate changer builds is never materialised.  The signal is
    extended past both ends along the line through its first and last
    sample, so a bed that does not start or end at silence gets no click
    from the filter running off the edge.  Pure numpy: the build must not
    fail on a machine that has no scipy.
    """
    if source_rate <= 0 or target_rate <= 0:
        raise ValueError("sample rates must be positive")
    if source_rate == target_rate or len(samples) == 0:
        return samples
    if len(samples) == 1:
        return np.repeat(samples, max(1, round(target_rate / source_rate)))

    divisor = math.gcd(source_rate, target_rate)
    up, down = target_rate // divisor, source_rate // divisor
    max_rate = max(up, down)
    half_len = 10 * max_rate
    kernel = design_lowpass(2 * half_len + 1, 1.0 / max_rate) * up

    x = samples.astype(np.float64)
    n_in = len(x)
    count = max(1, round(n_in * target_rate / source_rate))
    slope = (x[-1] - x[0]) / (n_in - 1)

    # Output n sits at input position n*down/up.  The taps that reach it
    # are the inputs i with |n*down - i*up| <= half_len.
    span = 2 * half_len // up + 2
    offsets = np.arange(span)
    out = np.empty(count, dtype=np.float32)
    for start in range(0, count, chunk):
        n = np.arange(start, min(start + chunk, count))
        centre = n * down
        i_lo = -((half_len - centre) // up)               # ceil((centre - half_len) / up)
        i = i_lo[:, None] + offsets[None, :]
        k = centre[:, None] - i * up + half_len
        valid = (k >= 0) & (k <= 2 * half_len)
        coef = np.where(valid, kernel[np.clip(k, 0, 2 * half_len)], 0.0)
        # Linear extension past either end of the input.
        inside = np.clip(i, 0, n_in - 1)
        values = x[inside] + (i - inside) * slope
        out[start:start + len(n)] = (coef * values).sum(axis=1)
    return out


def convert_wav_to_raw(input_path, output_path, target_rate):
    with wave.open(str(input_path), "rb") as wav_file:
        channels = wav_file.getnchannels()
        sample_width = wav_file.getsampwidth()
        source_rate = wav_file.getframerate()
        frames = wav_file.readframes(wav_file.getnframes())

    samples = decode_pcm_frames(frames, sample_width)
    samples = fold_to_mono(samples, channels)
    samples = resample_bandlimited(samples, source_rate, target_rate)
    # A reconstruction filter can overshoot full scale around a sharp transient.
    peak = np.max(np.abs(samples)) if len(samples) else 0.0
    if peak > 1.0:
        samples = samples / peak

    pcm16 = np.clip(np.rint(samples * 32767.0), -32768, 32767).astype("<i2")
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(pcm16.tobytes())
    write_rate(output_path, target_rate)


def main():
    parser = argparse.ArgumentParser(
        description="Convert a PCM WAV file to raw 16-bit signed little-endian mono PCM."
    )
    parser.add_argument("input_path")
    parser.add_argument("output_path")
    parser.add_argument("--rate", type=int, required=True, help="Target sample rate in Hz")
    args = parser.parse_args()

    convert_wav_to_raw(pathlib.Path(args.input_path), pathlib.Path(args.output_path), args.rate)


if __name__ == "__main__":
    main()
