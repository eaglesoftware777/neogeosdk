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
extern unsigned char fm_patch_table[];
extern unsigned char ssg_preset_table[];
extern unsigned char fm_note_table[];
extern unsigned char ssg_track_table[];
extern unsigned char sample_address_table[];
asm {
.define fifo_buf $F800
.define fifo_read $F820
.define fifo_write $F821
.define shadow_a $F900
.define shadow_b $FA00
}
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
unsigned char tmp_a;
void driver_init(void);
void process_fifo(void);
void execute_command(void);
void ticker_update(void);
void init_ssg(void);
void init_fm(void);
void fm_silence_all(void);
void adpcma_stop(void);
void adpcmb_stop(void);
void apply_music_volume(void);
void apply_master_volumes(void);
void music_tick(void);
void fm_tick(void);
void ssg_tick(void);
void fade_tick(void);
void fm_apply_patch(void);
void ssg_apply_preset(void);
unsigned char tempo_to_frames(unsigned char bpm);
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
    out(0x18, 0);
    out(0x0C, 0);
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
    out(0x08, 0);
    out(0x0C, 0x01);
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
            write_b(0x01, var_adpcma_vol);
        }
        else if (var_wait_tempo == 3) {
            var_adpcmb_vol = var_command;
            var_adpcmb_base = var_adpcmb_vol;
            write_a(0x1B, var_adpcmb_vol);
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
        if (var_fm_ticks == 0) { force_a(0x28, 0x01); fm_silence_all(); }
    }
    if (var_adpcma_ticks > 0) {
        var_adpcma_ticks--;
        if (var_adpcma_ticks == 0) { adpcma_stop(); }
    }
    if (var_fm_active) fm_tick();
    if (var_ssg_active) ssg_tick();
    var_tick++;
    if (var_tick >= var_tempo) {
        var_tick = 0;
        if (var_music_active) music_tick();
    }
    fade_tick();
}
void fade_tick(void) {
    if (var_fade_mode == 0) return;
    if (var_fade_ticks > 0) {
        var_fade_ticks--;
        return;
    }
    var_fade_ticks = 255 - var_fade_speed;
    if (var_fade_ticks == 0) var_fade_ticks = 1;
    if (var_fade_mode == 1) {
        if (var_music_vol > 0) var_music_vol--;
        if (var_adpcma_vol > 0) var_adpcma_vol--;
        if (var_adpcmb_vol > 0) var_adpcmb_vol--;
        apply_master_volumes();
        if (var_music_vol == 0) { if (var_adpcma_vol == 0) { if (var_adpcmb_vol == 0) var_fade_mode = 0; } }
    }
    else if (var_fade_mode == 2) {
        if (var_music_vol < var_music_vol_base) var_music_vol++;
        if (var_adpcma_vol < var_adpcma_base) var_adpcma_vol++;
        if (var_adpcmb_vol < var_adpcmb_base) var_adpcmb_vol++;
        apply_master_volumes();
        if (var_music_vol == var_music_vol_base) { if (var_adpcma_vol == var_adpcma_base) { if (var_adpcmb_vol == var_adpcmb_base) var_fade_mode = 0; } }
    }
}
void apply_master_volumes(void) {
    apply_music_volume();
    write_b(0x01, var_adpcma_vol);
    write_a(0x1B, var_adpcmb_vol);
}
void adpcma_stop(void) { force_b(0x00, 0xBF); }
void adpcmb_stop(void) { force_a(0x10, 0x01); }
void fm_silence_all(void) {
    unsigned char idx;
    idx = 0x01;
    while (idx < (0x01 + 4)) {
        force_a(0x28, idx);
        force_a(0x28, idx + 4);
        idx++;
    }
    idx = 0x41;
    while (idx < 0x4D) {
        write_a(idx, 0x7F);
        write_b(idx, 0x7F);
        idx++;
    }
}
void init_ssg(void) {
    write_a(0x07, 0x3F);
    write_a(0x08, 0); write_a(0x09, 0); write_a(0x0A, 0);
    write_a(0x06, 0); write_a(0x0B, 0); write_a(0x0C, 0); write_a(0x0D, 0);
}
void init_fm(void) {
    write_a(0x26, 0x0F);
    force_a(0x27, 0x2A);
    write_a(0x27, 0x3A);
    fm_silence_all();
    write_a(0x22, 0x00);
}
void apply_music_volume(void) {
    write_a(0x08, var_music_vol & 0x0F);
    write_a(0x09, (var_music_vol & 0x0F) >> 1);
}
void fm_apply_patch(void) {
    unsigned char idx;
    unsigned char* p;
    unsigned char vol_scaling;
    idx = var_fm_patch;
    if (idx >= 16) idx = 0;
    p = fm_patch_table;
    while (idx > 0) { p += 31; idx--; }
    write_a(0x22, p[0]);
    write_a(0xB1, p[1]);
    write_a(0xB5, p[2]);
    vol_scaling = (15 - var_fm_vol) << 2;
    write_a(0x31, p[3]); write_a(0x41, p[4] + vol_scaling);
    write_a(0x35, p[10]); write_a(0x45, p[11] + vol_scaling);
}
void ssg_apply_preset(void) {
    unsigned char idx;
    unsigned char* p;
    idx = var_ssg_preset;
    if (idx >= 3) idx = 0;
    p = ssg_preset_table;
    while (idx > 0) { p += 6; idx--; }
    write_a(0x07, p[0]);
    write_a(0x08, p[1]);
    write_a(0x09, p[2]);
    write_a(0x0A, p[3]);
    write_a(0x06, p[4]);
}
void music_tick(void) {
    if (var_music_wait > 0) { var_music_wait--; return; }
}
void fm_tick(void) {
    if (var_fm_wait > 0) { var_fm_wait--; return; }
}
void ssg_tick(void) {
    if (var_ssg_wait > 0) { var_ssg_wait--; return; }
}
unsigned char tempo_to_frames(unsigned char bpm) {
    if (bpm >= 200) return 1;
    if (bpm >= 140) return 2;
    if (bpm >= 100) return 3;
    if (bpm >= 60) return 4;
    return 6;
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
