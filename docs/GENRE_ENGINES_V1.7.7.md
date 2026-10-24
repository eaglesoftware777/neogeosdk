# Genre engines (NeoGeoSDK v1.7.7)

v1.7.7 adds four optional engines, one C source and one header each, for
games the SDK's 2D engine did not cover on its own:

| `GAME_GENRE_ENGINE` | Source | What it runs |
|---|---|---|
| `puzzle` | `sdk/2d_engine/ng_puzzle.c` / `.h` | match-3 boards and launched-bubble lattices |
| `mahjong` | `sdk/2d_engine/ng_mahjong.c` / `.h` | four-seat Mahjong (Hong Kong Old Style) and Mahjong solitaire |
| `rpg` | `sdk/2d_engine/ng_rpg.c` / `.h` | top-down adventure: rooms, party, items, dialogue, environmental puzzles |
| `beatemup` | `sdk/2d_engine/ng_beatemup.c` / `.h` | belt-scrolling 2.5D brawler for one player against a crowd |

Each one is game logic only. None of them touches video RAM, the palette or
the sound driver: a game draws with the SDK's sprite groups and FIX layer
and plays sounds from the events the engine reports, exactly as before.
Nothing is compulsory: a game that leaves the selector empty builds and
links exactly as it did before.

Example games built on them: `games/sevenwonders` (puzzle),
`games/maiyaluna` (rpg), `games/nosignal` (beatemup) and
`games/mahjong_demo` (mahjong). The code below is sketched for reading;
the complete minimal programs are in `tests/genre_examples.c`, which
`tests/test_genre_engines.py` compiles (with `-Werror`) and runs.

## 1. Choosing an engine

One line in the game's `game.mk`:

```make
GAME_GENRE_ENGINE = puzzle
```

The build then compiles that one engine as C into `out/ng_puzzle0.o`, links
it after the core engine objects, and passes `-DNG_GENRE_ENGINE_PUZZLE=1` to
the game's sources and the engine (likewise `_MAHJONG`, `_RPG`,
`_BEATEMUP`). Both `Makefile` and `MakefileWin32.mak` read the selector
from `sdk/genre_engine.mk`.

- Empty (the default): no genre source is compiled, no object linked.
- A typo stops the build: `GAME_GENRE_ENGINE='puzle' is not a genre engine;
  use one of: puzzle mahjong rpg beatemup, or leave it empty`.
- Two values stop it too: `a game links one genre engine, not 2`.
- `make GAME=mygame genre-info` prints what the selector resolved to:
  `game=mygame genre=puzzle source=sdk/2d_engine/ng_puzzle.c object=out/ng_puzzle0.o define=-DNG_GENRE_ENGINE_PUZZLE=1`.

One engine per game is deliberate. Each engine carries what its genre
needs inside its own file: the adventure engine has its own switch, plate,
gear, water and light puzzles, so an adventure never needs the puzzle
engine; the Mahjong engine holds both the four-seat game and solitaire.

The game includes the matching header (`#include "sdk/2d_engine/ng_puzzle.h"`
from a game source, or `ng_puzzle.h` with the SDK's include paths).

### Dependencies

| Engine | Uses from the SDK | Notes |
|---|---|---|
| puzzle | `ng_trig` (already in every game's SDK library) | bubble aiming |
| mahjong | nothing | |
| rpg | nothing | |
| beatemup | nothing | |

### C++14 games

The engines are C. A `USE_2D_PLUS=1` game, whose own sources are built with
`g++`, still gets the engine compiled with the C compiler; the headers wrap
their declarations in `extern "C"`. `tests/genre_cpp_test.cpp` builds a
C++14 program against all four engines on the host. Designated initializers
in the examples below are C99; a C++14 game writes its tables positionally.

## 2. What every engine has in common

- **The caller owns the state.** Each engine works on a context the game
  declares (`static NGPuzzle board;`); there is no heap, no hidden global,
  no floating point and no recursion. Data tables (levels, worlds, stages,
  fighters) are `const` and live in ROM; the context keeps pointers to
  them, so they must stay valid while the context is used.
- **One update is one step.** `ng_*_update()` advances the simulation by
  one frame (or, for the puzzle board, one resolution phase). It never
  waits for the vertical blank. A game calls it from its own frame loop.
- **Events, not callbacks.** What happened (a match, a hit, a discard, a
  door opening) is queued in the context. The game drains the queue after
  each update with `ng_*_event()` and turns entries into sounds, effects
  and text. Queues are bounded; a full queue counts what it drops
  (`ev_dropped`).
- **Read the context, change it through calls.** Fields marked read-only
  (positions, boards, hands, scores) are there to draw from.
- **Deterministic.** Each context has its own seeded generator. The same
  data, seed and inputs give the same game, which `ng_*_hash()` confirms;
  the puzzle engine replays action lists, and the puzzle, Mahjong and
  adventure engines save and load their state (big-endian, versioned).
- **Return codes.** Calls that can fail return `0` (`NGP_OK`, `NGM_OK`,
  `NGR_OK`, `NGB_OK`) or a negative error from the header.

## 3. Puzzle (`ng_puzzle`)

One `NGPuzzle` (1312 bytes) plays one level of either family:

- `NGP_MODE_MATCH`: swap neighbours to line up three or more; runs of four
  make line pieces, five a prism, crossing runs a bomb; crates, ice, locks
  and walls; cascades; a shuffle when no move is left (or a loss, by level
  flag); objectives for score, colours, ice, crates and specials.
- `NGP_MODE_BUBBLE`: aim (1024 steps per turn) and fire into a staggered
  lattice; rebounds off the side walls; groups of three or more pop,
  anything no longer hanging falls; anchors, stones, caged friends to
  rescue, new rows pushed in, a loss line; an exact path preview and a hint.

Levels are `NGPuzzleLevel` tables with the board written as text
(`ng_puzzle.h` lists the characters). `ng_puzzle_level_check()` reports the
first bad character. An action is checked and committed at once; what
follows (clear, fall, refill, cascade, a bubble's flight) runs one phase
per `ng_puzzle_update()`, so a game can wait for its animation between
phases and still play the identical game.

```c
static const NGPuzzleLevel k_level = {
    .mode = NGP_MODE_MATCH, .w = 8, .h = 8, .colors = 5, .seed = 1234u,
    .moves = 20u,                                   /* twenty swaps */
    .goals = { { NGP_OBJ_SCORE, 0, 30u } },         /* 3000 points */
    .stars = { 3000u, 5000u, 8000u },
};
static NGPuzzle s_board;

ng_puzzle_init(&s_board, &k_level, 0);
for (;;) {                                         /* the game's frame loop */
    if (ng_puzzle_status(&s_board) == NGP_STATUS_READY && player_swapped(&cell, &dir))
        ng_puzzle_action(&s_board, NGP_CMD_SWAP, cell, dir);
    ng_puzzle_update(&s_board);
    while (ng_puzzle_event(&s_board, &e)) { /* NGP_EV_MATCH: a sound; NGP_EV_FALL: animate */ }
    /* draw s_board.color[], .kind[], .cover[] (the cells ng_puzzle_next_dirty() lists) */
}
```

Calls: `level_check`, `init`, `reset`, `action` (`NGP_CMD_SWAP`,
`SWAP_CELLS`, `AIM`, `AIM_BY`, `FIRE`, `SWAP_NEXT`, `HAMMER`, `SHUFFLE`,
`COLOR_SHOT`), `update`, `settle`, `status`, `flying`, `event`, `cell`,
`neighbor`, `row_shifted`, `row_cells`, `cell_px`, `next_dirty`,
`dirty_clear`, `hint`, `has_moves`, `bubble_preview`, `bubble_hint`,
`hash`, `save` / `load` (`NGP_SAVE_SIZE`, 840 bytes), `replay`.

`ng_puzzle_bubble_target()` returns the first occupied bubble struck by the
same collision path used for flight and preview. Use it for aimed boosters;
the empty attachment cell returned by preview is a different location.
It returns anchors/stones too, allowing the game to enforce its own booster
rules, and `NGP_NO_CELL` for an unobstructed path to the ceiling.

For a live aim guide, `NGPuzzlePreview` is optional caller-owned storage.
Begin with `ng_puzzle_preview_begin()` and advance with
`ng_puzzle_preview_step()`; each call spends at most eight collision substeps.
Pass a new generation when the board or geometry changes to cancel stale work.
A READY result matches the synchronous six-point preview and target exactly.
A BUSY result is only the computed path prefix, ending at its current Q8
position; show it as a short direction guide rather than a predicted landing.
Neither API changes the board, gameplay RNG, timer or notifications. The source
context and its immutable level must remain alive throughout the request.

The save buffer capacity remains 840 bytes; use the length returned by
`ng_puzzle_save()` (at most 652 bytes for a full board). New v2 saves include
all level rules in their identity and a checksum for accidental corruption.
Invalid loads leave the entire live context untouched. The loader still accepts
v1 saves, which retain their original weaker identity and lack a checksum.
Successful loads discard pending events and mark every board cell dirty; rebuild
animation from the restored state. Board/flight continuation is preserved at each
update boundary. State hashes include rules, tick, reset seed, pending swap and
attachment, and flight velocity; older hash recordings must be regenerated.
Presentation queues, fall/clear masks and search scratch are excluded from the
hash. A checksum detects corruption; it does not authenticate an untrusted save.

Limits: boards up to 12 x 16 cells, 8 colours, 4 objectives, 32 queued
events. `ng_puzzle_bubble_hint()` tries about 40 paths: call it on request,
not every frame.

## 4. Mahjong (`ng_mahjong`)

Two games that share tiles and nothing else, with separate contexts:

**Four seats, Hong Kong Old Style** (`NGMahjong`, 748 bytes). The wall
(136 tiles, 144 with the optional flowers and seasons), the deal, draws
and discards, chow / pung / kong claims with their priorities (a win
first, then pung or kong, then the next seat's chow), concealed and added
kongs with replacement draws, self-drawn and discard wins, faan scoring
(self-drawn, concealed, all chows, all pungs, half and full flush, dragon
and wind pungs, small and big dragons and winds, all honours, all
terminals, thirteen orphans, nine gates, last tile, win on a kong, flower
patterns, heavenly hand) with a limit, payments (base x 2^faan; the
discarder pays double, a self-drawn win is paid double by everyone), the
exhausted wall, dealer rotation and rounds.

CPU seats (levels 0-2) decide only from `NGMahjongView`: their own hand and
what every seat can see, never other hands or the wall order. The same
chooser is public (`ng_mahjong_cpu_choose()`), so a game can offer it as a
hint, as the demo does.

```c
static NGMahjong s_table;
NGMahjongRules rules;

ng_mahjong_rules_default(&rules);          /* no flowers, 0 minimum faan, limit 10, east round */
ng_mahjong_init(&s_table, &rules, seed);
ng_mahjong_set_cpu(&s_table, 0, NGM_HUMAN);   /* seat 0 plays; 1-3 stay CPU seats */
for (;;) {
    uint16_t can = ng_mahjong_wants(&s_table, 0);
    if (can) {                                 /* the player's decision: NGM_CAN_DISCARD, _TSUMO, _PUNG ... */
        ng_mahjong_command(&s_table, 0, NGM_ACT_DISCARD, kind);
    } else {
        ng_mahjong_update(&s_table);           /* deals, draws, CPU turns, claims: up to one discard */
    }
    while (ng_mahjong_event(&s_table, &e)) { /* NGM_EV_DISCARD: a clack; NGM_EV_WIN: the result */ }
    if (s_table.phase == NGM_PH_HAND_OVER && ng_mahjong_next_hand(&s_table) != NGM_OK) break;
}
```

**Solitaire** (`NGMahjongSol`, 1016 bytes) on any layout of up to 144
positions given in half-tile units (`ng_mjsol_layout_turtle()` writes the
classic one). A tile is free with nothing on top and its left or right side
open. Deals are always clearable: tiles are laid by removing free pairs
from the full layout, and that order is kept in `solution[]`. Pairs,
undo, hints, a shuffle of what is left (clearable again), and events.

```c
static NGMahjongPos s_layout[NGMS_MAX_TILES];
static NGMahjongSol s_sol;

ng_mjsol_init(&s_sol, s_layout, ng_mjsol_layout_turtle(s_layout), seed);
/* A on a tile: */  ng_mjsol_select(&s_sol, tile);   /* picks it, or removes the pair */
/* B: */            ng_mjsol_undo(&s_sol);
/* C: */            ng_mjsol_hint(&s_sol, &a, &b);
/* stuck: */        ng_mjsol_shuffle(&s_sol);
```

The solitaire context keeps, for every position, how many tiles lie on it
and beside it, so `ng_mjsol_is_free()` is a lookup and dealing the turtle
takes about half a second on the 68000. Layout rows (`y`) must be below
`NGMS_ROWS` (32). A tool that edits `pos`, `n` or `gone` directly calls
`ng_mjsol_recount()` afterwards.

Not implemented: robbing a kong, seven pairs, Japanese riichi rules (yaku,
furiten, dora), Chinese Official scoring, dead-wall variants. Solitaire
tiles are never routed to hand scoring.

## 5. Adventure / RPG (`ng_rpg`)

One `NGRpg` (3256 bytes) runs a world described by ROM tables
(`NGRpgWorld`): rooms on a 16-pixel tile grid (walls, water, pits, narrow
gaps, climbable faces, hazards, doorways, one-way ledges) with exits that
can be locked by conditions; a party of up to four with abilities (narrow,
climb, swim, push, fine work) where one leads and the others follow its
trail, wait, or walk to a tile to help (a bounded path search), and are put
back beside the leader only after being stuck for a while; objects (people,
signs, chests, pickups, rest spots, locks, hidden passages, pushable blocks
that fill pits, plates, levers, gates, bridges, gear trains, water channels
and valves, mirrors and lenses carrying light to receivers, bells and
sequences, balances, moving platforms, timed hazards, creatures that are
calmed rather than killed); 32 signals per room linking them; 512 flags,
32 counters, an inventory of 40 slots with key items that can't be lost,
crafting, 256 collectibles, restored regions, a checkpoint; dialogue graphs
with conditions, choices and actions. `ng_rpg_world_check()` validates the
tables and names the first faulty room and index.

```c
static const NGRpgWorld k_world = { k_rooms, k_items, k_recipes, k_lines, 1, 0, 0, 0, 1 };
static const NGRpgMemberDef k_hero = { 1, NGR_AB_PUSH, 24, 10, 2 };   /* id, abilities, speed, hp, weight */
static NGRpg s_world;

ng_rpg_init(&s_world, &k_world, seed);
ng_rpg_party_add(&s_world, &k_hero);
ng_rpg_enter_room(&s_world, 0, 2, 2, NGR_FACE_RIGHT);
for (;;) {
    ng_rpg_input(&s_world, pad & 0x0F);       /* NGR_IN_UP/DOWN/LEFT/RIGHT, the BIOS bits */
    if (pressed & A) ng_rpg_interact(&s_world);
    ng_rpg_update(&s_world);
    while (ng_rpg_event(&s_world, &e)) { /* NGR_EV_DIALOGUE: show line e.value; NGR_EV_EXIT: fade, then ng_rpg_take_exit() */ }
    /* draw s_world.party[], s_world.obj[] over the room */
}
```

Other calls: `take_exit`, `reset_room` (a room's puzzles back to how they
were on entry; permanent results stay), `use_item`, `strike`, `switch`
(change the leader), `order` (follow / wait / assist), `party_remove` (a
member leaves; the others move up, the first one left leads if the leader
went; a member added while the party is in a room starts at the leader's
side, so a companion can join for a chapter and leave after it), `respawn`, the
dialogue calls (`dialogue_start`, `_next`, `_choose`, `_offered`), the world
state calls (`flag`, `set_flag`, `check`, `run`, `count`, `give`, `take`,
`drop`, `craft`, `collected`), tile queries (`tile`, `walkable`,
`object_at`), `hash`, `save` / `load` (`NGR_SAVE_SIZE`, 320 bytes,
checksummed; a loaded game starts at the saved room with its puzzles at
their entry state).

Limits: rooms up to 40 x 32 tiles, 32 objects and 64 rooms, the path search
window is 24 x 16 tiles and paths are at most 24 steps.
`games/maiyaluna/tools/worldlib.py` shows one way to author worlds in
Python and generate the C tables, including one world per playable
character over the same rooms (each `NGRpgWorld` is just tables; a game picks
one at `ng_rpg_init`); `tools/check_world.py` plays every chapter of both
through the engine on the host.

## 6. Beat-'em-up (`ng_beatemup`)

One `NGBeatEmUp` (1728 bytes) runs one stage for one player: ground x along
the street, depth y across it and height z kept apart; eight-way walking,
double-tap dash, jump, attack chains with buffered presses, air, low and
dash attacks, a special that costs health, grabs with pummels and throws
both ways, weapons with durability, food (taken with C, or just by walking
over it on a stage with `NGB_SF_AUTO_PICKUP`: food and points always, a
weapon when the hands are empty); per-move startup, active and
recovery frames and three-axis hit volumes; hitstop, hitstun, knockback,
launches and juggles, knockdowns with invulnerable get-ups (a short hitstop:
2 frames plus a quarter of the damage, at most 6; a beaten fighter on the
ground can't be hit again, so it always goes out); an enemy that has come
into view stays in it, whole (twice its half width from the edges), and a
fighter's feet stay at least its height down the screen, so a tall boss's
head is never cut off; a crowd
director that hands out a few attack tokens, spaces the others on both
sides and telegraphs every attack; waves that lock the camera, boss
phases, props and a rideable light vehicle (mount, accelerate, steer, hop,
ram, break into a swingable wreck). Enemy kinds come from data (brawler,
charger, ranged, shield, grappler, jumper, boss) with an optional
per-kind callback.

```c
static NGBeatEmUp s_street;

ng_beatemup_init(&s_street, &k_stage, &k_hero_def, 3, seed);
for (;;) {
    ng_beatemup_input(&s_street, pad & 0x0F, pad & 0xF0);   /* held: directions, A B C D */
    ng_beatemup_update(&s_street);
    while (ng_beatemup_event(&s_street, &e)) { /* NGB_EV_HIT: spark and sound; NGB_EV_GAME_OVER: continue? */ }
    n = ng_beatemup_draw_list(&s_street, ids, sizeof(ids));   /* fighters, props, shots far to near */
}
```

Other calls: `spawn`, `add_prop`, `continue` (after `NGB_EV_GAME_OVER`),
`status`, `enemies_alive`, `hash`. Drawing a fighter in `NGB_ST_ATTACK`,
mind two `move` values that are not rows of its table: `0xFE`, the swing of
a held weapon (its move is `stage->weapons[prop[f->weapon].contents].move`),
and `NGB_NONE`, a throw's recovery; read as rows they are memory past the
table. The fighter, move, wave, prop and stage
tables are listed field by field in `ng_beatemup.h`;
`games/nosignal/scenes/ns_data.c` is a complete stage with three heroes, a
street crew, a three-phase boss, weapons and the scooter.

Limits: one player (`NGB_PLAYER`), 10 enemies out at once, 12 props, 8
shots, 64 queued events. No save: a stage is short; a game keeps its own
progress between stages.

## 7. Costs

Measured on 2026-10-18 with the SDK's m68k GCC (`tools/check_genre_codegen.py`,
`-O2`) and the host tests:

| Engine | Context (RAM) | Save data | Code at -O2 | Largest stack frame |
|---|---|---|---|---|
| puzzle | 1312 B | 840 B | 26,902 B | 588 B (`match_phase`) |
| mahjong | 748 B (table), 1016 B (solitaire) | 553 B used of 1024 | 27,422 B | 248 B (`lay`) |
| rpg | 3256 B | 320 B | 33,778 B | 192 B (`find_path`) |
| beatemup | 1728 B | - | 20,826 B | 176 B (`ng_beatemup_update`) |

None of the four uses 32-bit multiply or divide library calls at `-O2`, and
none contains the post-increment copy pattern GCC miscompiles for the
68000 (`tests/test_genre_engines.py` fails if the check finds one). A game that
selects no engine carries none of this; one that selects an engine carries
only that one (see `docs/roadmap/V1.7.7_TEST_REPORT.md` for the per-game
link check).

## 8. Tools that came with v1.7.7

- `tools/check_genre_codegen.py`: compiles each engine for the 68000 and
  reports code size, library calls, the largest frames and the known
  miscompile pattern.
- `tools/genre_qa.py`: runs a built game in MAME from a timeline (`coin`,
  `start`, `press:A`, `hold:LEFT`, `snap:NAME`) and logs RAM by symbol each
  frame (`--watch s_board+12:4`), for automated play-throughs.
- `tools/install_wip_mame.py`: copies a build into a MAME folder for
  testers, with launchers and a note per game.
- `tools/ng_artpack.py`: packs a game's pictures into C1/C2, S1 and a
  header (palette banks per 16 x 16 tile, utility tiles, the boot logo
  bank); `refine=N` on a picture adds refit passes to the palette fit, for
  paintings whose skies or roads would otherwise break into tile-sized
  blocks.
- `sound/tools/ng_synth.py` and `ng_songs.py`: instruments and a small
  sequencer that render original effects and music to WAV for the sound
  build.

## 9. What these engines are not

They are rules and simulation, not games: art, music, text, level design,
menus, saves to the memory card or backup RAM and every screen are the
game's. They do not draw, so a game keeps full control of sprite budgets
and scanline limits (see `docs/performance_rules.md`). Their maturity is
that of their tests and the four example games: MAME on AES and MVS with
EagleBIOS; physical hardware is unverified.
