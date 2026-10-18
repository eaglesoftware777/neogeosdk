SEVEN WONDERS PUZZLES  --  v1 ROMs  --  Eagle Software, 2026
============================================================

!! WORK IN PROGRESS !!  A preview build, not a finished game.

Seven Wonders Puzzles' first ROM set, shared with the NeoGeoSDK v1.7.7
pre-release. Every file carries "_v1" in its name.

A puzzle game: seven wonders, four levels each, every one with its cat,
its mood and its goals. Match pieces on a board (colours, ice, special
pieces, crates) or launch bubbles to rescue the kittens floating in them.
28 levels, stars and best scores kept. (Every change:
SevenWonders-CHANGELOG_v1.txt)

FILES
-----
SevenWonders-WIP-NeoSD_MVS_v1.neo      NeoSD image, arcade (MVS) build
SevenWonders-WIP-NeoSD_AES_v1.neo      NeoSD image, console (AES) build
                                       Cartridge data only: the NeoSD uses the
                                       system's own BIOS.
SevenWonders-Darksoft-MVS_v1.zip       Darksoft Neo Geo Multi folder, MVS build
SevenWonders-Darksoft-AES_v1.zip       Darksoft Neo Geo Multi folder, AES build
                                       Copy the folder inside to the SD card. No
                                       BIOS included: the Multi uses the board's.
SevenWonders-BackBit-MVS_v1.zip        BackBit Platinum folder, MVS build
SevenWonders-BackBit-AES_v1.zip        BackBit Platinum folder, AES build
                                       Copy the sevenwonders folder inside to the
                                       microSD card. No BIOS included. The
                                       BackBit is an MVS cartridge: use the MVS
                                       folder on an arcade board.
SevenWonders-WIP-EagleBIOS_v1.zip      MAME: the MVS build with EagleBIOS
                                         mame neogeo -rompath roms -hashpath hash -bios euro -cart1 sevenwonders
SevenWonders-WIP-AES-EagleBIOS_v1.zip  MAME: the AES build with EagleBIOS and a blank memory card
                                         mame aes -rompath roms -hashpath hash -bios asia -cart1 sevenwonders -memc card.bin
SevenWonders-CHANGELOG_v1.txt          what is in this set
SevenWonders-MANIFEST_v1.txt           size, CRC32 and SHA-256 of every v1 file

EagleBIOS is the free, open system ROM written for NeoGeoSDK. It is the
only system ROM in these packages: no SNK BIOS and no Universe BIOS is
included. MAME warns that its checksums differ from the original BIOS:
that is expected.

CONTROLS
--------
Stick       move the paw (match) or aim (bubbles)
A           pick up a piece, then push it into its neighbour; or fire
B           put the piece down; or swap the loaded and next bubble
C           paw hammer, once a level
D           hint (match) or aim guide (bubbles)
START       pause (console)
Arcade settings (MVS): TRIES, HOW TO PLAY, DIFFICULTY, AIM GUIDE.

STATUS
------
Checked in MAME (MVS and AES, EagleBIOS): boot, title, play; a test
build's bot plays all 28 levels to the ending.

Real hardware is not verified yet: reports from NeoSD, Darksoft, BackBit,
MVS and AES owners are very welcome (board or console, BIOS, cart firmware,
what you saw and heard).
