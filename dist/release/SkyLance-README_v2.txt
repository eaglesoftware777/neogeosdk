SKY LANCE  --  v2 ROMs  --  Eagle Software, 2026
================================================

!! WORK IN PROGRESS !!  A preview build, not a finished game.

Sky Lance's second ROM set, shared with the NeoGeoSDK v1.7.7 pre-release.
Every file carries "_v2" in its name.

A vertical arcade shooter: three pilots, seven long routes over painted
terrain, three guarded command targets on every route and a boss at its
end. v2 is the hardware fix first published as Hardware RC1, now in every
format: on some BIOSes the terrain and aircraft could stay black because
a palette upload kept waiting; the finished palette now goes up at the
start of the cartridge's own vertical blank. The overhead cloud keeps a
straight course, the HUD sits inside CRT overscan and enemy bullets are
drawn first. Gameplay and difficulty are those of v1.
(Every change: SkyLance-CHANGELOG_v2.txt)

FILES
-----
SkyLance-WIP-NeoSD_MVS_v2.neo      NeoSD image, arcade (MVS) build
SkyLance-WIP-NeoSD_AES_v2.neo      NeoSD image, console (AES) build
                                   Cartridge data only: the NeoSD uses the
                                   system's own BIOS.
SkyLance-Darksoft-MVS_v2.zip       Darksoft Neo Geo Multi folder, MVS build
SkyLance-Darksoft-AES_v2.zip       Darksoft Neo Geo Multi folder, AES build
                                   Copy the folder inside to the SD card. No
                                   BIOS included: the Multi uses the board's.
SkyLance-BackBit-MVS_v2.zip        BackBit Platinum folder, MVS build
SkyLance-BackBit-AES_v2.zip        BackBit Platinum folder, AES build
                                   Copy the skylance folder inside to the
                                   microSD card. No BIOS included. The
                                   BackBit is an MVS cartridge: use the MVS
                                   folder on an arcade board.
SkyLance-WIP-EagleBIOS_v2.zip      MAME: the MVS build with EagleBIOS
                                     mame neogeo -rompath roms -hashpath hash -bios euro -cart1 skylance
SkyLance-WIP-AES-EagleBIOS_v2.zip  MAME: the AES build with EagleBIOS and a blank memory card
                                     mame aes -rompath roms -hashpath hash -bios asia -cart1 skylance -memc card.bin
SkyLance-CHANGELOG_v2.txt          what is in this set
SkyLance-MANIFEST_v2.txt           size, CRC32 and SHA-256 of every v2 file

EagleBIOS is the free, open system ROM written for NeoGeoSDK. It is the
only system ROM in these packages: no SNK BIOS and no Universe BIOS is
included. MAME warns that its checksums differ from the original BIOS:
that is expected.

CONTROLS
--------
Stick       fly; hold left or right to bank to 45 degrees
A           fire (hold for automatic fire)
B           afterburner; with a direction: barrel roll (left/right),
            high-G climb (up), Split-S dive (down)
C           super bomb (counted)
D           heavy missile (counted)
B + D       chaff (six per mission)

STATUS
------
Checked in MAME (MVS and AES, EagleBIOS): boot, title, attract and play
on the v2 build; the fix itself was checked in four 90-second runs on MVS
and AES with EagleBIOS and Universe BIOS 4.0 (Hardware RC1).

Real hardware is not verified yet: reports from NeoSD, Darksoft, BackBit,
MVS and AES owners are very welcome (board or console, BIOS, cart firmware,
what you saw and heard).
