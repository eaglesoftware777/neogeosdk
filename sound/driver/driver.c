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

#define FM_PATCH_COUNT 16
#define FM_PATCH_SIZE 31
#define SSG_PRESET_COUNT 3
#define SSG_PRESET_SIZE 6

extern unsigned char fm_patch_table[];
extern unsigned char ssg_preset_table[];

// RAM layout managed by z80cc starting at $FE00
asm {
.define fifo_buf   $F800
.define fifo_read  $F820
.define fifo_write $F821
.define shadow_a   $F900
.define shadow_b   $FA00
}

// Global variables
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

// Temp variables
unsigned char reg;
unsigned char val;
unsigned char tmp_a;
unsigned char tmp_b;
unsigned char tmp_c;
unsigned char tmp_d;
unsigned char tmp_e;
unsigned char tmp_hl_lo;
unsigned char tmp_hl_hi;

void ym_wait(void) {
    asm { rst $08 }
}

void shadowed_write_a_c(unsigned char r, unsigned char v) {
    asm {
    ld a,(r)
    ld d,a
    ld a,(v)
    ld e,a
    call shadowed_write_a
    }
}

void shadowed_write_b_c(unsigned char r, unsigned char v) {
    asm {
    ld a,(r)
    ld d,a
    ld a,(v)
    ld e,a
    call shadowed_write_b
    }
}

void force_write_a_c(unsigned char r, unsigned char v) {
    asm {
    ld a,(r)
    ld d,a
    ld a,(v)
    ld e,a
    call force_write_a
    }
}

void force_write_b_c(unsigned char r, unsigned char v) {
    asm {
    ld a,(r)
    ld d,a
    ld a,(v)
    ld e,a
    call force_write_b
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
    unsigned char i;
    i = DRIVER_FM_KEY_CH1;
    while (i < (DRIVER_FM_KEY_CH1 + DRIVER_FM_CHANNEL_COUNT)) {
        force_write_a_c(DRIVER_FM_KEY_REG, i);
        force_write_a_c(DRIVER_FM_KEY_REG, i + 4);
        i = i + 1;
    }
    i = DRIVER_FM_SILENCE_TL_WRITES;
    reg = 0x41;
    while (i > 0) {
        shadowed_write_a_c(reg, 0x7F);
        shadowed_write_b_c(reg, 0x7F);
        reg = reg + 1;
        i = i - 1;
    }
}

void init_fm(void) {
    shadowed_write_a_c(0x26, 0x0F);
    force_write_a_c(0x27, 0x2A);
    shadowed_write_a_c(0x27, 0x3A);
    fm_silence_all();
    shadowed_write_a_c(0x22, 0x00);
}

void apply_music_volume(void) {
    shadowed_write_a_c(0x08, var_music_vol & 0x0F);
    tmp_a = (var_music_vol & 0x0F) >> 1;
    shadowed_write_a_c(0x09, tmp_a + 3);
}

void fm_write_operator_patch(unsigned char base, unsigned char* p) {
    unsigned char scaled_tl;
    unsigned char vol_scaling;

    // Reg x1: DT/MULTI
    shadowed_write_a_c(base, p[0]);
    
    // Reg x5: TL (Total Level) - Scaled by FM Volume
    // scaling = (15 - volume) * 4
    vol_scaling = (15 - var_fm_vol) << 2;
    scaled_tl = p[1] + vol_scaling;
    if (scaled_tl > 0x7F) scaled_tl = 0x7F;
    shadowed_write_a_c(base + 0x10, scaled_tl);

    // Reg x9: AR/RS
    shadowed_write_a_c(base + 0x20, p[2]);
    // Reg xD: DR/AM
    shadowed_write_a_c(base + 0x30, p[3]);
    // Reg x1: SR (next operator bank...)
    // Actually the registers are grouped by operators.
    // This is simplified.
}

void fm_apply_patch(void) {
    unsigned char idx;
    unsigned char* p;
    idx = var_fm_patch;
    if (idx >= FM_PATCH_COUNT) idx = 0;
    
    // Manual pointer math since compiler * is simple
    p = fm_patch_table;
    tmp_a = idx;
    while (tmp_a > 0) {
        p = p + FM_PATCH_SIZE;
        tmp_a = tmp_a - 1;
    }

    shadowed_write_a_c(0x22, p[0]);
    shadowed_write_a_c(0xB1, p[1]);
    shadowed_write_a_c(0xB5, p[2]);

    // Operators 1-4 starting at offset 3
    fm_write_operator_patch(0x31, p + 3);
    fm_write_operator_patch(0x35, p + 10);
    fm_write_operator_patch(0x39, p + 17);
    fm_write_operator_patch(0x3D, p + 24);
}

void ssg_apply_preset(void) {
    unsigned char idx;
    unsigned char i;
    unsigned char* p;
    idx = var_ssg_preset;
    if (idx >= SSG_PRESET_COUNT) idx = 0;

    p = ssg_preset_table;
    i = idx;
    while (i > 0) {
        p = p + SSG_PRESET_SIZE;
        i = i - 1;
    }

    shadowed_write_a_c(0x07, p[0]);
    shadowed_write_a_c(0x08, p[1] & 0x0F);
    shadowed_write_a_c(0x09, p[2] & 0x0F);
    shadowed_write_a_c(0x0A, p[3] & 0x0F);
    shadowed_write_a_c(0x06, p[4]);
}

void driver_init(void) {
    di();
    out(DRIVER_PORT_NMI_DISABLE, 0);
    out(DRIVER_PORT_STATUS, 0);
    var_music_active = 0;
    var_music_wait = 0;
    var_wait_tempo = 0;
    var_tick = 0;
    var_adpcma_ch = 0;
    var_adpcma_ticks = 0;
    var_param_mode = DRIVER_PARAM_NONE;
    var_fade_mode = 0;
    var_ssg_vol = DRIVER_DEFAULT_SSG_VOLUME;
    var_tempo = DRIVER_DEFAULT_TEMPO_FRAMES;
    var_music_vol = DRIVER_DEFAULT_MUSIC_VOLUME;
    var_music_vol_base = DRIVER_DEFAULT_MUSIC_VOLUME;
    var_adpcma_vol = DRIVER_DEFAULT_ADPCMA_VOLUME;
    var_adpcma_base = DRIVER_DEFAULT_ADPCMA_VOLUME;
    var_adpcmb_vol = DRIVER_DEFAULT_ADPCMB_VOLUME;
    var_adpcmb_base = DRIVER_DEFAULT_ADPCMB_VOLUME;
    
    init_ssg();
    init_fm();
    
    out(DRIVER_PORT_NMI_ENABLE, 0);
    out(DRIVER_PORT_STATUS, DRIVER_READY_VALUE);
    ei();
}

void execute_command(void) {
    if (var_param_mode != DRIVER_PARAM_NONE) {
        var_param_mode = DRIVER_PARAM_NONE;
        if (var_wait_tempo == DRIVER_PARAM_TEMPO) { var_tempo = var_command; }
        else if (var_wait_tempo == DRIVER_PARAM_ADPCMA_VOLUME) { 
            var_adpcma_vol = var_command & 0x3F;
            var_adpcma_base = var_adpcma_vol;
            shadowed_write_b_c(DRIVER_ADPCMA_MASTER_REG, var_adpcma_vol);
        }
        else if (var_wait_tempo == DRIVER_PARAM_ADPCMB_VOLUME) {
            var_adpcmb_vol = var_command;
            var_adpcmb_base = var_adpcmb_vol;
            shadowed_write_a_c(DRIVER_ADPCMB_VOLUME_REG, var_adpcmb_vol);
        }
        else if (var_wait_tempo == DRIVER_PARAM_SSG_VOLUME) {
            var_music_vol = var_command & 0x0F;
            var_music_vol_base = var_music_vol;
            apply_music_volume();
        }
        else if (var_wait_tempo == DRIVER_PARAM_SSG_PRESET) {
            var_ssg_preset = var_command & 0x0F;
            ssg_apply_preset();
        }
        else if (var_wait_tempo == DRIVER_PARAM_FM_VOLUME) {
            var_fm_vol = var_command & 0x0F;
            var_fm_vol_base = var_fm_vol;
            fm_apply_patch();
        }
        return;
    }

    if (var_command == 0) return;
    if (var_command == 0x01) { driver_init(); return; }
    if (var_command == 0x03) { driver_init(); return; }
    if (var_command == DRIVER_CMD_SET_ADPCMA_VOLUME) { var_param_mode = 1; var_wait_tempo = DRIVER_PARAM_ADPCMA_VOLUME; return; }
    if (var_command == DRIVER_CMD_SET_ADPCMB_VOLUME) { var_param_mode = 1; var_wait_tempo = DRIVER_PARAM_ADPCMB_VOLUME; return; }
    if (var_command == DRIVER_CMD_SET_SSG_VOLUME) { var_param_mode = 1; var_wait_tempo = DRIVER_PARAM_SSG_VOLUME; return; }
    if (var_command == DRIVER_CMD_SET_TEMPO) { var_param_mode = 1; var_wait_tempo = DRIVER_PARAM_TEMPO; return; }
    if (var_command == DRIVER_CMD_SET_FM_VOLUME) { var_param_mode = 1; var_wait_tempo = DRIVER_PARAM_FM_VOLUME; return; }
    if (var_command == DRIVER_CMD_SET_SSG_PRESET) { var_param_mode = 1; var_wait_tempo = DRIVER_PARAM_SSG_PRESET; return; }
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

void ticker_update(void) {
    if (var_fm_ticks > 0) {
        var_fm_ticks = var_fm_ticks - 1;
    }
}

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
    call driver_init
main_loop_c:
    call process_fifo
    halt
    jp main_loop_c
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

.include "fm_patch_table.inc"
.include "fm_data.inc"
.include "music_data.inc"
.include "ssg_config.inc"
.include "ssg_data.inc"
.include "sample_table.inc"
}
