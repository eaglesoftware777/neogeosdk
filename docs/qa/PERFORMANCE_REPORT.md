# Performance Report

Figures are measured unless marked otherwise; the method is given with each.

## Sprites per scanline (Sky Lance)

The LSPC draws at most 96 sprites on a line and drops the highest-numbered
ones beyond that. Sky Lance counts its own load each frame
(`ng_shooter_budget`, the busiest band of lines, terrain and clouds
included) and, from v2.1.0, keeps the figure in `s_line_peak`; the shot probe
records its maximum over a run.

| Run (Linux MAME, 75 s, scripted) | Busiest line | Live enemy shots (max) |
|---|---|---|
| v2.1.0 candidate, sector 1 waves | 79 of 96 | 7 |
| v2.1.0 candidate, sector 1, plane held under each enemy | 65 of 96 | 6 |
| v2 release, earlier inspections (PRODUCTION_STATUS.md, 2026-10-11) | 54 of 96 | |

Policies now in the game: optional particles yield above 90, aircraft
shadows when a shadow would pass 90, the low cloud bank above 88, and enemy
fire is held above 94 (`SKY_FIRE_LINE_LIMIT`), so no bullet is spawned onto
a line where the hardware could not draw it. The order of the fixed sprite
slots now sacrifices the pilot portrait and the player's own shots before
enemy shots (`scenes/sky_draw.h`).

## Game logic rate

Sky Lance updates its game logic on about half of the video frames: 41.8
updates/s before the 2026-10-20 work and 48.7 after, at stage 1 (recorded in
`games/skylance/CHANGELOG.md`; MVS stress captures). A sustained 60 updates/s
is not reached. The difficulty table's shot rates assume about 29 updates/s
of firing logic (measured 49% of video frames) and are documented as such.

## ROM space (`tools/rom_budget.py`, today's builds)

| Game | P | C | S | M | V |
|---|---|---|---|---|---|
| Sky Lance v2.1.0 candidate | 58% | 41% | 11% | 27% | 54% |
| No Signal v3 | 19% | 9% | 9% | 16% | 48% |
| Shadow League v3 | 67% | 62% | 41% | 18% | **95%** |

Shadow League's V ROM is nearly full: new voices or music need samples cut
or the V ROM grown.

## VRAM writes on the hot path

The engine's sprite-group move is designed to cost two VRAM words (the
driver strip's SCB3 and SCB4). The host test measures 2 in steady state; the
first move after a vertical shrink change writes 10 (BASELINE_AUDIT.md). A
shrink change once per scene is harmless; per frame it would be 8 words a
group.

## Not measured in this pass

VBlank execution time and VRAM transfer volume per frame need the MAME
debugger or an instrumented build (Sky Lance's probe already records pending
sprite groups and duplicates: `vram_pending`, `vram_duplicates` in
`tools/qa.lua`); CPU load per subsystem and frame drops under the heaviest
boss patterns of sectors 5 to 7 (the new difficulty tiers) need stress
captures on those sectors.
