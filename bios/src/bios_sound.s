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
    jp z,fanfare
    cp 4
    jp z,tick
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
;------------------------------------------------------------------
; The Eagle fanfare (code 2): three rising notes, the top note held,
; then a short answer back on the top.  SSG channel A carries the tune
; and channel B a root under it; each note fades in three steps so it
; rings instead of buzzing.  Code 4 is one short tick of the top note.
;
; Note table: duration in 10 ms units, period A (lo, hi), period B
; (lo, hi), volume A, volume B.  Duration 0 ends the tune.
;------------------------------------------------------------------
fanfare_notes:
    .db 9,  $EF,$00, $DE,$01, 12, 6     ; C5 over C4
    .db 9,  $BE,$00, $DE,$01, 12, 6     ; E5 over C4
    .db 9,  $9F,$00, $7B,$01, 13, 6     ; G5 over E4
    .db 26, $77,$00, $3F,$01, 14, 7     ; C6 over G4, held
    .db 5,  $77,$00, $3F,$01, 0,  0     ; breath
    .db 10, $9F,$00, $7B,$01, 12, 5     ; G5 over E4
    .db 34, $77,$00, $DE,$01, 14, 7     ; C6 over C4, held
    .db 0
tick_notes:
    .db 4,  $77,$00, $77,$00, 10, 0     ; C6, short
    .db 0

tick:
    ld hl,tick_notes
    jr play_notes
fanfare:
    ld hl,fanfare_notes
play_notes:
    call mute
    ld a,7                  ; mixer: tones A and B on, C and all noise off
    ld c,$3C
    call write_a
notes_loop:
    ld a,(hl)
    or a
    jp z,notes_end
    ld d,a                  ; d = duration
    inc hl
    ld a,0                  ; R0/R1: channel A period
    ld c,(hl)
    call write_a
    inc hl
    ld a,1
    ld c,(hl)
    call write_a
    inc hl
    ld a,2                  ; R2/R3: channel B period
    ld c,(hl)
    call write_a
    inc hl
    ld a,3
    ld c,(hl)
    call write_a
    inc hl
    ld e,(hl)               ; e = volume A
    inc hl
    ld b,(hl)               ; b = volume B
    inc hl
    push hl
    ; first half at full volume
    call set_volumes
    ld a,d
    srl a
    call wait_units
    jr nz,notes_abort
    ; second quarter, both channels three steps softer
    call soften
    ld a,d
    srl a
    srl a
    call wait_units
    jr nz,notes_abort
    ; last quarter, three steps softer again
    call soften
    ld a,d
    srl a
    srl a
    call wait_units
    jr nz,notes_abort
    pop hl
    jr notes_loop
notes_abort:
    pop hl
notes_end:
    call mute
    jp ready

; e = volume A, b = volume B
set_volumes:
    push bc
    ld a,8
    ld c,e
    call write_a
    pop bc
    ld a,9
    ld c,b
    call write_a
    ret

soften:
    ld a,e
    sub 3
    jr nc,soften_a
    xor a
soften_a:
    ld e,a
    ld a,b
    sub 3
    jr nc,soften_b
    xor a
soften_b:
    ld b,a
    jp set_volumes

; a = number of 10 ms units to wait (about 1670 turns of the inner loop each).
; Returns with Z set when the time passed, NZ when a command arrived.
wait_units:
    or a
    ret z
wait_outer:
    push af
    push bc
    ld bc,1670
wait_inner:
    dec bc
    ld a,b
    or c
    jr nz,wait_inner
    pop bc
    pop af
    ld b,a
    ld a,($F800)            ; a new command cuts the tune short
    or a
    ret nz
    ld a,b
    dec a
    jr nz,wait_outer
    ret
