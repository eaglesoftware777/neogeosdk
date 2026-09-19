; ============================================================================
;  EagleBIOS Open-Source SM1 Sound Firmware (Z80)
;  Target: Zilog Z80 / Yamaha YM2610 (OPNB)
; ============================================================================

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

; --- Vector 0: System Reset ($0000) ---
    di
    jp sound_init

; --- Vector 1: YM2610 Wait Ready ($0008) ---
.org $0008
    jp ym_wait

; --- Vector 2: Write Port A ($0010) ---
.org $0010
    jp write_a

; --- Vector 3: Write Port B ($0018) ---
.org $0018
    jp write_b

; --- Vector 7: IRQ ($0038) ---
.org $0038
    jp irq_entry

; --- NMI ($0066) ---
.org $0066
    jp nmi_entry

.org $00D0

; ----------------------------------------------------------------------------
;  Low-Level YM2610 I/O Routines
; ----------------------------------------------------------------------------
ym_wait:
    in a, ($04)
    rla
    jr c, ym_wait
    ret

write_a:
    push af
    call ym_wait
    pop af
    out ($04), a
    call ym_wait
    ld a, c
    out ($05), a
    ret

write_b:
    push af
    call ym_wait
    pop af
    out ($06), a
    call ym_wait
    ld a, c
    out ($07), a
    ret

irq_entry:
    di
    push af
    in a, ($06)
    in a, ($04)
    ld a, $27
    ld c, $2A
    call write_a
    pop af
    ei
    reti

; ----------------------------------------------------------------------------
;  NMI: Command from 68000 CPU
; ----------------------------------------------------------------------------
nmi_entry:
    push af
    push bc
    push hl

    in a, ($00)         ; Read command byte from 68000
    ld b, a
    xor a
    out ($00), a        ; Clear command latch
    out ($0C), a        ; Clear status register

    ld a, b
    cp $01
    jp z, cmd_slot_switch ; Command 0x01: Prepare for cartridge slot switch
    cp $03
    jp z, cmd_soft_reset  ; Command 0x03: Soft reset / mute
    cp $02
    jp z, cmd_play_chime  ; Command 0x02: Play EagleBIOS startup fanfare

    ; Default: Acknowledge with READY (0x01)
    ld a, $01
    out ($0C), a
    pop hl
    pop bc
    pop af
    retn

; ----------------------------------------------------------------------------
;  Slot Switch Handler (Command 0x01)
; ----------------------------------------------------------------------------
cmd_slot_switch:
    call mute_all

    ; Install trampoline in Z80 Work RAM at $FFFD: JP $FFFD (C3 FD FF)
    ld hl, $FFFD
    ld (hl), $C3
    inc hl
    ld (hl), $FD
    inc hl
    ld (hl), $FF

    ; Signal 0x01 (Ready) to 68000 via Port $0C
    ld a, $01
    out ($0C), a

    ; Jump to RAM loop so ROM can be safely swapped out
    jp $FFFD

; ----------------------------------------------------------------------------
;  Soft Reset Handler (Command 0x03)
; ----------------------------------------------------------------------------
cmd_soft_reset:
    call mute_all
    ld a, $01
    out ($0C), a
    pop hl
    pop bc
    pop af
    retn

; ----------------------------------------------------------------------------
;  Play Chime Handler (Command 0x02)
; ----------------------------------------------------------------------------
cmd_play_chime:
    call play_bios_fanfare
    ld a, $01
    out ($0C), a
    pop hl
    pop bc
    pop af
    retn

; ----------------------------------------------------------------------------
;  Mute all YM2610 channels
; ----------------------------------------------------------------------------
mute_all:
    ld a, $07
    ld c, $3F
    call write_a
    ld a, $08
    ld c, $00
    call write_a
    ld a, $09
    ld c, $00
    call write_a
    ld a, $0A
    ld c, $00
    call write_a
    ret

; ----------------------------------------------------------------------------
;  EagleBIOS Boot Fanfare (Harmonic Chime on SSG Channels A, B, C)
; ----------------------------------------------------------------------------
play_bios_fanfare:
    ld a, $07
    ld c, $38           ; Enable channels A, B, C
    call write_a

    ; Note 1: C4 (period 0x01DE) on Channel A
    ld a, $00
    ld c, $DE
    call write_a
    ld a, $01
    ld c, $01
    call write_a
    ld a, $08
    ld c, $0C
    call write_a
    call delay_short

    ; Note 2: E4 (period 0x017B) on Channel B
    ld a, $02
    ld c, $7B
    call write_a
    ld a, $03
    ld c, $01
    call write_a
    ld a, $09
    ld c, $0D
    call write_a
    call delay_short

    ; Note 3: G4 (period 0x013F) on Channel C
    ld a, $04
    ld c, $3F
    call write_a
    ld a, $05
    ld c, $01
    call write_a
    ld a, $0A
    ld c, $0E
    call write_a
    call delay_short

    ; Final Triad: C5 (0x00EF), E5 (0x00BD), G5 (0x009F)
    ld a, $00
    ld c, $EF
    call write_a
    ld a, $01
    ld c, $00
    call write_a
    ld a, $02
    ld c, $BD
    call write_a
    ld a, $03
    ld c, $00
    call write_a
    ld a, $04
    ld c, $9F
    call write_a
    ld a, $05
    ld c, $00
    call write_a

    ld a, $08
    ld c, $0F
    call write_a
    ld a, $09
    ld c, $0F
    call write_a
    ld a, $0A
    ld c, $0F
    call write_a

    call delay_long

    ; Soft decay
    ld a, $08
    ld c, $09
    call write_a
    ld a, $09
    ld c, $09
    call write_a
    ld a, $0A
    ld c, $09
    call write_a
    call delay_short

    ld a, $08
    ld c, $04
    call write_a
    ld a, $09
    ld c, $04
    call write_a
    ld a, $0A
    ld c, $04
    call write_a
    call delay_short

    call mute_all
    ret

delay_short:
    ld b, $30
.d1:
    ld d, $FF
.d2:
    dec d
    jr nz, .d2
    djnz .d1
    ret

delay_long:
    ld b, $90
.d3:
    ld d, $FF
.d4:
    dec d
    jr nz, .d4
    djnz .d3
    ret

; ----------------------------------------------------------------------------
;  Main Initialization on Cold Boot
; ----------------------------------------------------------------------------
sound_init:
    ld sp, $FFFC
    xor a
    out ($18), a        ; Disable NMIs during init
    call mute_all

    ; Signal 0x01 (Ready) to 68000
    ld a, $01
    out ($0C), a

    xor a
    out ($08), a        ; Enable NMIs

    ; Automatically play the EagleBIOS startup chime on cold boot!
    call play_bios_fanfare

    ei
idle_loop:
    halt
    jr idle_loop

