Maiya v1.7.1 prerelease MAME tests
================================

Windows CMD, from this folder:
    run_maiya_mvs.bat          MVS sound-only fix, Europe BIOS
    run_maiya_aes.bat          AES boot-graphics fix, UniBIOS 4.0
    run_maiya.bat aes asia     Optional standard AES BIOS

MVS and AES have separate ROMs, matching software-list hashes, configurations
and NVRAM under tests/mvs and tests/aes. Existing system BIOS files under roms
are retained. UniBIOS 4.0 must already be available in the system BIOS set.
The MVS image retains the original graphics; the AES image retains the original
sound driver. Both retain the 8 MiB V1 sample region.

Old installed files are backed up under backups before replacement.
sv_mvs.bat and v.bat use the corrected MVS set for new recordings/playbacks.
Old input recordings may not reproduce correctly with a changed program ROM.

Reinstall after updating the SDK test images:
    py tools\install_maiya_mame_tests.py C:\mame\neogeosdk
Run that command from the SDK checkout. Any MAME installation path is accepted.

MAME verification does not replace physical NeoSD hardware testing.
