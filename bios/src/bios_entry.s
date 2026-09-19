/*
 * ============================================================================
 *  EagleBIOS Entry Points, Vector Table & System Jump Table
 *  Target: Motorola 68000 (Big Endian)
 * ============================================================================
 */

    .cpu 68000
    .text

/* -------------------------------------------------------------------------
 *  Section 1: Exception Vector Table (0xC00000 .. 0xC003FF, 256 entries)
 * ------------------------------------------------------------------------- */
    .section .vectors, "a"
    .global vectors_start
vectors_start:
    .long   0x0010F300          /* 0x000: Initial Supervisor Stack Pointer   */
    .long   0x00C00402          /* 0x004: Initial PC -> Reset Entry          */
    .long   0x00C00408          /* 0x008: Bus Error                          */
    .long   0x00C0040E          /* 0x00C: Address Error                      */
    .long   0x00C00414          /* 0x010: Illegal Instruction                */
    .long   0x00C00426          /* 0x014: Zero Divide                        */
    .long   0x00C00426          /* 0x018: CHK Instruction                   */
    .long   0x00C00426          /* 0x01C: TRAPV Instruction                 */
    .long   0x00C0041A          /* 0x020: Privilege Violation                */
    .long   0x00C00420          /* 0x024: Trace                              */
    .long   0x00C00426          /* 0x028: Line 1010 Emulator                 */
    .long   0x00C00426          /* 0x02C: Line 1111 Emulator                 */
    .long   0x00C00426          /* 0x030: Reserved                           */
    .long   0x00C00426          /* 0x034: Reserved                           */
    .long   0x00C00426          /* 0x038: Reserved                           */
    .long   0x00C0042C          /* 0x03C: Uninitialized Interrupt            */
    .long   0x00C00426          /* 0x040: Reserved                           */
    .long   0x00C00426          /* 0x044: Reserved                           */
    .long   0x00C00426          /* 0x048: Reserved                           */
    .long   0x00C00426          /* 0x04C: Reserved                           */
    .long   0x00C00426          /* 0x050: Reserved                           */
    .long   0x00C00426          /* 0x054: Reserved                           */
    .long   0x00C00426          /* 0x058: Reserved                           */
    .long   0x00C00426          /* 0x05C: Reserved                           */
    .long   0x00C00432          /* 0x060: Spurious Interrupt                 */
    .long   0x00C00438          /* 0x064: Level 1 IRQ (VBlank) -> SYS_INT1   */
    .long   0x00C0043E          /* 0x068: Level 2 IRQ (Timer/HBlank)         */
    .long   irq3_wrap           /* 0x06C: Power-on interrupt                 */
    .long   0x00C00426          /* 0x070: Level 4 IRQ                        */
    .long   0x00C00426          /* 0x074: Level 5 IRQ                        */
    .long   0x00C00426          /* 0x078: Level 6 IRQ                        */
    .long   0x00C00426          /* 0x07C: Level 7 IRQ                        */

    /* Vectors 32 .. 255: TRAP vectors and user vectors filled with trap handler */
    .rept   224
    .long   0x00C00426
    .endr

/* -------------------------------------------------------------------------
 *  Section 2: BIOS Jump Table (Fixed addresses 0xC00400 .. 0xC004DF)
 *  Each entry is exactly 6 bytes: jmp <target> (0x4EF9 0x00C0xxxx)
 * ------------------------------------------------------------------------- */
    .section .jump_table, "a"
    .global jump_table_start
jump_table_start:
    .word   BIOS_ID             /* 0xC00400: Platform and region             */
    jmp     bios_reset          /* 0xC00402: Cold / Warm Boot Reset          */
    jmp     bios_bus_err_wrap   /* 0xC00408: Bus Error Handler               */
    jmp     bios_addr_err_wrap  /* 0xC0040E: Address Error Handler           */
    jmp     bios_illegal        /* 0xC00414: Illegal Instruction Handler     */
    jmp     bios_privilege      /* 0xC0041A: Privilege Violation Handler     */
    jmp     bios_trace          /* 0xC00420: Trace Exception Handler         */
    jmp     bios_trap           /* 0xC00426: Generic Trap / Reserved Handler */
    jmp     bios_uninit         /* 0xC0042C: Uninitialized Interrupt Handler */
    jmp     bios_spurious       /* 0xC00432: Spurious Interrupt Handler      */
    jmp     sys_int1_wrap       /* 0xC00438: SYS_INT1 (VBlank housekeeping)  */
    jmp     sys_int2_wrap       /* 0xC0043E: SYS_INT2 (Timer / HBlank)       */
    jmp     sys_return_wrap     /* 0xC00444: SYS_RETURN (Game dispatcher)    */
    jmp     sys_io_wrap         /* 0xC0044A: SYS_IO (Controller & coin read) */
    jmp     sys_credit_check_wrap /* 0xC00450: SYS_CREDIT_CHECK              */
    jmp     sys_credit_down_wrap  /* 0xC00456: SYS_CREDIT_DOWN               */
    jmp     sys_read_calendar_wrap/* 0xC0045C: SYS_READ_CALENDAR             */
    jmp     sys_set_calendar_wrap /* 0xC00462: SYS_SET_CALENDAR              */
    jmp     sys_card_wrap       /* 0xC00468: SYS_CARD                        */
    jmp     sys_card_error_wrap /* 0xC0046E: SYS_CARD_ERROR                  */
    jmp     sys_howtoplay_wrap  /* 0xC00474: SYS_HOWTOPLAY                   */
    jmp     sys_stub_rts        /* 0xC0047A: Reserved                        */
    jmp     sys_stub_rts        /* 0xC00480: Reserved                        */
    jmp     sys_stub_rts        /* 0xC00486: Reserved                        */
    jmp     sys_stub_rts        /* 0xC0048C: Reserved                        */
    jmp     sys_stub_rts        /* 0xC00492: Reserved                        */
    jmp     sys_stub_rts        /* 0xC00498: Reserved                        */
    jmp     sys_stub_rts        /* 0xC0049E: Reserved                        */
    jmp     sys_stub_rts        /* 0xC004A4: Reserved                        */
    jmp     sys_stub_rts        /* 0xC004AA: Reserved                        */
    jmp     sys_stub_rts        /* 0xC004B0: Reserved                        */
    jmp     sys_stub_rts        /* 0xC004B6: Reserved                        */
    jmp     sys_stub_rts        /* 0xC004BC: Reserved                        */
    jmp     sys_fix_clear_wrap  /* 0xC004C2: SYS_FIX_CLEAR                   */
    jmp     sys_lsp_1st_wrap    /* 0xC004C8: SYS_LSP_1ST                     */
    jmp     sys_mess_out_wrap   /* 0xC004CE: SYS_MESS_OUT                    */
    jmp     controller_wrap     /* 0xC004D4: Controller setup                */
    jmp     sys_int2_wrap       /* 0xC004DA: Timer interrupt                 */

/* -------------------------------------------------------------------------
 *  Section 3: BIOS Identification Header (0xC004E0)
 * ------------------------------------------------------------------------- */
    .section .header, "a"
    .ascii  "EAGLE OPEN BIOS 1.0 (C) EAGLE SOFTWARE "

/* -------------------------------------------------------------------------
 *  Section 4: Assembly Wrapper Routines
 * ------------------------------------------------------------------------- */
    .section .text

.global bios_addr_err_wrap
bios_addr_err_wrap:
    move.l  10(%sp), %d0        /* Fault PC */
    move.l  2(%sp), %d1         /* Fault Address */
    clr.l   %d2
    move.w  6(%sp), %d2         /* Instruction Register */
    move.l  %sp, %a0            /* Fault SP */
    movea.l #0x0010F300, %sp    /* Clean supervisor stack */
    move.l  %a0, -(%sp)
    move.l  %d2, -(%sp)
    move.l  %d1, -(%sp)
    move.l  %d0, -(%sp)
    jsr     bios_addr_err_c

.global bios_bus_err_wrap
bios_bus_err_wrap:
    move.l  10(%sp), %d0        /* Fault PC */
    move.l  2(%sp), %d1         /* Fault Address */
    clr.l   %d2
    move.w  6(%sp), %d2         /* Instruction Register */
    move.l  %sp, %a0            /* Fault SP */
    movea.l #0x0010F300, %sp    /* Clean supervisor stack */
    move.l  %a0, -(%sp)
    move.l  %d2, -(%sp)
    move.l  %d1, -(%sp)
    move.l  %d0, -(%sp)
    jsr     bios_bus_err_c

sys_stub_rts:
    rts

    .global bios_call_cart
bios_call_cart:
    movem.l %d0-%d7/%a0-%a6, -(%sp)
    movea.l 64(%sp), %a0
    jsr     (%a0)
    movem.l (%sp)+, %d0-%d7/%a0-%a6
    rts

irq3_wrap:
    move.w  #1, 0x3C000C
    rte

controller_wrap:
    movem.l %d0-%d7/%a0-%a6, -(%sp)
    jsr     bios_controller_setup
    movem.l (%sp)+, %d0-%d7/%a0-%a6
    rts

sys_int1_wrap:
    movem.l %d0-%d7/%a0-%a6, -(%sp)
    jsr     sys_int1_c
    movem.l (%sp)+, %d0-%d7/%a0-%a6
    rte

sys_int2_wrap:
    movem.l %d0-%d7/%a0-%a6, -(%sp)
    jsr     sys_int2_c
    movem.l (%sp)+, %d0-%d7/%a0-%a6
    rte

sys_return_wrap:
    move.w  #0x2700, %sr
    /* 1. Restore BIOS vector table and unprotect backup RAM (keep CRTFIX/M1 active) */
    move.b  %d0, 0x3A0003       /* REG_SWPBIOS: map BIOS vectors at 0x00..0x7F */

    /* 2. Restore BIOS supervisor stack pointer */
    movea.l #0x0010F300, %sp

    /* 3. Call C return dispatcher */
    jmp     sys_return_c

.global call_cart_user_asm
call_cart_user_asm:
    /* Cartridge graphics and sound have already been selected safely. */
    move.w  #0x2700, %sr
    move.b  %d0, 0x3A0013       /* REG_SWPROM: map cartridge vectors at 0x00..0x7F */
    move.b  %d0, 0x3A000D       /* REG_SRAMLOCK: write-protect backup RAM */

    /* 2. Reset supervisor stack to clean top */
    movea.l #0x0010F300, %sp

    /* 3. Fully zero-extend D0 with BIOS_USER_REQUEST (0..3) */
    clr.l   %d0
    move.b  0x10FDAE, %d0

    /* 4. Clear all other registers to 0 so cart has clean environment */
    clr.l   %d1
    clr.l   %d2
    clr.l   %d3
    clr.l   %d4
    clr.l   %d5
    clr.l   %d6
    clr.l   %d7
    suba.l  %a0, %a0
    suba.l  %a1, %a1
    suba.l  %a2, %a2
    suba.l  %a3, %a3
    suba.l  %a4, %a4
    suba.l  %a5, %a5
    suba.l  %a6, %a6

    /* USER enters with interrupts masked; the game installs its VBlank path. */
    move.w  #0x2700, %sr

    /* 6. Jump directly to Cartridge USER dispatcher */
    jmp     0x000122

sys_io_wrap:
    movem.l %d1-%d7/%a0-%a6, -(%sp)
    jsr     sys_io_c
    movem.l (%sp)+, %d1-%d7/%a0-%a6
    rts

sys_credit_check_wrap:
    movem.l %d1-%d7/%a0-%a6, -(%sp)
    jsr     sys_credit_check_c
    /* D0 holds remaining credits; set CCR carry if insufficient credits */
    tst.l   %d0
    beq.s   .no_credits
    andi.b  #0xFE, %ccr         /* Clear carry flag (success) */
    movem.l (%sp)+, %d1-%d7/%a0-%a6
    rts
.no_credits:
    ori.b   #0x01, %ccr         /* Set carry flag (error) */
    movem.l (%sp)+, %d1-%d7/%a0-%a6
    rts

sys_credit_down_wrap:
    movem.l %d0-%d7/%a0-%a6, -(%sp)
    jsr     sys_credit_down_c
    movem.l (%sp)+, %d0-%d7/%a0-%a6
    rts

sys_read_calendar_wrap:
    movem.l %d0-%d7/%a0-%a6, -(%sp)
    jsr     sys_read_calendar_c
    movem.l (%sp)+, %d0-%d7/%a0-%a6
    rts

sys_set_calendar_wrap:
    movem.l %d0-%d7/%a0-%a6, -(%sp)
    jsr     sys_set_calendar_c
    movem.l (%sp)+, %d0-%d7/%a0-%a6
    rts

sys_card_wrap:
    movem.l %d1-%d7/%a0-%a6, -(%sp)
    jsr     sys_card_c
    movem.l (%sp)+, %d1-%d7/%a0-%a6
    rts

sys_card_error_wrap:
    movem.l %d0-%d7/%a0-%a6, -(%sp)
    jsr     sys_card_error_c
    movem.l (%sp)+, %d0-%d7/%a0-%a6
    rts

sys_howtoplay_wrap:
    movem.l %d0-%d7/%a0-%a6, -(%sp)
    jsr     sys_howtoplay_c
    movem.l (%sp)+, %d0-%d7/%a0-%a6
    rts

sys_fix_clear_wrap:
    movem.l %d0-%d7/%a0-%a6, -(%sp)
    jsr     sys_fix_clear_c
    movem.l (%sp)+, %d0-%d7/%a0-%a6
    rts

sys_lsp_1st_wrap:
    movem.l %d0-%d7/%a0-%a6, -(%sp)
    jsr     sys_lsp_1st_c
    movem.l (%sp)+, %d0-%d7/%a0-%a6
    rts

sys_mess_out_wrap:
    movem.l %d0-%d7/%a0-%a6, -(%sp)
    jsr     sys_mess_out_c
    movem.l (%sp)+, %d0-%d7/%a0-%a6
    rts
