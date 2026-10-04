# Maiya: smooth play (hitstop, frame spikes, the sound queue)

Measured on 2026-10-04, MAME 0.264, EagleBIOS (euro). Same method as phase 5: every stage's road from its start and every guardian's arena (`gate`), 1,800 frames each, on the timing-only build.

    python3 games/maiya/tools/build.py --perf-lite
    python3 games/maiya/tools/perf_report.py --all-stages --frames 1800

## What read as the game hanging

**The hitstop, not the frame rate.** A frame-by-frame trace of the first valley (Maiya running right, jumping and striking, as `perf_capture.lua` plays it) showed eight 3-frame holds in 1,000 frames of play and one late frame:
- every creature beaten went through `NG_IMPACT_LIGHT`, whose preset holds the whole scene for 3 frames, Maiya included;
- a heavy blow a creature survived held 4, a guardian's hit 5 (`NG_IMPACT_MEDIUM`), her own hurt 5;
- over the forest guardian's run (1,200 frames: the walk in, the fight, the flight after), 47 frames were held.

The holds are now Maiya's own (`maiya_feel.h`, applied by `mg_impact()`; the engine's presets are unchanged):

| Blow | Held before | Held now |
|---|---|---|
| A creature beaten | 3 | 0 |
| A heavy blow a creature survives | 4 | 0 |
| A guardian struck | 5 | 2 |
| Maiya struck | 5 | 2 |
| A guardian's last blow | 14 | 10 |

The shake, the flash, the sparks and the sound are as before. The first valley's trace now has no held frame and no late one; the forest guardian's run has 18 held frames.

## Late frames

An instruction trace (MAME's debugger, `trace` without loop folding) costed per function and per frame found the spikes that pushed a frame past its 264 lines:

| Where | Cost | Change |
|---|---|---|
| The far painting's streaming (`mg_layer_stream`) | about 12,000 cycles on each frame that moves the window a column: a `divu` for each of 32 strips | one divide; each strip is a step on from the window's column (checked against the old formula for every offset of the three painting widths) |
| The engine's timers, progress trackers and NPCs | about 7,000 cycles every frame: 64 + 64 + 32 slots looked at, none in use | each list is skipped while nothing in it runs (set by a start or a spawn, worked out again by each update) |
| The healed valley's colour lift (`mg_glow_step`) | about 35,000 cycles on the frame that worked out a step's tables and recoloured two banks | the tables' frame recolours nothing; the banks follow |

| Scenario | Overran before | after | Work, avg lines of 264, before | after |
|---|---|---|---|---|
| Stage 1 Emerald Forest | 0% | 0% | 161 | 157 |
| Stage 1 guardian | 2% | 1% | 167 | 156 |
| Stage 2 Valley Of Falls | 0% | 0% | 173 | 160 |
| Stage 2 guardian | 5% | 1% | 163 | 150 |
| Stage 3 Azure Coast | 0% | 1% | 184 | 139 |
| Stage 3 guardian | 2% | 1% | 169 | 160 |
| Stage 4 Autumn Grove | 1% | 0% | 170 | 184 |
| Stage 4 guardian | 3% | 0% | 166 | 163 |
| Stage 5 Crystal Grotto | 0% | 1% | 187 | 160 |
| Stage 5 guardian | 2% | 0% | 172 | 165 |
| Stage 6 Sacred World Tree | 1% | 1% | 179 | 195 |
| Stage 6 guardian | 2% | 0% | 173 | 166 |
| Stage 7 Rio Negro Works | 0% | 0% | 189 | 181 |
| Stage 7 guardian | 0% | 0% | 137 | 135 |
| Stage 8 Sunken Reef | 1% | 1% | 202 | 187 |
| Stage 8 guardian | 3% | 0% | 173 | 167 |
| Stage 9 Silver Cave | 0% | 0% | 178 | 179 |
| Stage 9 guardian | 2% | 0% | 180 | 170 |
| Stage 10 Golden Savanna | 1% | 1% | 167 | 156 |
| Stage 10 guardian | 2% | 0% | 169 | 161 |
| Stage 11 Sky Road | 0% | 0% | 133 | 123 |
| Stage 12 Smog Citadel | 1% | 0% | 204 | 191 |
| Stage 12 guardian | 1% | 1% | 144 | 140 |

- A road's average can rise: the frames that were held cost little, and they now run the whole game. The runs differ too, with no holds in them.
- The `gate` runs go on past the fight. Counted by the game's state over 1,800 frames: the fights themselves had no late frame. What is left is in the flight over the healed valley (4 frames in 633 on the Valley of Falls, 10 before), the warp's white-out, and the frames that build a scene behind a fade.
- In play on the roads it is a few frames in half a minute (the World Tree: 3 in 1,801), most of them a few scanlines over.

## The sound CPU

The queue (`NG_SOUND_QUEUE`, `sdk/neogeolib.c`) sends a byte when the Z80 shows ready and 16 lines have passed since the last. It is tried at each sound call, once mid-frame and in the blank. Now:
- the blank tries it first, before the colours and the sprites;
- the end of each frame's work (`game_frame`) tries it once more;
- the game's own waits on the Z80's reply (`isZ80Ready()` before the music and voice calls) are gone, as the queue waits byte by byte.

Checked with a write tap on `$320000` and a tap on the Z80's reply port (`$0C`) over 70 seconds of boot, credit, chooser and play: every byte the 68000 sent was taken (the reply dropped to 0 once for each), on MVS and AES, and the recorded sound's level per second matches the previous build's.

## Interrupts

The vertical blank's interrupt sets the frame flag, acknowledges, kicks the watchdog and reads the controls through the system ROM (`SYS_IO`); the frame's VRAM work runs after the wait, in the main loop. Measured, the interrupt side and the whole sound path cost under 1.5% of a frame: there was nothing to gain there.

The 68000 is one processor: what runs beside it is the Z80, which plays all the sound on its own from the queued bytes. Work that doesn't need every frame is spread over frames instead: the road's spawn scan (half a frame's worth every other frame), a fade (half the banks a frame), the colour lift (two banks a frame, its tables on a frame of their own).

## Writes only in the blank

Measured after the first part of this work, on the timing-only build with the commit's own start and end recorded: on a busy road (the Autumn Grove, the Valley of Falls) the commit ran 30 to 50 lines from line 244, past the first line of the picture (16), on 30 to 75% of frames.

The commit (`ng_vram_commit`, `sdk/2d_engine/ng_sprite_group.c`) now has a deadline, `NG_VRAM_DEADLINE` (line 8): no job starts after it. What doesn't fit stays listed for the next blank:
- phase A, every listed group that only moved: its driving strip's two words, one VRAM address with the step set to $200 (SCB3 and SCB4 are $200 apart);
- phase B, content by priority: `NG_SG_PRIO_HIGH` (the player, the scrolling layers), a streaming layer's strip columns, `NORMAL` (the cast, shots, objects), the FIX text, `LOW` (decoration). A job starts only with its estimate and half again to spare.

The colour upload (`ng_palfx_vblank`) keeps to the same deadline; a whole-screen upload (82 banks, about 18 lines) starts only with room for it.

Measured on 1,500 frames each, running and jumping:

| Scenario | Commits whose last write fell on a drawn line | Content changes left for the next blank, a frame |
|---|---|---|
| Stage 2 Valley of Falls | 0 | 2.7 |
| Stage 4 Autumn Grove | 0 | 2.7 |
| Stage 6 World Tree | 1 | 1.8 |
| Stage 10 Golden Savanna | 2 | 1.2 |

(The content count is taken at the commit's end in the measurement build, which costs it a little of the blank: a release build leaves less.)

What made room:
- a plain move from about 300 to under 200 clocks, and no move waits behind content;
- `ngsg_put_strip` always inlined: a 2 x 2 block's new map from 1,676 to 1,368 clocks, the player's from 5,832 to 4,348;
- shrinks (sparks, petals) through their own path, and dust sparks rescaled in sixteenths, the hardware's own sideways step, instead of every frame;
- ledges, hazards and decoration keep their sprite groups while in view (`mg_pool_show`): handed out in screen order, every block took its neighbour's piece each time the first left the screen.


## The v3 roads

Measured on 2026-10-05 after the v3 work (her stride and walk, the camera, every road laid out again, the ledges' new pieces and undersides, dust at her feet), the same way, 1,800 frames each:

| Scenario | Game fps | Overran | Work, avg lines of 264 | Most sprites on a line |
|---|---|---|---|---|
| Stage 1 Emerald Forest | 58.9 | 0% | 176 | 70 |
| Stage 1 guardian | 58.6 | 0% | 166 | 62 |
| Stage 2 Valley Of Falls | 59.2 | 1% | 186 | 72 |
| Stage 2 guardian | 58.4 | 0% | 158 | 55 |
| Stage 3 Azure Coast | 59.2 | 0% | 202 | 69 |
| Stage 3 guardian | 58.7 | 0% | 171 | 56 |
| Stage 4 Autumn Grove | 59.1 | 1% | 200 | 69 |
| Stage 4 guardian | 58.9 | 0% | 172 | 55 |
| Stage 5 Crystal Grotto | 58.9 | 1% | 194 | 69 |
| Stage 5 guardian | 58.9 | 0% | 175 | 56 |
| Stage 6 Sacred World Tree | 59.1 | 2% | 218 | 68 |
| Stage 6 guardian | 58.9 | 0% | 172 | 55 |
| Stage 7 Rio Negro Works | 59.2 | 0% | 204 | 66 |
| Stage 7 guardian | 59.2 | 0% | 136 | 56 |
| Stage 8 Sunken Reef | 59.2 | 0% | 212 | 59 |
| Stage 8 guardian | 58.9 | 0% | 168 | 57 |
| Stage 9 Silver Cave | 58.6 | 1% | 180 | 65 |
| Stage 9 guardian | 58.9 | 0% | 177 | 56 |
| Stage 10 Golden Savanna | 58.6 | 1% | 177 | 66 |
| Stage 10 guardian | 58.9 | 0% | 172 | 57 |
| Stage 11 Sky Road | 59.2 | 0% | 151 | 53 |
| Stage 12 Smog Citadel | 58.9 | 1% | 205 | 64 |
| Stage 12 guardian | 59.0 | 0% | 159 | 54 |

- Frames that run late stay at 0-2%, every scenario at about 59 frames a second, and no line has more than 72 sprites (the hardware draws 96).
- A road's average work can't be set beside the table above: the runs follow the new layouts, where she spends more time up on ledges with more of them in view.
