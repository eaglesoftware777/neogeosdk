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
| `soundStopMusic()` | Stop music / SSG only; leaves SFX alone. |
| `soundCancelFade()` | Abort a fade and restore the stored base volumes. |
| `soundSceneReset()` | The one to call on a scene change — see §5. |

### Playback

| Call | Plays |
|---|---|
| `playMusic(track)` | MML music track (`SOUND_MUSIC_A`…`_H`, 8 tracks) |
| `playSFX(n)` | ADPCM-A one-shot (`SOUND_SFX_1`…`_12`, plus the voice bank) |
| `playSFXB(n)` | ADPCM-B stream (`SOUND_TRACK_*`, 9 tracks; TRACK 4 is reserved for the eyecatcher) |
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
soundFadeOut();            soundFadeIn();
soundFadeOutSpeed(speed);  soundFadeInSpeed(speed);
```

### Chip-level controls

| Call | YM2610 register |
|---|---|
| `soundSetADPCMBPan(pan)` | `$11` L/R pan — `$C0` both, `$80` left, `$40` right, `$00` mute |
| `soundFMSetLFO(rateEn)` | `$22` LFO — bit 3 enables, bits 0–2 set the rate |
| `soundSetSSGNoise(period)` | `$06` noise period, 5 bits |

The LFO register is global to the chip, so patch changes deliberately leave
it alone: a `soundFMSetLFO()` setting survives every patch switch and
note-on.

### CSM — Composite Sine Mode

CSM auto-keys FM channel 2 from Timer A, which is how you get formant and
vowel-slide effects out of an FM chip.

```c
soundFMCSMBegin(period_hi);          /* driver cmd $1B — reg $24/$25/$27 */
soundFMCSMSweep(from, to, ms);       /* slide the formant, then end CSM   */
soundFMCSMEnd();                     /* driver cmd $1C — reg $27 = $00    */
```

Pair CSM with an active `playFMTrack()` so channel 2 has a patch to be keyed
with. `soundStopAll()` ends CSM too, so the mode cannot bleed into the next
scene.

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
| `$0F` | Stop music / SSG only |
| `$11` | Cancel fade, restore base volumes |
| `$12` *n* | Fade-in speed |
| `$13` *n* | FM volume (low nibble, 0–15) |
| `$14` *n* | SSG preset index (low nibble, 0–15) |
| `$16` *n* | ADPCM-A sample by full index — used for sample ids ≥ 64 |
| `$1B` *n* | CSM begin, Timer-A high byte |
| `$1C` | CSM end |
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
| `sound/fm/*.fm` | `fm_compile.py` | `fm` | `fm_data.inc` |
| `sound/fm/patches.fm` | `fm_patch_compile.py` | `fmpatches` | `fm_patch_table.inc` |
| `sound/ssg/*` | `ssg_compile.py` | `ssg` | `ssg_data.inc` |
| `sound/ssg/` config | `ssg_config_compile.py` | `ssgconfig` | `ssg_config.inc` |
| `sound/samples/in_wav_a/*.wav` | `adpcm_enc.py` | `samples` | ADPCM-A, `sample_table.inc` |
| `sound/samples/in_wav_b/*.wav` | `adpcm_enc.py` | `samples` | ADPCM-B |
| `sound/samples/in_wav_a_voice/*.wav` | `vrom.py` | `vrom` | V-ROM + `sdk/sound_voice_ids.h` |
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

Chapter 23 of the demo ROM (`SOUND`) plays through the mix: MML music via
`soundPlayGameLoop(SOUND_MUSIC_E)`, SFX triggers, FM, and SSG in sequence.
Chapter 24 (`SKY LANCE`) drives a pure-SSG mix under gameplay, with every
Z80 command spaced by `waitVbl()` so multi-step setup applies cleanly. See
[`DEMO_CHAPTERS.md`](./DEMO_CHAPTERS.md).
