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

## Taking a Credit From Game Code

```c
void ng_sys_credit_check(void);   /* SYS_CREDIT_CHECK, registers kept */
void ng_sys_credit_down(void);    /* SYS_CREDIT_DOWN, registers kept */
```

The system ROM's two credit routines change registers that compiled C
expects a call to keep (D2, D4 and A3 to A5; the check clears D2 as it
counts). Called through `CALLNEOGEOF` from game code, a value the compiler
kept in D2 comes back wrong, and a function pointer kept in A4 sends the
CPU into backup RAM, which resets the board. These two save those
registers around the call.

Set the decrements first: one BCD byte per player from `BIOS_CREDIT_DEC`
($10FDB0). The check clears the ones a player cannot pay for; the down call
takes the rest. The system ROM decides where a player's credits are kept:
the US arcade one keeps player 2's apart (at $D00035, his own coin chute),
the others count one pool. So ask the check rather than reading
`P1_CREDITS` yourself:

```c
volatile uint8_t *dec = (volatile uint8_t *)BIOS_CREDIT_DEC;
dec[0] = 0; dec[1] = 1; dec[2] = 0; dec[3] = 0;   /* player 2, one credit */
ng_sys_credit_check();
if (dec[1]) ng_sys_credit_down();                 /* he could pay: take it */
```

Do it with interrupts masked, so the system's own Start handling cannot
run between the check and the take. From the cartridge's `PLAYER_START`
the system has set the decrements already. `ng_join` uses these calls,
and so does Century XI's versus screen. Arcade board only.

## Only What a Game Uses

The files in `sdk/cabinet/` are built into `out/libng_sdk.a` (with the
engine's other on-demand modules, such as `ng_trig`), which is linked last.
The linker takes a module from it only if the game calls it, so a game that
never asks carries none of this code.

## Tested

In MAME, on the arcade driver (`neogeo`) with the Europe, USA, Japan and
Asia system ROMs and on the console driver (`aes`), the two calls report
the machine and the region the system ROM was set for.

The credit calls: in MAME, Century XI's `tests/test_boot_flow.py` takes
exactly one credit for a second player joining, from the shared count on the
Europe, Japan and Asia system ROMs and from his own on the US one and on
EagleBIOS, with no reset; a paid join in the middle of a match is accepted.
