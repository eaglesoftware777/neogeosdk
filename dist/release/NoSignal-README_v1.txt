NO SIGNAL Z ALPHA  --  v1 ROMs  --  Eagle Software, 2026
========================================================

!! WORK IN PROGRESS !!  A preview build, not a finished game.

No Signal Z Alpha's first ROM set, shared with the NeoGeoSDK v1.7.7
pre-release. Every file carries "_v1" in its name.

A beat-'em-up for one player. Someone stole the phone, then the whole city
lost its signal: choose Jay, Zoe or Kai and get it back across eight
stages, each with its own crew and a boss in three phases. Story Arcade,
then Boss Rush and Stage Select, a two-minute Challenge and the Gallery.
(Every change: NoSignal-CHANGELOG_v1.txt)

FILES
-----
NoSignal-WIP-NeoSD_MVS_v1.neo      NeoSD image, arcade (MVS) build
NoSignal-WIP-NeoSD_AES_v1.neo      NeoSD image, console (AES) build
                                   Cartridge data only: the NeoSD uses the
                                   system's own BIOS.
NoSignal-Darksoft-MVS_v1.zip       Darksoft Neo Geo Multi folder, MVS build
NoSignal-Darksoft-AES_v1.zip       Darksoft Neo Geo Multi folder, AES build
                                   Copy the folder inside to the SD card. No
                                   BIOS included: the Multi uses the board's.
NoSignal-BackBit-MVS_v1.zip        BackBit Platinum folder, MVS build
NoSignal-BackBit-AES_v1.zip        BackBit Platinum folder, AES build
                                   Copy the nosignal folder inside to the
                                   microSD card. No BIOS included. The
                                   BackBit is an MVS cartridge: use the MVS
                                   folder on an arcade board.
NoSignal-WIP-EagleBIOS_v1.zip      MAME: the MVS build with EagleBIOS
                                     mame neogeo -rompath roms -hashpath hash -bios euro -cart1 nosignal
NoSignal-WIP-AES-EagleBIOS_v1.zip  MAME: the AES build with EagleBIOS and a blank memory card
                                     mame aes -rompath roms -hashpath hash -bios asia -cart1 nosignal -memc card.bin
NoSignal-CHANGELOG_v1.txt          what is in this set
NoSignal-MANIFEST_v1.txt           size, CRC32 and SHA-256 of every v1 file

EagleBIOS is the free, open system ROM written for NeoGeoSDK. It is the
only system ROM in these packages: no SNK BIOS and no Universe BIOS is
included. MAME warns that its checksums differ from the original BIOS:
that is expected.

CONTROLS
--------
Stick       walk along and across the street; double tap runs
A           attack (a chain of four); down + A sweeps
B           jump (A in the air kicks)
C           grab, throw, pick up, ride the scooter
D           the hero's special (costs a little battery)
START       pause (console)
Arcade settings (MVS): LIVES, DIFFICULTY, HOW TO PLAY, BARKS.

STATUS
------
Checked in MAME (MVS and AES, EagleBIOS): boot, title, hero select and
Stage 1 fights; test builds clear all eight stages to the ending. Still
open: flicker when many fighters stand close together, the repainting of
stages 2 to 8, and the feel of the fights.

Real hardware is not verified yet: reports from NeoSD, Darksoft, BackBit,
MVS and AES owners are very welcome (board or console, BIOS, cart firmware,
what you saw and heard).
