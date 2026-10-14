# Shadow League Fighting — changelog

Shadow League Fighting's ROM sets ship with the NeoGeoSDK pre-releases and
are numbered on their own: `_v1` with v1.7.5.

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
