    .cpu 68000
    .section .text
    .long 0x10F300, 0xC00402
    .rept 23
    .long 0xC00426
    .endr
    .long vblank, 0xC0043E, irq3
    .rept 36
    .long 0xC00426
    .endr
    .org 0x100
    .ascii "NEO-GEO\0"
    .word 0x077F
    .long 0x100000, 0x100100
    .word 0
    .byte LOGOFLAG,0
    .long dips,dips,dips
    .org 0x122
    .word 0x4EF9
    .long user
    .word 0x4EF9
    .long player_start
    .word 0x4EF9
    .long demo_end
    .word 0x4EF9
    .long coin_sound
    .org 0x200
user:
    move.w %sr, 0x100010
    moveq #0,%d0
    move.b 0x10FDAE,%d0
    moveq #1,%d1
    lsl.w %d0,%d1
    or.w %d1,0x100004
    tst.b %d0
    beq done
    cmpi.b #1,%d0
    beq done
    move.l #0xDEADBEEF,0x100000
    move.b #1,0x10FDAF
    move.b #0x80,0x10FD80
    jsr 0xC004C2
    jsr 0xC004C8
    move.w #0x7FFF,0x400002
    move.l #message,0x10FF00
    move.l #0x10FF04,0x10FDBE
    jsr 0xC004CE
    lea 0x10FF00,%a0
    clr.l (%a0)+
    move.w #0x0301,(%a0)+
    move.w #2,(%a0)+
    move.w #0x2002,(%a0)+
    move.w #0x0003,(%a0)+
    move.w #0x7318,(%a0)+
    move.w #0x0004,(%a0)+
    move.l #inline_text,(%a0)+
    clr.w (%a0)+
    move.l %a0,0x10FDBE
    jsr 0xC004CE
    move.w #0x2000,%sr
idle:
    move.b %d0,0x300001
    bra idle
done:
    jmp 0xC00444
vblank:
    movem.l %d0-%d7/%a0-%a6,-(%sp)
    move.w #4,0x3C000C
    move.b %d0,0x300001
    addq.l #1,0x10000C
    jsr 0xC0044A
    movem.l (%sp)+,%d0-%d7/%a0-%a6
    rte
irq3:
    move.w #1,0x3C000C
    rte
player_start:
    addq.w #1,0x100006
    move.b #1,0x10FDB6
    move.b #2,0x10FDAF
    rts
coin_sound:
    addq.w #1,0x100008
    rts
demo_end:
    addq.w #1,0x10000A
    rts
dips:
    .ascii "EAGLE BIOS PROBE"
    .byte 0
    .rept 16
    .byte 0
    .endr
    .balign 2
message:
    .word 1,0x00FF,0x2002,3,0x720E,4
    .long text
    .word 3,0x7240,0x0102,0x040D,0x00FE
    .word 0
text:
    .ascii "EAGLE BIOS READY"
    .byte 255
    .balign 2
inline_text:
    .word 0x104F, 0x104B
