MAIYA & LUNA: WONDERFUL ADVENTURES IN NATURE FANTASY  --  v1 ROMs  --  Eagle Software, 2026
===========================================================================================

!! WORK IN PROGRESS !!  A preview build, not a finished game.

Maiya & Luna's first ROM set, shared with the NeoGeoSDK v1.7.7 pre-release.
Every file carries "_v1" in its name.

The sequel to Maiya: Super Nature Girl, an exploration and puzzle
adventure. After the storm the valley's seven wonders have gone quiet.
Choose who sets out: Maiya, who climbs, pushes and shoos, or Luna, who
slips through gaps, works fine gears and looks closely. Each has her own
journey through seven chapters; they meet at the World Tree in Chapter VI.
(Every change: MaiyaLuna-CHANGELOG_v1.txt)

FILES
-----
MaiyaLuna-WIP-NeoSD_MVS_v1.neo      NeoSD image, arcade (MVS) build
MaiyaLuna-WIP-NeoSD_AES_v1.neo      NeoSD image, console (AES) build
                                    Cartridge data only: the NeoSD uses the
                                    system's own BIOS.
MaiyaLuna-Darksoft-MVS_v1.zip       Darksoft Neo Geo Multi folder, MVS build
MaiyaLuna-Darksoft-AES_v1.zip       Darksoft Neo Geo Multi folder, AES build
                                    Copy the folder inside to the SD card. No
                                    BIOS included: the Multi uses the board's.
MaiyaLuna-BackBit-MVS_v1.zip        BackBit Platinum folder, MVS build
MaiyaLuna-BackBit-AES_v1.zip        BackBit Platinum folder, AES build
                                    Copy the maiyaluna folder inside to the
                                    microSD card. No BIOS included. The
                                    BackBit is an MVS cartridge: use the MVS
                                    folder on an arcade board.
MaiyaLuna-WIP-EagleBIOS_v1.zip      MAME: the MVS build with EagleBIOS
                                      mame neogeo -rompath roms -hashpath hash -bios euro -cart1 maiyaluna
MaiyaLuna-WIP-AES-EagleBIOS_v1.zip  MAME: the AES build with EagleBIOS and a blank memory card
                                      mame aes -rompath roms -hashpath hash -bios asia -cart1 maiyaluna -memc card.bin
MaiyaLuna-CHANGELOG_v1.txt          what is in this set
MaiyaLuna-MANIFEST_v1.txt           size, CRC32 and SHA-256 of every v1 file

EagleBIOS is the free, open system ROM written for NeoGeoSDK. It is the
only system ROM in these packages: no SNK BIOS and no Universe BIOS is
included. MAME warns that its checksums differ from the original BIOS:
that is expected.

CONTROLS
--------
Stick       walk
A           talk, read, take, turn, ring, fit
B           Maiya's whip / Luna's close look
C           use the tool chosen in the notebook
D           switch sisters while both travel (held: wait / follow)
START       notebook: bag, finds, notes, journey, saving
Arcade settings (MVS): HOW TO PLAY, DANGER, HINTS.

STATUS
------
Checked in MAME (MVS, EagleBIOS): boot, title, choosing a sister, the
prologue; test builds play each journey through all seven chapters to the
credits. On the host, both journeys are played to the end with every find
collected and no place where a sister can be stranded.

Real hardware is not verified yet: reports from NeoSD, Darksoft, BackBit,
MVS and AES owners are very welcome (board or console, BIOS, cart firmware,
what you saw and heard).
