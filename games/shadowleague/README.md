# Shadow League Fighting

Original Neo Geo MVS/AES fighting game, cartridge ID 781. This isolated candidate
uses fighting component 1.7.7. The game package for hardware review is V1; it does
not publish or replace the SDK release being prepared separately.

Two match formats: 1 vs 1 and 3 vs 3 with one selected striker per side. All 44
fighters now load complete indexed artwork exports. This includes the previously
unavailable cast, Atlas, and Batu's covered wrestling costume. The normal build
consumes these exports and rejects missing poses. The four existing arenas remain;
a new arena art/depth pass and further visual polish are unfinished.

The new title and skippable illustrated introduction use encoded cartridge art.
The renderer uploads fighter poses into hidden sprite banks and shows a pose only
when complete. This prevents partial texture uploads; it does not establish that
every reported face flicker or slowdown has been resolved. Native screenshots,
telemetry and review sheets are in `assets/review/sdk177/`.

## Playing

| Input | Action |
|---|---|
| Stick | Walk, crouch, jump; back guards, down-back guards low |
| Forward, forward / back, back | Dash / backdash |
| A / B / C / D | Light punch / light kick / heavy punch / heavy kick |
| A+B | Evade; back+A+B evades away |
| C+D | Blowback; guard attack while blocking |
| B+C | MAX power mode, one stock |
| Forward+C / forward+D, close | Long / short throw |
| Back+C or D, close | Reverse throw |
| C or D when grabbed | Throw escape |
| A+C, team battle | Call the selected striker |

Boxers use punches on all four buttons. Motions are relative to the opponent.
Kenji: quarter-circle forward+A/C lunging punch, quarter-circle forward+B/D
energy wave, quarter-circle back+A/C counter, forward/down/down-forward+B/D
axe kick, double quarter-circle forward+A/C super. His wave is an original move.
Jack: quarter-circle forward+A/C rush, quarter-circle back+B/D weave,
forward/down/down-forward+A/C uppercut, double quarter-circle forward+A/C super.
Arun and Zero have original flame projectiles; Atlas has an energy pulse.

Host input tests verify all 347 selectable specials/supers. Kenji, Jack, Claire
and Ryuhei also have verified three-hit A > C > quarter-circle-forward+C combos
at close range. Press the next move after the previous strike connects.

## Difficulty and training

MVS software DIPs provide LEVEL 1–7, default LEVEL 3, rounds to win, 30/60/90-second
rounds and demo sound. Tournament progress does not secretly increase difficulty.
AES has ARCADE, VERSUS, TRAINING and OPTIONS in the home menu. Defaults are level 3,
two round wins and 60 seconds; options persist during the current game session.

In training, START opens the practice menu. Choose standing, guarding, crouching,
jumping, CPU level 3, player two, recording or playback. Recording uses player one's
controls for the dummy, stores up to 240 inputs and loops them in playback. Infinite
power, life recovery, position reset and the selected fighter's command list are
available. Practice is nonlethal and the clock stays at 99.

## Building and local installation

```sh
python3 games/shadowleague/tools/build_candidate.py --label sdk177 --output-root dist/candidates/shadowleague/sdk177
python3 -m unittest discover -s tests -p 'test_fight.py' -v
python3 -m unittest discover -s tests -p 'test_shadowleague*.py' -v
make GAME=shadowleague test-precheck budget
python3 games/shadowleague/tools/package_v1.py
python3 games/shadowleague/tools/install_milestone.py --milestone v1-hardware-test --version "V1 / SDK 1.7.7"
```

The installer replaces only Shadow League files in `C:\mame\neogeosdk`, verifies
checksums and makes no backups. Launch `START_SHADOW_LEAGUE_MILESTONE.bat` for AES
or `START_SHADOW_LEAGUE_MILESTONE_MVS.bat` for MVS. It preserves other games and the
shared BIOS. Do not use save states from previous builds.

V1 hardware-test files are under `dist/candidates/shadowleague/sdk177/hardware-v1/`:
NeoSD and Darksoft AES/MVS, MAME sets with matching software-list XML, and BackBit
MVS. Their manifest records SHA256 and exact decoding checks against all six ROMs.
Physical cartridge compatibility still needs tester confirmation. The dedicated
`START_SHADOW_LEAGUE_V1.bat` opens the newly installed AES candidate.

P is 3 MiB: one fixed region plus two aligned presentation data banks. C remains
16 MiB in equal 8-MiB lanes. Recorded voices/music fit the 8-MiB V budget. Indexed
fighter canvases are 160×160 or 192×192 with explicit per-frame roots; the game's
logical collision coordinates and floor stay consistent. Fighters use up to four
16-color tile banks. Hardware output is native resolution, not 1080p artwork.

`tools/art.py` validates exports and compares decoded C/S-ROM pixels with the
compiled source. `tools/painted_import/` imports completed seven-sheet pose sets;
it rejects missing drawings and opaque cropping. Flat sprite exports do not have
separate anatomy layers, so pixel-integrity gates cannot certify anatomy or appeal.
Artist/player review remains necessary.

## Audio sources

Original music/effects and the existing announcer, male/female grunts and robot
processing are preserved. Fifteen fighters receive 59 localized combat lines.
Other language performances remain unfinished. `tools/sync_voice_samples.py` maps
approved named WAVs to stable sample numbers before normal sound builds.

Existing speech: Piper `en_US-libritts_r-medium`, LibriTTS-R corpus (Koizumi et al.,
2023), CC BY 4.0. The localized pass uses Kokoro-82M (Apache 2.0) with Koniwa
(CC BY 3.0) and SIWIS (CC BY 4.0) attribution and full sources in
`assets/authored/voices/voice-sources-sdk177.md`.
