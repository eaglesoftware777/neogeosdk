# P1 Step A: the platform layer (`sdk/platform/ng_platform.*`)

One place for everything that differs between targets. This is the design
only; nothing is built until it is approved (Step B builds the cartridge
backends; the CD backends come with CD1–CD5).

Facts are held to `docs/platforms/CLASSIC_BASELINE.md`. Sources are named,
not linked:
- [W] the NeoGeoDev wiki, by page;
- [M] MAME `src/mame/snk/` at commit f1f39d964476;
- [S] this SDK's own files.

## 1. What exists today

| Service | Where | How |
|---|---|---|
| Arcade or console | `sdk/cabinet/ng_sys.c` `ng_sys_is_mvs()` | `BIOS_MVS_FLAG` $10FD82: 0 AES, $80 MVS ([W: BIOS RAM locations], cited in `sdk/macro.h`) |
| Region | `ng_sys_region()` | `BIOS_COUNTRY_CODE` $10FD83 (JP 0, US 1, EU 2) |
| Universe BIOS | `ng_system.h` `ng_sys_is_unibios()` | scans the system ROM for its name |
| Settings | `ng_system.h` `ng_dip_option/time/count()` | the soft DIP copy at `BIOS_GAME_DIP` $10FD84; the table is in each game's `neogeo_mvs.c` / `neogeo_aes.c` |
| Save | `ng_system.h` `ng_save_*` | the block the cartridge header names ($10E start, $112 size); on MVS the system ROM loads it from backup RAM before USER and writes it back ([S: `ng_system.h`], [W: 68k memory map]). On AES it lives only for the session |
| Credits and Start | each game's `user.c` / `neogeo.c` | `SYS_CREDIT_CHECK` $C00450, `SYS_CREDIT_DOWN` $C00456 on MVS; Start alone on AES |
| Return to the system | `SYS_RETURN` $C00444 | [S: `sdk/macro.h`] |
| Music | `sdk/ng_audio`, `neogeolib.c` | commands to the Z80 driver |
| Assets | — | everything is in ROM: nothing to load |

The layer wraps these; it does not replace them.

## 2. Targets

- **Compile time:** `TARGET=cart` (the default) or `TARGET=cd`, passed as
  `NG_TARGET_CART` / `NG_TARGET_CD` by the Makefile. A game built without
  `TARGET` is the cartridge build it is today.
- **Run time, within a target:**
  - cart: MVS or AES, from `ng_sys_is_mvs()`;
  - cd: the CD model. MAME shows no register that tells front loader, top
    loader and CDZ apart ([M: neogeocd.cpp]; only a region at $FF011C).
    **Unknown.** Until a source is found, the CD reports "CD, model
    unknown" and nothing depends on the model.

```c
typedef enum { NG_SYSTEM_MVS, NG_SYSTEM_AES, NG_SYSTEM_CD } NGSystem;
NGSystem ng_platform_system(void);
uint8_t  ng_platform_region(void);        /* NG_REGION_JP / US / EU */
uint8_t  ng_platform_language(void);      /* from the region on cart; CD: see below */
```

## 3. The services

Each is a small function with one backend per target. On the cartridge,
each calls today's code.

| Service | API (proposed) | Cart backend (Step B) | CD backend (CD tasks) |
|---|---|---|---|
| System, region | `ng_platform_system()`, `ng_platform_region()` | `ng_sys_is_mvs()`, `ng_sys_region()` | the CD system; the region at $FF011C [M: neogeocd.cpp `control_r` 0x011c] |
| Language | `ng_platform_language()` | the region's language | unknown (what the CD BIOS offers): region until cited |
| Settings | `ng_platform_option(n)`, `ng_platform_options_set()` | MVS: the soft DIP copy. AES, or a system that left it unset: the defaults in the cartridge's own soft DIP table, read from ROM, overridden by an options page the game shows (§4) | the options page, kept in the save |
| Save | `ng_save_*` (unchanged) behind `ng_platform_save_kind()`, `ng_platform_save_capacity()` | MVS: backup RAM via the header block (as now). AES: the memory card through `SYS_CARD` $C00468 when one is in; else session only | the CD's internal 8 KiB card at $800000–$803FFF [M: neogeocd.cpp `memcard_r/w`, "internal memory card"] |
| Credits, Start | `ng_platform_start_mode()`, `ng_platform_credits()`, `ng_platform_credit_down()` | MVS: `SYS_CREDIT_CHECK` / `SYS_CREDIT_DOWN`. AES: Start alone | Start alone |
| Assets | `ng_asset_load(id)` returns a pointer | no-op: the data is in ROM | load the file from the disc (CD BIOS call to be cited in CD1) |
| Music | `ng_music_play(track)`, `ng_music_stop()` | the Z80 driver, as now | CD audio tracks, or the driver (CD tasks) |
| Return to the system | `ng_platform_exit()` | `SYS_RETURN` $C00444 | CD BIOS return (to be cited) |

### Capability flags (for Part X)

```c
typedef struct {
    uint8_t  fast_cpu;        /* 1 when a frame's measured spare time says the CPU runs fast (G3 measures it) */
    uint16_t save_capacity;   /* bytes the save backend holds (0: none) */
    uint8_t  language_source; /* NG_LANG_FROM_REGION, NG_LANG_FROM_BIOS, NG_LANG_FROM_OPTIONS */
    uint8_t  display_profile; /* NG_DISPLAY_FULL_320 or NG_DISPLAY_MASKED_304 (from game.cfg; G4) */
    uint8_t  stereo;          /* 1 unless the options ask for mono */
} NGPlatformCaps;
const NGPlatformCaps *ng_platform_caps(void);
```

`fast_cpu` is measured, never guessed from a register. The baseline found
no documented way to detect the AES+ ([docs/platforms/CLASSIC_BASELINE.md §7]).

## 4. Settings on a console

The cartridge's soft DIP table is in ROM on every target. On a console, or
when a system ROM left the soft DIP copy unset (Maiya found this case:
zeros read as one life), `ng_platform_option(n)` returns the table's
default for option n. The first byte of the special list is `$FF` once a
system ROM has filled it in.

A game may show an options page. The SDK gives the page's data (option
names and choices, from the same table) and keeps the choices in the save.
Drawing the page stays the game's job.

## 5. Keeping the ROMs identical

- The layer is new code in `sdk/platform/`, built into `libng_sdk.a` like
  the cabinet modules. A game that doesn't call it doesn't link it.
- `ng_sys_*`, `ng_dip_*` and `ng_save_*` keep their names, signatures and
  code. The layer calls them; they don't call the layer.
- No game is moved onto the layer in P1. Moving one (Maiya first) is a
  later, separate commit, and it will change that game's hash.
- Checks after Step B:
  - helloworld, demo, demo_plus, skylance and maiya P-ROMs byte-identical;
  - zero warnings;
  - `make test` passes;
  - `games/maiya/tests/test_boot_flow.py` passes on euro, us and unibios40,
    for MVS and AES, and with `--eagle`.

## 6. Files

```
sdk/platform/ng_platform.h      the API above, target-neutral
sdk/platform/ng_platform_cart.c the cartridge backends (Step B)
sdk/platform/ng_platform_cd.c   the CD backends (CD1-CD5; a stub that fails the build until then)
Makefile                        TARGET=cart|cd -> NG_TARGET_CART / NG_TARGET_CD; cart by default
```

## 7. Not known, to research before the CD tasks

- How to tell the CD models apart: no register found in [M].
- The language setting on the CD, if the CD BIOS offers one.
- The CD BIOS calls for loading a file and for returning to the system:
  names and addresses to be cited in CD1.
- The AES memory-card flow through `SYS_CARD`: its command bytes (the
  `BIOS_CRDF` function byte at $10FDC4 is in `sdk/macro.h`) to be cited
  from [W: BIOS calls] before Step B writes the AES save backend. Until
  then the AES backend stays session-only, as today.

## 8. Not verified

Nothing is built yet. Everything above is design.

- Classic MVS: the layer is not built.
- Classic AES: memory-card saving not built or tested.
- AES+: no detection exists and none is planned; `fast_cpu` will be measured in G3.
- Neo Geo CD (front, top, CDZ): no backend exists; model detection is unknown.
