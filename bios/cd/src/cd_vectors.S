    .cpu 68000
    .section .vectors,"a"
    .long 0x0010F300, eagle_cd_reset
    .rept 19
    .long eagle_cd_exception
    .endr
    .long eagle_cd_cd_irq, eagle_cd_cd_irq
    .rept 2
    .long eagle_cd_exception
    .endr
    .long eagle_cd_irq, eagle_cd_irq, eagle_cd_irq
    .rept 228
    .long eagle_cd_exception
    .endr

    .section .services,"a"
    .word 0x0000
    jmp eagle_cd_reset
    .org 0x38
    jmp 0xC0A518
    jmp 0xC0A4FC
    jmp 0xC0A92C
    jmp 0xC0ADA0
    .org 0x140
    jmp 0xC0B9EE
    jmp 0xC09C36
    jmp 0xC0D850
    jmp 0xC0DB60
    jmp 0xC09122
    jmp 0xC0AD12
    jmp 0xC0DB6A
    jmp 0xC0B6DE
    jmp 0xC08E84
    jmp 0xC0C6D6
    jmp 0xC0D48A
    jmp 0xC0A822
    jmp 0xC08DB0
    .org 0x300

    .section .identity,"a"
    .ascii "EAGLECD\0"
    .word 1,7,7,CD_MODEL
    .long 1 /* independent diagnostics capability only; no disc claim */

    .text
    .global eagle_cd_reset, eagle_cd_wait
eagle_cd_reset:
    move.w #0x2700,%sr
    lea 0x1FF000,%sp
    move.b #0,0xFF0183 /* keep Z80 held until a driver has been installed */
    move.b #0,0xFF0181 /* no optical commands at the hello-screen gate */
    lea __data_load,%a0
    lea __data_start,%a1
    lea __data_end,%a2
1:  cmpa.l %a2,%a1
    bcc.s 2f
    move.b (%a0)+,(%a1)+
    bra.s 1b
2:  lea __bss_start,%a0
    lea __bss_end,%a1
3:  cmpa.l %a1,%a0
    bcc.s 4f
    clr.b (%a0)+
    bra.s 3b
4:  jsr eagle_cd_boot
    bra eagle_cd_reset

eagle_cd_irq:
    movem.l %d0-%d7/%a0-%a6,-(%sp)
    move.w #7,0x3C000C
    move.b #0,0x300001
    jsr eagle_cd_vblank
    movem.l (%sp)+,%d0-%d7/%a0-%a6
    rte
eagle_cd_cd_irq:
    move.b #0x30,0xFF000F
    rte
eagle_cd_io_service:
    movem.l %d0-%d7/%a0-%a6,-(%sp)
    jsr eagle_cd_io
    movem.l (%sp)+,%d0-%d7/%a0-%a6
    rts
eagle_cd_unsupported:
    moveq #-95,%d0
    rts
eagle_cd_wait:
    stop #0x2000
    rts
eagle_cd_exception:
    move.w #0x2700,%sr
    move.w #0x8F00,0x401FFE
5:  move.b #0,0x300001
    bra.s 5b
