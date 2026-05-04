;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Neo Geo YM2610 sound driver
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

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

;;; RAM Definitions
.define RAM_BASE           $F800
.define FIFO_BUF           $F800 ; $F800-$F81F (32 bytes)
.define FIFO_READ          $F820
.define FIFO_WRITE         $F821
.define SHADOW_A           $F900
.define SHADOW_B           $FA00

.define VAR_COMMAND        $FE00
.define VAR_TEMPO          $FE01
.define VAR_TICK           $FE02
.define VAR_MUSIC_PTR_LO   $FE03
.define VAR_MUSIC_PTR_HI   $FE04
.define VAR_ADPCMA_CH      $FE05
.define VAR_WAIT_TEMPO     $FE06
.define VAR_MUSIC_WAIT     $FE07
.define VAR_MUSIC_ACTIVE   $FE08
.define VAR_MUSIC_VOL      $FE09
.define VAR_ADPCMA_TICKS   $FE0A
.define VAR_PARAM_MODE     $FE0B
.define VAR_ADPCMA_VOL     $FE0C
.define VAR_ADPCMB_VOL     $FE0D
.define VAR_FM_TICKS       $FE0E
.define VAR_MUSIC_START_LO $FE0F
.define VAR_MUSIC_START_HI $FE10
.define VAR_FADE_MODE      $FE11
.define VAR_FADE_SPEED     $FE12
.define VAR_MUSIC_VOL_BASE $FE13
.define VAR_ADPCMA_BASE    $FE14
.define VAR_ADPCMB_BASE    $FE15
.define VAR_FADE_TICKS     $FE16
.define VAR_FM_PTR_LO      $FE17
.define VAR_FM_PTR_HI      $FE18
.define VAR_FM_WAIT        $FE19
.define VAR_FM_ACTIVE      $FE1A
.define VAR_FM_VOL         $FE1B
.define VAR_FM_VOL_BASE    $FE1C
.define VAR_FM_START_LO    $FE1D
.define VAR_FM_START_HI    $FE1E
.define VAR_FM_PATCH       $FE1F
.define VAR_FM_TEMPO       $FE20
.define VAR_FM_TICK        $FE21
.define VAR_SSG_PTR_LO     $FE22
.define VAR_SSG_PTR_HI     $FE23
.define VAR_SSG_START_LO   $FE24
.define VAR_SSG_START_HI   $FE25
.define VAR_SSG_WAIT       $FE26
.define VAR_SSG_ACTIVE     $FE27
.define VAR_SSG_PRESET     $FE28
.define VAR_SSG_TEMPO      $FE29
.define VAR_SSG_TICK       $FE2A
.define VAR_SSG_VOL        $FE2B

.define STACK              $FFFC
.define READY_VALUE        $01

;;; Command Range
.define MUSIC_BASE         $20
.define SFX_A_BASE         $40
.define SFX_B_BASE         $80
.define SSG_CMD_PRESET    $14
.define SSG_CMD_PLAY      $32

.bank 0 slot 0
.org $0000
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
main:
    ld sp,STACK
    im 1

    ; Clear Work RAM
    xor a
    ld hl,RAM_BASE
    ld de,RAM_BASE + 1
    ld bc,$07FF
    ld (hl),a
    ldir

    ; Initialize shadow registers to $FF.
    ld a,$FF
    ld hl,SHADOW_A
    ld de,SHADOW_A + 1
    ld bc,$01FF
    ld (hl),a
    ldir

    call driver_init

main_loop:
    call process_fifo
    halt
    jr main_loop

driver_init:
    di
    xor a
    out ($18),a       ; Disable NMIs during init
    out ($0C),a
    ld (VAR_MUSIC_ACTIVE),a
    ld (VAR_MUSIC_WAIT),a
    ld (VAR_FADE_MODE),a
    ld (VAR_FADE_SPEED),a
    ld (VAR_FADE_TICKS),a
    ld (VAR_FM_ACTIVE),a
    ld (VAR_FM_WAIT),a
    ld (VAR_FM_PATCH),a
    ld (VAR_FM_VOL),a
    ld (VAR_FM_VOL_BASE),a
    ld (VAR_FM_TEMPO),a
    ld (VAR_FM_TICK),a
    ld (VAR_SSG_ACTIVE),a
    ld (VAR_SSG_WAIT),a
    ld (VAR_SSG_PRESET),a
    ld (VAR_SSG_TEMPO),a
    ld (VAR_SSG_TICK),a
    ld a,$0A
    ld (VAR_SSG_VOL),a
    ld a,3
    ld (VAR_TEMPO),a
    ld a,$0F
    ld (VAR_MUSIC_VOL),a
    ld (VAR_MUSIC_VOL_BASE),a
    ld a,$3F
    ld (VAR_ADPCMA_VOL),a
    ld (VAR_ADPCMA_BASE),a
    ld a,$A8
    ld (VAR_ADPCMB_VOL),a
    ld (VAR_ADPCMB_BASE),a

    ; Yamaha Best Practice: Clear ADPCM/FM state
    ld de,$0100 ; Port A Reset
    call shadowed_write_a
    ld de,$0100 ; Port B TL Max
    call shadowed_write_b

    call init_ssg
    call init_fm
    call init_adpcma
    call init_adpcmb

    xor a
    out ($08),a       ; Enable NMIs
    ld a,READY_VALUE
    out ($0C),a
    ei
    ret

process_fifo:
    ld a,(FIFO_READ)
    ld b,a
    ld a,(FIFO_WRITE)
    cp b
    ret z ; Empty

    ld l,b
    ld h,>FIFO_BUF
    ld a,(hl) ; Command
    push af

    inc b
    ld a,b
    and $1F
    ld (FIFO_READ),a

    pop af
    call execute_command
    jr process_fifo

execute_command:
    ld c,a
    ld a,(VAR_PARAM_MODE)
    or a
    jp z,exec_normal
    xor a
    ld (VAR_PARAM_MODE),a
    ld a,(VAR_WAIT_TEMPO)
    cp 1
    jr z,exec_p_tempo
    cp 2
    jr z,exec_p_adpcma
    cp 3
    jr z,exec_p_adpcmb
    cp 4
    jr z,exec_p_ssg
    cp 5
    jr z,exec_p_fadeout
    cp 6
    jr z,exec_p_fadein
    cp 7
    jr z,exec_p_fmtrack
	cp 8
    jr z,exec_p_fmvol
    cp 9
    jr z,exec_p_ssgtrack
    cp 10
    jr z,exec_p_ssgpreset
    ret
exec_p_tempo:
    ld a,c
    ld (VAR_TEMPO),a
    ret
exec_p_fadeout:
    ld a,c
    ld (VAR_FADE_SPEED),a
    ld a,1
    ld (VAR_FADE_MODE),a
    ld a,$FF
    sub c
    jr nz,exec_fade_interval_ok
    ld a,1
exec_fade_interval_ok:
    ld (VAR_FADE_TICKS),a
    ret
exec_p_fadein:
    ld a,c
    ld (VAR_FADE_SPEED),a
    ld a,2
    ld (VAR_FADE_MODE),a
    ld a,$FF
    sub c
    jr nz,exec_fade_interval_ok2
    ld a,1
exec_fade_interval_ok2:
    ld (VAR_FADE_TICKS),a
    ret
exec_p_adpcma:
    ld a,c
    and $3F
    ld (VAR_ADPCMA_VOL),a
    ld (VAR_ADPCMA_BASE),a
    ld d,$01
    ld e,a
    call shadowed_write_b
    ret
exec_p_adpcmb:
    ld a,c
    ld (VAR_ADPCMB_VOL),a
    ld (VAR_ADPCMB_BASE),a
    ld d,$1B
    ld e,a
    call shadowed_write_a
    ret
exec_p_ssg:
    ld a,c
    and $0F
    ld (VAR_MUSIC_VOL),a
    ld (VAR_MUSIC_VOL_BASE),a
    call apply_music_volume
    ret
exec_p_fmtrack:
    ld a,c
    jp play_fm_index
exec_p_fmvol:
    ld a,c
    and $0F
    ld (VAR_FM_VOL),a
    ld (VAR_FM_VOL_BASE),a
    call fm_apply_patch
    ret
exec_p_ssgtrack:
    ld a,c
    jp play_ssg_index
exec_p_ssgpreset:
    ld a,c
    and $0F
    ld (VAR_SSG_PRESET),a
    call ssg_apply_preset
    ret
set_fmvol_wait:
    ld a,8
    ld (VAR_WAIT_TEMPO),a
    ld (VAR_PARAM_MODE),a
    ret
exec_normal:
    ld a,c
    or a
    ret z

    cp $01 ; Init
    jp z,driver_init
    cp $02 ; BIOS eyecatcher / boot music
    jp z,play_music1
    cp $03 ; Reset
    jp z,driver_soft_reset
    cp $04 ; Stop all
    jp z,stop_all
    cp $05 ; ADPCM-A volume parameter follows
    jr z,set_adpcma_volume_wait
    cp $06 ; ADPCM-B volume parameter follows
    jr z,set_adpcmb_volume_wait
    cp $07 ; SSG/MML volume parameter follows
    jr z,set_ssg_volume_wait
    cp $0A ; Fade out speed parameter follows
    jr z,set_fadeout_wait
    cp $0C ; ADPCM-A stop
    jp z,adpcma_stop
    cp $0D ; ADPCM-B stop
    jp z,adpcmb_stop
    cp $0E ; Tempo Wait
    jr z,set_tempo_wait
    cp $0F ; Stop SSG / music only
    jp z,stop_music
    cp $11 ; Stop fade out
    jp z,cancel_fade
    cp $12 ; Fade in speed parameter follows
    jr z,set_fadein_wait
	cp $13 ; FM volume parameter follows
    jr z,set_fmvol_wait
    cp SSG_CMD_PRESET ; SSG preset parameter follows
    jr z,set_ssgpreset_wait
    cp $30 ; FM debug tone
    jp z,play_fm_demo
    cp $31 ; FM track select parameter follows
    jr z,set_fmtrack_wait
    cp SSG_CMD_PLAY ; SSG track select parameter follows
    jr z,set_ssgtrack_wait
    cp $28 ; ADPCM-B direct sample 0
    jr z,play_demo_b0
    cp $29 ; ADPCM-B direct sample 1
    jr z,play_demo_b1

    cp SFX_B_BASE
    jp nc,play_adpcmb_cmd
    cp SFX_A_BASE
    jp nc,play_adpcma_cmd
    cp MUSIC_BASE
    jp nc,play_fm_cmd
    ret

set_tempo_wait:
    ld a,1
    ld (VAR_WAIT_TEMPO),a
    ld (VAR_PARAM_MODE),a
    ret

set_adpcma_volume_wait:
    ld a,2
    ld (VAR_WAIT_TEMPO),a
    ld (VAR_PARAM_MODE),a
    ret

set_adpcmb_volume_wait:
    ld a,3
    ld (VAR_WAIT_TEMPO),a
    ld (VAR_PARAM_MODE),a
    ret

set_ssg_volume_wait:
    ld a,4
    ld (VAR_WAIT_TEMPO),a
    ld (VAR_PARAM_MODE),a
    ret

set_fadeout_wait:
    ld a,5
    ld (VAR_WAIT_TEMPO),a
    ld (VAR_PARAM_MODE),a
    ret

set_fadein_wait:
    ld a,6
    ld (VAR_WAIT_TEMPO),a
    ld (VAR_PARAM_MODE),a
    ret

set_fmtrack_wait:
    ld a,7
    ld (VAR_WAIT_TEMPO),a
    ld (VAR_PARAM_MODE),a
    ret

set_ssgtrack_wait:
    ld a,9
    ld (VAR_WAIT_TEMPO),a
    ld (VAR_PARAM_MODE),a
    ret

set_ssgpreset_wait:
    ld a,10
    ld (VAR_WAIT_TEMPO),a
    ld (VAR_PARAM_MODE),a
    ret

play_demo_b0:
    xor a
    jp play_adpcmb_index

play_demo_b1:
    ld a,1
    jp play_adpcmb_index

;;; Shadowed Register Access
shadowed_write_a:
    ld h,>SHADOW_A
    ld l,d
    ld a,(hl)
    cp e
    ret z ; No change
    ld (hl),e
    ld a,d
    out ($04),a
    rst $08
    ld a,e
    out ($05),a
    rst $08
    ret

shadowed_write_b:
    ld h,>SHADOW_B
    ld l,d
    ld a,(hl)
    cp e
    ret z ; No change
    ld (hl),e
    ld a,d
    out ($06),a
    rst $08
    ld a,e
    out ($07),a
    rst $08
    ret

apply_music_volume:
    ld d,$08
    ld a,(VAR_MUSIC_VOL)
    and $0F         ; FIX: Ensure M bit (bit 4) is 0 for fixed amplitude mode
    ld e,a
    call shadowed_write_a
    ld d,$09
    ld a,(VAR_MUSIC_VOL)
    and $0F         ; FIX: Ensure M bit (bit 4) is 0 for fixed amplitude mode
    srl a
    add a,3
    ld e,a
    jp shadowed_write_a

ssg_apply_standalone_volume:
    ld d,$08
    ld a,(VAR_SSG_VOL)
    and $0F         ; FIX: Ensure M bit (bit 4) is 0 for fixed amplitude mode
    ld e,a
    call shadowed_write_a
    ld d,$09
    ld a,(VAR_SSG_VOL)
    and $0F         ; FIX: Ensure M bit (bit 4) is 0 for fixed amplitude mode
    srl a
    add a,3
    ld e,a
    jp shadowed_write_a

apply_master_volumes:
    ld d,$01
    ld a,(VAR_ADPCMA_VOL)
    ld e,a
    call shadowed_write_b
    ld d,$1B
    ld a,(VAR_ADPCMB_VOL)
    ld e,a
    call shadowed_write_a
    jp apply_music_volume

cancel_fade:
    xor a
    ld (VAR_FADE_MODE),a
    ld (VAR_FADE_SPEED),a
    ld (VAR_FADE_TICKS),a
    ld a,(VAR_MUSIC_VOL_BASE)
    ld (VAR_MUSIC_VOL),a
    ld a,(VAR_ADPCMA_BASE)
    ld (VAR_ADPCMA_VOL),a
    ld a,(VAR_ADPCMB_BASE)
    ld (VAR_ADPCMB_VOL),a
    jp apply_master_volumes

stop_music:
    xor a
    ld (VAR_MUSIC_ACTIVE),a
    ld (VAR_MUSIC_WAIT),a
    ld (VAR_FADE_MODE),a
    ld (VAR_FADE_SPEED),a
    ld (VAR_FADE_TICKS),a
    ld (VAR_FM_ACTIVE),a
    ld (VAR_FM_WAIT),a
    ld (VAR_SSG_ACTIVE),a
    ld (VAR_SSG_WAIT),a
    call fm_silence_all   ; FIX: Properly silence all FM channels
    jp init_ssg

force_write_a:
    ld h,>SHADOW_A
    ld l,d
    ld (hl),e
    ld a,d
    out ($04),a
    rst $08
    ld a,e
    out ($05),a
    rst $08
    ret

force_write_b:
    ld h,>SHADOW_B
    ld l,d
    ld (hl),e
    ld a,d
    out ($06),a
    rst $08
    ld a,e
    out ($07),a
    rst $08
    ret

;;; Driver Subsystems
init_ssg:
    ; FIX: Proper SSG initialization
    ld de,$073F ; All channels off (mixer: disable all tones and noise)
    call shadowed_write_a
    ld de,$0600 ; Initialize noise frequency to 0
    call shadowed_write_a
    ld de,$0800 ; Channel A volume = 0 (M=0, fixed amplitude)
    call shadowed_write_a
    ld de,$0900 ; Channel B volume = 0 (M=0, fixed amplitude)
    call shadowed_write_a
    ld de,$0A00 ; Channel C volume = 0 (M=0, fixed amplitude)
    call shadowed_write_a
    ld de,$0B00 ; Envelope period low = 0
    call shadowed_write_a
    ld de,$0C00 ; Envelope period high = 0
    call shadowed_write_a
    ld de,$0D00 ; Envelope shape = 0 (no envelope)
    call shadowed_write_a
    ret

init_fm:
    ld de,$260F ; Timer B period
    call shadowed_write_a
    ld de,$272A ; Reset flags, enable Timer B IRQ, load Timer B
    call force_write_a
    ld de,$273A ; Clear resets, keep Timer B IRQ+load active
    call shadowed_write_a
    
    ; FIX: Silence all FM channels properly
    call fm_silence_all
    
    ; FIX: Initialize LFO to off
    ld de,$2200
    call shadowed_write_a
    
    ret

fm_silence_all:
    ; FIX: Properly silence all 4 FM channels (key off all slots)
    ld b,4
    ld c,$01   ; Start with key-code $01
fm_silence_loop:
    push bc
    ld d,$28
    ld e,c
    call force_write_a    ; Key off this channel
    
    ; Also key off 2nd slot for this channel if in 2-operator mode
    ld a,c
    add a,4
    ld e,a
    call force_write_a
    
    pop bc
    inc c
    djnz fm_silence_loop
    
    ; FIX: Set all operator TL to max (silence)
    ld b,$0D
    ld de,$417F
fm_silence_tl:
    call shadowed_write_a
    call shadowed_write_b
    inc d
    djnz fm_silence_tl
    
    ret

init_adpcma:
    call adpcma_stop
    ld d,$01
    ld a,(VAR_ADPCMA_VOL)
    ld e,a
    call shadowed_write_b
    ld b,6
    ld d,8
adpcma_vol_loop:
    ld e,$C0
    call shadowed_write_b
    inc d
    djnz adpcma_vol_loop
    ret

init_adpcmb:
    ld de,$1001 ; Reset
    call force_write_a
    ld de,$1C80 ; Clear ADPCM-B end flag
    call force_write_a
    ld de,$1C00
    call force_write_a
    ld de,$1000 ; Release reset before programming
    call force_write_a
    ld de,$11C0 ; Left and right output
    call shadowed_write_a
    ld de,$19BA ; 16kHz low byte
    call shadowed_write_a
    ld de,$1A49 ; 16kHz high byte
    call shadowed_write_a
    ld d,$1B
    ld a,(VAR_ADPCMB_VOL)
    ld e,a
    call shadowed_write_a
    ret

stop_all:
    xor a
    ld (VAR_MUSIC_ACTIVE),a
    ld (VAR_ADPCMA_TICKS),a
    ld (VAR_FM_TICKS),a
    ld (VAR_FADE_MODE),a
    ld (VAR_FADE_SPEED),a
    ld (VAR_FADE_TICKS),a
    ld (VAR_FM_ACTIVE),a
    ld (VAR_FM_WAIT),a
    ld (VAR_FM_PATCH),a
    ld (VAR_FM_TEMPO),a
    ld (VAR_FM_TICK),a
    ld (VAR_SSG_ACTIVE),a
    ld (VAR_SSG_WAIT),a
    ld (VAR_SSG_PRESET),a
    ld (VAR_SSG_TEMPO),a
    ld (VAR_SSG_TICK),a
    call init_ssg
    call fm_silence_all   ; FIX: Use proper silence function
    call adpcma_stop
    jp adpcmb_stop

driver_soft_reset:
    xor a
    ld (VAR_MUSIC_ACTIVE),a
    ld (VAR_ADPCMA_TICKS),a
    ld (VAR_FM_TICKS),a
    ld (VAR_MUSIC_WAIT),a
    ld (VAR_WAIT_TEMPO),a
    ld (VAR_PARAM_MODE),a
    ld (VAR_TICK),a
    ld (FIFO_READ),a
    ld (FIFO_WRITE),a
    ld (VAR_ADPCMA_CH),a
    ld (VAR_MUSIC_START_LO),a
    ld (VAR_MUSIC_START_HI),a
    ld (VAR_FADE_MODE),a
    ld (VAR_FADE_SPEED),a
    ld (VAR_FADE_TICKS),a
    ld (VAR_FM_ACTIVE),a
    ld (VAR_FM_WAIT),a
    ld (VAR_FM_PATCH),a
    ld (VAR_FM_VOL),a
    ld (VAR_FM_VOL_BASE),a
    ld (VAR_FM_TEMPO),a
    ld (VAR_FM_TICK),a
    ld (VAR_SSG_ACTIVE),a
    ld (VAR_SSG_WAIT),a
    ld (VAR_SSG_PRESET),a
    ld (VAR_SSG_TEMPO),a
    ld (VAR_SSG_TICK),a
    ld a,$0A
    ld (VAR_SSG_VOL),a
    ld a,3
    ld (VAR_TEMPO),a
    ld a,$08
    ld (VAR_MUSIC_VOL),a
    ld (VAR_MUSIC_VOL_BASE),a
    ld a,$3F
    ld (VAR_ADPCMA_VOL),a
    ld (VAR_ADPCMA_BASE),a
    ld a,$A8
    ld (VAR_ADPCMB_VOL),a
    ld (VAR_ADPCMB_BASE),a
    jp stop_all

adpcma_stop:
    ld de,$00BF
    jp force_write_b

adpcmb_stop:
    ld de,$1001
    jp force_write_a

;;; Playback Handlers
play_fm_cmd:
    sub MUSIC_BASE
play_music_index:
    cp MUSIC_TRACK_COUNT
    ret nc
    add a,a
    ld e,a
    ld d,0
    ld hl,music_track_table
    add hl,de
    ld a,(hl)
    ld (VAR_MUSIC_PTR_LO),a
    ld (VAR_MUSIC_START_LO),a
    inc hl
    ld a,(hl)
    ld (VAR_MUSIC_PTR_HI),a
    ld (VAR_MUSIC_START_HI),a
    xor a
    ld (VAR_MUSIC_WAIT),a
    ld a,1
    ld (VAR_MUSIC_ACTIVE),a
    jp music_step

play_music0:
    ld a,0
    jp play_music_index

play_music1:
    ld a,1
    jp play_music_index

play_fm_index:
    cp FM_TRACK_COUNT
    ret nc
    call fm_stop
    add a,a
    ld e,a
    ld d,0
    ld hl,fm_track_table
    add hl,de
    ld a,(hl)
    ld (VAR_FM_PTR_LO),a
    ld (VAR_FM_START_LO),a
    inc hl
    ld a,(hl)
    ld (VAR_FM_PTR_HI),a
    ld (VAR_FM_START_HI),a
    xor a
    ld (VAR_FM_WAIT),a
    ld a,1
    ld (VAR_FM_ACTIVE),a
    ld a,0
    ld (VAR_FM_PATCH),a
    ld a,$0F
    ld (VAR_FM_VOL_BASE),a
    ld (VAR_FM_VOL),a
    ld a,3
    ld (VAR_FM_TEMPO),a
    xor a
    ld (VAR_FM_TICK),a
    call fm_apply_patch
    jp fm_step

play_adpcma_cmd:
    sub SFX_A_BASE
play_adpcma_index:
    cp ADPCMA_COUNT
    ret nc
    push af
    ld e,a
    ld d,0
    ld hl,adpcma_stop_ticks
    add hl,de
    ld a,(hl)
    ld (VAR_ADPCMA_TICKS),a
    pop af
    call get_sample_ptr
    push hl
    push bc
    ld a,(VAR_ADPCMA_CH)
    ld c,a
    inc a
    cp 6
    jr c,adpcma_channel_ok
    xor a
adpcma_channel_ok:
    ld (VAR_ADPCMA_CH),a

    ld hl,channel_masks \ ld e,c \ ld d,0 \ add hl,de
    ld a,(hl)
    ld (VAR_COMMAND),a
    ld d,$1C \ ld e,a \ call force_write_a
    ld de,$1C00 \ call force_write_a

    ld d,$01 \ ld a,(VAR_ADPCMA_VOL) \ ld e,a \ call shadowed_write_b
    ld a,c \ add a,$08 \ ld d,a \ ld e,$C0 \ call shadowed_write_b
    pop hl
    ld a,c \ add a,$10 \ ld d,a \ ld e,h \ call shadowed_write_b
    ld a,c \ add a,$18 \ ld d,a \ ld e,l \ call shadowed_write_b
    pop hl
    ld b,h
    ld a,c \ add a,$20 \ ld d,a \ ld e,l \ call shadowed_write_b
    ld a,c \ add a,$28 \ ld d,a \ ld e,b \ call shadowed_write_b
    ld a,(VAR_COMMAND)
    ld e,a \ ld d,$00 \ call force_write_b
    ret

play_adpcmb_cmd:
    sub SFX_B_BASE
play_adpcmb_index:
    cp ADPCMB_COUNT
    ret nc
    add a,ADPCMA_COUNT
    call get_sample_ptr
    push hl
    ld de,$1001 \ call force_write_a
    ld de,$1C80 \ call force_write_a
    ld de,$1C00 \ call force_write_a
    ld d,$12 \ ld e,b \ call shadowed_write_a
    ld d,$13 \ ld e,c \ call shadowed_write_a
    pop hl
    ld c,h
    ld d,$14 \ ld e,l \ call shadowed_write_a
    ld d,$15 \ ld e,c \ call shadowed_write_a
    ld d,$1B \ ld a,(VAR_ADPCMB_VOL) \ ld e,a \ call shadowed_write_a
    ld de,$1000 \ call force_write_a
    ld de,$1080 \ call force_write_a
    ret

play_ssg_index:
    cp SSG_TRACK_COUNT
    ret nc
    call init_ssg
    add a,a
    ld e,a
    ld d,0
    ld hl,ssg_track_table
    add hl,de
    ld a,(hl)
    ld (VAR_SSG_PTR_LO),a
    ld (VAR_SSG_START_LO),a
    inc hl
    ld a,(hl)
    ld (VAR_SSG_PTR_HI),a
    ld (VAR_SSG_START_HI),a
    xor a
    ld (VAR_SSG_WAIT),a
    ld a,1
    ld (VAR_SSG_ACTIVE),a
    ld a,0
    ld (VAR_SSG_PRESET),a
    ld a,$0A
    ld (VAR_SSG_VOL),a
    ld a,3
    ld (VAR_SSG_TEMPO),a
    xor a
    ld (VAR_SSG_TICK),a
    call ssg_apply_preset
    jp ssg_step

play_fm_demo:
    xor a
    jp play_fm_index

get_sample_ptr:
    ld e,a \ ld d,0 \ ld hl,sample_address_table
    add hl,de \ add hl,de \ add hl,de \ add hl,de
    ld b,(hl) ; Start Lo
    inc hl
    ld c,(hl) ; Start Hi
    inc hl
    ld e,(hl) ; End Lo
    inc hl
    ld d,(hl) ; End Hi
    ld l,e
    ld h,d
    ret

ticker_update:
    ld a,(VAR_FM_TICKS)
    or a
    jr z,ticker_adpcma
    dec a
    ld (VAR_FM_TICKS),a
    jr nz,ticker_adpcma
    call fm_stop
ticker_adpcma:
    ld a,(VAR_ADPCMA_TICKS)
    or a
    jr z,ticker_music
    dec a
    ld (VAR_ADPCMA_TICKS),a
    jr nz,ticker_music
    call adpcma_stop
ticker_music:
    ld a,(VAR_FM_ACTIVE)
    or a
    call nz,fm_tick
    ld a,(VAR_SSG_ACTIVE)
    or a
    call nz,ssg_tick
    ld a,(VAR_TICK) \ inc a \ ld (VAR_TICK),a
    ld b,a \ ld a,(VAR_TEMPO) \ or a \ ret z
    cp b \ ret nz
    xor a \ ld (VAR_TICK),a
    ld a,(VAR_MUSIC_ACTIVE)
    or a
    call nz,music_tick
    call fade_tick
    ret

fade_tick:
    ld a,(VAR_FADE_MODE)
    or a
    ret z
    ld b,a
    ld a,(VAR_FADE_TICKS)
    or a
    jr z,fade_step
    dec a
    ld (VAR_FADE_TICKS),a
    ret nz
fade_step:
    ld a,(VAR_FADE_SPEED)
    ld e,a
    ld a,$FF
    sub e
    jr nz,fade_interval_ok
    ld a,1
fade_interval_ok:
    ld (VAR_FADE_TICKS),a
    ld a,b
    cp 1
    jr z,fade_out_step
    cp 2
    jr z,fade_in_step
    ret

fade_out_step:
    ld a,(VAR_MUSIC_VOL)
    or a
    jr z,fade_out_adpcma
    dec a
    ld (VAR_MUSIC_VOL),a
fade_out_adpcma:
    ld a,(VAR_ADPCMA_VOL)
    or a
    jr z,fade_out_adpcmb
    dec a
    ld (VAR_ADPCMA_VOL),a
fade_out_adpcmb:
    ld a,(VAR_ADPCMB_VOL)
    or a
    jr z,fade_out_apply
    dec a
    ld (VAR_ADPCMB_VOL),a
fade_out_apply:
    call apply_master_volumes
    ld a,(VAR_MUSIC_VOL)
    or a
    jr nz,fade_tick_done
    ld a,(VAR_ADPCMA_VOL)
    or a
    jr nz,fade_tick_done
    ld a,(VAR_ADPCMB_VOL)
    or a
    jr nz,fade_tick_done
    xor a
    ld (VAR_FADE_MODE),a
    ret

fade_in_step:
    ld a,(VAR_MUSIC_VOL)
    ld e,a
    ld a,(VAR_MUSIC_VOL_BASE)
    cp e
    jr c,fade_in_adpcma
    jr z,fade_in_adpcma
    ld a,(VAR_MUSIC_VOL)
    inc a
    ld (VAR_MUSIC_VOL),a
fade_in_adpcma:
    ld a,(VAR_ADPCMA_VOL)
    ld e,a
    ld a,(VAR_ADPCMA_BASE)
    cp e
    jr c,fade_in_adpcmb
    jr z,fade_in_adpcmb
    ld a,(VAR_ADPCMA_VOL)
    inc a
    ld (VAR_ADPCMA_VOL),a
fade_in_adpcmb:
    ld a,(VAR_ADPCMB_VOL)
    ld e,a
    ld a,(VAR_ADPCMB_BASE)
    cp e
    jr c,fade_in_apply
    jr z,fade_in_apply
    ld a,(VAR_ADPCMB_VOL)
    inc a
    ld (VAR_ADPCMB_VOL),a
fade_in_apply:
    call apply_master_volumes
    ld a,(VAR_MUSIC_VOL)
    ld e,a
    ld a,(VAR_MUSIC_VOL_BASE)
    cp e
    jr c,fade_tick_done
    jr z,fade_tick_done
    ld a,(VAR_ADPCMA_VOL)
    ld e,a
    ld a,(VAR_ADPCMA_BASE)
    cp e
    jr c,fade_tick_done
    jr z,fade_tick_done
    ld a,(VAR_ADPCMB_VOL)
    ld e,a
    ld a,(VAR_ADPCMB_BASE)
    cp e
    jr c,fade_tick_done
    jr z,fade_tick_done
    xor a
    ld (VAR_FADE_MODE),a
    ret

fade_tick_done:
    ret

music_tick:
    ld a,(VAR_MUSIC_WAIT)
    or a
    jr z,music_step
    dec a
    ld (VAR_MUSIC_WAIT),a
    ret

music_step:
    ld a,(VAR_MUSIC_PTR_LO)
    ld l,a
    ld a,(VAR_MUSIC_PTR_HI)
    ld h,a
music_step_next:
    ld a,(hl)
    inc hl
    cp $FF
    jp z,music_stop
    cp $FE
    jr z,music_loop
    cp $F0
    jr z,music_set_tempo
    cp $F1
    jr z,music_set_volume
    cp $F2
    jr z,music_play_adpcma
    cp $F3
    jr z,music_play_adpcmb
    cp $F4
    jp z,music_play_fm

    cp $F5
    jp z,music_play_ssg

    cp $F6
    jp z,music_set_ssg_preset

    cp $80
    jr z,music_rest
    ld b,a
    ld a,(hl)
    inc hl
    ld (VAR_MUSIC_WAIT),a
    call store_music_ptr
    jp ssg_note_on

music_set_tempo:
    ld a,(hl)
    inc hl
    call tempo_to_frames
    ld (VAR_TEMPO),a
    jp music_step_continue

music_set_volume:
    ld a,(hl)
    inc hl
    and $0F
    ld (VAR_MUSIC_VOL),a
    ld (VAR_MUSIC_VOL_BASE),a
    call store_music_ptr
    call apply_music_volume
    jp music_step
music_step_continue:
    call store_music_ptr
    jp music_step_next

music_play_adpcma:
    ld a,(hl)
    inc hl
    call store_music_ptr
    call play_adpcma_index
    jp music_step_next

music_play_adpcmb:
    ld a,(hl)
    inc hl
    call store_music_ptr
    call play_adpcmb_index
    jp music_step_next



music_rest:
    ld a,(hl)
    inc hl
    ld (VAR_MUSIC_WAIT),a
    call store_music_ptr
    jp init_ssg

music_loop:
    call init_ssg
    ld a,(VAR_MUSIC_START_LO)
    ld l,a
    ld a,(VAR_MUSIC_START_HI)
    ld h,a
    xor a
    ld (VAR_MUSIC_WAIT),a
    call store_music_ptr
    jp music_step
music_stop:
    xor a
    ld (VAR_MUSIC_ACTIVE),a
    ld (VAR_MUSIC_WAIT),a
    ld (VAR_FADE_MODE),a
    jp init_ssg


music_play_fm:
    ld a,(hl)
    inc hl

    ; save music stream pointer before starting FM,
    ; because play_fm_index / fm_step use HL
    call store_music_ptr

    push hl
    call play_fm_index
    pop hl

    jp music_step_next
	
music_play_ssg:
    ld a,(hl)
    inc hl

    ; save music stream pointer before starting standalone SSG,
    ; because play_ssg_index / ssg_step use HL
    call store_music_ptr

    push hl
    call play_ssg_index
    pop hl

    jp music_step_next


music_set_ssg_preset:
    ld a,(hl)
    inc hl
    and $0F
    ld (VAR_SSG_PRESET),a

    call store_music_ptr

    call ssg_apply_preset
    jp music_step

fm_tick:
    ld a,(VAR_FM_TICK)
    inc a
    ld (VAR_FM_TICK),a

    ld b,a
    ld a,(VAR_FM_TEMPO)
    or a
    ret z

    cp b
    ret nz

    xor a
    ld (VAR_FM_TICK),a

    ld a,(VAR_FM_WAIT)
    or a
    jr z,fm_step

    dec a
    ld (VAR_FM_WAIT),a
    ret nz

    jp fm_step


fm_step:
    ld a,(VAR_FM_PTR_LO)
    ld l,a
    ld a,(VAR_FM_PTR_HI)
    ld h,a

fm_step_next:
    ld a,(hl)
    inc hl

    cp $FF
    jp z,fm_stop

    cp $FE
    jp z,fm_loop

    cp $F0
    jp z,fm_set_tempo

    cp $F1
    jp z,fm_set_volume

    cp $F2
    jp z,fm_set_patch

    cp $80
    jp z,fm_rest

    ; normal note
    ld b,a
    ld a,(hl)
    inc hl
    ld (VAR_FM_WAIT),a

    ; save FM stream pointer before fm_note_on,
    ; because fm_note_on uses HL
    call store_fm_ptr

    call fm_note_on
    ret


fm_set_tempo:
    ld a,(hl)
    inc hl
    call tempo_to_frames
    ld (VAR_FM_TEMPO),a

    ; save pointer after consuming tempo byte
    call store_fm_ptr

    ; continue parsing immediately
    jp fm_step


fm_set_volume:
    ld a,(hl)
    inc hl
    and $0F
    ld (VAR_FM_VOL),a
    ld (VAR_FM_VOL_BASE),a

    ; save pointer before fm_apply_patch,
    ; because fm_apply_patch uses HL
    call store_fm_ptr

    call fm_apply_patch
    jp fm_step


fm_set_patch:
    ld a,(hl)
    inc hl
    and $0F
    ld (VAR_FM_PATCH),a

    ; save pointer before fm_apply_patch,
    ; because fm_apply_patch uses HL
    call store_fm_ptr

    call fm_apply_patch
    jp fm_step


fm_rest:
    ld a,(hl)
    inc hl
    ld (VAR_FM_WAIT),a

    call store_fm_ptr
    jp fm_note_off


fm_loop:
    call fm_note_off

    ld a,(VAR_FM_START_LO)
    ld l,a
    ld a,(VAR_FM_START_HI)
    ld h,a

    xor a
    ld (VAR_FM_WAIT),a

    call store_fm_ptr
    jp fm_step


fm_note_off:
    ; key off FM channel using key-code $01
    ld de,$2801
    jp force_write_a


fm_stop:
    xor a
    ld (VAR_FM_ACTIVE),a
    ld (VAR_FM_WAIT),a

    ; FIX: Key off channel and silence all operators
    ld de,$2801
    call force_write_a
    
    ; FIX: Also silence all channels to prevent hanging notes
    jp fm_silence_all

fm_apply_patch:
    ; Load YM2610 FM patch from fm_patch_table.
    ;
    ; Patch format from fm_patch_compile.py:
    ; byte 0  = LFO register $22
    ; byte 1  = B1 feedback/algorithm
    ; byte 2  = B5 stereo/AMS/PMS
    ; byte 3+ = 4 operators x 7 bytes:
    ;          DT/MUL, TL, AR, DR, SR, SL/RR, ENV

    ld a,(VAR_FM_PATCH)
    cp FM_PATCH_COUNT
    jr c,fm_apply_patch_index_ok
    xor a

fm_apply_patch_index_ok:
    ld b,a
    ld hl,fm_patch_table

    ; seek: HL += patch_index * FM_PATCH_SIZE
    ld a,b
    or a
    jr z,fm_apply_patch_ready

fm_patch_seek_loop:
    ld de,FM_PATCH_SIZE
    add hl,de
    dec a
    jr nz,fm_patch_seek_loop

fm_apply_patch_ready:
    ; LFO register $22
    ld d,$22
    ld e,(hl)
    call fm_patch_write_a
    inc hl

    ; B1 feedback/algorithm
    ld d,$B1
    ld e,(hl)
    call fm_patch_write_a
    inc hl

    ; B5 L/R + AMS/PMS
    ld d,$B5
    ld e,(hl)
    call fm_patch_write_a
    inc hl

    ; OP1 base $31
    ld b,$31
    call fm_write_operator_patch

    ; OP2 base $35
    ld b,$35
    call fm_write_operator_patch

    ; OP3 base $39
    ld b,$39
    call fm_write_operator_patch

    ; OP4 base $3D
    ld b,$3D
    call fm_write_operator_patch

    ret
fm_patch_write_a:
    ; force_write_a clobbers HL through SHADOW_A.
    ; Preserve HL while reading patch table.
    push hl
    call force_write_a
    pop hl
    ret


fm_write_operator_patch:
    ; HL points to:
    ; DT/MUL, TL, AR, DR, SR, SL/RR, ENV
    ;
    ; B = operator base:
    ; OP1=$31, OP2=$35, OP3=$39, OP4=$3D

    ; DT/MUL
    ld d,b
    ld e,(hl)
    call fm_patch_write_a
    inc hl

    ; TL = base + $10, with global FM volume
    ld a,b
    add a,$10
    ld d,a

    ld a,(VAR_FM_VOL)
    cpl
    and $0F
    add a,a
    add a,a
    ld c,a

    ld a,(hl)
    add a,c

    ; clamp TL to $7F
    cp $80
    jr c,fm_tl_ok
    ld a,$7F
fm_tl_ok:
    ld e,a
    call fm_patch_write_a
    inc hl

    ; AR = base + $20
    ld a,b
    add a,$20
    ld d,a
    ld e,(hl)
    call fm_patch_write_a
    inc hl

    ; DR = base + $30
    ld a,b
    add a,$30
    ld d,a
    ld e,(hl)
    call fm_patch_write_a
    inc hl

    ; SR = base + $40
    ld a,b
    add a,$40
    ld d,a
    ld e,(hl)
    call fm_patch_write_a
    inc hl

    ; SL/RR = base + $50
    ld a,b
    add a,$50
    ld d,a
    ld e,(hl)
    call fm_patch_write_a
    inc hl

    ; ENV = base + $60
    ld a,b
    add a,$60
    ld d,a
    ld e,(hl)
    call fm_patch_write_a
    inc hl

    ret

fm_note_on:
    ; force key off first so envelope retriggers
    ld de,$2801
    call force_write_a

    ; look up note frequency
    ld a,b
    ld e,a
    ld d,0
    ld hl,fm_note_table
    add hl,de
    add hl,de

    ld c,(hl)        ; FNUM low
    inc hl
    ld a,(hl)        ; BLOCK + FNUM high

    ; YM2610 latch order:
    ; write high buffer first, then low byte to latch frequency
    ld e,a
    ld d,$A5
    call force_write_a

    ld e,c
    ld d,$A1
    call force_write_a

    ; key on all 4 operators for key-code $01
    ld de,$28F1
    call force_write_a

    ret


store_fm_ptr:
    ld a,l
    ld (VAR_FM_PTR_LO),a
    ld a,h
    ld (VAR_FM_PTR_HI),a
    ret


store_music_ptr:
    ld a,l
    ld (VAR_MUSIC_PTR_LO),a
    ld a,h
    ld (VAR_MUSIC_PTR_HI),a
    ret

tempo_to_frames:
    cp 180
    jr nc,tempo_fastest
    cp 140
    jr nc,tempo_fast
    cp 100
    jr nc,tempo_mid
    cp 70
    jr nc,tempo_slow
    ld a,5
    ret
tempo_fastest:
    ld a,1
    ret
tempo_fast:
    ld a,2
    ret
tempo_mid:
    ld a,3
    ret
tempo_slow:
    ld a,4
    ret

ssg_tick:
    ld a,(VAR_SSG_TICK)
    inc a
    ld (VAR_SSG_TICK),a

    ld b,a
    ld a,(VAR_SSG_TEMPO)
    or a
    ret z

    cp b
    ret nz

    xor a
    ld (VAR_SSG_TICK),a

    ld a,(VAR_SSG_WAIT)
    or a
    jr z,ssg_step

    dec a
    ld (VAR_SSG_WAIT),a
    ret nz

    jp ssg_step


ssg_step:
    ld a,(VAR_SSG_PTR_LO)
    ld l,a
    ld a,(VAR_SSG_PTR_HI)
    ld h,a

ssg_step_next:
    ld a,(hl)
    inc hl

    cp $FF
    jp z,ssg_stop

    cp $FE
    jp z,ssg_loop

    cp $F0
    jp z,ssg_set_tempo

    cp $F1
    jp z,ssg_set_volume

    cp $F2
    jp z,ssg_set_preset

    cp $80
    jp z,ssg_rest

    ; normal note
    ld b,a
    ld a,(hl)
    inc hl
    ld (VAR_SSG_WAIT),a
    call store_ssg_ptr
    jp ssg_standalone_note_on


ssg_set_tempo:
    ld a,(hl)
    inc hl
    call tempo_to_frames
    ld (VAR_SSG_TEMPO),a
    call store_ssg_ptr
    jp ssg_step


ssg_set_volume:
    ld a,(hl)
    inc hl
    and $0F
    ld (VAR_SSG_VOL),a
    call store_ssg_ptr
    call ssg_apply_standalone_volume
    jp ssg_step


ssg_set_preset:
    ld a,(hl)
    inc hl
    and $0F
    ld (VAR_SSG_PRESET),a
    call store_ssg_ptr
    call ssg_apply_preset
    jp ssg_step


ssg_rest:
    ld a,(hl)
    inc hl
    ld (VAR_SSG_WAIT),a
    call store_ssg_ptr
    jp init_ssg


ssg_loop:
    call init_ssg
    ld a,(VAR_SSG_START_LO)
    ld l,a
    ld a,(VAR_SSG_START_HI)
    ld h,a
    xor a
    ld (VAR_SSG_WAIT),a
    call store_ssg_ptr
    jp ssg_step


ssg_stop:
    xor a
    ld (VAR_SSG_ACTIVE),a
    ld (VAR_SSG_WAIT),a
    jp init_ssg


store_ssg_ptr:
    ld a,l
    ld (VAR_SSG_PTR_LO),a
    ld a,h
    ld (VAR_SSG_PTR_HI),a
    ret


ssg_apply_preset:
    ; Load SSG preset from ssg_preset_table.
    ; Preset format:
    ; byte 0 = YM2610 SSG mixer register $07 tone/noise mask
    ; byte 1 = volume A
    ; byte 2 = volume B
    ; byte 3 = volume C
    ; byte 4 = noise frequency $06
    ; byte 5 = reserved

    ld a,(VAR_SSG_PRESET)
    cp SSG_PRESET_COUNT
    jr c,ssg_preset_index_ok
    xor a

ssg_preset_index_ok:
    ld b,a
    ld hl,ssg_preset_table

    ld a,b
    or a
    jr z,ssg_preset_ready

ssg_preset_seek_loop:
    ld de,SSG_PRESET_SIZE
    add hl,de
    dec a
    jr nz,ssg_preset_seek_loop

ssg_preset_ready:
    ; mixer $07
    ld d,$07
    ld e,(hl)
    call ssg_preset_write_a
    inc hl

    ; volume A $08 (FIX: Ensure M=0 mode)
    ld d,$08
    ld a,(hl)
    and $0F         ; Ensure M bit is 0 for fixed amplitude
    ld e,a
    call ssg_preset_write_a
    inc hl

    ; volume B $09 (FIX: Ensure M=0 mode)
    ld d,$09
    ld a,(hl)
    and $0F         ; Ensure M bit is 0 for fixed amplitude
    ld e,a
    call ssg_preset_write_a
    inc hl

    ; volume C $0A (FIX: Ensure M=0 mode)
    ld d,$0A
    ld a,(hl)
    and $0F         ; Ensure M bit is 0 for fixed amplitude
    ld e,a
    call ssg_preset_write_a
    inc hl

    ; noise frequency $06
    ld d,$06
    ld e,(hl)
    call ssg_preset_write_a
    inc hl

    ; skip reserved
    inc hl
    ret


ssg_preset_write_a:
    push hl
    call shadowed_write_a
    pop hl
    ret


ssg_note_on:
    ; FIX: SSG note-on with proper mixer settings
    ld a,b
    ld c,0
ssg_octave_loop:
    cp 12
    jr c,ssg_note_index
    sub 12
    inc c
    jr ssg_octave_loop

ssg_note_index:
    add a,a
    ld e,a
    ld d,0
    ld hl,ssg_period_table
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld a,c
    cp 5
    jr z,ssg_period_ready
    jr c,ssg_shift_left
ssg_shift_right:
    sub 5
    jr z,ssg_period_ready
ssg_shift_right_loop:
    srl d
    rr e
    dec a
    jr nz,ssg_shift_right_loop
    jr ssg_period_ready
ssg_shift_left:
    ld a,5
    sub c
    jr z,ssg_period_ready
ssg_shift_left_loop:
    sla e
    rl d
    dec a
    jr nz,ssg_shift_left_loop
ssg_period_ready:
    ; FIX: Clamp period to 12-bit max
    ld a,d
    and $0F
    ld d,a
    
    push de
    ld d,$00
    call shadowed_write_a
    pop de
    push de
    ld e,d
    ld d,$01
    call shadowed_write_a
    pop de
    sla e
    rl d
    push de
    ld d,$02
    call shadowed_write_a
    pop de
    ld e,d
    ld d,$03
    call shadowed_write_a
    
    ; FIX: Enable tones on all channels, disable noise
    ld de,$0738    ; Was $073C - FIXED: Enable tones (bits 3-5=0), disable noise (bits 0-2=1)
    call shadowed_write_a
    
    ld d,$08
    ld a,(VAR_MUSIC_VOL)
    and $0F         ; FIX: Ensure M=0 mode
    ld e,a
    call shadowed_write_a
    ld d,$09
    ld a,(VAR_MUSIC_VOL)
    and $0F         ; FIX: Ensure M=0 mode
    srl a
    add a,3
    ld e,a
    jp shadowed_write_a

ssg_standalone_note_on:
    ; Same as ssg_note_on but uses VAR_SSG_VOL for channel volumes.
    call ssg_note_on
    ld d,$08
    ld a,(VAR_SSG_VOL)
    and $0F         ; FIX: Ensure M=0 mode
    ld e,a
    call shadowed_write_a
    ld d,$09
    ld a,(VAR_SSG_VOL)
    and $0F         ; FIX: Ensure M=0 mode
    srl a
    add a,3
    ld e,a
    jp shadowed_write_a

channel_masks:
    .db $01, $02, $04, $08, $10, $20

adpcma_stop_ticks:
    .db 24, 34, 70, 42, 12, 38, 32, 38, 56, 32, 44, 30

ssg_period_table:
    .dw $01DE, $01C3, $01AA, $0192, $017B, $0166
    .dw $0152, $013F, $012E, $011D, $010E, $00FF

.include "fm_patch_table.inc"
.include "fm_data.inc"
.include "music_data.inc"
.include "ssg_config.inc"
.include "ssg_data.inc"
.include "sample_table.inc"