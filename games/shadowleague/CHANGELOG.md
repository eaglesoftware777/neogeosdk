# Shadow League Fighting — changelog

Shadow League Fighting's ROM sets ship with the NeoGeoSDK pre-releases and
are numbered on their own: `_v1` and `_v2` with v1.7.5, `_v3` with v1.7.7.

## v3.2.0 candidate: the referee's ceremony and the league's own calls (2026-10-30, 2026-10-31)

- **The arbiter.** The referee is his own character now, painted with the
  fighters' pipeline: a stern bald man with a grey beard in a long black
  coat with silver trim and a crimson sash, a red flag in one hand and a
  blue one in the other. He takes shape out of the dark at mid-screen, bows,
  holds his arms out between the fighters while the round is called, raises
  the red flag on 3, the blue on 2, both on 1, chops down on the call and
  leaps away. The fighters start 104 pixels either side of mid-stage, well
  apart, while he does.
- **Title.** A new painted title: Claire, Kenji and Amara in the courtyard
  of a mountain shrine at dusk under a red gate, with the SHADOW LEAGUE /
  FIGHTING logo dropping onto it, flashing as it lands and catching a glint
  now and then. It drops on the coin-in screen too.
- **Colour.** Fighters, the referee and the select faces are graded for
  livelier colour and firmer light and shade (more saturation where a colour
  is greyest, gentler on skin, an S-curve on the mid-tones); the arenas get
  a lighter version of the same so the fighters stand out from them.
- **Effects.** Hit sparks, guard flashes, dust and the super flash are new
  painted 64x64 bursts, a third larger than before. Dust kicks up from the
  pushing foot when a dash or back-dash starts and under every landing, and
  the arena shakes on heavy blows, knockdowns, guard breaks and the KO.
- **Menus.** CHOOSE YOUR BATTLE lists both formats, the chosen one in gold
  between markers, with its description and the seconds left. On the select
  screen the cursor frame no longer touches the heading, and each player's
  fighting style sits in gold over the name with a clear row between them;
  the countries moved to the VS screen, under the names.

- **Throws.** A forward throw carries the opponent over the thrower's
  shoulder and drops them behind, so the fighters change sides and the
  camera follows; a back throw still tosses them in front. With the bodies
  pressed together a heavy button throws at once (holding back still picks
  the back throw), and the throw buttons are read over 8 frames instead of 4.
- **The shadow referee.** Every round opens with the arena's lights down.
  A dark figure rises out of the dark between the fighters, bows to each
  of them on the count, 3, 2, 1, and leaps out of the light in a puff of
  dust as the lights come up on the fight call. The figure's colours were
  read with the wrong channel layout and came out speckled; they are a
  clean dark blue now.
- **The league's calls.** The fight starts on CLASH!, a knockout is
  BLACKOUT, a round won without a scratch is UNTOUCHED, each with a new
  announcer line and new lettering. When a player wins the match the
  winner's name comes up and THE NIGHT IS YOURS sweeps across, flashes and
  settles to gold.
- **Win scene.** After every match the winner keeps their victory poses
  over the beaten loser, who lies darkened and bruised in the arena with
  its lights down, and says their piece: a two-line quote for each of the
  44 fighters (assets/authored/win_quotes.json). Any button after a second
  moves on; versus mode uses it in place of the old WINS card.
- **Continue.** The player's fighter stands beaten and swaying, darkened,
  in the arena they lost in, while the clock runs. Continue and they shake
  it off (RISE AGAIN, lights up, back in their stance); let it run out and
  they collapse before GAME OVER. The countdown digits drew the lower half
  of the next digit and now read correctly.
- **Winning hop.** The winner cycles their victory poses and hops instead of
  holding one pose.

## v3.1.0 candidate: faces and clean edges (2026-10-29)

Versions are now `MAJOR.MINOR.PATCH` (docs/releases/VERSIONING_POLICY.md);
this graphics pass is v3.1.0.

- **Select wall.** Each 24-pixel face is now the head alone (a hand-set box
  for the 25 crops that were busts, a mask or a robot), area-averaged and
  sharpened instead of a soft Lanczos shrink of the whole portrait, and the
  wall's top and bottom halves get sixteen palettes each (32-63) instead of
  sixteen for all 44 faces, so one face's skin and hair no longer bleed
  into its neighbour's. The magenta key fringe is painted out of the faces
  and of the 64x64 corner portraits.
- **Pink edges.** Nine painted fighters (Aisha, Hayato, Irina, Lucas, Maia,
  Mason, Ryuhei, Tomas, Xiaomei) kept a pink colour from the magenta key
  along their silhouettes and in their hair; on Xiaomei it filled her hands.
  Every such pixel now takes the colour beside it (tools/clean_fringe.py).
- **Faces in the fight.** The painted import averaged each block of the
  sheets, and a face ten pixels wide lost its eyes, brows and mouth. They
  are found again on the source sheets and painted into the approved frames
  where the frame shows skin, head only, with the colours each tile already
  has (tools/painted_import/face_detail.py). Silhouettes, hit boxes and
  animation are unchanged.

## v3 update (2026-10-26, NeoGeoSDK v1.7.7 pre-release, same file names)

- The fight HUD stays inside columns 3-36 of the 40 (x 24-295), the part of
  the picture every CRT shows: life bars of 14 cells between their caps,
  names, guard and spirit meters, round marks, power gauges, assist badges
  and combo counts. The HUD plates are the same art with their ends moved in.
- The select screen's lines and faces, the VS names, how to play and the
  training readouts moved in too; four story captions and the Chimera
  stage's name are shorter so they fit.

## v3 (2026-10-18, NeoGeoSDK v1.7.7 pre-release) — painted roster, training and CPU ranks

- All 44 fighters load complete painted pose sets (2,460 runtime frames, four
  tile palettes per fighter) with detailed selection faces. Jack fights as a
  boxer; Batu wears a closed jacket; Malik, Viktor and Ryuhei use wider,
  uncropped poses.
- Poses are uploaded into hidden sprite banks and shown only when complete,
  with bounded work per frame and stable sprite slots for fighters,
  projectiles and strikers.
- CPU levels 1 to 7 (3 by default) that react to what they see, choose
  projectiles and keep the same level across rounds and team members.
- Long, short and reverse throws; throw escapes; original projectiles and
  effects with their own palettes.
- MVS soft DIPs: level, rounds to win, 30/60/90-second rounds, demo sound.
  AES home menu: ARCADE, VERSUS, TRAINING and OPTIONS.
- Training: dummy standing, guarding, crouching, jumping, CPU or player two;
  recording and playback of up to 240 inputs; infinite power, life recovery,
  position reset and the fighter's command list.
- New title and skippable illustrated introduction.
- 59 localized voice lines for 15 fighters.
- Open: occasional slowdown or flicker reported in play, arena art and depth,
  further movement and title polish, and physical cartridge tests.

### Rebirth pass (2026-10-15)

- Original native title/logo, three larger portraits and six illustrated story
  cuts, with existing music and immediate START/credit handling.
- Sixteen indexed FIX announcements, including FIGHT, K.O., VICTORY, PERFECT,
  COUNTER and GUARD BREAK, verified against decoded S-ROM pixels.
- Completed palette buffers upload before BIOS input handling. Live emulator
  checks reject prolonged palette starvation, in addition to sprite overload.
- A 3-MiB P image with two aligned data banks and fixed interrupt/code addresses;
  16-MiB C capacity and complete hardware sprite tile addressing.
- Per-frame attack contacts, ordered rotational inputs and strict authored
  imports with optional per-tile fighter palettes.
- Activated native playable pose sets for Kenji, Jack and Amara: 203 referenced
  pose exports, 185 distinct drawings, six indexed layers each, explicit contacts
  and eight-drawing forward/backward footwork. They remain visual proofs awaiting
  artistic review. The remaining 41 fighter bodies and four arenas are preserved.
- Revised native title busts and portraits; metal HUD plates, shaped timer digits,
  recent-damage trails and separate Guard/Spirit gauges.
- Separate AES/MVS test packages, nine current emulator scenarios and audible
  cartridge recordings. Installed locally with verified ROM hashes and an old-build
  backup. Professional art acceptance, remaining roster production, pacing
  improvements and physical-cartridge tests remain.

## v2 (2026-10-13) — roster, arenas and fighting core

- Playable Duel and Team Battle selection: three unique fighters and one assist.
- Reserve entries after KO, double KO and timeout clear the previous bout, reset
  the clock and refresh character art, palette and HUD.
- Each teammate retains separate health; defeated members stay eliminated.
- Assists use their own animation, collision and damage, with three calls per match.
- Super pauses freeze both fighters and keep buffered buttons responsive.
- Replay checks include input queues, animation timers, reserves and assist state.
- Previous V1 distributions archived outside the repository; current game files
  retain the V2 version.

- All 44 fighters: distinct anime faces, clothing, silhouettes, guards and footwork.
- Six alternating foot phases, quieter held guards, separate preparation/recovery,
  chambered kicks and boxing attacks that consistently use the hands.
- Shaped fists, wraps, boots, cuffs, seams and far-side shading; intact limb layers
  and sufficient canvas room for extended strikes, falls and overhead hands.
- Four detailed pixel arenas: mountain dojo, Bangkok riverside, Brooklyn rooftop
  and the Chimera laboratory, with clear fighting floors and grounded scenery.
- New FIX lettering, vitality and spirit gauges, win markers and arena dimming
  during super freezes. Strikers use their own character palettes.
- Lossless sparse sprite sharing and one copy of the fighter definitions in ROM.
- Artwork gates cover all 2,124 frames, including actual C-ROM pixel equality.
- V2 release filenames retained for MAME, NeoSD, Darksoft and BackBit.
- Installer stages the new sets, preserves controls, backs up the old game and
  directs each launcher to the verified private V2 ROMs and software list.

## Earlier V2 production slice

### The game

- Expanded 7-fighter roster:
  - Kenji Arata (Karate, Japan)
  - Jack Mercer (Boxing, United States; no kicks)
  - Min-Jun Park (Taekwondo, South Korea; kicking focus)
  - Batu Temur (Bökh wrestling, Mongolia; 360/720 command throws)
  - Amara Silva (Capoeira, Brazil; fluid acrobatics & evasive rolls)
  - Niran Vong (Muay Lao, Laos; devastating elbows & knee strikes)
  - ATLAS (Combat Robotics, The Veil Facility; Boss Prototype with projectiles & super armor)
- Super freeze mechanics: 30-frame screen freeze on super activation with backdrop darkening.
- Negative-edge input buffering for special move execution on button release.
- 7-fighter arcade ladder (6 opponents + mirror match closure).
- Arcade continue screen: 10-second countdown with MVS credit deduction (`SYS_CREDIT_DOWN`).
- Title screen P2 join support.
- 3v3 team battle and striker summon modes in engine.

### Presentation

- Two-layer parallax stage: 512x224 Arata Mountain Dojo interior with 512x160 dawn mountains scrolling at half speed through open windows.
- Anatomical character art renderer: articulated torso plates, uniform drapery & gear, expressive hands, unique silhouettes & hair, and leg contours.
- Hardware camera zoom: dynamic SCB2 scaling (192..255) based on fighter distance.
- Complete FIX HUD: health bars, 60s timer, names, 3-stock power gauges, round markers, combo counter, guard gauge, spirit bar, and striker status badge.

## v1 (2026-10-12, NeoGeoSDK v1.7.5 pre-release) — first playable

### The game

- Kenji Arata (traditional karate, Japan) and Jack Mercer (professional
  boxing, United States; no kicks).
- Each fighter: standing, crouching and jumping normals, a heavy command
  attack (C+D), two throws (forward or back + C, escaped with C or D), three
  special moves, a guard attack while blocking (spirit), forward and back
  rolls (A+B) and a super (one power stock).
- Kenji: Ressen Zuki (lunging reverse punch), Kokoro Uke (a counter stance
  answering a strike with a palm strike), Kakato Otoshi (an overhead axe
  kick), Ichigeki Hissatsu (super). Jack: Brooklyn Rush (dashing straight),
  Weave (slips under strikes into a body blow), Rising Bell (uppercut),
  Final Round (super).
- Guarding high and low, guard break, counter hits, combos with damage
  scaling, dashes and backdashes, knockdowns and wake-up.
- Arcade: pick a fighter, beat the other fighter and then your own shadow
  (mirror match); a second player's START during a match opens a versus
  match. Attract mode plays a CPU demo fight.
- MVS soft DIPs: DIFFICULTY (EASY to EXPERT) sets the CPU level; ROUNDS
  (1-3) sets the round wins a match needs. A console plays NORMAL, best of
  three.

### Presentation

- The Arata Mountain Dojo at dawn: open screens onto the peaks, lanterns,
  a hanging scroll and a plank floor.
- HUD: life bars, a 60-second clock, names, power gauges with stock count,
  round-win markers, combo count; ROUND / FIGHT / K.O. / TIME UP / WINS /
  PERFECT announcements.
- Hit, guard and dust effects; synthesized effects and three music tracks.

### Hardware

- Separate MVS and AES builds; EagleBIOS for emulator use. The NeoSD,
  Darksoft and BackBit packages carry no BIOS.
- Checked in MAME (MVS and AES, EagleBIOS); physical boards are still to
  be verified.
