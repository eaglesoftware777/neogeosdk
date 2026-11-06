# Maiya v4 phase 5: sprites per line, measured on every stage

Measured on 2026-10-03, MAME 0.264, original MVS BIOS (euro): every stage's road from its start, and every guardian's arena (`gate`), 1,800 frames each.

    python3 games/maiya/tools/build.py --perf-lite
    python3 games/maiya/tools/perf_report.py --mame <mame> --all-stages --frames 1800 --strip-limit 96

- `--strip-limit 96` makes the report exit 1 when any scenario puts more than 96 sprites on one line.
- Sprites per line come from the sprite chip's video RAM every 15 frames, counted the way the hardware picks them (MAME `neogeo_spr.cpp`, `parse_sprites`).
- The strip column is from the all-counters build (`build.py --perf`). Its frame times run high, so the frame-time columns are from the timing-only build.

| Scenario | Overran | Work, avg lines of 264 | Most sprites on a line |
|---|---|---|---|
| Stage 1 Emerald Forest | 0% | 159 of 264 | 70 (line 180) |
| Stage 1 guardian | 1% | 163 of 264 | 71 (line 193) |
| Stage 2 Valley Of Falls | 0% | 171 of 264 | 78 (line 183) |
| Stage 2 guardian | 4% | 158 of 264 | 66 (line 192) |
| Stage 3 Azure Coast | 0% | 182 of 264 | 68 (line 186) |
| Stage 3 guardian | 1% | 165 of 264 | 66 (line 192) |
| Stage 4 Autumn Grove | 1% | 185 of 264 | 69 (line 176) |
| Stage 4 guardian | 1% | 163 of 264 | 66 (line 192) |
| Stage 5 Crystal Grotto | 0% | 185 of 264 | 66 (line 192) |
| Stage 5 guardian | 1% | 169 of 264 | 68 (line 192) |
| Stage 6 Sacred World Tree | 1% | 192 of 264 | 71 (line 194) |
| Stage 6 guardian | 2% | 170 of 264 | 66 (line 192) |
| Stage 7 Rio Negro Works | 0% | 188 of 264 | 63 (line 106) |
| Stage 7 guardian | 0% | 139 of 264 | 68 (line 192) |
| Stage 8 Sunken Reef | 14% | 238 of 264 | 64 (line 96) |
| Stage 8 guardian | 1% | 170 of 264 | 65 (line 192) |
| Stage 9 Silver Cave | 0% | 177 of 264 | 67 (line 162) |
| Stage 9 guardian | 1% | 177 of 264 | 68 (line 192) |
| Stage 10 Golden Savanna | 1% | 186 of 264 | 64 (line 183) |
| Stage 10 guardian | 1% | 166 of 264 | 64 (line 192) |
| Stage 11 Sky Road | 0% | 131 of 264 | 66 (line 24) |
| Stage 12 Smog Citadel | 1% | 203 of 264 | 65 (line 192) |
| Stage 12 guardian | 1% | 146 of 264 | 64 (line 193) |

**Every stage and arena stays at 78 sprites a line or fewer, against the hardware's 96.**

The table was measured before the reef fix below. After it, the **Sunken Reef** is at 1% over budget and 200 lines on average (1,800 frames). It had been at 14% and 238:
- Its painting's raster sway (phase 3) cost about 16,000 cycles a frame (14 band interrupts, and the table that drives them), so it's gone.
- Five creature-animation sites and the hideout's sparkle had used C's `%` and `/` on promoted ints: library calls of several hundred cycles each, 20 a frame on the reef. They are now single `divu.w` instructions (`mg_mod16`, `mg_div16`).

The savanna's horizon haze stays: 0% over budget, 184 lines on average.

## Parallax

Up to three full planes, with more stacked in bands, is something the engine can already do:
- a plane is a sprite group;
- a raster band (`ng_raster_vram`) can move a group's driving strip partway down the screen, which scrolls a band of one painting at its own pace.

**Maiya's paintings were not built for it.** Each stage is one 512 × 192 painting, and its tall shapes run through every depth: trees, cranes, peaks.

**The Sky Road was tried** (`mg_raster_step`):
- clouds above line 64 at a quarter of the view's pace, mountains below at half;
- measured right: over 64 frames, mountains 32 px, clouds 16 px;
- but the clouds that reach below line 64 shear along a straight seam, so it was taken back out.

More depth needs paintings made as separate layers (phase 6 art).

**Strips per line:**
- Today a painting is a 32-strip chain, and a chained strip counts on every line it covers, wherever it is on screen (`parse_sprites`).
- A plane that streams its columns needs 21 strips (320 / 16 + 1). That would free 11 strips a line per plane, should a third plane ever need the room.
- The check above is measured rather than computed at build time: only a run shows where the cast and the props stand on the screen.
