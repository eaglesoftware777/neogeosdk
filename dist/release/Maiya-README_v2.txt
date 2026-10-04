MAIYA: SUPER NATURE GIRL  --  v2 ROMs  --  Eagle Software, 2026
================================================================

!! WORK IN PROGRESS !!  A preview build, not a finished game.

These are the second ROM set shared with the NeoGeoSDK v1.7.1 pre-release.
The v1 files on this release stay exactly as they were published; every
v2 file carries "_v2" in its name.

WHAT'S NEW SINCE v1   (every change: Maiya-CHANGELOG_v2.txt)
-------------------
- Smoother game: the engine writes sprites and text in the vertical blank
  and queues sound commands, so frames no longer wait on the hardware.
  Every stage was measured at about 59 frames a second with 0-4% of
  frames running long.
- Clean scene changes: every screen change fades through black (white for
  the chooser), the new scene built while nothing shows -- no torn or
  half-drawn frames, no leftover sprites, no scrambled tiles.
- A livelier run: two strides per cycle, legs passing under her, arms
  moving with them; she brakes when she turns, and her jump has four poses.
- Backgrounds without a seam, and longer: five valleys show their whole
  painted band, the others the painting and its mirror; small creatures and
  lights of each valley drift far off (butterflies, leaves, snow, bubbles,
  fireflies, embers).
- The checkered shadows under every character are gone.
- The guardian's health bar keeps up with its health; the flying guardians
  fall sooner.
- The chooser gives 15 seconds to choose; "how to play" and the story each
  move on after 10 seconds, or at a button -- and on "how to play", A and B
  together shows both girls' moves.
- Every stage stays within the hardware's 96 sprites per line (78 at most).
- Heat haze over the Golden Savanna (a raster effect on the line timer).
- Maiya squashes and stretches as she lands and leaps, and her run settles
  on each footfall.
- Console saves: on an AES with a memory card, scores, names and progress
  are kept on the card ("MAIYA SCORES", two card blocks), under EagleBIOS,
  the Universe BIOS and the original console BIOS alike.
- Arcade sound fix (from the v1.7.1 MVS sound pre-release) is in both
  builds: coin and start sounds after slot switching on MVS boards.
- Clean, simple boot: the Universe BIOS splash shows the Maiya logo on
  both AES and MVS (the arcade build used to show stray tiles there), and
  a console boots with one logo and no jingle under every BIOS.
- Same cartridge layout as v1: P 512 KiB, S 128 KiB, M 128 KiB,
  V 8 MiB, C 8 MiB; the .neo files are the same size as v1's.

FILES
-----
Maiya-WIP-NeoSD_MVS_v2.neo     NeoSD image, arcade (MVS) build
Maiya-WIP-NeoSD_AES_v2.neo     NeoSD image, console (AES) build
                               Cartridge data only: the NeoSD uses the
                               system's own BIOS.
Maiya-Darksoft-MVS_v2.zip      Darksoft Neo Geo Multi folder, MVS build
Maiya-Darksoft-AES_v2.zip      Darksoft Neo Geo Multi folder, AES build
                               Copy the folder inside to the SD card. No
                               BIOS included: the Multi uses the board's.
Maiya-WIP-EagleBIOS_v2.zip     MAME: the MVS build with EagleBIOS
                                 mame neogeo -rompath roms -hashpath hash -bios euro -cart1 maiya
Maiya-WIP-AES-EagleBIOS_v2.zip MAME: the AES build with EagleBIOS and a blank memory card
                                 mame aes -rompath roms -hashpath hash -bios asia -cart1 maiya -memc card.bin
Maiya-CHANGELOG_v2.txt         every change from v1 to v2
Maiya-MANIFEST_v2.txt          size, CRC32 and SHA-256 of every v2 file

EagleBIOS is the free, open system ROM written for NeoGeoSDK. It is the
only system ROM in these packages: no SNK BIOS and no Universe BIOS is
included. MAME warns that its checksums differ from the original BIOS:
that is expected. EagleBIOS keeps console saves on a memory card in the
same layout as the original console BIOS, so one card works with all of
them. The AES MAME zip comes with a blank card, card.bin, in its command.

To play under the Universe BIOS or an original BIOS in MAME, put your own
copy in MAME's roms/neogeo (or roms/aes) and pick it with -bios, for
example -bios unibios40.

UNIVERSE BIOS IN-GAME MENU
--------------------------
During play:   MVS  Start + Coin, or Start + A + B + C
               AES  Start + Select, or Start + A + B + C
In MAME's default keys: Start = 1, Coin / Select = 5, A = Left Ctrl,
B = Left Alt, C = Space. Some keyboards cannot register 1 + Ctrl + Alt +
Space together; Start + Coin / Select (1 + 5) avoids that.

CONTROLS
--------
A  jump (Up + A in the air: a second jump while a sky lily lasts;
   kneel, then Up + A: the high leap)
B  thorn / whip up close (hold: run)
C  dash
D  Secret Art (uses a rose charge)
Forward, Down, Down-Forward + B : Rising Bloom
Down, Forward + B               : Rose Blossom Surge

STATUS
------
Checked in MAME (MVS and AES; EagleBIOS, the Universe BIOS 4.0 and the
original system ROMs): boot, eye-catcher, title, play, sound, the
Universe BIOS menu, and memory card saves. The Darksoft folders are
checked to turn back into the exact MAME ROMs. Real hardware is not
verified yet: reports from NeoSD, Darksoft, MVS and AES owners are very
welcome (board or console, BIOS, cart firmware, what you saw and heard).
