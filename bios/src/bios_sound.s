; EagleBIOS system audio: short SSG cue and a ROM-independent handoff.
.memorymap
defaultslot 0
slotsize $10000
slot 0 $0000
.endme
.rombankmap
bankstotal 1
banksize $10000
banks 1
.endro
.bank 0 slot 0
.org $0000
    di
    jp init
.org $0038
    reti
.org $0066
    push af
    in a,($00)
    ld ($F800),a
    xor a
    out ($00),a
    out ($0C),a
    pop af
    retn
.org $00D0
init:
    di
    ld sp,$FFFC
    im 1
    xor a
    out ($18),a
    out ($00),a
    ld ($F800),a
    call mute
    xor a
    out ($08),a
ready:
    ld a,1
    out ($0C),a
loop:
    ld a,($F800)
    or a
    jr z,loop
    ld b,a
    xor a
    ld ($F800),a
    ld a,b
    cp 1
    jr z,park
    cp 2
    jp z,chime
    call mute
    jr ready

; A command handler in the cartridge may merely queue RESET and return.
; Polling from RAM works with that driver as well as reset-in-NMI drivers.
park:
    di
    xor a
    out ($18),a
    call mute
    ld hl,ram_start
    ld de,$F880
    ld bc,ram_end-ram_start
    ldir
    jp $F880
ram_start:
    ld a,1
    out ($0C),a
ram_poll:
    in a,($00)
    cp 3
    jr nz,ram_poll
    xor a
    out ($00),a
    jp $0000
ram_end:

wait_ym:
    in a,($04)
    and $80
    jr nz,wait_ym
    ret
write_a:
    push af
    call wait_ym
    pop af
    out ($04),a
    call wait_ym
    ld a,c
    out ($05),a
    ret
write_b:
    push af
    call wait_ym
    pop af
    out ($06),a
    call wait_ym
    ld a,c
    out ($07),a
    ret
mute:
    ld a,$27
    ld c,$30
    call write_a
    ld a,$22
    ld c,0
    call write_a
    ld a,$07
    ld c,$3F
    call write_a
    ld a,$08
    ld c,0
    call write_a
    ld a,$09
    call write_a
    ld a,$0A
    call write_a
    ld a,$28
    ld c,1
    call write_a
    ld a,$28
    ld c,2
    call write_a
    ld a,$28
    ld c,5
    call write_a
    ld a,$28
    ld c,6
    call write_a
    ld a,$10
    ld c,1
    call write_a
    ld a,0
    ld c,$BF
    call write_b
    ret
chime:
    ld a,0
    ld c,$ED
    call write_a
    ld a,1
    ld c,0
    call write_a
    ld a,7
    ld c,$3E
    call write_a
    ld a,8
    ld c,9
    call write_a
    ld hl,$7000
chime_wait:
    dec hl
    ld a,($F800)
    or a
    jr nz,chime_end
    ld a,h
    or l
    jr nz,chime_wait
chime_end:
    call mute
    jp ready
