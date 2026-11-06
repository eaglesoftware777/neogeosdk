No Signal Z Alpha V3 - Native Neo Geo CD

Original single-player beat-'em-up: Jay, Zoe or Kai, eight stages, console
menus and the cartridge V3's moves, graphics, effects, voices and music.

This package contains the complete disc/nosignal.chd, including the data
track and all 18 audio tracks. Select it directly in MAME. To produce a
burnable mixed-mode CUE/BIN image, use extract_disc.bat or run:
  chdman extractcd -i disc/nosignal.chd -o extracted/nosignal.cue
Create the extracted directory first. Burn the resulting CUE with a suitable
mixed-mode CD writer. Physical hardware has not been tested. The source
build also produces the full ISO/CUE/WAV set.

Windows MAME:
1. Supply your own legitimate system ROM and LO data. They are not bundled.
2. Put the stock CDZ neocd.bin and 000-lo.lo in roms-stock/neocdz/.
   For EagleBIOS-CDZ, use the equivalent files in roms-eagle/neocdz/.
   EagleBIOS CD firmware is distributed separately with SDK 1.7.7.
3. Set MAME_EXE to your MAME executable, or place MAME on PATH.
4. Use run_stock.bat or run_eagle.bat. Configurations and saves are separate.
   The drive-menu helper releases forced controls before the game starts.

Controls: stick moves, double-tap runs; A attacks, Down+A sweeps;
B jumps (A airborne kicks); C grabs/throws; D uses the special.
Walk over pickups or onto the scooter; B leaves the scooter. START pauses.

Native MAME CDZ testing passed for all three heroes on both BIOS profiles,
including controller actions, gameplay, effects, music, exact program and
firmware readback, all 13 scenery transfers and resident sprite integrity.
CHD integrity and boot were checked. This is emulator evidence, not physical
hardware certification. Front/top models, full campaigns and power-cycle
save persistence remain unverified. Session progress/unlocks are retained.

Source and build instructions: games/nosignal/cd/README.md in NeoGeoSDK.

Source commit: 44d6daa3d2a88e45e38d06720528c183f6e69bd6
https://github.com/eaglesoftware777/neogeosdk/commit/44d6daa3d2a88e45e38d06720528c183f6e69bd6
