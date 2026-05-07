#!/usr/bin/env python3

import argparse
import pathlib
import wave

import numpy as np


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


def convert_wav_to_raw(input_path, output_path, target_rate):
    with wave.open(str(input_path), "rb") as wav_file:
        channels = wav_file.getnchannels()
        sample_width = wav_file.getsampwidth()
        source_rate = wav_file.getframerate()
        frames = wav_file.readframes(wav_file.getnframes())

    samples = decode_pcm_frames(frames, sample_width)
    samples = fold_to_mono(samples, channels)
    samples = resample_linear(samples, source_rate, target_rate)

    pcm16 = np.clip(np.rint(samples * 32767.0), -32768, 32767).astype("<i2")
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(pcm16.tobytes())


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
