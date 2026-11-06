# Shadow League native character direction

This is the direction for the three playable proofs, not artistic approval.
Judge the actual indexed exports and cartridge motion at 320x224. The existing
arenas remain outside the redraw scope. No Aseprite step is used.

## Shared drawing standard

Use native 160x160 fighter canvases with root [80,141]. Grounded supporting soles
touch row 141. Whole-body airborne height comes from the combat engine; authored
floor and inverted poses have their own contours. Keep a clear neck, rib cage,
pelvis, supporting knee and recognizable closed fist or open palm. Clothing
must wrap those forms rather than replacing their structure with tubes.

The principal light is above and toward the front of the fighter. Use a deep
indigo or wine outline, cool far-side shadows and warmer face/hand highlights.
Preserve the light direction when the game mirrors a sprite; inspect this
hardware limitation in mirror matches instead of adding per-frame random light.
Avoid interior black lines that split a forearm or leg into disconnected pieces.
Highlights should describe knuckles, cheekbones, cloth planes and shoe toes.

All work stays on the integer grid. No interpolation, soft edges, rotations,
non-integer resizing, generated in-betweens or painting reduction. Deliberate
clusters and explicit native contours define the image. The current recipes
compose specified parts; further artistic production should improve complete
action drawings and body mechanics, rather than treating composition as the
finished quality standard.

## Three independent identities

| Fighter | Shape and costume | Motion and attack identity | Review priority |
| --- | --- | --- | --- |
| Kenji Arata | Lean angular face, red headband, white gi, crossed collar, dark belt, bare hands/feet; wine accents and blue-gray cloth folds | Stable karate guard, alternating grounded step, reverse punch, chambered kick, planted supporting foot | Hip/shoulder connection, gi folds around bent elbows, fist orientation and heel pressure |
| Jack Mercer | Broader jaw/neck/shoulders, short hair, red vest and gloves, dark striped trousers and boxing boots | High boxing guard, advance/gather footwork retaining lead, jabs, crosses, hooks and uppercuts; button attacks remain punches | Glove size, wrist continuity, heel lift, shoulder roll and differentiated heavy preparation |
| Amara Silva | Distinct warm complexion, tied hair, gold details, green top and pale trousers; clear waist and pelvis | Ginga weight shift, open palms, rotational kicks, separately drawn floor/inverted transitions | Supporting hand contact, pelvis rotation, two-leg readability and continuous hair/costume identity |

The same head is retained through standing actions using exact integer placement;
floor and inverted heads are separate drawings. Head lighting and expression
must remain individual. Do not mirror a face inside a pose to fake a leg change.

## Palette ownership

The embedded palette of each active PNG is authoritative. Current initial
source ramps are also recorded in `tools/native_art/showcase_ink.py`:

| Index | Material role |
| --- | --- |
| 0 | Transparent |
| 1 | Main outline |
| 2..6 | Skin shadow, half-tones and selective highlights |
| 7..9 | Hair/deep trim |
| 10..12 | Character accent cloth/gloves |
| 13..16 | Main cloth folds and highlights |
| 17..18 | Footwear/deep trim |
| 19..20 | Gold/cream details |
| 21..23 | Extra material neutrals |

These are global source indices, not one hardware palette. The importer assigns
each 16x16 tile to a bank with at most fifteen opaque colors. Current counts are
Kenji 2, Jack 2 and Amara 4 banks; the fighter limit remains four. A new color or
tile combination that cannot fit fails the build. Never quantize an approved
fighter to make the check pass. Mirror palettes must preserve material roles.

## Editable source and timing

Active sources: `assets/authored/fighters/<id>/manifest.json`, six layer PNGs per
pose and matching flattened PNGs. The layer order is far_arm, far_leg, torso,
head_hair, near_leg, near_arm. Initial `.pixels` files document exact original
indices; the PNG layers are what artists edit. Older `models` and `footwork`
studies are references, not active sources.

Use `tools/native_art/recompose_fighter.py <folder> [--pose <name>]` after an edit.
Update the manifest's hurt/contact boxes deliberately when anatomy or reach
changes. Timings and move flags belong to `fighters/<id>.json`. Preview GIFs are
snapshots; rebuilding a flat PNG alone does not refresh them.

The forward eight-drawing cycle holds 5,3,4,5,5,3,4,5 simulation frames. Reverse
order is 4,3,2,1,0,7,6,5 with one extra frame per drawing. Contact points exist,
but exact foot speed has not been proven against world movement. Review this
at normal speed and in slow motion before accepting the cycle.

## Acceptance before roster expansion

Show the actual game at native size, 2x nearest-neighbor and real speed. Review
each normal, special, super, reaction and throw with contact overlays. Reject
unreadable fists, unrelated cloth shadows, sliding support feet, misplaced hips,
clipped limbs, weak anticipation or an action whose contact does not match its
visible hand/foot. Review both facing directions and alternate palettes.

Technical checks establish indexed integrity, connected layer pixels, palette
limits, contact metadata and encoded-ROM reconstruction. They cannot establish
appeal, anatomy or martial-arts credibility. The three playable proofs still
need that independent review before the remaining 41 characters are redrawn.
