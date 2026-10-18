SHADOW LEAGUE FIGHTING  --  v3 ROMs  --  Eagle Software, 2026
=============================================================

!! WORK IN PROGRESS !!  A preview build, not a finished game.

Shadow League Fighting's third ROM set, shared with the NeoGeoSDK v1.7.7
pre-release. Every file carries "_v3" in its name.

An original one-on-one and three-on-three fighting game: 44 fighters, four
arenas, strikers in team battle. v3 brings every fighter's painted pose set
(2,460 frames), CPU levels 1 to 7, long, short and reverse throws, a
training mode with recording and playback, a new title and introduction,
and localized voice lines. (Every change: ShadowLeague-CHANGELOG_v3.txt)

FILES
-----
ShadowLeague-WIP-NeoSD_MVS_v3.neo      NeoSD image, arcade (MVS) build
ShadowLeague-WIP-NeoSD_AES_v3.neo      NeoSD image, console (AES) build
                                       Cartridge data only: the NeoSD uses the
                                       system's own BIOS.
ShadowLeague-Darksoft-MVS_v3.zip       Darksoft Neo Geo Multi folder, MVS build
ShadowLeague-Darksoft-AES_v3.zip       Darksoft Neo Geo Multi folder, AES build
                                       Copy the folder inside to the SD card. No
                                       BIOS included: the Multi uses the board's.
ShadowLeague-BackBit-MVS_v3.zip        BackBit Platinum folder, MVS build
ShadowLeague-BackBit-AES_v3.zip        BackBit Platinum folder, AES build
                                       Copy the shadowleague folder inside to the
                                       microSD card. No BIOS included. The
                                       BackBit is an MVS cartridge: use the MVS
                                       folder on an arcade board.
ShadowLeague-WIP-EagleBIOS_v3.zip      MAME: the MVS build with EagleBIOS
                                         mame neogeo -rompath roms -hashpath hash -bios euro -cart1 shadowleague
ShadowLeague-WIP-AES-EagleBIOS_v3.zip  MAME: the AES build with EagleBIOS and a blank memory card
                                         mame aes -rompath roms -hashpath hash -bios asia -cart1 shadowleague -memc card.bin
ShadowLeague-CHANGELOG_v3.txt          what is in this set
ShadowLeague-MANIFEST_v3.txt           size, CRC32 and SHA-256 of every v3 file

EagleBIOS is the free, open system ROM written for NeoGeoSDK. It is the
only system ROM in these packages: no SNK BIOS and no Universe BIOS is
included. MAME warns that its checksums differ from the original BIOS:
that is expected.

CONTROLS
--------
Stick              walk, crouch, jump; back guards, down-back guards low
Fwd,fwd / back,back dash / backdash
A / B / C / D      light punch / light kick / heavy punch / heavy kick
A + B              evade; back + A + B evades away
C + D              blowback; guard attack while blocking
B + C              MAX power mode (one stock)
Fwd + C / Fwd + D  long / short throw, close
Back + C or D      reverse throw, close
C or D when held   throw escape
A + C              call the striker (team battle)
AES: START opens the practice menu in TRAINING.

STATUS
------
Checked in MAME (MVS and AES, EagleBIOS): boot, title, battle choice,
fighter and striker select, the VS card, duels and team battles, victory
and continue. Training, attract and the AES options were checked on the
same ROMs before release. Still open: occasional slowdown or flicker in
play, arena art and depth, movement and title polish.

Real hardware is not verified yet: reports from NeoSD, Darksoft, BackBit,
MVS and AES owners are very welcome (board or console, BIOS, cart firmware,
what you saw and heard).
