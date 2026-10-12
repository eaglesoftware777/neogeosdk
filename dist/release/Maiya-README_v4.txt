MAIYA: SUPER NATURE GIRL  --  v4 ROMs  --  Eagle Software, 2026
================================================================

!! WORK IN PROGRESS !!  A preview build, not a finished game.

These are the fourth ROM set, shared with the NeoGeoSDK v1.7.3 pre-release.
The v1, v2 and v3 files (on the v1.7.1 and v1.7.2 pre-releases) stay
exactly as they were published; every v4 file carries "_v4" in its name.

WHAT'S NEW SINCE v3   (every change: Maiya-CHANGELOG_v4.txt)
-------------------
- Her walk and run, redrawn. Arms with elbows, brown bracers and small
  closed fists instead of stiff sticks ending in pale discs. Walking, her
  arms swing loosely; running, they pump with the elbow bent, the fist
  from her chin back past her hip.
- Her legs come from her original drawings, both in the same colours:
  no darker "shadow" leg flashing every step. One leg lifts while the
  other carries her.
- A real run: she lands, sinks, drives her knee through and flies with
  both feet off the ground.
- No more skating: each pose is held for the ground her planted foot
  covers, so her feet stay put on the road.
- She stops with her feet together and is straight back in her idle pose.
- No more blinking out for a frame when a creature ahead of her came or
  went.
- Throwing on the move: hold B and she plants her feet for each throw,
  then walks a few steps before the next.
- Emerald Forest, Azure Coast and Autumn Grove re-laid; Luna's hair has
  no more light specks.

FILES
-----
Maiya-WIP-NeoSD_MVS_v4.neo     NeoSD image, arcade (MVS) build
Maiya-WIP-NeoSD_AES_v4.neo     NeoSD image, console (AES) build
                               Cartridge data only: the NeoSD uses the
                               system's own BIOS.
Maiya-Darksoft-MVS_v4.zip      Darksoft Neo Geo Multi folder, MVS build
Maiya-Darksoft-AES_v4.zip      Darksoft Neo Geo Multi folder, AES build
                               Copy the folder inside to the SD card. No
                               BIOS included: the Multi uses the board's.
Maiya-BackBit-MVS_v4.zip       BackBit Platinum folder, MVS build
Maiya-BackBit-AES_v4.zip       BackBit Platinum folder, AES build
                               Copy the maiya folder inside to the microSD
                               card. No BIOS included: the BackBit uses
                               the board's.
Maiya-WIP-EagleBIOS_v4.zip     MAME: the MVS build with EagleBIOS
                                 mame neogeo -rompath roms -hashpath hash -bios euro -cart1 maiya
Maiya-WIP-AES-EagleBIOS_v4.zip MAME: the AES build with EagleBIOS and a blank memory card
                                 mame aes -rompath roms -hashpath hash -bios asia -cart1 maiya -memc card.bin
Maiya-CHANGELOG_v4.txt         every change, v1 to v4
Maiya-MANIFEST_v4.txt          size, CRC32 and SHA-256 of every v4 file
Maiya_Setup_Wizard.zip         the setup wizard: installs the MAME sets and
                               launchers, and downloads the NeoSD,
                               Darksoft and BackBit files for an SD card

EagleBIOS is the free, open system ROM written for NeoGeoSDK. It is the
only system ROM in these packages: no SNK BIOS and no Universe BIOS is
included. MAME warns that its checksums differ from the original BIOS:
that is expected. EagleBIOS keeps console saves on a memory card in the
same layout as the original console BIOS, so one card works with all of
them. The AES MAME zip comes with a blank card, card.bin, in its command.

To play under the Universe BIOS or an original BIOS in MAME, put your own
copy in MAME's roms/neogeo (or roms/aes) and pick it with -bios, for
example -bios unibios40.

CONTROLS
--------
Left / Right  tap to walk; hold to build into a run; release to brake
A  jump (Up + A in the air: a second jump while a sky lily lasts;
   kneel, then Up + A: the high leap)
B  thorn / whip up close; hold to keep throwing (on the move she plants
   her feet for each throw)
C  dash
D  Secret Art (uses a rose charge)
Up beside a poison drum: shut it (the works)
Forward, Down, Down-Forward + B : Rising Bloom
Down, Forward + B               : Rose Blossom Surge

STATUS
------
Checked in MAME (MVS and AES, EagleBIOS): her walk and run frame by frame
(every new frame shown, no sprite jump, the stop then idle, throwing on
the move), and the game's own movement, climbing, guardian, bonus,
continue, pick-up, pit and flight checks. Real hardware is not verified
yet: reports from NeoSD, Darksoft, BackBit, MVS and AES owners are very
welcome (board or console, BIOS, cart firmware, what you saw and heard).
