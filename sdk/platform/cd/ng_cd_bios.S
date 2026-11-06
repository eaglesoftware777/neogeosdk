    .cpu 68000
    .text
    .global ng_cd_bios_load,ng_cd_bios_cdda
/* Preserve all callee-saved registers even if a BIOS revision clobbers more
 * than expected. Raw D0 is returned; its interpretation is experimental
 * until a stock-firmware conformance trace is captured. */
ng_cd_bios_load:
    movem.l %d2-%d7/%a2-%a6,-(%sp)
    movea.l 48(%sp),%a0
    jsr 0xC00564
    movem.l (%sp)+,%d2-%d7/%a2-%a6
    rts
ng_cd_bios_cdda:
    movem.l %d2-%d7/%a2-%a6,-(%sp)
    move.l 48(%sp),%d0
    jsr 0xC0056A
    movem.l (%sp)+,%d2-%d7/%a2-%a6
    rts
