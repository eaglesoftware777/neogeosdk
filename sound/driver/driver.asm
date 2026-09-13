;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Neo Geo YM2610 sound driver 

;;   - SSG period table: accurate A=440Hz tuning for 8MHz/64 master divider
;;   - SSG mixer: correct bit layout per YM2149/YM2610 spec
;;   - SSG volume: M bit (bit 4) always cleared for fixed amplitude mode
;;   - SSG channel C: now used for detuned unison (richer tone)
;;   - FM init: LFO off, all channels key-off, all TL to $7F
;;   - FM note-on: key-off before frequency latch, then key-on (envelope retrigger)
;;   - FM note-off: clean key-off only (no volume ramp variable)
;;   - FM frequency latch: high byte first ($A5), then low byte ($A1) per spec
;;   - FM patch: operator base registers corrected to $31/$35/$39/$3D
;;   - FM TL volume: global volume offset with $7F clamp
;;   - ADPCM-A: flag reset before playback ($1C stop then $1C00 clear)
;;   - ADPCM-B: proper reset sequence (reset, clear flag, release, program, start)
;;   - init_ssg: full register clear including noise, envelope period, envelope shape
;;   - fm_silence_all: key-off all channels + set all operator TL to $7F
;;   - music_rest: silence SSG channels during rest
;;   - Tempo: independent fractional BPM clocks, with legacy raw-period control
;;   - All external includes preserved at end of file
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
.define VAR_CLOCK_PHASE    $FE2C
.define VAR_LEGACY_TICK    $FE2D
.define VAR_FM_PHASE       $FE2E ; word
.define VAR_SSG_PHASE      $FE30 ; word
.define VAR_MUSIC_PHASE    $FE32 ; word
.define VAR_FM_BPM         $FE34
.define VAR_SSG_BPM        $FE35
.define VAR_MUSIC_BPM      $FE36
.define VAR_FM_LFO_VALUE   $FE37
.define VAR_FM_LFO_LOCK    $FE38
.define VAR_FM_TEMPO_LOCK  $FE39
.define VAR_SSG_TEMPO_LOCK $FE3A
.define VAR_SSG_ENVELOPE   $FE3B
.define VAR_SSG_MIXER      $FE3C
.define VAR_SSG_LEVEL_A    $FE3D
.define VAR_SSG_LEVEL_B    $FE3E
.define VAR_SSG_LEVEL_C    $FE3F
.define VAR_FM_CARRIERS    $FE40
.define VAR_ADPCMB_REPEAT  $FE41

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
    ld a,(SHADOW_A+$27)
    and $CF
    or $20
    ld e,a
    ld d,$27
    call force_write_a
    call ticker_update
    pop hl
    pop de
    pop bc
    pop af
    ei
    reti
irq_end:
.assert irq_end <= $0066

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
    call reset_sequence_clocks

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
    ; Commands and timer IRQs share YM address latches and sequencer pointers.
    ; NMI only queues bytes, so it can stay enabled throughout this transaction.
    di
    call execute_command
    ei
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
    jp z,exec_p_tempo
    cp 2
    jp z,exec_p_adpcma
    cp 3
    jp z,exec_p_adpcmb
    cp 4
    jp z,exec_p_ssg
    cp 5
    jp z,exec_p_fadeout
    cp 6
    jp z,exec_p_fadein
    cp 7
    jp z,exec_p_fmtrack
    cp 8
    jp z,exec_p_fmvol
    cp 9
    jp z,exec_p_ssgtrack
    cp 10
    jp z,exec_p_ssgpreset
    cp 11
    jp z,exec_p_adpcmb_pan
    cp 12
    jp z,exec_p_fm_lfo
    cp 13
    jp z,exec_p_ssg_noise
    cp 14
    jp z,exec_p_fm_tempo
    cp 15
    jp z,exec_p_fm_csm_begin
    cp 16
    jp z,exec_p_adpcma_sample
    cp 17
    jp z,exec_p_fm_csm_patch
    cp 18
    jp z,exec_p_fm_bpm
    cp 19
    jp z,exec_p_ssg_bpm
    cp 20
    jp z,exec_p_adpcmb_repeat
    ret
exec_p_tempo:
    ld a,c
    ld (VAR_TEMPO),a
    xor a
    ld (VAR_MUSIC_BPM),a
    ld (VAR_TICK),a
    ret
exec_p_adpcma_sample:
    ld a,c
    jp play_adpcma_index
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
    ld (VAR_SSG_VOL),a
    ld a,(VAR_SSG_ACTIVE)
    or a
    jp nz,ssg_apply_standalone_volume
    jp apply_music_volume
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

; --- ADPCM-B L/R pan (reg $11, active-high) ---
;   $C0 = stereo, $80 = L only, $40 = R only, $00 = mute
exec_p_adpcmb_pan:
    ld a,c
    and $C0
    ld d,$11
    ld e,a
    jp force_write_a

; --- FM LFO control (reg $22) ---
;   bit 3 = enable, bits 0..2 = rate
exec_p_fm_lfo:
    ld a,c
    and $0F
    ld (VAR_FM_LFO_VALUE),a
    ld d,$22
    ld e,a
    ld a,1
    ld (VAR_FM_LFO_LOCK),a
    jp force_write_a

; Repeat is latched when the next ADPCM-B sample starts.
exec_p_adpcmb_repeat:
    xor a
    bit 0,c
    jr z,exec_p_adpcmb_repeat_store
    ld a,$10
exec_p_adpcmb_repeat_store:
    ld (VAR_ADPCMB_REPEAT),a
    ret

; --- SSG noise period (reg $06, 5 bits) ---
exec_p_ssg_noise:
    ld a,c
    and $1F
    ld d,$06
    ld e,a
    jp shadowed_write_a

; --- FM tempo override (writes VAR_FM_TEMPO directly) ---
; Raw value 1..8 = legacy 69.4 ms units per music step (1 = fastest).
; This is the live equivalent of the MML F0 directive but takes the
; cooked period directly so the SDK caller doesn't need to call
; tempo_to_frames.
exec_p_fm_tempo:
    ld a,c
    or a
    jr nz,exec_p_fm_tempo_ok
    ld a,1
exec_p_fm_tempo_ok:
    cp 9
    jr c,exec_p_fm_tempo_range_ok
    ld a,8
exec_p_fm_tempo_range_ok:
    ld (VAR_FM_TEMPO),a
    xor a
    ld (VAR_FM_TICK),a
    ld (VAR_FM_BPM),a
    ld a,1
    ld (VAR_FM_TEMPO_LOCK),a
    ret

exec_p_fm_bpm:
    ld a,c
    or a
    jr nz,exec_p_fm_bpm_valid
    inc a
exec_p_fm_bpm_valid:
    ld (VAR_FM_BPM),a
    ld a,1
    ld (VAR_FM_TEMPO_LOCK),a
    ld hl,0
    ld (VAR_FM_PHASE),hl
    ret

exec_p_ssg_bpm:
    ld a,c
    or a
    jr nz,exec_p_ssg_bpm_valid
    inc a
exec_p_ssg_bpm_valid:
    ld (VAR_SSG_BPM),a
    ld a,1
    ld (VAR_SSG_TEMPO_LOCK),a
    ld hl,0
    ld (VAR_SSG_PHASE),hl
    ret

; --- FM CSM (Composite Sine Mode) ---
; Cmd $1B begin (1-byte param = Timer A high byte, 0..255):
;   writes reg $24 = byte (Timer A MSB, bits 9..2 of 10-bit period)
;          reg $25 = 0    (Timer A LSB, bits 1..0 of 10-bit period)
;          reg $27 = $CB  (MODE=11 CSM + Enable B IRQ + Load B + Load A)
; ENA (Enable A IRQ, bit 2) is DELIBERATELY zero — enabling it floods
; the Z80 with Timer A interrupts at the auto-key rate (hundreds to
; thousands per second) and starves the music engine.  The chip
; still auto-keys ch3 from Timer A overflows regardless of IRQ
; enable; we just don't need the Z80 to know about every tick.
; Bits 3+1 (Enable B + Load B) preserve Timer B so the music engine
; keeps stepping FM/SSG/ADPCM-B tracks while CSM runs.
exec_p_fm_csm_begin:
    ld d,$24
    ld e,c
    call force_write_a
    ld d,$25
    ld e,0
    call force_write_a
    ld d,$27
    ld e,$CB
    jp force_write_a

set_fm_csm_begin_wait:
    ld a,15
    ld (VAR_WAIT_TEMPO),a
    ld (VAR_PARAM_MODE),a
    ret

set_fm_csm_patch_wait:
    ld a,17
    ld (VAR_WAIT_TEMPO),a
    ld (VAR_PARAM_MODE),a
    ret

; Cmd $1C end (immediate, no parameter).
; Single $27 write to leave CSM cleanly:
;   $1A = Reset A + Enable B IRQ + Load B
;         (MODE=00 — no CSM, Load A=0 halts Timer A, Timer B kept alive)
; This avoids the "halt-then-restart Timer B" gap that previous
; versions had — Timer B never stops, so the music engine never
; misses a tick at section transitions.
; Then key-off ch3 and slam its 4 operator TLs to $7F (full
; attenuation) so any envelope-release tail from the rapid CSM
; auto-keys can't bleed audibly into the next section.
exec_csm_end:
    ld d,$27
    ld e,$1A
    call force_write_a
    ld d,$28
    ld e,$02
    call force_write_a
    ld d,$42
    ld e,$7F
    call force_write_a
    ld d,$46
    ld e,$7F
    call force_write_a
    ld d,$4A
    ld e,$7F
    call force_write_a
    ld d,$4E
    ld e,$7F
    jp force_write_a

; --- FM CSM voice load (writes to FM channel 3's own registers) ---
; Cmd $1D (1-byte param = patch index into fm_patch_table).
;
; CSM's Timer-A auto-key only ever triggers channel 3 (pitch regs
; $A2/$A6, tone regs $B2/$B6/$22/$32/$36/$3A/$3E) — but every other FM
; routine in this driver (fm_apply_patch / fm_note_on, used by
; playFMTrack) writes channel 2's registers instead ($B1/$B5/$31/$35/
; $39/$3D, key-on $28=$F1).  A CSM effect that calls playFMTrack() to
; "give the channel something to buzz" therefore loads a patch onto
; the wrong channel; channel 3 is left with whatever it last had,
; which since nothing else in this driver ever writes it is the
; power-on state — no patch and a frequency latch of 0 (silence).
; This loads the patch directly onto channel 3 and latches a base
; pitch so CSM's auto key pulses have an actual instrument and note
; to sound.
exec_p_fm_csm_patch:
    ld a,c
    cp FM_PATCH_COUNT
    jr c,fm_csm_patch_index_ok
    xor a
fm_csm_patch_index_ok:
    ld b,a
    ld hl,fm_patch_table
    ld a,b
    or a
    jr z,fm_csm_patch_ready
fm_csm_patch_seek_loop:
    ld de,FM_PATCH_SIZE
    add hl,de
    dec a
    jr nz,fm_csm_patch_seek_loop
fm_csm_patch_ready:
    ; LFO register $22 (chip-global, shared with channel 2 patches)
    ld d,$22
    ld e,(hl)
    call fm_patch_write_a
    inc hl
    ; Feedback/algorithm $B2 (channel 3, vs. $B1 for channel 2)
    ld d,$B2
    ld e,(hl)
    call fm_patch_write_a
    inc hl
    ; L/R + AMS/PMS $B6 (channel 3, vs. $B5 for channel 2)
    ld d,$B6
    ld e,(hl)
    call fm_patch_write_a
    inc hl
    ; Operators — channel 3 bases (channel 2 uses $31/$35/$39/$3D)
    ld b,$32
    call fm_write_operator_patch
    ld b,$36
    call fm_write_operator_patch
    ld b,$3A
    call fm_write_operator_patch
    ld b,$3E
    call fm_write_operator_patch
    ; Latch a base pitch on channel 3 — F-Num2/Block high byte first,
    ; then F-Num1 low byte, per spec (same ordering fm_note_on uses)
    ; to avoid a glitch on the low-byte write.  Table index 24 is a
    ; mid-range carrier note; the CSM "voice" character comes from
    ; the Timer-A sweep on top of it, not from this base pitch.
    ld hl,fm_note_table+48
    ld c,(hl)
    inc hl
    ld a,(hl)
    ld e,a
    ld d,$A6
    call force_write_a
    ld e,c
    ld d,$A2
    jp force_write_a

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
    jp z,set_adpcma_volume_wait
    cp $06 ; ADPCM-B volume parameter follows
    jp z,set_adpcmb_volume_wait
    cp $07 ; SSG/MML volume parameter follows
    jp z,set_ssg_volume_wait
    cp $0A ; Fade out speed parameter follows
    jp z,set_fadeout_wait
    cp $0C ; ADPCM-A stop
    jp z,adpcma_stop
    cp $0D ; ADPCM-B stop
    jp z,adpcmb_stop
    cp $0E ; Tempo Wait
    jp z,set_tempo_wait
    cp $0F ; Stop SSG / music only
    jp z,stop_music
    cp $11 ; Stop fade out
    jp z,cancel_fade
    cp $12 ; Fade in speed parameter follows
    jp z,set_fadein_wait
    cp $13 ; FM volume parameter follows
    jp z,set_fmvol_wait
    cp SSG_CMD_PRESET ; SSG preset parameter follows
    jp z,set_ssgpreset_wait
    cp $30 ; FM debug tone
    jp z,play_fm_demo
    cp $31 ; FM track select parameter follows
    jp z,set_fmtrack_wait
    cp SSG_CMD_PLAY ; SSG track select parameter follows
    jp z,set_ssgtrack_wait
    cp $15 ; ADPCM-B L/R pan parameter follows
    jp z,set_adpcmb_pan_wait
    cp $16 ; ADPCM-A full sample index parameter follows
    jp z,set_adpcma_sample_wait
    cp $17 ; FM LFO enable+rate parameter follows
    jp z,set_fm_lfo_wait
    cp $18 ; ADPCM-B repeat for subsequent samples (0/1)
    jp z,set_adpcmb_repeat_wait
    cp $19 ; SSG noise period parameter follows
    jp z,set_ssg_noise_wait
    cp $1A ; FM tempo (raw Timer-B period) parameter follows
    jp z,set_fm_tempo_wait
    cp $1B ; FM CSM begin — Timer-A high byte parameter follows
    jp z,set_fm_csm_begin_wait
    cp $1C ; FM CSM end — no parameter
    jp z,exec_csm_end
    cp $1D ; FM CSM voice load — patch index parameter follows
    jp z,set_fm_csm_patch_wait
    cp $1E ; FM beats per minute
    jp z,set_fm_bpm_wait
    cp $1F ; SSG beats per minute
    jp z,set_ssg_bpm_wait
    cp $28 ; ADPCM-B direct sample 0
    jp z,play_demo_b0
    cp $29 ; ADPCM-B direct sample 1
    jp z,play_demo_b1

    cp SFX_B_BASE
    jp nc,play_adpcmb_cmd
    cp SFX_A_BASE
    jp nc,play_adpcma_cmd
    cp MUSIC_BASE
    jp nc,play_fm_cmd
    ret

set_adpcmb_repeat_wait:
    ld a,20
    jr set_bpm_wait
set_fm_bpm_wait:
    ld a,18
    jr set_bpm_wait
set_ssg_bpm_wait:
    ld a,19
set_bpm_wait:
    ld (VAR_WAIT_TEMPO),a
    ld a,1
    ld (VAR_PARAM_MODE),a
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

set_adpcmb_pan_wait:
    ld a,11
    ld (VAR_WAIT_TEMPO),a
    ld (VAR_PARAM_MODE),a
    ret

set_adpcma_sample_wait:
    ld a,16
    ld (VAR_WAIT_TEMPO),a
    ld (VAR_PARAM_MODE),a
    ret

set_fm_lfo_wait:
    ld a,12
    ld (VAR_WAIT_TEMPO),a
    ld (VAR_PARAM_MODE),a
    ret

set_ssg_noise_wait:
    ld a,13
    ld (VAR_WAIT_TEMPO),a
    ld (VAR_PARAM_MODE),a
    ret

set_fm_tempo_wait:
    ld a,14
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

;;; FIX: apply_music_volume sets channels A, B, C with proper volume
;;; Channel A = full volume, Channel B = half + offset for detuned unison,
;;; Channel C = lower for subtle thickness
;;; All channels: M bit (bit 4) MUST be 0 for fixed amplitude mode per YM2149 spec
apply_music_volume:
    ld a,(VAR_MUSIC_VOL)
    or a
    jp z,ssg_silence_channels
    ld d,$08
    ld a,(VAR_MUSIC_VOL)
    and $0F
    ld e,a
    call shadowed_write_a
    ld d,$09
    ld a,(VAR_MUSIC_VOL)
    and $0F
    srl a
    add a,2
    cp $10
    jr c,apply_music_vol_b_ok
    ld a,$0F
apply_music_vol_b_ok:
    ld e,a
    call shadowed_write_a
    ld d,$0A
    ld a,(VAR_MUSIC_VOL)
    and $0F
    srl a
    srl a
    add a,1
    cp $10
    jr c,apply_music_vol_c_ok
    ld a,$0F
apply_music_vol_c_ok:
    ld e,a
    jp shadowed_write_a

;;; FIX: ssg_apply_standalone_volume sets channels A, B, C for standalone SSG
ssg_apply_standalone_volume:
    ld d,$07
    ld a,(VAR_SSG_MIXER)
    ld e,a
    call shadowed_write_a
    ld a,(VAR_SSG_VOL)
    and $0F
    jp z,ssg_silence_channels
    ld c,a
    ld a,(VAR_SSG_LEVEL_A)
    ld b,a
    ld hl,VAR_SSG_LEVEL_A
    ld d,$08
ssg_volume_channel:
    ld a,(hl)
    or a
    jr z,ssg_volume_ready
    sub b
    add a,c
    jp m,ssg_volume_zero
    cp $10
    jr c,ssg_volume_ready
    ld a,$0F
    jr ssg_volume_ready
ssg_volume_zero:
    xor a
ssg_volume_ready:
    ld e,a
    ld a,d
    cp $08
    jr nz,ssg_volume_write
    ld a,(VAR_SSG_ENVELOPE)
    or a
    jr z,ssg_volume_write
    ld e,$10
ssg_volume_write:
    push hl
    call shadowed_write_a
    pop hl
    inc hl
    inc d
    ld a,d
    cp $0B
    jr nz,ssg_volume_channel
    ret

ssg_silence_channels:
    ld de,$0800
    call shadowed_write_a
    ld de,$0900
    call shadowed_write_a
    ld de,$0A00
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
    call fm_silence_all
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

;;; FIX: Full SSG initialization per YM2149 spec
;;; Register $07 bit layout:
;;;   bit 0 = /Tone A enable (0=on, 1=off)
;;;   bit 1 = /Tone B enable
;;;   bit 2 = /Tone C enable
;;;   bit 3 = /Noise A enable (0=on, 1=off)
;;;   bit 4 = /Noise B enable
;;;   bit 5 = /Noise C enable
;;;   $3F = all tones off, all noise off
init_ssg:
    ld de,$073F ; All tones off, all noise off
    call shadowed_write_a
    ld de,$0600 ; Noise period = 0
    call shadowed_write_a
    ld de,$0800 ; Channel A volume = 0, M=0
    call shadowed_write_a
    ld de,$0900 ; Channel B volume = 0, M=0
    call shadowed_write_a
    ld de,$0A00 ; Channel C volume = 0, M=0
    call shadowed_write_a
    ld de,$0B00 ; Envelope period low = 0
    call shadowed_write_a
    ld de,$0C00 ; Envelope period high = 0
    call shadowed_write_a
    ld de,$0D00 ; Envelope shape = 0
    call shadowed_write_a
    ; FIX: Zero all tone period registers to prevent stale frequencies
    ld de,$0000 ; Channel A fine tune = 0
    call shadowed_write_a
    ld de,$0100 ; Channel A coarse tune = 0
    call shadowed_write_a
    ld de,$0200 ; Channel B fine tune = 0
    call shadowed_write_a
    ld de,$0300 ; Channel B coarse tune = 0
    call shadowed_write_a
    ld de,$0400 ; Channel C fine tune = 0
    call shadowed_write_a
    ld de,$0500 ; Channel C coarse tune = 0
    call shadowed_write_a
    ret

;;; FIX: FM initialization with proper Timer B setup and full silence
init_fm:
    ; Timer B period: controls IRQ rate
    ; Timer B divides the 8 MHz master clock by 144 * 16 * (256-N).
    ld de,$26E4 ; 8 MHz / (144 * 16 * 28) = 124.008 Hz
    call shadowed_write_a
    ; Reset flags, enable Timer B IRQ, load Timer B
    ; Reg $27: bit5=ResetB, bit3=EnableB_IRQ, bit1=LoadB
    ld de,$272A
    call force_write_a
    ; Clear reset bits, keep enable+load
    ld de,$273A
    call shadowed_write_a

    ; Silence all FM channels
    call fm_silence_all

    ; LFO off
    ld de,$2200
    call shadowed_write_a

    ret

;;; FIX: Properly silence all 4 FM channels
;;; YM2610 has four FM channels with key-codes $01,$02,$05,$06.
;;; Key-on register $28: bits 4-7 = slot mask, bits 0-2 = channel
;;; Writing $00+channel = key off all slots for that channel
fm_silence_all:
    ; The base YM2610 omits the OPN channels addressed by $00 and $04.
    ld de,$2806
    call force_write_a
    ld de,$2801
    call force_write_a
    ld de,$2802
    call force_write_a
    ld de,$2805
    call force_write_a

    ; Set all operator Total Level to $7F (maximum attenuation = silence)
    ; TL registers: Port A $41-$4E, Port B $41-$4E
    ; Operators for ch1: $41,$45,$49,$4D  ch2: $42,$46,$4A,$4E
    ; Port A covers channels 1,2  Port B covers channels 3,4
    ld d,$41
    ld e,$7F
fm_silence_tl_loop:
    call force_write_a
    call force_write_b
    inc d
    ld a,d
    cp $4E
    jr c,fm_silence_tl_loop
    call force_write_a   ; write the final channel's $4E carrier
    call force_write_b

    ret

init_adpcma:
    call adpcma_stop
    ; Set ADPCM-A master volume: reg $01 Port B
    ld d,$01
    ld a,(VAR_ADPCMA_VOL)
    ld e,a
    call force_write_b
    ; Set all 6 ADPCM-A channel L/R + volume: regs $08-$0D Port B
    ld b,6
    ld c,$08
adpcma_vol_loop:
    ld d,c
    ld e,$DF
    call force_write_b
    inc c
    djnz adpcma_vol_loop
    ret

;;; FIX: ADPCM-B init with proper reset sequence per Yamaha spec
init_adpcmb:
    ; Step 1: Assert reset
    ld de,$1001
    call force_write_a
    ; Step 2: Clear ADPCM-B end flag via flag control register
    ld de,$1C80
    call force_write_a
    ld de,$1C00
    call force_write_a
    ; Step 3: Release reset before programming registers
    ld de,$1000
    call force_write_a
    ; Step 4: Enable Left + Right output
    ld de,$11C0
    call force_write_a
    ; Step 5: Set default sample rate (Delta-N for ~16kHz)
    ; Delta-N = round(rate * 65536 * 144 / 8000000).
    ld de,$19BA ; Delta-N low byte
    call force_write_a
    ld de,$1A49 ; Delta-N high byte
    call force_write_a
    ; Step 6: Set volume
    ld d,$1B
    ld a,(VAR_ADPCMB_VOL)
    ld e,a
    call force_write_a
    ret

stop_all:
    call reset_sequence_clocks
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
    call fm_silence_all
    ; Clear CSM mode but keep Timer B running — Timer B is the music
    ; engine's tick source.  Writing $27=$00 (or anything with bit 1
    ; clear) halts Timer B and freezes every subsequent FM/SSG note
    ; advance.  $3A matches init_fm's resting value: MODE=00 (no
    ; CSM/FM3), Timer B enable+load on (bits 3,1), Timer A halted,
    ; and both overflow flags cleared (bits 5,4 as write-1-to-clear).
    ld d,$27
    ld e,$3A
    call force_write_a
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
    ; The queue is left alone.  This command was itself taken out of it,
    ; and anything queued behind it was sent after it on purpose; zeroing
    ; the pointers here would throw those bytes away.  Cold boot clears the
    ; queue when it clears work RAM.
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

;;; FIX: ADPCM-A stop: dump all 6 channels
;;; Register $00 bit 7 = dump mode, bits 0-5 = channel mask
;;; $BF = dump + all 6 channels
adpcma_stop:
    ; Stop all 6 ADPCM-A channels: reg $00 Port B, data = $BF (dump + all channels)
    ld de,$00BF
    jp force_write_b

;;; FIX: ADPCM-B stop: assert reset bit
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
    ld (VAR_TICK),a        ; FIX: Reset tick counter for clean start
    ld (VAR_MUSIC_PHASE),a
    ld (VAR_MUSIC_PHASE+1),a
    ld a,120
    ld (VAR_MUSIC_BPM),a
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
    ld b,a          ; preserve track index (fm_stop clobbers A via xor a + jp)
    call fm_stop
    ld a,b          ; restore track index
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
    ld (VAR_FM_LFO_LOCK),a
    ld (VAR_FM_TEMPO_LOCK),a
    ld (VAR_FM_PHASE),a
    ld (VAR_FM_PHASE+1),a
    ld a,120
    ld (VAR_FM_BPM),a
    ld a,1
    ld (VAR_FM_ACTIVE),a
    xor a
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
    ld hl,channel_masks
    ld e,c
    ld d,0
    add hl,de
    ld a,(hl)
    ld (VAR_COMMAND),a
    or $80
    ld e,a
    ld d,0
    call force_write_b
    ld a,(VAR_COMMAND)
    ld e,a
    ld d,$1C
    call force_write_a
    ld de,$1C00
    call force_write_a
    ld a,(VAR_ADPCMA_VOL)
    ld e,a
    ld d,$01
    call force_write_b
    ld a,c
    add a,$08
    ld d,a
    ld e,$DF
    call force_write_b

    ; BC was pushed as start-low/start-high. Preserve addresses across writes.
    pop hl
    ld a,c
    add a,$10
    ld d,a
    ld e,h
    push hl
    call force_write_b
    pop hl
    ld a,c
    add a,$18
    ld d,a
    ld e,l
    call force_write_b
    pop hl
    ld a,c
    add a,$20
    ld d,a
    ld e,l
    push hl
    call force_write_b
    pop hl
    ld a,c
    add a,$28
    ld d,a
    ld e,h
    call force_write_b

    ; Hardware end addresses stop each voice independently, without timeouts.
    xor a
    ld (VAR_ADPCMA_TICKS),a
    ld d,a
    ld a,(VAR_COMMAND)
    ld e,a
    jp force_write_b

play_adpcmb_cmd:
    sub SFX_B_BASE
play_adpcmb_index:
    cp ADPCMB_COUNT
    ret nc
    push af
    add a,ADPCMA_COUNT
    call get_sample_ptr
    push hl

    ; FIX: Full ADPCM-B reset sequence before playback
    ; Step 1: Assert reset
    ld de,$1001
    call force_write_a
    ; Step 2: Clear end flag
    ld de,$1C80
    call force_write_a
    ld de,$1C00
    call force_write_a

    ; Step 3: Set start address (MUST force_write - shadow skip breaks re-trigger)
    ld d,$12
    ld e,b
    call force_write_a
    ld d,$13
    ld e,c
    call force_write_a

    ; Step 4: Set end address
    pop hl
    ld c,h
    ld d,$14
    ld e,l
    call force_write_a
    ld d,$15
    ld e,c
    call force_write_a

    ; Step 5: Set volume (force_write to ensure hardware gets it)
    pop af
    add a,a
    ld e,a
    ld d,0
    ld hl,adpcmb_delta_n_table
    add hl,de
    ld e,(hl)
    inc hl
    ld c,(hl)
    ld d,$19
    call force_write_a
    ld d,$1A
    ld e,c
    call force_write_a

    ld d,$1B
    ld a,(VAR_ADPCMB_VOL)
    ld e,a
    call force_write_a

    ; Step 6: Release reset
    ld de,$1000
    call force_write_a

    ; Step 6b: Ensure L+R outputs are enabled.  Reg $11 (active-high
    ; pan) defaults indeterminate after some chip resets; explicitly
    ; writing $C0 (L on + R on) prevents the "mute despite playback"
    ; failure mode.  soundSetADPCMBPan can override this at any time.
    ld de,$11C0
    call force_write_a

    ; Step 7: Start playback
    ld a,(VAR_ADPCMB_REPEAT)
    or $80
    ld e,a
    ld d,$10
    call force_write_a
    ret

play_ssg_index:
    cp SSG_TRACK_COUNT
    ret nc
    ld b,a          ; preserve track index (init_ssg clobbers A via shadowed writes)
    call init_ssg
    ld a,b          ; restore track index
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
    ld (VAR_SSG_TEMPO_LOCK),a
    ld (VAR_SSG_ENVELOPE),a
    ld (VAR_SSG_PHASE),a
    ld (VAR_SSG_PHASE+1),a
    ld a,120
    ld (VAR_SSG_BPM),a
    ld a,1
    ld (VAR_SSG_ACTIVE),a
    xor a
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
    ld e,a
    ld d,0
    ld hl,sample_address_table
    add hl,de
    add hl,de
    add hl,de
    add hl,de
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

;;; FIX: ticker_update with proper ordering
ticker_update:
    ; Keep fades and timed effects on the previous 241-unit timer period.
    xor a
    ld (VAR_LEGACY_TICK),a
    ld a,(VAR_CLOCK_PHASE)
    add a,28
    jr c,ticker_legacy_due
    cp 241
    jr c,ticker_clock_store
ticker_legacy_due:
    sub 241
    ld b,a
    ld a,1
    ld (VAR_LEGACY_TICK),a
    ld a,b
ticker_clock_store:
    ld (VAR_CLOCK_PHASE),a
    ld a,(VAR_FM_ACTIVE)
    or a
    call nz,fm_tick
    ld a,(VAR_SSG_ACTIVE)
    or a
    call nz,ssg_tick
    ld a,(VAR_MUSIC_ACTIVE)
    or a
    call nz,music_clock_tick
    ld a,(VAR_LEGACY_TICK)
    or a
    ret z
    ; Handle timed auto-stop for FM SFX
    ld a,(VAR_FM_TICKS)
    or a
    jr z,ticker_adpcma
    dec a
    ld (VAR_FM_TICKS),a
    jr nz,ticker_adpcma
    call fm_stop

ticker_adpcma:
    ; Handle timed auto-stop for ADPCM-A SFX
    ld a,(VAR_ADPCMA_TICKS)
    or a
    jr z,ticker_music
    dec a
    ld (VAR_ADPCMA_TICKS),a
    jr nz,ticker_music
    call adpcma_stop

ticker_music:
    ; Legacy fade divider is independent of the high-resolution music clock.
    ld a,(VAR_TICK)
    inc a
    ld (VAR_TICK),a
    ld b,a
    ld a,(VAR_TEMPO)
    or a
    ret z
    cp b
    ret nz
    xor a
    ld (VAR_TICK),a

    ; Tick fade engine
    call fade_tick
    ret

reset_sequence_clocks:
    push bc
    push hl
    xor a
    ld hl,VAR_CLOCK_PHASE
    ld b,VAR_ADPCMB_REPEAT-VAR_CLOCK_PHASE+1
reset_sequence_clocks_loop:
    ld (hl),a
    inc hl
    djnz reset_sequence_clocks_loop
    pop hl
    pop bc
    ret

; A=BPM, HL=phase; return HL=new phase and carry on a 1/12-beat step.
; 124.007936 Hz * 60 / 12 = 620.03968: under 0.007% tempo error.
tempo_clock_step:
    ld e,a
    ld d,0
    add hl,de
    ld de,620
    or a
    sbc hl,de
    jr nc,tempo_clock_due
    add hl,de
    or a
    ret
tempo_clock_due:
    scf
    ret

music_clock_tick:
    ld a,(VAR_MUSIC_BPM)
    or a
    jr z,music_clock_legacy
    ld hl,(VAR_MUSIC_PHASE)
    call tempo_clock_step
    ld (VAR_MUSIC_PHASE),hl
    ret nc
    jp music_tick
music_clock_legacy:
    ld a,(VAR_LEGACY_TICK)
    or a
    ret z
    ld a,(VAR_TICK)
    inc a
    ld b,a
    ld a,(VAR_TEMPO)
    cp b
    ret nz
    jp music_tick

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
    ; Reload fade interval counter
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
    ; Decrement mixer levels by 16. Each fade interval also includes
    ; VAR_TEMPO legacy ticks; its default divider is three.
    ld a,(VAR_MUSIC_VOL)
    sub 16
    jr nc,fade_out_music_ok
    xor a
fade_out_music_ok:
    ld (VAR_MUSIC_VOL),a
    ld a,(VAR_ADPCMA_VOL)
    sub 16
    jr nc,fade_out_adpcma_ok
    xor a
fade_out_adpcma_ok:
    ld (VAR_ADPCMA_VOL),a
    ld a,(VAR_ADPCMB_VOL)
    sub 16
    jr nc,fade_out_adpcmb_ok
    xor a
fade_out_adpcmb_ok:
    ld (VAR_ADPCMB_VOL),a
fade_out_apply:
    call apply_master_volumes
    ; Check if all volumes reached 0 -> fade complete
    ld a,(VAR_MUSIC_VOL)
    or a
    jr nz,fade_tick_done
    ld a,(VAR_ADPCMA_VOL)
    or a
    jr nz,fade_tick_done
    ld a,(VAR_ADPCMB_VOL)
    or a
    jr nz,fade_tick_done
    ; All at zero, stop fade
    xor a
    ld (VAR_FADE_MODE),a
    ret

fade_in_step:
    ; Increment all volume channels by 16 per step toward base.
    ; Matches the +/-16 used by fade_out for symmetric ramp speed.
    ld a,(VAR_MUSIC_VOL)
    add a,16
    jr nc,fade_in_music_nowrap
    ld a,$FF
fade_in_music_nowrap:
    ld e,a
    ld a,(VAR_MUSIC_VOL_BASE)
    cp e
    jr nc,fade_in_music_keep
    ld e,a
fade_in_music_keep:
    ld a,e
    ld (VAR_MUSIC_VOL),a

    ld a,(VAR_ADPCMA_VOL)
    add a,16
    jr nc,fade_in_adpcma_nowrap
    ld a,$FF
fade_in_adpcma_nowrap:
    ld e,a
    ld a,(VAR_ADPCMA_BASE)
    cp e
    jr nc,fade_in_adpcma_keep
    ld e,a
fade_in_adpcma_keep:
    ld a,e
    ld (VAR_ADPCMA_VOL),a

    ld a,(VAR_ADPCMB_VOL)
    add a,16
    jr nc,fade_in_adpcmb_nowrap
    ld a,$FF
fade_in_adpcmb_nowrap:
    ld e,a
    ld a,(VAR_ADPCMB_BASE)
    cp e
    jr nc,fade_in_adpcmb_keep
    ld e,a
fade_in_adpcmb_keep:
    ld a,e
    ld (VAR_ADPCMB_VOL),a

fade_in_apply:
    call apply_master_volumes
    ; Check if all volumes reached base -> fade complete
    ld a,(VAR_MUSIC_VOL)
    ld e,a
    ld a,(VAR_MUSIC_VOL_BASE)
    cp e
    jr nz,fade_tick_done
    ld a,(VAR_ADPCMA_VOL)
    ld e,a
    ld a,(VAR_ADPCMA_BASE)
    cp e
    jr nz,fade_tick_done
    ld a,(VAR_ADPCMB_VOL)
    ld e,a
    ld a,(VAR_ADPCMB_BASE)
    cp e
    jr nz,fade_tick_done
    ; All at base, stop fade
    xor a
    ld (VAR_FADE_MODE),a
    ret

fade_tick_done:
    ret

;;; Music MML tick handler
music_tick:
    ld a,(VAR_MUSIC_WAIT)
    or a
    jr z,music_step
    dec a
    ld (VAR_MUSIC_WAIT),a
    ret nz
    jp music_step

;;; Music MML step: parse and execute next command(s)
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
    jp z,music_loop
    cp $F0
    jp z,music_set_tempo
    cp $F1
    jp z,music_set_volume
    cp $F2
    jp z,music_play_adpcma
    cp $F3
    jp z,music_play_adpcmb
    cp $F4
    jp z,music_play_fm
    cp $F5
    jp z,music_play_ssg
    cp $F6
    jp z,music_set_ssg_preset
    cp $80
    jp z,music_rest

    ; Normal SSG note: byte = note index, next byte = duration
    ld b,a
    ld a,(hl)
    inc hl
    ld (VAR_MUSIC_WAIT),a
    call store_music_ptr
    jp ssg_note_on

music_set_tempo:
    ld a,(hl)
    inc hl
    ld (VAR_MUSIC_BPM),a
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
    ; Sample address lookup uses HL; keep the following MML event cursor.
    push hl
    call play_adpcma_index
    pop hl
    jp music_step_next
music_play_adpcmb:
    ld a,(hl)
    inc hl
    call store_music_ptr
    push hl
    call play_adpcmb_index
    pop hl
    jp music_step_next
;;; FIX: music_rest silences SSG then waits
music_rest:
    ld a,(hl)
    inc hl
    ld (VAR_MUSIC_WAIT),a
    call store_music_ptr
    ; Silence SSG channels but do not reset mixer (preserve preset state)
    ld de,$0800 ; Channel A volume = 0
    call shadowed_write_a
    ld de,$0900 ; Channel B volume = 0
    call shadowed_write_a
    ld de,$0A00 ; Channel C volume = 0
    call shadowed_write_a
    ret

music_loop:
    ; Silence SSG before looping
    ld de,$0800
    call shadowed_write_a
    ld de,$0900
    call shadowed_write_a
    ld de,$0A00
    call shadowed_write_a
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
    ; Save music stream pointer before starting FM
    call store_music_ptr
    push hl
    call play_fm_index
    pop hl
    jp music_step_next

music_play_ssg:
    ld a,(hl)
    inc hl
    ; Save music stream pointer before starting standalone SSG
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

;;; FM standalone track tick handler
fm_tick:
    ld a,(VAR_FM_BPM)
    or a
    jr z,fm_tick_legacy
    ld hl,(VAR_FM_PHASE)
    call tempo_clock_step
    ld (VAR_FM_PHASE),hl
    ret nc
    jr fm_tick_duration
fm_tick_legacy:
    ld a,(VAR_LEGACY_TICK)
    or a
    ret z
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
fm_tick_duration:
    ld a,(VAR_FM_WAIT)
    or a
    jr z,fm_step
    dec a
    ld (VAR_FM_WAIT),a
    ret nz
    jp fm_step

;;; FM standalone track step: parse and execute next command(s)
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

    ; Normal FM note: byte = note index, next byte = duration
    ld b,a
    ld a,(hl)
    inc hl
    ld (VAR_FM_WAIT),a
    call store_fm_ptr
    call fm_note_on
    ret

fm_set_tempo:
    ld b,(hl)
    inc hl
    ld a,(VAR_FM_TEMPO_LOCK)
    or a
    jr nz,fm_set_tempo_done
    ld a,b
    ld (VAR_FM_BPM),a
    call tempo_to_frames
    ld (VAR_FM_TEMPO),a
fm_set_tempo_done:
    call store_fm_ptr
    jp fm_step

fm_set_volume:
    ld a,(hl)
    inc hl
    and $0F
    ld (VAR_FM_VOL),a
    ld (VAR_FM_VOL_BASE),a
    call store_fm_ptr
    call fm_apply_patch
    jp fm_step

fm_set_patch:
    ld a,(hl)
    inc hl
    and $0F
    ld (VAR_FM_PATCH),a
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

;;; FIX: FM note off - clean key-off for channel 1 (key-code $01)
;;; Register $28: bits 4-7 = slot mask (0 = all off), bits 0-2 = channel
fm_note_off:
    ld de,$2801
    jp force_write_a

;;; FIX: FM stop - key off and silence all operators
fm_stop:
    xor a
    ld (VAR_FM_ACTIVE),a
    ld (VAR_FM_WAIT),a
    ; Key off channel 1
    ld de,$2801
    call force_write_a
    ; Silence all channels to prevent hanging notes
    jp fm_silence_all

;;; FIX: FM patch loader with correct operator register mapping
;;; YM2610 FM register layout for channel 1 (key-code $01):
;;;   OP1=$31, OP2=$35, OP3=$39, OP4=$3D  (DT/MUL)
;;;   +$10 for TL, +$20 for KS/AR, +$30 for AM/DR, +$40 for SR, +$50 for SL/RR, +$60 for SSG-EG
;;; Patch format (31 bytes):
;;;   byte 0  = LFO register $22
;;;   byte 1  = feedback/algorithm ($B1)
;;;   byte 2  = L/R + AMS/PMS ($B5)
;;;   bytes 3-9   = OP1 (DT/MUL, TL, KS/AR, AM/DR, SR, SL/RR, SSG-EG)
;;;   bytes 10-16  = OP2
;;;   bytes 17-23  = OP3
;;;   bytes 24-30  = OP4
fm_apply_patch:
    ld a,(VAR_FM_PATCH)
    cp FM_PATCH_COUNT
    jr c,fm_apply_patch_index_ok
    xor a
fm_apply_patch_index_ok:
    ld b,a
    ld hl,fm_patch_table
    ; Seek: HL += patch_index * FM_PATCH_SIZE
    ld a,b
    or a
    jr z,fm_apply_patch_ready
fm_patch_seek_loop:
    ld de,FM_PATCH_SIZE
    add hl,de
    dec a
    jr nz,fm_patch_seek_loop
fm_apply_patch_ready:
    ; A live override survives patch/volume reloads and loop headers.
    ; Starting another track restores that track's own defaults.
    ld d,$22
    ld e,(hl)
    ld a,(VAR_FM_LFO_LOCK)
    or a
    jr z,fm_patch_lfo_ready
    ld a,(VAR_FM_LFO_VALUE)
    ld e,a
fm_patch_lfo_ready:
    call fm_patch_write_a
    inc hl
    ; Feedback/algorithm $B1
    ld d,$B1
    ld e,(hl)
    push hl
    ld a,e
    and $07
    ld hl,fm_carrier_masks
    ld c,a
    ld b,0
    add hl,bc
    ld a,(hl)
    ld (VAR_FM_CARRIERS),a
    pop hl
    call fm_patch_write_a
    inc hl
    ; L/R + AMS/PMS $B5
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
    push hl
    call force_write_a
    pop hl
    ret

;;; FIX: FM operator patch writer with proper TL volume scaling
;;; B = operator register base ($31/$35/$39/$3D)
;;; HL = pointer to 7 bytes: DT/MUL, TL, KS/AR, AM/DR, SR, SL/RR, SSG-EG
fm_write_operator_patch:
    ; DT/MUL
    ld d,b
    ld e,(hl)
    call fm_patch_write_a
    inc hl

    ; Attenuate carriers only: changing a modulator TL changes the timbre.
    ld a,b
    add a,$10
    ld d,a
    ld a,b
    and $0C
    rrca
    rrca
    ld c,a
    ld a,(VAR_FM_CARRIERS)
fm_carrier_shift:
    dec c
    jp m,fm_carrier_test
    srl a
    jr fm_carrier_shift
fm_carrier_test:
    and 1
    ld a,(hl)
    jr z,fm_tl_ok
    ld a,(VAR_FM_VOL)
    or a
    jr z,fm_tl_mute
    cpl
    and $0F
    add a,a
    add a,a
    ld c,a
    ld a,(hl)
    add a,c
    cp $80
    jr c,fm_tl_ok
fm_tl_mute:
    ld a,$7F
fm_tl_ok:
    ld e,a
    call fm_patch_write_a
    inc hl

    ; KS/AR
    ld a,b
    add a,$20
    ld d,a
    ld e,(hl)
    call fm_patch_write_a
    inc hl

    ; AM/DR
    ld a,b
    add a,$30
    ld d,a
    ld e,(hl)
    call fm_patch_write_a
    inc hl

    ; SR
    ld a,b
    add a,$40
    ld d,a
    ld e,(hl)
    call fm_patch_write_a
    inc hl

    ; SL/RR
    ld a,b
    add a,$50
    ld d,a
    ld e,(hl)
    call fm_patch_write_a
    inc hl

    ; SSG-EG
    ld a,b
    add a,$60
    ld d,a
    ld e,(hl)
    call fm_patch_write_a
    inc hl
    ret

; Operator register order: $31, $35, $39, $3D (not logical OPN slot order).
fm_carrier_masks:
    .db $08,$08,$08,$08,$0C,$0E,$0E,$0F

;;; FIX: FM note on with proper key-off -> frequency latch -> key-on sequence
;;; B = note index into fm_note_table
;;; Per YM2610 spec: write $A5 (F-Num2/Block high) BEFORE $A1 (F-Num1 low)
;;; to avoid frequency glitch during latch
fm_note_on:
    ; Step 1: Key off to retrigger envelope
    ld de,$2801
    call force_write_a

    ; Step 2: Look up note frequency from table
    ld a,b
    ld e,a
    ld d,0
    ld hl,fm_note_table
    add hl,de
    add hl,de
    ld c,(hl)        ; F-Num low byte
    inc hl
    ld a,(hl)        ; Block + F-Num high bits

    ; Step 3: Latch frequency (high byte first per spec)
    ld e,a
    ld d,$A5         ; F-Num2 / Block (write first to buffer)
    call force_write_a
    ld e,c
    ld d,$A1         ; F-Num1 (write second to latch both)
    call force_write_a

    ; Step 4: Key on all 4 operators for channel 1
    ; Register $28: bits 4-7 = slot mask ($F0 = all 4 slots), bits 0-2 = channel ($01)
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

; Legacy step/fade divider. Musical note clocks use tempo_clock_step.
; A = tempo hint; return a divider of the 14.4076 Hz legacy clock.
tempo_to_frames:
    cp 200
    jr nc,tempo_fastest
    cp 170
    jr nc,tempo_very_fast
    cp 140
    jr nc,tempo_fast
    cp 120
    jr nc,tempo_med_fast
    cp 100
    jr nc,tempo_mid
    cp 80
    jr nc,tempo_med_slow
    cp 60
    jr nc,tempo_slow
    ld a,8
    ret
tempo_fastest:
    ld a,1
    ret
tempo_very_fast:
    ld a,2
    ret
tempo_fast:
    ld a,3
    ret
tempo_med_fast:
    ld a,3
    ret
tempo_mid:
    ld a,4
    ret
tempo_med_slow:
    ld a,5
    ret
tempo_slow:
    ld a,6
    ret

;;; SSG standalone track tick handler
ssg_tick:
    ld a,(VAR_SSG_BPM)
    or a
    jr z,ssg_tick_legacy
    ld hl,(VAR_SSG_PHASE)
    call tempo_clock_step
    ld (VAR_SSG_PHASE),hl
    ret nc
    jr ssg_tick_duration
ssg_tick_legacy:
    ld a,(VAR_LEGACY_TICK)
    or a
    ret z
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
ssg_tick_duration:
    ld a,(VAR_SSG_WAIT)
    or a
    jr z,ssg_step
    dec a
    ld (VAR_SSG_WAIT),a
    ret nz
    jp ssg_step

;;; SSG standalone track step: parse and execute next command(s)
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
    cp $F7
    jp z,ssg_set_envelope_shape
    cp $F8
    jp z,ssg_set_envelope_period
    cp $F9
    jp z,ssg_set_envelope_mode
    cp $80
    jp z,ssg_rest
    cp $80
    jp nc,ssg_stop

    ; Normal SSG note: byte = note index, next byte = duration
    ld b,a
    ld a,(hl)
    inc hl
    ld (VAR_SSG_WAIT),a
    call store_ssg_ptr
    jp ssg_standalone_note_on

ssg_set_tempo:
    ld b,(hl)
    inc hl
    ld a,(VAR_SSG_TEMPO_LOCK)
    or a
    jr nz,ssg_set_tempo_done
    ld a,b
    ld (VAR_SSG_BPM),a
    call tempo_to_frames
    ld (VAR_SSG_TEMPO),a
ssg_set_tempo_done:
    call store_ssg_ptr
    jp ssg_step

ssg_set_envelope_shape:
    ld a,(hl)
    inc hl
    and $0F
    ld d,$0D
    ld e,a
    call store_ssg_ptr
    call force_write_a
    ld a,1
    ld (VAR_SSG_ENVELOPE),a
    call ssg_apply_standalone_volume
    jp ssg_step

ssg_set_envelope_period:
    ld e,(hl)
    inc hl
    ld d,$0B
    call store_ssg_ptr
    call force_write_a
    ld de,$0C00
    call force_write_a
    jp ssg_step

ssg_set_envelope_mode:
    ld a,(hl)
    inc hl
    and 1
    ld (VAR_SSG_ENVELOPE),a
    call store_ssg_ptr
    call ssg_apply_standalone_volume
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
    ; FIX: Only silence volumes, don't full-reset SSG (preserve preset/mixer state)
    ld de,$0800
    call shadowed_write_a
    ld de,$0900
    call shadowed_write_a
    ld de,$0A00
    call shadowed_write_a
    ret

ssg_loop:
    ; Silence volumes before looping
    ld de,$0800
    call shadowed_write_a
    ld de,$0900
    call shadowed_write_a
    ld de,$0A00
    call shadowed_write_a
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

;;; SSG preset loader
;;; Preset format (6 bytes):
;;;   byte 0 = mixer register $07 mask
;;;   byte 1 = channel A volume
;;;   byte 2 = channel B volume
;;;   byte 3 = channel C volume
;;;   byte 4 = noise frequency
;;;   byte 5 = reserved
ssg_apply_preset:
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
    ; Mixer $07
    ld d,$07
    ld e,(hl)
    ld a,e
    ld (VAR_SSG_MIXER),a
    call ssg_preset_write_a
    inc hl
    ; Volume A $08 (M=0 fixed amplitude)
    ld d,$08
    ld a,(hl)
    and $0F
    ld (VAR_SSG_LEVEL_A),a
    ld e,a
    call ssg_preset_write_a
    inc hl
    ; Volume B $09 (M=0 fixed amplitude)
    ld d,$09
    ld a,(hl)
    and $0F
    ld (VAR_SSG_LEVEL_B),a
    ld e,a
    call ssg_preset_write_a
    inc hl
    ; Volume C $0A (M=0 fixed amplitude)
    ld d,$0A
    ld a,(hl)
    and $0F
    ld (VAR_SSG_LEVEL_C),a
    ld e,a
    call ssg_preset_write_a
    inc hl
    ; Noise frequency $06
    ld d,$06
    ld e,(hl)
    call ssg_preset_write_a
    inc hl
    ; Skip reserved byte
    inc hl
    ret

ssg_preset_write_a:
    push hl
    call shadowed_write_a
    pop hl
    ret

;;; FIX: SSG note on with accurate period calculation and 3-channel unison
;;; B = note index (0-based, where 0=C, 11=B, 12=C+1oct, etc.)
;;; SSG period formula: Period = Fmaster / (64 * Fnote)
;;;   where Fmaster = 8,000,000 Hz (YM2610 master clock)
;;;   and period is 12-bit (0-4095)
;;; The period table below is for octave 5 (middle octave).
;;; Higher octaves: right-shift period. Lower octaves: left-shift period.
;;; Channel A = exact pitch
;;; Channel B = slight detune (+1 period) for chorus/unison effect
;;; Channel C = slight detune (-1 period) for richer sound
ssg_note_on:
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

    ; Shift period for octave
    ; MML uses MIDI numbering: C5 is 72, hence octave index 6 here.
    ld a,c
    cp 6
    jr z,ssg_period_ready
    jr c,ssg_shift_left
ssg_shift_right:
    sub 6
    jr z,ssg_period_ready
ssg_shift_right_loop:
    srl d
    rr e
    dec a
    jr nz,ssg_shift_right_loop
    jr ssg_period_ready
ssg_shift_left:
    ld a,6
    sub c
    jr z,ssg_period_ready
ssg_shift_left_loop:
    sla e
    rl d
    dec a
    jr nz,ssg_shift_left_loop
ssg_period_ready:
    ; Saturate; masking overflow would turn low notes into high notes.
    ld a,d
    cp $10
    jr c,ssg_period_min
    ld de,$0FFF
ssg_period_min:
    ld a,d
    or e
    jr nz,ssg_period_valid
    inc de
ssg_period_valid:

    ; Channel A: exact period
    push de
    ld d,$00
    call shadowed_write_a   ; Ch A fine tune
    pop de
    push de
    ld e,d
    ld d,$01
    call shadowed_write_a   ; Ch A coarse tune
    pop de

    ; FIX: Channel B: period + 1 (slight detune up for chorus)
    push de
    inc de          ; +1 period = slightly lower frequency
    ld a,d
    cp $10
    jr c,ssg_ch_b_valid
    dec de
ssg_ch_b_valid:
    push de
    ld d,$02
    call shadowed_write_a   ; Ch B fine tune
    pop de
    ld e,d
    ld d,$03
    call shadowed_write_a   ; Ch B coarse tune
    pop de

    ; FIX: Channel C: period - 1 (slight detune down for chorus)
    ld a,d
    or a
    jr nz,ssg_ch_c_detune
    ld a,e
    cp 2
    jr c,ssg_ch_c_no_detune
ssg_ch_c_detune:
    dec de
ssg_ch_c_no_detune:
    ld a,d
    and $0F
    ld d,a
    push de
    ld d,$04
    call shadowed_write_a   ; Ch C fine tune
    pop de
    ld e,d
    ld d,$05
    call shadowed_write_a   ; Ch C coarse tune

    ld a,(VAR_SSG_ACTIVE)
    or a
    jp nz,ssg_apply_standalone_volume

    ; FIX: Enable all 3 tone channels, disable all noise
    ; Register $07 bit layout:
    ;   bit 0 = /Tone A (0=enable)
    ;   bit 1 = /Tone B (0=enable)
    ;   bit 2 = /Tone C (0=enable)
    ;   bit 3 = /Noise A (1=disable)
    ;   bit 4 = /Noise B (1=disable)
    ;   bit 5 = /Noise C (1=disable)
    ;   $38 = tones A,B,C on + noise A,B,C off
    ld de,$0738
    call shadowed_write_a

    jp apply_music_volume

ssg_standalone_note_on:
    jp ssg_note_on

channel_masks:
    .db $01, $02, $04, $08, $10, $20

adpcma_stop_ticks:
    .db 24, 34, 70, 42, 12, 38, 32, 38, 56, 32, 44, 30

;;; FIX: Accurate SSG period table for A=440Hz tuning
;;; Master clock = 8,000,000 Hz, SSG divider = 64
;;; Period = 8000000 / (64 * Fnote)
;;; This table is for octave 5 (C5=523.25Hz to B5=987.77Hz)
;;; C5    = 8000000/(64*523.25)  = 238.89 -> $EF = 239
;;; C#5   = 8000000/(64*554.37)  = 225.48 -> $E1 = 225
;;; D5    = 8000000/(64*587.33)  = 212.86 -> $D5 = 213
;;; D#5   = 8000000/(64*622.25)  = 200.93 -> $C9 = 201
;;; E5    = 8000000/(64*659.26)  = 189.63 -> $BE = 190
;;; F5    = 8000000/(64*698.46)  = 178.93 -> $B3 = 179
;;; F#5   = 8000000/(64*739.99)  = 168.92 -> $A9 = 169
;;; G5    = 8000000/(64*783.99)  = 159.44 -> $9F = 159
;;; G#5   = 8000000/(64*830.61)  = 150.43 -> $96 = 150
;;; A5    = 8000000/(64*880.00)  = 142.05 -> $8E = 142
;;; A#5   = 8000000/(64*932.33)  = 134.05 -> $86 = 134
;;; B5    = 8000000/(64*987.77)  = 126.51 -> $7F = 127
ssg_period_table:
    .dw $00EF  ; C5  = 239
    .dw $00E1  ; C#5 = 225
    .dw $00D5  ; D5  = 213
    .dw $00C9  ; D#5 = 201
    .dw $00BE  ; E5  = 190
    .dw $00B3  ; F5  = 179
    .dw $00A9  ; F#5 = 169
    .dw $009F  ; G5  = 159
    .dw $0096  ; G#5 = 150
    .dw $008E  ; A5  = 142
    .dw $0086  ; A#5 = 134
    .dw $007F  ; B5  = 127

;;; Label-addressed data stays in the unbanked 32 KiB ROM window.
.include "fm_patch_table.inc"
.include "fm_data.inc"
.include "music_data.inc"
.include "ssg_config.inc"
.include "ssg_data.inc"
.include "sample_table.inc"
sound_data_end:
.assert sound_data_end <= $8000
