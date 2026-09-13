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
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
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
void NEOGEO_USER soundSetSSGVolume(uint8_t v);
void NEOGEO_USER soundSetFMVolume(uint8_t v);
int  NEOGEO_USER read_p1credit(void);

#define NGO_START_FLAG  0xD00100

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
static uint8_t s_speed_bonus;
static uint32_t s_hud_score, s_hud_high;
static uint8_t s_hud_lives, s_hud_energy, s_hud_missiles, s_hud_boss;

#define SKY_ENERGY_MAX   100u
#define SKY_ENERGY_HIT    34u
#define SKY_START_LIVES    3u
#define SKY_LIVES_MAX      4u

/* ------------------------------------------------------------------ */
/*  HUD                                                                 */
/* ------------------------------------------------------------------ */
/*
 * Row ownership on the 40x28 FIX grid:
 *   0..1    SCORE / HI counters (16x16 infix art, so two rows each)
 *   2       boss health bar, only while a boss is alive
 *   26..27  lives pips and energy cells
 * Everything between belongs to the playfield.
 */
static void NEOGEO_USER sky_hud_static(void)
{
    char digit[2];
    sky_infix(1u,  0u, SKY_INFIX_SCORE);
    sky_infix(24u, 0u, SKY_INFIX_HI);
    sky_infix(1u, 26u, (uint8_t)(SKY_INFIX_1P + (s_pick % 3u)));
    /* The route line: which of the seven this is, and the missile rack.
     * The digits are written on their own so no string is built here. */
    sky_puts(8u, 24u, "STAGE  /7  D MISSILE", SKY_PAL_BODY);
    digit[1] = '\0';
    digit[0] = (char)('1' + s_stage);
    sky_puts(14u, 24u, digit, SKY_PAL_BODY);
    s_hud_score = s_hud_high = 0xFFFFFFFFu;
    s_hud_lives = s_hud_energy = s_hud_missiles = s_hud_boss = 0xFFu;
}

static void NEOGEO_USER sky_hud_draw(void)
{
    uint8_t i;
    uint8_t cells = (uint8_t)(s_energy / 10u);

    if (s_hud_score != s_score) {
        sky_infix_number(7u, 0u, s_score, 6u);
        s_hud_score = s_score;
    }
    if (s_hud_high != s_hiscore) {
        sky_infix_number(27u, 0u, s_hiscore, 6u);
        s_hud_high = s_hiscore;
    }
    if (s_hud_missiles != s_missiles) {
        char ammo[2];
        ammo[0] = (char)('0' + s_missiles); ammo[1] = '\0';
        sky_puts(28u, 24u, ammo, SKY_PAL_SCORE);
        s_hud_missiles = s_missiles;
    }

    /* Lives pips: one per spare plane, blanked back out as they go. */
    if (s_hud_lives != s_lives) for (i = 0u; i < SKY_LIVES_MAX; i++) {
        if (i < s_lives) sky_infix((uint8_t)(4u + i * 2u), 26u, SKY_INFIX_LIFE);
        else {
            sky_fix_blank((uint8_t)(4u + i * 2u), 26u, 2u);
            sky_fix_blank((uint8_t)(4u + i * 2u), 27u, 2u);
        }
    }
    s_hud_lives = s_lives;

    if (cells > 10u) cells = 10u;
    if (s_hud_energy != cells) for (i = 0u; i < 10u; i++) {
        sky_infix((uint8_t)(14u + i * 2u), 26u,
                  (uint8_t)(i < cells ? SKY_INFIX_ENERGY : SKY_INFIX_ENERGY_MT));
    }
    s_hud_energy = cells;
}

static void NEOGEO_USER sky_hud_boss(void)
{
    char bar[26];
    uint8_t n = sky_stage_boss_bar();
    uint8_t i;

    if (s_hud_boss == n) return;
    s_hud_boss = n;

    for (i = 0u; i < 24u; i++) bar[i] = (char)(i < n ? '#' : '.');
    bar[24] = '\0';
    sky_puts(8u, 2u, bar, SKY_PAL_WARN);
}

/* ------------------------------------------------------------------ */
/*  Player                                                              */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER sky_fire(void)
{
    const SkyPilot *p = &k_pilot[s_pick % SKY_PILOTS];
    int16_t y;

    if (!s_player || s_fire_cd) return;
    if (sky_count_kind(SKY_KIND_PSHOT) + (p->twin ? 2u : 1u) > SKY_MAX_PSHOTS) return;

    y = (int16_t)(s_player->y - 16);
    if (p->twin) {
        NGCharacter *a = sky_spawn(SKY_KIND_PSHOT, SKY_SHOT_PLAYER,
                                   (int16_t)(s_player->x - 10), y,
                                   SKY_SCALE_FULL, NG_RENDER_BAND_FX);
        NGCharacter *b = sky_spawn(SKY_KIND_PSHOT, SKY_SHOT_PLAYER,
                                   (int16_t)(s_player->x + 10), y,
                                   SKY_SCALE_FULL, NG_RENDER_BAND_FX);
        if (a) { ng_char_set_speed(a, 0, -7); ng_char_set_body(a, -3, -10, 6, 20); }
        if (b) { ng_char_set_speed(b, 0, -7); ng_char_set_body(b, -3, -10, 6, 20); }
    } else {
        NGCharacter *a = sky_spawn(SKY_KIND_PSHOT, SKY_SHOT_PLAYER,
                                   s_player->x, y, SKY_SCALE_FULL, NG_RENDER_BAND_FX);
        if (a) { ng_char_set_speed(a, 0, -8); ng_char_set_body(a, -3, -10, 6, 20); }
    }

    s_fire_cd = p->fire_gap;
    playSFX(SOUND_SFX_4);
}

static uint8_t NEOGEO_USER sky_player_hit(void)
{
    if (s_invuln) return 0u;

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
    s_invuln = 150u;
    if (s_player) {
        ng_char_set_pos(s_player, SKY_FIELD_X + SKY_FIELD_W / 2, SKY_FIELD_BOTTOM - 24);
    }
    return 0u;
}

static void NEOGEO_USER sky_player_step(void)
{
    const SkyPilot *p = &k_pilot[s_pick % SKY_PILOTS];
    uint16_t joy = sky_joy();
    int16_t x, y, dx, dy, speed;

    if (!s_player || !s_player->active) return;

    x = s_player->x;
    y = s_player->y;
    speed = (int16_t)(p->speed - 1u + s_speed_bonus);
    dx = (joy & JOY_RIGHT ? 1 : 0) - (joy & JOY_LEFT ? 1 : 0);
    dy = (joy & JOY_DOWN ? 1 : 0) - (joy & JOY_UP ? 1 : 0);
    if (dx && dy) speed = (int16_t)(speed * 3 / 4);
    x = (int16_t)(x + dx * speed);
    y = (int16_t)(y + dy * speed);

    ng_char_set_pos(s_player,
                    (int16_t)NG_CLAMP(x, SKY_FIELD_LEFT, SKY_FIELD_RIGHT),
                    /* The ceiling keeps the plane under the boss station,
                     * so a boss is always fought from below. */
                    (int16_t)NG_CLAMP(y, SKY_FIELD_TOP + 40, SKY_FIELD_BOTTOM - 28));

    /* Auto-fire on hold, like the arcade board: A is a trigger, not a
     * typing test. */
    if (joy & (BUTTON_A | BUTTON_B)) sky_fire();
    if (s_fire_cd) s_fire_cd--;
    if ((sky_joy_pressed() & BUTTON_D) && s_missiles &&
        sky_count_kind(SKY_KIND_PSHOT) < SKY_MAX_PSHOTS) {
        NGCharacter *missile = sky_spawn(SKY_KIND_PSHOT, SKY_ITEM_MISSILE,
            s_player->x, s_player->y - 16, 0xFFu, NG_RENDER_BAND_FX);
        if (missile) {
            missile->data0 = 1u;
            ng_char_set_speed(missile, 0, -5);
            ng_char_set_body(missile, -5, -7, 10, 14);
            s_missiles--;
            playSFX(SOUND_SFX_7);
        }
    }

    if (s_invuln) {
        s_invuln--;
        /* Blink at 4 Hz so it reads as invulnerability, not as a dropped
         * sprite. */
        s_player->visible = (uint8_t)((s_invuln & 8u) ? 0u : 1u);
    } else {
        s_player->visible = 1u;
    }
}

/* ------------------------------------------------------------------ */
/*  Collisions                                                          */
/* ------------------------------------------------------------------ */
/*
 * Returns 1 when the last plane is lost.
 *
 * Two index lists are gathered first rather than running a nested sweep
 * over all NG_MAX_CHARS slots: the outer loop has to walk the whole array
 * (ng_chars_count() counts active slots, not the highest one in use), and
 * 64x64 rectangle tests a frame is far more than a 12 MHz 68000 will give
 * up for collision.  Gathering first makes the inner loop bolts x targets,
 * which is at most six by seven.
 */
static void NEOGEO_USER sky_drop_pickup(int16_t x, int16_t y)
{
    static const uint8_t assets[3] = { SKY_ITEM_SPEED, SKY_ITEM_MISSILE, SKY_ITEM_LIFE };
    uint16_t kills = sky_stage_kills();
    uint8_t type;
    NGCharacter *pickup;
    if (!kills || (kills % 4u) || sky_count_kind(SKY_KIND_PICKUP) >= SKY_MAX_PICKUPS) return;
    type = (uint8_t)((kills / 4u - 1u) % 3u);
    pickup = sky_spawn(SKY_KIND_PICKUP, assets[type], x, y, 0xFFu, NG_RENDER_BAND_FX);
    if (!pickup) return;
    pickup->data0 = type;
    pickup->data1 = 0u;
    ng_char_set_speed_fp(pickup, 0, NG_FP_ONE / 2);
    ng_char_set_body(pickup, -10, -10, 20, 20);
}

static uint8_t NEOGEO_USER sky_collide(void)
{
    uint8_t bolts[SKY_MAX_PSHOTS + 2u];
    uint8_t marks[SKY_MAX_ENEMIES + 2u];
    uint8_t nbolts = 0u;
    uint8_t nmarks = 0u;
    uint8_t i, j;
    uint8_t dead = 0u;

    for (i = 0u; i < NG_MAX_CHARS; i++) {
        NGCharacter *c = chars_at(i);
        if (!c || !c->active) continue;

        if (c->kind == SKY_KIND_PSHOT) {
            if (nbolts < (uint8_t)(SKY_MAX_PSHOTS + 2u)) bolts[nbolts++] = i;
        } else if (c->kind == SKY_KIND_ENEMY || c->kind == SKY_KIND_BOSS) {
            if (nmarks < (uint8_t)(SKY_MAX_ENEMIES + 2u)) marks[nmarks++] = i;
        }
    }

    /* Player bolts against enemies and the boss. */
    for (i = 0u; i < nbolts; i++) {
        NGCharacter *a = chars_at(bolts[i]);
        if (!a || !a->active || a->kind != SKY_KIND_PSHOT) continue;

        for (j = 0u; j < nmarks; j++) {
            NGCharacter *b = chars_at(marks[j]);
            if (!b || !b->active ||
                (b->kind != SKY_KIND_ENEMY && b->kind != SKY_KIND_BOSS)) continue;
            if (!ng_rect_hit(ng_char_body_rect(a), ng_char_body_rect(b))) continue;

            ng_char_damage(b, a->data0 ? 6u : 1u);
            if (b->hp == 0u) {
                int16_t bx = b->x, by = b->y;
                uint8_t was_boss = (uint8_t)(b->kind == SKY_KIND_BOSS);
                s_score += sky_stage_defeated(b);
                ng_chars_remove(b);
                sky_spawn_blast(bx, by);
                if (was_boss) {
                    sky_spawn_blast((int16_t)(bx - 24), (int16_t)(by - 16));
                    sky_spawn_blast((int16_t)(bx + 24), (int16_t)(by - 12));
                    sky_spawn_blast((int16_t)(bx - 12), (int16_t)(by + 20));
                    playSFX(SOUND_SFX_10);
                } else {
                    sky_drop_pickup(bx, by);
                    playSFX(SOUND_SFX_5);
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
        for (i = 0u; i < NG_MAX_CHARS; i++) {
            NGCharacter *a = chars_at(i);
            if (!a || !a->active) continue;
            if (a->kind != SKY_KIND_ESHOT && a->kind != SKY_KIND_ENEMY &&
                a->kind != SKY_KIND_BOSS) continue;
            if (!ng_rect_hit(ng_char_body_rect(a), ng_char_body_rect(s_player))) continue;
            if (a->kind == SKY_KIND_ESHOT) ng_chars_remove(a);
            if (sky_player_hit()) { dead = 1u; }
            break;      /* one hit per frame - the i-frames cover the rest */
        }
    }

    if (s_player && s_player->active) for (i = 0u; i < NG_MAX_CHARS; i++) {
        NGCharacter *pickup = chars_at(i);
        if (!pickup || !pickup->active || pickup->kind != SKY_KIND_PICKUP) continue;
        if (!ng_rect_hit(ng_char_body_rect(pickup), ng_char_body_rect(s_player))) continue;
        if (pickup->data0 == 0u) s_speed_bonus = 1u;
        else if (pickup->data0 == 1u) s_missiles = (uint8_t)NG_MIN(9u, s_missiles + 2u);
        else if (s_lives < SKY_LIVES_MAX) s_lives++;
        else s_energy = SKY_ENERGY_MAX;
        s_score += 250u;
        ng_chars_remove(pickup);
        playSFX(SOUND_SFX_1);
    }
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
    soundSetADPCMBVolume(0xB8u); waitVbl();
    soundSetSSGVolume(0x00u);    waitVbl();
    soundSetFMVolume(0x00u);     waitVbl();
    playSFXB(track);
    waitVbl();
}


static void NEOGEO_USER sky_title_card(void)
{
    uint16_t t;

    sky_scene_begin();
    sky_bg_select(SKY_BG_MOUNTAIN);

    sky_puts(15u,  7u, "SKY  LANCE",            SKY_PAL_TITLE);
    sky_puts( 9u,  9u, "NEO GEO 2D  SORTIE 01", SKY_PAL_BODY);
    sky_puts(11u, 20u, "PRESS A TO SORTIE",     SKY_PAL_SCORE);
    sky_puts( 6u, 22u, "MOVE  A/B FIRE  D MISSILE", SKY_PAL_BODY);

    for (t = 0u; t < 420u; t++) {
        sky_frame();
        sky_bg_advance(1u);
        sky_bg_draw();
        if (sky_joy_pressed() & (BUTTON_A | BUTTON_B)) return;
    }
}

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
        int16_t cx = (int16_t)(SKY_FIELD_X + 48 + i * 80);
        face[i]  = sky_spawn(SKY_KIND_FACE,   k_pilot[i].face,  cx,  90,
                             SKY_SCALE_PORTRAIT, NG_RENDER_BAND_NPC);
        plane[i] = sky_spawn(SKY_KIND_PLAYER, k_pilot[i].plane, cx, 170,
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
                    face[i]->visible = sel;
                }
            }
            sky_fix_blank(6u, 21u, 28u);
            sky_fix_blank(6u, 22u, 28u);
            sky_puts(12u, 21u, k_pilot[s_pick].call, SKY_PAL_SCORE);
            sky_puts(12u, 22u, k_pilot[s_pick].name, SKY_PAL_BODY);
        }

        ng_chars_update();
        ng_chars_draw();

        if (pressed & (BUTTON_A | BUTTON_B)) {
            playSFX(SOUND_SFX_3);
            return 1u;
        }
    }
}

/* One stage.  0 = squad down, 1 = stage cleared. */
static uint8_t NEOGEO_USER sky_fly_stage(uint8_t stage)
{
    uint8_t cleared = 0u;
    uint16_t banner = 120u;
    char tag[20];

    sky_scene_begin();
    sky_bg_select(sky_stage_bg(stage));
    sky_stage_begin(stage);
    sky_hud_static();

    /* The pilot chosen at the roster rides along in the margin beside
     * the playfield for the whole sortie, the way the arcade cabinet
     * keeps the character you picked in view.  The margin is 32 px wide,
     * which three sixteenths of the portrait fills exactly.  It sits in
     * the back band: characters are allotted sprite slots back to front,
     * so on a crowded frame the front-most are the ones dropped, and
     * nothing else is ever drawn in the margin for it to be behind. */
    sky_spawn(SKY_KIND_FACE, k_pilot[s_pick % SKY_PILOTS].face,
              SKY_FIELD_X / 2, 56, NG_SCALE(3), NG_RENDER_BAND_BACK);

    s_player = sky_spawn(SKY_KIND_PLAYER, k_pilot[s_pick % SKY_PILOTS].plane,
                         SKY_FIELD_X + SKY_FIELD_W / 2, SKY_FIELD_BOTTOM - 32,
                         SKY_SCALE_PLAYER, NG_RENDER_BAND_PLAYER);
    if (!s_player) return 0u;
    if (s_player) {
        /* The plane's hitbox is deliberately tiny - a shmup is read from
         * the nose of the aircraft, not from its wingspan. */
        ng_char_set_body(s_player, -6, -6, 12, 12);
    }
    s_invuln  = 120u;
    s_fire_cd = 0u;

    tag[0] = 'S'; tag[1] = 'T'; tag[2] = 'A'; tag[3] = 'G'; tag[4] = 'E';
    tag[5] = ' '; tag[6] = (char)('0' + ((stage + 1u) / 10u));
    tag[7] = (char)('0' + ((stage + 1u) % 10u)); tag[8] = '\0';
    sky_puts(16u, 11u, tag, SKY_PAL_TITLE);
    sky_puts(7u, 13u, sky_stage_subtitle(stage), SKY_PAL_SCORE);
    playSFX(SOUND_SFX_3);

    for (;;) {
        sky_frame();

        sky_bg_advance(1u);
        sky_bg_draw();

        sky_player_step();
        if (sky_stage_tick(s_player ? s_player->x : 0,
                           s_player ? s_player->y : 0)) {
            cleared = 1u;
        }

        ng_chars_update();
        if (sky_collide()) return 0u;
        ng_chars_draw();

        sky_hud_draw();
        if (sky_stage_boss_active()) {
            sky_hud_boss();
        }

        if (banner) {
            banner--;
            if (!banner) {
                sky_fix_blank(6u, 11u, 28u);
                sky_fix_blank(6u, 13u, 28u);
            }
        }

        if (cleared) {
            sky_fix_blank(8u, 2u, 24u);          /* the boss bar */
            sky_puts(13u, 12u, "STAGE CLEAR", SKY_PAL_SCORE);
            playSFX(SOUND_SFX_11);
            {
                uint16_t t;
                uint8_t i;
                for (i = 0u; i < NG_MAX_CHARS; i++) {
                    NGCharacter *c = chars_at(i);
                    if (c && c->active && c->kind != SKY_KIND_PLAYER &&
                        c->kind != SKY_KIND_FACE && c->kind != SKY_KIND_BLAST) ng_chars_remove(c);
                }
                for (t = 0u; t < 150u; t++) {
                    sky_frame();
                    sky_bg_advance(1u);
                    sky_bg_draw();
                    sky_stage_effects_tick();
                    ng_chars_draw();
                }
            }
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
    sky_puts( 6u, 18u, "ART      ARTBOX PIPELINE",     SKY_PAL_BODY);
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
    s_speed_bonus = 0u;

    soundSceneReset();
    sky_music(SOUND_TRACK_A);
    (void)sky_select();

    for (;;) {
        static const uint8_t k_stage_track[SKY_STAGE_COUNT] = {
            SOUND_TRACK_B, SOUND_TRACK_C, SOUND_TRACK_D, SOUND_TRACK_F,
            SOUND_TRACK_B, SOUND_TRACK_C, SOUND_TRACK_D
        };
        sky_music(k_stage_track[s_stage % SKY_STAGE_COUNT]);
        if (!sky_fly_stage(s_stage)) {
            sky_music(SOUND_TRACK_H);
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
        if (NEO_REGISTER8(NGO_START_FLAG) || read_p1credit() > 0) return;

        sky_scene_begin();
        sky_bg_select(SKY_BG_COAST);
        sky_puts(12u, 1u, "SKY LANCE  SQUADRON", SKY_PAL_TITLE);

        for (i = 0u; i < SKY_PILOTS; i++) {
            int16_t cx = (int16_t)(SKY_FIELD_X + 48 + i * 80);
            sky_spawn(SKY_KIND_FACE,   k_pilot[i].face,  cx,  90,
                      SKY_SCALE_PORTRAIT, NG_RENDER_BAND_NPC);
            sky_spawn(SKY_KIND_PLAYER, k_pilot[i].plane, cx, 170,
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
                if (NEO_REGISTER8(NGO_START_FLAG) || read_p1credit() > 0) return;
            }
        }
    }
}
