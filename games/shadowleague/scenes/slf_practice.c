#include <stdint.h>
#include "sdk/macro.h"
#include "sdk/neogeo.h"
#include "games/shadowleague/scenes/slf.h"
#include "games/shadowleague/artbox/generated/slf_assets.h"

uint8_t slf_home_difficulty;
uint8_t slf_home_rounds, slf_home_seconds;
volatile uint8_t slf_home_live, slf_training_active;
volatile uint8_t slf_home_selection;
NGFMatch slf_training_match;
static NGFTraining s_practice;

static const char *const dummy_names[NGF_DUMMY_COUNT] = {
    "STAND", "GUARD", "CROUCH", "JUMP", "CPU LEVEL 3", "PLAYER TWO", "RECORD", "PLAYBACK"
};

static void NEOGEO_USER number_line(uint8_t row, const char *label, uint8_t value)
{
    char digits[3];
    digits[0] = value >= 10u ? (char)('0' + value / 10u) : ' ';
    digits[1] = (char)('0' + value % 10u); digits[2] = 0;
    slf_puts(7u, row, label, SLF_PAL_TEXT);
    slf_puts(29u, row, digits, SLF_PAL_GOLD);
}

static void NEOGEO_USER options(void)
{
    uint8_t selected = 0u;
    slf_scene_begin(); slf_home_live = 2u;
    slf_stage_set(0u); slf_stage_show(96);
    slf_puts_tall_center(3u, "OPTIONS", SLF_TALL_GOLD);
    slf_puts_center(21u, "LEFT / RIGHT TO CHANGE", SLF_PAL_TEXT);
    slf_puts_center(23u, "A OR START TO RETURN", SLF_PAL_TEXT);
    slf_puts_center(26u, "VERSION 1.7.7", SLF_PAL_DIM);
    for (;;) {
        uint8_t edge;
        slf_frame(); edge = slf_pressed(0u);
        if (edge & NGF_RAW_UP) selected = (uint8_t)((selected + 2u) % 3u);
        if (edge & NGF_RAW_DOWN) selected = (uint8_t)((selected + 1u) % 3u);
        if (edge & (NGF_RAW_LEFT | NGF_RAW_RIGHT)) {
            uint8_t forward = (uint8_t)((edge & NGF_RAW_RIGHT) != 0u);
            if (selected == 0u) slf_home_difficulty = (uint8_t)(1u + (slf_home_difficulty + (forward ? 0u : 5u)) % 7u);
            else if (selected == 1u) slf_home_rounds = (uint8_t)(1u + (slf_home_rounds + (forward ? 0u : 1u)) % 3u);
            else slf_home_seconds = forward ? (slf_home_seconds == 90u ? 30u : (uint8_t)(slf_home_seconds + 30u)) :
                                                  (slf_home_seconds == 30u ? 90u : (uint8_t)(slf_home_seconds - 30u));
            slf_sfx(SLF_SFX_MENU);
        }
        number_line(9u, "CPU LEVEL", slf_home_difficulty);
        number_line(13u, "ROUNDS TO WIN", slf_home_rounds);
        number_line(17u, "ROUND TIME", slf_home_seconds);
        slf_puts(4u, 9u, selected == 0u ? ">" : " ", SLF_PAL_GOLD);
        slf_puts(4u, 13u, selected == 1u ? ">" : " ", SLF_PAL_GOLD);
        slf_puts(4u, 17u, selected == 2u ? ">" : " ", SLF_PAL_GOLD);
        if ((edge & (NGF_BTN_A | NGF_BTN_B | NGF_BTN_C | NGF_BTN_D)) || slf_start_pressed(0u)) break;
    }
    slf_home_live = 0u;
}

uint8_t NEOGEO_USER slf_home_menu(void)
{
    static const char *const names[4] = {"ARCADE", "VERSUS", "TRAINING", "OPTIONS"};
    uint8_t selected = 0u, i;
    uint16_t timer;
    /* Writable data shares the cleared NOLOAD RAM section on this cartridge. */
    if (slf_home_difficulty < 1u || slf_home_difficulty > 7u) slf_home_difficulty = SLF_DEFAULT_DIFFICULTY;
    if (slf_home_rounds < 1u || slf_home_rounds > 3u) slf_home_rounds = 2u;
    if (slf_home_seconds != 30u && slf_home_seconds != 60u && slf_home_seconds != 90u) slf_home_seconds = 60u;
    for (;;) {
        slf_scene_begin(); slf_home_live = 1u;
        slf_home_selection = selected;
        slf_stage_set(0u); slf_stage_show(96);
        slf_puts_tall_center(3u, "SHADOW LEAGUE", SLF_TALL_GOLD);
        slf_puts_center(24u, "UP / DOWN - A TO SELECT", SLF_PAL_TEXT);
        timer = 600u;
        for (;;) {
            uint8_t edge;
            slf_frame(); edge = slf_pressed(0u);
            if (edge & NGF_RAW_UP) selected = (uint8_t)((selected + 3u) & 3u);
            if (edge & NGF_RAW_DOWN) selected = (uint8_t)((selected + 1u) & 3u);
            slf_home_selection = selected;
            for (i = 0u; i < 4u; i++) {
                slf_puts_tall_center((uint8_t)(8u + i * 3u), names[i], selected == i ? SLF_TALL_GOLD : SLF_TALL_SILVER);
                slf_puts(7u, (uint8_t)(8u + i * 3u), selected == i ? ">" : " ", SLF_PAL_GOLD);
            }
            if ((edge & (NGF_BTN_A | NGF_BTN_C)) || slf_start_pressed(0u) || !--timer) break;
        }
        slf_home_live = 0u;
        if (selected != 3u) return selected;
        options();
    }
}

static void NEOGEO_USER practice_scene(const uint8_t *defs)
{
    slf_scene_begin(); slf_fx_reset();
    slf_stage_set(slf_fighter_stages[defs[1]]);
    slf_fighter_palettes(defs[0], 0u, defs[1], (uint8_t)(defs[0] == defs[1]));
    slf_puts(1u, 7u, "TRAINING", SLF_PAL_BLUE);
    slf_puts_center(26u, "START: TRAINING MENU", SLF_PAL_GOLD);
}

static void NEOGEO_USER practice_commands(uint8_t def)
{
    uint8_t i, row;
    slf_scene_begin();
    slf_puts_tall_center(1u, "COMMAND PRACTICE", SLF_TALL_GOLD);
    slf_puts_center(4u, slf_fighter_defs[def]->name, SLF_PAL_WHITE);
    slf_puts(2u,6u,"A: LIGHT PUNCH   B: LIGHT KICK",SLF_PAL_TEXT);
    slf_puts(2u,7u,"C: HEAVY PUNCH   D: HEAVY KICK",SLF_PAL_TEXT);
    slf_puts(2u,9u,"CLOSE + FORWARD C: LONG THROW",SLF_PAL_BLUE);
    slf_puts(2u,10u,"CLOSE + FORWARD D: SHORT THROW",SLF_PAL_BLUE);
    slf_puts(2u,11u,"BACK + C/D: REVERSE THROW",SLF_PAL_BLUE);
    slf_puts(2u,13u,"COMBO: A > C > SPECIAL",SLF_PAL_GOLD);
    slf_puts(2u,14u,"CANCEL AFTER THE STRIKE CONNECTS",SLF_PAL_TEXT);
    row=16u;
    for (i=0u;i<6u;i++) if (slf_command_guides[def][i])
        slf_puts(2u,row++,slf_command_guides[def][i],SLF_PAL_TEXT);
    slf_puts_center(25u,"A / B / START TO RETURN",SLF_PAL_GOLD);
    for (;;) {
        slf_frame();
        if (slf_start_pressed(0u) || (slf_pressed(0u)&(NGF_BTN_A|NGF_BTN_B))) break;
    }
}

static uint8_t NEOGEO_USER practice_menu(uint8_t def)
{
    uint8_t selected = 0u, i;
    static const char *const labels[7] = {"RESUME", "DUMMY", "POWER", "LIFE RECOVERY", "RESET POSITION", "COMMAND LIST", "EXIT TRAINING"};
    slf_scene_begin(); slf_home_live = 3u;
    slf_puts_tall_center(2u, "TRAINING MENU", SLF_TALL_GOLD);
    slf_puts_center(25u, "LEFT / RIGHT - A TO SELECT", SLF_PAL_TEXT);
    for (;;) {
        uint8_t edge;
        slf_frame(); edge = slf_pressed(0u);
        if (edge & NGF_RAW_UP) selected = (uint8_t)((selected + 6u) % 7u);
        if (edge & NGF_RAW_DOWN) selected = (uint8_t)((selected + 1u) % 7u);
        if (edge & (NGF_RAW_LEFT | NGF_RAW_RIGHT)) {
            if (selected == 1u) ngf_training_set_dummy(&s_practice, &slf_training_match,
                (uint8_t)((s_practice.dummy + ((edge & NGF_RAW_RIGHT) ? 1u : NGF_DUMMY_COUNT - 1u)) % NGF_DUMMY_COUNT));
            if (selected == 2u) s_practice.infinite_power ^= 1u;
            if (selected == 3u) s_practice.recover_life ^= 1u;
        }
        for (i = 0u; i < 7u; i++) {
            uint8_t row = (uint8_t)(5u + i * 3u);
            slf_puts(3u, row, selected == i ? ">" : " ", SLF_PAL_GOLD);
            slf_puts(5u, row, labels[i], selected == i ? SLF_PAL_GOLD : SLF_PAL_TEXT);
        }
        slf_puts(23u, 8u, "              ", SLF_PAL_TEXT);
        slf_puts(23u, 8u, dummy_names[s_practice.dummy], SLF_PAL_BLUE);
        slf_puts(24u, 11u, s_practice.infinite_power ? "INFINITE" : "NORMAL  ", SLF_PAL_BLUE);
        slf_puts(24u, 14u, s_practice.recover_life ? "ON " : "OFF", SLF_PAL_BLUE);
        if (slf_start_pressed(0u) || (edge & NGF_BTN_B)) { slf_home_live = 0u; return 0u; }
        if (edge & (NGF_BTN_A | NGF_BTN_C)) {
            if (selected == 0u || selected == 6u) { slf_home_live = 0u; return (uint8_t)(selected == 6u); }
            if (selected == 5u) { practice_commands(def); slf_home_live=0u; return 0u; }
            if (selected == 4u) { ngf_training_reset(&s_practice, &slf_training_match); slf_home_live = 0u; return 0u; }
        }
    }
}

void NEOGEO_USER slf_training_run(uint8_t fighter, uint8_t partner)
{
    uint8_t defs[2];
    char input[16];
    NGFMatch *m = &slf_training_match;
    input[0] = 'P'; input[1] = '1'; input[2] = ':'; input[3] = ' ';
    input[5] = input[7] = input[9] = input[11] = input[13] = ' '; input[15] = 0;
    defs[0] = fighter; defs[1] = partner;
    ngf_match_init(m, slf_fighter_defs[fighter], slf_fighter_defs[partner], SLF_STAGE_W, slf_rand());
    ngf_training_init(&s_practice, m);
    slf_training_active = 1u;
    slf_music((uint8_t)(SLF_TRACK_STAGE + slf_fighter_stages[partner]));
    practice_scene(defs);
    for (;;) {
        uint8_t raw;
        slf_frame();
        if (slf_start_pressed(0u)) {
            if (practice_menu(fighter)) break;
            practice_scene(defs);
        }
        raw = slf_joy(0u);
        ngf_training_step(&s_practice, m, raw, slf_joy(1u));
        slf_combat_events(m, defs);
        slf_match_draw(m, defs); slf_hud_draw(m);
        input[4] = (raw & NGF_BTN_A) ? 'A' : '-'; input[6] = (raw & NGF_BTN_B) ? 'B' : '-';
        input[8] = (raw & NGF_BTN_C) ? 'C' : '-'; input[10] = (raw & NGF_BTN_D) ? 'D' : '-';
        input[12] = (raw & NGF_RAW_UP) ? 'U' : (raw & NGF_RAW_DOWN) ? 'D' : '-';
        input[14] = (raw & NGF_RAW_LEFT) ? 'L' : (raw & NGF_RAW_RIGHT) ? 'R' : '-';
        slf_puts(1u, 23u, input, SLF_PAL_TEXT);
        slf_puts(23u, 23u, dummy_names[s_practice.dummy], SLF_PAL_BLUE);
    }
    slf_training_active = 0u; m->training = 0u;
}
