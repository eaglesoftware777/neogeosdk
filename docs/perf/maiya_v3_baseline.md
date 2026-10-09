# Maiya v3: performance baseline (v4 phase 0)

Measured on 2026-10-03 in MAME 0.264 (original MVS BIOS, euro), MVS build of
commit `864216a` with `PERF=1`. Reproduce with:

    python3 games/maiya/tools/build.py --perf
    python3 games/maiya/tools/perf_report.py --mame <mame>

Each scenario is 3,600 frames of scripted play: walking right, jumping and
striking, unhurt, from the stage's start. The guardian and rush runs walk
through the gate first. One frame is 264 scanlines at 59.19 Hz, and
16..239 are drawn.

| Scenario | Game fps | Overran | Work, avg lines | Peak | VRAM words, avg / peak | Written on drawn lines | Most strips on a line |
|---|---|---|---|---|---|---|---|
| Stage 1 Emerald Forest | 58.2 | 35% | 259 of 264 | 527+ | 54 / 717 | 88% | 71 (line 192) |
| Stage 2 Valley of Falls | 56.8 | 46% | 277 of 264 | 527+ | 63 / 2240 | 87% | 72 (line 189) |
| Stage 4 Autumn Grove | 55.4 | 58% | 281 of 264 | 527+ | 72 / 755 | 88% | 65 (line 176) |
| Stage 6 World Tree | 50.2 | 89% | 314 of 264 | 527+ | 99 / 2240 | 86% | 69 (line 222) |
| Guardian fight (forest) | 56.3 | 32% | 240 of 264 | 527+ | 52 / 2753 | 88% | 71 (line 192) |
| Stage 11 Sky Road | 58.7 | 24% | 240 of 264 | 527+ | 51 / 1187 | 89% | 65 (line 24) |
| Stage 12 citadel and rush | 57.5 | 32% | 251 of 264 | 527+ | 63 / 3543 | 90% | 70 (line 160) |

**How to read it:**
- **Game fps:** game frames in the 3,600 video frames.
- **Overran:** the share of frames whose work ran past the next vertical blank. That frame then starts late, mid-picture, instead of on the blank.
- **Work, avg lines:** scanlines from the end of the blank wait to the end of the frame's work.
- **Peak 527+:** two frames or more; the counter can't see past that.
- **VRAM words:** sprites and FIX text, per frame.
- **Written on drawn lines:** written while the screen was being drawn.
- **Most strips on a line:** sprites on one drawn line, at the worst sample, taken every 15 frames from the video RAM. The hardware limit is 96.

The citadel run ended in the ending sequence (state 6), so its numbers include part of the ending.

## What it says

1. **The CPU is full.**
   - An average frame needs 240–314 of the 264 lines, and every stage overruns: from a quarter of its frames (the Sky Road) to nine in ten (the World Tree, 50 fps).
   - MAME doesn't slow ROM access with wait states (CLASSIC_BASELINE §9), so real boards have less margin still.
   - Phase 2's target is below 238 lines (90%) on average, and no peak past 264, on every stage.
2. **Nearly all video writes land on drawn lines (86–90%).**
   - Sprites change while they are being drawn, which is the tearing seen in play.
   - This is phase 1's problem, and it is cheap to fix: 51–99 words a frame on average is about 1,000–2,000 cycles, a small part of the ~30,700-cycle vertical blank.
3. **The peaks are scene changes.** Up to 3,543 words, when a backdrop or an arena is drawn in, more than one blank can take (~1,500 words). Phase 1 carries what doesn't fit over to the next blank and draws scene changes behind a fade.
4. **Sprites per line are fine: 65–72 of 96.** About 24 strips a line are free for phase 5's parallax.
