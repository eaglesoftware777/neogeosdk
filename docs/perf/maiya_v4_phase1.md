# Maiya v4 phase 1: sprites and text written in the vertical blank

Measured on 2026-10-03, MAME 0.264, original MVS BIOS (euro), `PERF=1` build.
The method and scenarios are the same as `maiya_v3_baseline.md`:

    python3 games/maiya/tools/build.py --perf
    python3 games/maiya/tools/perf_report.py --mame <mame>

## What changed

- **Sprite groups** (`NG_VRAM_DEFER`, opt-in per game; Maiya sets it):
  - `ng_sprite_group_flush()` only puts the group on a list.
  - `ng_vram_commit()`, called right after the wait for the vertical blank, writes the listed groups in the state they have then.
  - Character hides are listed too and written first, so a character moved to other slots isn't missing for part of a frame.
  - A removed character's waiting writes are dropped, so the commit can't bring it back.
- **FIX text cells** go through the same list, after the sprites. A clear of the whole layer drops the ones waiting.
- **The common case is cheap.** Nine flushes in ten only move a group. The commit writes those two position words directly, without the general routine.
- **Other games are unaffected.** helloworld, demo and demo_plus are byte-identical, since none of this is built without `NG_VRAM_DEFER`.

## The commit's cost (release build, exact cycles)

Profiled with breakpoints in MAME's debugger, driven from Lua (`manager.machine.debugger`), on stage 1:

| | Before the fast path | After |
|---|---|---|
| One group | 904 cycles (median) | position-only ~500, the rest ~900–3,500 |
| A gameplay commit (22–23 groups) | 23,101 median, 32,898 p90 | 15,268 median, 24,290 p90, 37,016 p99, 69,408 max |
| The vertical blank | ~30,700 cycles | |

The commit now fits the blank in nine frames in ten. The longest ones are scene changes: a backdrop or an arena drawn in, behind a fade.

## In play (PERF build)

| Scenario | Game fps | Overran | Work, avg lines | Peak | VRAM words, avg / peak | Written on drawn lines | Most strips on a line |
|---|---|---|---|---|---|---|---|
| Stage 1 Emerald Forest | 58.0 | 37% | 259 of 264 | 527+ | 53 / 866 | 63% | 71 (line 192) |
| Stage 2 Valley of Falls | 56.8 | 39% | 257 of 264 | 527+ | 56 / 955 | 69% | 74 (line 181) |
| Stage 4 Autumn Grove | 54.6 | 59% | 284 of 264 | 527+ | 75 / 854 | 75% | 66 (line 184) |
| Stage 6 World Tree | 50.3 | 91% | 333 of 264 | 527+ | 87 / 840 | 85% | 66 (line 192) |
| Guardian fight (forest) | 56.1 | 33% | 243 of 264 | 527+ | 52 / 2526 | 58% | 71 (line 193) |
| Stage 11 Sky Road | 58.7 | 23% | 234 of 264 | 527+ | 51 / 1187 | 40% | 70 (line 24) |
| Stage 12 citadel and rush | 57.3 | 35% | 256 of 264 | 527+ | 63 / 2880 | 65% | 70 (line 162) |

| Scenario | Commit started late (the frame overran) | Commit ran past the blank | Drawn-line writes made outside the commit |
|---|---|---|---|
| Stage 1 Emerald Forest | 29% | 13% | 1% |
| Stage 2 Valley of Falls | 31% | 10% | 1% |
| Stage 4 Autumn Grove | 48% | 12% | 1% |
| Stage 6 World Tree | 78% | 12% | 1% |
| Guardian fight (forest) | 25% | 7% | 11% |
| Stage 11 Sky Road | 17% | 8% | 0% |
| Stage 12 citadel and rush | 27% | 8% | 27% |

**Reading it:**
- **Writes still landing on drawn lines** come almost all from frames that overran: their commit starts after the blank is over. Fixing that is phase 2 (a steady frame rate).
- **Spills:** in this build the counters add their own cost to every write, so spills read higher here than in the release figures above.
- **Writes outside the commit** are near zero in play (0–1%). The guardian and citadel runs show 11% and 27%: their arenas are drawn in at once behind a white flash, as intended.

On a frame that finishes in time, Maiya's sprites and text change only in the vertical blank. The `bonus` regression scenario fails as it did before this change, a harness issue; idle, walk, climb, boss and continue pass.
