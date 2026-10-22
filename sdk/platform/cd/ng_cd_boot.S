    .cpu 68000
    .section .cd_header,"a"
    .long 0x0010F300,0x00C00402
    .rept 23
    .long ng_cd_exception
    .endr
    .long ng_cd_irq_entry,ng_cd_irq_entry,ng_cd_irq_entry
    .rept 36
    .long ng_cd_exception
    .endr
    .ascii "NEO-GEO"
    .byte 2
    .word NG_CD_GAME_ID
    .long 0x00080000,0x00100000
    .word 0
    .byte 2,0
    .long 0,0,0
    .global ng_cd_user_entry
ng_cd_user_entry:
    jmp ng_cd_entry
    jmp ng_cd_callback_return
    jmp ng_cd_callback_return
    jmp ng_cd_callback_return
    .word 0
    .org 0x200
    .text
ng_cd_entry:
    cmpi.b #2,0x10FDAE
    bcs ng_cd_return
    move.w #0x2700,%sr
    lea 0x10F000,%sp
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
4:  move.l 0xC00054,0x54 /* retain the firmware's live CD interrupt handlers */
    move.l 0xC00058,0x58
    move.l 0xC0005C,0x5C
    move.b #0xFF,0xFF016D /* CD-native RAM vectors, not cartridge latch */
    move.w #7,0x3C000C
    jsr ng_cd_start
ng_cd_return:
    jmp 0xC00444
ng_cd_callback_return:
    rts
    .global ng_cd_wait_vblank
ng_cd_wait_vblank:
    stop #0x2000
    rts
ng_cd_irq_entry:
    movem.l %d0-%d7/%a0-%a6,-(%sp)
    move.w #4,0x3C000C
    move.b #0,0x300001
#ifdef NG_CD_SKYLANCE
    btst #7,0x10FD80
    beq.s 6f
    jsr sky_palette_vblank
6:
#endif
#ifdef NG_CD_MAIYA
    btst #7,0x10FD80
    beq.s 7f
    jsr maiya_video_vblank
7:
#endif
    jsr 0xC0044A
    jsr ng_cd_irq
    movem.l (%sp)+,%d0-%d7/%a0-%a6
    jmp 0xC00438 /* firmware owns interrupt completion and USER scheduling */
ng_cd_exception:
    move.w #0x2700,%sr
    move.w #0x8F00,0x401FFE
5:  move.b #0,0x300001
    bra.s 5b
