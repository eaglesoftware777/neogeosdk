# Shadow League Fighting

An original one-on-one fighting game for Neo Geo MVS and AES, built on the
SDK's `ng_fight` engine (`docs/fighting.md`). Cartridge id 781.

**Status: V2 playable.** 44 individually styled fighters, four panoramic arenas,
arcade and versus play against a CPU or a second player. Each fighter has a
discipline-specific guard, costume, six-phase footwork and strike preparation.
Boxing fighters use punches on all four buttons, including jumping attacks. See `docs/roadmap/V1.7.5_STATUS.md`.

## Controls

```
Stick       walk, crouch, jump; hold back to guard (down-back for low attacks)
            6 6 dash, 4 4 backdash
A  light punch      B  light kick      C  heavy punch      D  heavy kick
A + B       evasive roll (back + A + B rolls away)
C + D       heavy attack; while guarding: guard attack (costs spirit)
6 or 4 + C  throw (close); press C or D as you are grabbed to escape
```

### Kenji Arata — traditional karate (Japan)

| Move | Input |
|---|---|
| Ressen Zuki (lunging reverse punch) | 236 + A or C |
| Kokoro Uke (counter stance) | 214 + A or C |
| Kakato Otoshi (axe kick, overhead) | 623 + B or D |
| Ichigeki Hissatsu (super, one stock) | 236236 + A or C |
| Yoko Geri (side kick) | C + D |

### Jack Mercer — professional boxing (United States)

| Move | Input |
|---|---|
| Brooklyn Rush (dashing straight) | 236 + A or C |
| Weave (slip under strikes, body blow) | 214 + B or D |
| Rising Bell (uppercut, anti-air) | 623 + A or C |
| Final Round (super, one stock) | 236236 + A or C |
| Haymaker | C + D |

Jack throws no kicks: B and D are body punches and hooks.

## Arcade settings (MVS soft DIPs)

DIFFICULTY (EASY to EXPERT, NORMAL by default) sets the CPU level; ROUNDS
(1-3) is the number of round wins a match needs. A console plays NORMAL,
best of three.

## Building

```sh
make GAME=shadowleague all            # PLATFORM=aes for the console build
python3 games/shadowleague/tools/qa.py
```

Sources:

- `fighters/*.json`: each fighter's build, palette, poses, animations, frame
  data, moves and CPU profile. `tools/fighter_validate.py` checks them.
- `tools/fighter_cook.py` draws every pose (`tools/fighter_art.py`) and
  writes the ROM tables.
- `tools/art.py` builds the C and S ROMs and `artbox/generated/slf_assets.h`.
- `tools/sound.py` synthesizes the effects and music.
- `scenes/slf_game.c` is the game flow, `scenes/slf_draw.c` the hardware side.

## V2 artwork and installation

Native action canvases are 160x160, anchored at (80,141). The extra transparent
margin keeps extended kicks, falls and raised hands inside the canvas. The game
keeps the same floor position and world coordinate system. Collision follows the
new drawing. Bodies use separate
near/far limbs, shaped hands, footwear, cloth folds and authored character heads.
All 2,124 frames are indexed with transparent colour 0 and 15 opaque colours.

`assets/authored/profiles.json` lists the 44 costume and fighting designs.
`assets/authored/art-direction.json` records the visual brief. The master portrait,
roster and four arena sheets are retained beside the indexed heads and portraits.
`tools/redesign_v2.py` rebuilds the design from `roster-baseline.json`; ordinary
builds read the editable fighter JSON files, not that regeneration recipe.

`tools/check_art_v2.py` runs during the asset build. It rejects clipping, detached
clusters, disconnected limb layers, palette errors and missed planted-foot rows,
and compares every sprite with pixels decoded from the final cartridge ROMs.
Sparse shared strips preserve the pixels while fitting the existing C-ROM budget.
Fighter definitions live in one cartridge translation unit, so strikers and the
main cast share the same definitions. Super moves dim the arena while leaving the
fighters visible. Private FIX lettering, vitality, spirit and win markers use the
new palette and crisp pixel geometry.

The four arenas are Mountain Dojo, Bangkok Riverside, Brooklyn Rooftop and Chimera
Laboratory. Combat floors stay clear; scenery and spectators remain behind them.
The opaque panoramas use one visible stage layer to protect the scanline budget.

After packaging, install with:

```sh
python3 games/shadowleague/tools/install_v2.py --target /mnt/c/mame/neogeosdk
```

The installer stages and verifies both sets, then moves only the previous Shadow
League files to `backups/shadowleague/<date>-before-v2-redraw`. Other games and
shared BIOS files are preserved. MVS/AES launchers select the fresh private ROMs
and hashes. Controls and soft DIPs are kept; old save states are left in the backup.
