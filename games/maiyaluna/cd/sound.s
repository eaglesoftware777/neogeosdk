; Original CD RAM driver: six ADPCM-A channels, bounded game-specific samples.
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
    reti
.ORG $66
    push af
    in a,($00)
    ld ($F800),a
    xor a
    out ($00),a
    out ($0C),a
    pop af
    retn
.ORG $100
init:
    im 1
    xor a
    ld ($F800),a
    ld ($F801),a
    ld a,$01
    ld d,$3C
    call ym
    xor a
    out ($08),a
    ei
ready:
    ld a,1
    out ($0C),a
idle:
    ld a,($F800)
    or a
    jr z,idle
    ld b,a
    xor a
    ld ($F800),a
    ld a,b
    cp 1
    jr z,stop
    bit 7,a
    jr nz,volume
    sub $40
    cp sample_count
    jr nc,ready
    ld l,a
    ld h,0
    add hl,hl
    add hl,hl
    ld de,samples
    add hl,de
    ld a,($F801)
    ld c,a
    inc a
    cp 6
    jr c,channel
    xor a
channel:
    ld ($F801),a
    ld b,c
    inc b
    ld a,1
mask:
    dec b
    jr z,gotmask
    add a,a
    jr mask
gotmask:
    ld e,a
    or $80
    ld d,a
    xor a
    call ym
    ld d,(hl)
    inc hl
    ld a,c
    add $10
    call ym
    ld d,(hl)
    inc hl
    ld a,c
    add $18
    call ym
    ld d,(hl)
    inc hl
    ld a,c
    add $20
    call ym
    ld d,(hl)
    ld a,c
    add $28
    call ym
    ld a,c
    add $08
    ld d,$DF
    call ym
    ld d,e
    xor a
    call ym
    jp ready
stop:
    xor a
    ld d,$BF
    call ym
    jp ready
volume:
    and $3F
    ld d,a
    ld a,$01
    call ym
    jp ready
ym:
    out ($06),a
    push bc
    ld b,4
addrwait:
    djnz addrwait
    ld a,d
    out ($07),a
    ld b,8
datawait:
    djnz datawait
    pop bc
    ret
; The builder appends a checked sample_count and 256-byte-unit sample table.
.ORG $300
