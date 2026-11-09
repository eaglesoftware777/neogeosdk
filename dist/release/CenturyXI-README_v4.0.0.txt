# Century XI: Road to 2030

Original arcade football for Neo Geo, cartridge ID 787. Current release: **4.0.0** (see [CHANGELOG](CHANGELOG.md)); [status and what is still open](docs/STATUS.md).

A cute, funny parody of 1994 international football: forty-eight nations (thirty-two of them reach each championship's finals), large chibi figurines, a pseudo-3D stadium and a full 64-match championship in a fictional Japan-hosted tournament: the draw, group tables, the knock-out bracket, line-ups, top scorers and suspensions. It has no connection to any official competition, and no real player names or likenesses are used: every man is an original character.

## Controls

Choose Classic or Modern in Options; both drive the same football.

| Button | Classic, with the ball | Classic, without | Modern, with the ball | Modern, without |
|---|---|---|---|---|
| A | Short pass | Sliding tackle (a standing tackle when close) | Short pass | Standing tackle |
| B | Shoot: hold to wind it up, let go to strike | Change player | The same | Sliding tackle |
| C | Long pass; near goal a chip; wide a cross | | The same | Change player |
| D | | | Sprint | Sprint |

- A and B together in a one-on-one is the feint, and the stick chooses its direction.
- Near a ball in the air, B goes for goal (a volley, header, diving header or overhead kick, depending on its height and how you face) and A flicks it on.
- B is a shot you wind up: a tap places it, the longer you hold it (the POWER gauge) the harder it flies, and it goes when you let go.
- When the other side shoots, the cursor jumps to your keeper. Leave him alone and he saves on his own; touch the stick and he is yours: the stick moves him, A, B or C dives the way the stick points (with the stick at rest, across to where the shot is going). Even after he has started a dive by himself, A, B or C with the stick in the first moments turns it your way.
- In a penalty against you, the stick moves your keeper along his line during the run-up, A or B commits the dive (UP with it for a high one). Go too soon and the taker may see it.
- When your keeper has the ball in his hands the cursor is on him: the stick walks him about his box and points, A rolls it out to a man close by, B kicks it long to the freest man up the pitch, C throws it long and flat. Wait three and a half seconds and he kicks it long himself. Your defenders split wide to take it from him.
- After a pass the man it is for runs to meet it on his own, along the ground or where a high ball drops, and takes a dropping ball on his chest; hold D and he sprints to it.
- As a ground pass travels: A with the stick at rest asks for it straight back (the one-two), A with the stick pointing plays it on first time that way, B shoots first time. As a high ball comes, A or B goes up for it: a volley, a header or the overhead kick, at goal (B) or on to a team-mate (A).
- Your own cross or long ball: the cursor goes to the man under where it drops, and he goes for it by himself (near their goal, at goal: a header, a volley or the overhead kick by its height). Steer him and he is yours; A or B sends him for it from anywhere. Near their box the bottom line says what C does there: the chip over the keeper, or the cross.
- A pass with the stick finds the man nearest that way, out to the side if need be, and never rolls into nobody; at the kick-off, A with the stick at rest gives it to the forward alongside.
- Defending, leave the stick alone near their man on the ball and yours jockeys him: goal-side, a stride off, facing him. A close to him is a standing tackle.
- At a free kick or a corner, D opens the aim view: the stick moves a target, A plays it along the ground, C lofts it, B strikes it.
- START opens the pause menu: tactics, substitutions and quit.

## Building

The game links the SDK's football engine (`GAME_GENRE_ENGINE = soccer` in `game.mk`: `sdk/2d_engine/ng_soccer.c`, the ball, the lines, offside and the championship); everything else is in `scenes/`.

- `make GAME=centuryxi PLATFORM=mvs all` builds the game, or `PLATFORM=aes`. `make GAME=centuryxi test` starts the emulator.
- `python3 games/centuryxi/tests/run.py` runs the host tests.
- `python3 games/centuryxi/tests/test_boot_flow.py --set dist/neo/centuryxi --elf out/game --platform mvs` (or `aes`, after building that platform) checks coins, Start, Versus and join-in under each system ROM in MAME; the system ROMs come from `roms/` (kept out of git).
- `games/centuryxi/tools/sfx.py` regenerates the sounds, and `tools/art.py` (run by the build) packs the art.
- The voices' sources and licences: [sound/VOICE_SOURCES.md](sound/VOICE_SOURCES.md).
- `tools/voices.py` (with kokoro_onnx) records the announcer, the chants and every shirt name; `--names` only the first 316 names; `--new` the 48 sides' added chants and names (they come after the first names, which keep their numbers).
- `tools/live_debug.py mvs|aes C:\mame\neogeosdk --elf out/game`, after building the platform that is installed, writes the play-test launcher `live\live_centuryxi_<mode>.bat` and its logger (see `live\README-CENTURYXI.txt` there); `--out FILE` writes only the logger, for `tools/genre_qa.py --lua`.
