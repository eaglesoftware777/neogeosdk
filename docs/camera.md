# Camera System

## What It Does

`NGCamera` (Stage 5) provides smooth-follow, look-ahead, dead zone, camera shake,
and cinematic pan on top of the existing `NGLevelState` scroll.

## Why It Matters on NeoGeo

The NeoGeo has no hardware scroll register for all sprites.  "Scrolling" is
implemented by adjusting SCB4 (X position) and SCB3 (Y position) for every
sprite group and background layer.  Abrupt per-frame jumps look jarring.
The camera applies sub-pixel interpolation so motion is perceptually smooth
at 60 fps even when the player is accelerating.

## Hardware Limitation It Solves

- Sub-pixel position storage (16.16 fixed-point) prevents the "1-pixel snap"
  stutter that integer-only camera causes at slow scroll speeds.
- Shake pattern uses a pre-baked 8-entry array — no random calls, no division.

## Key Parameters

| Parameter        | Type    | Description |
|-----------------|---------|-------------|
| `follow_speed`  | uint8_t | 1=very slow, 255=instant snap, 64=smooth arcade feel |
| `dead_zone_x/y` | uint8_t | pixels target can move before camera follows |
| `look_ahead_x`  | int16_t | horizontal look-ahead in pixels |
| `shake_amp`     | uint8_t | max shake amplitude in pixels (2-4 recommended) |

## Example

```c
NGCamera cam;
ng_camera_init(&cam);
ng_camera_set_bounds(&cam, 0, 0, 2047, 224);
ng_camera_set_follow_speed(&cam, 0x30);
ng_camera_set_dead_zone(&cam, 32, 16);

// Per frame:
ng_camera_apply(&cam, player->x, player->y, player->vx_fp >> NG_FP_SHIFT);
```

## Common Mistakes

- Forgetting to call `ng_camera_set_bounds()` after `ng_level_set_world_bounds()`:
  the camera will not clamp to world edges.
- Setting `follow_speed = 255` (instant): looks like no camera system at all.
  Use 32..80 for arcade smoothness.
- Shake amplitude > 6 pixels: creates nausea on a CRT.  Keep it 2..4.

## Performance Advice

- One `NGCamera` per scene.  No need for multiple camera instances.
- `ng_camera_apply()` calls `ng_level_set_scroll()` internally — no extra work.
- Camera shake costs zero at rest (just a conditional on `shake_frames`).
