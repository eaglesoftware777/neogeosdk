MAIYA: SUPER NATURE GIRL  --  v3 ROMs  --  Eagle Software, 2026
================================================================

!! WORK IN PROGRESS !!  A preview build, not a finished game.

These are the third ROM set, shared with the NeoGeoSDK v1.7.2 pre-release.
The v1 and v2 files (on the v1.7.1 pre-release) stay exactly as they were
published; every v3 file carries "_v3" in its name.

WHAT'S NEW SINCE v2   (every change: Maiya-CHANGELOG_v3.txt)
-------------------
- No more catching on every blow: beating a creature no longer freezes
  the whole screen for a few frames (it read as the game hanging while
  she ran and jumped). Guardian blows and her own hurt hold 2 frames.
- Nothing is drawn while the picture is on screen: sprites, text and
  colours go to video memory only in the vertical blank, with a deadline
  eight lines before the picture starts; what doesn't fit waits for the
  next blank, most important first, and positions never wait.
- Lighter frames: every stage runs at about 59 frames a second, with 0-2%
  of frames running long, and at most 72 sprites on any line.
- Her stride: she builds up to a walk and a run instead of snapping to
  speed, stops firmly, skids when turned against her momentum and only
  then turns round. She now runs as fast to the left as to the right
  (an old rounding gave the left a pixel a frame more).
- The camera eases after her and keeps more road ahead, without jolting
  on stops and turns.
- Every road laid out again: each valley built from its own mix of steps,
  stairs, bridges, islands, shelves, climbs and open road, instead of one
  staircase repeated. Every ledge, pick-up and secret can be reached.
- Ledges with more to them: a second middle piece, small one-block
  islands, broken ends on crumbling ledges, and something hanging under
  the wide ones in each valley (roots, icicles, a chain, kelp...).
- The Rio Negro Works pour poison from three drums: shut all three (stand
  close, press Up) and the gate will open.
- Dust at her feet as she runs, skids and lands from a fall.
- "How to play", A and B together: each move played for real on the
  first valley's road, Maiya's then Luna's, one at a time.
- From the attract demo to the chooser cleanly, without a torn frame.
- The flight over a healed valley is clear: no shower falling across
  the screen; the colours still come back as she flies.
- Same cartridge layout as v1 and v2: P 512 KiB, S 128 KiB, M 128 KiB,
  V 8 MiB, C 8 MiB; the sound (M and V) is v2's, byte for byte, the
  arcade sound fix included; the boot is the same.

FILES
-----
Maiya-WIP-NeoSD_MVS_v3.neo     NeoSD image, arcade (MVS) build
Maiya-WIP-NeoSD_AES_v3.neo     NeoSD image, console (AES) build
                               Cartridge data only: the NeoSD uses the
                               system's own BIOS.
Maiya-Darksoft-MVS_v3.zip      Darksoft Neo Geo Multi folder, MVS build
Maiya-Darksoft-AES_v3.zip      Darksoft Neo Geo Multi folder, AES build
                               Copy the folder inside to the SD card. No
                               BIOS included: the Multi uses the board's.
Maiya-BackBit-MVS_v3.zip       BackBit Platinum folder, MVS build
Maiya-BackBit-AES_v3.zip       BackBit Platinum folder, AES build
                               Copy the maiya folder inside to the microSD
                               card. No BIOS included: the BackBit uses
                               the board's.
Maiya-WIP-EagleBIOS_v3.zip     MAME: the MVS build with EagleBIOS
                                 mame neogeo -rompath roms -hashpath hash -bios euro -cart1 maiya
Maiya-WIP-AES-EagleBIOS_v3.zip MAME: the AES build with EagleBIOS and a blank memory card
                                 mame aes -rompath roms -hashpath hash -bios asia -cart1 maiya -memc card.bin
Maiya-CHANGELOG_v3.txt         every change from v2 to v3
Maiya-MANIFEST_v3.txt          size, CRC32 and SHA-256 of every v3 file

EagleBIOS is the free, open system ROM written for NeoGeoSDK. It is the
only system ROM in these packages: no SNK BIOS and no Universe BIOS is
included. MAME warns that its checksums differ from the original BIOS:
that is expected. EagleBIOS keeps console saves on a memory card in the
same layout as the original console BIOS, so one card works with all of
them. The AES MAME zip comes with a blank card, card.bin, in its command.

To play under the Universe BIOS or an original BIOS in MAME, put your own
copy in MAME's roms/neogeo (or roms/aes) and pick it with -bios, for
example -bios unibios40.

ABOUT STALLS IN MAME ON WINDOWS
-------------------------------
Short, regular pauses (about a third of a second every few seconds) seen
on one Windows PC came from the PC's display and timing, not from the
game: the game kept emulating at many times full speed through them, and
they showed in other programs too. If you see such pauses, try 60 Hz on
the display, the latest graphics driver, the PC's power features off
(or a "high performance" plan), and closing background utilities; a
latency checker shows which driver is at fault.

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
Up beside a poison drum: shut it (the works)
Forward, Down, Down-Forward + B : Rising Bloom
Down, Forward + B               : Rose Blossom Surge

STATUS
------
Checked in MAME (MVS and AES, EagleBIOS): boot and eye-catcher (the same
as v2's, frame for frame), title, the attract demo, the chooser, play on
every stage, the guardians, continues, the move demo; the game's own
movement, climbing, arena, bonus, pick-up, pit and flight checks pass on
both. Real hardware is not verified yet: reports from NeoSD, Darksoft,
MVS and AES owners are very welcome (board or console, BIOS, cart
firmware, what you saw and heard).
