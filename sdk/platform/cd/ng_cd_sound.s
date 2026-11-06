; Original minimal CD RAM driver: one ADPCM-A diagnostic cue, no ADPCM-B.
.MEMORYMAP
  DEFAULTSLOT 0
  SLOTSIZE $10000
  SLOT 0 $0000
.ENDME
.ROMBANKMAP
  BANKSTOTAL 1
  BANKSIZE $10000
  BANKS 1
.ENDRO
.BANK 0 SLOT 0
.ORG 0
    di
    ld sp,$FF00
    jp init
.ORG $38
    ei
    reti
.ORG $66
    push af
    in a,($00)
    cp $40
    jr nz,done
    xor a
    out ($06),a
    nop
    nop
    ld a,$81
    out ($07),a
    nop
    nop
    xor a
    out ($06),a
    nop
    nop
    inc a
    out ($07),a
done:
    ld a,1
    out ($0C),a
    pop af
    retn
.ORG $100
init:
    ld hl,registers
    ld b,7
setup:
    ld a,(hl)
    inc hl
    out ($06),a
    nop
    nop
    nop
    nop
    ld a,(hl)
    inc hl
    out ($07),a
    nop
    nop
    nop
    nop
    djnz setup
    xor a
    out ($08),a
    ld a,1
    out ($0C),a
    ei
idle:
    halt
    jr idle
registers:
    .DB $01,$3F, $08,$DF, $10,$00, $18,$00, $20,$0F, $28,$00, $00,$81
