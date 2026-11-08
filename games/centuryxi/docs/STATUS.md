# Century XI V4 development status

Build identifier: `4.0.0`, the release. What is built is listed with how it was checked, and what is not built is listed plainly.

The look and tone follow the owner's review of dev.2: a 1994 summer, every player a man, parody throughout, with football that is still football.

## 4.0.0 (owner's notes on dev.14, and the dev.14 session)

**The SDK's football engine:** the ball's flight, swerve, grass and bounces, the weighted passes, where a high ball comes down, the posts and bar, the offside line and the whole championship (qualifying, draw, tables, bracket, scorers, bookings) moved into `sdk/2d_engine/ng_soccer.c` (`GAME_GENRE_ENGINE = soccer`), and the game now runs on it. `CxBall` and `CxTournament` are the SDK's `NGSBall` and `NGSCup` under the game's names, field for field, so saves keep their layout. Checked: every frame of 47 host matches (CPU and scripted players, the whole world hashed each tick) and 300 championships byte-identical before and after; in MAME the CPU-v-CPU bench plays the same match (1-0, same shots, saves, foul and card, full time 0.05 s later). It costs two calls a tick: 4 seconds of play under 50 frames a second against 2 (123 dropped pictures against 118 in five minutes).

**The net:** the grass under it showed as a checkerboard of light and dark 16 x 16 tiles: textured grass, its shade, the net and the sunlit grass beside it were more colours than one tile's palette holds. Under the net's footprint the grass is now the two bands' flat shade. The coach's box showed empty for a frame before his picture (it now waits two frames for him).

**Coins, Start and the second player, checked against Maiya:** the cartridge's start-up code was the older template that Maiya has since fixed. The Start flag lived in backup RAM at D00100, which the US arcade system ROM write-protects during play: Start took a credit and the title stayed (a second credit got in). It now lives in work RAM, cleared with interrupts masked; the title reports user mode 1; a Start while player 1 is in the game is declined (the system asks on every press, and START also skips scenes between matches, each one a credit); the end of a session marks him game over so the next Start pays. Player 2's versus credit is taken by the game with the decrement set, from his own count where the system keeps one per chute (the US system ROM): it was two credits, and a further try ran into backup RAM and restarted the board, because the system's credit routines change registers the compiled caller keeps (the SDK now has `ng_sys_credit_check`/`down`, which save them). The same fault made `ng_join` refuse a player 2 who had paid mid-match. The header names the boot logo's C bank ($49) on both boards and the console's eye-catcher flag (2), as the other games; the arcade eye-catcher is a black screen that commits nothing before work RAM is cleared. Player 2's cursor reads 2P. Checked in MAME by `tests/test_boot_flow.py` (below); the old 4.0.0 build fails every one of its scenarios.

**More colours:** the title, the stands, the VAR room and the penalty view have 32 palettes each (banks 96-127, one picture on screen at a time; they had 12 to 16), the pitch and near stand 32 (banks 200-231, from 16). Palette bank switching swaps all 256 palettes at once and cannot add colours to one screen; the hardware's single bank was three-fifths unused.

**The championship:** an animated draw of the 32 who qualified, the knock-out bracket before each knock-out match and at the end, both sides' line-ups before every match, the leading scorers under the group table (the player's goals as scored, the CPU's results credited mostly to forwards), bookings carried from match to match (two, or a red, and a man sits out the next; the bench takes his place), and a trophy ceremony with five of the champions celebrating and the golden boot named. Host test: scorers, line-ups without a suspended man, 1000 tournaments.

**Text:** the C hint near their box has its own row (it mixed with the one-on-one's A+B FEINT), the kick-off help fits the screen.

## New in dev.14 (owner's notes on dev.13, and the dev.13 session)

The session (Classic controls, BAGUETTIA against CAMEROAR): 59-60 frames a second, one slow second. Thirteen of the player's passes rolled into nobody; all four of his fouls were Classic A slides and every one was a yellow card; six of his long balls were headed away before they arrived; the CPU scored two overhead kicks, he never got a header.

**Crosses and long balls of the player's own** (measured on the host from the right byline, 60 crosses): before, 0 shots: B had to be pressed within 120 units of the drop or it was a slide on the grass, and the man under the cross stood still. Now the cursor goes to the man under it, he goes for it by himself, and near their goal at goal: 29 shots, 13 goals (11 headers, 13 diving headers, 5 overhead kicks). B or A sends him from anywhere and is never a tackle. A pass in the air is its man's: team-mates leave it, a defender beats him to it less often.

**Passes:** the stick's pass now looks across its whole half of the pitch, nearest the stick's line first (a 60-degree cone left sideways passes with nobody), and with nobody that side goes to the best man anywhere. Host, 50 passes each way from midfield: into space 16 -> 0 pointing at the near touchline, 0 elsewhere; 33-50 reached. **Kick-off:** the second forward stands a stride back and to the side, a midfielder further back, and A with the stick at rest plays it to him (30 of 30). Beside a player's man on the ball, the two nearest team-mates come to help, one alongside, one behind.

**Defending:** a Classic A close to the man on the ball is a standing tackle; a late slide that catches a man is a foul half the time and a booking only from behind (CPU averages: 1.2 cards a game, from about 2); the stick at rest near their man on the ball and the player's man jockeys him goal-side. Standing tackles weigh the dribbler's touch.

**Dribbling:** closer touches at a run, and a good dribbler turns quicker with the ball. **The chip** reaches 360 units out; the bottom line tells the player what C does near their box.

**Graphics:** the pitch and near stand have sixteen palettes (banks 200-215) instead of four, and the grass is repainted: sun patches and the stands' shade (world space), the mower's cross-cut, the blades' texture, worn goalmouths and spots. The men are drawn bigger far off and in the middle (52 and 56 pixels from 46 and 53; the near size, 60, already fills the 64 x 80 frame). Frame rate held: CPU benchmark 0 seconds under 50, routes 0; goal scene 72 sprites a line at most.

## New in dev.13 (owner's notes on dev.12, and the dev.12 session)

The session (fifteen minutes on AES, six matches) ran at 59-60 frames a second almost throughout. Of its 17 goals the CPU scored 16, nearly all the same shot from 180-420 units; the player's keeper's kicks and throws were mostly lost; his long balls were lost or ran out of play.

**The keeper is the player's.** A shot at his goal moves the cursor to him: left alone he saves as before; the stick takes him over (steps about his box), A, B or C dives the stick's way. A CPU dive already under way turns the player's way with a button in its first ten frames. Host test, near-post shots from 220 units: the CPU keeper lets in 20 of 40, a dive the right way 8-24 frames before the line 0-4. Penalties: the keeper shuffles along his line through the run-up, commits with A or B (UP for high), and a CPU taker may read an early dive.

**A shot straight at a standing keeper went in** (40 of 40 from 320 units: only a diving keeper could save). He now stops what reaches him: 0 of 40. Shots and saves share one routine (cx_keeper.c keeper_stop).

**Long balls.** Measured on the host: they landed within 14 units of the man but the bounce carried on at full pace to the defender behind him (lost 20 of 40 with a marker 40 units off). Now the bounce takes a quarter of the pace off, the long ball is driven lower, the man it is for takes it on his chest as it drops, every pass target is kept on the pitch, and the long ball looks for the freest man: 27 of 40 kept, none out of play. The keeper's punts and throws: 4 and 0 of 60 to their man before, 60 and 60 now (a punt with nobody the stick's way goes to the best man anywhere; the roll looks further; the back men split wide when he holds it).

**Shots** are wound up: B held fills the gauge, letting go strikes it (a tap 205, a full wind-up 260). **First time:** B as a ground pass comes shoots first time, A with the stick plays it on first time. A ball met in the air on to a man is a volley, an overhead kick or a header by its height.

**The CPU attacks in more ways:** fewer shots from range (the side works it in), cut-backs from the byline, crosses for headers and volleys, the one-two, more through balls, shots aimed at the side the keeper has left open. **Defending:** backs and midfielders mark the other side's four most advanced men (cx_marks, every 16 frames), goal-side, tight near the box. Host averages, 32 CPU matches: 1.6 goals a game, by kind 34 shots, 16 from range, 12 volleys, 4 headers, rebounds, a chip, an overhead kick.

**Goal scene:** each goal's celebration is the nation's own a third of the time, otherwise another, never twice running for a side, and the men who join the scorer each do their own. The replay is the goal only, with one caption line under REPLAY; the coach, the talk and the caption are off before it, and the kick-off follows at once (the scorer used to celebrate a second time). The coach's line moved from row 26 (over the stand's boards) to row 10, and waits, with his picture, for the GOAL words to finish.

**48 nations**, the 2026 finals' number: SILKINA (the silk road), HYGGEMARK, FONDUZERLAND, PIEROLAND, KAJMAKIA, DELIGHTIYE, TARTANLAND, CRYSTALIA, ROSEGARIA, FETALLAS, CONDORIA, LLAMARU, HARPAGUAY, EQUATORIA, ELEPHANTOIRE, FENNECIA, with flags, kits, rosters of joke names, chants and 215 newly recorded names (the first 316 recordings keep their numbers: tools/voice_names_v1.txt, voices.py --new). The championship's 32 qualify from the 48 (strength plus luck; the player's side always); its table is kept by draw slot, so the save is the same size (save version 4).

**Looks:** each region's men are drawn from the figures that suit it (the East Asian sides mostly the short spiky cut, some shaved, never the frizz-and-goatee or the locks), a warm light skin tone, and a build: a tall man is drawn a size up (target men, stoppers, keepers, and more of them where sides run tall), a slight one a size down.

**The stands carry the day's two sides:** supporters in the home shirt and flags in the home nation's colours at the left end, the away side's at the right, in the far and the near stands (painted in key colours; art.py finds where they landed, cx_crowd_kits sets them at kick-off).

**Frame rate** held: CPU-against-CPU benchmark 1 second under 50 of two minutes, routes A and B none (the marking pass was a 31-line spike every 4 frames at first; one sweep every 16 frames now, and the lane and receiver maths use one-instruction multiplies).

## New in dev.12 (from the owner's dev.11 play-test session)

The session (seven minutes on AES, two matches) still had 75 of 253 seconds of play under 50 frames a second, mostly in the third of the pitch and the boxes. The logger also showed what the player's presses did: B for a high ball in his own half was taken as a shot at a goal a thousand units off, and a long kick from his keeper was lost while the player steered the man it was for.

**The frame rate.** Measured with raster marks on every frame and function profiles in MAME, on a CPU-against-CPU benchmark (the player's side handed to the CPU, two minutes of open play, the same every run): seconds under 50 frames, 34 for dev.11, **0** now, every second between 54 and 60.
- A frame already used about 210 of its 264 lines (the match tick about 105, drawing the men about 55). Paying a late frame's tick by running two ticks with no drawing held one picture for three frames and then jumped three ticks, and that frame (about 240 lines) was often late itself, which tipped play into 30 pictures a second. A late frame on its own (none in the half second before) is now let go: the picture stands two frames, and the match loses one tick. Over two minutes the clock ran 97.6% of real frames, a foul stoppage included. Late frames in a run are still paid.
- While a ball was in the air (crosses, corners, long balls) every outfielder ran the full AI every tick; only the men it can drop on need to. At a set piece being taken, every man did too. Both now run it one tick in four like the rest of the shape, a keeper holding the ball excepted. A corner's tick went from 130 lines to about 100. The match plays exactly as before (host suite identical, checked to fail when the rule is changed).
- Drawing the men: each figure's poses, first tile and shirt-number tile are worked out when the kits are set, not every frame, and the per-man helper is inlined. A check build ran the old and new code side by side for 1,867 frames: identical sprite data (and caught a deliberate one-tile error in 252 of 681 frames).
- Movement: the rare states (slides, dives, picking the ball up) moved out of the step, and the 22 steps run in one loop, so each man no longer pays a call and ten saved registers. A wall's distance at a free kick or corner used two library divisions a tick: now one 68000 divide each.

**B for a high ball deep in his own half** heads or volleys it up the pitch instead of shooting at a goal a thousand units away (host test added; it fails on the old rule).

**Long balls reach their man.** The man a lofted pass is for now runs to where it drops whatever the stick says, as along the ground; the keeper's long kick looks for a free man before the furthest one up.

## New in dev.11 (from the owner's dev.10 play-test sessions)

The play-test logger's three sessions showed no flickering men, but the picture at about 30 frames a second for most of the match (320 slow seconds in a seven-minute AES session), and only half the player's aimed passes reaching their man (the CPU's: 74%), most of the others intercepted while the player was steering the receiver.

**The frame rate.** Three causes, measured with a function profile and raster marks in MAME:
- Every sound command waited for the sound driver to answer, and now and then the driver took up to a whole frame; the crowd's volume step every eight frames was enough to push a frame late, and the catch-up tick then held the picture at 30. The SDK's queued sound mode (`NG_SOUND_QUEUE`) now never waits (its senders' ready checks only move the queue on), and Century XI builds with it.
- A man was 42 bytes, so every `w->m[i]` was a multiply; at 64 bytes it is one shift. Movement costs a third less per man, the AI a sixth less.
- The men keeping the team's shape decide where to run one tick in four; the AI is no longer called for them on the other three.

On the same automated route: seconds of play under 50 frames a second, 21 of 24 before, 5 of 24 after. A crowded goalmouth can still drop below 60.

**Passes along the ground always reach the man who can reach them.** The man a ground pass is for now goes to meet it whatever the stick says (D sprints to it); only for a ball in the air can the player steer him elsewhere. Host test, the stick pointed away from the ball and D held: 60 of 60 taken.

**A pass the player never asked for.** A, pressed as the man lost the ball, was kept for the A+B chord and fired a pass the next time the side won the ball. It now dies with the ball (host test added).

**Logger.** Kick-off taps no longer count as passes in the summary, and the cut to the replay is no longer reported as men vanishing.

## New in dev.10 (owner's notes on dev.9)

**The pitch near the goal paints right.** The pitch's new columns as the camera moves were queued behind the men and could wait several blanks: old grass, or the goal net, left where the camera had moved on (the "ghosts" and wrong painting going to the net). They are now written first, in the same blank as the move, and the camera never asks for more than one new column a frame. Measured in MAME on a fast run to the net and back: columns a blank late in 354 of 1540 frames (up to 17 behind) before, none after.

**Men flickering at the screen's edges.** A man stepping one pixel in and out of view each frame reshuffled the depth order of every man after him, rewriting all their slots twice in two blanks (more than a blank holds). A man must now go further out to leave the picture than to come in, and two men at almost one depth no longer swap back and forth. The play-test logger (below), which looks for any man missing from the picture for a frame wherever his slot has moved, finds none on a 60-second automated route through midfield, a goal and the celebration.

**Your keeper is yours.** With the ball in his hands the cursor goes to him: the stick walks him about his box and points, A rolls it out short, B kicks it long (up the middle with the stick at rest), C throws it long and flat. He kicks it himself after three and a half seconds. A hint shows on screen while he holds it.

**Passes reach their man.** Two faults made passes on the run miss: the receiver kept running the way the stick still pointed, and A pressed to ask for the one-two also made him lunge into a tackle, so he could not take the ball. He now runs across to where the ball's path meets him, on his own while the stick stays where it was, and A is only the call for the return. Host test, sixty passes on the run with the stick held and A pressed: none reached the man before, all sixty after. Standing still, A passes the way he faces, or to the man most free if nobody is there.

**Goals celebrated at the crowd.** The scorer wheels away to the touchline in front of the stand on his side and celebrates there in his nation's way, team-mates running to join him, the camera with him.

**The commentator says the names.** Every one of the 316 shirt names is recorded: he names the man who has just got the ball (never the same one twice running, at most about once a second), the scorer after "Goal! Goal! Goal!", the keeper after a save, and the man booked after the card. Checked in MAME from the sound commands: the goal call, then 2.5 seconds later the scorer's name; after the kick-off, the man on the ball. 316 names add 2 MB to the V ROM (now 5.4 MiB, 405 ADPCM-A samples of the driver's 511).

**A play-test kit.** `live_centuryxi_mvs.bat` and `_aes.bat` in `C:\mame\neogeosdk\live` play the installed build and record the session: besides the live log (marks with M, snapshots, save states, the inputs for an exact replay) Century XI's logger writes the match as it happened (goals with the scorer, shots, saves, cards, the keepers), every pass of yours that missed its man and why, sprites lost to the 96-a-line limit, men who vanished or showed another's colours for a frame, slow seconds with what was on screen, and a summary. The same logger runs in the automated MAME tests. Run `tools/live_debug.py` again after every install: the addresses it reads change with each build.

**Faster rebuilds.** The SDK's sample encoders now skip a sample already encoded from the same file by the same encoder: the 405 samples were converted twice over on every build.

## New in dev.9

**The aim view at free kicks and corners.** D swaps to a view between the ball and a target arrow the stick moves anywhere on the pitch; A plays it along the ground to the arrow, C lofts it there, B strikes it at the arrow with a bend toward goal (the wall still jumps). Checked in MAME on a staged free kick.

**A commentator.** Spoken calls on the play, on top of the captions with names: "Shot at goal!", "Up for the header!", "What a strike!", "Through ball!", "He's through!", "One two!", "A long ball forward!", "Great tackle!", "Great passing!" after six passes in a row, "Danger here!" when a side carries it into the other's box. Checked by speech recognition (short shouted calls are hard for it: "Shot at goal" comes back as "Shot it gold").

**Team styles from the real ones.** Brazil's 4-4-2 of 1994, Italy's 4-4-2 on the counter, Germany's 3-5-2 with a sweeper, Romania countering round a playmaker, France on the break as today.

## New in dev.8 (owner's notes on dev.7)

**Names every country can smile at.** Nations play on something they are proud of, never a jibe: SAMBALIA (Brazil, a toucan), MARIACHIA (Mexico, a guitar), CARPATHIA (Romania, mountains), PURAVIDIA (Costa Rica, its motto, the tree frog), MATEGUAY (Uruguay, a mate cup), TULIPLANDS (Netherlands, a tulip), BURGERLAND (USA). BANANIA went: besides the old joke it is a real French brand. The vampire shirt names went with Vampirania, and the host's crowd chant no longer says the real name of Japan. The chants were re-recorded.

**A crowd you can see and hear.** A near stand along the bottom of the picture (boards, then the backs of the front rows, scarves and arms up) shows whenever play is in the near half. The far stand's crowd moves: pairs of colours only the crowd wears swap now and then (faster when the crowd is up). The crowd's noise follows the danger (a murmur in midfield, rising as a side nears goal with the ball, a roar at a shot), is louder late in a tight match and in extra time, boos a dive and whistles a card or a foul from behind.

**The goal is the scorer's.** The GOAL words come quicker and clear after two seconds; the camera follows the scorer, and his side's celebration plays in full before the replay.

**Shooting and passing reviewed.** B shoots at once (as the earliest design had it), hard, aimed with the stick; harder on the run. C is a long, high lob to the man furthest up in the stick's direction. Keepers no longer shuffle at an outfielder's sprint and dive as far as their rating: on the host, an unblocked shot scores 30% (was 23%), from close in 55%; a placed shot beats an ordinary keeper. Defenders block fewer shots. Men are about 30% quicker. The review screen is half as long.

**Commentary with names.** "ONIGIRI SHOOTS!", "SAVED BY ...", "... OVERHEAD KICK!", "THROUGH BALL BY ...", "ONE-TWO, ...!" on the line under the score.

**Tricks for the sides that play that way.** Flair sides and dribblers on the CPU use the step-over, drag-back and rainbow flick, not only the feint.

**Flicker.** A man arriving in an empty slot, or in a slot that held another man, is always written now (he vanished, or showed another man's shirt, for a frame), and men off the screen no longer shuffle the depth order.

## New in dev.7 (owner's notes on dev.6)

**Fixed: black blocks and flicker when the camera moves, worst near the goal.** The pitch and stands are two chains of 21 columns; when the camera crossed a column, two position words had to change together, and they went through the engine's queue, which stops when the vertical blank runs short. Half a move left a block of columns off the screen for a frame: black areas and flicker on every pan, and the black columns after a camera jump. The chain words are now written together at the very start of each blank. Measured in MAME on a staged run up and down beside the goal: 30-48% of the picture black before, none after.

**The camera looks up with a high ball**, so a ball in the air is no longer a shadow on the grass with nothing above it.

**Shots, passes, long balls.** Shots are much harder (a tap 171, a full wind-up about 234, was 148). The player's own passes stray very little. C is a long ball to the man the stick points at anywhere outside the box edge (it only chips from the edge of the box, central, and crosses from wide).

**Headers, volleys and the overhead scissor kick.** A or B pressed while a ball flies in hands control to the man under where it drops; he goes for it and meets it there: a header, a volley, a diving header if he has to stretch, or the overhead kick with his back to goal (and now and then when it drops on him). No frame-perfect press. Host test, 100 crosses with B pressed early: 77 headers, 23 overhead kicks, 21 goals.

**Tricks.** A and B together on the ball: with a man square in front, the feint as before; with nobody, the stick decides: back is the drag-back, up or down the step-over, otherwise the rainbow flick over his own head.

**The frame rate.** dev.6's catch-up kept the match at full speed but, once a frame was late, held the picture at 30 for good. Every eighth late frame now tries a single tick, so the picture returns to 60 when the crowd thins. Measured on the QA route: 58-60 frames a second in open play, 30-32 in crowds, the match at 52-60 ticks a second throughout (dev.5: 32-42 in crowds).

**OPTIONS: ERASE SAVED DATA** (A, then A again) clears the options, records and any Road to 2030 kept, on the arcade board and the memory card, and starts from the defaults.

## New in dev.6 (owner's notes on dev.5)

**A parody of the world's football.** The 32 sides are parody nations, recognisable by their kit colours, their flag's layout and a cute symbol where the real emblem was: SAMBALIA (a toucan in the diamond), BURGERLAND (a burger in the canton), SUSHIPON (a sushi roll for the sun), MAPLEDONIA, BAGUETTIA, PASTALIA, TEALAND, KIMCHIREA, PERSICATIA and so on (tools/nations.py PARODY, tools/flags.py). Kits are unchanged, with no maker's marks. Players have cute joke names by region (CAFEZINHO, ONIGIRI, BREZELMANN...). The grounds are parody grounds (BIG RAMEN BOWL in TOKYO-YO, LUCKY CAT STADIUM...), the ad boards made-up brands only (three that came close to real products were renamed), the quick cup is QUICK CUP, and the terrace chants were re-recorded with the parody names: no real country is named on screen or in the sound.

**Faster football.** A late frame no longer slows the match: when a crowded screen misses a vertical blank, the next frame runs the match twice (the picture drops to 30, the game keeps its pace). Men run about 20% faster and turn quicker; ground passes are hit firmer on a grippier pitch (a 300-unit pass arrives in 52 frames instead of 89); long balls are driven, not floated. Library divisions in the hot paths (a / 5 is a 600-cycle call on a 68000) were replaced by single instructions: the simulation's average fell from 118 to 111 lines a frame.

**Shooting.** B is now a power shot: a tap places it as before, held it winds up (a POWER gauge under the player's half) and leaves as a hard rising drive when B comes up; a full-blooded one is a little wilder. Keepers tip the ones they can't hold round the post; defenders block shots in their path; a keeper no longer re-catches his own throw.

**The CPU: eight levels, a cup that gets harder, a CPU that reads the score.** CPU LEVEL 1-8 (Sunday League to Impossible) in the options; through a cup the CPU climbs a level every two rounds and sharpens; it plays harder when behind and late, and eases off when it is well ahead. Each side's ratings now show: passing errors, first touches, shot accuracy, tackling, pressing pace and the keeper all come from the nation's ratings and the level (the SDK's new ng_tune curves). Measured against a level-4 side in host matches: level 1 scores 0.30 a game and concedes 0.58; level 8 scores 0.79 and concedes 0.30.

**Set pieces and keepers.** Corners: A short, B driven to the near post (with the stick into the pitch, swung to the spot), C floated to the far post, curling in or out; attackers take their spots, defenders guard the posts and mark goal-side, one forward stays up. Throw-ins: A to feet, B down the line, C long (into the box from near their goal); the nearest team-mate comes short, the next runs the line. Keepers come for crosses dropping into their area and punch when crowded, rush out at a man through on goal and dive at his feet, throw out to a free man or punt, and waste time when protecting a lead late. Curl now bends a ball's path in any direction (a corner from the side curls properly).

**Graphics fixes.** The GOAL! GOAL! GOAL! words had not appeared since dev.5: their first write cost more than the busy vertical blank had and waited for ever. They are written at kick-off and now zoom in at the goal (the SDK's sprite groups gained an exact-width shrink). The SDK queue now lets a starved job through after a wait instead of never. Faces: stubble, jaw shade, lips and blush were coloured as hair or shirt (masks and kit-coloured smudges on faces); the figure converter now keeps them skin. The grass is the saturated broadcast green of 1994 with bold mowing bands.

**A second player can join.** During a one-player match, player 2's START (with a credit on an arcade board) hands him the CPU's side ("P2 JOINS!"). This also fixes a fault where player 2's START during a match could take player 1's credit. Operators get real settings (CPU LEVEL, MATCH TIME, JOIN IN) instead of the template's meaningless LIVES table.

**SDK additions used here:** ng_tune (rating curves), ng_task (cooperative tasks that sleep N frames; tested in MAME), ng_fixscript (FIX-layer screen scripts), ng_devmenu (a developer test menu: slow motion, timer stop, free camera, RAM watch, sound test; in Century XI's QA builds, A+B+C held and D), ng_join (players joining mid-game), tools/softdip.py (operator settings tables from a short text file), exact-width shrink for sprite groups.

**Not done or not measured:** the frame rate with a crowd on screen is still 30-37 frames a second in MAME (the match itself keeps its speed now); a zoomed close-up of the scorer is not possible by hardware shrink (it can only make sprites smaller), so the goal's zoom is the captions; the terrace chants are crowd-processed and speech recognition cannot confirm the words (they contain no real names by construction); the human corner and throw-in choices have been checked in host simulation and one staged corner in MAME.

## New in dev.5 (owner's notes on dev.4)

**Fixed: things vanishing near the goal.** The men's sprite writer used most of the vertical blank, so the engine's own queue (the pitch's new columns, the goal, the big figures) missed its deadline: in a crowded goalmouth the picture stopped scrolling and the goal disappeared. The writer now writes only what changed, with words worked out during the frame, and leaves the queue its share. Measured in MAME: the writer averages 4-12 lines instead of 15-33, and never runs into the picture. The same fault hid the referee in two of his gags; all 17 now show.

**The goal and the ball redrawn.** Round white posts and crossbar with a lit edge, a cool shade and a crisp outline; a bright square mesh with depth; the near side net drawn over the men; the goal's shade on the grass; a billowing back net. A 1994 match ball, rendered as a shaded sphere with pentagons, round outline; the ball flying away from the camera matches it.

**Keepers protected.** A man who barges a keeper holding the ball, or one down on it, is booked (red under modern rules if already booked). Nobody takes a ball from under a keeper, and in his six-yard box a loose ball is his if he is near and no further from it. CPU men no longer press a keeper with the ball. Measured: in dev.4, 107 of the CPU's 156 shots (and 10 of 17 goals, 8 matches) came within five seconds of winning the keeper's release; now 8. CPU attacks shoot and cross more to make up for it.

**Air drag fixed.** It rounded a leftward or upward ball down a whole step every frame, so shots at the left-hand goal died in the air. It is the same both ways now.

**Free kicks as on a real pitch.** A wall of two to five men 9.15 m away (130 units), on the near half of the goal, walked back by the referee; it jumps at the kick and blocks a ball low enough to meet it. The keeper takes the far half and is unsighted until the ball clears the wall, unless he read it. Everyone else stands 9.15 m off at free kicks and corners; attackers go into the box for free kicks in crossing range. B lofts it over the wall, B with up or down bends it round, C chips it into the box, A plays it short.

**The board on the FIX layer.** Flags, codes and the score in tall digits; a running clock, red and flashing through the last minute and the added time; the added time shown as +N; the mood bar in the two shirts' colours; a status line (injury, ten men, a coach's new orders, last minute, stoppage). It uses no sprites (the old plate took 11 on its lines). Added time now counts what held the game up: fouls, cards, substitutions, injuries, goals, offsides, reviews and throw-ins, as well as dead-ball time.

Measured on the host: all sim and tournament checks pass. Thirty-two CPU matches average 1.2 goals, 9.8 shots, 3.1 fouls, 1.4 cards and 2.9 free kicks. CPU free kicks from five spots: 0-63% scored by spot (angled ones near the box dangerous, long central ones not): uneven, to tune after playing.

Still open: the frame rate in a goalmouth crowd (lowest second 30 fps with all 22 men in the box, 42 fps on the normal route); the simulation's own work is the cost. Black stand columns for about half a second after a camera jump (seen only when a QA script teleports the men). The human's B free kick has been checked in the host simulation, not on screen.

## New in dev.4 (owner's review of dev.3)

Five-minute whole match by default. Throw-ins by hand. Fouls stop play, with the referee's own moments. Separate passes per button (short, long, chip, cross, through ball, one-two, back-heel). Yellow and red cards for charging the keeper. A side may ask for the video review. National celebrations and national chants on momentum. Shirt numbers. Eight grounds. Men written whole to video memory in the blank, using only the strips a pose fills: worst scanline 53 of 96 in short MAME runs (dev.3: 96).

Open in dev.4 (now fixed in dev.5): two referee gags showed no referee. The national celebrations have still not been seen running. The figures further down this page are dev.3's unless stated.

## What is new in this candidate

**1994 look.** A bright sunlit pitch with sharply mown bands in true perspective, a concrete stadium on a blue-sky afternoon, a small corner score box, parody advertising boards and baggy collared kits.

**The men.** Five rugged male archetypes: spiky hair, wild frizz and goatee, mullet and stubble, shaved head, dreadlocks and moustache. Nations mix them with their own skin and hair, and each side's star is a recognisable *type* of 1990s player, never a person: no real names or likenesses. Shadows are solid ovals with their own palette role. 69 frames each in three depth sizes.

**Football.**
- Dribbling is a run of touches: the ball is knocked ahead and swings round late on a turn. Men have momentum, tire with the minutes and lose top speed.
- Passes and shots carry error from a man's rating and from pressure. A hard pass can bounce off a clumsy touch.
- Every way to score: shots, long range, tap-ins, headers, diving headers, volleys, overhead kicks, chips over the keeper, free kicks, penalties, rebounds, deflections and own goals. Crosses come from wide, and men run to where the ball will land.
- Nutmegs, fumbling keepers, handball, diving, injuries and arguments with the referee.
- A clock that runs through stoppages, as in the real game, so a match lasts what the option says: 3, 5, 7 (default), 10 or 14 real minutes. Level cup ties: extra time, golden goal or straight to penalties.

**Tactics.** Pause for shape (4-4-2, 4-3-3, 3-5-2, 5-3-2), mentality and pressing, and for substitutions with a fitness read-out. CPU sides change men and mentality too.

**The parody.** GOAL! GOAL! GOAL! with the announcer, a billowing net and a scorer's own celebration while team-mates pile on and the losers hang their heads. A coach in a television window (shouting, despairing, kicking his water bottle), a panicking linesman with his flag, a moustached referee with three personalities and a giant card, and a stream of talk from everyone.

**3D penalties.** A low view behind the spot: the taker from the back, the keeper facing us, the ball shrinking as it flies. Match penalties and shoot-outs both use it.

**Sound.** A stadium announcer and officials by speech synthesis, chanting terraces, a crowd that swells near goal, a goal fanfare, and full band tunes for the title, menus, penalties, final and champions. As on 1994 television, a match itself has crowd, not music.

## Verification (measured)

| Check | Result |
|---|---|
| Host tests `python3 tests/run.py` (address and undefined-behaviour sanitisers) | Pass: laws, feints, offside, review, cards, advantage, set pieces, determinism, substitutions, match length, golden goal, goal kinds, dribbling |
| 20,000 shoot-outs | All end, winner always scored more, turns stay level |
| 1,000 tournaments | 64 fixtures, 32 distinct entrants, byte-identical replay |
| CPU averages for a 7-minute match (`tests/stats.c`, 32 matches) | About 4.4 goals, 34 shots, 11 saves, 11 fouls, 2.6 cards |
| MVS and AES builds | Pass; both boot from the installed folders. Only P1 differs |
| ROM budget | P 39%, C 31%, S 9%, M 4%, V 32% |
| Sprites per scanline, 280-second MAME run | Worst 96 of 96: at the limit, none over |
| Frame rate, same run | Lowest second 55; 59 of 255 seconds below 59 |
| Frame work, MAME raster marks | About 190 of 264 lines on average |
| Voices | Lines checked by speech recognition; music rendered and measured, not heard |
| Screens seen running in MAME | Goal sequence, 3D penalty, red card (forced by a QA hook), options, pause, tactics, substitutions |
| Save and resume, MAME MVS | Road survives quitting and rebooting |
| Coins and Start, `tests/test_boot_flow.py` in MAME | Pass on MVS under the US, Europe, Japan, Asia, UniBIOS 4.0 and EagleBIOS system ROMs and on AES under the Asia, Japan and EagleBIOS ones: one credit per game, free Starts in the menus, a second game after a session, player 2 paying one credit for Versus, a paid join mid-match |

All evidence is from the host or an emulator. **Nothing has been tested on real hardware.**

## Known defects and limits

- On an arcade board the system ROM writes the save back when the game returns to it: a power cut mid-Road loses progress since then.
- AES memory-card saving has still not been exercised.
- Two-player: paying, Versus starting and joining mid-match are checked by the boot-flow test on this candidate; a whole two-player match was last played through on dev.2.
- The sprite limit is met with nothing to spare at the busiest moments. New effects at a goal need room made first.
- Reds: about one match in seven since dev.6 (CPU defenders slide only from goal-side and never on a yellow).
- Direct free kicks and chips are rarely scored by the CPU, so a player will see them mostly from themselves.
- The crowd is seen only when play nears a touchline (the stands lie above and below a pitch deeper than the screen) and at a goal, when the scorer runs to it. Flags and nets do not move. The stadium panorama repeats, mirrored.
- When the screen is crowded near a goal the picture can still drop below 60 frames a second (the match itself keeps its speed).
- Music and voices were checked by measurement and speech recognition, not by ear.
- The feint's input is an original design: no reliable source for the reference game's own input was found.

## Not built yet

A Legends dream match; striped and patterned kits; star portraits; a sixth figure drawn with East Asian features (painted art, to be approved first); a larger near size for the men (the near frames fill their 64 x 80 sprite: a larger frame and slot layout); qualification on real hardware (MVS board, AES console and memory card). The `ng_soccer` SDK module holds the ball, lines, offside and championship; players, tactics, the CPU and the referee stay in the game.
