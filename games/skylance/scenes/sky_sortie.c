/*
 * sky_sortie.c — Sky Lance front end and flight loop.
 *
 * Attract reel, pilot select, the sortie itself and the continue prompt.
 * The director in sky_stage.c owns the enemies; this file owns the
 * player, collisions, scoring and the HUD.
 *
 * https://eaglesoftware.biz
 */

#include "sky.h"
#include "sky_draw.h"
#include "sky_stage.h"
#include "sky_combat.h"
#include "games/skylance/artbox/generated/sky_native_assets.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "sdk/sound_voice_ids.h"
#include "sdk/2d_engine/ng_defs.h"
#include "sdk/2d_engine/ng_chars.h"
#include "sdk/2d_engine/ng_fixed.h"

void NEOGEO_USER waitVbl(void);
void NEOGEO_USER clearFix(void);
void NEOGEO_USER setBACKDROP(uint16_t backdrop_color);
void NEOGEO_USER playSFX(uint8_t n);
void NEOGEO_USER playSFXB(uint8_t n);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundCancelFade(void);
void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v);
void NEOGEO_USER soundSetADPCMBVolume(uint8_t v);
void NEOGEO_USER soundSetADPCMBLoop(uint8_t enable);
void NEOGEO_USER soundSetSSGVolume(uint8_t v);
void NEOGEO_USER soundSetFMVolume(uint8_t v);
int  NEOGEO_USER read_p1credit(void);

#define NGO_START_FLAG  SKY_START_FLAG

/* ------------------------------------------------------------------ */
/*  Pilots                                                              */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t plane;
    uint8_t face;
    const char *name;
    const char *call;
    uint8_t fire_gap;    /* frames between shots  */
    uint8_t speed;       /* pixels per frame      */
    uint8_t twin;        /* 1 = fires a twin bolt */
} SkyPilot;

/*
 * Three pilots, differentiated the way the arcade original does it -
 * not by damage numbers but by feel.  Rook is the baseline, Kira trades
 * rate of fire for a wider twin bolt, Blaze is fast and rapid but has to
 * get close because her shots are single.
 */
#define SKY_PILOTS 3u
static const SkyPilot k_pilot[SKY_PILOTS] = {
    { SKY_P1_PLANE, SKY_P1_PILOT, "ROOK",  "BLUE LANCE",  8u, 5u, 0u },
    { SKY_P2_PLANE, SKY_P2_PILOT, "KIRA",  "GREEN WING", 11u, 5u, 1u },
    { SKY_P3_PLANE, SKY_P3_PILOT, "BLAZE", "RED STRIKER", 6u, 7u, 0u }
};

/* ------------------------------------------------------------------ */
/*  Run state                                                           */
/* ------------------------------------------------------------------ */
static uint8_t  s_pick;
static uint8_t  s_lives;
static uint8_t  s_energy;
static uint8_t  s_invuln;
static uint8_t  s_fire_cd;
static uint8_t  s_stage;
static uint32_t s_score;
static uint32_t s_hiscore;
static NGCharacter *s_player;
static uint8_t s_missiles;
static SkyResources s_resources;
static uint16_t s_resource_hud;
static uint16_t s_sortie_frames;
static uint16_t s_plane_pose;
static int8_t s_bank;
static SkyRoll s_roll;

static const uint8_t k_bank_pose[3][7] = {
    {35u, 72u, 73u, 4u, 74u, 75u, 36u},
    {37u, 76u, 77u, 6u, 78u, 79u, 38u},
    {39u, 80u, 81u, 8u, 82u, 83u, 40u}
};
static uint32_t s_hud_score, s_hud_high;
static uint8_t s_hud_lives, s_hud_energy, s_hud_missiles, s_hud_boss;
static uint8_t s_hud_targets;
static uint8_t s_combo_count, s_combo_timer;
static uint8_t s_boss_critical_alert, s_boss_warned;
static uint8_t s_special_banner;
static uint8_t s_hud_gmeter = 0xFFu;
static uint8_t s_hud_thrust = 0xFFu;
static uint8_t s_radio_timer;
static char s_radio_msg[38];
static uint8_t s_hit_flash;
static NGCharacter *s_hud_face;
static int16_t s_flight_vx, s_flight_vy;
static int8_t s_loop_dir;
static uint8_t s_maneuver_cooldown;
static uint8_t s_roll_maneuver;
static int8_t  s_roll_dir;
static uint8_t s_loop_maneuver;
static uint16_t s_prev_joy;
static uint8_t s_cruise_called;      /* the flight was sent on to the arena */

#define SKY_ENERGY_MAX   100u
#define SKY_ENERGY_HIT    34u
#define SKY_START_LIVES    3u
#define SKY_LIVES_MAX      6u

/* ------------------------------------------------------------------ */
/*  HUD                                                                 */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER sky_radio_chatter(const char *msg, uint8_t sfx)
{
    uint8_t k;
    if (!msg) return;
    for (k = 0u; k < 30u && msg[k] != '\0'; k++) s_radio_msg[k] = msg[k];
    s_radio_msg[k] = '\0';
    s_radio_timer = 120u;
    sky_fix_blank(2u, 4u, 30u);
    sky_puts(2u, 4u, s_radio_msg, (msg[1] == 'A') ? SKY_PAL_SCORE : SKY_PAL_TITLE);
    if (sfx) playSFX(sfx);
}

static void NEOGEO_USER sky_hud_cockpit(void)
{
    uint8_t g = (s_loop_maneuver || s_roll_maneuver) ? 3u :
                s_resources.boosting ? 2u : s_roll.held > 12u ? 1u : 0u;
    int8_t bank = s_bank;
    if (bank < -3) bank = -3;
    if (bank > 3) bank = 3;
    /* The instruments stand in the HUD column right of the field (x 272-304)
     * under the portrait: the attitude indicator follows the jet's bank; the
     * radar scope sweeps, red under a radar lock or a guided missile still
     * on the jet; the G meter's needle reads the load, its lamp the
     * afterburner. (sky_instrument_show ignores an unchanged frame.) */
    sky_instrument_show(0u, (uint16_t)(SKY_ATTITUDE_IND_0 + bank + 3), 272, 66);
    sky_instrument_show(1u, (uint16_t)(SKY_RADAR_SCOPE_0 + ((s_sortie_frames >> 3) & 7u) +
                        ((sky_stage_radar_alert() || sky_stage_seekers()) ? 8u : 0u)), 272, 101);
    sky_instrument_show(2u, (uint16_t)(SKY_G_METER_0 + g * 2u + (s_resources.boosting != 0u)), 272, 136);
    if (g != s_hud_gmeter) {
        s_hud_gmeter = g;
    }
    if (s_hud_thrust != s_resources.boosting) {
        s_hud_thrust = s_resources.boosting;
    }
    /* Portrait position and colors stay fixed through fire, damage and roll. */
}

static void NEOGEO_USER sky_hud_cockpit_remove(void)
{
    if (s_hud_face) ng_chars_remove(s_hud_face);
    s_hud_face = 0;
    sky_instruments_hide();
}

/*
 * Row ownership on the 40x28 FIX grid:
 *   1..2    SCORE label and counter (cols 2..18), HI (cols 20..33),
 *           aircraft and callsign (cols 34..37)
 *   3       TGT, stage, radar, flight state, CHAFF stock
 *   4       ATC & Pilot radio chatter
 *   5       boss health bar (cols 8..31), while a boss is alive
 *   22..24  equipment captions, icons and counters
 *   25..26  lives pips and energy cells
 * Everything between belongs to the playfield.
 */
static void NEOGEO_USER sky_hud_static(void)
{
    char digit[2];
    sky_infix(2u,  1u, SKY_INFIX_SCORE);
    sky_infix(20u, 1u, SKY_INFIX_HI);
    sky_infix(2u, 25u, (uint8_t)(SKY_INFIX_1P + (s_pick % 3u)));
    /* The route line: which of the seven this is, and the missile rack.
     * The digits are written on their own so no string is built here. */
    sky_puts(11u, 3u, "/7", SKY_PAL_BODY);
    sky_puts(2u, 22u, "PWR", SKY_PAL_BODY);
    sky_puts(8u, 22u, "AMMO", SKY_PAL_BODY);
    sky_puts(14u, 22u, "ARM", SKY_PAL_BODY);
    sky_puts(20u, 22u, "FUEL", SKY_PAL_BODY);
    sky_puts(27u, 22u, "C:BMB", SKY_PAL_BODY);
    sky_puts(34u, 22u, "SPD", SKY_PAL_BODY);
    sky_infix(2u, 23u, 18u);
    sky_infix(8u, 23u, 19u);
    sky_infix(14u, 23u, 20u);
    sky_infix(20u, 23u, 21u);
    sky_infix(27u, 23u, 22u);
    sky_infix(34u, 23u, 23u);
    digit[1] = '\0';
    digit[0] = (char)('1' + s_stage);
    sky_puts(10u, 3u, digit, SKY_PAL_BODY);

    {
        static const char *models[3] = {"F-16", "F-35", "F-15"};
        static const char *calls[3] = {"ROOK", "KIRA", "BLZE"};
        sky_puts(34u, 1u, models[s_pick % 3u], SKY_PAL_TITLE);
        sky_puts(34u, 2u, calls[s_pick % 3u], SKY_PAL_SCORE);
    }
    sky_puts(32u, 3u, "CHAFF", SKY_PAL_BODY);

    s_hud_gmeter = 0xFFu;
    s_hud_thrust = 0xFFu;

    s_hud_score = s_hud_high = 0xFFFFFFFFu;
    s_hud_lives = s_hud_energy = s_hud_missiles = s_hud_boss = 0xFFu;
    s_resource_hud = 0xFFFFu;
    s_hud_targets = 0xFFu;
    s_radio_timer = 0u;
}

static void NEOGEO_USER sky_hud_draw(void)
{
    uint8_t i;
    uint8_t targets = sky_stage_targets();
    uint8_t fuel = s_resources.fuel;
    uint16_t state = (uint16_t)(fuel | (s_resources.power << 8) |
                               (s_resources.cores << 11) | (s_resources.shields << 13) |
                               (s_resources.turbo << 15));

    sky_hud_cockpit();

    if (s_radio_timer) {
        if (--s_radio_timer == 0u) {
            sky_fix_blank(2u, 4u, 30u);
        }
    }
    if (s_special_banner) s_special_banner--;

    {
        uint8_t event = sky_stage_target_alert();
        if (event == 1u) sky_radio_chatter("TARGET AHEAD: DEFEAT DEFENDERS", SOUND_SFX_12);
        else if (event == 2u) sky_radio_chatter("DEFENDERS DOWN: DESTROY TARGET", SOUND_SFX_6);
        else if (event == 3u) sky_radio_chatter("TARGET DOWN: ROUTE RESUMING", SOUND_SFX_6);
        else if (event == 4u) sky_radio_chatter("RADAR LOCK: REINFORCEMENTS", SOUND_SFX_12);
    }
    {
        /* Threats blink in the warning ink: a radar lock, a missile on the jet. */
        uint8_t blink = (uint8_t)((s_sortie_frames >> 4) & 1u);
        if (sky_stage_radar_alert())
            sky_puts(14u, 3u, "RADAR:ALERT", blink ? SKY_PAL_WARN : SKY_PAL_BODY);
        else
            sky_puts(14u, 3u, "RADAR:QUIET", SKY_PAL_BODY);
        if (sky_stage_seekers())
            sky_puts(26u, 3u, "LOCKED", blink ? SKY_PAL_WARN : SKY_PAL_SCORE);
        else
            sky_puts(26u, 3u, sky_stage_encounter() == 3u ? "TARGET" :
                     sky_stage_paused() ? "DEFEND" : "FLIGHT", SKY_PAL_SCORE);
    }
    {
        char stock[2];
        stock[0] = (char)('0' + sky_stage_chaff_stock()); stock[1] = '\0';
        sky_puts(37u, 3u, stock, SKY_PAL_SCORE);
    }

    if (targets != s_hud_targets) {
        char label[] = "TGT 0/3";
        label[4] = (char)('0' + targets);
        sky_puts(2u, 3u, label, SKY_PAL_SCORE);
        if (s_hud_targets != 0xFFu && targets > s_hud_targets) {
            sky_radio_chatter("COMMAND TARGET DESTROYED +1000", SOUND_SFX_6);
        }
        s_hud_targets = targets;
    }
    if (state != s_resource_hud) {
        char number[4];
        number[1] = '\0';
        number[0] = (char)('0' + s_resources.power);
        sky_puts(5u, 24u, number, SKY_PAL_SCORE);
        number[0] = (char)('0' + s_resources.shields);
        sky_puts(17u, 24u, number, SKY_PAL_SCORE);
        number[0] = (char)('0' + s_resources.cores);
        sky_puts(30u, 24u, number, SKY_PAL_SCORE);
        number[0] = (char)('0' + s_resources.turbo);
        sky_puts(37u, 24u, number, SKY_PAL_SCORE);
        number[0] = (char)('0' + fuel / 100u);
        number[1] = (char)('0' + (fuel / 10u) % 10u);
        number[2] = (char)('0' + fuel % 10u);
        number[3] = '\0';
        sky_puts(23u, 24u, number, fuel <= 30u ? SKY_PAL_WARN : SKY_PAL_SCORE);
        s_resource_hud = state;
    }

    if (s_hud_score != s_score) {
        sky_infix_number(7u, 1u, s_score, 6u);
        s_hud_score = s_score;
    }
    if (s_hud_high != s_hiscore) {
        sky_infix_number(22u, 1u, s_hiscore, 6u);
        s_hud_high = s_hiscore;
    }
    if (s_hud_missiles != s_missiles) {
        char ammo[2];
        ammo[0] = (char)('0' + s_missiles); ammo[1] = '\0';
        sky_puts(11u, 24u, ammo, SKY_PAL_SCORE);
        s_hud_missiles = s_missiles;
    }

    /* Lives pips: one per spare plane, blanked back out as they go. */
    if (s_hud_lives != s_lives) for (i = 0u; i < SKY_LIVES_MAX; i++) {
        if (i < s_lives) sky_infix((uint8_t)(4u + i * 2u), 25u, SKY_INFIX_LIFE);
        else {
            sky_fix_blank((uint8_t)(4u + i * 2u), 25u, 2u);
            sky_fix_blank((uint8_t)(4u + i * 2u), 26u, 2u);
        }
    }
    s_hud_lives = s_lives;

    if (s_hud_energy != s_energy) {
        char hp[5];
        hp[0] = (char)('0' + s_energy / 100u);
        hp[1] = (char)('0' + (s_energy / 10u) % 10u);
        hp[2] = (char)('0' + s_energy % 10u);
        hp[3] = '%'; hp[4] = '\0';
        /* cyan while sound, gold once damaged, red when one more hit is fatal */
        uint8_t ink = s_energy <= 34u ? SKY_PAL_WARN : s_energy <= 67u ? SKY_PAL_SCORE : SKY_PAL_TITLE;
        sky_puts(20u, 25u, "HULL", SKY_PAL_BODY);
        sky_puts(26u, 25u, hp, ink);
        sky_meter(20u, 26u, 18u, (uint8_t)(((uint16_t)s_energy * 18u + 99u) / 100u), ink);
    }
    s_hud_energy = s_energy;
}

static void NEOGEO_USER sky_hud_boss(void)
{
    uint8_t n = sky_stage_boss_bar();

    if (n > 0u && n <= 6u && !s_boss_critical_alert) {
        s_boss_critical_alert = 1u;
        sky_radio_chatter("[PILOT] HOSTILE BOSS CRITICAL! FINISH IT!", SOUND_SFX_13);
    }
    if (s_hud_boss == n) return;
    s_hud_boss = n;

    /* A beaten boss or mini-boss leaves no empty bar behind. */
    if (!n) sky_fix_blank(8u, 5u, 24u);
    else sky_meter(8u, 5u, 24u, n, SKY_PAL_WARN);
}

/* ------------------------------------------------------------------ */
/*  Player                                                              */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER sky_fire(void)
{
    SkyShot shots[3];
    uint8_t count, i;
    int16_t y;

    if (!s_player || s_fire_cd) return;
    count = sky_weapon_pattern(s_pick, s_resources.power, shots);
    if (sky_count_kind(SKY_KIND_PSHOT) + count > SKY_MAX_PSHOTS) return;

    y = (int16_t)(s_player->y - 16);
    for (i = 0u; i < count; i++) {
        NGCharacter *a = sky_spawn(SKY_KIND_PSHOT,
            (uint8_t)(SKY_WEAPON_FIRST + shots[i].art),
            (int16_t)(s_player->x + shots[i].x), y, SKY_SCALE_FULL, NG_RENDER_BAND_FRONT);
        if (a) {
            a->data0 = shots[i].damage;
            ng_char_set_speed(a, shots[i].vx, -8);
            ng_char_set_body(a, -3, -7, 6, 14);
        }
    }

    s_fire_cd = sky_weapon_gap(s_pick, s_resources.power);
    playSFX(s_pick == 1u ? SOUND_SFX_14 : s_pick == 2u ? SOUND_SFX_15 : SOUND_SFX_4);
}

static uint8_t NEOGEO_USER sky_player_hit(void)
{
    if (s_invuln) return 0u;
    s_hit_flash = 14u;
    if (s_player) {
        sky_spawn_blast(s_player->x, s_player->y);
        sky_spawn_smoke(s_player->x, (int16_t)(s_player->y + 14));
    }
    sky_radio_chatter("[ATC] CAUTION PILOT! HULL BREACH DETECTED!", SOUND_SFX_12);
    if (s_resources.shields) {
        s_resources.shields--;
        s_invuln = 60u;
        playSFX(SOUND_SFX_2);
        return 0u;
    }

    if (s_energy > SKY_ENERGY_HIT) {
        s_energy = (uint8_t)(s_energy - SKY_ENERGY_HIT);
        s_invuln = 90u;
        playSFX(SOUND_SFX_2);
        return 0u;
    }

    /* Out of energy: lose the plane. */
    if (s_player) sky_spawn_blast(s_player->x, s_player->y);
    playSFX(SOUND_SFX_5);
    s_energy = SKY_ENERGY_MAX;
    if (s_lives == 0u) return 1u;        /* squad down */
    s_lives--;
    sky_resources_death(&s_resources);
    if (s_resources.cores < 1u) s_resources.cores = 1u;
    s_invuln = 150u;
    if (s_player) {
        ng_char_set_pos(s_player, SKY_FIELD_X + SKY_FIELD_W / 2, SKY_FIELD_BOTTOM - 46);
        s_bank = 0;
        s_roll.angle = s_roll.held = s_roll.direction = 0;
    }
    return 0u;
}

static void NEOGEO_USER sky_player_step(void)
{
    static uint8_t smoke_tick;
    uint16_t joy = sky_joy();
    uint16_t pressed = joy & (uint16_t)~s_prev_joy;
    int16_t y, dx, dy;
    int32_t next_x, next_y;

    s_prev_joy = joy;

    if (!s_player || !s_player->active) return;

    if (s_maneuver_cooldown) s_maneuver_cooldown--;
    if ((pressed & BUTTON_B) && !s_maneuver_cooldown &&
        !s_roll_maneuver && !s_loop_maneuver && !(joy & BUTTON_D)) {
        if (joy & (JOY_LEFT | JOY_RIGHT)) {
            s_roll_maneuver = 30u;
            s_roll_dir = (joy & JOY_LEFT) ? -1 : 1;
            s_invuln = 36u;
            s_maneuver_cooldown = 75u;
            sky_radio_chatter("EVASIVE BARREL ROLL", SOUND_SFX_8);
        } else if (joy & (JOY_UP | JOY_DOWN)) {
            s_loop_maneuver = 30u;
            s_loop_dir = (joy & JOY_UP) ? -1 : 1;
            s_invuln = 36u;
            s_maneuver_cooldown = 75u;
            sky_radio_chatter(s_loop_dir < 0 ? "HIGH-G CLIMB" : "SPLIT-S DIVE", SOUND_SFX_7);
        }
    }

    y = s_player->y;
    sky_resources_tick(&s_resources, (uint8_t)((joy & BUTTON_B) != 0u));
    dx = (joy & JOY_RIGHT ? 1 : 0) - (joy & JOY_LEFT ? 1 : 0);
    dy = (joy & JOY_DOWN ? 1 : 0) - (joy & JOY_UP ? 1 : 0);
    s_flight_vx = sky_flight_axis(s_flight_vx, (int8_t)dx, s_pick, s_resources.boosting);
    s_flight_vy = sky_flight_axis(s_flight_vy, (int8_t)dy, s_pick, s_resources.boosting);
    next_x = s_player->x_fp + (dx && dy ? (int32_t)s_flight_vx * 181 / 256 : s_flight_vx);
    next_y = s_player->y_fp + (dx && dy ? (int32_t)s_flight_vy * 181 / 256 : s_flight_vy);
    y = (int16_t)(next_y / NG_FP_ONE);

    if (s_roll_maneuver) {
        uint8_t pose;
        uint8_t roll_idx;
        s_roll_maneuver--;
        roll_idx = (uint8_t)((30u - s_roll_maneuver) * 7u / 30u);
        if (roll_idx >= 7u) roll_idx = 6u;
        if (roll_idx == 0u || roll_idx == 6u) pose = k_pilot[s_pick].plane;
        else if (roll_idx == 1u) pose = k_bank_pose[s_pick][s_roll_dir < 0 ? 0u : 6u];
        else if (roll_idx == 5u) pose = k_bank_pose[s_pick][s_roll_dir < 0 ? 6u : 0u];
        else pose = (uint8_t)(SKY_MANEUVER_FIRST + s_pick * 4u +
                         (s_roll_dir < 0 ? 4u-roll_idx : roll_idx-2u));
        s_bank = (int8_t)(s_roll_dir * 3);
        if (pose != s_plane_pose) {
            sky_bind(s_player, pose, SKY_SCALE_PLAYER, NG_RENDER_BAND_PLAYER);
            s_plane_pose = pose;
        }
        for (NGCharacter *b = chars_at(0u), *end = b + ng_chars_slots_used(); b < end; b++) {
            if (b && b->active && b->kind == SKY_KIND_ESHOT) {
                int16_t bdx = (int16_t)(b->x - s_player->x);
                int16_t bdy = (int16_t)(b->y - s_player->y);
                if (bdx > -36 && bdx < 36 && bdy > -36 && bdy < 36) {
                    sky_spawn_blast(b->x, b->y);
                    ng_chars_remove(b);
                }
            }
        }
    } else if (s_loop_maneuver) {
        s_loop_maneuver--;
        y = (int16_t)(y + s_loop_dir * 2);
        next_y = (int32_t)y * NG_FP_ONE;
        {
            uint8_t pose = (uint8_t)(SKY_MANEUVER_FIRST + s_pick * 4u +
                            (s_loop_dir < 0 ? 3u : 1u));
            if (pose != s_plane_pose) {
                sky_bind(s_player, pose, SKY_SCALE_PLAYER, NG_RENDER_BAND_PLAYER);
                s_plane_pose = pose;
            }
        }
        sky_spawn_smoke((int16_t)(s_player->x - 6), (int16_t)(s_player->y + 14));
        sky_spawn_smoke((int16_t)(s_player->x + 6), (int16_t)(s_player->y + 14));
        for (NGCharacter *b = chars_at(0u), *end = b + ng_chars_slots_used(); b < end; b++) {
            if (b && b->active && b->kind == SKY_KIND_ESHOT) {
                int16_t bdx = (int16_t)(b->x - s_player->x);
                int16_t bdy = (int16_t)(b->y - s_player->y);
                if (bdx > -28 && bdx < 28 && bdy > -48 && bdy < 24) {
                    sky_spawn_blast(b->x, b->y);
                    ng_chars_remove(b);
                }
            }
        }
    } else {
        uint16_t pose;
        s_bank = sky_roll_step(&s_roll, (int8_t)dx, s_resources.boosting);
        pose = k_bank_pose[s_pick % SKY_PILOTS][s_bank + 3];
        if(s_energy<=34u) {
            static const uint16_t first[3]={SKY_NATIVE_DAMAGE_0_0,SKY_NATIVE_DAMAGE_1_0,SKY_NATIVE_DAMAGE_2_0};
            pose=(uint16_t)(first[s_pick]+s_bank+3);
        } else if(s_resources.boosting && !s_bank) {
            static const uint16_t first[3]={SKY_NATIVE_BOOST_0_0,SKY_NATIVE_BOOST_1_0,SKY_NATIVE_BOOST_2_0};
            pose=(uint16_t)(first[s_pick]+((s_sortie_frames>>2)&1u));
        }
        if (pose != s_plane_pose) {
            sky_bind(s_player, pose, SKY_SCALE_PLAYER, NG_RENDER_BAND_PLAYER);
            ng_char_set_body(s_player, -6, -6, 12, 12);
            s_plane_pose = pose;
        }
    }

    next_x = NG_CLAMP(next_x, (int32_t)(SKY_FIELD_LEFT + 12) * NG_FP_ONE,
                      (int32_t)(SKY_FIELD_RIGHT - 12) * NG_FP_ONE);
    next_y = NG_CLAMP(next_y, (int32_t)(SKY_FIELD_TOP + 40) * NG_FP_ONE,
                      (int32_t)(SKY_FIELD_BOTTOM - 30) * NG_FP_ONE);
    ng_char_set_pos(s_player, (int16_t)(next_x / NG_FP_ONE), (int16_t)(next_y / NG_FP_ONE));
    s_player->x_fp = next_x;
    s_player->y_fp = next_y;

    /* Engine exhaust trail when boosting */
    if (s_resources.boosting) {
        if (++smoke_tick >= 12u) {
            smoke_tick = 0u;
            sky_spawn_smoke(s_player->x, (int16_t)(s_player->y + 16));
        }
    } else smoke_tick = 0u;

    /* Auto-fire on hold, like the arcade board: A is a trigger, not a
     * typing test. */
    if (joy & BUTTON_A) sky_fire();
    if (((pressed & BUTTON_C) || (sky_joy_pressed() & BUTTON_C)) && sky_special_begin(&s_resources)) {
        s_invuln = SKY_SPECIAL_FRAMES + 20u;
        s_special_banner = 60u;
        sky_radio_chatter("[PILOT] SUPER BOMB DETONATING!", SOUND_SFX_10);
        playSFX(SOUND_VOICE_WORD_GO);
        playSFX(SOUND_SFX_10);
        sky_spawn_blast(s_player->x, s_player->y);
        sky_spawn_blast(80, 100);
        sky_spawn_blast(240, 100);
    }
    if (s_fire_cd) s_fire_cd--;
    if ((pressed & BUTTON_D) && (joy & BUTTON_B)) {
        uint8_t chaff = sky_stage_chaff(s_player->x, s_player->y);
        if (chaff == 1u)
            sky_radio_chatter("CHAFF RELEASE: MISSILE DECOY", SOUND_SFX_8);
        else if (chaff == 2u)
            sky_radio_chatter("[PILOT] STILL LOCKED! BREAK!", SOUND_SFX_12);
    } else if ((sky_joy_pressed() & BUTTON_D) && s_missiles &&
        sky_count_kind(SKY_KIND_PSHOT) < SKY_MAX_PSHOTS) {
        NGCharacter *missile = sky_spawn(SKY_KIND_PSHOT, SKY_HOSTILE_FIRST + 7u,
            s_player->x, s_player->y - 16, 0xFFu, NG_RENDER_BAND_FRONT);
        if (missile) {
            missile->data0 = (uint16_t)(6u + s_stage * 2u);
            ng_char_set_speed(missile, 0, -5);
            ng_char_set_body(missile, -5, -7, 10, 14);
            s_missiles--;
            playSFX(SOUND_SFX_7);
        }
    }

    if (s_invuln) s_invuln--;
    s_player->visible = 1u;
    if (s_hit_flash) {
        s_hit_flash--;
        sky_damage_flash(s_player, 1u);
    } else {
        sky_damage_flash(s_player, 0u);
        sky_damage_color(s_player, (uint8_t)(s_energy <= 34u));
    }
    if (s_energy <= 34u) {
        if (++smoke_tick >= 24u) {
            smoke_tick = 0u;
            sky_spawn_smoke(s_player->x, (int16_t)(s_player->y + 14));
        }
    } else smoke_tick = 0u;
}

/* ------------------------------------------------------------------ */
/*  Collisions                                                          */
/* ------------------------------------------------------------------ */
/*
 * Returns 1 when the last plane is lost.
 *
 * Two lists are gathered first rather than running a nested sweep over
 * every slot in use: the outer loop has to walk them all, and
 * 64x64 rectangle tests a frame is far more than a 12 MHz 68000 will give
 * up for collision.  Gathering first makes the inner loop bolts x targets,
 * which is at most six by seven.
 */
static void NEOGEO_USER sky_spawn_pickup(uint8_t type, int16_t x, int16_t y)
{
    NGCharacter *pickup;
    if (type >= SKY_PICK_COUNT || sky_count_kind(SKY_KIND_PICKUP) >= SKY_MAX_PICKUPS) return;
    pickup = sky_spawn(SKY_KIND_PICKUP, (uint8_t)(SKY_PICKUP_FIRST + type), x, y,
                       0xFFu, NG_RENDER_BAND_FX);
    if (!pickup) return;
    pickup->data0 = type;
    pickup->data1 = 0u;
    ng_char_set_speed_fp(pickup, 0, NG_FP_ONE / 2);
    ng_char_set_body(pickup, -10, -10, 20, 20);
}

static void NEOGEO_USER sky_drop_pickup(int16_t x, int16_t y)
{
    uint16_t kills = sky_stage_kills();
    if (kills && !(kills % 4u))
        sky_spawn_pickup((uint8_t)((kills / 4u - 1u) % SKY_PICK_COUNT), x, y);
}

static void NEOGEO_USER sky_special_draw(void)
{
    static const char *k_special_names[3] = {
        "F-16 PRECISION STRIKE",
        "F-35 STEALTH BARRAGE ",
        "F-15 HEAVY BOMB RUN "
    };
    NGCharacter *effects[6] = {0, 0, 0, 0, 0, 0};
    uint8_t i, age = (uint8_t)(SKY_SPECIAL_FRAMES - s_resources.special);
    for (NGCharacter *c = chars_at(0u), *end = c + ng_chars_slots_used(); c < end; c++) {
        if (!c || !c->active) continue;
        /* Clearing hostile shots also frees a bounded six-object effect
         * budget without stealing the player's sprite allocation. */
        if (s_resources.special && c->kind == SKY_KIND_ESHOT) ng_chars_remove(c);
        else if (c->kind == SKY_KIND_SUPER) {
            if (!s_resources.special || c->data0 >= 6u) ng_chars_remove(c);
            else effects[c->data0] = c;
        }
    }
    if (!s_resources.special) {
        if (s_special_banner) {
            sky_fix_blank(8u, 3u, 22u);
            s_special_banner = 0u;
        }
        return;
    }
    if (!s_special_banner) {
        sky_puts(8u, 3u, k_special_names[s_pick % 3u], SKY_PAL_SCORE);
        s_special_banner = 1u;
    }
    for (i = 0u; i < 6u; i++) {
        uint8_t frame = (uint8_t)((age / 5u + i) & 3u);
        uint8_t asset = (uint8_t)(SKY_SPECIAL_FIRST + s_pick * 4u + frame);
        int16_t x = (int16_t)(32 + (i % 3u) * 92);
        int16_t y = (int16_t)(56 + (i / 3u) * 96);
        int16_t phase = (int16_t)((age + i * 9u) % 40u);
        int16_t sway = phase < 20 ? phase - 10 : 30 - phase;
        if (s_pick == 0u) {
            x = (int16_t)(x + sway * 2);
            y = (int16_t)(40 + (age * 3u + (i / 3u) * 80u) % 144u);
        } else if (s_pick == 1u) {
            x = (int16_t)(x + sway * 2);
            y = (int16_t)(y + (i & 1u ? sway : -sway) * 2);
        } else {
            x = (int16_t)(x + sway * 2);
            y = (int16_t)(y + ((age / 6u + i) & 1u ? 14 : -14));
        }
        if (!effects[i]) {
            effects[i] = sky_spawn(SKY_KIND_SUPER, asset, x, y,
                                   SKY_SCALE_FULL, NG_RENDER_BAND_FX);
            if (!effects[i]) continue;
            effects[i]->data0 = i;
            effects[i]->data1 = frame;
        } else {
            ng_char_set_pos(effects[i], x, y);
            if (effects[i]->data1 != frame) {
                sky_bind(effects[i], asset, SKY_SCALE_FULL, NG_RENDER_BAND_FX);
                effects[i]->data1 = frame;
            }
        }
    }
}

static uint8_t NEOGEO_USER sky_collide(void)
{
    NGCharacter *bolts[SKY_MAX_PSHOTS + 2u];
    NGCharacter *marks[SKY_MAX_ENEMIES + 2u];
    uint8_t nbolts = 0u;
    uint8_t nmarks = 0u;
    uint8_t i, j;
    uint8_t dead = 0u;
    uint8_t pulse = sky_special_tick(&s_resources);

    sky_special_draw();

    for (NGCharacter *c = chars_at(0u), *end = c + ng_chars_slots_used(); c < end; c++) {
        if (!c->active) continue;

        if (c->kind == SKY_KIND_PSHOT) {
            if (nbolts < (uint8_t)(SKY_MAX_PSHOTS + 2u)) bolts[nbolts++] = c;
        } else if (c->kind == SKY_KIND_ENEMY || c->kind == SKY_KIND_BOSS || c->kind == SKY_KIND_MODULE) {
            if (nmarks < (uint8_t)(SKY_MAX_ENEMIES + 2u)) marks[nmarks++] = c;
        }
    }

    /* Six screen-wide pulses include surface targets. Boss damage remains
     * bounded independently of projectiles and the visual effect count. */
    if (pulse && s_player) {
        for (NGCharacter *c = chars_at(0u), *end = c + ng_chars_slots_used(); c < end; c++) {
            if (!c || !c->active) continue;
            if (c->x < 0 || c->x >= SKY_FIELD_RIGHT || c->y < SKY_FIELD_TOP ||
                c->y >= SKY_FIELD_BOTTOM) continue;
            if (c->kind == SKY_KIND_ESHOT) ng_chars_remove(c);
            else if (c->kind == SKY_KIND_ENEMY || c->kind == SKY_KIND_BOSS || c->kind == SKY_KIND_MODULE) {
                sky_stage_damage(c, sky_special_stage_damage(s_pick, s_stage, c->kind == SKY_KIND_BOSS));
                sky_spawn_blast(c->x, c->y);
                if (!c->hp) {
                    s_score = sky_score_add(s_score, sky_stage_defeated(c));
                    sky_drop_pickup(c->x, c->y);
                    if (!sky_stage_wreck(c)) ng_chars_remove(c);
                    s_combo_count++;
                    s_combo_timer = 45u;
                    if (s_combo_count == 4u) {
                        sky_radio_chatter("[PILOT] 4-HIT COMBO! ROLLING IN!", SOUND_VOICE_WORD_SCORE);
                    } else if (s_combo_count == 8u) {
                        sky_radio_chatter("[ATC] 8-HIT ACE! AIRSPACE SECURED!", SOUND_VOICE_WORD_GO);
                    }
                }
            }
        }
    }
    /* Independently targetable weapon modules precede the backing hull. */
    { uint8_t parts=0u;for(i=0;i<nmarks;i++) {
        NGCharacter *m=marks[i];
        if(m->kind==SKY_KIND_MODULE) { NGCharacter *tmp=marks[parts];marks[parts++]=marks[i];marks[i]=tmp; }
    } }
    /* Player bolts against enemies and the boss. */
    for (i = 0u; i < nbolts; i++) {
        NGCharacter *a = bolts[i];
        if (!a || !a->active || a->kind != SKY_KIND_PSHOT) continue;

        for (j = 0u; j < nmarks; j++) {
            NGCharacter *b = marks[j];
            if (!b || !b->active ||
                (b->kind != SKY_KIND_ENEMY && b->kind != SKY_KIND_BOSS && b->kind != SKY_KIND_MODULE)) continue;
            if (!ng_rect_hit(ng_char_body_rect(a), ng_char_body_rect(b))) continue;

            sky_stage_damage(b, a->data0 ? a->data0 : 1u);
            playSFX(SOUND_SFX_1);
            if (b->hp == 0u) {
                int16_t bx = b->x, by = b->y;
                uint8_t was_boss = (uint8_t)(b->kind == SKY_KIND_BOSS);
                uint8_t was_target = (uint8_t)(b->data2 >= 7u && b->data2 <= 8u);
                s_score = sky_score_add(s_score, sky_stage_defeated(b));
                if (!sky_stage_wreck(b)) ng_chars_remove(b);
                sky_spawn_blast(bx, by);
                if (was_boss) {
                    /* the stage queued its chain of blasts across the hull */
                    playSFX(SOUND_SFX_10);
                } else {
                    sky_drop_pickup(bx, by);
                    playSFX(SOUND_SFX_5);
                    if (was_target) {
                        sky_radio_chatter("[PILOT] AA COMMAND TARGET NEUTRALIZED!", SOUND_SFX_1);
                    }
                    s_combo_count++;
                    s_combo_timer = 45u;
                    if (s_combo_count == 4u) {
                        sky_radio_chatter("[PILOT] 4-HIT COMBO! ROLLING IN!", SOUND_VOICE_WORD_SCORE);
                    } else if (s_combo_count == 8u) {
                        sky_radio_chatter("[ATC] 8-HIT ACE! AIRSPACE SECURED!", SOUND_VOICE_WORD_GO);
                    }
                }
            } else {
                sky_spawn_blast(a->x, a->y);
            }
            ng_chars_remove(a);
            break;      /* one bolt, one target */
        }
    }

    /* Anything hostile against the player. */
    if (s_player && s_player->active && !s_invuln) {
        for (NGCharacter *a = chars_at(0u), *end = a + ng_chars_slots_used(); a < end; a++) {
            if (!a->active) continue;
            if (a->kind != SKY_KIND_ESHOT && a->kind != SKY_KIND_ENEMY &&
                a->kind != SKY_KIND_BOSS) continue;
            if (a->kind == SKY_KIND_ENEMY &&
                (a->data2 == 4u || a->data2 == 5u || a->data2 >= 7u)) continue;
            if (!ng_rect_hit(ng_char_body_rect(a), ng_char_body_rect(s_player))) continue;
            if (a->kind == SKY_KIND_ESHOT) ng_chars_remove(a);
            if (sky_player_hit()) { dead = 1u; }
            break;      /* one hit per frame - the i-frames cover the rest */
        }
    }

    if (s_player && s_player->active) for (NGCharacter *pickup = chars_at(0u), *end = pickup + ng_chars_slots_used(); pickup < end; pickup++) {
        if (!pickup || !pickup->active || pickup->kind != SKY_KIND_PICKUP) continue;
        if (pickup->x - s_player->x < 28 && s_player->x - pickup->x < 28 &&
            pickup->y - s_player->y < 28 && s_player->y - pickup->y < 28) {
            ng_char_set_pos(pickup,
                (int16_t)(pickup->x + (s_player->x > pickup->x) - (s_player->x < pickup->x)),
                (int16_t)(pickup->y + (s_player->y > pickup->y) - (s_player->y < pickup->y)));
        }
        if (!ng_rect_hit(ng_char_body_rect(pickup), ng_char_body_rect(s_player))) continue;
        sky_resources_collect(&s_resources, pickup->data0, &s_missiles, &s_lives, &s_energy);
        s_score = sky_score_add(s_score, pickup->data0 == SKY_PICK_STAR ?
                                 sky_star_score(&s_resources) : 250u);
        ng_chars_remove(pickup);
        playSFX(SOUND_SFX_1);
    }
    sky_score_extends(&s_resources, s_score, &s_lives);
    if (s_score > s_hiscore) s_hiscore = s_score;
    return dead;
}

/* ------------------------------------------------------------------ */
/*  Screens                                                             */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER sky_music(uint8_t track)
{
    soundStopAll();          waitVbl();
    soundCancelFade();       waitVbl();
    soundSetADPCMAVolume(0x30u); waitVbl();
    soundSetADPCMBVolume(0xE0u); waitVbl();
    soundSetSSGVolume(0x00u);    waitVbl();
    soundSetFMVolume(0x00u);     waitVbl();
    soundSetADPCMBLoop(1u);     waitVbl();
    playSFXB(track);
    waitVbl();
}


static void NEOGEO_USER sky_title_card(void)
{
    uint16_t t;

    sky_scene_begin();
    sky_title_draw();
    sky_puts(8u, 27u, "(C) 2026 EAGLE SOFTWARE", SKY_PAL_BODY);
    sky_puts(14u, 25u, "INSERT COIN", SKY_PAL_SCORE);

    for (t = 0u; t < 420u; t++) {
        sky_frame();
        if (sky_start_pending()) return;
    }
}

static void NEOGEO_USER sky_flight_demo(uint8_t attract)
{
    NGCharacter *planes[3];
    NGCharacter *traffic[3];
    uint16_t t;
    uint8_t i;
    static const uint8_t opponents[3] = {18u, 19u, 21u};

    sky_scene_begin();
    sky_bg_select(SKY_BG_MOUNTAIN);
    sky_puts(14u, 1u, "FLIGHT DISPLAY", SKY_PAL_TITLE);
    sky_puts(10u, 26u, attract ? "INSERT COIN TO PLAY" : "A / B  RETURN TO MENU", SKY_PAL_BODY);
    for (i = 0u; i < 3u; i++) {
        planes[i] = sky_spawn(SKY_KIND_PLAYER, k_pilot[i].plane,
                              (int16_t)(68 + i*92), 160,
                              SKY_SCALE_PLAYER, NG_RENDER_BAND_PLAYER);
        traffic[i] = sky_spawn(SKY_KIND_PLAYER, opponents[i],
                               (int16_t)(70 + i*90), 40,
                               SKY_SCALE_ROSTER, NG_RENDER_BAND_NPC);
    }
    for (t = 0u; t < 600u; t++) {
        int16_t phase = (int16_t)(t % 120u);
        int16_t drift = phase < 60 ? phase-30 : 90-phase;
        int8_t bank = phase < 60 ? 2 : -2;
        sky_frame();
        sky_bg_advance(1u);
        sky_bg_draw();
        for (i = 0u; i < 3u; i++) {
            if (planes[i]) {
                if ((t % 60u) == 0u)
                    sky_bind(planes[i], k_bank_pose[i][bank+3],
                             SKY_SCALE_PLAYER, NG_RENDER_BAND_PLAYER);
                ng_char_set_pos(planes[i], (int16_t)(68+i*92+drift/2),
                                (int16_t)(150+(i == 1u ? -18 : 0)));
            }
            if (traffic[i])
                ng_char_set_pos(traffic[i], (int16_t)(70+i*90-drift/2),
                                (int16_t)(40+(t+i*40u)%150u));
        }
        ng_chars_update();
        ng_chars_draw();
        if ((attract && sky_start_pending()) ||
            (!attract && (sky_joy_pressed() & (BUTTON_A | BUTTON_B)))) return;
    }
}

static void NEOGEO_USER sky_how_to_play(uint8_t attract)
{
    uint16_t t;
    uint8_t i;
    static const int16_t k_icon_x[9] = {24, 24, 24, 24, 24, 168, 168, 168, 168};
    static const int16_t k_icon_y[9] = {68, 92, 116, 140, 164, 68, 92, 116, 140};
    static const uint8_t k_icon_types[9] = {
        SKY_PICK_POWER, SKY_PICK_MISSILE, SKY_PICK_SHIELD, SKY_PICK_FUEL, SKY_PICK_TURBO,
        SKY_PICK_CORE, SKY_PICK_REPAIR, SKY_PICK_LIFE, SKY_PICK_STAR
    };

    sky_scene_begin();
    sky_bg_select(SKY_BG_COAST);

    sky_puts(11u, 1u, "TACTICAL FLIGHT MANUAL", SKY_PAL_TITLE);
    sky_puts(3u, 3u, "A FIRE  B BOOST  C BOMB  D MISSILE", SKY_PAL_SCORE);
    sky_puts(5u, 4u, "HOLD STICK: BANK TO 45 DEGREES", SKY_PAL_BODY);
    sky_puts(4u, 5u, "--------------------------------", SKY_PAL_WARN);
    sky_puts(11u, 6u, "- SUPPLY DROP INTEL -", SKY_PAL_WARN);

    sky_puts(5u,  8u, "POWER  : LV+1",  SKY_PAL_BODY);
    sky_puts(5u, 11u, "MISSILE: AMMO",  SKY_PAL_BODY);
    sky_puts(5u, 14u, "SHIELD : SHIELD",SKY_PAL_BODY);
    sky_puts(5u, 17u, "FUEL   : +30 SEC",SKY_PAL_BODY);
    sky_puts(5u, 20u, "TURBO  : SPEED", SKY_PAL_BODY);

    sky_puts(23u,  8u, "CORE  : BOMB",   SKY_PAL_BODY);
    sky_puts(23u, 11u, "REPAIR: ARMOR",  SKY_PAL_BODY);
    sky_puts(23u, 14u, "1-UP  : EXTRA",  SKY_PAL_BODY);
    sky_puts(23u, 17u, "STAR  : COMBO",  SKY_PAL_BODY);

    sky_puts(4u, 22u, "B+D CHAFF  AVOID RADAR CIRCLES", SKY_PAL_SCORE);
    sky_puts(4u, 23u, "3 TARGETS: +2500  FUEL=TIME", SKY_PAL_SCORE);

    for (i = 0u; i < 9u; i++) {
        sky_spawn(SKY_KIND_FACE,
                  (uint8_t)(SKY_PICKUP_FIRST + k_icon_types[i]),
                  k_icon_x[i], k_icon_y[i],
                  0xFFu, NG_RENDER_BAND_NPC);
    }

    sky_puts(14u, 25u, attract ? "INSERT COIN" : "PRESS BUTTON", SKY_PAL_SCORE);

    for (t = 0u; t < 480u; t++) {
        sky_frame();
        sky_bg_advance(1u);
        sky_bg_draw();
        ng_chars_update();
        ng_chars_draw();
        if (attract && sky_start_pending()) return;
        if (!attract && (sky_joy_pressed() & (BUTTON_A | BUTTON_B | BUTTON_C | BUTTON_D))) break;
    }
}

#if defined(NG_AES)
static void NEOGEO_USER sky_main_menu(void)
{
    uint8_t selection = 0u;
    static const char *entries[3] = {"START", "DISPLAY", "HOW TO PLAY"};
    static const uint8_t columns[3] = {3u, 15u, 26u};
    for (;;) {
        uint16_t t;
        sky_scene_begin();
        sky_title_draw();
        sky_puts(8u, 27u, "(C) 2026 EAGLE SOFTWARE", SKY_PAL_BODY);
        for (t = 0u; t < 1800u; t++) {
            uint16_t pressed;
            uint8_t i;
            sky_frame();
            pressed = sky_joy_pressed();
            if (pressed & (JOY_UP | JOY_LEFT)) selection = (uint8_t)((selection+2u)%3u);
            if (pressed & (JOY_DOWN | JOY_RIGHT)) selection = (uint8_t)((selection+1u)%3u);
            for (i = 0u; i < 3u; i++)
                sky_puts(columns[i], 25u, entries[i],
                         i == selection ? SKY_PAL_SCORE : SKY_PAL_BODY);
            sky_puts(8u, 26u, "LEFT / RIGHT   A CONFIRM", SKY_PAL_BODY);
            ng_chars_update();
            ng_chars_draw();
            if (pressed & BUTTON_A) break;
        }
        if (t >= 1800u || selection == 1u) {
            sky_flight_demo(0u);
        } else if (selection == 0u) {
            return;
        } else {
            sky_how_to_play(0u);
        }
    }
}
#endif

/*
 * Pilot select.  All three planes and their pilot portraits stay on
 * screen at once; the highlight is the FIX caption plus the fact that
 * only the selected plane is drawn at full size.
 */
static uint8_t NEOGEO_USER sky_select(void)
{
    NGCharacter *face[SKY_PILOTS];
    NGCharacter *plane[SKY_PILOTS];
    uint8_t i;
    uint8_t last = 0xFFu;

    sky_scene_begin();
    sky_bg_select(SKY_BG_COAST);
    sky_puts(13u, 1u, "SELECT PILOT", SKY_PAL_TITLE);
    sky_puts( 7u, 25u, "LEFT / RIGHT    A CONFIRM", SKY_PAL_BODY);

    for (i = 0u; i < SKY_PILOTS; i++) {
        int16_t cx = (int16_t)(68 + i * 92);
        face[i]  = sky_spawn(SKY_KIND_FACE,   k_pilot[i].face,  cx,  72,
                             SKY_SCALE_PORTRAIT, NG_RENDER_BAND_NPC);
        plane[i] = sky_spawn(SKY_KIND_PLAYER, k_pilot[i].plane, cx, 144,
                             SKY_SCALE_ROSTER, NG_RENDER_BAND_PLAYER);
    }

    for (;;) {
        uint16_t pressed;

        sky_frame();
        sky_bg_advance(1u);
        sky_bg_draw();

        pressed = sky_joy_pressed();
        if (pressed & JOY_LEFT)  s_pick = (uint8_t)((s_pick + SKY_PILOTS - 1u) % SKY_PILOTS);
        if (pressed & JOY_RIGHT) s_pick = (uint8_t)((s_pick + 1u) % SKY_PILOTS);

        if (s_pick != last) {
            last = s_pick;
            playSFX(SOUND_SFX_1);
            for (i = 0u; i < SKY_PILOTS; i++) {
                uint8_t sel = (uint8_t)(i == s_pick);
                if (plane[i]) {
                    sky_bind(plane[i], k_pilot[i].plane,
                             sel ? SKY_SCALE_PLAYER : SKY_SCALE_ROSTER, NG_RENDER_BAND_PLAYER);
                }
                if (face[i]) {
                    face[i]->visible = 1u;
                }
            }
            sky_fix_blank(4u, 21u, 32u);
            sky_fix_blank(4u, 22u, 32u);
            sky_fix_blank(4u, 23u, 32u);
            sky_puts(12u, 21u, k_pilot[s_pick].call, SKY_PAL_SCORE);
            sky_puts(12u, 22u, k_pilot[s_pick].name, SKY_PAL_BODY);
            {
                static const char *k_wdesc[3] = {
                    "WEAPON: PULSE LASER (BALANCED)",
                    "WEAPON: TWIN SPREAD (WIDE FLAK)",
                    "WEAPON: CRIMSON VULCAN (RAPID)"
                };
                sky_puts(5u, 23u, k_wdesc[s_pick % 3u], SKY_PAL_TITLE);
            }
        }

        ng_chars_update();
        ng_chars_draw();

        if (pressed & (BUTTON_A | BUTTON_B)) {
            playSFX(SOUND_SFX_3);
            return 1u;
        }
    }
}

static void NEOGEO_USER sky_mission_briefing(uint8_t stage)
{
    NGCharacter *portrait;
    uint16_t t;
    const SkyPilot *p = &k_pilot[s_pick % SKY_PILOTS];
    static const char *loadouts[3] = {
        "PULSE LASER [BALANCED]",
        "TWIN PHALANX [WIDE FLAK]",
        "CRIMSON VULCAN [RAPID]"
    };

    sky_scene_begin();
    sky_bg_select(sky_stage_bg(stage));

    sky_puts(5u,  2u, "== MISSION TACTICAL BRIEFING ==", SKY_PAL_TITLE);
    sky_puts(5u,  4u, sky_stage_subtitle(stage),              SKY_PAL_SCORE);

    portrait = sky_spawn(SKY_KIND_FACE, p->face, 58, 110,
                         SKY_SCALE_PORTRAIT, NG_RENDER_BAND_NPC);

    sky_puts(13u,  6u, "PILOT :", SKY_PAL_SCORE);
    sky_puts(21u,  6u, p->name,   SKY_PAL_BODY);
    sky_puts(13u,  8u, "LOAD  :", SKY_PAL_SCORE);
    sky_text_box(13u, 9u, 26u, 2u, loadouts[s_pick % 3u], SKY_PAL_TITLE);

    sky_puts(13u, 12u, "[OBJECTIVE]", SKY_PAL_SCORE);
    sky_text_box(13u, 13u, 26u, 2u, sky_stage_objective(stage), SKY_PAL_BODY);

    sky_puts(13u, 16u, "[ENEMY INTEL]", SKY_PAL_SCORE);
    sky_text_box(13u, 17u, 26u, 2u, sky_stage_intel(stage), SKY_PAL_BODY);

    sky_puts(3u, 20u, "B+LEFT/RIGHT: ROLL  B+UP: CLIMB", SKY_PAL_SCORE);
    sky_puts(3u, 21u, "B+DOWN: SPLIT-S  C: SUPER BOMB", SKY_PAL_BODY);

    sky_puts(3u, 22u, "B+D: CHAFF  FUEL: MISSION TIME", SKY_PAL_WARN);
    sky_puts(3u, 23u, "AVOID RADAR / DEFEAT DEFENDERS", SKY_PAL_BODY);
    sky_puts(11u, 25u, "PRESS A TO SCRAMBLE", SKY_PAL_SCORE);

    playSFX(SOUND_SFX_12);
    playSFX(SOUND_VOICE_WORD_READY);

    for (t = 0u; t < 360u; t++) {
        sky_frame();
        sky_bg_advance(1u);
        sky_bg_draw();
        ng_chars_draw();
        if (t >= 60u && (sky_joy_pressed() & (BUTTON_A | BUTTON_B | BUTTON_C | BUTTON_D))) break;
    }
    if (portrait) ng_chars_remove(portrait);
    for (t = 1u; t <= 26u; t++) sky_fix_blank(0u, (uint8_t)t, 40u);
}

/* One stage.  0 = squad down, 1 = stage cleared. */
static uint8_t NEOGEO_USER sky_fly_stage(uint8_t stage)
{
    uint8_t cleared = 0u;

    sky_mission_briefing(stage);

    sky_scene_begin();
    sky_bg_select(sky_stage_bg(stage));
    sky_stage_begin(stage);
    sky_hud_static();

    s_player = sky_spawn(SKY_KIND_PLAYER, k_pilot[s_pick % SKY_PILOTS].plane,
                         SKY_FIELD_X + SKY_FIELD_W / 2, SKY_FIELD_BOTTOM - 46,
                         SKY_SCALE_PLAYER, NG_RENDER_BAND_PLAYER);
    if (!s_player) return 0u;
    s_hud_face = sky_spawn(SKY_KIND_FACE, k_pilot[s_pick % SKY_PILOTS].face,
                           288, 44, NG_SCALE(5), NG_RENDER_BAND_PLAYER);
    if (s_player) {
        /* The plane's hitbox is deliberately tiny - a shmup is read from
         * the nose of the aircraft, not from its wingspan. */
        ng_char_set_body(s_player, -6, -6, 12, 12);
    }
    s_invuln  = 120u;
    s_fire_cd = 0u;
    s_plane_pose = k_pilot[s_pick % SKY_PILOTS].plane;
    s_bank = 0;
    s_roll.angle = s_roll.held = s_roll.direction = 0;
    s_sortie_frames = 0u;
    s_cruise_called = 0u;
    s_flight_vx = s_flight_vy = 0;
    s_roll_maneuver = s_loop_maneuver = s_maneuver_cooldown = 0u;
    s_prev_joy = 0u;
    s_resources.boosting = s_resources.special = s_resources.special_pulses = 0u;
    s_resources.fuel = SKY_FUEL_MAX;
    s_resources.idle_recovery = 0u;
    if (s_resources.cores < 2u) s_resources.cores = 2u;

    s_combo_count = s_combo_timer = 0u;
    s_boss_critical_alert = s_boss_warned = 0u;
    s_special_banner = 0u;

    playSFX(SOUND_SFX_3);
    playSFX(SOUND_VOICE_WORD_START);
    sky_radio_chatter("[ATC] VIPER-1 SCRAMBLED! WEAPONS FREE!", SOUND_SFX_12);

    for (;;) {
        sky_frame();

        if (s_combo_timer) {
            if (--s_combo_timer == 0u) {
                s_combo_count = 0u;
            }
        }

        sky_player_step();
        if (!s_resources.fuel) {
            sky_radio_chatter("FUEL EXHAUSTED: MISSION FAILED", SOUND_SFX_12);
            sky_hud_draw();
            sky_puts(5u, 12u, "FUEL OUT - MISSION FAILED", SKY_PAL_WARN);
            { uint16_t t; for (t=0u; t<120u; t++) sky_frame(); }
            return 0u;
        }
        if (++s_sortie_frames == 180u || (s_sortie_frames % 600u) == 0u)
            sky_spawn_pickup((uint8_t)((s_sortie_frames / 600u + stage) % SKY_PICK_COUNT),
                            160, 64);
        if (s_invuln == 149u && s_player)
            sky_spawn_pickup(SKY_PICK_POWER, s_player->x, s_player->y - 20);
        if (s_player) sky_bg_follow(s_player->x, s_player->y);
        if (sky_stage_tick(s_player ? s_player->x : 0,
                           s_player ? s_player->y : 0)) {
            cleared = 1u;
        }
        if (sky_stage_cruising() && !s_cruise_called) {
            s_cruise_called = 1u;
            sky_radio_chatter("[ATC] PROCEED TO TARGET AREA!", SOUND_SFX_12);
        }
        sky_bg_advance(sky_stage_paused() ? 0u : sky_stage_cruising() ? 4u :
                       stage==0u?2u:1u);
        sky_bg_draw();

        ng_chars_update();
        sky_stage_parts_update();
        if (sky_collide()) {
            sky_hud_cockpit_remove();
            return 0u;
        }
        sky_depth_draw();
        ng_chars_draw();

        sky_hud_draw();
        if (sky_stage_boss_active()) {
            if (!s_boss_warned) {
                s_boss_warned = 1u;
                sky_radio_chatter("[ATC] WARNING! HEAVY BOSS DETECTED!", SOUND_SFX_12);
            }
            sky_hud_boss();
        } else if (sky_stage_encounter() == 2u) {
            sky_hud_boss();
        } else if (s_hud_boss != 0u) {
            sky_fix_blank(8u, 4u, 24u);
            s_hud_boss = 0u;
        }

        if (cleared) {
            s_boss_critical_alert = 0u;
            s_boss_warned = 0u;
            if (sky_stage_targets() == 3u) s_score = sky_score_add(s_score, 2500u);
            sky_fix_blank(8u, 4u, 24u);          /* the boss bar */
            sky_radio_chatter("[ATC] AIRSPACE SECURED! STAGE CLEAR!", SOUND_VOICE_WORD_STAGE);
            sky_puts(14u, 12u, "STAGE CLEAR", SKY_PAL_SCORE);
            playSFX(SOUND_SFX_11);
            {
                uint16_t t;
                for (NGCharacter *c = chars_at(0u), *end = c + ng_chars_slots_used(); c < end; c++) {
                    if (c && c->active && c->kind != SKY_KIND_PLAYER && c->kind != SKY_KIND_FACE &&
                        c->kind != SKY_KIND_BLAST && c->kind != SKY_KIND_WRECK) ng_chars_remove(c);
                }
                for (t = 0u; t < 150u; t++) {
                    sky_frame();
                    sky_bg_advance(1u);
                    sky_bg_draw();
                    sky_stage_effects_tick();
                    if (s_player && t >= 36u) {
                        if (t == 36u) {
                            sky_bind(s_player, k_pilot[s_pick].plane,
                                     SKY_SCALE_PLAYER, NG_RENDER_BAND_PLAYER);
                            playSFX(SOUND_SFX_7);
                        }
                        ng_char_set_pos(s_player,
                            (int16_t)(s_player->x + (s_player->x < 160) - (s_player->x > 160)),
                            (int16_t)(s_player->y - (t < 72u ? 1 : (t < 110u ? 3 : 5))));
                        if ((t & 3u) == 0u) {
                            sky_spawn_smoke((int16_t)(s_player->x - 6), (int16_t)(s_player->y + 20));
                            sky_spawn_smoke((int16_t)(s_player->x + 6), (int16_t)(s_player->y + 20));
                        }
                    }
                    ng_chars_draw();
                }
            }
            sky_hud_cockpit_remove();
            return 1u;
        }
    }
}

static void NEOGEO_USER sky_victory_credits(void)
{
    uint16_t t;

    sky_scene_begin();
    sky_bg_select(SKY_BG_COAST);

    sky_puts(12u,  2u, "MISSION COMPLETE",          SKY_PAL_TITLE);
    sky_puts( 7u,  4u, "ALL SEVEN SECTORS SECURED", SKY_PAL_WARN);

    sky_puts( 6u,  7u, "PILOT",                     SKY_PAL_SCORE);
    sky_puts(13u,  7u, k_pilot[s_pick % SKY_PILOTS].name, SKY_PAL_BODY);
    sky_puts(20u,  7u, k_pilot[s_pick % SKY_PILOTS].call, SKY_PAL_SCORE);

    sky_puts( 6u,  9u, "SCORE",                     SKY_PAL_SCORE);
    sky_infix_number(13u, 9u, s_score, 6u);

    sky_puts(14u, 12u, "- CREDITS -",               SKY_PAL_TITLE);
    sky_puts( 6u, 14u, "DESIGN   EAGLE SOFTWARE 2026", SKY_PAL_BODY);
    sky_puts( 6u, 16u, "ENGINE   NEO GEO SDK 2D",      SKY_PAL_BODY);
    sky_puts( 6u, 18u, "ART      NATIVE TILE ROMS",    SKY_PAL_BODY);
    sky_puts( 6u, 20u, "SOUND    YM2610 FM SSG ADPCM", SKY_PAL_BODY);
    sky_puts( 9u, 23u, "THANK YOU FOR PLAYING",        SKY_PAL_SCORE);

    playSFX(SOUND_SFX_3);

    /* Ten seconds to read it; a button skips ahead once the first
     * few have passed, so a held trigger from the last fight cannot
     * blow straight through it. */
    for (t = 0u; t < 600u; t++) {
        sky_frame();
        sky_bg_advance(1u);
        sky_bg_draw();
        if (t > 240u && (sky_joy_pressed() & (BUTTON_A | BUTTON_B | BUTTON_C | BUTTON_D))) {
            break;
        }
    }
}

static void NEOGEO_USER sky_happy_win_moves(void)
{
    NGCharacter *plane;
    uint16_t t;
    const int16_t cx = SKY_FIELD_X + SKY_FIELD_W / 2;

    sky_scene_begin();
    sky_bg_select(SKY_BG_MOUNTAIN);
    sky_puts(11u, 2u, "ACE VICTORY FLIGHT", SKY_PAL_TITLE);
    plane = sky_spawn(SKY_KIND_PLAYER, k_pilot[s_pick % SKY_PILOTS].plane,
                      cx, 176, SKY_SCALE_PLAYER, NG_RENDER_BAND_PLAYER);
    if (!plane) return;
    playSFX(SOUND_SFX_7);

    for (t = 0u; t < 360u; t++) {
        sky_frame();
        sky_bg_advance(1u);
        sky_bg_draw();
        if (t < 90u) {
            ng_char_set_pos(plane, cx, (int16_t)(176 - t * 40u / 90u));
        } else if (t < 270u) {
            uint8_t a = (uint8_t)((t - 90u) * 256u / 180u);
            ng_char_set_pos(plane,
                (int16_t)(cx + ((int32_t)ng_sin_tab[a] * 52 / 32768)),
                (int16_t)(96 + ((int32_t)ng_cos_tab[a] * 40 / 32768)));
            if ((t % 30u) == 0u) sky_spawn_blast(plane->x, plane->y + 16);
        } else {
            ng_char_set_pos(plane, cx, (int16_t)(136 - (t - 270u) * 2u));
        }
        sky_stage_effects_tick();
        ng_chars_draw();
    }
}

static void NEOGEO_USER sky_game_over(void)
{
    sky_scene_begin();
    sky_bg_select(SKY_BG_COAST);
    sky_puts(15u, 10u, "SQUAD DOWN", SKY_PAL_WARN);
    sky_infix(13u, 13u, SKY_INFIX_SCORE);
    sky_infix_number(19u, 13u, s_score, 6u);
    sky_puts(11u, 18u, "THANKS FOR FLYING", SKY_PAL_BODY);
    playSFX(SOUND_VOICE_WORD_GAME_OVER);

    {
        uint16_t t;
        for (t = 0u; t < 360u; t++) {
            sky_frame();
            sky_bg_advance(1u);
            sky_bg_draw();
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Entry points                                                        */
/* ------------------------------------------------------------------ */
void NEOGEO_USER sky_run(void)
{
    s_pick   = 0u;
    s_lives  = SKY_START_LIVES;
    s_energy = SKY_ENERGY_MAX;
    s_score  = 0u;
    s_stage  = 0u;
    s_missiles = 3u;
    sky_resources_init(&s_resources);
    if (s_resources.cores < 1u) s_resources.cores = 1u;

    soundSceneReset();
    sky_music(SOUND_TRACK_A);
#ifdef NG_AES
    sky_main_menu();
#endif
    (void)sky_select();

    for (;;) {
        static const uint8_t k_stage_track[SKY_STAGE_COUNT] = {
            SOUND_TRACK_B, SOUND_TRACK_C, SOUND_TRACK_D, SOUND_TRACK_F,
            SOUND_TRACK_G, SOUND_TRACK_H, SOUND_TRACK_B
        };
        sky_music(k_stage_track[s_stage % SKY_STAGE_COUNT]);
        if (!sky_fly_stage(s_stage)) {
            sky_music(SOUND_TRACK_I);
            sky_game_over();
            soundStopAll();
            return;
        }

        /* The standalone campaign ends only after its seventh boss. */
        if (s_stage >= (SKY_STAGE_COUNT - 1u)) {
            sky_music(SOUND_TRACK_A);
            sky_victory_credits();
            sky_happy_win_moves();
            soundStopAll();
            return;
        }

        s_stage++;
        s_energy = SKY_ENERGY_MAX;
    }
}

/*
 * Attract loop.  Title card, then the pilot roster, on repeat until the
 * BIOS reports a credit - the MVS dispatcher polls NGO_START_FLAG, so
 * bailing out early here is what makes the coin feel instant.
 */
void NEOGEO_USER sky_run_attract(void)
{
    uint8_t i;

    s_pick = 0u;
    sky_music(SOUND_TRACK_A);

    for (;;) {
        sky_title_card();
        if (sky_start_pending()) return;

        sky_scene_begin();
        sky_bg_select(SKY_BG_COAST);
        sky_puts(12u, 1u, "SKY LANCE  SQUADRON", SKY_PAL_TITLE);

        for (i = 0u; i < SKY_PILOTS; i++) {
            int16_t cx = (int16_t)(68 + i * 92);
            sky_spawn(SKY_KIND_FACE,   k_pilot[i].face,  cx,  72,
                      SKY_SCALE_PORTRAIT, NG_RENDER_BAND_NPC);
            sky_spawn(SKY_KIND_PLAYER, k_pilot[i].plane, cx, 144,
                      SKY_SCALE_ROSTER, NG_RENDER_BAND_PLAYER);
            sky_puts((uint8_t)(4u + i * 10u), 24u, k_pilot[i].name, SKY_PAL_BODY);
        }

        {
            uint16_t t;
            for (t = 0u; t < 300u; t++) {
                sky_frame();
                sky_bg_advance(1u);
                sky_bg_draw();
                ng_chars_update();
                ng_chars_draw();
                if (sky_start_pending()) return;
            }
        }
        sky_how_to_play(1u);
        if (sky_start_pending()) return;

        sky_flight_demo(1u);
        if (sky_start_pending()) return;
    }
}
