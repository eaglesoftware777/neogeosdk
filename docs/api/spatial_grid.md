# Optional screen-local collision candidates

Include `sdk/2d_engine/ng_spatial_grid.h` to use a static 64px broad phase.
It is independent of rendering, character storage and physics precision.
No engine module or existing public structure changes are required.

`NGSpatialGrid` contains a world-space origin and thirty 16-bit cell masks:
six columns by five rows, covering 384x320px. IDs are fixed integers 0..15.
Reset the grid each simulation step, insert current object bounds, then query
candidate masks for projectiles or other moving objects. Coordinates are
half-open: `[left,right)` and `[top,bottom)`.

```c
NGSpatialGrid grid;
ng_spatial_grid_reset(&grid, camera_x - 32, camera_y - 64);
ng_spatial_grid_insert(&grid, enemy_id, left, top, right, bottom);
uint16_t candidates = ng_spatial_grid_query(&grid, x, y, x + 1, y + 1);
```

The mask is a broad-phase result: always perform the original precise test
against every selected ID. Insert bounds must conservatively enclose that
test. Empty or fully out-of-coverage queries return zero; partly overlapping
bounds are clipped. Use a full-list fallback if a query can interact with
objects beyond the grid. Rebuild after movement before testing collisions.

For a tiny cast or one projectile, a linear scan can cost less than building
the grid. Maiya keeps its established linear collision path. The helper remains
optional for games with larger casts. The host test checks cell boundaries, clipping and candidate coverage.
