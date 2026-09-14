# NeoGeo SDK Desktop Studios Manual

This document is the comprehensive user manual and technical reference for the NeoGeo SDK desktop authoring suite:
- **Artbox Studio** (`artbox/artbox_studio.py`): Graphics, tile generation, palette banks, sprite composition, and C-ROM authoring.
- **Sound Studio** (`sound/sound_studio.py`): Yamaha YM2610 FM/SSG/ADPCM sound design, MML composition, and Z80 M1/V-ROM audio pipeline.

Both tools share a common architecture for workspace discovery, game switching, safe atomic document saving, and Make-delegated build pipelines.

---

## 1. Quick Start & Prerequisites

### Prerequisites

The studios require **Python 3.10+** and the following libraries:

Use the same interpreter to install dependencies and launch the studios. On
Linux/WSL, create and activate a virtual environment first:

```bash
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install PyQt6 numpy scipy Pillow pypng
```

On Windows CMD:

```bat
py -m pip install PyQt6 numpy scipy Pillow pypng
py -c "import PyQt6, numpy, scipy, PIL, png; print('Studio dependencies OK')"
py artbox\artbox_studio.py demo
py sound\sound_studio.py demo
```

Viewing/editing assets does not require the 68000 compiler. ROM builds require
GNU Make, the configured 68000 cross-toolchain, and `wla-z80`/`wlalink` for sound.
See [build installation](../README.md#installation-on-linux) and [dependencies](DEPENDENCIES.md).

### Launching the Studios

From the root directory of the SDK checkout:

```bash
# Launch Artbox Studio for default or active game
python3 artbox/artbox_studio.py

# Launch Artbox Studio targeting a specific game (e.g., skylance, demo, demo_plus)
python3 artbox/artbox_studio.py skylance

# Launch Sound Studio for default or active game
python3 sound/sound_studio.py

# Launch Sound Studio targeting a specific game
python3 sound/sound_studio.py demo
```

---

## 2. Common Studio Architecture

Both authoring environments are built upon a shared set of core services located in `tools/`:
- `tools/studio_project.py`: Project contracts, path resolution, and multi-game discovery.
- `tools/studio_widgets.py`: UI components, vertical workspace sidebar, atomic document storage, and build panel.

### Multi-Game Project Discovery (`StudioProject`)

The SDK supports multiple simultaneous games in `games/<game_name>/`. The `StudioProject` class automatically locates all game projects and resolves per-game asset hierarchies:

| Attribute / Method | Description | Fallback Location |
|---|---|---|
| `project.game` | Name of current game folder (e.g. `demo`, `skylance`) | Derived from `games/` or `CURRENT_GAME` |
| `project.game_id` | Literal decimal ROM ID (e.g. `777`, `779`) | Read from `games/<game>/game.mk` |
| `project.art_source` | Source owner's art root; PNGs under `in/`, FIX images under `infix/` | `games/<GAME_ART_FROM or game>/artbox` |
| `project.art_data` | Selected game's generated art root | `games/<game>/artbox` |
| `project.sound` | Sound owner's audio root | `games/<GAME_SOUND_FROM or game>/sound`; `sound_dir` falls back to root `sound/` |
| `project.c_rom_paths()` | First available C1/C2 pair | Selected game's artbox, then ROM directory, then demo fallback; inspect the loaded paths |
| `project.make_command(target)` | Formats the platform-specific make invocation | `make -f Makefile GAME=<game> <target>`; `MakefileWin32.mak` on Windows |

### Workspace Sidebar & Game Switching

Each studio features a unified sidebar (`mount_workspace`):
1. **Workspace Navigator**: A searchable vertical list of all tabs. Typing in the search field filters tabs dynamically.
2. **Project Toolbar**:
   - **Game Selector Dropdown**: Instantly switch between detected games (`demo`, `demo_plus`, `skylance`, etc.). When changed, all studio tabs reload their datasets for the chosen project.
   - **Reload Project Button**: Reloads all ROM buffers, manifests, patches, and source files directly from disk.
   - **Open Source Folder**: Launches your native system file explorer in the active game's art or sound directory.
   - **Studio Manual**: Opens this manual (`docs/DESKTOP_STUDIOS.md`).
3. **Status Bar**: Always indicates the active game name, ROM identifier, tile/asset counts, and source folder locations.

### Safe Document Saving (`save_document`)

The shared `save_document` helper supports safer document writes:
- **Atomic File Writing**: Edits are staged through `QSaveFile` (written to a temporary file and atomically renamed upon flush).
- **Conflict Checking (`FileSnapshot`)**: Callers that supply a snapshot compare the file's SHA-256 before saving and request confirmation if it changed. This is not a guarantee for every editor; reload before editing files changed by another session.
- **Generated ROMs**: Direct ROM edits are replaced by a subsequent art build. Keep lasting changes in source PNGs and conversion rules.

### Build Delegation (`BuildPanel`)

Instead of re-implementing makefile pipelines inside Python GUI logic, builds are strictly delegated to GNU Make via asynchronous `QProcess` executions:
- **Build Safety Checkbox**: Prevents accidental rebuilding during active team checkouts. Users must explicitly tick *"Enable builds in this checkout"* before running targets.
- **Confirmation Prompts**: Detailed confirmation dialog showing the exact targets and affected game.
- **Real-Time Streaming**: Merged stdout and stderr outputs stream in real-time to the build console.
- **Log Export**: Save the entire compilation log for debugging.

---

## 3. Artbox Studio

Artbox Studio (`artbox/artbox_studio.py`) is the primary workstation for 2D graphics conversion, sprite frame animation, hitbox tuning, and C-ROM validation.

### Neo Geo Graphics Architecture

Artbox Studio is designed around the actual hardware specifications of the Neo Geo Neo-B1 / LSPC-A2 video system:
1. **16×16 Pixel Tiles**: All Neo Geo background and sprite graphics are composed of 16×16 pixel blocks.
2. **4bpp Indexed Color**: Each pixel is represented by a 4-bit index (0–15). Index 0 is strictly transparent in the sprite hardware. Indices 1–15 correspond to colors defined in a 16-color palette bank.
3. **Hardware Palette DAC (16-bit Color Word)**:
   The Neo Geo video DAC uses a 6-bit per channel RGB architecture packed into a 16-bit word, with an inverted shared low bit in bit 15:
   ```
   Word format: [D15 | R-low | G-low | B-low | R-high(4) | G-high(4) | B-high(4)]
   where:
     low_bit = 1 - ((word >> 15) & 1)
     Red   (0..63) = (((word >> 8) & 0x0F) << 2) | (((word >> 14) & 1) << 1) | low_bit
     Green (0..63) = (((word >> 4) & 0x0F) << 2) | (((word >> 13) & 1) << 1) | low_bit
     Blue  (0..63) = (( word       & 0x0F) << 2) | (((word >> 12) & 1) << 1) | low_bit
   ```
   This is the conversion implemented by `ng_rgb` and `rgb_word` in
   `artbox/studio_assets.py`; monitor and CRT presentation can still differ.
4. **C-ROM Interleaving**: One 16x16 4bpp tile occupies 128 bytes total: 64 in C1 and 64 in C2. Use `artbox/tile_codec.py` for bitplane layout and ROM byte ordering rather than treating the data as packed pixel nibbles.
5. **Sprite Strips & Hardware Shrinking**: Neo Geo sprites are vertical columns of 16x16 tiles (up to 32 rows). Horizontal shrink has 16 hardware steps and vertical shrink has 256; scaling is discrete, not continuous.

### Workbenches & Tabs

#### 1. Asset Inspector (`AssetWorkbench`)
The primary inspection workbench for compiled assets:
- **PNG vs. C-ROM Split View**: Visually inspect the original source PNG side-by-side with the decoded C-ROM tiles to immediately identify palette quantization or edge artifacts.
- **Frame Animation Player**: Play, pause, and step through animation frames with variable FPS playback (1 to 60 FPS).
- **Display Toggles**: Toggle 16×16 tile grids, transparent magenta/dark backgrounds, and frame cropping.
- **Exporting**: Export decoded frames directly as clean PNGs or export sprite metadata as JSON.

#### 2. Tile Grid (`TileGridTab`)
Browse every 16×16 tile compiled into the game's C-ROMs:
- Visual zoom controls (1× to 8×).
- Palette bank selector (preview tiles under any of the 256 hardware palettes).
- Click any tile to inspect its tile index, hex address, and byte offset across C1/C2.

#### 3. Sprite Designer (`SpriteDesignerTab`)
Assemble multi-tile sprites and test hardware properties:
- Configure sprite strip count (width in 16px columns) and tile height (rows per strip).
- Adjust active tile rows.
- **Hardware Shrink Preview**: Test how the sprite looks under Neo Geo hardware scaling factors (zoom levels 16 to 255).

#### 4. Hitbox Editor (`HitboxEditorTab`)
Frame-by-frame hitbox and bounding box configuration:
- Define and edit **Collision Box** (green), **Hurt Box** (blue), and **Attack Box** (red).
- Drag box handles visually or set precise coordinate offsets.
- Save hitbox bounds directly into the game's animation metadata.

#### 5. Pixel Paint (`PixelPaintTab`)
Direct, in-place pixel art touchups on C-ROM tiles:
- Pencil, eraser, and color picker tools.
- Palette selector for indices 1–15.
- Edits update the live C-ROM byte buffers directly. Use **File > Save ROMs** to atomically persist modifications to `.c1` and `.c2` files.

#### 6. ROM Addresses (`RomAddressGridTab`)
Detailed tabular breakdown of the game's asset manifest:
- Asset name, starting tile index, total tile count, strip dimensions, and ROM memory range.
- Search and filter assets by name.

#### 7. Palette Manager & Manual Palette Editor
- **Palette Manager (`PaletteManagerTab`)**: View and organize 16-color palette banks allocated to assets.
- **Manual Palette Editor (`ManualPaletteEditorTab`)**: Fine-tune individual color entries using 6-bit DAC RGB sliders (values 0–63). Colors are automatically snapped to valid Neo Geo hardware DAC words.

#### 8. Asset Rules & Browser
- **Asset Rules (`AssetRulesTab`)**: Edit `assets.cfg` rules (palette sharing, tile alignment, quantization options, trimming rules) with atomic file saving.
- **Asset Browser (`AssetBrowserTab`)**: Interactive tree browser for PNG files under the selected source owner's `artbox/in/`.

#### 9. Hex Sprite Inspector & Movement Designer
- **Hex Sprite Inspector (`HexSpriteInspectorTab`)**: Deep inspection of raw 4bpp bitplane nibbles in binary and hexadecimal with live palette swapping.
- **HD Conversion & Quality Workbench (`HdCompareTab`)**: High-fidelity art conversion comparing standard `img2neo.py` with enhanced `img2neo_hd.py`.
  - **Pre-filtering Options**: Toggle CLAHE contrast enhancement, Unsharp Mask, and Bilateral edge-preserving smoothing.
  - **Dithering Modes**: Blue Noise dither (dispersed dot pattern with minimal visual banding), Floyd-Steinberg error diffusion, or None.
  - **3-Way Visual Comparison**: Side-by-side display of Original 32-bit source, Standard conversion, and HD enhanced output.
  - **Quality Metrics**: Computes PSNR (Peak Signal-to-Noise Ratio in dB), MSE (Mean Squared Error), and unique Neo Geo DAC color count.
  - **Direct Game Deployment**: Writes a selected conversion to the source art directory. Inspect its destination and preserve the original PNG before replacing it.
- **ROM Inventory (`RomInventoryTab`)**: Real-time audit of all built ROM kinds (`p1`, `m1`, `s1`, `v1`, `c1`, `c2`) across all game projects in `roms/`, including file sizes and modification timestamps.
- **Movement Designer (`MovementDesignerTab`)**: Simulate kinematic trajectories, velocity curves, and physics paths for moving sprites.

#### 10. Build & Make (`BuildPanel`)
Execute graphics build targets directly:
- `art`: Converts configured PNGs under `games/<art_owner>/artbox/in/` into interleaved C-ROMs (`<game_id>-c1.c1` and `<game_id>-c2.c2`) and generates C header manifests.
- `sfix`: Builds the S-ROM fix layer tiles (`<game_id>-s1.s1`).

---

## 4. Sound Studio

Sound Studio (`sound/sound_studio.py`) authors audio for the Yamaha YM2610 (OPNB)
and Z80 sound driver. Desktop synthesis and ADPCM auditions are previews, not
cycle-accurate hardware validation. Compile and listen in MAME or on hardware
before approving a sound for a game.

### Yamaha YM2610 Sound Architecture

The Neo Geo sound subsystem consists of an 8-bit Z80 CPU clocked at 4 MHz managing a Yamaha YM2610 sound synthesizer chip:
1. **FM Synthesis (4 Channels)**:
   - 4 operators per channel (Op1, Op2, Op3, Op4).
   - **8 Algorithms (ALG 0–7)**: Defines operator modulation routing (from single serial modulator-carrier chains to 4-operator additive synthesis).
   - **Feedback (FB 0–7)**: Operator 1 self-feedback.
   - **Stereo Panning**: Independent left/right enable per channel.
   - **Operator Parameters**: Detune/Multiplier (DT/MUL), Total Level / Attenuation (TL), Attack Rate (AR), First Decay Rate (DR), Secondary Decay / Sustain Rate (SR), Sustain Level / Release Rate (SL/RR).
2. **SSG Sound (3 Channels + Noise)**:
   - AY-3-8910 compatible programmable sound generator.
   - Three independent square wave channels (SSG A, SSG B, SSG C).
   - Hardware noise generator (period 0–31) assignable to any channel.
3. **ADPCM-A (6 Channels, Sampled SFX)**:
   - Fixed approximately 18.5 kHz sample rate (8 MHz / 432), Yamaha 4-bit ADPCM-A encoding.
   - Stored in the V-ROM (`<game_id>-v1.v1`).
   - All 6 channels can trigger and mix simultaneously for sound effects and percussion.
4. **ADPCM-B (1 Channel, Streaming Audio)**:
   - Variable-rate Yamaha ADPCM-B playback from V-ROM for voice lines and long tracks. The encoded format differs from ADPCM-A; do not substitute arbitrary OKI ADPCM files.

### Workbenches & Tabs

#### 1. FM Patches (`FMPatchTab`)
Visual FM instrument synthesizer and patch bank editor (`patches.fm`):
- **Algorithm Diagram**: Visual flow diagram updates dynamically as you change ALG (0–7).
- **Operator Sliders**: Full interactive hex spinboxes for DT, MUL, TL, AR, DR, SR, SL, and RR.
- **Instrument Library**: 10 starting-point patches to audition and tune in-game:
  - *Slap Bass* (ALG 4, punchy percussive attack)
  - *Synth Bass* (ALG 2, deep sub-bass foundation)
  - *FM Rhodes Piano* (ALG 5, bell-like electric piano)
  - *Brass Fanfare* (ALG 4, bright brass swell)
  - *Strings & Pad* (ALG 5, soft warm orchestral bed)
  - *Saw Lead* (ALG 7, 4-operator detuned lead)
  - *Crystal Bell* (ALG 1, sharp FM metallic chime)
  - *Laser Shot SFX* (ALG 3, fast downward FM frequency sweep)
  - *Explosion FX* (ALG 0, high feedback noise-like impact)
  - *FM Kick Drum* (ALG 3, punchy low-end thud)
- **Library Controls**: Audition presets directly, apply preset parameters to the current selected patch, or insert as a brand new patch.
- **Auditioning**: Play test tones via the built-in piano keyboard, or trigger C-major arpeggios and chords.
- **Waveform Display**: Synthesized audition generated in Python/NumPy; envelope, modulation, and mixing can differ from the running driver.
- **Save**: Writes changes safely to `sound/fm/patches.fm`.

#### 2. MML Composer (`MmlComposerTab`)
Full-featured Music Macro Language tracker for FM and SSG tracks:
- **Dual Mode**: Switch seamlessly between FM MML (`sound/fm/*.mml`) and SSG MML (`sound/ssg/*.mml`).
- **Syntax Highlighting**: Comments, octave directives, tempo markers, and notes are cleanly colorized.
- **Quick Snippet Toolbar**: Single-click insertion of common MML idioms:
  - `+ Header`: Standard tempo, volume, and initial octave initialization.
  - `+ Loop`: Inserts a loop template. Check the selected compiler's syntax; the FM compiler uses `~` to restart and does not accept bracket-repeat blocks.
  - `+ Drum Cue`: ADPCM drum trigger directive.
  - `+ Mix Directive`: Channel pan and volume adjustments.
- **Syntax Validator**: Real-time syntax validator verifying bracket matching, tempo bounds (1–255), volume levels (0–15), octave ranges (1–8), and valid note lengths.
- **Live Piano Roll**: As you type MML syntax, notes render immediately as colored bars on a time-pitch piano roll grid.
- **Compile All**: Directly triggers MML compilers (`fm_compile.py` / `ssg_compile.py`) to generate assembly include tables (`.inc`) for the Z80 driver.

**Standard MML Syntax:**
```text
; Example FM Lead Theme
T135 V10 I2 O4 L8
c d e f g4 a8 b > c2
```
- `T<bpm>`: Tempo in beats per minute.
- `V<vol>`: Volume level (0–15).
- `I<inst>`: Instrument / patch index.
- `O<octave>`: Octave selection (1–7).
- `L<len>`: Default note duration (4 = quarter, 8 = eighth, 16 = sixteenth).
- `<` / `>`: Step octave down / up.
- `R`: Rest.
- `~`: Restart an FM track. Dotted durations and bracket repeats are not supported by `fm_compile.py`; desktop snippets are not a substitute for compiling.

#### 3. MML Designer (`MMLDesignerTab`)
Interactive step-grid piano roll for quickly sketching melodies:
- Click grid cells (2-octave × configurable step count) to place notes.
- MML text generator automatically outputs optimized MML code with pitch, tempo, and octave changes.
- Save directly to a `.mml` file with atomic collision protection.

#### 4. SSG Presets (`SSGPresetTab`)
Edit hardware square wave configurations (`config.ssg`):
- Set Tone Mask (register 7) to enable/disable square wave tones and noise per channel.
- Independent volume control for SSG Channels A, B, and C (0x0 to 0xF).
- Adjust Noise Generator frequency period.
- **Retro SSG Library**: 7 authentic chiptune presets:
  - *Square Lead* (Ch A pure square melody)
  - *Sub Bass Pulse* (Ch A heavy low tone)
  - *Triple Harmony* (Ch A+B+C chord voicing)
  - *Noise Snare* (Ch A tone + noise crack)
  - *Hi-Hat Click* (Short noise burst)
  - *Arcade Coin* (High-frequency ping)
  - *Explosion Rumble* (Low noise rumble)
- Audition square wave presets via keyboard or preset library controls.

#### 5. ADPCM Samples (`ADPCMTab`)
Manage WAV audio assets intended for V-ROM compilation:
- Switch between **Channel A (SFX)** and **Channel B (Streaming)**.
- Browse existing samples, check audio length, bit depth, and sample rate.
- Import new WAV files with automatic staging.
- **Dual Playback Auditioning**:
  - *Play Source PCM*: Auditions the original uncompressed source audio.
  - *Audition 4-bit ADPCM Hardware Emulation*: Software audition of sample quantization, with reconstructed waveform and SNR (Signal-to-Noise Ratio). Validate the actual A/B encoder output and playback rate in the built V-ROM; the desktop label does not imply cycle-accurate YM2610 emulation.
- **V-ROM Byte Footprint**: Shows total encoded byte consumption in the V-ROM.

#### 6. Step Sequencer / Composer (`ComposerTab`)
7-channel multitrack groove box:
- Channels: FM 1–4 and SSG A–C.
- 16 or 32 steps per pattern.
- Assign note pitches, instrument patches, and volumes per track.
- Real-time pattern playback with moving playhead.
- Export groove patterns directly into MML tracks.

#### 7. YM2610 Simulator (`YM2610SimTab`)
Audition the panel's ten preview lanes concurrently. The hardware has fourteen
voices (4 FM, 3 SSG, 6 ADPCM-A, 1 ADPCM-B); the panel shows only two ADPCM-A lanes:
- Adjust pitch and instrument assignment for FM 1–4, SSG A–C, ADPCM-A 1–2, and ADPCM-B.
- Master audition button triggers synchronized synthesis across all channels.
- Output waveform visualization checks for signal clipping or balance issues.

#### 8. Live Waveform & Spectrum Analyzer (`LiveWaveformTab`)
Real-time audio monitoring:
- Generates test clips from current FM patches, SSG presets, or MML phrases.
- Animates scrolling waveform in lock-step with `QAudioSink` audio output.
- **FFT Spectrum Analyzer**: Color-graded frequency distribution (green = low bass, yellow = midrange, red = high treble) to assess harmonic balance.

#### 9. Global Audio Mixer (`AudioMixTab`)
Balance the hardware sound chips and generate driver code:
- Sliders for ADPCM-A, ADPCM-B, SSG, and FM levels (0–255).
- One-click presets: *Default Game*, *FM Showcase*, *Pure SSG*, *Stage Mix*, and *Mute*.
- Auto-generates the corresponding C function call:
  ```c
  soundApplyMix(0x30u, 0xB8u, 0x08u, 0x08u);
  ```

#### 10. ROM Inspector (`RomInspectorTab`)
Inspect compiled ROM files across all project games:
- Lists all expected Neo Geo ROM kinds:
  - `p1`: 68000 Program ROM
  - `m1`: Z80 Sound Driver ROM
  - `s1`: Fix Layer Text/HUD Tiles
  - `v1`: ADPCM Audio Sample ROM
  - `c1` / `c2`: Sprite & Background Tile ROMs
- Displays file sizes, last modified timestamps, and missing ROM warnings.

#### 11. Identifiers Reference (`DriverDefsTab`)
Side-by-side read-only reference of critical sound header files:
- `sdk/sound_ids.h`: High-level sound commands sent from 68000 CPU to Z80.
- `sound/driver/driver_defs.h`: Internal Z80 sound driver commands, register offsets, and memory maps.
- `sound/SOUND_DRIVER_GUIDE.txt`: Architectural guide to driver timing and queues.

#### 12. Build & Make (`BuildPanel`)
Run audio compilation targets without leaving the studio:
- `samples`: Re-encodes WAVs into ADPCM-A and ADPCM-B binary blocks.
- `vrom`: Packs encoded samples into the V-ROM (`<game_id>-v1.v1`).
- `fmpatches`: Compiles `patches.fm` into assembly tables.
- `mml`: Compiles the main MML music data.
- `fm` and `ssg`: Compile the separate FM and standalone SSG track banks.
- `ssgconfig`: Rebuilds SSG preset tables from `config.ssg`.
- `m1rom`: Assembles the complete Z80 M1 ROM (`<game_id>-m1.m1`).
- `sound`: Executes the complete audio build pipeline.

---

## 5. Typical Authoring Workflows

### Graphics Workflow: From Sprite Sheet to Neo Geo ROM

1. **Source Preparation**:
   Place RGBA PNGs under `games/<art_owner>/artbox/in/`, and FIX sources under
   `infix/`. Configure `assets.cfg` for screen, sprite, or background handling;
   the importer pads to tile boundaries. Preserve transparent margins and
   inspect the generated metadata instead of guessing tile addresses.
2. **Launch Artbox Studio**:
   ```bash
   python3 artbox/artbox_studio.py <game_name>
   ```
3. **Inspect and Tune Assets**:
   - Open **Asset Inspector** to verify how colors map to Neo Geo 4bpp palettes.
   - Adjust quantization rules in **Asset Rules** (`assets.cfg`) if specific colors or palette sharing need refinement.
4. **Tune Hitboxes & Animations**:
   - In **Hitbox Editor**, define collision and attack boxes across frames.
   - Play back animations to verify frame pacing and cycle smoothness.
5. **Build C-ROMs**:
   - Navigate to the **Build & Make** tab.
   - Check *"Enable builds in this checkout"*, select `art`, and click **Build selected**.
   - Review build logs to confirm tiles packed into `<game_id>-c1.c1` and `<game_id>-c2.c2`.

### Audio Workflow: From MML Composition to Z80 Driver

1. **Synthesize Instruments**:
   - Open Sound Studio:
     ```bash
     python3 sound/sound_studio.py <game_name>
     ```
   - In **FM Patches**, select an empty or existing patch, pick an algorithm (e.g., ALG 4 for punchy bass or ALG 7 for organs), adjust attack and decay rates, and audition using the piano keyboard.
   - Click **Save patches.fm**.
2. **Compose Melody in MML**:
   - In **MML Composer**, click **+ New Track**, name it (e.g. `stage1_bgm.mml`), and compose using standard MML notation.
   - Watch the **Piano Roll** update in real-time.
   - Click **Compile All** to compile the track into assembly format.
3. **Import Sound Effects**:
   - In **ADPCM Samples**, select `ADPCM-A (SFX)` and click **Add WAV Sample…** to import short 16-bit 18.5 kHz or 22 kHz WAV files (hitsparks, explosions, coin chimes).
4. **Rebuild Audio ROMs**:
   - In **Build & Make**, click **Build sound** (or run `vrom` then `m1rom`).
   - Open **ROM Inspector** to verify that `<game_id>-m1.m1` and `<game_id>-v1.v1` have refreshed timestamps and correct byte sizes.
5. **Trigger in Game Code**:
   In your 68000 C engine code:
   ```c
   playSFXB(SOUND_TRACK_B);
   playSFX(SOUND_SFX_1);
   ```
   Initialize sound once through the normal game startup first. Include
   `sdk/neogeo.h` and `sdk/sound_ids.h`; the constants above are zero-based
   sample indices, not raw Z80 command bytes. Match them to the active game's
   sample table. See [driver guide](../sound/SOUND_DRIVER_GUIDE.txt) for looping,
   stop, mixing, and scene-transition calls.

---

## 6. Best Practices & Safety Guidelines

- **Atomic Saves**: Always use the in-studio Save buttons (`save_document`). If you receive a conflict alert indicating that files were modified externally, inspect the differences before overwriting.
- **Build Coordination**: Because builds overwrite shared generated files and ROM directories, verify that no automated batch runs or peer sessions are actively rebuilding before toggling *"Enable builds in this checkout"*.
- **Color DAC Limits**: The Neo Geo hardware only supports 6 bits per channel (values 0–63) with an inverted shared low bit. Avoid relying on full 8-bit 24-bit RGB palettes when drawing in external tools; use Artbox Studio's Palette Editor to verify valid hardware colors.
- **Index 0 Transparency**: Never use color index 0 for solid pixels in sprites—it is hard-wired to transparent pass-through in the sprite rendering hardware.
