# Tools and Utilities

**Eagle Software · Neo Geo SDK v1.7.0**

Every helper script that ships with the SDK, what it does, and when you would
run it by hand. Most of these are invoked for you by a make target — the
reason to know them individually is debugging, one-off conversions, and
building your own pipeline steps.

Anything under `artbox/` reads and writes the *current game's* artbox
directory, which the makefiles select through `ARTBOX_DATA_DIR`. Run them
through `make art` unless you specifically want to drive one stage on its
own.

---

# 1. Graphics — `artbox/`

## 1.1 Conversion cores

These are the scripts that turn pixels into Neo Geo data. One of them is
selected per build; the others are alternatives you opt into.

| Script | Role |
|---|---|
| `img2neo_tile.py` | **The default.** Tile-local quantiser: per-tile Lab k-means++, per-tile dither, hardware-lattice palette refit, void-and-cluster blue-noise dithering, greedy MAE bank dedup, Lab-nearest remap. Both makefiles export `ARTBOX_TILE=1`, so this is the route every ROM takes. |
| `img2neo_crt.py` | CRT-preprocessing route: CIE-Lab k-means, horizontal-biased Floyd-Steinberg, gamma 1.20 / contrast 1.10 pre-boost. `make art-crt` or `ARTBOX_CRT=1`. |
| `img2neo_hd.py` | High-quality photo route: bilateral filter, CLAHE on the Lab L channel, unsharp mask, blue-noise dither. CLI flags `--no-bilateral`, `--no-clahe`, `--no-unsharp`, `--dither blue\|fs\|ordered\|none`. |
| `img2neo.py` | The original nearest-neighbour path, kept for diffing (`ARTBOX_LEGACY=1`). Also home to `alpha_bleed()` and `fit_sprite_rgba()`, which the other routes use. |
| `fixtiles.py` | FIX-layer (S-ROM) tile conversion. |
| `fixtiles_hd.py` | Per-tile-palette FIX conversion. `--sharp-text` binarises glyph and HUD sources; writes a sidecar `*.pal.json` with the per-tile palette table. |

## 1.2 Pipeline stages

Run in this order by `makeartbox.sh` / `makeartbox.bat`:

| Script | What it does |
|---|---|
| `createromdb.py` | Creates the ROM database (`neorom.db`) the later stages import into. |
| `romdbimgimport.py` | Imports converted sprite and screen images into the ROM database. Owns the screen fitting modes — `crop`, `contain`, `pad`, `letterbox` — and records the real content box per asset. |
| `romdbfiximport.py` | Imports FIX-layer tiles into the ROM database. |
| `romtiles.py` | Packs the database's tile data into the C-ROM pair (`<id>-c1.c1`, `<id>-c2.c2`). |
| `genscreens.py` | Emits `screens.c` — one `showScreenN()` entry point per asset, plus the geometry each needs. |
| `gen_sprite_meta.py` | Emits `sprite_meta.h` from `assets_manifest.json` — per-asset tile base, strips, rows and padding. |
| `gen_eyecatcher.py` | Emits `eyecatcher.c` from the manifest entries with `category == "eyecatcher"`, as an animation. |
| `gen_assets_cfg.py` | Regenerates `assets.cfg` with the default per-category rules on each `make art`. |
| `genmapdb.py`, `genmapfile.py` | Build the tile map database and the `map` file the ROM tooling consumes. |
| `asset_rules.py` | The rule engine behind `assets.cfg`: category matching, filename patterns, fit and anchor resolution. Imported by the stages above rather than run directly. |

## 1.3 Utilities

| Script | Use |
|---|---|
| `romtool.py` | A portable Python 3 port of ROMWak. Splits and interleaves ROM images — `/b` splits alternating bytes, and the byte-swap and pad operations the P-ROM build relies on. |
| `validate_assets.py` | Checks generated `sprite_meta.h` for tile-window safety — that every asset's declared window is inside its allocated tile range. `python3 artbox/validate_assets.py games/demo/artbox/sprite_meta.h` |
| `count_assets.py` | Prints the number of assets the current `in/` tree will produce. Useful before an id-sensitive change, because an asset id is its position in the build order. |
| `fix_sprite_alpha.py` | Repairs sprite transparency before C1/C2 generation, for sources whose alpha channel does not survive an editor round-trip. Run from the repository root. |
| `process_eyecatcher_ng.py` | Pre-quantises eyecatcher PNGs onto the Neo Geo colour grid before the main conversion, so the k-means stage starts from colours the hardware can actually hold. |
| `sync_main_screens.py` | Syncs the generated `showScreenN()` declarations and `ng_screen_table[]` into a game's `main.c`. |
| `artbox_studio.py` | The PyQt6 desktop front end — see §5. |

## 1.4 Shell wrappers

| Script | Platform | What it runs |
|---|---|---|
| `makeartbox.sh` / `makeartbox.bat` | Linux / Windows | The full art build for one game. `GAME_ID=777 ./artbox/makeartbox.sh demo` |
| `artbox.sh` / `artbox.bat` | both | The bare stage sequence, without the per-game path setup. |
| `romts.sh` / `romts.bat` | both | Sprite-tile ROM packing. |
| `romfx.sh` / `romfx.bat` | both | FIX ROM packing. |
| `makeclean.sh` / `makeclean.bat`, `clean.sh` / `clean.bat` | both | Remove generated artbox intermediates. |

## 1.5 GIMP plug-ins — `tools/gimp-plugins/`

Manual pre-processing for source art, before it reaches `img2neo*.py`.
Two sets of the same three tools: `gimp2.10/` (Python-Fu, `gimpfu`) and
`gimp3.2/` (GObject-Introspection, GEGL buffers).

| Plug-in | What it does |
|---|---|
| `smart_background_remove` | Removes a flat or near-flat background while keeping anti-aliased contours intact. |
| `remove_white_edge_noise` | Strips the pale fringe that survives a background removal — the same class of artefact `alpha_bleed()` handles in the pipeline, but applied at source. |
| `neogeo_hd_pixel_enhance` | Sharpens and cleans up-scaled or photographic source so the quantiser has crisper edges to cluster. |

---

# 2. Sound — `sound/tools/`

## 2.1 Compilers

| Script | Input | Output |
|---|---|---|
| `mml_compile.py` | `*.mml` music tracks | `music_data.inc`. Handles the SSG/music MML directives, including `@aN` to fire ADPCM-A sample *N* from a track. |
| `fm_compile.py` | `*.fm` FM tracks | `fm_data.inc` |
| `fm_patch_compile.py` | `sound/fm/patches.fm` | `fm_patch_table.inc` — 31-byte patches over four operators |
| `ssg_compile.py` | SSG tracks | `ssg_data.inc` |
| `ssg_config_compile.py` | SSG preset config | `ssg_config.inc` — 6-byte presets |
| `vrom.py` | encoded ADPCM banks | the V-ROM, `sample_table.inc`, and the generated `sdk/sound_voice_ids.h`. Bundles the voice bank immediately after the SFX bank and exports `ADPCMA_VOICE_BASE` / `ADPCMA_VOICE_COUNT`. |

## 2.2 Sample encoding

| Script | What it does |
|---|---|
| `adpcm_enc.py` | The YM2610 ADPCM encoder, matching real hardware decoding exactly. ADPCM-A is OKI-style 4-bit at a fixed 18.5 kHz with a 12-bit signed accumulator; ADPCM-B is the streamed format. |
| `wav_to_raw_pcm.py` | WAV to raw PCM, the stage before ADPCM encoding. Handles 8-, 16-, 24- and 32-bit sources. |
| `enc_wave16le_a.sh` / `.bat` | WAV to 16-bit little-endian PCM for the ADPCM-A bank. |
| `enc_wave16le_b.sh` / `.bat` | The same for the ADPCM-B bank. |
| `enc_wave16le_a_voice.sh` / `.bat` | The same for the voice bank. Skips cleanly when `in_wav_a_voice/` is absent. |
| `adpcm_enc_a.sh` / `.bat` | PCM to ADPCM-A. |
| `adpcm_enc_b.sh` / `.bat` | PCM to ADPCM-B. |
| `adpcm_enc_a_voice.sh` / `.bat` | PCM to ADPCM-A for the voice bank. |
| `adpcm_enc_process.sh` / `.bat` | Chains all three encode stages in order: SFX, voice, then ADPCM-B. |

The Python path is the default on both platforms. Set `SOX=/path/to/sox`
only when you specifically want the SoX conversion route.

## 2.3 Driver build and comparison

| Script | What it does |
|---|---|
| `m1rom.sh` / `m1rom.bat` | Assembles and links the M1 (Z80 driver) ROM with `wla-z80` + `wlalink`. |
| `vrom.sh` / `vrom.bat` | Shell wrappers around `vrom.py`. |
| `combine_split_driver.py` | Merges the fixed Z80 prelude (`driver_prelude.asm`) with compiler-generated driver assembly. Used only by the experimental C driver path. |
| `compare_m1.py` | Compares two generated M1 ROM images and reports the first mismatch, with SHA256 of each. This is what `make compare-driver` uses to check the assembly and C drivers against each other. |

## 2.4 Sound Studio

`sound/sound_studio.py` — the PyQt6 desktop front end. See §5.

---

# 3. Verification — `tools/`

Run by the build, and worth running by hand when something looks wrong.

| Script | Checks |
|---|---|
| `check_game_cfg.py` | That `GAME=` matches `CURRENT_GAME` in the selected `game.cfg`. Every make target depends on this, and it is what stops one game's assets being built into another game's ROM. |
| `verify_artbox_palettes.py` | Per-game artbox palette output: manifest count, palette bank range, and `neopal.bin` record integrity (136 bytes per record — an int32 index plus sixteen 64-bit palette words). `make art` runs it and fails the build on a mismatch. |
| `verify_sfix_output.py` | The generated FIX ROM: size, format, and tile layout. |
| `gen_api_reference.py` | Regenerates `docs/API_2D_ENGINE_C.md` and `docs/API_2D_ENGINE_CPP.md` from the engine headers, so the API reference cannot drift from the code. Run it after adding or changing a public engine call. |
| `make_manual_pdf.py` | Builds `docs/neogeosdk_v1.7.0_manual.pdf` — the programmer's manual, the SDK library reference, and both 2D engine references — from the markdown in `docs/`. Run `gen_api_reference.py` first if the engine changed. |

```sh
python3 tools/verify_artbox_palettes.py --root . --game demo
python3 tools/verify_sfix_output.py --root .
python3 tools/gen_api_reference.py
python3 tools/make_manual_pdf.py
```

---

# 4. Launchers and environment

## 4.1 Repository root

| Script | What it does |
|---|---|
| `neogeosdk.sh` / `neogeosdk.bat` | The interactive build menu — pick a game and a target without remembering the flags. Same as `make menu`. |
| `run.sh` / `run.bat` | Build and run the current game in MAME. |
| `run_neogeosdk.sh` / `.bat` | Run the built ROM in MAME with the correct rompath, hashpath and BIOS. |
| `run_neogeosdk_debug.sh` / `.bat` | The same with the MAME debugger attached. |
| `run_neogeosdk_lua.bat` | Run with a MAME Lua `-autoboot_script`, for automated inspection and input driving. |
| `run_neogeosdk_trace.bat` | Run with MAME tracing enabled. |
| `set_sdkhome.bat` | Sets `SDKHOME` for a Windows shell session. |

## 4.2 `dist/`

`make dist` produces per-game run scripts alongside the packaged ROM:
`run_demo.sh` / `.bat`, `run_helloworld.sh` / `.bat`, `run_game.sh` / `.bat`,
and a `_debug` variant of each.

## 4.3 `install/`

One-shot toolchain installers:

| Script | Host |
|---|---|
| `install-linux.sh` | Generic Linux |
| `install-ubuntu.sh` | Ubuntu, with the distro package names resolved |
| `install-windows.bat` | Native Windows |
| `install-wsl.bat` | WSL, including the Windows-side setup |

See [`../install/README.md`](../install/README.md).

## 4.4 `win/`

Prebuilt Windows helper binaries used by `MakefileWin32.mak` so the Windows
build needs no extra installs: `srec_cat.exe`, `srec_cmp.exe`,
`srec_info.exe`, `xxd.exe`.

---

# 5. Desktop applications

Both are PyQt6 and both drive the same scripts the makefiles do — they make
the feedback loop visual, they do not replace the build.

## 5.1 Artbox Studio — `artbox/artbox_studio.py`

| Tab | What it does |
|---|---|
| **Pipeline** | Runs every conversion step with a status pill per step and a live log. "Run Full Pipeline" chains the standard set; HD variants are opt-in. |
| **Asset Browser** | Tree of every PNG under `artbox/in/*` and `artbox/infix/`, with dimensions, file size, and a native-scale preview. |
| **Hex Sprite Inspector** | Pick a tile index and see its decoded 16 × 16 grid, the raw C1/C2 ROM bytes, and a live palette-swap dropdown. The tool for debugging palette assignment and tile order. |
| **Movement Designer** | Build a frame sequence (tile + duration), loop-preview it at a chosen FPS, and export `anim_frame_tile[]` / `anim_frame_dur[]` C arrays in vblank units. |
| **Level Designer** | Paint a 20 × 14 tilemap with tile thumbnails; export a `level_tile[LEVEL_H][LEVEL_W]` C array. |
| **HD Compare** | Run one PNG through both the standard and HD pipelines and compare the indexed results. Output lands in `artbox/out/_hd_cmp/`; originals are untouched. |
| **ROM Inventory** | Every ROM kind (p1 / m1 / s1 / v1 / c1 / c2) for every game folder, with size, mtime, and present/absent colour coding. |
| **Asset Rules** | Editor for `artbox/assets.cfg`, with save and reload. |

## 5.2 Sound Studio — `sound/sound_studio.py`

Track, Mix and ROM tabs, a live waveform view, and an MML designer for
composing and auditioning music without a full `make sound` cycle.

Full guide: [`SOUND_STUDIO_GUIDE.md`](./SOUND_STUDIO_GUIDE.md).

---

# 6. Debug artefacts

`make debug-artifacts` writes these into `dump/`, using the cross-toolchain's
own binutils:

| File | From | Use |
|---|---|---|
| size report | `m68k-unknown-elf-size` | Section sizes — is the ROM about to overflow |
| symbol table | `m68k-unknown-elf-nm -n` | Symbol addresses, which is how you find a variable to watch from a MAME Lua script |
| ELF headers | `m68k-unknown-elf-readelf` | Segment layout |
| linker map | `ld -Map` | Where every object landed |
| disassembly | `m68k-unknown-elf-objdump -d` | What the compiler actually emitted |

`make gdb-trace` runs a batch GDB script and writes `dump/gdb_trace.txt`.
See [`GDB_GUIDE.md`](./GDB_GUIDE.md).
