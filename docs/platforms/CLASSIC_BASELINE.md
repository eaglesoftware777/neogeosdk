# Classic Baseline: MVS, AES, AES+ and Neo Geo CD

The hardware facts every engine task (G-tasks, P-tasks, CD-tasks) is held to.
Classic MVS and AES are the reference; the AES+ and the Neo Geo CD are
compared against them. No code in this task.

**Sources**, named rather than linked:
- [W] the NeoGeoDev wiki, by page title (for example [W: VRAM]).
- [M] the MAME source, `src/mame/snk/` at commit `f1f39d964476`
  (`neogeo.cpp`, `neogeo.h`, `neogeo_spr.cpp/.h`, `neogeo_v.cpp`,
  `neogeocd.cpp`, `ng_memcard.cpp`).
- [C] Charles MacDonald's `mvstech.txt`, as quoted by MAME and the wiki.
- [P] public press reports on the AES+. Not verified by us.

"Unknown" means no source was found. It does not mean "no difference".

## 1. The common core (every cartridge system)

| Fact | Value | Source |
|---|---|---|
| Master clock | 24 MHz (MVS). MAME lists its AES clock as a known issue: the NTSC AES input clock is 24.167829 MHz, so an AES frame is ~59.60 Hz | [M: neogeo.h `NEOGEO_MASTER_CLOCK`, neogeo.cpp known issues] |
| 68000 | 12 MHz (master / 2) | [M: neogeo.h] |
| Z80 | 4 MHz (master / 6); YM2610 8 MHz (master / 3) | [M: neogeo.h] |
| Pixel clock | 6 MHz (master / 4, "a pixel lasts 4 mclk") | [M: neogeo.h], [W: Display timing] |
| Line | 384 px total, 320 px active (1536 mclk) = **768 68k cycles**, 64 µs | [W: Display timing], [M: neogeo_spr.h `NEOGEO_HTOTAL` 0x180] |
| Frame | 264 lines total, 224 active: **202,752 68k cycles**, 59.1856 Hz | [W: Display timing], [M: `NEOGEO_VTOTAL` 0x108, `VBEND` 0x10, `VBSTART` 0xF0] |
| Blanking | 40 lines: 8 sync + 16 top + 16 bottom border = **30,720 68k cycles** (2.56 ms), before wait states and interrupt overhead | [W: Display timing] |
| VBlank IRQ | raised 58 mclk after the start of line 0xF0 (MAME's model) | [M: neogeo.h `NEOGEO_VBLANK_IRQ_HTIM`] |
| Refresh | ~59.19 Hz (MVS), ~59.60 Hz (NTSC AES), **never 60**: every "per second" rate in the game is in frames | [W: Display timing], [M] |
| PAL timing | 384 × 312 per frame, with the borders shown | [W: Display timing]; "PAL region AES behavior is not verified" [M: neogeo.cpp] |
| Sprites | 381 per screen, 96 per scanline | [M: neogeo_spr.cpp `MAX_SPRITES_PER_SCREEN`, `MAX_SPRITES_PER_LINE`] |
| VRAM | 64 KiB lower zone (SCB1 $0000–$6FFF, FIX map $7000–$74FF) and 4 KiB upper zone (SCB2 $8000, SCB3 $8200, SCB4 $8400, sprite lists $8600) | [W: VRAM] |
| VRAM ports | `REG_VRAMADDR` $3C0000, `REG_VRAMRW` $3C0002, `REG_VRAMMOD` $3C0004 (signed step added after each write) | [W: Memory mapped registers] |
| VRAM spacing | after an address write, a read is valid after **16** CPU cycles; after a data write, a new address after **16** cycles; another data write after **12** cycles | [W: VRAM] |
| VRAM in active display | "VRAM can be modified even during active display", within the spacing above | [W: VRAM] |
| Palettes | 2 banks × 256 palettes × 16 colours; colour 0 transparent; $400000 must stay black ($8000); backdrop at $401FFE | [W: Palettes] |
| Palette in active display | the CPU has priority: a colour written during active display shows for at least one pixel, so "snow" if many change | [W: Palettes] |
| Bank select | byte write to `REG_PALBANK0` $3A000F / `REG_PALBANK1` $3A001F | [W: Palettes], [W: Memory mapped registers] |
| Shadow | `REG_SHADOW` $3A0011 / `REG_NOSHADOW` $3A0001 (system latch bit 0; palette bank is bit 7) | [W: Memory mapped registers], [M: neogeo.cpp system latch] |
| Timer IRQ | `REG_LSPCMODE` $3C0006: bit 4 enable, bit 5 reload on `REG_TIMERLOW` write, bit 6 reload at start of the first VBlank line, bit 7 reload at 0; the counter counts pixels (6 MHz); `REG_TIMERHIGH/LOW` $3C0008/$3C000A; acknowledge with `REG_IRQACK` $3C000C | [W: Timer interrupt], [W: Memory mapped registers] |
| Timer floor | the reload value must stay above 4, or the 68000 floods with interrupts and locks up | [W: Timer interrupt] |
| Timer reload point | MAME reloads at VBSTART + 1146 mclk; it notes "some raster effects are imperfect (off by a couple of lines)" in emulation | [M: neogeo.cpp `NEOGEO_VBLANK_RELOAD_HTIM`, driver notes] |
| `REG_LSPCMODE` read | bits 15–8 raster line counter; low bits auto-animation counter | [W: Memory mapped registers] |
| Auto-animation | a counter stepped every (speed + 1) frames at vsync, or held when disabled | [M: neogeo_spr.cpp `auto_animation_timer_callback`] |
| Watchdog | kicked by any write to `REG_DIPSW` $300001; resets the whole system after ~0.13 s = 3,244,030 master cycles (~8 frames). The wiki calls the exact figure unverified (other readings are 20 and 7.6 frames) | [M: neogeo.cpp notes and `WATCHDOG_TIMER`], [C], [W: Watchdog] |
| FIX | 40 × 32 cells of 8 × 8 (map at VRAM $7000) | [W: VRAM] |

## 2. MVS (arcade)

Board families, as MAME tabulates them ([M: neogeo.cpp "Known motherboards"]).
The wiki page [W: MVS board types] agrees.

| Generation | Chipset | Boards | Notes |
|---|---|---|---|
| 1 | PRO-B0 / PRO-C0 / LSPC-A0 (multi-chip, NEC) | MV-1, MV-2B, MV-4, MV-6 (also MV1T) | original full-featured boards |
| 2 | NEO-B1 / LSPC2-A2 (Fujitsu) | MV-1F, MV-1FZ(S), MV-2F(S), MV-4F(S), MV4FT(2) | MV-1F: no memory card headers; MV-1FZ: no LED displays, mahjong inputs or stereo output |
| 3 | NEO-MGA / NEO-GRC | MV-1A, MV-1ACH, MV-1AX | 1995; removes coin lockouts; MV-1AX has a soldered BIOS |
| 4 | NEO-GRC2 | MV-1B, MV1B1 | soldered BIOS, no SM1 ROM |
| 5 | NEO-GRZ | MV-1C | 1999; the final board, vertical slot |

- **1 slot and multi-slot.** Multi-slot boards run the BIOS game-select
  path: the player picks among 2, 4 or 6 cartridges, and the BIOS switches
  the slot. The system ROM, sound path and backup RAM are shared across
  slots. What a game must do (or must not assume) when it runs in a slot
  other than 1 is **unknown**: to be tested on a multi-slot board.
- **Backup RAM** at $D00000–$D0FFFF, MVS only; `REG_SRAMLOCK/UNLOCK`
  ($3A000D/$3A001D) are MVS only; there is a calendar chip
  ([W: 68k memory map], [W: Memory mapped registers], [W: MVS]). The SDK
  must not assume backup RAM exists on other targets (see P1).
- **BIOS versions** MAME knows: Europe v1/v2, US v1/v2/4-slot/U3/U4,
  Japan v1/v2/v3/J3-alt/MV1B/MV1C/hotel, Asia MV1B/MV1C, plus the
  Universe BIOS hacks 1.0–4.0 ([M: neogeo.cpp `ROM_SYSTEM_BIOS`]).
- **Behaviour differences between generations** (timer IRQ, VRAM access
  spacing, auto-animation, shadow, palette banks): **none documented** in
  [W] or [M]. MAME models all generations with one video device. Treat this
  as unknown, and test raster work on at least one generation-1 board
  (LSPC-A0) and one later board.

## 3. AES (home)

| Board | Chipset | Notes |
|---|---|---|
| NEO-AES | PRO-B0 / PRO-C0 / LSPC-A0 (NEC) | first version |
| NEO-AES3-3 | NEO-B1 / NEO-C1 / LSPC2-A2 | reportedly patched for 9 V supplies only |
| NEO-AES3-4 | LSPC2-A2 | "third (?)" version |
| NEO-AES3-5 | — | analog video section changed; 5 V or 9 V supply; different audio op-amps |
| NEO-AES3-6 | — | analog video section changed |
| NEO-AES4-1 | — | the latest; some heavily shielded |

Sources: [W: NEO-AES board], [W: NEO-AES3-3 board] to [W: NEO-AES4-1 board],
[W: LSPC2-A2], [W: AES hardware].

- **Outputs:** RGB and composite ([W: AES hardware]). RF output: **unknown**
  (not in the sources). RGB shows the most; composite smears fine
  detail, 1-pixel lines and dithering; how much each output crops at the
  edges depends on the TV (see §6).
- **No backup RAM and no calendar**; a memory-card slot at $800000–$BFFFFF.
  MAME's card image is 2 KiB ([W: AES hardware], [W: 68k memory map],
  [M: ng_memcard.cpp]).
- **BIOS versions** MAME knows: Asia AES, Japan AES, and a development ROM
  ([M: neogeo.cpp]). MAME lists its AES input clock as wrong: the NTSC AES
  runs at 24.167829 MHz (PAL: "is same?", unknown). Game timing therefore
  runs ~0.7% faster on an AES than in MAME.
- **Behaviour differences between revisions** in timer IRQ, VRAM access or
  video timing: **none documented**. AES3-5 and 3-6 changed only the
  analog video (the picture, not the timing).

## 4. What affects us, per feature

| Feature | MVS | AES | Difference known? | Source / status |
|---|---|---|---|---|
| Timer IRQ | LSPC-A0 or LSPC2-A2 or integrated | same families | none documented; MAME emulation is off by a couple of lines | unknown on hardware |
| Video timing | 264 lines, 59.19 Hz | same (NTSC); PAL 312 | PAL AES not verified | [W], [M] |
| Watchdog | $300001, ~8 frames | same | exact timeout uncertain | [M], [C], [W] |
| VRAM access | 12/16-cycle spacing | same | none documented | [W: VRAM] |
| Auto-animation | LSPCMODE | same | none documented | [M] |
| Shadow | `REG_SHADOW` | same | none documented | [W] |
| Palette banks | `REG_PALBANK0/1` | same | none documented; which bank each BIOS leaves selected is **unknown**: test it (G5) | [W] |
| Memory card | some boards only (not MV-1F) | slot, 2 KiB | per board | [M] |
| Backup RAM | yes | no | yes | [W] |
| Soft DIPs / settings | BIOS soft DIPs | none: an in-game menu is needed | yes | SDK `sdk/cabinet` |

## 5. The Neo Geo CD, compared

- **Models:** front loader (CDROM-1 and CDROM-2 boards), top loader
  (NEO-CDM3-1/3-2/4-1/4-2/5-1) and CDZ (NEO-CDC1-1)
  ([W: CD hardware versions]). The system chips named on the wiki are
  NEO-MGA (front), NEO-MGA-T (top loader) and NEO-MGA2-SA (CDZ).
- **Same video chipset family?** MAME builds the CD from the same AES base
  class and the same sprite generator device, pointed at RAM instead of
  C ROM ([M: neogeocd.cpp: `aes_base_state`, `m_sprgen->set_sprite_region`]).
  So sprites, FIX, palettes, VRAM rules and the timer are *emulated* the
  same. A primary hardware document confirming this is **not found**.
- **Memory** [M: neogeocd.cpp]:

  | What | Size |
  |---|---|
  | 68000 program RAM | 2 MiB ($000000–$1FFFFF) |
  | Sprite (C) RAM | 4 MiB |
  | FIX (S) RAM | 128 KiB |
  | ADPCM RAM | 1 MiB |
  | Z80 RAM | 64 KiB |
  | Internal save memory ("memcard is internal") | 8 KiB |

- **CD speed:** 1× = 150 kB/s (front and top loader), 2× = 300 kB/s (CDZ);
  a 4.48 s constant and 0.65 s per file ([W: Loading time]).
- **Emulation caveats:** MAME's notes say games using raster effects are
  broken without a kludge (the CPU floods with timer IRQs), and that the
  original (non-Z) Neo Geo CD does not run ([M: neogeocd.cpp]).
- **Watchdog on the CD:** the wiki asks "The NeoGeo CD 2 also has a watchdog
  timer ?". **Unknown**.
- **BIOS:** top loader, top-loader prototype (ver 0.02) and front loader
  (neocd), and the official CDZ BIOS (neocdz) ([M: neogeocd.cpp]).
  The BIOS calls for loading files are **not yet cited**; this is for CD1.

## 6. The picture on a CRT and on HDMI

- The hardware outputs 320 × 224 active. MAME notes: "the real hardware can
  display 320x224 but most of the games seem designed to work with a width
  of 304, some less" ([M: neogeo.cpp, confirmed non-bugs]).
- **Arcade monitors** can be adjusted to show nearly all of it, but edges
  and corners are often lost. Rule: nothing that matters in FIX columns 0
  and 39, and the 8-pixel border beyond 304 px wide must hold clean art.
- **Consumer TVs** overscan. The broadcast guidance (SMPTE RP 27.3, 4:3):
  action-safe is the centre 90%, title-safe the centre 80%. For 320 × 224:
  - **action-safe:** about 288 × 202, from (16, 11)
  - **title-safe:** about 256 × 180, from (32, 22)

  These are guidelines, not measurements of any TV.
- **HDMI (AES+) and RGB upscalers** show everything, the border included.
  So the unsafe margin must hold finished art, never garbage (G4).
- **Our rule:**
  - play inside x 16–303 and y 8–215;
  - important text inside FIX columns 4–35 and rows 3–26;
  - HUD within title-safe where it can be.

## 7. The AES+, public facts only ([P], not verified)

The press reports agree on these:
- HDMI output up to 1080p, plus the original analog AV output;
- DIP switches under the console for language, display modes and an
  overclock setting;
- re-engineered ASICs, not software emulation, meant to run original
  cartridges.

They conflict on saving: internal memory for high scores, or a memory card
still required.

Reports also say the launch moved to 16 September 2027. The outlets are
Hypebeast, Time Extension, TechEBlog and Plaion's Replai page (2026).

Nothing in the ROM may depend on the AES+. What it runs at under its
overclock setting is **unknown**, and so is whether it reports itself in
any documented way (no documented detection found). Everything is
**not verified on AES+**.

## 8. The checklist every G-task passes

1. **Timing in frames.** No game timing in wall-clock units: frames only,
   at 59.19 Hz.
2. **No CPU-speed assumptions.** No delay loops, NOP padding or dbra waits;
   VRAM spacing kept to 12/16 cycles at 1×, and still met at 2×.
3. **The watchdog.** Kicked every frame from the frame loop; no path runs
   longer than ~8 frames without a kick, loading screens included.
4. **VRAM and palettes.** VRAM and palette writes stay within the VBlank
   budget (30,720 cycles less overhead), or are shown to be glitch-free.
5. **Sprite counts.** Sprites per line ≤ 96 in the worst frame (measured),
   ≤ 381 in total.
6. **Timer IRQ.** Its reload value is never ≤ 4; raster work is tested on
   an LSPC-A0 board and a later one, or listed as not verified on each.
7. **The screen edges.** The safe-area rules (§6) hold; the border holds
   clean art.
8. **Palette banks.** The bank each BIOS expects is restored on return to
   the BIOS (MVS, AES, Uni-BIOS, EagleBIOS, CD).
9. **No console-only features.** Nothing needs the AES+, the memory card,
   backup RAM or soft DIPs to work: each has a fallback.
10. **Not-verified lists.** One per target: MVS generations 1–5, AES
    revisions, AES+, CD front / top / CDZ.

## 9. Risk per feature

| Feature | Risk | Why | Mitigation |
|---|---|---|---|
| Raster effects (timer IRQ) | **high** | revision differences unknown; MAME off by lines; the CD emulation floods | test on LSPC-A0 and LSPC2 boards; keep the effects cosmetic; guard the reload > 4 |
| VRAM writes in active display | medium | allowed [W], but with spacing, and each generation's arbitration is unknown | two-phase frame, flushed in VBlank (G2) |
| Palette writes in active display | medium | snow [W] | write in VBlank, or to the hidden bank (G5) |
| Palette bank switching | medium | which bank each BIOS expects is unknown | restore on exit; single-bank fallback |
| CPU speed (AES+ overclock) | medium | loops tuned to 12 MHz | spacing by construction, frame-based timing (G3) |
| Watchdog | low–medium | exact timeout uncertain | kick every frame, never block over 2 frames |
| Screen edges | medium | HDMI shows everything | FULL_320 / MASKED_304 per game, clean border (G4) |
| Auto-animation | low | one documented model | — |
| Shadow | low | documented | — |
| Memory card / saves | medium | per board and system; CD internal 8 KiB | save backends per target (P1) |
| Multi-slot MVS | medium | what a slot other than 1 changes is unknown | test on a multi-slot board |
| CD loading | medium | 150–300 kB/s, 0.65 s a file | few, large files; the watchdog kicked while loading |
| MAME perf readings | medium | MAME does not emulate the cartridge ROM wait states (a known issue in [M]); real boards can be slower | keep ≥10% frame headroom in G1 |
| PAL AES | low | 312 lines, not verified in MAME | frame-based timing works; test the borders |
| AES clock | low | 24.167829 MHz vs 24 MHz: ~0.7% faster | frame-based timing; no wall-clock music sync |
