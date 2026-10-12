SHADOW LEAGUE FIGHTING  --  v1 ROMs  --  Eagle Software, 2026
==============================================================

!! WORK IN PROGRESS !!  The first playable build, not a finished game.

Shadow League Fighting's first ROM set, shared with the NeoGeoSDK v1.7.5
pre-release. Every file carries "_v1" in its name.

An original one-on-one fighting game: two fighters, one arena, arcade and
versus play. (Every change: ShadowLeague-CHANGELOG_v1.txt)

- Kenji Arata, traditional karate: lunging punches, a counter stance, an
  overhead axe kick.
- Jack Mercer, professional boxing (no kicks): jabs, hooks, a dashing
  straight, a slip-and-counter weave, an uppercut.
- Guard high and low, throws and throw escapes, rolls, guard attacks,
  supers from the power gauge.
- Arcade: beat the other fighter, then your own shadow. A second player's
  START during a match opens a versus match.

FILES
-----
ShadowLeague-WIP-NeoSD_MVS_v1.neo     NeoSD image, arcade (MVS) build
ShadowLeague-WIP-NeoSD_AES_v1.neo     NeoSD image, console (AES) build
                                      Cartridge data only: the NeoSD uses
                                      the system's own BIOS.
ShadowLeague-Darksoft-MVS_v1.zip      Darksoft Neo Geo Multi folder, MVS build
ShadowLeague-Darksoft-AES_v1.zip      Darksoft Neo Geo Multi folder, AES build
                                      Copy the folder inside to the SD card.
                                      No BIOS included.
ShadowLeague-BackBit-MVS_v1.zip       BackBit Platinum folder, MVS build
ShadowLeague-BackBit-AES_v1.zip       BackBit Platinum folder, AES build
                                      Copy the shadowleague folder inside to
                                      the microSD card. No BIOS included.
ShadowLeague-WIP-EagleBIOS_v1.zip     MAME: the MVS build with EagleBIOS
    mame neogeo -rompath roms -hashpath hash -bios euro -cart1 shadowleague
ShadowLeague-WIP-AES-EagleBIOS_v1.zip MAME: the AES build with EagleBIOS and a blank memory card
    mame aes -rompath roms -hashpath hash -bios asia -cart1 shadowleague -memc card.bin
ShadowLeague-CHANGELOG_v1.txt         what is in this set
ShadowLeague-MANIFEST_v1.txt          size, CRC32 and SHA-256 of every v1 file

EagleBIOS is the free, open system ROM written for NeoGeoSDK. It is the
only system ROM in these packages: no SNK BIOS and no Universe BIOS is
included. MAME warns that its checksums differ from the original BIOS:
that is expected.

CONTROLS
--------
Stick       walk, crouch, jump; hold back to guard (down-back for low attacks)
            forward forward: dash      back back: backdash
A  light punch      B  light kick      C  heavy punch      D  heavy kick
A + B       roll (back + A + B rolls away)
C + D       heavy attack; while guarding: guard attack (uses spirit)
fwd/back+C  throw (close); press C or D as you are grabbed to escape

Kenji Arata                          Jack Mercer
  Ressen Zuki     down, fwd + A/C      Brooklyn Rush  down, fwd + A/C
  Kokoro Uke      down, back + A/C     Weave          down, back + B/D
  Kakato Otoshi   fwd, down, df + B/D  Rising Bell    fwd, down, df + A/C
  Ichigeki Hissatsu (super)            Final Round (super)
                  down, fwd, down, fwd + A/C

STATUS
------
Checked in MAME (MVS and AES, EagleBIOS): boot, title, attract demo,
fighter select, VS card, rounds with K.O., time-up and perfect calls, game
over, the arcade ladder. Real hardware is not verified yet: reports from
NeoSD, Darksoft, BackBit, MVS and AES owners are very welcome.
