# Depth FX

## What It Does

`ng_depthfx` (Stage 10) projects 3D (x, y, z) world positions onto the
NeoGeo screen with perspective-correct scaling using the SCB2 shrink register.
Z distance selects both the sprite's apparent size and its fog palette.

## Why It Matters on NeoGeo

NeoGeo sprites have a hardware shrink register (SCB2) that scales sprites down
from full size (0xFF vertical, 0x0F horizontal nibble) to tiny.  By driving
this register with a Z-distance lookup table, you get convincing depth scaling
for flying objects, bosses approaching, background elements at varying depth.

## Hardware Limitation It Solves

- Avoids division for projection: uses `ng_shrink_tab[]` lookup table.
- SCB2 horizontal shrink is a 4-bit nibble (0..15); SCB2 vertical is 8-bit (0..255).
  The projection maps both from a single Z value.
- Fog palette selection: 4 depth bands map to palette slot offsets.

## Coordinate System

```
X: left-right,  positive = right
Y: up-down,     positive = down (screen space)
Z: depth,       0 = camera plane, 127 = far clip
```

Vanishing point default: screen centre (160, 112).

## Example

```c
// At scene init:
ng_depthfx_init();
ng_depthfx_set_fog_palettes(0, 0, 1, 2);  // near=+0, far=+2 palette offset

// Per frame:
NGVec3 boss_pos = { 160, 112, 40 };  // Z=40 = mid-distance
ng_depthfx_draw_group(&boss_group, boss_pos, PAL_BOSS);
```

## Shrink Table

`ng_shrink_tab[z]` maps Z=0..127 to shrink value 0xFF..0x20.
- Z=0:   0xFF (full size)
- Z=64:  ~0x7F (half size)
- Z=127: 0x20 (minimum visible)

## Common Mistakes

- Using Z > 127: clamps to 127 automatically but consider culling farther objects.
- Forgetting fog palettes: at default, all Z depths use the same palette.
  Call `ng_depthfx_set_fog_palettes()` to enable distance fog.

## Performance Advice

- Two multiplies per projected object (screen_x, screen_y) = fast on 68000.
- The shrink table lookup is a single array read — O(1).
- Cull objects with `proj.visible == 0` before calling `ng_sprite_group_update_transform()`.
