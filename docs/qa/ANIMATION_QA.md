# Animation QA

`tools/animation_qa.py --game <game>` measures the frames that go into the
C ROM (for No Signal, `artbox/generated/f_<name>.png`) and draws a contact
sheet per character in `docs/qa/animation/<game>/`, every cycle on its own
row with the cycle's floor line in red and each frame's foot anchor in blue.
`metrics.json` beside the sheets holds every number below. The numbers are
evidence for a reviewer, not a verdict: a hop or a lunge is meant to move.

Measured per cycle:

- **baseline jitter**: range of the lowest opaque row over the grounded frames
  (0 = a steady floor; airborne frames such as the run's stride and jumps are
  excluded);
- **anchor drift**: range of the figure's horizontal centre (a large value
  with small poses reads as the sprite origin jumping);
- **distinct**: frames that differ; fewer than the cycle length means repeats;
- **height range**: the figure's height across the cycle (sudden resizing).

## No Signal Z Alpha (2026-10-28, v3 art)

| Character group | Walk | Run | Baseline jitter (walk) | Anchor drift (walk) | Findings |
|---|---|---|---|---|---|
| Heroes Jay, Kai, Zoe | 6 distinct | 4 distinct | 0-1 px | 1.7-1.8 px | walk is solid; run has 4 drawings (target 8-12) and two of them airborne |
| 11 street enemies (thief ... guard) | 4 steps, **3 distinct** | uses walk | 0-1 px | 0.2-2.8 px | `walk = {WALK1, WALK2, WALK3, WALK2}`: the passing pose repeats |
| 7 bosses (Rico ... Overmind) and the Archivist | 4 steps, **3 distinct** | uses walk | 0-2 px | 0.6-5.2 px | Zerox drifts 5.2 px sideways in the walk; Overmind's height swings 131-184 px within the walk |

Walking and running already follow the distance covered (`ns_draw.c`: one
picture per 16 pixels of stride), so feet do not skate whatever the speed;
the gaps are in the drawings:

1. **Hero run**: 4 drawings (`RUN4 RUN5 RUN6 RUN3`). The target is 8: contact,
   down, passing, up for each leg, with forward lean and stronger arms.
   New frames need the art pipeline (`tools/import_sheets.py` imports
   painted sheets into `artbox/painted/fighters/<name>/NN.png`) and new
   `HF_` indices.
2. **Enemy and boss walks**: a fourth drawing per character replacing the
   repeated `WALK2`, so the cycle reads contact, passing, contact, passing
   with opposite legs.
3. **Overmind**: check whether the 53 px height swing is a deliberate hover
   bob or a frame drawn at another scale.
4. **Zerox**: the 5 px sideways jump in the walk.

These need new or redrawn pixel art; this pass measured and documented them
and did not generate art. The contact sheets are the reference for that
work: `docs/qa/animation/nosignal/<name>.png`.

## Shadow League Fighting

44 fighters are defined in `games/shadowleague/fighters/*.json`: idle 4
steps, `walk_f` and `walk_b` 6 frames each (5 and 6 game frames per
drawing), jumps in three phases, named attack frames with active markers.
The game has its own checker, `games/shadowleague/tools/fighter_validate.py`.
The portrait and face redesign of Part IV needs new portrait art for every
fighter; not started in this pass.

## Maiya & Luna

Scale and perspective (Part V) need measurements from captures of the
reported scenes; not started in this pass. `tools/animation_qa.py` takes a
new game with a frame folder and a cycle table (`GAMES` in the script).

## Sky Lance

Sky Lance's aircraft frames are banked poses, not walk cycles; its fixes in
this pass are motion and rendering (projectiles, clouds, sprite order); see
`games/skylance/DIFFICULTY.md`.
