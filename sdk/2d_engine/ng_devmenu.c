/*
 * ng_devmenu.c — A developer's test menu (see ng_devmenu.h)
 */

#include "ng_devmenu.h"
#include "ng_fix.h"
#include "neogeo.h"

NGDev ng_dev;
static NGDevConfig s_cfg;
static uint8_t s_dirty;

enum { PAGE_MAIN, PAGE_WATCH, PAGE_SOUND };
enum { ROW_SPEED, ROW_TIMER, ROW_CAMERA, ROW_HITBOX, ROW_WATCH, ROW_SOUND, ROWS };

#define W 24u          /* the menu's width in cells */
#define H 9u

static const char *const k_speed[5] = {"FULL", "1/2 ", "1/4 ", "1/8 ", "STEP"};

static void NEOGEO_USER hex(char *out, uint32_t v, uint8_t digits)
{
    static const char k_hex[] = "0123456789ABCDEF";
    uint8_t i;
    for (i = 0; i < digits; i++) out[i] = k_hex[(v >> ((digits - 1u - i) * 4u)) & 15u];
    out[digits] = 0;
}

static void NEOGEO_USER dec5(char *out, uint16_t v)
{
    uint8_t i;
    for (i = 0; i < 5u; i++) { out[4u - i] = (char)('0' + v % 10u); v = (uint16_t)(v / 10u); }
    out[5] = 0;
}

static void NEOGEO_USER line(uint8_t row, const char *label, const char *value)
{
    uint8_t pal = (ng_dev.row == row) ? s_cfg.pal_hi : s_cfg.pal;
    ng_fix_puts((uint8_t)(s_cfg.x + 1u), (uint8_t)(s_cfg.y + 1u + row), label, pal);
    if (value) ng_fix_puts((uint8_t)(s_cfg.x + 12u), (uint8_t)(s_cfg.y + 1u + row), value, pal);
}

static void NEOGEO_USER draw(void)
{
    char buf[12];
    uint8_t k;
    ng_fix_clear_rect(s_cfg.x, s_cfg.y, W, H, s_cfg.pal);
    if (ng_dev.page == PAGE_MAIN) {
        ng_fix_puts((uint8_t)(s_cfg.x + 1u), s_cfg.y, "TEST MENU", s_cfg.pal_hi);
        line(ROW_SPEED, "SPEED", k_speed[ng_dev.speed]);
        line(ROW_TIMER, "TIMER", ng_dev.timer_stop ? "STOPPED" : "RUNS");
        line(ROW_CAMERA, "CAMERA", ng_dev.free_cam ? "FREE" : "GAME");
        line(ROW_HITBOX, "HIT BOXES", ng_dev.hitboxes ? "ON" : "OFF");
        line(ROW_WATCH, "WATCH", "A");
        line(ROW_SOUND, "SOUND", "A");
    } else if (ng_dev.page == PAGE_WATCH) {
        ng_fix_puts((uint8_t)(s_cfg.x + 1u), s_cfg.y, "WATCH  L/R +-16 B BACK", s_cfg.pal_hi);
        for (k = 0; k < 4u; k++) {
            uint32_t a = ng_dev.watch[k] & 0xFFFFFEuL;
            uint8_t pal = ng_dev.row == k ? s_cfg.pal_hi : s_cfg.pal;
            hex(buf, a, 6);
            ng_fix_puts((uint8_t)(s_cfg.x + 1u), (uint8_t)(s_cfg.y + 2u + k), buf, pal);
            hex(buf, *(volatile const uint16_t *)(uintptr_t)a, 4);
            ng_fix_puts((uint8_t)(s_cfg.x + 9u), (uint8_t)(s_cfg.y + 2u + k), buf, pal);
        }
    } else {
        ng_fix_puts((uint8_t)(s_cfg.x + 1u), s_cfg.y, "SOUND  A PLAY B BACK", s_cfg.pal_hi);
        ng_fix_puts((uint8_t)(s_cfg.x + 1u), (uint8_t)(s_cfg.y + 2u), "UP DOWN +-1", s_cfg.pal);
        ng_fix_puts((uint8_t)(s_cfg.x + 1u), (uint8_t)(s_cfg.y + 3u), "LEFT RIGHT +-10", s_cfg.pal);
        dec5(buf, ng_dev.sound);
        ng_fix_puts((uint8_t)(s_cfg.x + 1u), (uint8_t)(s_cfg.y + 5u), "CODE", s_cfg.pal);
        ng_fix_puts((uint8_t)(s_cfg.x + 8u), (uint8_t)(s_cfg.y + 5u), buf, s_cfg.pal_hi);
    }
}

void NEOGEO_USER ng_dev_init(const NGDevConfig *cfg)
{
    uint8_t k;
    if (cfg) s_cfg = *cfg;
    if (!s_cfg.hold && !s_cfg.press) { s_cfg.hold = START1; s_cfg.press = BUTTON_D; }
    if (!s_cfg.up) {
        s_cfg.up = JOY_UP; s_cfg.down = JOY_DOWN; s_cfg.left = JOY_LEFT; s_cfg.right = JOY_RIGHT;
        s_cfg.a = BUTTON_A; s_cfg.b = BUTTON_B; s_cfg.d = BUTTON_D;
    }
    if (!s_cfg.y) s_cfg.y = 4;
    if (!s_cfg.x) s_cfg.x = 8;
    ng_dev.open = 0;
    ng_dev.page = PAGE_MAIN;
    ng_dev.row = 0;
    ng_dev.speed = 0;
    ng_dev.timer_stop = ng_dev.free_cam = ng_dev.hitboxes = 0;
    ng_dev.cam_x = ng_dev.cam_y = 0;
    for (k = 0; k < 4u; k++) ng_dev.watch[k] = 0x100000uL + (uint32_t)k * 2u;
    ng_dev.sound = 0;
    ng_dev.frame = 0;
}

static void NEOGEO_USER close_menu(void)
{
    ng_fix_clear_rect(s_cfg.x, s_cfg.y, W, H, s_cfg.pal);
    ng_dev.open = 0;
    if (s_cfg.on_close) s_cfg.on_close();
}

static void NEOGEO_USER menu_input(uint16_t held, uint16_t pressed)
{
    int8_t lr = (int8_t)((pressed & s_cfg.right) ? 1 : ((pressed & s_cfg.left) ? -1 : 0));
    if (ng_dev.page == PAGE_MAIN) {
        if (pressed & s_cfg.up) { ng_dev.row = ng_dev.row ? (uint8_t)(ng_dev.row - 1u) : (uint8_t)(ROWS - 1u); s_dirty = 1; }
        if (pressed & s_cfg.down) { ng_dev.row = (uint8_t)((ng_dev.row + 1u) % ROWS); s_dirty = 1; }
        if (lr) {
            if (ng_dev.row == ROW_SPEED) ng_dev.speed = (uint8_t)((ng_dev.speed + (lr > 0 ? 1u : 4u)) % 5u);
            else if (ng_dev.row == ROW_TIMER) ng_dev.timer_stop ^= 1u;
            else if (ng_dev.row == ROW_CAMERA) { ng_dev.free_cam ^= 1u; ng_dev.cam_x = ng_dev.cam_y = 0; }
            else if (ng_dev.row == ROW_HITBOX) ng_dev.hitboxes ^= 1u;
            s_dirty = 1;
        }
        if (pressed & s_cfg.a) {
            if (ng_dev.row == ROW_WATCH) { ng_dev.page = PAGE_WATCH; ng_dev.row = 0; s_dirty = 1; }
            else if (ng_dev.row == ROW_SOUND) { ng_dev.page = PAGE_SOUND; s_dirty = 1; }
        }
        if (pressed & s_cfg.b) close_menu();
        return;
    }
    if (ng_dev.page == PAGE_WATCH) {
        if (pressed & s_cfg.up) { ng_dev.row = (uint8_t)((ng_dev.row + 3u) & 3u); s_dirty = 1; }
        if (pressed & s_cfg.down) { ng_dev.row = (uint8_t)((ng_dev.row + 1u) & 3u); s_dirty = 1; }
        if (lr) {
            /* with A held, a whole 256 at a time */
            uint32_t step = (held & s_cfg.a) ? 256u : 16u;
            uint32_t *a = &ng_dev.watch[ng_dev.row & 3u];
            *a = lr > 0 ? *a + step : *a - step;
            s_dirty = 1;
        }
        if (pressed & s_cfg.b) { ng_dev.page = PAGE_MAIN; ng_dev.row = ROW_WATCH; s_dirty = 1; }
        if ((ng_dev.frame & 7u) == 0) s_dirty = 1;          /* the values are live */
        return;
    }
    if (pressed & s_cfg.up) { ng_dev.sound++; s_dirty = 1; }
    if (pressed & s_cfg.down) { ng_dev.sound--; s_dirty = 1; }
    if (lr) { ng_dev.sound = (uint16_t)(ng_dev.sound + (lr > 0 ? 10 : -10)); s_dirty = 1; }
    if (pressed & s_cfg.a) { if (s_cfg.play_sound) s_cfg.play_sound(ng_dev.sound); else playSoundtest(ng_dev.sound); }
    if (pressed & s_cfg.b) {
        if (s_cfg.stop_sound) s_cfg.stop_sound(); else soundStopAll();
        ng_dev.page = PAGE_MAIN;
        ng_dev.row = ROW_SOUND;
        s_dirty = 1;
    }
}

uint8_t NEOGEO_USER ng_dev_frame(uint16_t held, uint16_t pressed)
{
    ng_dev.frame++;
    if ((held & s_cfg.hold) && (pressed & s_cfg.press)) {
        if (ng_dev.open) close_menu();
        else { ng_dev.open = 1; ng_dev.page = PAGE_MAIN; ng_dev.row = 0; s_dirty = 1; }
        return 0;
    }
    if (ng_dev.open) {
        menu_input(held, pressed);
        if (ng_dev.open && s_dirty) { s_dirty = 0; draw(); }
        return 0;
    }
    if (ng_dev.free_cam) {
        if (held & s_cfg.left) ng_dev.cam_x -= 2;
        if (held & s_cfg.right) ng_dev.cam_x += 2;
        if (held & s_cfg.up) ng_dev.cam_y -= 2;
        if (held & s_cfg.down) ng_dev.cam_y += 2;
    }
    switch (ng_dev.speed) {
    case 1: return (uint8_t)((ng_dev.frame & 1u) == 0);
    case 2: return (uint8_t)((ng_dev.frame & 3u) == 0);
    case 3: return (uint8_t)((ng_dev.frame & 7u) == 0);
    case 4: return (uint8_t)((pressed & s_cfg.d) != 0);
    default: return 1;
    }
}
