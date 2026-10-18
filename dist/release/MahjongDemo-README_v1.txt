MAHJONG DEMO  --  v1 ROMs  --  Eagle Software, 2026
===================================================

!! WORK IN PROGRESS !!  A preview build, not a finished game.

Mahjong Demo's first ROM set, shared with the NeoGeoSDK v1.7.7 pre-release.
Every file carries "_v1" in its name.

A small demonstration of the SDK's Mahjong engine: four-seat Hong Kong Old
Style Mahjong against three CPU seats, solitaire on the 144-tile turtle
layout (every deal can be cleared), and the rules on screen.
(Every change: MahjongDemo-CHANGELOG_v1.txt)

FILES
-----
MahjongDemo-WIP-NeoSD_MVS_v1.neo      NeoSD image, arcade (MVS) build
MahjongDemo-WIP-NeoSD_AES_v1.neo      NeoSD image, console (AES) build
                                      Cartridge data only: the NeoSD uses the
                                      system's own BIOS.
MahjongDemo-Darksoft-MVS_v1.zip       Darksoft Neo Geo Multi folder, MVS build
MahjongDemo-Darksoft-AES_v1.zip       Darksoft Neo Geo Multi folder, AES build
                                      Copy the folder inside to the SD card. No
                                      BIOS included: the Multi uses the board's.
MahjongDemo-BackBit-MVS_v1.zip        BackBit Platinum folder, MVS build
MahjongDemo-BackBit-AES_v1.zip        BackBit Platinum folder, AES build
                                      Copy the mahjong_demo folder inside to the
                                      microSD card. No BIOS included. The
                                      BackBit is an MVS cartridge: use the MVS
                                      folder on an arcade board.
MahjongDemo-WIP-EagleBIOS_v1.zip      MAME: the MVS build with EagleBIOS
                                        mame neogeo -rompath roms -hashpath hash -bios euro -cart1 mahjong_demo
MahjongDemo-WIP-AES-EagleBIOS_v1.zip  MAME: the AES build with EagleBIOS and a blank memory card
                                        mame aes -rompath roms -hashpath hash -bios asia -cart1 mahjong_demo -memc card.bin
MahjongDemo-CHANGELOG_v1.txt          what is in this set
MahjongDemo-MANIFEST_v1.txt           size, CRC32 and SHA-256 of every v1 file

EagleBIOS is the free, open system ROM written for NeoGeoSDK. It is the
only system ROM in these packages: no SNK BIOS and no Universe BIOS is
included. MAME warns that its checksums differ from the original BIOS:
that is expected.

CONTROLS
--------
Table       LEFT / RIGHT choose a tile, A discards, C declares a win
            or a kong, D asks for a hint; in a claim window A takes
            and B passes; hold B to speed the CPU turns up
Solitaire   stick moves between free tiles, A picks, B undoes,
            C shows a pair, D shuffles, START pauses
Arcade settings (MVS): CPU LEVEL, FLOWERS, ROUNDS, HOW TO PLAY.

STATUS
------
Checked in MAME (MVS and AES, EagleBIOS): attract with four CPU seats,
the player's turns, hints and discards, CPU claims and scored wins,
solitaire picks, hints and shuffles.

Real hardware is not verified yet: reports from NeoSD, Darksoft, BackBit,
MVS and AES owners are very welcome (board or console, BIOS, cart firmware,
what you saw and heard).
