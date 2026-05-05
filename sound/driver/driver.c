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

.org $0100
}

#define READY_VALUE 0x01

// RAM layout managed by z80cc starting at $FE00
// We use a manual ramsection for FIFO and Shadows
asm {
.ramsection "manual_ram" slot 0 OFFSET $F800
fifo_buf: dsb 32
fifo_read: dsb 1
fifo_write: dsb 1
.ends
.ramsection "shadow_ram" slot 0 OFFSET $F900
shadow_a: dsb 256
.ends
.ramsection "shadow_ram_b" slot 0 OFFSET $FA00
shadow_b: dsb 256
.ends
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

// Constants
#define MUSIC_BASE 0x20
#define SFX_A_BASE 0x40
#define SFX_B_BASE 0x80
#define SSG_CMD_PRESET 0x14
#define SSG_CMD_PLAY 0x32

void ym_wait(void) {
    asm { rst $08 }
}

void shadowed_write_a(unsigned char r, unsigned char v) {
    reg = r;
    val = v;
    asm {
    ld a,(reg)
    ld l,a
    ld h,$F9 ; shadow_a
    ld a,(hl)
    ld b,a
    ld a,(val)
    cp b
    ret z ; No change
    ld (hl),a
    ld a,(reg)
    out ($04),a
    rst $08
    ld a,(val)
    out ($05),a
    rst $08
    }
}

void shadowed_write_b(unsigned char r, unsigned char v) {
    reg = r;
    val = v;
    asm {
    ld a,(reg)
    ld l,a
    ld h,$FA ; shadow_b
    ld a,(hl)
    ld b,a
    ld a,(val)
    cp b
    ret z ; No change
    ld (hl),a
    ld a,(reg)
    out ($06),a
    rst $08
    ld a,(val)
    out ($07),a
    rst $08
    }
}

void force_write_a(unsigned char r, unsigned char v) {
    reg = r;
    val = v;
    asm {
    ld a,(reg)
    ld l,a
    ld h,$F9 ; shadow_a
    ld a,(val)
    ld (hl),a
    ld a,(reg)
    out ($04),a
    rst $08
    ld a,e
    out ($05),a
    rst $08
    }
}

void force_write_b(unsigned char r, unsigned char v) {
    reg = r;
    val = v;
    asm {
    ld a,(reg)
    ld l,a
    ld h,$FA ; shadow_b
    ld a,(val)
    ld (hl),a
    ld a,(reg)
    out ($06),a
    rst $08
    ld a,e
    out ($07),a
    rst $08
    }
}

void adpcma_stop(void) {
    force_write_b(0x00, 0xBF);
}

void adpcmb_stop(void) {
    force_write_a(0x10, 0x01);
}

void init_ssg(void) {
    shadowed_write_a(0x07, 0x3F);
    shadowed_write_a(0x06, 0x00);
    shadowed_write_a(0x08, 0x00);
    shadowed_write_a(0x09, 0x00);
    shadowed_write_a(0x0A, 0x00);
    shadowed_write_a(0x0B, 0x00);
    shadowed_write_a(0x0C, 0x00);
    shadowed_write_a(0x0D, 0x00);
}

void fm_silence_all(void) {
    tmp_a = 1;
    while (tmp_a < 5) {
        force_write_a(0x28, tmp_a);
        tmp_b = tmp_a + 4;
        force_write_a(0x28, tmp_b);
        tmp_a = tmp_a + 1;
    }
    tmp_a = 0x0D;
    reg = 0x41;
    while (tmp_a > 0) {
        shadowed_write_a(reg, 0x7F);
        shadowed_write_b(reg, 0x7F);
        reg = reg + 1;
        tmp_a = tmp_a - 1;
    }
}

void init_fm(void) {
    shadowed_write_a(0x26, 0x0F);
    force_write_a(0x27, 0x2A);
    shadowed_write_a(0x27, 0x3A);
    fm_silence_all();
    shadowed_write_a(0x22, 0x00);
}

void init_adpcma(void) {
    adpcma_stop();
    shadowed_write_b(0x01, var_adpcma_vol);
    tmp_a = 6;
    reg = 8;
    while (tmp_a > 0) {
        shadowed_write_b(reg, 0xC0);
        reg = reg + 1;
        tmp_a = tmp_a - 1;
    }
}

void init_adpcmb(void) {
    force_write_a(0x10, 0x01);
    force_write_a(0x1C, 0x80);
    force_write_a(0x1C, 0x00);
    force_write_a(0x10, 0x00);
    shadowed_write_a(0x11, 0xC0);
    shadowed_write_a(0x19, 0xBA);
    shadowed_write_a(0x1A, 0x49);
    shadowed_write_a(0x1B, var_adpcmb_vol);
}

void apply_music_volume(void) {
    shadowed_write_a(0x08, var_music_vol & 0x0F);
    tmp_a = (var_music_vol & 0x0F) >> 1;
    shadowed_write_a(0x09, tmp_a + 3);
}

void apply_master_volumes(void) {
    shadowed_write_b(0x01, var_adpcma_vol);
    shadowed_write_a(0x1B, var_adpcmb_vol);
    apply_music_volume();
}

void driver_init(void) {
    di();
    out(0x18, 0); // Disable NMIs
    out(0x0C, 0); // Status busy
    var_music_active = 0;
    var_music_wait = 0;
    var_fade_mode = 0;
    var_fm_active = 0;
    var_ssg_active = 0;
    var_ssg_vol = 0x0A;
    var_tempo = 3;
    var_music_vol = 0x0F;
    var_music_vol_base = 0x0F;
    var_adpcma_vol = 0x3F;
    var_adpcma_base = 0x3F;
    var_adpcmb_vol = 0xA8;
    var_adpcmb_base = 0xA8;

    force_write_a(0x01, 0x00);
    force_write_b(0x01, 0x00);

    init_ssg();
    init_fm();
    init_adpcma();
    init_adpcmb();

    out(0x08, 0); // Enable NMIs
    out(0x0C, READY_VALUE);
    ei();
}

void stop_all(void) {
    var_music_active = 0;
    var_fm_active = 0;
    var_ssg_active = 0;
    init_ssg();
    fm_silence_all();
    adpcma_stop();
    adpcmb_stop();
}

unsigned char tempo_to_frames(unsigned char t) {
    if (t >= 180) return 1;
    if (t >= 140) return 2;
    if (t >= 100) return 3;
    if (t >= 70) return 4;
    return 5;
}

void fm_stop(void) {
    var_fm_active = 0;
    force_write_a(0x28, 0x01);
    fm_silence_all();
}

void execute_command(void) {
    if (var_param_mode != 0) {
        var_param_mode = 0;
        if (var_wait_tempo == 1) { var_tempo = var_command; }
        else if (var_wait_tempo == 2) { 
            var_adpcma_vol = var_command & 0x3F;
            var_adpcma_base = var_adpcma_vol;
            shadowed_write_b(0x01, var_adpcma_vol);
        }
        else if (var_wait_tempo == 3) {
            var_adpcmb_vol = var_command;
            var_adpcmb_base = var_adpcmb_vol;
            shadowed_write_a(0x1B, var_adpcmb_vol);
        }
        else if (var_wait_tempo == 4) {
            var_music_vol = var_command & 0x0F;
            var_music_vol_base = var_music_vol;
            apply_music_volume();
        }
        else if (var_wait_tempo == 10) {
            var_ssg_preset = var_command & 0x0F;
            // ssg_apply_preset() to be implemented
        }
        else if (var_wait_tempo == 13) {
            var_fm_vol = var_command & 0x0F;
            var_fm_vol_base = var_fm_vol;
            // fm_apply_patch() to be implemented
        }
        return;
    }

    if (var_command == 0) return;
    if (var_command == 0x01) { driver_init(); return; }
    if (var_command == 0x03) { driver_init(); return; } // Soft reset
    if (var_command == 0x04) { stop_all(); return; }
    if (var_command == 0x05) { var_param_mode = 1; var_wait_tempo = 2; return; } // ADPCM-A volume
    if (var_command == 0x06) { var_param_mode = 1; var_wait_tempo = 3; return; } // ADPCM-B volume
    if (var_command == 0x07) { var_param_mode = 1; var_wait_tempo = 4; return; } // SSG volume
    if (var_command == 0x0C) { adpcma_stop(); return; }
    if (var_command == 0x0D) { adpcmb_stop(); return; }
    if (var_command == 0x0E) { var_param_mode = 1; var_wait_tempo = 1; return; } // Tempo
    if (var_command == 0x13) { var_param_mode = 1; var_wait_tempo = 13; return; } // FM volume
    if (var_command == SSG_CMD_PRESET) { var_param_mode = 1; var_wait_tempo = 10; return; }
}

void ticker_update(void) {
    if (var_fm_ticks > 0) {
        var_fm_ticks = var_fm_ticks - 1;
        if (var_fm_ticks == 0) fm_stop();
    }
    if (var_adpcma_ticks > 0) {
        var_adpcma_ticks = var_adpcma_ticks - 1;
        if (var_adpcma_ticks == 0) adpcma_stop();
    }
    
    // SSG tick, Music tick, etc. to be implemented
    var_tick = var_tick + 1;
    if (var_tick == var_tempo) {
        var_tick = 0;
        // music_tick(), fade_tick()
    }
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
        execute_command();
        fifo_read = (fifo_read + 1) & 0x1F;
    }
}

void main(void) {
    di();
    fifo_read = 0;
    fifo_write = 0;
    driver_init();
    while (1) {
        process_fifo();
        halt();
    }
}

asm {
.include "fm_patch_table.inc"
.include "fm_data.inc"
.include "music_data.inc"
.include "ssg_config.inc"
.include "ssg_data.inc"
.include "sample_table.inc"
}
