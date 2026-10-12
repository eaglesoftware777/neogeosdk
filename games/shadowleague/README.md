# Shadow League Fighting

An original one-on-one fighting game for Neo Geo MVS and AES, built on the
SDK's `ng_fight` engine (`docs/fighting.md`). Cartridge id 781.

**Status: first playable.** Two fighters, one stage, arcade and versus play
against a CPU or a second player. See `docs/roadmap/V1.7.5_STATUS.md`.

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
rm out/*.o
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
