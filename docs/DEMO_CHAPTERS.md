# The Demo Reel — 25 Chapters

**Eagle Software · Neo Geo SDK v1.7.0 · `games/demo`, id 777**

The demo ROM is a single linear flow of 25 chapters. Each one isolates a
subsystem, prints its own title and a one-line caption, and shows the
chapter number in the top-right corner of the screen — so a problem can be
reported as "chapter 14" rather than described.

```sh
make GAME=demo GAME_CFG_FILE=games/demo/game.cfg all
make GAME=demo GAME_CFG_FILE=games/demo/game.cfg test
```

| Control | Effect |
|---|---|
| **A** | Advance to the next chapter |
| **C** | Restart the current chapter from the top |

Chapters that need **C** for their own gameplay disable the restart binding
for their duration.

The reel lives in `games/demo/scenes/demo_unified.c`, in
`demo_unified_run()`. Every chapter is a function returning `uint8_t`, run
through `run_chapter()`, which re-invokes it on a restart request — so no
chapter needs its own reset path.

---

## The chapters

| # | Title | Subtitle | What it demonstrates |
|---|---|---|---|
| 01 | BOOT | ENGINE READY | Hardware brought up, engine initialised, chapter index explained |
| 02 | TITLE | ATTRACT REEL | Auto-centred attract sequence over generated title art |
| 03 | FIX LAYER | TEXT · PALETTES | The 40 × 28 visible text plane, three palettes, the dirty-cell cache |
| 04 | FIX FX | INFIX PHOTOS · 4 PHASE ANIMATION | Photographic INFIX pages, animated in four phases with palette cycling |
| 05 | SPRITE SCREENS | GENERATED FRAMES | The artbox screen reel — full-page sprite art drawn from `screens.c` |
| 06 | CHARACTERS | FULL WARRIOR MOVESET | A complete moveset cycle: stand, walk, strike, special A, special B |
| 07 | CHAR SELECT | PORTRAIT DEMO · JOYSTICK OVERRIDE | Portrait grid with live joystick selection |
| 08 | PHYSICS | GRAVITY · SOLIDS · GROUNDED | Gravity, a solid, and a grounded test — the eagle lands on the drawn floor line |
| 09 | CAMERA LAB | AUTO TOUR: HARD · SMOOTH · SHAKE · PAN · VERT · DEADZONE | Every camera mode in sequence, with parallax behind |
| 10 | PALETTE FX | SPRITE PALETTE STAGES | Fade, flash, pulse and colour-cycle staged on one sprite palette |
| 11 | PARTICLES | HERO SPECIAL MOVE + FX | Particles in narrative use — a special move with synced particle beats |
| 12 | PARTICLE LOAD | POOL STRESS + PRIORITY EVICTION | The 32-slot pool deliberately flooded, with the live count on screen |
| 13 | FEEDBACK | SHAKE · PARTICLE IMPACTS | Hitstop + shake + palette flash + sound in one `ng_impact_event()` call, at four intensities |
| 14 | DEPTH FX | ONE WARRIOR · CLEAN Z SCALE | `ng_depthfx_project()` — perspective projection and Z → shrink |
| 15 | DEPTH PARALLAX | TWO SCROLLING BG LAYERS | Two independently scrolled sprite background layers at different rates |
| 16 | NPCS | PATROL + THINK CALLBACK | NPC patrol bounds and the periodic think callback |
| 17 | MINI-GAME | FIGHT THE CLONE · B STRIKE | Arrows + B. A multi-glyph sword arc with per-frame palette shimmer |
| 18 | JOYSTICK | LIVE INPUT · TWO-BUTTON SPECIALS | Live input readout; B+C light special, B+D heavy finisher, B basic strike, against a crate hitbox target |
| 19 | SCROLL LEVEL | WORLD MAP · H/V STAGES | A 768-px world with the camera following a jump arc |
| 20 | CHAR 2D | EAGLE FLIGHT ARC | Character flight over vertical parallax |
| 21 | TARGET RANGE | MOVING TARGETS · SPRITE DEPTH | Crates approaching down depth lanes, breaking apart where they are hit |
| 22 | DEPTH RIDE | OBJECTS APPROACH AS YOU WALK | Sprite-scaling pseudo-3D: NPCs approach from the horizon, L/R to move |
| 23 | SOUND | YM2610 TOUR · ADPCM-B / FM / SSG / SPEECH | Every audio subsystem in turn, with labels naming the active driver call |
| 24 | SKY LANCE | VERTICAL SHOOTER MINI | A playable slice of the full Sky Lance game — waves, boss, scoring. Full-screen scrolling sky with the HUD letterboxed onto opaque FIX bands |
| 25 | CREDITS | EAGLE SOFTWARE 2026 | Module roll with a palette fade-out |

## Reading the reel as documentation

Each chapter is deliberately self-contained, and `chap_header()` performs a
full hardware and engine reset at the top of every one:

```c
ng_clear_screen_full();          /* VRAM + SCB wipe, on the clear colour  */
demo_sprite_window_cache_reset();/* the tracking cache is NOT VRAM state  */
setBACKDROP(DEMO_BG);
ng_level_set_scroll(0, 0);
ng_particles_init();
ng_feedback_init();
ng_palette_fx_init();
```

That reset is itself worth copying. Two of its lines exist because of bugs
that were hard to find:

**The sprite-window tracking cache is separate from VRAM.** It is global
across the whole ROM run, so slot numbers claimed by an early chapter stay
claimed and starve a later, slot-heavy chapter until they are evicted.
Resetting it per chapter gives each one the full budget.

**`clearSprs()` is deliberately *not* called here.** `ng_clear_screen_full()`
already does a full `ng_sprite_hide_all()` with the blank tile `0xFFFF`. A
second `clearSprs()` would overwrite that safe fill with tile 0 — which may
carry actual art.

## Page colours

The demo uses a white page with dark ink, defined once in `demo.h`:

```c
#define DEMO_BG_CLEAR    BLACK    /* only during the wipe */
#define DEMO_BG          WHITE
#define DEMO_INK_BODY    BLACK
#define DEMO_INK_SUB     BLUE
#define DEMO_INK_ACCENT  MIDGREEN
#define DEMO_INK_PROMPT  RED
```

The clear runs on black and the page is restored to white afterwards,
because a wipe straight to white flashes.

## FIX row map

The demo's chapters share a row layout so captions never collide:

| Row | Content | Written by |
|---|---|---|
| 0 | Chapter tag, title, index | `chap_header()`, once |
| 1 | Chapter subtitle | `chap_header()`, once |
| 2–3 | Per-chapter caption lines | the chapter |
| 4–25 | Chapter body | the chapter |
| 26 | Separator rule + control hint | `chap_header()` / `chap_hint()` |
| 27 | Bottom caption bar | `chap_header()` |

Only 28 FIX rows are visible; the visible row *y* is map row *y + 2*, and
the SDK's FIX helpers already apply that offset.
