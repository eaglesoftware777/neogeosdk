Maiya MAME tests (v2 ROMs)
==========================

Windows, from this folder:
    run_maiya_mvs.bat           MVS build, EagleBIOS (euro)
    run_maiya_aes.bat           AES build, EagleBIOS (asia)
    run_maiya_mvs_unibios.bat   MVS build, UniBIOS 4.0 (own settings and saves)
    run_maiya_aes_unibios.bat   AES build, UniBIOS 4.0 (own settings and saves)
    run_maiya.bat mvs|aes BIOS  either build with any BIOS name MAME knows

EagleBIOS, the open NeoGeoSDK system ROM, is installed in roms\neogeo and
roms\aes. UniBIOS is never installed by the SDK: put uni-bios_4_0.rom in
roms\neogeo and roms\aes yourself for the UniBIOS launchers. MAME warns that
EagleBIOS's checksums differ from the original BIOS: that is expected.

UniBIOS in-game menu, during play:
    MVS  Start+Coin    (keys 1 and 5)  or Start+A+B+C
    AES  Start+Select  (keys 1 and 5)  or Start+A+B+C
    (A, B, C = Left Ctrl, Left Alt, Space; some keyboards cannot take
    1+Ctrl+Alt+Space at once, so 1+5 is the safer one)

MVS and AES have separate ROMs, software lists, settings and saves under
tests\mvs and tests\aes. The AES launchers insert a memory card
(tests\aes\memcard.bin, and its own for UniBIOS): console saves outlast
MAME, and the card works the same on EagleBIOS and UniBIOS. roms\maiya and roms\maiya.zip hold the MVS build.

Reinstall after `make neo GAME=maiya`, from the SDK checkout:
    py tools\install_maiya_mame_tests.py C:\mame\neogeosdk --tidy
Replaced files, older launchers and the test folders of earlier builds are
moved to backups\ first.

MAME testing does not replace NeoSD, Darksoft or real-board testing.
