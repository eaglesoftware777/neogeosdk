# Shadow League Fighting — changelog

Shadow League Fighting's ROM sets ship with the NeoGeoSDK pre-releases and
are numbered on their own: `_v1` and `_v2` with v1.7.5, `_v3` with v1.7.7.

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
