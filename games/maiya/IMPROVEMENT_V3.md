# Maiya V3 Test Pass

This is a separate test build. It does not replace the existing release images.

## Visual changes

- The 12 stage backgrounds keep their existing compositions, tile indices, and parallax bands.
- The active stage may cycle a close pair of blue palette entries every 16 frames. The pair is selected from the bank used by a sky or waterfall tile; stages without a sufficiently close pair do not cycle. Arena backgrounds disable the cycle. Palette writes follow the normal VBlank queue.
- Shared flower, mushroom, coin, and beetle art has small native-resolution cleanup. No extra sprite strips or palette banks are allocated.
- Character selection flushes only dirty sprite fields instead of rewriting both character tile maps every frame.

## Build and capture

From the repository root on WSL:

```sh
python3 games/maiya/tools/build.py --platform mvs --rebuild-art
python3 games/maiya/tools/build.py --platform mvs --quick
python3 games/maiya/tools/prepare_v3_capture.py --refresh
python3 games/maiya/tools/package_v3_test.py --output dist/release/Maiya-WIP-NeoSD_MVS_v3_RC2_TEST.neo
```

On the first capture setup, supply `--bios-dir` pointing to your own MAME-compatible Neo Geo BIOS files (`sp-s2.sp1`, `sm1.sm1`, `000-lo.lo`, and `sfix.sfix`). The preparer verifies their CRCs and stages an isolated `neogeo.zip`. Later `--refresh` runs can reuse it. The isolated cartridge is named `maiya.zip` so MAME loads it through the generated software-list entry; loading the ZIP by absolute path bypasses that metadata and does not boot this set correctly.

The capture preparer stages an isolated cartridge under `C:\mame\neogeosdk\render-v3-tests\mvs` and generates Windows batch files in `C:\mame\neogeosdk`. Use `run_maiya_v3_01_emerald_forest.bat` through `run_maiya_v3_12_smog_citadel.bat` to inspect each level. Each matching `record_` file captures its AVI and WAV separately, then makes an MP4 when `ffmpeg` is installed. Existing recordings are never overwritten.

Transition launchers are named `run_maiya_v3_transition_01_to_02.bat` through `run_maiya_v3_transition_11_to_12.bat`, with matching `record_` files. The script uses the game's interlude state; it does not add a shortcut to the cartridge. Attract, title, selection, intro, bonus, ending, and full-run launchers are also generated. The ending launcher enters the final-stage clear path, allowing the game to perform its own ending setup.

The MVS test image uses the current P/S/C graphics and program with the known-good M1 and 8 MiB V1 from the protocol test image. `package_v3_test.py` rejects changes to the M1/V1/V2 regions and writes a region hash manifest next to the image. A separate AES image is intentionally not created by this test script because the existing AES image and current program use different sound initialization protocols; pairing them without a full AES sound/UniBIOS test would be unsafe.

`Maiya-WIP-NeoSD_MVS_v3_TEST.neo` was an earlier preliminary build. Use the `v3_RC2_TEST` image for this pass.

## Verification status

The Python asset tests and SDK sprite/geometry tests pass. Both MVS and AES program builds compile. Windows MAME 0.283 reached gameplay in all 12 stage selectors, and representative stills from all 12 showed the character, HUD, and distinct backgrounds together. The ending selector reached the ending state and displayed a healed-valley page. Full motion/playthrough review, sound checks, NeoSD hardware, and the AES UniBIOS menu remain unverified. Do not treat this test image as a release asset until those checks pass.
