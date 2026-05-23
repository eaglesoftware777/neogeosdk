# Sound Studio Guide

> **v1.3.1 — new tabs**
>
> Sound Studio gained five new tabs in v1.3.1:
>
> - **Pipeline** — runs every audio build step (samples / vrom /
>   fmpatches / mml / ssg / ssgconfig / fm / m1rom) with a status pill
>   per step and a shared live log.  "Run Full Pipeline" chains them
>   sequentially and stops on the first failure.
> - **Track Browser** — tree of every audio asset in `sound/` (MML
>   scripts, SSG presets, WAV samples).  Click a file to preview its
>   content (text files inline, binaries reported by size).
> - **Audio Mix** — live mixer for `soundApplyMix(adpcma, adpcmb,
>   ssg, fm)`.  Four sliders, five preset buttons (Default game / FM
>   showcase / Pure SSG / Stage mix / Mute) and an auto-updated C call
>   you can copy into your game init.
> - **ROM Inspector** — table of every ROM kind (p1 / m1 / s1 / v1 /
>   c1 / c2) for every game folder, with size + mtime + present/absent
>   colour-coded.
> - **Identifiers** — read-only side-by-side view of `sdk/sound_ids.h`
>   and `sound/driver/driver_defs.h` so you don't have to grep when
>   wiring code or MML.
>
> The unified demo's chapter 03 (SOUND) replaces the noisy raw FM-patch
> demo with a clean MML music intro (`SOUND_MUSIC_EAGLE_FANFARE`).
> When auditioning patches through Sound Studio, prefer the
> MML/composition path over `playFMTrack(SOUND_FM_PATCH_SHOWCASE)`
> alone — the patch table doesn't carry sequencing.
>
> Sound Studio's panels map 1-to-1 to the on-ROM channels: FM (4 ch),
> SSG (3 ch), ADPCM-A (6 ch sampled), ADPCM-B (1 ch streamed).  The
> identifier headers `sdk/sound_ids.h` and `sound/driver/driver_defs.h`
> remain the source of truth for what each command does on the Z80
> side.

`sound/sound_studio.py` is a PyQt6 desktop tool for composing and previewing
NeoGeo YM2610 audio without leaving your dev environment.

```bash
python3 sound/sound_studio.py
```

**Requirements:** Python 3, PyQt6, numpy, scipy.

```bash
pip install PyQt6 numpy scipy
```

---

## Tabs

### FM Patches

Edit the FM patch bank (`sound/fm/patches.fm`).

- **Patch list** — select any of the 16 patches; click **+ New Patch** to add one.
- **Name / ALG / FB / Stereo / LFO** — top-level patch controls.
  - `ALG` (0–7): operator routing algorithm. The diagram on the right updates live.
  - `FB` (0–7): Op1 self-feedback amount.
  - `Stereo` (hex): `C0` = both channels, `80` = left, `40` = right.
  - `LFO` (hex): global LFO enable and rate.
- **Operator grid** (OP1–OP4) — each operator has seven parameters in hex:
  - `DT/MUL`: upper nibble = detune, lower nibble = frequency multiplier.
  - `TL`: total level (attenuation). `00` = loudest, `7F` = silent.
  - `AR`: attack rate (0–31). Higher = faster attack.
  - `DR`: first decay rate.
  - `SR`: sustain/second decay rate.
  - `SL/RR`: upper nibble = sustain level, lower nibble = release rate.
  - `ENV`: SSG-EG envelope type (`00` = off).
- **Piano keyboard** — click any key to synthesize and play a preview note through your speakers. The waveform display shows the simulated output.
- **Save patches.fm** — writes changes back to `sound/fm/patches.fm`.

After saving, rebuild the patch table:

```bash
make fmpatches          # Linux
nmake -f MakefileWin32.mak fmpatches   # Windows
```

---

### MML Composer

Write and preview MML melody tracks for FM and SSG channels.

- **Mode** selector — switch between FM tracks (`sound/fm/*.mml`) and SSG tracks (`sound/ssg/*.mml`).
- **Track list** — lists all `.mml` files in the selected directory.
- **Editor** — syntax-highlighted MML text editor. Comments, note commands, and tempo/volume directives are colour-coded.
- **Piano roll** — updates live as you type. Each note appears as a coloured bar at its pitch and position in time. Different instrument indices use different colours.
- **+ New Track** — creates a new `.mml` file with a starter template.
- **Save Track** — writes the editor content back to disk.
- **Compile All** — runs `fm_compile.py` or `ssg_compile.py` over all tracks and writes the output `.inc` file into `sound/driver/`.
- **Piano keyboard** (right panel) — audition individual notes using the currently simulated patch.

**MML syntax reference:**

| Command | Meaning |
|---------|---------|
| `T<n>` | Tempo in BPM (default 120) |
| `V<n>` | Volume 0–15 |
| `I<n>` | Instrument / patch index (0–15 for FM, 0–N for SSG) |
| `O<n>` | Octave 0–8 |
| `L<n>` | Default note length (1=whole, 4=quarter, 16=sixteenth) |
| `>` / `<` | Octave up / down |
| `C`–`B` | Note name |
| `C+` / `C-` | Sharp / flat |
| `R` | Rest |
| `C4.` | Dotted note (1.5× length) |
| `;comment` | Line comment |

After compiling, rebuild the M1 ROM:

```bash
make m1rom          # Linux
nmake -f MakefileWin32.mak m1rom   # Windows
```

---

### SSG Presets

Edit the SSG preset bank (`sound/ssg/config.ssg`).

- **Preset list** — all named presets; click **+ New Preset** to add one.
- **Name** — free-text preset name.
- **Tone Mask** (hex) — mixer register. `0x38` = all three SSG channels output tone, noise off.
- **Vol A / B / C** (hex, 0–F) — per-channel volume. `0` = silent, `F` = maximum.
- **Noise Freq** (hex) — noise generator period. `00` = highest pitch noise.
- **Piano keyboard** — click a key to hear the preset as a square wave at that pitch.
- **Save config.ssg** — writes the preset bank back to disk.

After saving, rebuild the SSG config and M1 ROM:

```bash
make ssgconfig m1rom
```

---

### ADPCM Samples

Browse and preview the raw WAV source files used to build the VROM.

- **Channel selector** — switch between ADPCM-A (`sound/samples/in_wav_a/`) for sound effects and ADPCM-B (`sound/samples/in_wav_b/`) for streaming music/ambient.
- **Sample list** — all `.wav` files in the selected input directory.
- **Waveform display** — shows the waveform of the selected file.
- **Info panel** — sample rate, channels, duration, frame count.
- **▶ Play** — plays the selected WAV through your speakers.
- **Add WAV Sample…** — file picker to copy a new WAV into the input directory.

After adding or replacing samples, re-encode and rebuild the VROM:

```bash
make samples vrom       # Linux
nmake -f MakefileWin32.mak samples vrom   # Windows
```

**Channel limits:**

| Channel | Simultaneous | Use case |
|---------|-------------|---------|
| ADPCM-A | 6 | Short SFX, punches, voices |
| ADPCM-B | 1 | Long loops, ambient beds, ending themes |

---

### YM2610 Simulator

Preview all channels together to hear how they blend before building a ROM.

- **Channel grid** — 10 rows: FM 1–4, SSG A–C, ADPCM-A 1–2, ADPCM-B.
  - **Note** spinbox — MIDI note number with note name suffix (e.g. `60  C4`).
  - **Instrument** spinbox — patch index for FM, preset index for SSG.
  - **Volume** spinbox — 0–15 per channel.
  - **▶ button** — play that single channel in isolation.
- **▶ Play All FM + SSG Channels** — synthesizes all FM and SSG channels simultaneously, mixes them, and plays the combined result. The waveform display shows the mix.
- **Waveform display** — shows the most recently synthesized output.

The simulator uses numpy-based FM and square-wave synthesis at 44100 Hz and plays through `QAudioSink`. It approximates YM2610 timbre — use it to tune patches and balance volumes before committing a build.

---

## Typical Workflow

**New FM instrument:**

1. Open **FM Patches** → **+ New Patch**
2. Set `ALG=7` (all carriers additive) to start
3. Lower `TL` on operators you want audible (`00`–`30`)
4. Set `AR=1F` for instant attack; adjust `DR`, `SL/RR` for the shape
5. Click piano keys to audition — iterate until it sounds right
6. **Save patches.fm** → `make fmpatches m1rom`

**New melody track:**

1. Open **MML Composer**, Mode = FM (or SSG)
2. **+ New Track** → write the melody in the editor
3. Watch the piano roll update as you type
4. **Compile All** → `make m1rom`

**New sound effect:**

1. Drop a `.wav` file into `sound/samples/in_wav_a/`
2. Open **ADPCM Samples** → confirm waveform and duration
3. `make samples vrom` to re-encode and pack

**Balance a scene:**

1. Open **YM2610 Simulator**
2. Set each channel's note and instrument to match your scene
3. **Play All** → adjust volume spinboxes until the mix feels right
4. Apply those volume values to your `soundSetFMVolume()` / `soundSetSSGVolume()` calls in `main.c`
