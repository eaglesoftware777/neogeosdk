# The Sound Driver

**Eagle Software · Neo Geo SDK v1.7.0**

Everything the Neo Geo makes noise with runs on a second CPU. This document
explains the architecture, the 68000-side API you actually call, the raw
command protocol underneath it, the content pipelines that feed it, and the
practices that keep audio stable across scene changes.

The exhaustive per-function argument reference lives in
[`../sound/SOUND_DRIVER_GUIDE.txt`](../sound/SOUND_DRIVER_GUIDE.txt); this
document is the map. For the composing tools see
[`SOUND_STUDIO_GUIDE.md`](./SOUND_STUDIO_GUIDE.md).

---

## 1. Architecture

```
   68000  ──write one byte──▶  $320000  ──NMI $0066──▶   Z80
     ▲                                                    │
     │                                              command FIFO
     └──read status byte───── $320000 ◀──────────    main loop
                                                          │
                                       Timer B IRQ $0038 ──┤ music engine tick
                                                          ▼
                                                      YM2610
                                          FM · SSG · ADPCM-A · ADPCM-B
```

1. The 68000 sends one-byte commands through `REG_SOUND`.
2. The Z80 receives each command by NMI at vector `$0066`.
3. The driver pushes commands into a FIFO in Z80 RAM.
4. The main Z80 loop drains the FIFO and dispatches.
5. Timer B IRQ at vector `$0038` advances the music engine.

Nothing about this is synchronous. `playSFX()` returns immediately; it has
posted a byte, not played a sound. Design around that.

### The four sound areas

| Area | Channels | Use |
|---|---|---|
| **ADPCM-A** | 6 hardware channels, one-shot | Short sound effects, voice samples |
| **ADPCM-B** | 1 stream at a time | Backing TRACKs, ambience, long cues |
| **SSG** | 3 square waves | Melody from the MML engine, plus a standalone SSG sequencer |
| **FM** | Compiled sequencer, 16-patch voice bank | Music, stings, formant effects |

### Where the code lives

| File | Role |
|---|---|
| `sound/driver/driver.asm` | The Z80 driver. Command decoder, FIFO, music engine, patch loader. |
| `sound/driver/driver.c` | Experimental C rewrite of the same driver (`make m1rom-c`). |
| `sound/m1/m1.asm` | M1 ROM wrapper the driver is assembled into. |
| `sdk/neogeo.h`, `sdk/neogeolib.c` | The 68000-side API — every function below. |
| `sdk/sound_ids.h` | Named track / SFX / FM / SSG ids. |
| `sdk/sound_voice_ids.h` | Generated voice-sample ids (`SOUND_VOICE_*`). |
| `sound/tools/*.py` | MML, FM, SSG, patch, sample and V-ROM compilers. |

## 2. The 68000-side API

Include `sdk/neogeo.h` and `sdk/sound_ids.h`. Call `soundInit()` once at
boot before anything else.

### Core control

| Call | Effect |
|---|---|
| `soundInit()` | Boot the Z80 driver and YM2610. Required once. |
| `soundReset()` | Driver soft reset. |
| `soundStopAll()` | Stop every channel, every area. |
| `soundStopMusic()` | Stop MML, SSG and FM sequences; leaves ADPCM playback alone. |
| `soundCancelFade()` | Abort a fade and restore the stored base volumes. |
| `soundSceneReset()` | The one to call on a scene change — see §5. |

### Playback

| Call | Plays |
|---|---|
| `playMusic(track)` | MML music track (`SOUND_MUSIC_A`…`_H`, 8 tracks) |
| `playSFX(n)` | ADPCM-A one-shot (`SOUND_SFX_1`…`_12`, plus the voice bank) |
| `playSFXB(n)` | ADPCM-B sample (`SOUND_TRACK_*`, 9 tracks; index 4 / `SOUND_TRACK_E` / `5.wav` is reserved for the eyecatcher) |
| `playFMTrack(n)` | Compiled FM track (`SOUND_FM_A`…`_H`, 8 tracks) |
| `playFMDebug()` | FM track 0 as a bring-up tone |
| `playSSGTrack(n)` | Standalone SSG sequence (`SOUND_SSG_A`…`_I`, 9 tracks) |
| `soundSetSSGPreset(n)` | Select one of 16 SSG timbre presets |
| `playInsertCoinSSG()` | The coin jingle |

### Scene helpers

`soundPlayDemoFM(fm_track)`, `soundPlayTitleMusic(music_track)` and
`soundPlayGameLoop(music_track)` wrap the common "stop what's playing, start
this, at the right volume" sequences. Prefer them over hand-rolled
stop-then-start pairs.

### Voice

The driver carries a generated speech bank — the letters `a`–`z`, digits,
punctuation, and whole words — encoded as ADPCM-A samples.

| Call | Effect |
|---|---|
| `speakText("READY!")` | Prefers a whole-word sample, then falls back to letters, digits and punctuation |
| `playVoiceWord(SOUND_VOICE_WORD_GAME_OVER)` | One whole-word sample |
| `playVoiceLetter(SOUND_VOICE_LETTER_A)` | One letter |
| `playVoiceNumber(n)` | A direct number sample when one exists, otherwise digit-by-digit |
| `playGetReadyVoice()`, `playAttackVoice()`, `playCoinThenReady()` | Prebuilt cues |

`speakText()` calls `soundStopMusic()` first, because speech and SSG melody
fight for the same channels.

### Volume, tempo, fades

```c
soundSetADPCMAVolume(v);   soundSetADPCMBVolume(v);
soundSetSSGVolume(v);      soundSetFMVolume(v);      /* 0..15 */
soundSetTempo(t);          soundFMSetTempo(period);  /* 1..8 */
soundFMSetBPM(120);        soundSSGSetBPM(120);      /* 1..255 BPM */
soundFadeOut();            soundFadeIn();
soundFadeOutSpeed(speed);  soundFadeInSpeed(speed);
```

### Chip-level controls

| Call | YM2610 register |
|---|---|
| `soundSetADPCMBPan(pan)` | `$11` L/R pan — `$C0` both, `$80` left, `$40` right, `$00` mute |
| `soundSetADPCMBLoop(enable)` | `$10` repeat on subsequent B starts; `0` one-shot, nonzero repeat |
| `soundFMSetLFO(rateEn)` | `$22` LFO — bit 3 enables, bits 0–2 set the rate |
| `soundSetSSGNoise(period)` | `$06` noise period, 5 bits |

The LFO register is global to the chip. Call `soundFMSetLFO()` after
`playFMTrack()`. The live setting survives volume changes, patch reloads and
track loops; starting another track or resetting releases the override.
PMS/AMS must be nonzero in the patch to make modulation audible.

### Background loops and fades

Enable repeat before starting a B sample. Playback then loops at the sample's
hardware end address, independently of scene frame rate. It does not require
a frame-count watchdog or an early software restart:

```c
soundSceneReset();
soundSetADPCMBVolume(0xB8);
soundSetTempo(1);            /* legacy fade divider, not the sample's pitch */
soundSetADPCMBLoop(1);
playSFXB(SOUND_TRACK_B);
```

The setting affects subsequent starts, not an already playing sample.
`soundStopAll()` and reset restore one-shot mode; stopping only B retains the
preference. The demo's chapter beds opt in explicitly. The existing
`soundPlayGameLoop()` helper still starts a one-shot bed despite its name.
Seamless musical phrasing also requires a source WAV with loopable endpoints.

Fade steps occur every `max(1, 255 - speed) * t / 14.4076` seconds, where `t`
is the legacy `soundSetTempo()` divider (default 3). Each step changes the
stored A/B/SSG mixer levels by 16. With divider 1, speed `0xFE` fades a B level
of `0xB8` to zero in about 0.84 seconds. Wait for that interval before stopping
the sample. The old no-argument fade default (`0x20`) is deliberately slow;
use an explicit speed for short transitions. FM volume is controlled separately.

### Precise tempo and sample conversion

The default ASM driver uses a 124.008 Hz Timer-B tick. FM, standalone SSG and
MML each maintain a fractional clock with 12 steps per quarter note. `T120`
means 120 BPM, not a rounded selection among a handful of slow tick divisors.
The live BPM functions accept 1..255, clamp zero to one, and persist across
MML loop headers until the next track starts. `soundFMSetTempo(1..8)` remains
available as the legacy raw-period API, approximately 69.4 ms per period unit.
MML duration expiry shares the FM/SSG tick rule, so sequences with equal BPM
and phrase lengths stay aligned. Inline A/B sample cues preserve the music
cursor while programming the sample's addresses.

New WAV conversions use a band-limited polyphase filter (plain numpy, no extra
package). ADPCM-A is encoded from the hardware reset predictor and its
256-byte alignment tail is encoded toward silence. Voices use their own
sample end addresses rather than a shared short timeout. ADPCM-A samples
never cross a 1 MiB page.

ADPCM-B conversions default to 32 kHz. A `<filename>.json` sidecar records
`sample_rate` for each raw/encoded file; keep it with the asset. VROM compilation
generates per-sample Delta-N values for registers `$19/$1A`. Legacy encoded
files without sidecars are treated as 16 kHz. Set `ADPCMB_RATE=16000` in the
environment before conversion for a smaller bank. Original WAV files are not
modified. Do not mix a newly packed V1 with an older M1 sample table.

```sh
make GAME=demo sound
make GAME=demo p1 hash
python3 -m unittest discover -s tests -p test_sound_tools.py
python3 tools/sound_capture.py --output ../showcase-qa/audio
python3 tools/sound_capture_report.py ../showcase-qa/audio
```

On Windows use `make -f MakefileWin32.mak` and `py` in place of `python3`.
`m1rom` packs matching V1/sample tables before assembly. Data tables are placed
contiguously below `$8000`; overlapping writes and fixed-bank overflow fail
the build. These timing and rate extensions currently target `driver.asm`;
the separately linked C driver remains experimental.

### CSM — Composite Sine Mode

CSM is an advanced existing path on the channel with register base `$32`
(`$A2/$A6` frequency). It needs its own patch; the normal sequencer writes
the other port-A channel and does not initialize the CSM voice.

```c
soundFMCSMLoadVoice(5);             /* driver cmd $1D, patch index 0..15 */
soundFMCSMBegin(period_hi);          /* driver cmd $1B — reg $24/$25/$27 */
soundFMCSMSweep(from, to, ms);       /* slide the formant, then end CSM   */
soundFMCSMEnd();                     /* driver cmd $1C, keeps Timer B alive */
```

Begin writes `$27 = $CB`; end writes `$1A`, preserving Timer B and silencing
the CSM channel. `soundStopAll()` also clears the mode. CSM synthesis was not
changed or acoustically qualified in this pass and is not part of the sound
chapter's LFO demonstration.

## 3. The raw command protocol

Use the SDK functions first. Reach for raw bytes only for a feature with no
wrapper yet — `playSoundtest(index)` in `sdk/neogeolib.c` sends one directly.

| Byte | Meaning |
|---|---|
| `$01` | Driver init |
| `$02` | BIOS eyecatcher / boot music (mapped to music track 1) |
| `$03` | Driver soft reset |
| `$04` | Stop all playback |
| `$05` *n* | ADPCM-A volume |
| `$06` *n* | ADPCM-B volume |
| `$07` *n* | SSG music volume |
| `$0A` *n* | Fade-out speed |
| `$0C` | Stop ADPCM-A only |
| `$0D` | Stop ADPCM-B only |
| `$0E` *n* | Live tempo divisor |
| `$0F` | Stop MML, SSG and FM sequences |
| `$11` | Cancel fade, restore base volumes |
| `$12` *n* | Fade-in speed |
| `$13` *n* | FM volume (low nibble, 0–15) |
| `$14` *n* | SSG preset index (low nibble, 0–15) |
| `$15` *n* | ADPCM-B pan (`$C0/$80/$40/$00`) |
| `$16` *n* | ADPCM-A sample by full index — used for sample ids ≥ 64 |
| `$17` *n* | FM LFO enable/rate (low nibble) |
| `$18` *n* | ADPCM-B repeat for subsequent starts (`0` off, `1` on) |
| `$19` *n* | SSG noise period (low 5 bits) |
| `$1A` *n* | Legacy FM step interval, 1..8 |
| `$1B` *n* | CSM begin, Timer-A high byte |
| `$1C` | CSM end |
| `$1D` *n* | CSM voice patch, 0..15 |
| `$1E` *n* | Live FM BPM, 1..255 |
| `$1F` *n* | Live standalone SSG BPM, 1..255 |
| `$20`+n | Start music track *n* |
| `$28`, `$29` | Direct trigger, ADPCM-B samples 0 and 1 |
| `$30` | FM debug tone (FM track 0) |
| `$31` *n* | FM track select |
| `$32` *n* | SSG track select |
| `$40`+n | ADPCM-A sample *n*, compact range 0–63 |
| `$80`+n | ADPCM-B sample *n* |

### Multi-byte commands need spacing

A prefix byte and its parameter are two separate NMIs on the Z80. Sending
them back to back from a tight 68000 loop can drop the second one. In the
demo's sound and shooter chapters every multi-step setup is separated by a
`waitVbl()`, and that is the pattern to copy:

```c
soundSetSSGPreset(3);
waitVbl();
playSSGTrack(SOUND_SSG_B);
```

## 4. Content pipelines

`make sound` runs all of these. Each has its own target if you only changed
one kind of asset.

| Source | Compiler | Target | Lands in |
|---|---|---|---|
| `games/<game>/sound/mml/*.mml` | `mml_compile.py` | `mml` | `music_data.inc` |
| `games/<game>/sound/fm/*.mml` | `fm_compile.py` | `fm` | `fm_data.inc` |
| `games/<game>/sound/fm/patches.fm` | `fm_patch_compile.py` | `fmpatches` | `fm_patch_table.inc` |
| `games/<game>/sound/ssg/*.mml` | `ssg_compile.py` | `ssg` | `ssg_data.inc` |
| `games/<game>/sound/ssg/config.ssg` | `ssg_config_compile.py` | `ssgconfig` | `ssg_config.inc` |
| `games/<game>/sound/samples/in_wav_a/*.wav` | WAV converter + `adpcm_enc.py` | `samples` | ADPCM-A + rate sidecars |
| `games/<game>/sound/samples/in_wav_b/*.wav` | WAV converter + `adpcm_enc.py` | `samples` | ADPCM-B + rate sidecars |
| `games/<game>/sound/samples/in_wav_a_voice/*.wav` | WAV converter + `adpcm_enc.py` | `samples` | ADPCM-A voice bank |
| encoded sample banks | `vrom.py` | `vrom` | V-ROM, `sample_table.inc`, `sdk/sound_voice_ids.h` |
| all of the above | `wla-z80` + `wlalink` | `m1rom` | `<id>-m1.m1` |

Sample numbering is positional: `playSFX(0)` is `1.wav`,
`playSFX(SOUND_SFX_7)` is `7.wav`. Renaming or inserting a file renumbers
everything after it.

The Python sample path is the default; `SOX=/path/to/sox` switches to the
SoX conversion path only if you explicitly want it.

### Two driver builds

```sh
make m1rom-asm       # assemble sound/driver/driver.asm  (the shipping driver)
make m1rom-c         # build the experimental C driver
make compare-driver  # build both and diff the ROM images
```

`driver.asm` is the driver that ships. The C version exists to be diffed
against it, and `compare-driver` is how you check a change to one against
the other.

## 5. Practices that keep audio stable

**Initialise once.** `soundInit()` in `game_boot()`, never per scene.

**Reset on scene change, do not stack stops.** `soundSceneReset()` exists
because the failure mode of hand-rolled teardown is a fade left running, a
CSM mode left set, or a volume left at zero, three scenes later.

**Space multi-byte commands.** See §3.

**Do not poll for completion.** There is no reliable "sample finished"
handshake for general use. Drive timing from your own frame counter.

**Fade, then stop.** A hard `soundStopAll()` under a playing ADPCM-B stream
is audible as a click. `soundFadeOut()` then stop reads as intentional.

**Watch the ADPCM-A channel count.** Six channels. A seventh trigger steals
one of the six, and the one it steals is whichever the driver picks — not
necessarily the least important. Budget effects the way you budget sprites.

## 6. What the demo shows

Chapter 23 of the demo ROM (`SOUND`) previews ADPCM-B beds, pan, short ADPCM-A
effects, FM 4/6, three SSG melodies, persistent LFO rates, live BPM changes,
recorded voice, formant effects and fades. The full mix uses a B bed with
sparse A cues, avoiding unrelated melodies stacked over each other.
The separate MML example pairs the warm FM bell with a quiet G-minor SSG bass
and one drum cue over a matching seven-bar phrase at 112 BPM.
Chapter 24 (`SKY LANCE`) drives a pure-SSG mix under gameplay, with every
Z80 command spaced by `waitVbl()` so multi-step setup applies cleanly. See
[`DEMO_CHAPTERS.md`](./DEMO_CHAPTERS.md).
