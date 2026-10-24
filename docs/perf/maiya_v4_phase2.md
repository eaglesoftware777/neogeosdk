# Maiya v4 phase 2: a steady frame rate

Measured on 2026-10-03, MAME 0.264, original MVS BIOS (euro). Same scenarios as `maiya_v3_baseline.md`, 3,600 frames each.

- **Timing only (`PERF=2`).** The closest to a release build: only the frame's start and end are read, with no counter on each video write.

      python3 games/maiya/tools/build.py --perf-lite
      python3 games/maiya/tools/perf_report.py --mame <mame>

- **All counters (`build.py --perf`).** Every video write is counted, which costs time of its own. Numbers in this build read higher.

## Frame time

`PERF=2`, against phase 1's `PERF=1` table (`maiya_v4_phase1.md`):

| Scenario | Game fps | Overran | Work, avg lines | Phase 1: overran / avg |
|---|---|---|---|---|
| Stage 1 Emerald Forest | 59.1 | 0% | 150 of 264 | 37% / 259 |
| Stage 2 Valley of Falls | 59.2 | 0% | 146 of 264 | 39% / 257 |
| Stage 4 Autumn Grove | 59.0 | 1% | 178 of 264 | 59% / 284 |
| Stage 6 World Tree | 59.0 | 1% | 187 of 264 | 91% / 333 |
| Guardian fight (forest) | 59.0 | 1% | 145 of 264 | 33% / 243 |
| Stage 11 Sky Road | 59.2 | 0% | 135 of 264 | 23% / 234 |
| Stage 12 citadel and rush | 59.1 | 1% | 131 of 264 | 35% / 256 |

59.19 frames a second is the machine's full rate. Average work is 131–187 lines of 264, which is 29–50% headroom.

**Where the remaining overruns are.** Logged frame by frame, before the last round of fixes:
- In play:
  - the World Tree had 13 frames over budget out of about 3,100;
  - Autumn Grove and the citadel had 2 each;
  - the other stages had none.
- Outside play:
  - the stage start (the first frame draws the stage in);
  - the guardian's entrance;
  - the healed valley's tour.

All counters (`PERF=1`), for the video-write columns:

| Scenario | Overran | Work, avg lines | VRAM words, avg / peak | Written on drawn lines | Most strips on a line |
|---|---|---|---|---|---|
| Stage 1 Emerald Forest | 1% | 165 | 58 / 2269 | 27% | 74 (line 192) |
| Stage 2 Valley of Falls | 3% | 160 | 50 / 827 | 35% | 78 (line 183) |
| Stage 4 Autumn Grove | 5% | 203 | 90 / 2266 | 41% | 69 (line 176) |
| Stage 6 World Tree | 8% | 215 | 86 / 2426 | 40% | 71 (line 195) |
| Guardian fight (forest) | 9% | 165 | 45 / 2526 | 39% | 71 (line 193) |
| Stage 11 Sky Road | 0% | 142 | 47 / 1183 | 9% | 66 (line 24) |
| Stage 12 citadel and rush | 1% | 137 | 49 / 2379 | 21% | 70 (line 168) |

**Reading it:**
- Drawn-line writes were 58–85% in phase 1. Almost all of the rest now come from commits that ran past the blank: 4–70% of frames in this build.
- In this build each write carries a counter, which makes the commit 23–41 lines long against a blank of about 40.
- Started late is now 0–7% of frames (it was 17–78%).
- Strips per line peak at 78 of 96.

## What changed

**Sound: the frame no longer waits for the Z80** (`NG_SOUND_QUEUE`; Maiya sets it).
- Each sound byte used to wait for the driver's short "taken" reply. Measured in MAME, the 68000 often missed that 22 µs window and ran out its whole wait: 38,000 cycles, about 50 lines, for one byte.
- Now bytes wait in a queue. A byte goes out only when the reply shows ready and the previous byte went out at least 16 lines (about 1 ms) earlier.
  - The driver reads the latch about 13 µs after the write and leaves its handler about 300 µs after it.
- The queue moves in three places:
  - at each sound call;
  - once a frame in the vertical blank;
  - once at the middle of the frame.
- The delay routines empty the queue first, so a sequence timed with delays reaches the driver as before.
- Checked over a minute of play: 341 bytes written, 341 read by the Z80, none written over an unread latch, the shortest gap 1.0 ms.

**Text layer** (`ng_fix`):
- It remembers each cell's map word and writes only the cells that change.
- Changed cells queue as runs along a row; the commit sets each run's address once.
- Maiya's tray and hint lines are built whole and then compared, so a redraw no longer clears and rewrites every cell.
- The tray follows the rest of the HUD one frame later.

**Characters:**
- One visibility test each.
- Each character's sprite update compares its fields directly instead of making eight setter calls.
- That update and the visibility test are kept out of line; inlined, GCC rebuilt each field's address from the index.
- `ng_chars_index` uses one 68000 divide.

**Physics:**
- It walks only the character slots in use.
- It rejects solids that are out of reach before the full test.
- It reads the world's properties once.

**Scenery:**
- `ng_sprite_group_show_at()` places a group with one call (tile, palette, position, shown) instead of five.
- `ng_sprite_groups_hide_all()` hides the unused rest of a pool.
- Ledges, decoration, hazards, the tray and falling pieces use them.
- Spent sparks are skipped.
- The spawn scan does half its tables each frame.
- Ledges are tested against the view before being looked up.

**Screen fades** (`ng_palette_fx`):
- A colour is now two byte-table lookups and an OR: 78 cycles, down from 200.
- A level between full colour and full white or black is blended half one frame and half the next, then shown whole.
- The end levels are a copy or a fill.
- Whole banks are copied with `movem`.
- The guardian's warp went from 541 lines in its worst frame to 323. Only one of its 64 measured frames now runs long: the one where the arena is drawn in.

**Healed valley glow:** two banks a frame instead of four, with a faster per-colour step.

**No library arithmetic in a frame:** 16-bit multiplies and 68000 divides replace `__mulsi3` and `__divsi3` in the sparks, the glow tables and the character index.

## Checks

- **Build:** zero warnings.
- **Regression scenarios:**
  - Pass: idle, walk, climb, boss, continue, continue-timeout, tray, pit, flight.
  - Fail: bonus, factory and pickups. bonus and factory fail the same way on the last commit, built on this machine; pickups is a known stale scenario.
- **The HUD** was checked in captures.
- **Other games:**
  - demo_plus builds byte-identical to the last commit.
  - helloworld and demo change with the engine.
  - Built from the last commit on this machine, both hang at boot. The CPU waits forever for the Z80, because their committed M1 drivers predate the current sound protocol. That is fixed with the Framework v1 rebuild of every game.

## Not verified

- Classic MVS, AES and AES+ hardware.
- Sound by ear on any of them. The queue was checked only at the Z80's ports in MAME.
