SKY LANCE  --  v1 ROMs  --  Eagle Software, 2026
================================================

!! WORK IN PROGRESS !!  A preview build, not a finished game.

Sky Lance's first ROM set, shared with the NeoGeoSDK v1.7.3 pre-release.
Every file carries "_v1" in its name.

A vertical arcade shooter: three pilots, seven long routes over painted
terrain, three guarded command targets on every route and a boss at its
end. Restore the peace: defeat the rogue faction and save the people
caught in their war. (Every change: SkyLance-CHANGELOG_v1.txt)

- Rook flies an F-16, Kira an F-35 and Blaze an F-15, each with its own
  handling, primary fire and super bomb.
- On each route, three command targets wait on their pads, guarded by
  escorts, a radar site and a mini-boss. Stay out of the radar circle or
  six more defenders come for you.
- Every boss is met in an open arena at its route's end: a battleship on
  the deep sea, a tank fortress on an empty desert, a castle over a lava
  plain...
- Chaff can pull a guided missile onto a decoy, but not always: later
  routes, a higher difficulty and the bosses' own missiles see through it.
  When a missile stays locked on, the radar scope turns red.
- Cockpit instruments beside the playfield: an attitude indicator that
  banks with the jet, a radar scope and a G meter.
- Arcade settings (MVS): LIVES, HOW TO PLAY and DIFFICULTY (EASY to
  EXPERT). A console plays NORMAL.

FILES
-----
SkyLance-WIP-NeoSD_MVS_v1.neo     NeoSD image, arcade (MVS) build
SkyLance-WIP-NeoSD_AES_v1.neo     NeoSD image, console (AES) build
                                  Cartridge data only: the NeoSD uses the
                                  system's own BIOS.
SkyLance-Darksoft-MVS_v1.zip      Darksoft Neo Geo Multi folder, MVS build
SkyLance-Darksoft-AES_v1.zip      Darksoft Neo Geo Multi folder, AES build
                                  Copy the folder inside to the SD card. No
                                  BIOS included: the Multi uses the board's.
SkyLance-BackBit-MVS_v1.zip       BackBit Platinum folder, MVS build
SkyLance-BackBit-AES_v1.zip       BackBit Platinum folder, AES build
                                  Copy the skylance folder inside to the
                                  microSD card. No BIOS included.
SkyLance-WIP-EagleBIOS_v1.zip     MAME: the MVS build with EagleBIOS
                                    mame neogeo -rompath roms -hashpath hash -bios euro -cart1 skylance
SkyLance-WIP-AES-EagleBIOS_v1.zip MAME: the AES build with EagleBIOS and a blank memory card
                                    mame aes -rompath roms -hashpath hash -bios asia -cart1 skylance -memc card.bin
SkyLance-CHANGELOG_v1.txt         what is in this set
SkyLance-MANIFEST_v1.txt          size, CRC32 and SHA-256 of every v1 file

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
Checked in MAME (MVS and AES, EagleBIOS): boot, title, attract, pilot
select, all seven routes with their encounters, every boss in its arena,
the Crimson Keep's turrets, the sea routes' warships, chaff against
guided missiles. Real hardware is not verified yet: reports from NeoSD,
Darksoft, BackBit, MVS and AES owners are very welcome.
