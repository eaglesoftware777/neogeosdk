# NeoGeoSDK v1.7.8 — Pre-release: the football engine

## A fifth genre engine, five small SDK modules and Century XI: Road to 2030

Hello developers and Neo Geo homebrew community,

NeoGeoSDK **v1.7.8** is available as a **work-in-progress pre-release**. **v1.7.7 remains the current release**; v1.7.8 adds to it without changing what v1.7.7 games rely on.

It brings association football to the SDK. `ng_soccer` is a fifth one-file genre engine, next to the puzzle, Mahjong, adventure and beat-'em-up engines of v1.7.7. Five small modules came with it, useful to any game: rating curves for difficulty levels, cooperative tasks, FIX-layer text scripts, a developer's test menu and a second player joining a game already under way. The pre-release also carries the first release of **Century XI: Road to 2030**, the original football game they were written for and tested with.

| In this pre-release | What it is |
|---|---|
| `ng_soccer` (`GAME_GENRE_ENGINE = soccer`) | The ball in world space, weighted passes, high balls, the goal and touch lines, offside, and a 32-side championship from up to 64 nations |
| `ng_tune` | Team ratings into game numbers through per-level curves, nudged by score, time and cup round |
| `ng_task` | Cooperative tasks on their own stacks: `ng_task_sleep(n)` waits n frames |
| `ng_fixscript` | Text screens on the FIX layer as short scripts: cursor, palette, typewriter, waits |
| `ng_devmenu` | A developer's test menu over a running game: slow motion, frame step, timer stop, free camera, RAM watch, sound test |
| `ng_join` | A second player joining mid-game through the system ROM, credit included |
| Century XI: Road to 2030, 4.0.0 | Original parody football, cartridge ID 787, MVS and AES |

**Repository:** https://github.com/eaglesoftware777/neogeosdk

**Pre-release and downloads:** https://github.com/eaglesoftware777/neogeosdk/releases/tag/v1.7.8

**Status checked:** 8 November 2026. SDK 1.7.8, Framework 1.6. Game files keep `WIP` in their names and carry the game's own version (4.0.0), independent of the SDK. Everything was tested on the host and in MAME; nothing in this pre-release has been tested on real hardware yet.

## 1. Where v1.7.8 sits

| Version | What it is |
|---|---|
| v1.7.0 | The 2D engine release |
| v1.7.7 | The games release: seven games, four genre engines, the fighting core, EagleBIOS, experimental Neo Geo CD. **Current release.** |
| v1.7.8 | Pre-release: the football engine, five modules, Century XI |

The framework's public calls change by additions only: `make api-check` (`tools/api_freeze.py`) passes against the list frozen at Framework 1.0, and games written for v1.7.0 and v1.7.7 build unchanged. `NG_SDK_VERSION` is 1.7.8 and `NG_FRAMEWORK_VERSION` 1.6, for compile-time checks.

## 2. `ng_soccer`: what every football game needs

The engine is the part of a football game that has no look of its own. It was written inside Century XI first, tested there over many play-test rounds, and then moved into the SDK. The game now runs on it.

Pick it in `game.mk`:

```make
GAME_GENRE_ENGINE = soccer
```

The build compiles `sdk/2d_engine/ng_soccer.c` into `out/ng_soccer0.o`, links it after the core engine and passes `-DNG_GENRE_ENGINE_SOCCER=1`. `make GAME=mygame genre-info` shows what was resolved.

**Units.** The pitch runs x from the left goal line (0) to the right one and y from the far touchline to the near one. Ball positions and speeds are Q4, sixteen to a pitch unit. One call of `ngs_ball_fly()` is one frame.

**Arithmetic.** On the 68000, plain C turns a 16-bit `a * b` or `a / b` into a library call of several hundred cycles. `ngs_mul()` and `ngs_div()` are one `MULS` and one `DIVS`, always inlined. The championship's remainders are taken in two 16-bit `DIVU` steps. The engine never calls the library's 32-bit divide.

**The ball** (`NGSBall`, 26 bytes):

- gravity, and air drag that is the same in both directions, so shots at either goal fly alike;
- bounces that take a quarter of the ball's pace;
- the grass's grip, and a ball struck hard skims until it slows;
- a swerve (`curve`) that turns the flight whichever way the ball goes, with the fraction of each frame's push carried over, so a small bend adds up;
- `ngs_ball_strike()` to send it toward a spot at a speed and a lift;
- `ngs_pass_speed(d)`: the speed that leaves a ground pass rolling to rest a little past a spot d units away, so it arrives firm;
- `ngs_ball_meet()`: the frames until a high ball drops to a given height, and where, for keepers and for the men who go up for it.

**The lines.** `ngs_ball_line()` answers in play, goal, the frame (posts or bar, with `ngs_frame_rebound()`), wide of goal, or over a touchline. Corners, goal kicks and throw-ins stay the game's decisions.

**Offside.** `ngs_offside_line()` gives the second-last defender's line, or the ball's, or halfway. `ngs_offside_mask()` gives the attackers past it by more than a margin, plus the closest call inside the margin for a video review.

**The championship** (`NGSCup`, 810 bytes on the 68000, saved as it is):

- up to 64 nations; the player's side always plays, and 31 others qualify by strength plus luck;
- a seeded draw into eight groups of four, then 48 group matches;
- round of 16 (group winners against the paired group's runners-up), quarter-finals, semi-finals, third place and the final: 64 matches in all;
- tables at 3/1/0 points, ranked by points, goal difference, goals scored, then the order of the draw;
- `ngs_cup_next()` plays every computer match before the player's next one through the game's own result callback, so no finalist is ever simply picked;
- `ngs_cup_result()` refuses unknown, repeated or inconsistent results;
- scorers for the golden boot, and the player's bookings carried from match to match: two, or a red, and a man sits out the next.

```c
static const NGSPitch k_pitch = { 1536, 704, 88, 40, 3 };   /* length, width, half goal, bar, gravity */
static NGSBall s_ball;
static NGSCup s_cup;

if (s_ball.owner == NGS_NOBODY) {
    if (ngs_ball_fly(&s_ball, k_pitch.gravity, frame) & NGS_FLY_BOUNCE) play_bounce();
    switch (ngs_ball_line(&s_ball, &k_pitch, &right)) {
    case NGS_LINE_GOAL:  goal(right); break;
    case NGS_LINE_FRAME: ngs_frame_rebound(&s_ball, &k_pitch, right, rnd()); break;
    case NGS_LINE_WIDE:  corner_or_goal_kick(right); break;
    case NGS_LINE_TOUCH: throw_in(); break;
    }
}

ngs_cup_init(&s_cup, my_nation, seed, 48, nation_power);
while ((n = ngs_cup_next(&s_cup, quick_result)) >= 0) {
    play_match(s_cup.fixture[n].home, s_cup.fixture[n].away, &home_goals, &away_goals, &winner);
    ngs_cup_result(&s_cup, n, home_goals, away_goals, winner);
}
```

Players, formations, the computer's thinking, the referee's manner, the camera, drawing and sound stay in the game. Like the other genre engines, `ng_soccer` never touches video RAM or the sound driver, uses no heap, no floating point and no hidden globals, and keeps everything in the caller's structures.

**Cost:** 6,002 bytes of code at `-O2`, no library divide or multiply calls, largest stack frame 164 bytes (`tools/check_genre_codegen.py`).

## 3. Five small modules

They live in the SDK library (`out/libng_sdk.a`). A game that calls none of them links none of their code.

### `ng_tune`: difficulty as data

Ratings run from 0 to 31. Each number a game needs (frames between CPU decisions, a tackle chance, an aiming error) is a curve: for every difficulty level, the value at rating 0, the value at rating 31 and a shape between them. A nudge shifts the rating while a match is played: a side two goals down plays a little sharper, and each cup round is a little harder. A value costs one `MULS`.

```c
static const NGTuneSpan k_think_span[8] = { {18, 13}, {15, 11}, {13, 9}, {11, 7}, {9, 6}, {7, 4}, {5, 3}, {4, 2} };
static const NGTuneCurve k_think = { k_think_span, 8, NG_TUNE_STRAIGHT };
frames = ng_tune_value(&k_think, level, team_rating + ng_tune_nudge(&k_nudge, goal_gap, late, round));
```

### `ng_task`: code that waits

A cut-scene, a referee's joke or a caption that zooms in, holds and leaves is normally a frame-by-frame switch on a step counter. As a task, it is straight-line code:

```c
static uint16_t s_stack[192];
static void caption(void *arg)
{
    for (k = 0; k < 16; k++) { grow(k); ng_task_sleep(1); }
    ng_task_sleep(90);
    hide();
}
ng_task_spawn(caption, 0, s_stack, sizeof s_stack);   /* the frame loop calls ng_task_run() */
```

Each task runs on its own stack, and the switch saves only the registers C must preserve. `ng_task_stack_left()` reports what a task never touched, so a stack can be sized from a test run.

### `ng_fixscript`: text screens as scripts

```c
static const char k_card[] =
    "{c}{8,4}{p3}THE ROAD TO 2030\n"
    "{6,10}{p0}{t2}EIGHT GROUNDS. THIRTY-TWO NATIONS.\n"
    "ONE CUP.{w60}\n"
    "{6,20}{p2}{t0}PRESS A{w}";
```

The commands cover cursor, palette, typewriter speed, waits for frames or a button, centring, and blanked or filled boxes. A script steps once a frame from the game loop, or plays from inside a task.

### `ng_devmenu`: a test menu over the running game

Held START plus D opens a small FIX menu and freezes the game behind it. It offers:

- speed (full, half, quarter, eighth, or one frame per press);
- a timer stop and a free camera;
- hit boxes for the game to draw;
- four RAM words watched live;
- a sound test (up and down for ±1, left and right for ±10).

The game decides which switches it honours. A release build need not link it at all.

### `ng_join`: the second player arrives

While a game is played, the system ROM watches START and calls the cartridge's `PLAYER_START` for a newcomer. `ng_join_bios_start()` accepts him, has the system take his credit, and `ng_join_poll()` tells the game loop who came in. Century XI hands the CPU's side to player two on the spot.

The credit goes through two new calls in `sdk/cabinet/ng_sys.h`, `ng_sys_credit_check()` and `ng_sys_credit_down()`. The system ROM's credit routines change registers that compiled C expects a call to keep. Called the plain way (`CALLNEOGEOF`) from game code, a value held in one of them came back wrong: `ng_join` turned away a player who had just paid. A function pointer held in another sent the CPU into backup RAM, which restarts the board. The new calls save those registers. `docs/cabinet.md` shows how to take a player's credit with them; it also works on the US system ROM, which keeps player two's credits apart from player one's.

## 4. Rendering, sound and build fixes

- **Exact-width shrink.** With `ng_sprite_group_set_exact_width()`, a shrunk sprite group shares its exact scaled width out across its strips a pixel at a time, so a zooming caption grows smoothly instead of in sixteen-pixel steps.
- **Streamed columns first.** `ng_vram_commit_strips_first()` puts a scrolling background's waiting strip columns up before a game's own blank-time writer, so the edge of the screen never shows a column one blank late.
- **Starving groups.** In deferred VRAM, a group that has waited 12 frames at the head of its list goes without the safety margin. After 48 frames it goes even if it runs a few lines into the picture. A wide first write that could never fit a short blank used to wait forever.
- **Sound queue.** With `NG_SOUND_QUEUE`, `isZ80Ready()` moves the queue on instead of waiting for the driver. The wait could hold a frame for up to a whole frame.
- **Linker script.** Code and read-only data from SDK library members are placed by a catch-all, so a library module's tables always land in ROM.
- **Sound build.** The sample converters skip files already converted from the same source by the same encoder. A game with hundreds of voice samples no longer re-encodes them all on every build.
- **Tools.** `tools/softdip.py` builds a game's soft DIP table (the arcade operator's settings) from a short text file. `tools/genre_qa.py` gained `coin2` and `start2` for player two.

## 5. Century XI: Road to 2030, version 4.0.0

An original arcade football game, and a cute parody of 1994 international football. It follows a fictional championship in an imaginary Japan, with no connection to any real competition, team or player.

- **48 parody nations,** each proud of the dish, the dance or the landscape it is named after: Sambalia, Baguettia, Pastalia, Paellania, Sushipon, Jollofia, Burgerland and the rest. Each has its own flag, kits, chant, roster of joke names called by the commentator, and way of playing.
- **The Road to 2030** is a full 64-match championship. 32 of the 48 qualify, then come an animated draw, group tables with top scorers, the knock-out bracket, both sides' line-ups before every match, bookings and suspensions carried between matches, and a trophy ceremony with the golden boot. The final is at the Big Ramen Bowl in Tokyo-yo.
- **Other modes:** Quick Cup, Exhibition, two-player Versus (with join-in), Penalty Challenge and a Training Ground. There are eight CPU levels, Classic and Modern controls, match length and golden-goal options, retro or modern laws, and video review on request.
- **Playing:**
  - weighted passes that find a man the way the stick points;
  - long balls taken on the chest, one-twos and through balls;
  - crosses that the man under them attacks by himself, with headers, volleys and overhead kicks;
  - the chip, and shots wound up by holding B.
  - When a shot comes, the keeper is the player's to move and dive. Penalties are taken in a low 3D view.
- **Presentation:** a pseudo-3D stadium on a bright 1994 afternoon, with chibi figurines in three depth sizes. Stands wear the day's colours. Every goal brings GOAL! GOAL! GOAL!, a different celebration and a replay. A coach shouts in his television window, and three referees (one loves cards, one is short, one naps) each have something to say.
- **Sound:** the announcer, commentary that names the men, every nation's chant, a crowd that rises with danger, and band music for the menus, penalties, final and champions.
- **Saves:** options, records and a Road in progress, in the arcade board's backup memory or on an AES memory card. Saving to the memory card has not been exercised yet; backup memory was checked in MAME.

The game's full story is in `artworks/storylines/centuryxi` (English, Spanish and French). Controls are in its README; what was measured and what is not built yet is in `games/centuryxi/docs/STATUS.md`.

## 6. Tests and measurements

All the evidence below comes from the host or from MAME, with EagleBIOS unless a row names other system ROMs. **Nothing in this pre-release has been tested on real hardware.**

| Check | Result |
|---|---|
| `ng_soccer` host tests (`tests/soccer_test.c`) | Pass: arithmetic, passes against the grass, flights, the meet point against the flight it predicts, the lines, offside, 1,000 championships (entrants, fixtures, tables, bracket, determinism), bookings |
| Genre engine suite (`tests/test_genre_engines.py`) | 9 of 9 pass: five engines on the host, the documented examples, a C++14 consumer, 68000 codegen, the build selector |
| Century XI moved onto the engine | Byte-identical behaviour: every frame of 47 host matches (CPU and scripted players, the whole world hashed each tick) and 300 championships before and after; in MAME the same CPU-v-CPU match to the minute |
| Century XI host tests (`games/centuryxi/tests/run.py`) | Pass, under the address and undefined-behaviour sanitisers |
| Century XI coins and Start (`games/centuryxi/tests/test_boot_flow.py`, MAME) | Pass on MVS under the US, Europe, Japan, Asia, UniBIOS 4.0 and EagleBIOS system ROMs and on AES under the Asia, Japan and EagleBIOS ones: one credit per game, free Starts in the menus, a second game after a session, player two paying one credit for Versus, a paid join mid-match |
| Century XI frame rate (a whole CPU-v-CPU match, MAME) | 57-60 frames a second for most of play; 4 seconds of five minutes under 50, with the match keeping its speed |
| Century XI ROM budget | P 62%, C 49%, S 18%, M 6%, V 90% |
| The seven v1.7.7 games on the new SDK (MVS, 60 s each in MAME with the same inputs, a snapshot every 5 s and every byte sent to the sound CPU) | All build. Maiya, Seven Wonders, Maiya & Luna, No Signal and Mahjong Demo: every snapshot and every sound byte identical to their v1.7.7 ROMs. Sky Lance: identical to the same source built on v1.7.7 (sound bytes the same, each within a frame). Shadow League: from 20 s it meets a different first CPU opponent, because the sound queue no longer holds the main loop and its random pick falls on another frame; rebuilt with only the old `isZ80Ready()`, it matches its v1.7.7 ROM exactly |

## 7. Downloads

| File | What |
|---|---|
| `CenturyXI-WIP-NeoSD_MVS_v4.0.0.neo` / `CenturyXI-WIP-NeoSD_AES_v4.0.0.neo` | NeoSD images (cartridge data only) |
| `CenturyXI-Darksoft-MVS_v4.0.0.zip` / `CenturyXI-Darksoft-AES_v4.0.0.zip` | Darksoft Neo Geo Multi folders (no BIOS) |
| `CenturyXI-BackBit-MVS_v4.0.0.zip` / `CenturyXI-BackBit-AES_v4.0.0.zip` | BackBit Platinum folders (no BIOS) |
| `CenturyXI-WIP-EagleBIOS_v4.0.0.zip` | MAME, MVS: `mame neogeo -rompath roms -hashpath hash -bios euro -cart1 centuryxi` |
| `CenturyXI-WIP-AES-EagleBIOS_v4.0.0.zip` | MAME, AES: `mame aes -rompath roms -hashpath hash -bios asia -cart1 centuryxi -memc card.bin` |
| `CenturyXI-README_v4.0.0.txt`, `CenturyXI-CHANGELOG_v4.0.0.txt`, `CenturyXI-MANIFEST_v4.0.0.txt` | Notes and controls, every change, sizes and checksums |

The only system ROM in any package is EagleBIOS, the SDK's open firmware.

## 8. Using it

```sh
make GAME=centuryxi PLATFORM=mvs all        # or PLATFORM=aes
python3 games/centuryxi/tests/run.py        # the game's host tests
python3 tests/test_genre_engines.py         # the five genre engines
make GAME=mygame genre-info                 # what a game links
```

To start a football game of your own, set `GAME_GENRE_ENGINE = soccer`, include `ng_soccer.h`, keep an `NGSBall` and an `NGSCup` in your game's RAM, and read section 7 of `docs/GENRE_ENGINES_V1.7.7.md`. Century XI's `scenes/` shows the whole of a match built around the engine.

Reports are most useful with the exact build (the MANIFEST's checksums), the machine and BIOS, the steps that reproduce the problem and a capture. A crowded goalmouth, a full Road to 2030 and a memory card on a real AES would be especially valuable.

## Credits

NeoGeoSDK and its games are by Eagle Software. Century XI's voices use the
Kokoro-82M model (Apache 2.0) through kokoro-onnx (MIT), with the Koniwa
kumonoito reading (CC BY 3.0) for the coach's Japanese voice, all credited in
`games/centuryxi/sound/VOICE_SOURCES.md`.
