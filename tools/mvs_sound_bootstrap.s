/* Release-specific adapter. Addresses are verified by the Python patcher. */
    .text
    .equ parameter_pending, 0x10ef00
    .global bootstrap_init, bootstrap_command
bootstrap_init:
    clr.b parameter_pending
    move.b #9,0x320000
    move.w #511,%d0
1:  move.b #0,0x300001
    dbra %d0,1b
    jsr sound_ready
    rts

    .org 0x40
bootstrap_command:
    movem.l %d2-%d3/%a2,-(%sp)
    moveq #0,%d2
    move.b 19(%sp),%d2
    tst.b parameter_pending
    beq new_command
    clr.b parameter_pending
    cmpi.b #1,%d2
    beq escaped
    cmpi.b #2,%d2
    beq escaped
    cmpi.b #3,%d2
    beq escaped
    cmpi.b #9,%d2
    beq escaped
    cmpi.b #255,%d2
    bne send
escaped:
    pea 255
    bsr raw_send
    addq.l #4,%sp
    eori.b #128,%d2
    bra send
new_command:
    lea parameter_commands(%pc),%a2
    moveq #19,%d3
2:  cmp.b (%a2)+,%d2
    beq expects_parameter
    dbra %d3,2b
    bra send
expects_parameter:
    move.b #1,parameter_pending
send:
    move.l %d2,-(%sp)
    bsr raw_send
    addq.l #4,%sp
    movem.l (%sp)+,%d2-%d3/%a2
    rts
raw_send:
    subq.l #4,%sp
    jsr sound_ready
    jmp sound_command_tail
parameter_commands:
    .byte 0x05,0x06,0x07,0x0a,0x0e,0x12,0x13,0x14,0x15,0x16
    .byte 0x17,0x18,0x19,0x1a,0x1b,0x1d,0x1e,0x1f,0x31,0x32
