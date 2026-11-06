# Improvement v2

## Objective

Make Maiya a coherent native-resolution arcade platformer: expressive character
animation, richly layered scenery, precise movement, readable combat, and stable
sprite rendering. The visual direction is cinematic ninja adventure: strong
silhouettes, deliberate pixel clusters, detailed environments, and restrained,
well-timed effects. Keep the existing characters and each level's identity.

This is a production plan, not a claim that the improvements are implemented.
No renderer, artwork, ROM, sound driver, release asset, or build setting is changed
by this document. Implementation remains uncommitted until approved and tested.

## 1. Establish the Reference Build

- Save the source state and hashes of P1, M1, V1, C1, C2, and S1 before changes.
- Record repeatable input sequences for running, reversing, jumping, climbing,
  combat, camera boundaries, doors, bosses, swimming, flying, and transitions.
- Capture native-resolution consecutive frames, not only attractive stills.
- Record CPU/frame time, upload cost, sprite-slot ownership, maximum strips per
  scanline, and palette ownership. A good average must not hide a bad peak.
- Preserve working MVS sound, AES startup/menu behavior, jump trajectories,
  controller shortcuts, and existing boot graphics reservations.

Begin with one 60-90 second Emerald Forest section containing a safe opening,
platforms, climbing, a waterfall, an enemy encounter, and a correctly scaled door.
Do not rebuild all levels before this section passes the visual and timing gates.

## 2. Pixel-Art Direction

Create a small art bible before drawing replacement frames or scenery:

- Work at the game's native pixel scale. Present enlarged previews with nearest
  neighbor; do not confuse filtered screenshots with better source artwork.
- Lock character proportions, head size, limb lengths, weapon scale, and the
  relationship between player height, ledges, doors, enemies, and bosses.
- Use deliberate clusters and directional highlights, not noisy photographic
  texture reduced into a tiny palette. Preserve readable faces and hands.
- Give each material an intentional color ramp: skin, cloth, metal, bark, stone,
  foliage, and water. Choose colors for separation as well as fidelity.
- Establish one light direction per environment. Background contrast and detail
  must support the characters, not compete with them.
- Separate far silhouettes, middle-distance texture, and playable terrain with
  value and saturation. Depth must remain readable without a palette effect.
- Retain recognizable scenery and character identities while redrawing weak
  details. Do not replace the entire visual identity in one conversion pass.

Review source and converted artwork side by side at native size, enlarged pixel
size, and under the intended CRT presentation. Conversion cannot repair missing
animation or poorly designed silhouettes.

## 3. Real Animation, Not Extra Frame Names

The current eight walking entries reuse four source poses. Add actual drawings,
not duplicate entries, blended screenshots, automatic body warps, or independently
resized limbs.

First deliver an 8-12 pose run cycle on the existing 80x64 character canvas:

1. Plant and compress.
2. Push off and extend.
3. Passing pose with a readable knee and opposite arm.
4. Flight/recovery.
5. Opposite contact, with the corresponding weight transfer.
6. Intermediate poses that carry those actions smoothly through the cycle.

Specify per-frame foot contact, root anchor, silhouette bounds, hand/weapon
attachments, duration, and motion state. Keep all strips of a pose in the same
frame. Stable root placement does not mean freezing every anatomical landmark;
the hips, shoulders, hands, and hair must move naturally relative to the root.

- Preserve collision dimensions and the working jump arc during the run pass.
- Advance ground gait from actual distance travelled, not camera movement or a
  reverse countdown. Returning to idle must not rewind through the run poses.
- Tune phase durations around contact and push-off; do not assume every drawing
  deserves an equal hold. Pause the gait appropriately when blocked by a wall.
- Draw a short start and stop transition after the run cycle is accepted.
- Improve climbing hand/foot contacts against the vine with authored poses.
  Replace air/landing art separately without changing jump timing by accident.
- Keep hurt and attack poses distinct, with anticipation and recovery that remain
  responsive. Presentation changes must not silently move attack windows.
- Keep character scale constant during ordinary play. Reserve scaling for an
  explicit depth demonstration or authored special event.

Acceptance: no detached feet, frame-to-frame size changes, sliding root, duplicated
limbs, leftover strips, or hand motion inconsistent with direction and gait.

## 4. Multi-Layer Scenery Built for the Hardware

The current far painting and road are two horizontal bands. They are not two
full-screen overlapping planes. Replace the appearance of a flat painting with
separable logical layers, without pretending the hardware provides unlimited
background planes.

| Logical layer | Content | Starting scroll ratio |
| --- | --- | --- |
| Backdrop | Stable environment color | None |
| Distant scenery | Sky, mountains, distant forest | 1/8 or 1/4 |
| Far vegetation | Canopy silhouettes and distant trunks | 1/2 |
| Midground islands | Rocks, ruins, falls, selected trunks | 3/4 |
| Playfield | Terrain, vines, doors, actors, collisions | 1 |
| Sparse foreground | Occasional leaves, reeds, branches | 5/4 |
| Effects and UI | Local impacts; FIX text | World or screen anchored |

These ratios are initial art-direction values, not final constants. Give every
layer a world origin, scroll ratio, wrap policy, vertical range, and slot owner.
Do not implement this as five full-width overlapping sprite paintings.

- Use one bounded wide scenery window and sparse sprite-built object islands.
  Transparent gaps are useful visually, but live transparent strips still cost
  hardware capacity; hide or omit strips that are not required.
- Cull islands using their rendered bounds and a small guard region. Recycle
  owned windows before they enter view, without rewriting the entire painting.
- Keep terrain, doors, enemies, and collision coordinates on the same playfield
  transform. A decorative parallax trunk must not become a collision reference.
- Support vertical camera travel deliberately: new upper/lower scenery chunks,
  suitable wrap rules, and no empty bands at camera limits.
- Put foreground objects ahead of actors only when the art requires it. Never
  accidentally hide the playable character behind a whole background image.
- Keep feet, landing surfaces, projectiles, and hazards readable. Design partial
  occlusion as a composition decision, not a side effect of slot allocation.

Optional read-only reference analysis may examine available arcade traces,
sprite-control changes, camera ratios, palette ownership, and animation timing.
Record observations and derive independent techniques. No such disassembly has
been performed for this plan; do not present a hypothetical comparison as evidence
or copy commercial art, code, or sound into the game.

## 5. Measured Sprite and Upload Budgets

The planning ceiling is 96 sprite strips per scanline and 381 displayed sprite
slots. A strip still counts when its pixels are transparent or horizontally
shrunk. Accurate height and window ownership matter as much as the visible image.

Use an initial worst-line target of at most 80 strips, leaving headroom. Example
allocation for a busy line, to validate against the actual renderer:

| Contributor | Proposed strip allowance |
| --- | ---: |
| Wide distant window, including guards | 22 |
| Far vegetation islands | 8 |
| Midground islands | 6 |
| Terrain details | 6 |
| Player, enemies, boss allocation | 24 |
| Sparse foreground | 4 |
| Effects and projectiles | 6 |
| Sprite-based HUD | 4 |
| Total planning target | 80 |

FIX text is separate from this sprite allowance. This table is not proof of
capacity: count actual live strips, vertical coverage, wrap behavior, and slot
order for every line. The current 32-strip far band must be measured as it exists;
do not assume a future 22-strip active window is already implemented.

- Audit all existing ranges before adding a layer; the current slot map already
  uses much of the displayed range. Title and gameplay ownership differ.
- Reserve the whole player and essential projectile groups. Never display only
  part of a character because an allocator or queue runs out of space.
- Reduce optional foreground islands, ambient particles, or distant detail before
  essential actors. Do not alternate visibility frame by frame to fake capacity.
- Keep VRAM storage capacity distinct from displayed-slot capacity.
- Count upload words and measured cycles. Palette uploads, animation changes,
  FIX updates, interrupts, and sound communication share the frame budget.

## 6. Tear-Free Rendering Architecture

Retain the existing stale-tail fixes and two-phase ownership cleanup. Do not
restore full-screen clearing every frame or clear a 32-strip window for a small
character on every pose change.

The intended frame pipeline is:

```text
input -> fixed-rate simulation -> camera snapshot -> visibility/budget plan
      -> complete render commands -> scheduled VRAM/palette commit
```

- Use one render-camera snapshot for every object in a frame.
- Give every object a reserved window and a known active strip count. Strip zero
  drives the chain; subsequent strips chain only inside that object's window.
- On shrinking or hiding a group, terminate stale strips and clear their chain
  state before another owner reuses them. Do not hide valid live strips as a
  routine step of every position update.
- Stage a complete pose's tile map and attributes. Within the available commit
  window, publish the complete group; if it cannot fit, retain the last complete
  pose. A software transaction is not a hardware double buffer.
- Update only dirty SCB fields. Preload static maps while safely hidden rather
  than repeatedly uploading them during scrolling.
- Align visible updates with measured display timing. Merely calling a function
  named VBlank-safe is not proof that its writes finish before the deadline.
- Use bounded CPU VRAM-port writes; do not design around an assumed generic
  sprite DMA facility.
- Make render-queue overflow observable and actionable. Complete the command
  types actually supported by its flush path before expanding its use.
- Snapshot queued palette colors into owned storage. Do not retain pointers to
  stack-local buffers until a later flush.
- Scene transitions hide previous owned ranges once, then initialize the next
  owners. Palette loading must never draw hidden sprites to obtain their colors.

Keep shared C engine behavior authoritative. Any C++ wrapper uses the same
ownership, capacity, and commit semantics, not a separate rendering model.

## 7. 32-Bit Precision Where It Helps

Use signed 32-bit fixed-point for long-lived camera positions, layer world
positions, and distance accumulators. Choose a documented representation, such
as eight fractional bits in an int32_t accumulator, with tested world bounds.
This preserves fractions that would otherwise produce uneven parallax steps.

- Preserve the existing physics representation and jump behavior initially.
  Convert at an explicit presentation boundary instead of replacing all motion
  fields with a new format in one patch.
- Compute world minus camera in fixed-point, then round once at projection.
  Use one rounding rule for actors, platforms, vines, and doors.
- Implement common parallax ratios with bounded additions/shifts or precomputed
  coefficients. Define behavior for negative positions and wrap boundaries;
  do not rely on ambiguous signed shifts or truncation toward zero.
- Use bounded integer easing for camera follow, with a defined dead zone,
  maximum catch-up speed, reversal behavior, and arena clamps.
- Keep render offsets out of physical collision coordinates. Never hide a
  collision discontinuity with sprite interpolation.
- Avoid floating-point work, general division, and wide multiplication in hot
  loops unless profiling demonstrates they are affordable. Use lookup tables
  for small periodic motions where appropriate.
- Keep tile indices, counts, and hardware fields narrow. Wider arithmetic helps
  precision; it does not fix wrong anchors, bad art, or insufficient sprite budget.
- Run simulation once per native game frame, not in proportion to host speed.
  Do not add a frame of input delay just to obtain visual interpolation.

## 8. Environmental Animation and Palette Ownership

Add localized animation that communicates life without filling the screen with
effects: waterfall flow, pool ripples, a few drifting leaves, cloth, lanterns,
and occasional wildlife.

- Author four or eight aligned waterfall frames where hardware tile
  auto-animation is appropriate. Lay out tile indices and attributes explicitly;
  verify all strips stay synchronized.
- Give ripples and foliage their own short sequences with different cadences.
  Do not make the whole level pulse on a single timer.
- Reserve dedicated water/light inks and effect palettes. Do not discover two
  nearest blues in a shared decorative palette and rotate unrelated objects.
- Maintain stable character palettes across frames and neighboring tiles.
- Use palette cycling only on intentionally designed ramps. The base art must
  remain attractive and readable when cycling is disabled.
- Design particles as a small, consistent vocabulary: landing dust, leaf drift,
  weapon sparks, water splash, and hit impact. Each has a distinct trajectory,
  lifespan, size, and purpose; avoid random full-screen noise.
- Keep effects attached to authored hand, weapon, impact, or surface anchors.
  Render an impact over the target, not over the player's entire body.

## 9. Artbox Fidelity and Asset Contracts

Improve the Maiya asset build first; change shared Artbox only after establishing
tests that protect other games.

- Fit palettes across related frames and scene materials, then assign compatible
  palettes per tile. Tiles in one strip may have different palette selections;
  enforce continuity of shared edges, not a false one-palette-per-strip rule.
- Reserve index zero for transparency. Keep visible black in a nonzero entry.
  Do not globally replace transparent padding with white or black pixels.
- Exclude transparent padding from palette fitting. Review semi-transparent
  edges separately so background contamination cannot create pale halos.
- Preserve authored pixel clusters. Choose dithering by asset type and compare
  results; do not force one global contrast/stretch/dither treatment on all art.
- Emit authoritative metadata: tile base and bounds, stride, strips, rows,
  content bounds, pivot, palette assignments, frame duration, and asset type.
- Add animated-group metadata and explicit attachment/hitbox points rather than
  guessing geometry from a filename or atlas cell.
- Compare reconstructed C1/C2 output to the source and intermediate indexed
  preview. Catch tile order, plane encoding, wrong bank, and stale header errors.
- Protect the AES boot tile reservation, private FIX region, and sample layout.
  Keep tile IDs stable where possible; if repacked, regenerate dependent headers
  and C ROMs together, never mix generations.

## 10. Implementation Locations

| Area | Primary files |
| --- | --- |
| Scene composition, ownership, animation integration | games/maiya/scenes/maiya_game.c |
| Gait and palette presentation helpers | games/maiya/scenes/maiya_presentation.h |
| Level placement and layer definitions | games/maiya/levels/*.json; games/maiya/tools/levels.py |
| Character frames, palette fitting, generated metadata | games/maiya/tools/build_commercial_assets.py |
| Environment and prop art | games/maiya/tools/nature_art.py; existing source asset folders |
| Atomic build consistency | games/maiya/tools/build.py |
| Queue capacity, payload lifetime, scheduling | sdk/2d_engine/ng_render_queue.h and .c |
| Sprite chains and object windows | sdk/2d_engine/ng_sprite_group.c; sdk/2d_engine/ng_chars.c |
| Camera and fixed-point support, if shared changes are needed | sdk/2d_engine/ng_camera.c; sdk/2d_engine/ng_fixed.h |
| Verification | games/maiya/tests; games/maiya/tools/regression.py; tests/test_maiya_presentation.py |

A separate layer module is justified only if it removes real duplication from
the scene code. Do not create a second general engine for one presentation pass.

## 11. Delivery Sequence

1. Baseline and counters: capture existing behavior, ownership, and worst-case
   load. Create regression cases before touching shared rendering behavior.
2. Animation prototype: draw and review the genuine run cycle on a plain stage;
   integrate with unchanged jumping and consistent anchors.
3. Renderer foundation: fix proven queue/ownership defects with focused tests;
   measure the commit budget and stop silent partial updates.
4. Forest composition: separate one environment into bounded layers, align
   playfield objects, and validate both camera directions and vertical travel.
5. Living scenery: add waterfalls, ripples, restrained palette animation, and
   coherent local effects under measured capacity.
6. Playable slice: test movement, climbing, combat, doors, transitions, and a busy
   encounter together. Review consecutive frames for tearing and stale limbs.
7. Expand only the accepted approach to other levels, enemies, bosses, swimming,
   and flying. Keep commits small when implementation is eventually approved.

## 12. Acceptance Gates

- Every new run pose is a distinct drawing with reviewed anatomy and silhouette.
- Run, reversal, climbing, and landing show no detached or lingering strips.
- No player/enemy disappears because of background ownership or capacity loss.
- No torn pose during flips, culling, camera wraps, doors, or boss transitions.
- Measured scanline usage stays within the chosen margin and hardware limit;
  optional effects cannot cause critical groups to be partially drawn.
- Queue overflow and timing overruns are detectable; stress tests exercise both.
- Scroll seams and camera discontinuities are absent in consecutive captures,
  including reverse travel and top/bottom level boundaries.
- Source/converted previews preserve silhouettes, ramps, transparency, and shared
  frame palettes. MAME output agrees with the reconstructed asset preview.
- Character, door, terrain, and boss scale is intentional and consistent.
- MVS/AES boot and audio retain their known-good behavior. The sound driver and
  established 8 MiB sample-region packaging remain outside this graphics pass.
- Verify the final slice on hardware as well as MAME; emulation alone cannot
  certify cartridge timing, controller shortcuts, or cabinet presentation.

The release decision follows these gates, not a promised frame count, layer
count, or screenshot. The first deliverable is a polished, repeatably stable
forest section that establishes the standard for the rest of the game.
