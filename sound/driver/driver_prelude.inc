reset:
    di
    jp main

;;; RST Vectors for optimized calls
.org $0008 ; wait for YM2610 not busy
ym_wait_ready:
    in a,($04)
    bit 7,a
    jr nz,ym_wait_ready
    ret

.org $0010 ; writeport_a (Shadowed)
    jp shadowed_write_a

.org $0018 ; writeport_b (Shadowed)
    jp shadowed_write_b

.org $0038 ; IRQ (Tempo Ticker)
    di
    push af
    push bc
    push de
    push hl
    in a,($06)
    in a,($04)
    ; Reset Timer B flag while keeping Load TB + Enable TB IRQ asserted.
    ; Writing 0 to bit 1 (Load TB) would zero the counter and stop the timer.
    ld de,$272A
    call force_write_a
    call ticker_update
    pop hl
    pop de
    pop bc
    pop af
    ei
    reti

.org $0066 ; NMI (Command Input)
    push af
    push bc
    push hl
    in a,($00)
    ld b,a
    xor a
    out ($0C),a
    ld a,(FIFO_WRITE)
    ld l,a
    ld h,>FIFO_BUF
    ld (hl),b
    inc a
    and $1F
    ld (FIFO_WRITE),a
    xor a
    out ($00),a
    ld a,READY_VALUE
    out ($0C),a
    pop hl
    pop bc
    pop af
    retn

.org $00D0
