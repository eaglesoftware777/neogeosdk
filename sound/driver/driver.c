asm {
.include "music_data.inc"

.org $0000
reset:
    di
    jp main

.org $0008
    nop
    nop
    nop
    nop
    ret

.org $0066
nmi:
    push af
    in a,($00)
    ld (command),a
    xor a
    out ($00),a
    ld a,$01
    out ($0C),a
    pop af
    retn
}

#define CMD_INIT 0x01
#define CMD_RESET 0x03
#define CMD_STOP_ALL 0x04
#define CMD_SSG_DEMO 0x21
#define CMD_BATTLE_DEMO 0x22
#define CMD_TITLE_DEMO 0x24
#define CMD_DEMO_B0 0x28
#define CMD_DEMO_B1 0x29
#define MUSIC_BASE 0x20
#define MUSIC_LIMIT 0x40
#define SFX_A_BASE 0x40
#define SFX_A_LIMIT 0x80
#define SFX_B_BASE 0x80

#define ADPCMB_STOP() ym_a(0x10, 0x01)
#define ADPCMB_START() ym_a(0x10, 0x80)

unsigned char command;
unsigned char sample;
unsigned char reg;
unsigned char val;
unsigned char adpcma_channel;

void ym_a(unsigned char reg, unsigned char val) {
    asm {
    ld a,(reg)
    out ($04),a
    rst $08
    ld a,(val)
    out ($05),a
    rst $08
    }
}

void ym_b(unsigned char reg, unsigned char val) {
    asm {
    ld a,(reg)
    out ($06),a
    rst $08
    ld a,(val)
    out ($07),a
    rst $08
    }
}

void ready(void) {
    asm {
    ld a,$01
    out ($0C),a
    }
}

void clear_ready(void) {
    asm {
    xor a
    out ($0C),a
    }
}

void adpcma_stop(void) {
    ym_b(0x00, 0xbf);
}

void adpcmb_stop(void) {
    ADPCMB_STOP();
}

void ssg_off(void) {
    ym_a(0x07, 0x3f);
    ym_a(0x08, 0x00);
    ym_a(0x09, 0x00);
    ym_a(0x0a, 0x00);
}

void ssg_demo(void) {
    ym_a(0x00, 0x56);
    ym_a(0x01, 0x01);
    ym_a(0x07, 0x3e);
    ym_a(0x08, 0x1f);
}

void init_adpcma(void) {
    adpcma_stop();
    ym_b(0x01, 0x3f);
    ym_b(0x02, 0xdf);
    ym_b(0x03, 0xdf);
    ym_b(0x04, 0xdf);
    ym_b(0x05, 0xdf);
    ym_b(0x06, 0xdf);
    ym_b(0x08, 0xdf);
    ym_b(0x09, 0xdf);
    ym_b(0x0a, 0xdf);
    ym_b(0x0b, 0xdf);
    ym_b(0x0c, 0xdf);
    ym_b(0x0d, 0xdf);
}

void init_adpcmb(void) {
    ADPCMB_STOP();
    ym_a(0x11, 0xc0);
    ym_a(0x18, 0xba);
    ym_a(0x19, 0x49);
    ym_a(0x1b, 0xff);
}

void driver_init(void) {
    clear_ready();
    ym_a(0x27, 0x30);
    init_adpcma();
    init_adpcmb();
    ready();
    ei();
}

void stop_all(void) {
    ssg_off();
    adpcma_stop();
    adpcmb_stop();
}

void play_adpcma_index(void) {
    if (sample == 0) {
        ym_b(0x01, 0xdf);
        ym_b(0x08, 0x00);
        ym_b(0x10, 0x1c);
        ym_b(0x00, 0x81);
        return;
    }
    if (sample == 1) {
        ym_b(0x02, 0xdf);
        ym_b(0x09, 0x1d);
        ym_b(0x11, 0x3a);
        ym_b(0x00, 0x82);
        return;
    }
}

void play_adpcmb_index(void) {
    if (sample == 0) {
        ADPCMB_STOP();
        ym_a(0x12, 0x3b);
        ym_a(0x13, 0x00);
        ym_a(0x14, 0xf6);
        ym_a(0x15, 0x00);
        ym_a(0x10, 0x00);
        ADPCMB_START();
        return;
    }
    if (sample == 1) {
        ADPCMB_STOP();
        ym_a(0x12, 0xf7);
        ym_a(0x13, 0x00);
        ym_a(0x14, 0x2f);
        ym_a(0x15, 0x02);
        ym_a(0x10, 0x00);
        ADPCMB_START();
        return;
    }
}

void execute_command(void) {
    if (command == 0) {
        return;
    }
    if (command == CMD_INIT) {
        driver_init();
        command = 0;
        return;
    }
    if (command == CMD_RESET) {
        driver_init();
        command = 0;
        return;
    }
    if (command == CMD_STOP_ALL) {
        stop_all();
        command = 0;
        return;
    }
    if (command == CMD_SSG_DEMO) {
        ssg_demo();
        command = 0;
        return;
    }
    if (command == CMD_BATTLE_DEMO) {
        sample = 1;
        play_adpcmb_index();
        command = 0;
        return;
    }
    if (command == CMD_TITLE_DEMO) {
        sample = 1;
        play_adpcmb_index();
        command = 0;
        return;
    }
    if (command == CMD_DEMO_B0) {
        sample = 0;
        play_adpcmb_index();
        command = 0;
        return;
    }
    if (command == CMD_DEMO_B1) {
        sample = 1;
        play_adpcmb_index();
        command = 0;
        return;
    }
    if (command > 0x3f) {
        if (command < SFX_A_LIMIT) {
            sample = command - SFX_A_BASE;
            play_adpcma_index();
            command = 0;
            return;
        }
    }
    if (command > 0x1f) {
        if (command < MUSIC_LIMIT) {
            sample = command & 0x01;
            play_adpcmb_index();
            command = 0;
            return;
        }
    }
    if (command > 0x7f) {
        sample = command - SFX_B_BASE;
        play_adpcmb_index();
        command = 0;
        return;
    }
    command = 0;
}

void main(void) {
    di();
    command = 0;
    sample = 0;
    adpcma_channel = 0;
    driver_init();
    while (1) {
        execute_command();
        halt();
    }
}
