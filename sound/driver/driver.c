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
    FORCE_A(0x27, 0x2A)
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
    ld h,$F8
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
asm {
.define fifo_buf   $F800
.define fifo_read  $F820
.define fifo_write $F821
.define shadow_a   $F900
.define shadow_b   $FA00
}

// Global variables - Order matches driver.asm
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

// Prototypes
void driver_init(void);
void process_fifo(void);
void ticker_update(void);
void init_ssg(void);
void init_fm(void);
void fm_silence_all(void);
void adpcma_stop(void);
void apply_music_volume(void);
void apply_master_volumes(void);
void execute_command(void);

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
    ld hl,shadow_a
    ld de,shadow_a + 1
    ld bc,$01FF
    ld (hl),a
    ldir
    }
    driver_init();
    while (1) {
        process_fifo();
        halt();
    }
}

void driver_init(void) {
    di();
    out(DRIVER_PORT_NMI_DISABLE, 0);
    out(DRIVER_PORT_STATUS, 0);
    var_music_active = 0;
    var_fm_active = 0;
    var_ssg_active = 0;
    var_fade_mode = 0;
    var_tempo = 3;
    var_music_vol = 0x0F;
    var_music_vol_base = 0x0F;
    var_adpcma_vol = 0x3F;
    var_adpcma_base = 0x3F;
    var_adpcmb_vol = 0xA8;
    var_adpcmb_base = 0xA8;
    
    init_ssg();
    init_fm();
    
    out(DRIVER_PORT_NMI_ENABLE, 0);
    out(DRIVER_PORT_STATUS, 1);
    ei();
}

void process_fifo(void) {
    while (fifo_read != fifo_write) {
        asm {
        ld a,(fifo_read)
        ld l,a
        ld h,>fifo_buf
        ld a,(hl)
        ld (var_command),a
        }
        fifo_read = (fifo_read + 1) & 0x1F;
        execute_command();
    }
}

void execute_command(void) {
    if (var_param_mode != 0) {
        var_param_mode = 0;
        if (var_wait_tempo == 1) { var_tempo = var_command; }
        else if (var_wait_tempo == 2) { 
            var_adpcma_vol = var_command & 0x3F;
            var_adpcma_base = var_adpcma_vol;
            WRITE_B(0x01, var_adpcma_vol);
        }
        else if (var_wait_tempo == 3) {
            var_adpcmb_vol = var_command;
            var_adpcmb_base = var_adpcmb_vol;
            WRITE_A(0x1B, var_adpcmb_vol);
        }
        else if (var_wait_tempo == 4) {
            var_music_vol = var_command & 0x0F;
            var_music_vol_base = var_music_vol;
            apply_music_volume();
        }
        return;
    }

    if (var_command == 0) return;
    if (var_command == 0x01) { driver_init(); return; }
    if (var_command == 0x03) { driver_init(); return; }
    if (var_command == 0x05) { var_param_mode = 1; var_wait_tempo = 2; return; }
    if (var_command == 0x06) { var_param_mode = 1; var_wait_tempo = 3; return; }
    if (var_command == 0x07) { var_param_mode = 1; var_wait_tempo = 4; return; }
    if (var_command == 0x0E) { var_param_mode = 1; var_wait_tempo = 1; return; }
}

void ticker_update(void) {
    if (var_fm_ticks > 0) {
        var_fm_ticks--;
        if (var_fm_ticks == 0) { FORCE_A(0x28, 0x01); fm_silence_all(); }
    }
    if (var_adpcma_ticks > 0) {
        var_adpcma_ticks--;
        if (var_adpcma_ticks == 0) { adpcma_stop(); }
    }
    var_tick++;
    if (var_tick >= var_tempo) {
        var_tick = 0;
    }
}

void adpcma_stop(void) { FORCE_B(0x00, 0xBF); }

void fm_silence_all(void) {
    unsigned char idx;
    idx = 0x01;
    while (idx < 5) {
        FORCE_A(0x28, idx);
        FORCE_A(0x28, idx + 4);
        idx++;
    }
    idx = 0x41;
    while (idx < 0x4D) {
        WRITE_A(idx, 0x7F);
        WRITE_B(idx, 0x7F);
        idx++;
    }
}

void init_ssg(void) {
    WRITE_A(0x07, 0x3F);
    WRITE_A(0x08, 0); WRITE_A(0x09, 0); WRITE_A(0x0A, 0);
    WRITE_A(0x06, 0); WRITE_A(0x0B, 0); WRITE_A(0x0C, 0); WRITE_A(0x0D, 0);
}

void init_fm(void) {
    WRITE_A(0x26, 0x0F);
    FORCE_A(0x27, 0x2A);
    WRITE_A(0x27, 0x3A);
    fm_silence_all();
    WRITE_A(0x22, 0x00);
}

void apply_music_volume(void) {
    WRITE_A(0x08, var_music_vol);
    WRITE_A(0x09, var_music_vol >> 1);
}

asm {
shadowed_write_a:
    ld h,>shadow_a
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
    ld h,>shadow_b
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
    ld h,>shadow_a
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
    ld h,>shadow_b
    ld l,d
    ld (hl),e
    ld a,d
    out ($06),a
    rst $08
    ld a,e
    out ($07),a
    rst $08
    ret

.include "fm_patch_table.inc"
.include "fm_data.inc"
.include "music_data.inc"
.include "ssg_config.inc"
.include "ssg_data.inc"
.include "sample_table.inc"
}
