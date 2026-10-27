# Maiya: Super Nature Girl

An original twelve-mission side-scrolling adventure using the SDK's C 2D engine.
Game name: `maiya`. Cartridge ID: `780`. The repository default remains `demo`.
New in SDK v1.7.1.

Current development: **V5 stability candidate with V4 artwork**.
The original combat, encounters, scoring, collision and rendering behavior are
restored. There are no intermediate guardians, rank system or experimental
moving ledges. Three small platform-height adjustments preserve reachable
routes; the archer on a raised platform follows its supporting surface.

Backgrounds, scenery, arenas, terrain, landmarks, portraits, effects, palettes
and fonts use the approved V4 sources. The only character-art exception is the
requested walk/run back-contour correction: 90 explicit native pixels across
16 drawings, shared by Maiya and Luna. Heads, hands, legs, feet, timing and
anchors are unchanged. No editor or image generation runs during a build.
The existing title font displays `V5 TEST`.

`tools/locked_art.py` checks immutable source graphics and metadata against
`artbox/v4_reference/lock.json`. `tools/maiya_posture.py` applies the finite
pixel recipe in `assets/authored/posture_correction.json`, with source and
result hashes checked. The ROM checker compares every decoded hero pixel with
that exact expected result. No approved reference hash is updated.

Build isolated AES/MVS test packages:

```sh
python3 games/maiya/tools/package_v5.py --out dist/maiya-v5-test
```

The optional `--install` destination refreshes only Maiya's ROMs, matching
software lists and NeoSD images. Saves, launch settings and other games are
preserved. An older launcher's Maiya ROM search path is corrected if needed.
Use a new output directory for each candidate. Native CD media is built with
`python3 tools/neocd/build.py --game maiya` using the same approved graphics.

Busy-scene timing, actual 12/24 MHz parity and physical hardware approval
remain open. This is a gameplay test candidate, not a hardware certification.

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
`make GAME=maiya` rebuilds P1 only after the first full build. `art` installs
the approved C1/C2 and FIX banks; `sfix` restores that same FIX bank; `sound` builds the game's
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
| Left / Right | Tap to walk; hold to build into a run; release to brake |
| Up / Down at a vine | Climb, or stop by releasing the direction |
| A | Jump; detach from a vine with a jump |
| B | Whip nearby enemies or throw thorns; hold to keep throwing. On the move she plants her feet for each throw and walks a few steps before the next |
| C | Dash |
| D | Spend a rose-art charge |
| Start alone | Pause / resume during play |
| Down + A | Drop through a one-way ledge |
| Up at the gate | Turn the collected Sun Key |

### Special moves and character differences

- **Rising Bloom (uppercut):** forward, down, down-forward + B. It hits
  nearby creatures in front/overhead during the ascending strike window;
  armored creatures take a hit, and an exposed guardian can be struck once.
  The descent is a recovery, not a second attack. Wraiths are immune.
- **Rose Blossom Surge:** down, forward + B; a short petal-trailing attack.
- **High leap:** kneel briefly (at least eight frames), then Up + A.
- **Stomp:** land on a susceptible creature; hold A for a higher bounce.
- **Sky Lily:** while active, Up + A gives one extra jump in the air.
- **Secret Art:** D spends a charge on the current learned art. The five arts
  are Rose Blossom Storm, Purifying Rain, Sunflare Dance, Frost Petal Storm
  and Leaf Gale. These intentionally affect enemies across different heights.

Swift Wind increases speed; Thorn Might strengthens blows; Mist Veil protects;
Spring Bud boosts jumping; Thorn Crown improves ordinary throws; Thorn Sheaf
gives double throws. Petal Fan, Golden Seed and Wind Leaf supply limited-ammo
spread, piercing and returning weapons. Bloom Bud adds an art charge; spirit
orbs teach an art; extra lives, currency, flowers, rescues and the Sun Key have
their own counters or mission roles.

Maiya and Luna share movement, collision, damage and move inputs. Their
appearance, voice, projectile art and effect colors differ; neither is a
stronger or faster character. Nearby whip attacks require overlap with the
enemy's actual body, and one-way ledges allow four pixels of edge forgiveness.

The Sun Key is on a canopy shelf. Rescue the villagers, collect roses and
reach the gate before entering the guardian arena.

Enemies wait at their spawn positions during the mission card. The first
forest encounter starts farther along the road, leaving room to move before
combat begins.

The bottom tray uses the actual pickup sprites at half size, with live counts
for rose arts, coins, flowers and rescued friends. Active powers show their
remaining seconds. The key appears only while it is carried. Ordinary road
pickups are streamed near the player; leaving one behind does not permanently
consume its spawn. High shelves hold the rarer treasures and extra lives.

Climbing uses position-based reaching poses with stable facing. Release the
stick to stop, or press A to detach. Release A during a jump for a shorter hop.

Start combined with an action button or Select is left to the system firmware,
not treated as a game pause. NeoSD uses a held A+D+Start combination to return
to its cartridge list; NeoSD PRO's configurable slot-menu combination is
normally D+Start. Start+Select may instead open the UniBIOS in-game menu.
These shortcuts depend on the installed firmware and require hardware testing.

## Lives and Continue

Each run starts with three lives and three available continuations. A lethal
hit costs one life: Maiya rises with a halo, then returns from the sky at the
mission entrance. Landing on either a shelf or the road restores control.
Collected map items, rescued allies and key/gate progress survive that return;
enemies and temporary powers reset.

When no lives remain, a ten-second Continue countdown appears:

- MVS: insert a coin, then press P1 Start. One credit buys three lives.
  Gameplay buttons do not spend a credit.
- AES: choose Continue or Exit with the directions and confirm with A, B, C
  or D. There is no coin requirement. Timeout selects Exit.

Exit returns to the title/attract flow. The console uses a game-RAM start latch
and the BIOS start-button edge, not MVS backup RAM or joystick Up.

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

Mission 7, Rio Negro Works, follows the six valleys. Its gantries, sludge and
fire lead to a two-guardian encounter: Iron Vulture followed by Lord Smoggar.
Both fights use the same locked arena and cover shelves.

## Music and Effects

The house eyecatcher has a short FM fanfare. Title, missions, guardians,
bonus rounds and interludes use Maiya's own ADPCM-B music bank, with ADPCM-A
effects mixed over it. Track changes reset the scene, restore the mix, enable
the repeat flag, and only then start the track. Setting repeat before a scene
reset loses it. ADPCM-A volume is six bits (0..63); 64 is not full volume.

The music and effects are credited in [sound/SOURCES.md](sound/SOURCES.md).
The copied voice bank has been removed; there are no voice sample commands in
the game flow. Rebuild `make GAME=maiya sound` when changing that bank.

## Rendering and Art

`tools/art.py` is the Makefile entry point. It copies the immutable approved
V4 banks and their matching metadata. The old conversion and authoring tools
are archival; they are not part of the build. Do not run them over approved
assets. The art lock rejects changes rather than accepting a new hash.

Maiya's canvas remains 80x64, guardians 96x96. The accepted indexed walk/run
strips, layers, head placement, hands, legs, frame timing and feet anchors
remain unchanged for both Maiya and Luna. Existing hero validators still
check those accepted drawings against the actual encoded C-ROMs.

The generated `artbox/generated/assets.json` describes exact tile bounds,
row stride and dimensions. The game never guesses a screen ID for a frame.

Scenery, the character pool, projectiles, front props and HUD use separate
sprite ranges. Decorative plants remain behind characters; only the explicit
front props pass in front. Foreground wrapping occurs outside the viewport.
Portraits use banks 41-42, decoration 14, villagers 33-34 and the gate 35,
so loading a villager or portrait cannot recolor the scenery or gate.

Gate dimensions and row stride come from the generated asset header. Its
three-strip window is separate from the cage, signs and vines. Walking and
dashing share a forward distance-based gait clock (walk: contact, down,
passing, up for each foot; run: contact, down, push-off, flight), without
changing character scale or jump physics. Every gait frame keeps one hip, so
the sprite moves with her and never jumps; the accepted poses are fixed and must not be
redrawn or stretched. Her sprite slots are pinned
(`ng_char_pin_sprite`), so creatures coming and going never move her into
slots still waiting to be written. Water shimmer resolves its two inks from the compiled
decor palette rather than relying on pre-quantization palette indices.

See [asset and audio notes](assets/CREDITS.md) for the source inventory.

## Optional EagleBIOS

NeoSD v1.7.1 hardware-test images and their region-preservation rules are
documented in [NeoSD test images](../../docs/NEOSD_V171_TEST_IMAGES.md).
The normal sound build requires P1 and M1 from the same SDK revision.
Maiya's private FIX font/HUD occupies `$D00-$FFF`; the system boot area is
separate. The AES BIOS logo occupies unused C bank `$49`, without shifting
gameplay sprite tile numbers.

```sh
make GAME=maiya all
make GAME=maiya test
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
python3 games/maiya/tools/regression.py --scenario continue --seconds 28
python3 games/maiya/tools/regression.py --scenario pickups --seconds 28
python3 games/maiya/tools/regression.py --scenario factory --seconds 30
python3 games/maiya/tools/regression.py --scenario tray --seconds 25
```

The quick build requires a previously staged full build made with
`python3 games/maiya/tools/build.py`. Captures require MAME with Lua support
and the toolchain's GDB to read structure offsets from the debug ELF. The
scenario harness positions the player through debugger memory only; no
scenario shortcuts are compiled into the game. `--output PATH` keeps captures
on another drive. Runs boot EagleBIOS from `bios/test_roms` (`make eagle-bios`);
`--stock-bios` boots the system ROM in `roms/` instead.
The older `tools/capture.py` command delegates to the same harness; `--idle`
captures startup without gameplay inputs. Telemetry is written as JSON lines.

For AES checks, first stage and compile the console entry points:

```sh
python3 games/maiya/tools/build.py --platform aes
python3 games/maiya/tools/build.py --quick --platform aes
python3 games/maiya/tools/regression.py --platform aes --scenario continue --seconds 28
python3 games/maiya/tools/regression.py --platform aes --scenario continue-exit --seconds 28
python3 games/maiya/tools/regression.py --platform aes --scenario continue-timeout --seconds 38
```

The quick build and harness reject a platform different from the staged
workspace. Run a full `--platform mvs` build to switch back. This isolated
workspace builds P1 against existing Maiya M1/V1 banks; it does not regenerate
audio. Continue tests exercise the real input and BIOS callbacks after placing
a death scenario through debugger memory, including a respawn onto a ledge.
