# Shadow League Fighting — changelog

Shadow League Fighting's ROM sets ship with the NeoGeoSDK pre-releases and
are numbered on their own: `_v1` with v1.7.5.

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
