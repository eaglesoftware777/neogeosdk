# ng_fight — one-on-one fighting games

`sdk/2d_engine/ng_fight.{h,c}` is an opt-in C module (`-DNG_FIGHT=1` in a
game's `GAME_ENGINE_DEFINES`). It owns the simulation of a two-fighter match;
the game owns drawing, sound and the HUD. Shadow League Fighting
(`games/shadowleague`) is the reference game.

## Contract

- One `ngf_match_step(&m, raw_p1, raw_p2)` per VBlank. `raw` is the BIOS
  controller byte (`BIOS_P1CURRENT` layout: up, down, left, right, A-D).
- Deterministic: no heap, no floating point, a seeded generator for the CPU.
  The same seed and inputs give the same match (`ngf_match_hash`).
- Positions are Q8 (pixels x 256): `x` grows right, `h` is the height above
  the floor. Frame-data speeds are in eighths of a pixel per frame.
- `NGFMatch` uses fixed storage for fighters, input queues, teams and assists;
  every fighter definition is ROM data.

## Input

`NGFInput` keeps 32 frames of raw input per player, press edges, and
back/down charge. Queries are relative to the fighter's facing:

| Motion | Notation | Notes |
|---|---|---|
| `NGF_MOTION_QCF` / `QCB` | 236 / 214 | must end within 5 frames, 15-frame window |
| `NGF_MOTION_DP` / `RDP` | 623 / 421 | |
| `NGF_MOTION_HCF` / `HCB` | 41236 / 63214 | 22-frame window |
| `NGF_MOTION_DQCF` / `DQCB` | 236236 / 214214 | supers |
| `NGF_MOTION_CHARGE_BF` / `DU` | [4]6 / [2]8 | 40 frames held, 10 to release |
| `NGF_MOTION_360` | all four directions | 30-frame window |
| `NGF_MOTION_DASH` / `BACKDASH` | 656 / 454 | |
| `NGF_MOTION_FWD` / `BACK` / `DOWN_FWD` | 6 / 4 / 3 held | command normals, throws |

`ngf_input_pressed(in, mask, buffer)` accepts several buttons pressed within
the buffer (simultaneous presses); a started move consumes its buttons, so
one press never fires twice. Opposite directions cancel (SOCD neutral). A
move's `window` overrides the default leniency. Button ages advance with the
simulation, so presses and releases survive hitstop and super pauses. Normals
accept four frames, specials five; `ngf_input_released` handles release edges.
Successful motion moves consume their completed motion as well as their buttons.

## Fighter data

`NGFFighterDef` points at ROM tables:

- `anims` / `frames`: every animation is a run of `NGFFrame {sprite, time,
  hurt, flags, dx, dy}`. The first `NGF_A_COUNT` animations are the standard
  set (idle, walk, crouch, jump, guard, hit, fall, down, getup, dash,
  backdash, thrown, win, lose, intro, dizzy). Frame flags: `ACTIVE` (hitbox
  live), `REHIT` (another hit may land), `SPAWN` (projectile), `INVULN`,
  `AIRBORNE` (dx/dy become launch speeds), `COUNTER` (a strike landing now
  starts the move's `follow` instead).
- `hurt`: up to three boxes per frame (`NGFBox {x, y, w, h}`, facing right,
  `y` measured up from the feet).
- `moves`, in priority order (the first match wins): kind (normal, command,
  special, super, throw, evade, guard attack), motion, buttons, stance
  (stand/crouch/air, close/far), level (mid, high, low, overhead, throw,
  unblockable), damage, guard damage, chip, hitstun, blockstun, pushback,
  on-hit effects (knockdown, launch), cancel rights (chain, special, super),
  meter gain and cost, hitstop, projectile, sounds, launch speed, range and
  one hitbox for the active frames.
- `cpu_profile`: aggression, preferred range, jump rate and the move each CPU tactic
  uses (poke, sweep, anti-air, special, rush, throw, super).

Shadow League writes these tables from human-edited JSON
(`games/shadowleague/fighters/*.json`) with `tools/fighter_cook.py`, which
also derives hurtboxes and hitboxes from the drawn poses;
`tools/fighter_validate.py` rejects broken definitions during the art build.

## Combat rules

- Blocking: hold back. Standing guard stops high, mid and overhead; crouching
  guard stops low and mid. No air guard. Chip damage never KOs.
- Guard meter: blocked hits drain it; empty means a guard break (dizzy).
  It refills while not blocking.
- Power meter: three stocks of 128; supers cost stocks.
- Spirit meter: gained by walking in, dashing, clean blocks, counters and
  throw escapes; spent on guard attacks.
- Counter hits during an attack's startup add damage and hitstun. Combos
  scale damage 10% per hit, down to 50%.
- Throws: close range, grounded, not in hit or block stun; pressing C or D
  within four frames escapes.
- Hitstop freezes the match for a few frames on contact (inputs are still
  buffered); projectiles cancel each other.
- Push boxes keep the fighters apart; both stay inside the stage and the
  320-pixel screen; the camera centres on them.

## Match flow

`NGF_M_INTRO` (96 frames, `NGF_EV_ROUND`, then `NGF_EV_FIGHT`) → `FIGHT`
(60-second clock) → `KO` or `TIME_UP` → `ROUND_END` (winner or draw) → next
round or `OVER` once a side has `rounds_to_win` (default 2). The game reads
`m.state`, `m.round`, `m.seconds`, each fighter's `life`, `power`, `wins`,
`combo`, and drains `ngf_pop_event` for sounds and effects (hit, block,
throw, tech, guard break, counter, KO, move start, landing).

## Teams and assists

After `ngf_match_init`, choose `NGF_MODE_3V3` or `NGF_MODE_TAG` with
`ngf_match_set_mode` and assign up to three members per side with `ngf_team_set`.
Teams play until a side has no remaining fighters. KO, double KO and timeout
advance defeated sides; the next bout clears movement, stun, projectiles and
assists, introduces the new members and resets the timer. The survivor recovers
one eighth of maximum life. Tag mode's `ngf_tag_switch` retains each member's
health separately and skips eliminated partners.

`ngf_striker_set` assigns a separate assist with three calls per match. A+C or
`ngf_call_striker` summons it while the caller can act. The assist runs in, uses
its strongest standing normal, and leaves. Its active hitbox must intersect an
opponent's vulnerable hurtbox; guard and damage rules still apply. A cooldown
prevents immediate repeat calls. Without an assigned assist, A+C does not summon
a duplicate of the active fighter.

## CPU

`ngf_set_cpu(&m, side, level)` (1-9; 0 = human) makes that side's input come
from `ngf_cpu_input`: guarding against attacks in range, anti-airs, whiff
punishes, spacing, specials at range, throws and supers, typed as real
motions so the same input rules apply.

## Tests

- `tests/test_fight.py` builds `tests/fight_test.c` on the host: motions and
  near misses, charge, dash, simultaneous presses, hits, guards by level,
  throws, projectiles, supers, frozen animation, buffered input, assist collision,
  tag health, team elimination, double KO, timeout, replay hashes and a
  deterministic CPU match.
- `tests/test_shadowleague.py`: Shadow League's data validates and its cooked
  fighters finish CPU matches in every pairing.
