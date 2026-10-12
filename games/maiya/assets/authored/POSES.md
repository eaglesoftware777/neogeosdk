# Maiya's walk, run and stop poses

Indexed 80x64 canvas, ground line Y=61, root X=40 (sprite offsets -40, -62,
as for every other drawing of hers). `tools/aseprite/compose_gait.py` writes
the per-layer text maps in `maps/`; `build_walkrun.lua` turns them into
`maiya_walkrun.aseprite`; `export_walkrun.lua` and `pipeline.py` reopen it
and export the strips and the per-layer evidence in `layers/`.

    python3 tools/aseprite/compose_gait.py --accept
    python3 tools/aseprite/pipeline.py --author
    make GAME=maiya art

Without Aseprite, `ASEPRITE=builtin` makes `pipeline.py` author, reopen and
export with `tools/aseprite/ase_file.py`, which writes the same
`maiya_walkrun.aseprite` byte for byte and the same exports (only the sheet
JSON's `meta.app` names it).

`--accept` records the composed layers in `cute-polish.json` as the accepted
art, so any later hand edit to a map fails `check_cute_polish.py` until it is
reviewed and accepted. Edit the drawings in `compose_gait.py` (the arms are
literal pixel rows there) or in the source sheets, never only in `maps/`.

## How each frame is made (2026-10-12 redraw)

- **Head and tunic** -- her stride figure from the original heroine sheet
  (`maps/base__head_hair.txt`, `maps/base__torso.txt`), the same drawing as
  her throws, jumps and landing, moved only by the whole-pixel bob.
- **Legs** -- one leg pair per phase, cut from her original drawings in
  `assets/source_art/WALK.png` and `RUN.png`, so every boot, knee and thigh
  is hand-drawn and both legs wear the same boots and colours (no darker far
  leg: that was the "shadow of a knee"). Each leg goes through every phase in
  turn and the other leg is half a cycle behind it, so one is up while the
  other is down. The pair is split into far and near layers along the gap
  between the boots; the B half uses the same drawing with the layers
  exchanged. The thighs run up under the tunic and are clipped to it, so no
  bob or shift opens a gap at the hem. Each boot band carries the idle
  outfit's two-pixel gold glint.
- **Arms** -- ten pixel drawings in `compose_gait.py`: five for the walk's
  relaxed swing, five for the run's pump (elbow near 90 degrees). Each is a
  short green sleeve with its pink trim, the bare upper arm, the brown bracer
  lit along one edge and shadowed along the other, and a small closed fist
  joined to the bracer at the wrist. The near arm (shoulder 47,26) swings
  against the near leg. The far arm, half a cycle behind it, takes the same
  path from a shoulder just behind and above it (45,25): so, as in her
  original stride drawing, one fist pumps ahead of her chest while the other
  arm is swung back with its elbow out past her back and its fist behind her
  hip, on every frame (the far fist ahead of her chest on its forward swing,
  her elbow behind her back on its back swing). Swinging back, each arm's
  shoulder rolls back with it (the sleeve tops move from x 47 to 42).
  Each fist moves at most 7 px a frame, the run's bob included
  (`tools/check_hero_frames.py`). Stopped, the far arm hangs at her back
  (shoulder 36,26).
- **Timing** -- each pose is held for the road its planted foot covers in
  the drawing, with the legs shifted at most 4 px under the tunic, so the
  planted foot stays put: at most 3 px of slip a pose at a walk, 1 px at a
  run (`check_stride_details.py` measures it). The same holds are in
  `scenes/maiya_presentation.h`.

## Poses

| Slot | Walk | Legs from | Leg shift | Bob | Arms (near) | Hold |
| --- | --- | --- | ---: | ---: | --- | ---: |
| 0 / 4 | contact | WALK 0: both feet down | 0 | 0 | ahead of the skirt / behind it | 8 |
| 1 / 5 | down | WALK 2: weight on the front foot, back on its toe | -3 | +1 | halfway | 10 |
| 2 / 6 | passing | WALK 1: support under the hip, the other heel lifting | -2 | 0 | hanging at her side | 14 |
| 3 / 7 | up | WALK 5: support behind, the swing knee lifted forward | +3 | -1 | halfway | 8 |

| Slot | Run | Legs from | Leg shift | Bob | Arms (near) | Hold |
| --- | --- | --- | ---: | ---: | --- | ---: |
| 0 / 4 | contact | RUN 0: landing ahead, the back foot pushing | 0 | 0 | fist at the chest / elbow out past her back, fist behind the hip | 10 |
| 1 / 5 | down | RUN 1: support under her, the other heel kicking up | 0 | +2 | fist coming down ahead of her | 14 |
| 2 / 6 | push-off | RUN 5: support extended behind, the knee driving through | +4 | 0 | hanging at her side | 8 |
| 3 / 7 | flight | RUN 3: the trailing leg folded, the leading one reaching; soles 4 px up | 0 | -2 | elbow going back past her back | 8 |

Holds are gait clock units (80 per two-step cycle); the clock runs 5/4 of
the road at a walk and 1 at a run. Stop: the earlier stop legs (feet
together) with both arms hanging; held MG_GAIT_STOP (6) frames, then idle.

## Superseded passes (history)

Everything below describes the 2026-10-10/11 versions that the redraw
replaced; their tools stay for reference only.

### How each frame was made (2026-10-11)

- **Head and tunic** -- her stride figure from the original heroine sheet
  (`reference_stride.png`), the sheet her throws, whip, jumps, landing and
  victory pose are cut from. Identical in every frame, moved only by
  whole-pixel bob, so her face never changes and matches her other moves.
- **Arms** -- three positions, with the idle drawing's green sleeves, pink
  sleeve trim and brown wrist bands, followed by visible skin palms:
  - 0, reach: the stride's arms (front fist forward at her chest, back hand
    behind her);
  - 1, halfway: the front fist lowered 4 px at the wrist, the back arm
    drawn 2 px in and down toward her side;
  - 2, hanging: the arms of her passing figure (`reference_pass.png`, the
    sheet's first jump-row drawing), hands by her hips.
  The arm nearest the viewer is drawn over her tunic, the other behind
  her. In the A half the near arm starts forward; in the B half the near arm
  is the one behind and swings forward -- so her arms alternate with her
  legs, each fist travelling 7-10 px over a step.
- **Legs** -- accepted walk, run contact and flight outlines retained from
  the native `assets/source_art/WALK.png` and `RUN.png` cells. The run's down
  and passing/push recovery legs are now authored as complete indexed pixel
  rows: heel recovery, forward knee drive, folded shin, then extension.
  Each physical foot follows the entire path before the other foot repeats it.
  Near legs have short highlights and warm knee rims; far legs use the darker
  shades of the same ramps. Boot bands carry the idle outfit's small glints.
- **Hair** -- `tools/hair_specks.py` turns stray cream/white/pink specks in
  her hair into hair colour (Luna wears the same pixels, where they showed
  as light dots on her brown hair).

2026-10-11 finishing pass: `tools/aseprite/polish_skin.py` applies exact
color-only skin highlight pixels to the accepted maps. Hands and knee/thigh
clusters use the existing light skin index 6 with small warm index-9 edges,
instead of broad brown skin patches. No outline, silhouette, boot, head,
tunic, layer position, root, pose, duration or palette change.
`skin-polish.json` records the pixel edits; `skin-preservation-check.json`
records the unchanged accepted shapes and upper body. Rebuild in Aseprite
with `pipeline.py --author`, then run the normal hero and decoded-ROM gates.

2026-10-11 hands follow-up: `tools/aseprite/repair_hands.py` draws six small
native hand patches directly into the reviewed maps. Index 6 is the palm,
9 a small warm shadow, and 1 its outline. The palms join the accepted wrists;
no new arm, detached hand or frame overlay is used. `hand-repair.json` records
their boxes and exact pixel edits. The far leg's skin highlight uses index 9
so exchanging the near and far feet reads clearly; both leg outlines and boots
stay exact. The head, torso, near leg, ground row, pose order and holds stay
unchanged. This replaces the earlier wrist-only tips, rather than re-running
the historical assembly script.

`tools/check_hands.py` checks the visible palms after all layer occlusion,
requires an actual opposite leading boot between contact halves, and rejects
arms leading on the same physical side as their contact leg. Both isolated
build paths run this extra check. The existing strict art and C-ROM checks
remain unchanged. `hand-check-result.json` records the latest check;
`build/hands-20261011/candidate/hand-check-result.json` also compares against
the accepted pre-edit head, torso, leg shapes and pose/palette metadata.

Layers, bottom to top: `far_arm`, `far_leg`, `near_leg`, `torso`,
`head_hair`, `near_arm`, plus the hidden, locked `ref` layer.

### Poses (2026-10-11)

| Slot | Walk | Legs from | Bob | Arms | Near arm | Hold |
| --- | --- | --- | ---: | ---: | --- | ---: |
| 0 / 4 | contact | WALK 0 (both feet down) | 0 | 0 reach | front / back | 12 |
| 1 / 5 | down | WALK 0 | +1 | 1 halfway | front / back | 8 |
| 2 / 6 | passing | WALK 1 (swing foot lifted beside the support foot) | 0 | 2 hanging | front / back | 10 |
| 3 / 7 | up | WALK 0 back leg (support), WALK 0 front leg raised 2 px | -1 | 1 halfway | back / front | 10 |

| Slot | Run | Legs from | Bob | Arms | Near arm | Hold |
| --- | --- | --- | ---: | ---: | --- | ---: |
| 0 / 4 | contact | RUN 0 | 0 | 0 reach | front / back | 8 |
| 1 / 5 | down | support moved 1 px back; authored recovering knee and tucked heel | +2 | 1 halfway | front / back | 10 |
| 2 / 6 | passing / push-off | support moved 4 px forward; authored forward knee with shin folded back | 0 | 2 hanging | front / back | 8 |
| 3 / 7 | flight | RUN 3 (both feet 4 px off the ground) | -2 | 1 halfway | back / front | 14 |

Stop: WALK 1's support leg and its other foot set down beside it, arms
hanging. She holds it MG_GAIT_STOP (6) frames after she stops, then her idle
pose.

Holds are gait clock units (80 per two-step cycle). The clock runs 5/4 of
the road at a walk -- a walking cycle covers 64 px, the length of her drawn
step, so her planted foot barely slides -- and 1 at a run (80 px).
`scenes/maiya_presentation.h` holds the same tables.

2026-10-11 stride/detail correction: `tools/aseprite/refine_stride.py` defines
the six complete arm maps and two complete recovery legs as exact indexed
pixel rows. It preserves head/torso pixels, walk/contact/flight leg outlines,
canvas, palette, root, bob and hold distances. It also places the run's
support soles to follow the existing 8/10 px contact/down holds within 1 px
between pose samples. It does not alter the runtime clock or physics.
`stride-details.json` records every pixel write; the maps and reopened
`.aseprite` file contain the final pixels. The earlier polish/hand manifests
above are history; the new pass restores the missing clothing details and
warm knee shading rather than repainting wrists as exposed skin.

`tools/check_stride_details.py` measures both physical feet over each half,
rejects a stationary/reversing recovery foot, checks that the raised shin
folds back at least 3 px, and verifies sleeve/trim/wrist/boot details. Together
with `check_hands.py`, the unchanged strict anatomy gate and the decoded-ROM
gate, it runs in both the normal art build and isolated Maiya build paths.
`stride-check-result.json` records the results. These checks accompany the
contact sheet and GIF review; they do not establish subjective acceptance.

2026-10-11 cute finishing pass: `tools/aseprite/polish_cute.py` applies 679
small indexed colour writes to the approved stride. Palms have uninterrupted
cream tops, a small warm lower rim and a dark-brown contour; shoulders and
elbows use continuous highlights. Both physical knees retain connected warm
rims and short highlights, with a smaller light area on the far leg. Boot
interiors have quieter highlights. All 102 layer silhouettes, joint/foot
positions, head/hair/tunic pixels, root, bob, palette, pose order and game
holds remain exact. Luna wears these same indices through her existing
palette; no separate mirrored drawing is introduced. The reviewed maps are
the final source; do not rerun the historical assembly or preceding patch
scripts over them. `cute-polish.json` records every exact colour write and
the preceding/current layer hashes.

`tools/check_cute_polish.py` reconstructs every preceding layer from that
evidence, verifies unchanged silhouettes and motion metadata, and checks the
reopened Aseprite exports against the reviewed maps. It runs alongside the
unchanged anatomy, hand, stride and decoded-ROM gates in the art/build paths.
It also outputs Maiya/Luna contact sheets and full-frame GIFs, using cumulative
10ms deadlines for the GIF format and clearing each complete frame. This
corrects preview rounding/ghosting without changing the accepted runtime
clock (walk 533.3ms per cycle; run 426.7ms at their target speeds).

### Final per-pose measurements (2026-10-11)

Root is canvas (40,62), rendered at offsets (-40,-62). Ground is Y=61.
Fists and feet below are measured from the exported native pixels. Holds are
clock distance units; walking road distance is hold x 0.8, running is hold.

| Pose | Support / planted-foot X | Contact rows (near, far) | Bounds (L,T,R,B) | Fists (near; far) | Hold | Motion |
| --- | --- | --- | --- | --- | ---: | --- |
| walk_contact_a | far_leg: 52.1 | 61, 61 | [21, 10, 60, 61] | [58.3, 28.2]; [28.2, 35.4] | 12 | contact |
| walk_down_a | far_leg: 49.1 | 60, 61 | [21, 11, 58, 61] | [56.6, 33.4]; [31.7, 36.4] | 8 | down |
| walk_passing_a | far_leg: 40.5 | 59, 61 | [24, 10, 57, 61] | [52.0, 36.6]; [34.5, 37.2] | 10 | passing |
| walk_up_a | far_leg: 30.8 | 59, 61 | [24, 9, 58, 61] | [31.7, 34.4]; [56.6, 31.4] | 10 | up |
| walk_contact_b | near_leg: 52.1 | 61, 61 | [21, 10, 60, 61] | [28.2, 35.4]; [58.3, 28.2] | 12 | contact |
| walk_down_b | near_leg: 49.1 | 61, 60 | [21, 11, 58, 61] | [31.7, 36.4]; [56.6, 33.4] | 8 | down |
| walk_passing_b | near_leg: 40.5 | 61, 59 | [24, 10, 57, 61] | [34.5, 37.2]; [52.0, 36.6] | 10 | passing |
| walk_up_b | near_leg: 30.8 | 61, 59 | [24, 9, 58, 61] | [56.6, 31.4]; [31.7, 34.4] | 10 | up |
| run_contact_a | far_leg: 51.8 | 61, 61 | [21, 10, 60, 61] | [58.3, 28.2]; [28.2, 35.4] | 8 | contact |
| run_down_a | far_leg: 44.0 | 55, 61 | [24, 12, 58, 61] | [56.6, 34.4]; [31.7, 37.4] | 10 | down |
| run_push_a | far_leg: 34.0 | 57, 61 | [24, 10, 57, 61] | [52.0, 36.6]; [34.5, 37.2] | 8 | push |
| run_flight_a | none | 57, 55 | [19, 8, 60, 57] | [31.7, 33.4]; [56.6, 30.4] | 14 | flight |
| run_contact_b | near_leg: 51.8 | 61, 61 | [21, 10, 60, 61] | [28.2, 35.4]; [58.3, 28.2] | 8 | contact |
| run_down_b | near_leg: 44.0 | 61, 55 | [24, 12, 58, 61] | [31.7, 37.4]; [56.6, 34.4] | 10 | down |
| run_push_b | near_leg: 34.0 | 61, 57 | [24, 10, 57, 61] | [34.5, 37.2]; [52.0, 36.6] | 8 | push |
| run_flight_b | none | 55, 57 | [19, 8, 60, 57] | [56.6, 30.4]; [31.7, 33.4] | 14 | flight |
| stop | near_leg: 41.5 | 61, 61 | [24, 10, 57, 61] | [52.0, 36.6]; [34.5, 37.2] | 0 | settle |
