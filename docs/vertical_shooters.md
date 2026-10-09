# Vertical Shooters: v1.7.3

The new C-only `ng_shooter.h` module is additive and opt-in. It does not
replace `ng_camera`, `ng_bg`, physics or any existing public function.
Maiya's configuration does not enable it. Framework version is now 1.3.

## Enable

Add `-DNG_SHOOTER=1` to the game's `GAME_ENGINE_DEFINES`. For hardware-safe
gameplay writes also enable `-DNG_VRAM_DEFER=1` and retain the normal vblank
commit. The build links this module only for games opting into it.

## Camera

`NGShooterCamera` keeps Q8 travel and follow offsets: 256 units = one pixel.
`ng_shooter_camera_init()` defaults to one pixel/frame with no follow.
`ng_shooter_camera_set_speed()` accepts signed speeds: 128 is half a pixel,
256 is one pixel; positive travel flies up, making scenery move down.
`ng_shooter_camera_set_follow()` sets pixel limits, a central dead zone and
an easing shift (0 snaps, 4 eases by 1/16, maximum 8).

Call `ng_shooter_camera_step()` once per gameplay frame with the player's
screen-space center. Follow is centered on the native 160,112 screen midpoint.
This is a scenery camera: characters, shots, FIX HUD and collision rectangles
remain in screen space. Do not subtract its offsets from those coordinates.
Travel saturates at +/-0x3FFFFFFF Q8 units instead of overflowing during an
extended session. All state is caller-owned; reset it between sectors.

## Layers

`NGVerticalLayer` owns two persistent sprite groups. Each page is 14..16
tiles tall (224..256 pixels), with 1..32 strips. Art must contain both pages
consecutively in row-major order, including its optional per-tile palette map.

```c
#include "ng_shooter.h"

static NGShooterCamera camera;
static NGVerticalLayer terrain, clouds;

/* Scene setup while faded out; asset palettes are loaded separately. */
ng_shooter_camera_init(&camera);
ng_shooter_camera_set_follow(&camera, 8, 6, 16, 4);
ng_vertical_layer_init(&terrain, 1, 22, 16, ground_tiles,
                       ground_palette, ground_bank_map, -16);
ng_vertical_layer_init(&clouds, 45, 22, 16, cloud_tiles,
                       cloud_palette, cloud_bank_map, -16);

/* Frame logic; the game's existing vblank routine commits these moves. */
ng_shooter_camera_step(&camera, player_x, player_y);
ng_vertical_layer_draw(&terrain, &camera, 256);
ng_vertical_layer_draw(&clouds, &camera, 384);

/* Scene teardown while hidden, before discarding caller-owned storage. */
ng_vertical_layer_hide(&terrain);
ng_vertical_layer_hide(&clouds);
```

Check initialization's return value: invalid slot/tile ranges or geometry
return zero and leave the layer disabled. Hide an active layer before
reinitializing it. Initialization uploads maps immediately; only do it during
scene setup behind a fade. Do not move or free an active layer's structure or
palette-map storage while deferred writes reference it.

`ng_vertical_layer_draw()` supports Q8 parallax ratios 0..1024 (larger values
are clamped). It retains fractional travel through positive/negative wraps
and updates high-priority positions only. Zero is stationary, 128 is distant,
256 terrain, 384 faster foreground. Supply horizontal gutters large enough
for `follow_limit * ratio / 256`; never shift an exact 320-pixel-wide map
sideways and expect the uncovered edge to be filled automatically.

## Sky Lance

Seven 352x512 terrains use 16-pixel gutters around the 320-pixel picture.
Terrain occupies slots 1..44; sparse clouds occupy 45..88; existing characters
start at 96. Camera limits are 8 horizontal/6 vertical pixels; clouds move at
1.5x terrain speed. Sparse clouds leave opponents readable. FIX HUD, player
steering and collision behavior are unchanged.

The direct asset builder retains the original pilot/plane designs, sharpens
native-size sprite pixels and fills only enclosed alpha holes of at most three
pixels. Larger silhouette openings and external transparency are preserved.
Portraits are now 80x96; gameplay craft remain 48x48 to preserve readability
and game balance. Title art is 320x224 with pixel-beveled metallic lettering,
squadron aircraft and `(C)1996 (+30) EAGLE SOFTWARE`.

All 47 asset PNG previews, tile maps, palettes and direct C1/C2 data come from
`games/skylance/tools/art.py`, using the SDK tile/palette codecs, as in Maiya's
direct-ROM workflow. The native output is not an HD video mode.

## Build And Checks

```sh
make api-check
make -C tests test_shooter
./tests/test_shooter
python3 tools/build_skylance_mame.py /mnt/c/mame/neogeosdk
python3 -m unittest discover -s tests -p test_skylance_assets.py
python3 games/skylance/tools/qa.py --platform mvs --bios stock
python3 games/skylance/tools/qa.py --platform aes --bios eagle
```

The installer backs up earlier Sky Lance files, leaves installed Maiya ROMs
alone and writes separate AES/MVS cartridges. Capture tests check gameplay,
audio, Z80 protocol/timer state and save images for visual inspection. Actual
Neo Geo hardware and CRTs still require testing. This is an unpublished
v1.7.3 working-tree build, not a published release.
