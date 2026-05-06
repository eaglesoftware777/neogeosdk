#ifndef DRIVER_SPLIT_PRELUDE
asm {
;; Vectors and Hardware low-level
.org $0000
reset:
    di
    jp main

.org $0008 ; ym_wait_ready
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
    ; Reset Timer B flag
    ld d,$27
    ld e,$2A
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
    ld a,(fifo_write)
    ld l,a
    ld h,$F8 ; Assume fifo_buf at $F800
    ld (hl),b
    inc a
    and $1F
    ld (fifo_write),a
    xor a
    out ($00),a
    ld a,$01
    out ($0C),a
    pop hl
    pop bc
    pop af
    retn

.org $00D0
}
#else
asm {
.org $00D0
}
#endif

#include "driver_defs.h"

// RAM layout managed by z80cc starting at $FE00
// FIFO and shadow buffers live at fixed RAM addresses.
asm {
.define fifo_buf   $F800
.define fifo_read  $F820
.define fifo_write $F821
.define shadow_a   $F900
.define shadow_b   $FA00
}

// Global variables (will follow at $FE00)
unsigned char var_command;
unsigned char var_tempo;
unsigned char var_tick;
unsigned char var_music_ptr_lo;
unsigned char var_music_ptr_hi;
unsigned char var_adpcma_ch;
unsigned char var_wait_tempo;
unsigned char var_music_wait;
unsigned char var_music_active;
unsigned char var_music_vol;
unsigned char var_adpcma_ticks;
unsigned char var_param_mode;
unsigned char var_adpcma_vol;
unsigned char var_adpcmb_vol;
unsigned char var_fm_ticks;
unsigned char var_music_start_lo;
unsigned char var_music_start_hi;
unsigned char var_fade_mode;
unsigned char var_fade_speed;
unsigned char var_music_vol_base;
unsigned char var_adpcma_base;
unsigned char var_adpcmb_base;
unsigned char var_fade_ticks;
unsigned char var_fm_ptr_lo;
unsigned char var_fm_ptr_hi;
unsigned char var_fm_wait;
unsigned char var_fm_active;
unsigned char var_fm_vol;
unsigned char var_fm_vol_base;
unsigned char var_fm_start_lo;
unsigned char var_fm_start_hi;
unsigned char var_fm_patch;
unsigned char var_fm_tempo;
unsigned char var_fm_tick;
unsigned char var_ssg_ptr_lo;
unsigned char var_ssg_ptr_hi;
unsigned char var_ssg_start_lo;
unsigned char var_ssg_start_hi;
unsigned char var_ssg_wait;
unsigned char var_ssg_active;
unsigned char var_ssg_preset;
unsigned char var_ssg_tempo;
unsigned char var_ssg_tick;
unsigned char var_ssg_vol;

// Temp variables for parameters
unsigned char reg;
unsigned char val;
unsigned char tmp_a;
unsigned char tmp_b;
unsigned char tmp_c;
unsigned char tmp_d;
unsigned char tmp_e;
unsigned char tmp_hl_lo;
unsigned char tmp_hl_hi;

void main(void) {
    asm {
    ld sp,$FFFC
    im 1
    xor a
    ld hl,$F800
    ld de,$F801
    ld bc,$07FF
    ld (hl),a
    ldir
    ld a,$FF
    ld hl,$F900
    ld de,$F901
    ld bc,$01FF
    ld (hl),a
    ldir
    call driver_init
main_loop_c:
    call process_fifo
    halt
    jp main_loop_c
    }
}

void ym_wait(void) {
    asm { rst $08 }
}

void shadowed_write_a_c(unsigned char r, unsigned char v) {
    asm {
    ld a,(r)
    ld d,a
    ld a,(v)
    ld e,a
    jp shadowed_write_a
    }
}

void shadowed_write_b_c(unsigned char r, unsigned char v) {
    asm {
    ld a,(r)
    ld d,a
    ld a,(v)
    ld e,a
    jp shadowed_write_b
    }
}

void force_write_a_c(unsigned char r, unsigned char v) {
    asm {
    ld a,(r)
    ld d,a
    ld a,(v)
    ld e,a
    jp force_write_a
    }
}

void force_write_b_c(unsigned char r, unsigned char v) {
    asm {
    ld a,(r)
    ld d,a
    ld a,(v)
    ld e,a
    jp force_write_b
    }
}

void adpcma_stop(void) {
    force_write_b_c(0x00, 0xBF);
}

void adpcmb_stop(void) {
    force_write_a_c(0x10, 0x01);
}

void init_ssg(void) {
    shadowed_write_a_c(0x07, DRIVER_SSG_SILENT_MIXER);
    shadowed_write_a_c(0x06, DRIVER_SSG_SILENT_LEVEL);
    shadowed_write_a_c(0x08, DRIVER_SSG_SILENT_LEVEL);
    shadowed_write_a_c(0x09, DRIVER_SSG_SILENT_LEVEL);
    shadowed_write_a_c(0x0A, DRIVER_SSG_SILENT_LEVEL);
    shadowed_write_a_c(0x0B, DRIVER_SSG_SILENT_LEVEL);
    shadowed_write_a_c(0x0C, DRIVER_SSG_SILENT_LEVEL);
    shadowed_write_a_c(0x0D, DRIVER_SSG_SILENT_LEVEL);
}

void fm_silence_all(void) {
    tmp_a = DRIVER_FM_KEY_CH1;
    while (tmp_a < (DRIVER_FM_KEY_CH1 + DRIVER_FM_CHANNEL_COUNT)) {
        force_write_a_c(DRIVER_FM_KEY_REG, tmp_a);
        tmp_b = tmp_a + 4;
        force_write_a_c(DRIVER_FM_KEY_REG, tmp_b);
        tmp_a = tmp_a + 1;
    }
    tmp_a = DRIVER_FM_SILENCE_TL_WRITES;
    reg = 0x41;
    while (tmp_a > 0) {
        shadowed_write_a_c(reg, 0x7F);
        shadowed_write_b_c(reg, 0x7F);
        reg = reg + 1;
        tmp_a = tmp_a - 1;
    }
}

void init_fm(void) {
    shadowed_write_a_c(0x26, 0x0F);
    force_write_a_c(0x27, 0x2A);
    shadowed_write_a_c(0x27, 0x3A);
    fm_silence_all();
    shadowed_write_a_c(0x22, 0x00);
}

void init_adpcma(void) {
    adpcma_stop();
    shadowed_write_b_c(DRIVER_ADPCMA_MASTER_REG, var_adpcma_vol);
    tmp_a = DRIVER_ADPCMA_CHANNEL_COUNT;
    reg = 8;
    while (tmp_a > 0) {
        shadowed_write_b_c(reg, 0xC0);
        reg = reg + 1;
        tmp_a = tmp_a - 1;
    }
}

void init_adpcmb(void) {
    force_write_a_c(0x10, 0x01);
    force_write_a_c(0x1C, 0x80);
    force_write_a_c(0x1C, 0x00);
    force_write_a_c(0x10, 0x00);
    shadowed_write_a_c(0x11, 0xC0);
    shadowed_write_a_c(0x19, 0xBA);
    shadowed_write_a_c(0x1A, 0x49);
    shadowed_write_a_c(DRIVER_ADPCMB_VOLUME_REG, var_adpcmb_vol);
}

void apply_music_volume(void) {
    shadowed_write_a_c(0x08, var_music_vol & 0x0F);
    tmp_a = (var_music_vol & 0x0F) >> 1;
    shadowed_write_a_c(0x09, tmp_a + 3);
}

void apply_master_volumes(void) {
    shadowed_write_b_c(DRIVER_ADPCMA_MASTER_REG, var_adpcma_vol);
    shadowed_write_a_c(DRIVER_ADPCMB_VOLUME_REG, var_adpcmb_vol);
    apply_music_volume();
}

void driver_reset_runtime_state(void) {
    var_music_active = 0;
    var_music_wait = 0;
    var_wait_tempo = 0;
    var_tick = 0;
    var_adpcma_ch = 0;
    var_adpcma_ticks = 0;
    var_param_mode = DRIVER_PARAM_NONE;
    var_fade_mode = 0;
    var_fade_speed = 0;
    var_fade_ticks = 0;
    var_fm_ticks = 0;
    var_fm_active = 0;
    var_fm_wait = 0;
    var_fm_patch = 0;
    var_fm_vol = 0;
    var_fm_vol_base = 0;
    var_fm_tempo = 0;
    var_fm_tick = 0;
    var_ssg_active = 0;
    var_ssg_wait = 0;
    var_ssg_preset = 0;
    var_ssg_tempo = 0;
    var_ssg_tick = 0;
}

void driver_load_default_mix(void) {
    var_ssg_vol = DRIVER_DEFAULT_SSG_VOLUME;
    var_tempo = DRIVER_DEFAULT_TEMPO_FRAMES;
    var_music_vol = DRIVER_DEFAULT_MUSIC_VOLUME;
    var_music_vol_base = DRIVER_DEFAULT_MUSIC_VOLUME;
    var_adpcma_vol = DRIVER_DEFAULT_ADPCMA_VOLUME;
    var_adpcma_base = DRIVER_DEFAULT_ADPCMA_VOLUME;
    var_adpcmb_vol = DRIVER_DEFAULT_ADPCMB_VOLUME;
    var_adpcmb_base = DRIVER_DEFAULT_ADPCMB_VOLUME;
}

void driver_boot_subsystems(void) {
    force_write_a_c(DRIVER_ADPCMA_MASTER_REG, 0x00);
    force_write_b_c(DRIVER_ADPCMA_MASTER_REG, 0x00);
    init_ssg();
    init_fm();
    init_adpcma();
    init_adpcmb();
}

void driver_init(void) {
    di();
    out(DRIVER_PORT_NMI_DISABLE, 0);
    out(DRIVER_PORT_STATUS, 0);
    driver_reset_runtime_state();
    driver_load_default_mix();
    driver_boot_subsystems();
    out(DRIVER_PORT_NMI_ENABLE, 0);
    out(DRIVER_PORT_STATUS, DRIVER_READY_VALUE);
    ei();
}

void stop_all(void) {
    asm { jp stop_all_impl }
}

unsigned char tempo_to_frames(unsigned char t) {
    asm { jp tempo_to_frames_impl }
}

void fm_stop(void) {
    asm { jp fm_stop_impl }
}

void execute_command(void) {
    asm { jp execute_command_impl }
}

void ticker_update(void) {
    asm { jp ticker_update_impl }
}

void process_fifo(void) {
    while (fifo_read != fifo_write) {
        asm {
        ld a,(fifo_read)
        ld l,a
        ld h,$F8
        ld a,(hl)
        ld (var_command),a
        }
        fifo_read = (fifo_read + 1) & DRIVER_FIFO_MASK;
        execute_command();
    }
}

asm {
shadowed_write_a:
    ld h,$F9
    ld l,d
    ld a,(hl)
    cp e
    ret z
    ld (hl),e
    ld a,d
    out ($04),a
    rst $08
    ld a,e
    out ($05),a
    rst $08
    ret

shadowed_write_b:
    ld h,$FA
    ld l,d
    ld a,(hl)
    cp e
    ret z
    ld (hl),e
    ld a,d
    out ($06),a
    rst $08
    ld a,e
    out ($07),a
    rst $08
    ret

force_write_a:
    ld h,$F9
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
    ld h,$FA
    ld l,d
    ld (hl),e
    ld a,d
    out ($06),a
    rst $08
    ld a,e
    out ($07),a
    rst $08
    ret

execute_command_impl:
    ld a,(var_command)
    ld c,a
    ld a,(var_param_mode)
    or a
    jp z,exec_normal
    xor a
    ld (var_param_mode),a
    ld a,(var_wait_tempo)
    cp DRIVER_PARAM_TEMPO
    jr z,exec_p_tempo
    cp DRIVER_PARAM_ADPCMA_VOLUME
    jr z,exec_p_adpcma
    cp DRIVER_PARAM_ADPCMB_VOLUME
    jr z,exec_p_adpcmb
    cp DRIVER_PARAM_SSG_VOLUME
    jr z,exec_p_ssg
    cp DRIVER_PARAM_FADE_OUT
    jr z,exec_p_fadeout
    cp DRIVER_PARAM_FADE_IN
    jr z,exec_p_fadein
    cp DRIVER_PARAM_FM_TRACK
    jr z,exec_p_fmtrack
    cp DRIVER_PARAM_FM_VOLUME
    jr z,exec_p_fmvol
    cp DRIVER_PARAM_SSG_TRACK
    jr z,exec_p_ssgtrack
    cp DRIVER_PARAM_SSG_PRESET
    jr z,exec_p_ssgpreset
    ret

exec_p_tempo:
    ld a,c
    ld (var_tempo),a
    ret

exec_p_fadeout:
    ld a,c
    ld (var_fade_speed),a
    ld a,1
    ld (var_fade_mode),a
    ld a,$FF
    sub c
    jr nz,exec_fade_interval_ok
    ld a,1
exec_fade_interval_ok:
    ld (var_fade_ticks),a
    ret

exec_p_fadein:
    ld a,c
    ld (var_fade_speed),a
    ld a,2
    ld (var_fade_mode),a
    ld a,$FF
    sub c
    jr nz,exec_fade_interval_ok2
    ld a,1
exec_fade_interval_ok2:
    ld (var_fade_ticks),a
    ret

exec_p_adpcma:
    ld a,c
    and $3F
    ld (var_adpcma_vol),a
    ld (var_adpcma_base),a
    ld d,$01
    ld e,a
    call shadowed_write_b
    ret

exec_p_adpcmb:
    ld a,c
    ld (var_adpcmb_vol),a
    ld (var_adpcmb_base),a
    ld d,$1B
    ld e,a
    call shadowed_write_a
    ret

exec_p_ssg:
    ld a,c
    and $0F
    ld (var_music_vol),a
    ld (var_music_vol_base),a
    call apply_music_volume
    ret

exec_p_fmtrack:
    ld a,c
    jp play_fm_index

exec_p_fmvol:
    ld a,c
    and $0F
    ld (var_fm_vol),a
    ld (var_fm_vol_base),a
    call fm_apply_patch
    ret

exec_p_ssgtrack:
    ld a,c
    jp play_ssg_index

exec_p_ssgpreset:
    ld a,c
    and $0F
    ld (var_ssg_preset),a
    call ssg_apply_preset
    ret

set_fmvol_wait:
    ld a,DRIVER_PARAM_FM_VOLUME
    ld (var_wait_tempo),a
    ld (var_param_mode),a
    ret

exec_normal:
    ld a,c
    or a
    ret z

    cp $01
    jp z,driver_init
    cp $02
    jp z,play_music1
    cp $03
    jp z,driver_soft_reset
    cp $04
    jp z,stop_all_impl
    cp DRIVER_CMD_SET_ADPCMA_VOLUME
    jr z,set_adpcma_volume_wait
    cp DRIVER_CMD_SET_ADPCMB_VOLUME
    jr z,set_adpcmb_volume_wait
    cp DRIVER_CMD_SET_SSG_VOLUME
    jr z,set_ssg_volume_wait
    cp DRIVER_CMD_FADE_OUT
    jr z,set_fadeout_wait
    cp DRIVER_CMD_STOP_ADPCMA
    jp z,adpcma_stop
    cp DRIVER_CMD_STOP_ADPCMB
    jp z,adpcmb_stop
    cp DRIVER_CMD_SET_TEMPO
    jr z,set_tempo_wait
    cp DRIVER_CMD_STOP_MUSIC
    jp z,stop_music
    cp DRIVER_CMD_CANCEL_FADE
    jp z,cancel_fade
    cp DRIVER_CMD_FADE_IN
    jr z,set_fadein_wait
    cp DRIVER_CMD_SET_FM_VOLUME
    jr z,set_fmvol_wait
    cp DRIVER_CMD_SET_SSG_PRESET
    jr z,set_ssgpreset_wait
    cp $30
    jp z,play_fm_demo
    cp DRIVER_CMD_PLAY_FM_PREFIX
    jr z,set_fmtrack_wait
    cp DRIVER_CMD_PLAY_SSG_PREFIX
    jr z,set_ssgtrack_wait
    cp $28
    jr z,play_demo_b0
    cp $29
    jr z,play_demo_b1

    cp $80
    jp nc,play_adpcmb_cmd
    cp $40
    jp nc,play_adpcma_cmd
    cp $20
    jp nc,play_fm_cmd
    ret

set_tempo_wait:
    ld a,DRIVER_PARAM_TEMPO
    ld (var_wait_tempo),a
    ld (var_param_mode),a
    ret

set_adpcma_volume_wait:
    ld a,DRIVER_PARAM_ADPCMA_VOLUME
    ld (var_wait_tempo),a
    ld (var_param_mode),a
    ret

set_adpcmb_volume_wait:
    ld a,DRIVER_PARAM_ADPCMB_VOLUME
    ld (var_wait_tempo),a
    ld (var_param_mode),a
    ret

set_ssg_volume_wait:
    ld a,DRIVER_PARAM_SSG_VOLUME
    ld (var_wait_tempo),a
    ld (var_param_mode),a
    ret

set_fadeout_wait:
    ld a,DRIVER_PARAM_FADE_OUT
    ld (var_wait_tempo),a
    ld (var_param_mode),a
    ret

set_fadein_wait:
    ld a,DRIVER_PARAM_FADE_IN
    ld (var_wait_tempo),a
    ld (var_param_mode),a
    ret

set_fmtrack_wait:
    ld a,DRIVER_PARAM_FM_TRACK
    ld (var_wait_tempo),a
    ld (var_param_mode),a
    ret

set_ssgtrack_wait:
    ld a,DRIVER_PARAM_SSG_TRACK
    ld (var_wait_tempo),a
    ld (var_param_mode),a
    ret

set_ssgpreset_wait:
    ld a,DRIVER_PARAM_SSG_PRESET
    ld (var_wait_tempo),a
    ld (var_param_mode),a
    ret

play_demo_b0:
    xor a
    jp play_adpcmb_index

play_demo_b1:
    ld a,1
    jp play_adpcmb_index

ssg_apply_standalone_volume:
    ld d,$08
    ld a,(var_ssg_vol)
    and $0F
    ld e,a
    call shadowed_write_a
    ld d,$09
    ld a,(var_ssg_vol)
    and $0F
    srl a
    add a,3
    ld e,a
    jp shadowed_write_a

cancel_fade:
    xor a
    ld (var_fade_mode),a
    ld (var_fade_speed),a
    ld (var_fade_ticks),a
    ld a,(var_music_vol_base)
    ld (var_music_vol),a
    ld a,(var_adpcma_base)
    ld (var_adpcma_vol),a
    ld a,(var_adpcmb_base)
    ld (var_adpcmb_vol),a
    jp apply_master_volumes

stop_music:
    xor a
    ld (var_music_active),a
    ld (var_music_wait),a
    ld (var_fade_mode),a
    ld (var_fade_speed),a
    ld (var_fade_ticks),a
    ld (var_fm_active),a
    ld (var_fm_wait),a
    ld (var_ssg_active),a
    ld (var_ssg_wait),a
    call fm_silence_all
    jp init_ssg

stop_all_impl:
    xor a
    ld (var_music_active),a
    ld (var_adpcma_ticks),a
    ld (var_fm_ticks),a
    ld (var_fade_mode),a
    ld (var_fade_speed),a
    ld (var_fade_ticks),a
    ld (var_fm_active),a
    ld (var_fm_wait),a
    ld (var_fm_patch),a
    ld (var_fm_tempo),a
    ld (var_fm_tick),a
    ld (var_ssg_active),a
    ld (var_ssg_wait),a
    ld (var_ssg_preset),a
    ld (var_ssg_tempo),a
    ld (var_ssg_tick),a
    call init_ssg
    call fm_silence_all
    call adpcma_stop
    jp adpcmb_stop

driver_soft_reset:
    xor a
    ld (var_music_active),a
    ld (var_adpcma_ticks),a
    ld (var_fm_ticks),a
    ld (var_music_wait),a
    ld (var_wait_tempo),a
    ld (var_param_mode),a
    ld (var_tick),a
    ld (fifo_read),a
    ld (fifo_write),a
    ld (var_adpcma_ch),a
    ld (var_music_start_lo),a
    ld (var_music_start_hi),a
    ld (var_fade_mode),a
    ld (var_fade_speed),a
    ld (var_fade_ticks),a
    ld (var_fm_active),a
    ld (var_fm_wait),a
    ld (var_fm_patch),a
    ld (var_fm_vol),a
    ld (var_fm_vol_base),a
    ld (var_fm_tempo),a
    ld (var_fm_tick),a
    ld (var_ssg_active),a
    ld (var_ssg_wait),a
    ld (var_ssg_preset),a
    ld (var_ssg_tempo),a
    ld (var_ssg_tick),a
    ld a,$0A
    ld (var_ssg_vol),a
    ld a,3
    ld (var_tempo),a
    ld a,$08
    ld (var_music_vol),a
    ld (var_music_vol_base),a
    ld a,$3F
    ld (var_adpcma_vol),a
    ld (var_adpcma_base),a
    ld a,$A8
    ld (var_adpcmb_vol),a
    ld (var_adpcmb_base),a
    call init_ssg
    call fm_silence_all
    call adpcma_stop
    call adpcmb_stop
    xor a
    ld (var_ssg_vol),a
    call ssg_apply_standalone_volume
    ld a,$08
    ld (var_music_vol),a
    ld (var_music_vol_base),a
    call apply_music_volume
    xor a
    ld (var_fade_mode),a
    ld (var_fade_speed),a
    ld (var_fade_ticks),a
    ret

play_fm_cmd:
    sub $20
play_music_index:
    cp MUSIC_TRACK_COUNT
    ret nc
    add a,a
    ld e,a
    ld d,0
    ld hl,music_track_table
    add hl,de
    ld a,(hl)
    ld (var_music_ptr_lo),a
    ld (var_music_start_lo),a
    inc hl
    ld a,(hl)
    ld (var_music_ptr_hi),a
    ld (var_music_start_hi),a
    xor a
    ld (var_music_wait),a
    ld a,1
    ld (var_music_active),a
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
    call fm_stop_impl
    add a,a
    ld e,a
    ld d,0
    ld hl,fm_track_table
    add hl,de
    ld a,(hl)
    ld (var_fm_ptr_lo),a
    ld (var_fm_start_lo),a
    inc hl
    ld a,(hl)
    ld (var_fm_ptr_hi),a
    ld (var_fm_start_hi),a
    xor a
    ld (var_fm_wait),a
    ld a,1
    ld (var_fm_active),a
    ld a,0
    ld (var_fm_patch),a
    ld a,$0F
    ld (var_fm_vol_base),a
    ld (var_fm_vol),a
    ld a,3
    ld (var_fm_tempo),a
    xor a
    ld (var_fm_tick),a
    call fm_apply_patch
    jp fm_step

play_adpcma_cmd:
    sub $40
play_adpcma_index:
    cp ADPCMA_COUNT
    ret nc
    push af
    ld e,a
    ld d,0
    ld hl,adpcma_stop_ticks
    add hl,de
    ld a,(hl)
    ld (var_adpcma_ticks),a
    pop af
    call get_sample_ptr
    push hl
    push bc
    ld a,(var_adpcma_ch)
    ld c,a
    inc a
    cp 6
    jr c,adpcma_channel_ok
    xor a
adpcma_channel_ok:
    ld (var_adpcma_ch),a
    ld hl,channel_masks
    ld e,c
    ld d,0
    add hl,de
    ld a,(hl)
    ld (var_command),a
    ld d,$1C
    ld e,a
    call force_write_a
    ld de,$1C00
    call force_write_a
    ld d,$01
    ld a,(var_adpcma_vol)
    ld e,a
    call shadowed_write_b
    ld a,c
    add a,$08
    ld d,a
    ld e,$C0
    call shadowed_write_b
    pop hl
    ld a,c
    add a,$10
    ld d,a
    ld e,h
    call shadowed_write_b
    ld a,c
    add a,$18
    ld d,a
    ld e,l
    call shadowed_write_b
    pop hl
    ld b,h
    ld a,c
    add a,$20
    ld d,a
    ld e,l
    call shadowed_write_b
    ld a,c
    add a,$28
    ld d,a
    ld e,b
    call shadowed_write_b
    ld a,(var_command)
    ld e,a
    ld d,$00
    call force_write_b
    ret

play_adpcmb_cmd:
    sub $80
play_adpcmb_index:
    cp ADPCMB_COUNT
    ret nc
    add a,ADPCMA_COUNT
    call get_sample_ptr
    push hl
    ld de,$1001
    call force_write_a
    ld de,$1C80
    call force_write_a
    ld de,$1C00
    call force_write_a
    ld d,$12
    ld e,b
    call shadowed_write_a
    ld d,$13
    ld e,c
    call shadowed_write_a
    pop hl
    ld c,h
    ld d,$14
    ld e,l
    call shadowed_write_a
    ld d,$15
    ld e,c
    call shadowed_write_a
    ld d,$1B
    ld a,(var_adpcmb_vol)
    ld e,a
    call shadowed_write_a
    ld de,$1000
    call force_write_a
    ld de,$1080
    call force_write_a
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
    ld (var_ssg_ptr_lo),a
    ld (var_ssg_start_lo),a
    inc hl
    ld a,(hl)
    ld (var_ssg_ptr_hi),a
    ld (var_ssg_start_hi),a
    xor a
    ld (var_ssg_wait),a
    ld a,1
    ld (var_ssg_active),a
    ld a,0
    ld (var_ssg_preset),a
    ld a,$0A
    ld (var_ssg_vol),a
    ld a,3
    ld (var_ssg_tempo),a
    xor a
    ld (var_ssg_tick),a
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
    ld b,(hl)
    inc hl
    ld c,(hl)
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld l,e
    ld h,d
    ret

ticker_update_impl:
    ld a,(var_fm_ticks)
    or a
    jr z,ticker_adpcma
    dec a
    ld (var_fm_ticks),a
    jr nz,ticker_adpcma
    call fm_stop_impl
ticker_adpcma:
    ld a,(var_adpcma_ticks)
    or a
    jr z,ticker_music
    dec a
    ld (var_adpcma_ticks),a
    jr nz,ticker_music
    call adpcma_stop
ticker_music:
    ld a,(var_fm_active)
    or a
    call nz,fm_tick
    ld a,(var_ssg_active)
    or a
    call nz,ssg_tick
    ld a,(var_tick)
    inc a
    ld (var_tick),a
    ld b,a
    ld a,(var_tempo)
    or a
    ret z
    cp b
    ret nz
    xor a
    ld (var_tick),a
    ld a,(var_music_active)
    or a
    call nz,music_tick
    call fade_tick
    ret

fade_tick:
    ld a,(var_fade_mode)
    or a
    ret z
    ld b,a
    ld a,(var_fade_ticks)
    or a
    jr z,fade_step
    dec a
    ld (var_fade_ticks),a
    ret nz
fade_step:
    ld a,(var_fade_speed)
    ld e,a
    ld a,$FF
    sub e
    jr nz,fade_interval_ok
    ld a,1
fade_interval_ok:
    ld (var_fade_ticks),a
    ld a,b
    cp 1
    jr z,fade_out_step
    cp 2
    jr z,fade_in_step
    ret

fade_out_step:
    ld a,(var_music_vol)
    or a
    jr z,fade_out_adpcma
    dec a
    ld (var_music_vol),a
fade_out_adpcma:
    ld a,(var_adpcma_vol)
    or a
    jr z,fade_out_adpcmb
    dec a
    ld (var_adpcma_vol),a
fade_out_adpcmb:
    ld a,(var_adpcmb_vol)
    or a
    jr z,fade_out_apply
    dec a
    ld (var_adpcmb_vol),a
fade_out_apply:
    call apply_master_volumes
    ld a,(var_music_vol)
    or a
    jr nz,fade_tick_done
    ld a,(var_adpcma_vol)
    or a
    jr nz,fade_tick_done
    ld a,(var_adpcmb_vol)
    or a
    jr nz,fade_tick_done
    xor a
    ld (var_fade_mode),a
    ret

fade_in_step:
    ld a,(var_music_vol)
    ld e,a
    ld a,(var_music_vol_base)
    cp e
    jr c,fade_in_adpcma
    jr z,fade_in_adpcma
    ld a,(var_music_vol)
    inc a
    ld (var_music_vol),a
fade_in_adpcma:
    ld a,(var_adpcma_vol)
    ld e,a
    ld a,(var_adpcma_base)
    cp e
    jr c,fade_in_adpcmb
    jr z,fade_in_adpcmb
    ld a,(var_adpcma_vol)
    inc a
    ld (var_adpcma_vol),a
fade_in_adpcmb:
    ld a,(var_adpcmb_vol)
    ld e,a
    ld a,(var_adpcmb_base)
    cp e
    jr c,fade_in_apply
    jr z,fade_in_apply
    ld a,(var_adpcmb_vol)
    inc a
    ld (var_adpcmb_vol),a
fade_in_apply:
    call apply_master_volumes
    ld a,(var_music_vol)
    ld e,a
    ld a,(var_music_vol_base)
    cp e
    jr c,fade_tick_done
    jr z,fade_tick_done
    ld a,(var_adpcma_vol)
    ld e,a
    ld a,(var_adpcma_base)
    cp e
    jr c,fade_tick_done
    jr z,fade_tick_done
    ld a,(var_adpcmb_vol)
    ld e,a
    ld a,(var_adpcmb_base)
    cp e
    jr c,fade_tick_done
    jr z,fade_tick_done
    xor a
    ld (var_fade_mode),a
    ret

fade_tick_done:
    ret

music_tick:
    ld a,(var_music_wait)
    or a
    jr z,music_step
    dec a
    ld (var_music_wait),a
    ret

music_step:
    ld a,(var_music_ptr_lo)
    ld l,a
    ld a,(var_music_ptr_hi)
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
    ld (var_music_wait),a
    call store_music_ptr
    jp ssg_note_on

music_set_tempo:
    ld a,(hl)
    inc hl
    call tempo_to_frames_impl
    ld (var_tempo),a
    jp music_step_continue

music_set_volume:
    ld a,(hl)
    inc hl
    and $0F
    ld (var_music_vol),a
    ld (var_music_vol_base),a
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
    ld (var_music_wait),a
    call store_music_ptr
    jp init_ssg

music_loop:
    call init_ssg
    ld a,(var_music_start_lo)
    ld l,a
    ld a,(var_music_start_hi)
    ld h,a
    xor a
    ld (var_music_wait),a
    call store_music_ptr
    jp music_step

music_stop:
    xor a
    ld (var_music_active),a
    ld (var_music_wait),a
    ld (var_fade_mode),a
    jp init_ssg

music_play_fm:
    ld a,(hl)
    inc hl
    call store_music_ptr
    push hl
    call play_fm_index
    pop hl
    jp music_step_next

music_play_ssg:
    ld a,(hl)
    inc hl
    call store_music_ptr
    push hl
    call play_ssg_index
    pop hl
    jp music_step_next

music_set_ssg_preset:
    ld a,(hl)
    inc hl
    and $0F
    ld (var_ssg_preset),a
    call store_music_ptr
    call ssg_apply_preset
    jp music_step

fm_tick:
    ld a,(var_fm_tick)
    inc a
    ld (var_fm_tick),a
    ld b,a
    ld a,(var_fm_tempo)
    or a
    ret z
    cp b
    ret nz
    xor a
    ld (var_fm_tick),a
    ld a,(var_fm_wait)
    or a
    jr z,fm_step
    dec a
    ld (var_fm_wait),a
    ret nz
    jp fm_step

fm_step:
    ld a,(var_fm_ptr_lo)
    ld l,a
    ld a,(var_fm_ptr_hi)
    ld h,a
fm_step_next:
    ld a,(hl)
    inc hl
    cp $FF
    jp z,fm_stop_impl
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
    ld b,a
    ld a,(hl)
    inc hl
    ld (var_fm_wait),a
    call store_fm_ptr
    call fm_note_on
    ret

fm_set_tempo:
    ld a,(hl)
    inc hl
    call tempo_to_frames_impl
    ld (var_fm_tempo),a
    call store_fm_ptr
    jp fm_step

fm_set_volume:
    ld a,(hl)
    inc hl
    and $0F
    ld (var_fm_vol),a
    ld (var_fm_vol_base),a
    call store_fm_ptr
    call fm_apply_patch
    jp fm_step

fm_set_patch:
    ld a,(hl)
    inc hl
    and $0F
    ld (var_fm_patch),a
    call store_fm_ptr
    call fm_apply_patch
    jp fm_step

fm_rest:
    ld a,(hl)
    inc hl
    ld (var_fm_wait),a
    call store_fm_ptr
    jp fm_note_off

fm_loop:
    call fm_note_off
    ld a,(var_fm_start_lo)
    ld l,a
    ld a,(var_fm_start_hi)
    ld h,a
    xor a
    ld (var_fm_wait),a
    call store_fm_ptr
    jp fm_step

fm_note_off:
    ld de,$2801
    jp force_write_a

fm_stop_impl:
    xor a
    ld (var_fm_active),a
    ld (var_fm_wait),a
    ld de,$2801
    call force_write_a
    jp fm_silence_all

fm_apply_patch:
    ld a,(var_fm_patch)
    cp FM_PATCH_COUNT
    jr c,fm_apply_patch_index_ok
    xor a
fm_apply_patch_index_ok:
    ld b,a
    ld hl,fm_patch_table
    ld a,b
    or a
    jr z,fm_apply_patch_ready
fm_patch_seek_loop:
    ld de,FM_PATCH_SIZE
    add hl,de
    dec a
    jr nz,fm_patch_seek_loop
fm_apply_patch_ready:
    ld d,$22
    ld e,(hl)
    call fm_patch_write_a
    inc hl
    ld d,$B1
    ld e,(hl)
    call fm_patch_write_a
    inc hl
    ld d,$B5
    ld e,(hl)
    call fm_patch_write_a
    inc hl
    ld b,$31
    call fm_write_operator_patch
    ld b,$35
    call fm_write_operator_patch
    ld b,$39
    call fm_write_operator_patch
    ld b,$3D
    call fm_write_operator_patch
    ret

fm_patch_write_a:
    push hl
    call force_write_a
    pop hl
    ret

fm_write_operator_patch:
    ld d,b
    ld e,(hl)
    call fm_patch_write_a
    inc hl
    ld a,b
    add a,$10
    ld d,a
    ld a,(var_fm_vol)
    cpl
    and $0F
    add a,a
    add a,a
    ld c,a
    ld a,(hl)
    add a,c
    cp $80
    jr c,fm_tl_ok
    ld a,$7F
fm_tl_ok:
    ld e,a
    call fm_patch_write_a
    inc hl
    ld a,b
    add a,$20
    ld d,a
    ld e,(hl)
    call fm_patch_write_a
    inc hl
    ld a,b
    add a,$30
    ld d,a
    ld e,(hl)
    call fm_patch_write_a
    inc hl
    ld a,b
    add a,$40
    ld d,a
    ld e,(hl)
    call fm_patch_write_a
    inc hl
    ld a,b
    add a,$50
    ld d,a
    ld e,(hl)
    call fm_patch_write_a
    inc hl
    ld a,b
    add a,$60
    ld d,a
    ld e,(hl)
    call fm_patch_write_a
    inc hl
    ret

fm_note_on:
    ld de,$2801
    call force_write_a
    ld a,b
    ld e,a
    ld d,0
    ld hl,fm_note_table
    add hl,de
    add hl,de
    ld c,(hl)
    inc hl
    ld a,(hl)
    ld e,a
    ld d,$A5
    call force_write_a
    ld e,c
    ld d,$A1
    call force_write_a
    ld de,$28F1
    call force_write_a
    ret

store_fm_ptr:
    ld a,l
    ld (var_fm_ptr_lo),a
    ld a,h
    ld (var_fm_ptr_hi),a
    ret

store_music_ptr:
    ld a,l
    ld (var_music_ptr_lo),a
    ld a,h
    ld (var_music_ptr_hi),a
    ret

tempo_to_frames_impl:
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
    ld a,(var_ssg_tick)
    inc a
    ld (var_ssg_tick),a
    ld b,a
    ld a,(var_ssg_tempo)
    or a
    ret z
    cp b
    ret nz
    xor a
    ld (var_ssg_tick),a
    ld a,(var_ssg_wait)
    or a
    jr z,ssg_step
    dec a
    ld (var_ssg_wait),a
    ret nz
    jp ssg_step

ssg_step:
    ld a,(var_ssg_ptr_lo)
    ld l,a
    ld a,(var_ssg_ptr_hi)
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
    ld b,a
    ld a,(hl)
    inc hl
    ld (var_ssg_wait),a
    call store_ssg_ptr
    jp ssg_standalone_note_on

ssg_set_tempo:
    ld a,(hl)
    inc hl
    call tempo_to_frames_impl
    ld (var_ssg_tempo),a
    call store_ssg_ptr
    jp ssg_step

ssg_set_volume:
    ld a,(hl)
    inc hl
    and $0F
    ld (var_ssg_vol),a
    call store_ssg_ptr
    call ssg_apply_standalone_volume
    jp ssg_step

ssg_set_preset:
    ld a,(hl)
    inc hl
    and $0F
    ld (var_ssg_preset),a
    call store_ssg_ptr
    call ssg_apply_preset
    jp ssg_step

ssg_rest:
    ld a,(hl)
    inc hl
    ld (var_ssg_wait),a
    call store_ssg_ptr
    jp init_ssg

ssg_loop:
    call init_ssg
    ld a,(var_ssg_start_lo)
    ld l,a
    ld a,(var_ssg_start_hi)
    ld h,a
    xor a
    ld (var_ssg_wait),a
    call store_ssg_ptr
    jp ssg_step

ssg_stop:
    xor a
    ld (var_ssg_active),a
    ld (var_ssg_wait),a
    jp init_ssg

store_ssg_ptr:
    ld a,l
    ld (var_ssg_ptr_lo),a
    ld a,h
    ld (var_ssg_ptr_hi),a
    ret

ssg_apply_preset:
    ld a,(var_ssg_preset)
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
    ld d,$07
    ld e,(hl)
    call ssg_preset_write_a
    inc hl
    ld d,$08
    ld a,(hl)
    and $0F
    ld e,a
    call ssg_preset_write_a
    inc hl
    ld d,$09
    ld a,(hl)
    and $0F
    ld e,a
    call ssg_preset_write_a
    inc hl
    ld d,$0A
    ld a,(hl)
    and $0F
    ld e,a
    call ssg_preset_write_a
    inc hl
    ld d,$06
    ld e,(hl)
    call ssg_preset_write_a
    inc hl
    inc hl
    ret

ssg_preset_write_a:
    push hl
    call shadowed_write_a
    pop hl
    ret

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
    ld de,$0738
    call shadowed_write_a
    ld d,$08
    ld a,(var_music_vol)
    and $0F
    ld e,a
    call shadowed_write_a
    ld d,$09
    ld a,(var_music_vol)
    and $0F
    srl a
    add a,3
    ld e,a
    jp shadowed_write_a

ssg_standalone_note_on:
    call ssg_note_on
    ld d,$08
    ld a,(var_ssg_vol)
    and $0F
    ld e,a
    call shadowed_write_a
    ld d,$09
    ld a,(var_ssg_vol)
    and $0F
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
}
