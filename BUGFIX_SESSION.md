# Bug Fix Session — May 17 2026

## Problem: Frozen white screen, no sound after rebuild

After applying the FIX layer text fix (Bug 15), a full rebuild produced a game that
booted to a frozen blank white screen with no sound whatsoever.

---

## Root Cause 1 — Z80 NMI race condition in `soundCommand`

**File:** `sdk/neogeolib.c` line 502

**Broken code:**
```c
void NEOGEO_USER soundCommand(uint8_t command) {
    isZ80Ready();
    NEO_REGISTER8(REG_SOUND) = command;
    isZ80Ready();   /* <-- this was the bug */
}
```

The trailing `isZ80Ready()` call after writing the command triggered a Z80 NMI by
writing 0 to `$300001` (REG_DIPSW). The NMI handler clears `$320000` (REG_SOUND) to 0
while processing the previous command. The 68k polling loop then reads 0, re-triggers
another NMI, and the cycle deadlocks permanently.

MAME CPU trace confirmed: the 68k was stuck in an infinite loop at address `0x0234C0`:

```
0234C0: move.b  #$0, $300001.l   ; write 0 to trigger NMI
0234C8: move.b  $320000.l, D0    ; read Z80 status
0234CE: cmpi.b  #$1, D0          ; is Z80 ready?
0234D2: bne     $234c0           ; no -> loop forever
```

**Fix:** Removed the trailing `isZ80Ready()`.

```c
void NEOGEO_USER soundCommand(uint8_t command) {
    isZ80Ready();
    NEO_REGISTER8(REG_SOUND) = command;
}
```

---

## Root Cause 2 — Wrong M1 ROM (changed sound driver)

The M1 ROM in the failing build was assembled from an updated `driver.asm` (2301 lines)
that differed significantly from the last known-working version (1947 lines). The binary
checksums did not match, confirming the driver had been changed during earlier
modifications to the C driver integration path.

**Fix:** Restored the working driver from `neogeosdk2_old`:

```
cp /mnt/c/neogeo/neogeosdk2_old/sound/driver/driver.asm \
   /mnt/c/neogeo/neogeosdk/sound/driver/driver.asm
```

Rebuilt M1 ROM using the pure assembler path (not the C driver path):

```
make GAME=demo m1rom-asm
```

This uses `wla-z80` directly (`USE_Z80C=0`, the default), bypassing the experimental
C driver introduced earlier that had been modified.

---

## Root Cause 3 — FIX layer BRDFIX register not restored (Bug 15, prior fix)

`clearFix()` in `sdk/neogeolib.c` called the BIOS routine `SYS_FIX_CLEAR`, which
resets the `BRDFIX` register (`$3A0003` bit 0) back to 0. This switched the hardware
to the BIOS S ROM, so all text drawn afterward used wrong tile indices and appeared
as blank/black.

**Fix:** Added `ASM_BSETB(#0, REG_BRDFIX)` immediately after the BIOS call to
restore the game S ROM:

```c
void NEOGEO_USER clearFix() {
    ASM_START
    ASM_JSR(SYS_FIX_CLEAR)
    ASM_BSETB(#0, REG_BRDFIX)  /* SYS_FIX_CLEAR resets BRDFIX; restore game S ROM */
    ...
    ASM_END
}
```

Also removed the redundant `setsfix()` call from `ngfix_clear()` in
`sdk/ng_fix/ng_fix.c` since `clearFix()` now handles it internally.

---

## Files changed

| File | Change |
|---|---|
| `sdk/neogeolib.c` | Removed trailing `isZ80Ready()` from `soundCommand` |
| `sdk/neogeolib.c` | Added `ASM_BSETB(#0, REG_BRDFIX)` inside `clearFix()` |
| `sdk/ng_fix/ng_fix.c` | Removed redundant `setsfix()` from `ngfix_clear()` |
| `sound/driver/driver.asm` | Restored to last working version (1947 lines) |

---

## How the freeze was diagnosed

1. Ran MAME with 68k CPU trace enabled (`m68k.trc`)
2. Loaded trace — 99,749 lines ending in the same 4-instruction loop repeated to EOF
3. Traced call chain: `INIT_GAME` → `soundInit` → `soundCommand(1)` → second `isZ80Ready`
4. Compared against old working trace (`working.trc`): the old build exited `isZ80Ready`
   after 1 iteration; the new build never exited
5. Diffed driver.asm line counts: old=1947, new=2301 — driver had been changed
6. MD5 confirmed the two M1 ROM binaries were different
