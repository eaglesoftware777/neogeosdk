# YM2610 Sound Chip Reference

Complete hardware reference for the Yamaha YM2610 (OPN2) used in the Neo Geo.

---

## Architecture Overview

The YM2610 provides four distinct audio subsystems accessible to the Z80 sound CPU:

| Subsystem | Channels | Format | Max Frequency |
|-----------|----------|--------|--------------|
| FM | 4 | 4-operator FM synthesis | ~18.5 kHz output |
| SSG | 3 | Square wave (AY-3-8910 compatible) | ~125 kHz tone range |
| ADPCM-A | 6 | OKI-format 4-bit ADPCM samples | ~18.5 kHz |
| ADPCM-B | 1 | OKI-format 4-bit ADPCM stream | variable rate |

All subsystems mix internally and output through a single stereo DAC.

---

## FM Synthesis

### Operator Model

Each FM channel uses 4 operators (Op1–Op4). Each operator is an FM oscillator with an independent ADSR-like envelope. Operators are connected according to the selected algorithm.

```
Operator parameters per operator (7 values in patches.fm):

  [DT/MUL]  Byte:  bits 6-4 = Detune (0-7), bits 3-0 = Multiplier (0-15)
  [TL]      Byte:  Total Level — attenuation. 0 = loudest, 0x7F = silent.
  [AR]      Byte:  Attack Rate (bits 4-0, range 0-31). Higher = faster attack.
  [DR]      Byte:  First Decay Rate (bits 4-0, range 0-31).
  [SR]      Byte:  Second Decay Rate / Sustain Rate (bits 4-0).
  [SL/RR]   Byte:  bits 7-4 = Sustain Level, bits 3-0 = Release Rate.
  [ENV]     Byte:  SSG-EG envelope type (0 = off, 8-15 = looping shapes).
```

### Envelope Stages

```
  Level
    |
    |    /\
    |   /  \        ___________
    |  /    \      /           \
    | /      \____/             \___
    |/                               \___
    +--AR---DR---SL---SR---------RR-------> Time
        Attack  Decay  Sustain  Release
```

- **AR** (Attack Rate): Rise from 0 to peak. 31 = instant, 0 = never.
- **DR** (Decay Rate): Fall from peak to sustain level.
- **SL** (Sustain Level): Level where decay rate changes to sustain rate (0 = 0dB, 15 = -93dB).
- **SR** (Sustain Rate): Slow decay while key held.
- **RR** (Release Rate): Fall from sustain on key off.

### Multiplier (MUL)

The operator frequency is `carrier_frequency × MUL`. MUL=0 applies a ½× multiplier.

| MUL | Frequency |
|-----|-----------|
| 0   | f × 0.5   |
| 1   | f × 1     |
| 2   | f × 2     |
| 4   | f × 4     |
| 8   | f × 8     |
| 12  | f × 12    |

### Detune (DT)

Fine pitch offset for creating chorus/detuning effects between operators.

| DT | Offset |
|----|--------|
| 0  | ±0 |
| 1  | +small |
| 2  | +medium |
| 3  | +large |
| 4  | ±0 |
| 5  | −small |
| 6  | −medium |
| 7  | −large |

### Feedback (FB)

Op1 modulates itself using its own output from the previous sample.

| FB | Self-modulation |
|----|-----------------|
| 0  | None |
| 1  | π/16 |
| 7  | π × 8 (heavy distortion) |

### Algorithms (ALG 0–7)

The algorithm selects the routing of modulation between operators.
`→` means "modulates". `OUT` means "contributes to audio output".

```
ALG 0:   Op1 → Op2 → Op3 → Op4 → OUT
ALG 1:  (Op1 + Op2) → Op3 → Op4 → OUT
ALG 2:  (Op1 → Op2) + Op1 → Op3 → Op4 → OUT  [Op1 feeds both 2 and 3]
ALG 3:   Op1 → Op2 → OUT; Op3 → Op4 → OUT     [2 chains]
ALG 4:   Op1 → Op2 → OUT; Op3 → Op4 → OUT     [like ALG3, different chain]
ALG 5:   Op1 → (Op2, Op3, Op4) → OUT          [1 modulator, 3 carriers]
ALG 6:   Op1 → Op2 → OUT; Op3 → OUT; Op4 → OUT
ALG 7:   Op1 + Op2 + Op3 + Op4 → OUT          [pure additive, all carriers]
```

**Choosing an algorithm:**
- ALG 0: deepest FM, most complex timbre, all 3 modulators in series
- ALG 5: thick sound — one modulator shapes three simultaneous carriers
- ALG 7: additive synthesis, richest harmonic content, like an organ

### Stereo Register (0xB4–0xB6)

Controls L/R output enable and LFO AMS/PMS depth per channel.

| Bits | Meaning |
|------|---------|
| 7    | Left output enable |
| 6    | Right output enable |
| 5-4  | AMS depth (amplitude modulation) |
| 2-0  | PMS depth (pitch/frequency modulation) |

Common values: `C0` = both channels, `80` = left only, `40` = right only.

### LFO Register (0x22)

Global LFO for vibrato and tremolo effects.

| Bits | Meaning |
|------|---------|
| 3    | LFO enable |
| 2-0  | LFO rate (0=3.98 Hz, 7=72.2 Hz) |

---

## SSG (Square Wave Generator)

Three-channel square wave generator compatible with the AY-3-8910. Each channel (A, B, C) generates a tone at the programmed pitch and volume.

### Tone Period

Pitch is set via a 12-bit period value. Lower period = higher pitch.

```
F_tone = F_clock / (64 × Period)
```

At 8 MHz clock: Period=1000 → ~125 Hz.

### Mixer Register (0x07)

Controls which channels output tone vs noise.

| Bit | Function |
|-----|---------|
| 0   | Channel A tone disable |
| 1   | Channel B tone disable |
| 2   | Channel C tone disable |
| 3   | Channel A noise enable |
| 4   | Channel B noise enable |
| 5   | Channel C noise enable |

`tone_mask=0x38` in `config.ssg` means all tones enabled, no noise.

### Volume Register

| Bits | Meaning |
|------|---------|
| 4    | Envelope enable (0 = fixed volume) |
| 3-0  | Volume (0=silent, 15=max) |

### config.ssg Format

```ini
[preset N]
name=Preset Name
tone_mask=0x38    ; mixer register value
vol_a=0A          ; channel A volume (hex 0-F)
vol_b=06          ; channel B volume
vol_c=00          ; channel C volume
noise_freq=00     ; noise period register
```

---

## ADPCM-A (Sample Playback, 6 Channels)

Plays short samples from VROM (052-v1.v1). Up to 6 channels simultaneously, each independently addressed.

### Encoding

Samples are encoded as 4-bit OKI ADPCM:
1. Source WAV → 16 kHz mono PCM (enc_wave16le_a.bat / .sh)
2. PCM → ADPCM-A (adpcm_enc_process.bat / .sh)
3. Packed into VROM by `sound/tools/vrom.py`

### Sample Table (sound/driver/sample_table.inc)

```asm
; Format: start_addr (24-bit), end_addr (24-bit) per sample
; Address units: each unit = 256 bytes in VROM
```

### SDK Usage

```c
playSFX(N);            // play ADPCM-A sample N on next free channel
soundSetADPCMAVolume(v); // master volume (0-255)
```

---

## ADPCM-B (Streaming, 1 Channel)

Plays one continuous sample from VROM. Suited for long music beds, ambient loops, and voice lines.

### Differences from ADPCM-A

| Property | ADPCM-A | ADPCM-B |
|----------|---------|---------|
| Channels | 6 | 1 |
| Use case | SFX, short | Music, ambient |
| Rate control | Fixed per chip | Variable rate register |
| Loop support | No | Yes (hardware loop bit) |

### SDK Usage

```c
playSFXB(N);             // play ADPCM-B sample N (streaming)
soundSetADPCMBVolume(v); // master volume (0-255)
soundPlayGameLoop(N);    // start looped background music via ADPCM-B
```

---

## Register Map Summary

| Address | Register |
|---------|---------|
| 0x22 | LFO control |
| 0x28 | Key On/Off (FM channels, operator mask) |
| 0x2B | DAC enable |
| 0x30–0x37 | DT/MUL (op 1–4 per channel) |
| 0x40–0x47 | TL |
| 0x50–0x57 | KS/AR |
| 0x60–0x67 | AM/DR |
| 0x70–0x77 | SR |
| 0x80–0x87 | SL/RR |
| 0x90–0x97 | SSG-EG |
| 0xA0–0xA2 | F-Number LSB (FM channels 1-3) |
| 0xA4–0xA6 | F-Number MSB + Block |
| 0xB0–0xB2 | FB/ALG |
| 0xB4–0xB6 | L/R/AMS/PMS |
| 0x100–0x1FF | Bank 2 (FM channels 4-6 mirror) |

SSG registers use addresses 0x00–0x0F (via separate write port).

---

## MML Format Reference

MML files in `sound/fm/*.mml` and `sound/ssg/*.mml` use this syntax:

| Command | Description |
|---------|-------------|
| `T<n>` | Tempo in BPM (default 120) |
| `V<n>` | Volume 0-15 (default 12) |
| `I<n>` | Instrument/patch index 0-15 |
| `O<n>` | Octave 0-8 (default 4) |
| `L<n>` | Default note length: 1=whole, 4=quarter, 16=sixteenth |
| `>` / `<` | Octave up / down |
| `C`–`B` | Note name (C, D, E, F, G, A, B) |
| `C+` / `C-` | Sharp / flat |
| `R` | Rest |
| `C4` | C with explicit length override |
| `C4.` | Dotted (1.5× length) |
| `;comment` | Comment to end of line |

### Duration in Ticks

At 48 ticks per beat, durations are:

| Length | Ticks |
|--------|-------|
| 1 (whole) | 48 |
| 2 (half) | 24 |
| 4 (quarter) | 12 |
| 8 (eighth) | 6 |
| 16 (sixteenth) | 3 |

---

## Tools

| Tool | Purpose |
|------|---------|
| `sound/tools/fm_compile.py` | Compile FM MML → `sound/driver/fm_data.inc` |
| `sound/tools/fm_patch_compile.py` | Compile `patches.fm` → `sound/driver/fm_patch_table.inc` |
| `sound/tools/ssg_compile.py` | Compile SSG MML → `sound/driver/ssg_data.inc` |
| `sound/tools/ssg_config_compile.py` | Compile `config.ssg` → `sound/driver/ssg_config.inc` |
| `sound/tools/mml_compile.py` | Compile music MML → `sound/driver/music_data.inc` |
| `sound/tools/vrom.py` | Pack ADPCM-A/B samples → `out/052-v1.v1` |
| `sound/sound_studio.py` | PyQt6 UI: FM editor, MML composer, SSG editor, ADPCM manager, YM2610 simulator |

---

## Sound Studio UI

`sound/sound_studio.py` is a PyQt6 desktop tool for working with all YM2610 subsystems without leaving your editor.

```
python3 sound/sound_studio.py
```

### Tabs

| Tab | Function |
|-----|---------|
| **FM Patches** | Edit all 4 operators, ALG/FB/LFO/Stereo, algorithm diagram, piano key preview |
| **MML Composer** | Edit FM/SSG MML with syntax highlighting, piano roll preview, one-click compile |
| **SSG Presets** | Edit tone mask, per-channel volume, noise freq, piano preview |
| **ADPCM Samples** | Browse WAV files, waveform display, playback, import |
| **YM2610 Simulator** | Configure all 10 channels (4 FM + 3 SSG + ADPCM), mixed preview playback |

Audio preview uses numpy FM/SSG synthesis at 44100 Hz output through `QAudioSink`.

---

## Quick Recipe: New FM Instrument

1. Open Sound Studio → **FM Patches** tab
2. Click **+ New Patch**, set a name
3. Set `ALG=7` (additive) to start — all 4 operators contribute to output
4. Set `TL` lower (louder) for operators you want to hear
5. Set `AR=31` (instant attack), `DR=8`, `SL=10`, `RR=3` for a plucked sound
6. Click piano keys to preview — adjust until it sounds right
7. Click **Save patches.fm** → rebuild with `make m1rom` (or `nmake -f MakefileWin32.mak m1rom`)

## Quick Recipe: New SSG Melody

1. Open Sound Studio → **MML Composer** tab, set Mode = SSG
2. Click **+ New Track**
3. Write: `T140 V12 O4 L8 C D E F G A B > C`
4. Watch the piano roll update in real-time
5. Click **Compile All** → writes `sound/driver/ssg_data.inc`
6. Rebuild M1 ROM: `make m1rom`
