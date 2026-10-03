MAIYA: SUPER NATURE GIRL  --  v2 ROMs  --  Eagle Software, 2026
================================================================

!! WORK IN PROGRESS !!  A preview build, not a finished game.

These are the second ROM set shared with the NeoGeoSDK v1.7.1 pre-release.
The v1 files on this release stay exactly as they were published; every
v2 file carries "_v2" in its name.

WHAT'S NEW SINCE v1
-------------------
- Smoother game: the engine writes sprites and text in the vertical blank
  and queues sound commands, so frames no longer wait on the hardware.
  Every stage was measured at about 59 frames a second with 0-4% of
  frames running long.
- Every stage stays within the hardware's 96 sprites per line (78 at most).
- Heat haze over the Golden Savanna (a raster effect on the line timer).
- Maiya squashes and stretches as she lands and leaps, and her run settles
  on each footfall.
- Console saves: on an AES with a memory card, scores, names and progress
  are kept on the card ("MAIYA SCORES", two card blocks).
- Arcade sound fix (from the v1.7.1 MVS sound pre-release) is in both
  builds: coin and start sounds after slot switching on MVS boards.
- Clean boot screens: the Universe BIOS splash shows the Maiya logo on
  both AES and MVS (the arcade build used to show stray tiles there), and
  the eye-catcher draws Maiya's own logo and text.
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
Maiya-WIP-AES-EagleBIOS_v2.zip MAME: the AES build with EagleBIOS
                                 mame aes -rompath roms -hashpath hash -bios asia -cart1 maiya
Maiya-MANIFEST_v2.txt          size, CRC32 and SHA-256 of every v2 file

EagleBIOS is the free, open system ROM written for NeoGeoSDK. It is the
only system ROM in these packages: no SNK BIOS and no Universe BIOS is
included. MAME warns that its checksums differ from the original BIOS:
that is expected. EagleBIOS's memory card routine is not written yet, so
under EagleBIOS a console keeps its save for the session only; on a real
console BIOS or the Universe BIOS the card works.

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
