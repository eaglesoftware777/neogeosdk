#!/usr/bin/env python3
"""
YM2610 ADPCM Encoder — matches real hardware decoding exactly.

ADPCM-A: OKI-style 4-bit ADPCM, 18.5kHz fixed rate, 12-bit signed accumulator.
ADPCM-B: Yamaha-style 4-bit ADPCM, variable rate, 16-bit signed accumulator.

Usage:
    python adpcm_enc.py a input.wav output.bin
    python adpcm_enc.py b input.wav output.bin

Input: WAV file or raw 16-bit signed LE PCM (auto-detected).
"""

import sys
import os
import struct
import math


# ─── ADPCM-A (OKI / MSM5205 compatible) ───────────────────────────────────

STEP_TABLE_A = [
     16,  17,  19,  21,  23,  25,  28,  31,  34,  37,
     41,  45,  50,  55,  60,  66,  73,  80,  88,  97,
    107, 118, 130, 143, 157, 173, 190, 209, 230, 253,
    279, 307, 337, 371, 408, 449, 494, 544, 598, 658,
    724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552
]
STEP_ADJ_A = [-1, -1, -1, -1, 2, 5, 7, 9]


def _adpcma_delta(step, mag):
    d = step >> 3
    if mag & 4: d += step
    if mag & 2: d += step >> 1
    if mag & 1: d += step >> 2
    return d


def adpcma_decode_nibble(state, code):
    """state = [acc, step_idx]. Mutates in place."""
    step = STEP_TABLE_A[state[1]]
    mag = code & 7
    delta = _adpcma_delta(step, mag)
    if code & 8:
        state[0] -= delta
    else:
        state[0] += delta
    if state[0] > 2047:    state[0] = 2047
    elif state[0] < -2048: state[0] = -2048
    state[1] += STEP_ADJ_A[mag]
    if state[1] < 0:  state[1] = 0
    elif state[1] > 48: state[1] = 48


def adpcma_encode_nibble(state, sample16):
    """Encode one 16-bit sample. state = [acc, step_idx]. Returns nibble 0-15."""
    target = sample16 >> 4
    diff = target - state[0]
    step = STEP_TABLE_A[state[1]]
    code = 8 if diff < 0 else 0
    diff = abs(diff)
    mag = 0
    if diff >= step:
        mag |= 4; diff -= step
    if diff >= (step >> 1):
        mag |= 2; diff -= (step >> 1)
    if diff >= (step >> 2):
        mag |= 1
    code |= mag
    adpcma_decode_nibble(state, code)
    return code


def adpcma_find_start_step(samples):
    """Scan first 50ms to find step index big enough for the largest jump."""
    if len(samples) < 2:
        return 0
    n = min(len(samples), 925)
    max_d = max(abs((samples[i] >> 4) - (samples[i-1] >> 4)) for i in range(1, n))
    for idx in range(49):
        s = STEP_TABLE_A[idx]
        if s + (s >> 1) + (s >> 2) + (s >> 3) >= max_d:
            return idx
    return 48


def adpcma_make_preroll(target_idx):
    """Build silent pre-roll nibbles that ramp step_idx from 0 to target.
    Uses alternating +7 / -7 so acc stays near zero.
    Returns a list of 4-bit nibble values."""
    nibs = []
    state = [0, 0]
    while state[1] < target_idx:
        nibs.append(0x07)          # +mag7 → step_idx += 9
        adpcma_decode_nibble(state, 0x07)
        if state[1] >= target_idx:
            break
        nibs.append(0x0F)          # -mag7 → step_idx += 9
        adpcma_decode_nibble(state, 0x0F)
    # Settle acc back toward 0 with a few quiet nibbles
    for _ in range(4):
        if state[0] > 0:
            nibs.append(0x08)      # small negative
        elif state[0] < 0:
            nibs.append(0x00)      # small positive
        else:
            nibs.append(0x00)
        adpcma_decode_nibble(state, nibs[-1])
    return nibs, state


def encode_adpcma(samples):
    """Full ADPCM-A encode with pre-roll. Returns bytearray."""
    target_idx = adpcma_find_start_step(samples)
    preroll_nibs, state = adpcma_make_preroll(target_idx)

    # Encode all audio samples
    all_nibs = list(preroll_nibs)
    for s in samples:
        all_nibs.append(adpcma_encode_nibble(state, s))

    # Pad to even
    if len(all_nibs) % 2:
        all_nibs.append(0x08)

    # Pack nibbles
    out = bytearray()
    for i in range(0, len(all_nibs), 2):
        out.append(((all_nibs[i] & 0xF) << 4) | (all_nibs[i+1] & 0xF))

    # Pad to 256-byte boundary
    r = len(out) % 256
    if r:
        out.extend(b'\x80' * (256 - r))

    return out, len(preroll_nibs)


# ─── ADPCM-B (Yamaha datasheet codec) ─────────────────────────────────────

STEP_MULT_B = [57, 57, 57, 57, 77, 102, 128, 153,
               57, 57, 57, 57, 77, 102, 128, 153]


def adpcmb_decode_nibble(state, code):
    """state = [xn, step_size]. Mutates in place."""
    mag = code & 7
    delta = (2 * mag + 1) * state[1] // 8
    if code & 8:
        state[0] -= delta
    else:
        state[0] += delta
    if state[0] > 32767:    state[0] = 32767
    elif state[0] < -32768: state[0] = -32768
    state[1] = state[1] * STEP_MULT_B[code & 0xF] // 64
    if state[1] < 127:    state[1] = 127
    elif state[1] > 24576: state[1] = 24576


def adpcmb_encode_nibble(state, sample16):
    diff = sample16 - state[0]
    code = 8 if diff < 0 else 0
    diff = abs(diff)
    if state[1] > 0:
        mag = (diff * 8 // state[1] - 1) // 2
    else:
        mag = 7
    mag = max(0, min(7, mag))
    code |= mag
    adpcmb_decode_nibble(state, code)
    return code


def encode_adpcmb(samples):
    """Full ADPCM-B encode. Returns bytearray."""
    state = [0, 127]
    nibs = []
    for s in samples:
        nibs.append(adpcmb_encode_nibble(state, s))
    if len(nibs) % 2:
        nibs.append(0x08)
    out = bytearray()
    for i in range(0, len(nibs), 2):
        out.append(((nibs[i] & 0xF) << 4) | (nibs[i+1] & 0xF))
    r = len(out) % 256
    if r:
        out.extend(b'\x80' * (256 - r))
    return out, 0


# ─── WAV / raw reader ─────────────────────────────────────────────────────

def read_input(path):
    with open(path, 'rb') as f:
        hdr = f.read(4)
        if hdr == b'RIFF':
            f.read(4); f.read(4)
            fmt = None; raw = None
            while True:
                cid = f.read(4)
                if len(cid) < 4: break
                csz = struct.unpack('<I', f.read(4))[0]
                if cid == b'fmt ':
                    fmt = f.read(csz)
                elif cid == b'data':
                    raw = f.read(csz); break
                else:
                    f.read(csz)
            if fmt is None or raw is None:
                raise ValueError("Bad WAV")
            ch   = struct.unpack('<H', fmt[2:4])[0]
            rate = struct.unpack('<I', fmt[4:8])[0]
            bits = struct.unpack('<H', fmt[14:16])[0]
            print(f"WAV: {rate}Hz {ch}ch {bits}bit")
            if bits == 16:
                n = len(raw) // 2
                samps = list(struct.unpack(f'<{n}h', raw))
            elif bits == 8:
                samps = [(b - 128) << 8 for b in raw]
            else:
                raise ValueError(f"Unsupported {bits}-bit")
            if ch == 2:
                samps = [(samps[i] + samps[i+1]) // 2
                         for i in range(0, len(samps) - 1, 2)]
            return samps, rate
        else:
            f.seek(0)
            raw = f.read()
            n = len(raw) // 2
            return list(struct.unpack(f'<{n}h', raw)), 0


# ─── Verification ─────────────────────────────────────────────────────────

def verify_adpcma(encoded, orig_samples, preroll_count):
    """Decode the encoded data and compute SNR against original."""
    state = [0, 0]
    decoded = []
    count = 0
    for byte_val in encoded:
        for shift in [4, 0]:
            code = (byte_val >> shift) & 0xF
            adpcma_decode_nibble(state, code)
            count += 1
            if count > preroll_count:
                decoded.append(state[0])

    n = min(len(orig_samples), len(decoded))
    if n == 0:
        return
    errors = [decoded[i] - (orig_samples[i] >> 4) for i in range(n)]
    sig = sum((orig_samples[i] >> 4)**2 for i in range(n)) / n
    noi = sum(e*e for e in errors) / n
    snr = 10 * math.log10(sig / noi) if noi > 0 else 999
    mx = max(abs(e) for e in errors)
    av = sum(abs(e) for e in errors) / n
    print(f"  Verify: SNR={snr:.1f}dB  max_err={mx}  avg_err={av:.1f}")


# ─── Main ─────────────────────────────────────────────────────────────────

def main():
    if len(sys.argv) < 4:
        print("YM2610 ADPCM Encoder")
        print("Usage: python adpcm_enc.py <a|b> <input> <output>")
        print("  a = ADPCM-A (18.5kHz, 12-bit, drums/SFX)")
        print("  b = ADPCM-B (variable rate, 16-bit, voice/music)")
        sys.exit(1)

    mode = sys.argv[1].lower()
    inp  = sys.argv[2]
    outp = sys.argv[3]

    if mode not in ('a', 'b'):
        print(f"Unknown mode: {mode}"); sys.exit(1)
    if not os.path.exists(inp):
        print(f"Not found: {inp}"); sys.exit(1)

    samples, rate = read_input(inp)
    if rate and mode == 'a' and rate != 18500:
        print(f"WARNING: ADPCM-A expects 18500Hz, got {rate}Hz")

    if mode == 'a':
        encoded, preroll = encode_adpcma(samples)
        print(f"Encoded {len(samples)} samples + {preroll} preroll nibbles"
              f" -> {len(encoded)} bytes")
        verify_adpcma(encoded, samples, preroll)
    else:
        encoded, preroll = encode_adpcmb(samples)
        print(f"Encoded {len(samples)} samples -> {len(encoded)} bytes")

    with open(outp, 'wb') as f:
        f.write(encoded)
    print(f"Written to {outp}")


if __name__ == "__main__":
    main()
