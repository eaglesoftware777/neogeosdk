    .cpu 68000
    .section .vectors,"a"
    .long 0x0010F300, eagle_cd_reset
    .rept 19
    .long eagle_cd_exception
    .endr
    .long eagle_cd_cd_irq, eagle_cd_cd_irq, eagle_cd_cd_irq
    .long eagle_cd_exception
    .long eagle_cd_irq, eagle_cd_irq, eagle_cd_irq
    .rept 228
    .long eagle_cd_exception
    .endr

    .section .services,"a"
    .word 0x0000
    jmp eagle_cd_reset
    .org 0x38
    jmp eagle_cd_irq
    jmp eagle_cd_irq
    jmp eagle_cd_user_return
    jmp eagle_cd_io_service
    .org 0x140
    jmp eagle_cd_unsupported /* C00540 */
    jmp eagle_cd_unsupported /* C00546 upload is separate from file loading */
    jmp eagle_cd_unsupported /* C0054C */
    jmp eagle_cd_file_service /* C00552 */
    jmp eagle_cd_unsupported /* C00558 */
    jmp eagle_cd_unsupported /* C0055E */
    jmp eagle_cd_file_service /* C00564 */
    jmp eagle_cd_cdda_service /* C0056A */
    .rept 5
    jmp eagle_cd_unsupported
    .endr
    .org 0x300

    .section .identity,"a"
    .ascii "EAGLECD\0"
    .word 1,7,7,CD_MODEL
    .long 15 /* diagnostics, native IPL boot, asset-file loading, direct CDDA */

    .text
    .global eagle_cd_reset, eagle_cd_wait
eagle_cd_reset:
    move.w #0x2700,%sr
    lea 0x1FF000,%sp
    move.b #0,0xFF0183 /* keep Z80 held until a driver has been installed */
    move.b #0,0xFF0181 /* reset transport before drive initialization */
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
    move.b #0x38,0xFF000F
    rte
eagle_cd_io_service:
    movem.l %d0-%d7/%a0-%a6,-(%sp)
    jsr eagle_cd_io
    movem.l (%sp)+,%d0-%d7/%a0-%a6
    rts
eagle_cd_file_service:
    movem.l %d1-%d7/%a0-%a6,-(%sp)
    move.w %sr,-(%sp)
    andi.w #0xF8FF,%sr
    move.l %a0,-(%sp)
    jsr eagle_cd_load_list
    addq.l #4,%sp
    move.w (%sp)+,%sr
    movem.l (%sp)+,%d1-%d7/%a0-%a6
    rts
eagle_cd_cdda_service:
    movem.l %d1-%d7/%a0-%a6,-(%sp)
    move.w %sr,-(%sp)
    andi.w #0xF8FF,%sr
    move.l %d0,-(%sp)
    jsr eagle_cd_cdda
    addq.l #4,%sp
    move.w (%sp)+,%sr
    movem.l (%sp)+,%d1-%d7/%a0-%a6
    rts
    .global eagle_cd_enter_user
eagle_cd_enter_user:
    move.w #0x2700,%sr
    lea 0x10F300,%sp
    move.b #0xFF,0xFF016D
    jmp 0x000122
eagle_cd_user_return:
    move.w #0x2700,%sr
    lea 0x1FF000,%sp
    move.b #0,0xFF016D
    jsr eagle_cd_next_user
    bra eagle_cd_reset
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
