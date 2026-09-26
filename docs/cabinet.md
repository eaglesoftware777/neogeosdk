# Cabinet: Which Machine Is This?

## What It Does

`sdk/cabinet/ng_sys.h` tells a game, at run time, what it is running on:

```c
int ng_sys_is_mvs(void);   /* 1 = arcade board (MVS), 0 = console (AES) */
int ng_sys_region(void);   /* NG_REGION_JP, NG_REGION_US or NG_REGION_EU */
```

Both read what the system ROM leaves in its RAM before it starts the game:
`BIOS_MVS_FLAG` ($10FD82: 0 on a console, 0x80 on an arcade board) and
`BIOS_COUNTRY_CODE` ($10FD83: 0 Japan, 1 USA, 2 Europe). The addresses,
and where they are documented, are in `sdk/macro.h`. Any other country
code reads as `NG_REGION_US`; neither call can fail.

`sdk/ng_system.h` (included by `neogeo.h`) pulls this header in, and keeps
`NG_REGION_JAPAN`, `NG_REGION_USA` and `NG_REGION_EUROPE` as other names
for the three codes.

## Why Ask at Run Time

A build flag (`NG_AES`) picks the cartridge header, but not the machine:
the same ROM can go into either, and the UniBIOS can switch a board between
arcade and console behaviour. Anything that depends on coins — credits,
"INSERT COIN", a countdown on a credited title, paying to continue — should
ask `ng_sys_is_mvs()`. Maiya does: on an arcade board a coin and Start
begin or continue a game; on a console Start alone does, and the continue
card offers CONTINUE / EXIT.

## Only What a Game Uses

The files in `sdk/cabinet/` are built into `out/libng_sdk.a` (with the
engine's other on-demand modules, such as `ng_trig`), which is linked last.
The linker takes a module from it only if the game calls it, so a game that
never asks carries none of this code.

## Tested

In MAME, on the arcade driver (`neogeo`) with the Europe, USA, Japan and
Asia system ROMs and on the console driver (`aes`), the two calls report
the machine and the region the system ROM was set for.
