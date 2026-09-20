# Maiya: Super Nature Girl

An original six-valley side-scrolling adventure using the SDK's C 2D engine.
Game name: `maiya`. Cartridge ID: `780`. The repository default remains `demo`.

## Build and Play

From the SDK root on Linux or WSL:

```sh
make GAME=maiya all
make GAME=maiya test
```

From Windows CMD:

```bat
make -f MakefileWin32.mak GAME=maiya all
make -f MakefileWin32.mak GAME=maiya test
```

`GAME=maiya` selects its configuration without editing the root `game.cfg`.
`make GAME=maiya` rebuilds P1 only after the first full build. `art` rebuilds
C1/C2 and the HUD font; `sfix` refreshes the HUD font; `sound` builds the game's
M1/V1 bank. Outputs are copied into `roms/maiya/`.

Use the SDK's configured 68000 toolchain, WLA-DX, Python 3, NumPy, Pillow,
SciPy and the standard sound-tool dependencies. In Windows, install the art
dependencies into the same interpreter that Make uses:

```bat
py -m pip install numpy pillow scipy
```

## Controls

| Control | Action |
| --- | --- |
| Left / Right | Walk, accelerate and brake |
| Up / Down at a vine | Climb, or stop by releasing the direction |
| A | Jump; detach from a vine with a jump |
| B | Whip nearby enemies or throw thorns |
| C | Dash |
| D | Spend a rose-art charge |
| Down + A | Drop through a one-way ledge |
| Up at the gate | Turn the collected Sun Key |

The Sun Key is on a canopy shelf. Rescue the villagers, collect roses and
reach the gate before entering the guardian arena.

## Encounters and Transitions

The last 320 pixels of each valley form a locked arena. The camera settles
there and both combatants remain inside it. The guardian walks, gives a
36-frame charging warning, commits to its charge direction, jumps and fires,
then recovers. The two side shelves can be jumped onto or used as shelter:
they intercept hostile projectiles. Recovery is the opportunity to retaliate.

After missions 2 and 4, a separate 25-second bonus playfield replaces the
arena. Movement and attacks remain enabled, the timer and hit count are
visible, and eight kills award a life. The interlude then leads to the next
mission. Bonus mode does not reuse the defeated guardian or its collision state.

## Rendering and Art

`tools/art.py` is the Makefile entry point. It runs the frame-based converter
in `tools/build_commercial_assets.py`, not the full-screen Artbox importer.
Do not run `prepare_assets.py` over this game's current assets: it is the
earlier prototype layout and has different sprite dimensions.

Maiya's canvas is 80x64, guardians 96x96. Animation groups share one palette
and fixed feet anchors. Guardian source sheets contain differently sized
drawings; their longest visible dimension is normalized before conversion.
The generated `artbox/generated/assets.json` describes exact tile bounds,
row stride and dimensions. The game never guesses a screen ID for a frame.

Scenery, the character pool, projectiles, front props and HUD use separate
sprite ranges. Decorative plants remain behind characters; only the explicit
front props pass in front. Foreground wrapping occurs outside the viewport.
Portraits use banks 41-42, decoration 14, villagers 33-34 and the gate 35,
so loading a villager or portrait cannot recolor the scenery or gate.

See [asset and audio notes](assets/CREDITS.md) for the source inventory.

## Optional EagleBIOS

```sh
make GAME=maiya all USE_EAGLE_BIOS=1
make GAME=maiya test USE_EAGLE_BIOS=1
make GAME=maiya bios-package
```

Add `-f MakefileWin32.mak` to each command on Windows. The package is
`dist/maiya-eagle-bios.zip`. Installed system ROMs are never overwritten.
For a console build, use `PLATFORM=aes` on both the build and test commands.
The firmware remains experimental; see [its compatibility limits](../../bios/README.md).

## Regression Checks

```sh
python3 -m unittest discover -s games/maiya/tests
gcc -std=c99 games/maiya/tests/test_levels.c -o /tmp/maiya-levels
/tmp/maiya-levels
python3 games/maiya/tools/build.py --quick
python3 games/maiya/tools/regression.py --scenario climb
python3 games/maiya/tools/regression.py --scenario boss
python3 games/maiya/tools/regression.py --scenario bonus --seconds 65
```

The quick build requires a previously staged full build made with
`python3 games/maiya/tools/build.py`. Captures require MAME with Lua support
and the toolchain's GDB to read structure offsets from the debug ELF. The
scenario harness positions the player through debugger memory only; no
scenario shortcuts are compiled into the game. `--output PATH` keeps captures
on another drive, and `--eagle-bios` selects the separately installed firmware.
The older `tools/capture.py` command delegates to the same harness; `--idle`
captures startup without gameplay inputs. Telemetry is written as JSON lines.
