# NeoUniversal Video Orchestration Model

This note documents the scene/video ownership model added for the NeoGeoSDK demo and engine layers.

## Purpose

Neo Geo software is visually powerful, but many systems can write to the same hardware state:

- generated `showScreenN()` functions
- engine character sprite chains
- raw pseudo-3D floor effects
- software raycaster columns
- MVS eyecatcher code
- FIX text/HUD output
- palette loading

When those systems are called directly, sprites can remain partially enabled, old SCB3 sticky-chain data can survive between scenes, and the next scene may show only one vertical strip or become visually occluded.

The new model separates ownership:

```text
sdk/ng_video/   scene-level video ownership, clears, captions, sprite ranges
sdk/ng_show/    safe wrapper for generated showScreenN() art
sdk/ng_scene/   scene sequencing and manual advance flow
sdk/ng_audio/   sound-scene helpers and labeled playback
sdk/ng_fix/     FIX/S1 8x8 tile and text layer
sdk/2d_engine/ng_depthfx.* optical pseudo-3D depth for 2D sprites
```

## Rules

1. Demo code should use `ngvideo_begin_scene()` before a visual scene.
2. Generated `showScreenN()` calls should go through `ngshow_draw()` or `ngshow_draw_simple()`.
3. Captions and HUD text should use `ngfix_*`, not raw `fixtext_out()`.
4. 2D sprite depth illusions should use `ng_depthfx_apply_y()` rather than local scale code.
5. A scene should end with `ngvideo_end_scene()` or `ngvideo_hard_clear()`.

## Result

The demo becomes a stable compositor instead of unrelated routines fighting for SCB state.  This is still true NeoGeo-style 2D: no polygon renderer is implied.  The depth effects are optical illusions made from sprite shrink, Y sorting, priority bands, FIX overlays and careful scene timing.
