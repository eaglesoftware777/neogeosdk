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

#define SKY_ENERGY_MAX   100u
#define SKY_ENERGY_HIT    34u
#define SKY_START_LIVES    2u

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
    sky_infix(1u,  0u, SKY_INFIX_SCORE);
    sky_infix(24u, 0u, SKY_INFIX_HI);
    sky_infix(1u, 26u, (uint8_t)(SKY_INFIX_1P + (s_pick % 3u)));
}

static void NEOGEO_USER sky_hud_draw(void)
{
    uint8_t i;
    uint8_t cells = (uint8_t)(s_energy / 10u);

    sky_infix_number(7u,  0u, s_score,   6u);
    sky_infix_number(27u, 0u, s_hiscore, 6u);

    /* Lives pips: one per spare plane, blanked back out as they go. */
    for (i = 0u; i < 3u; i++) {
        if (i < s_lives) sky_infix((uint8_t)(4u + i * 2u), 26u, SKY_INFIX_LIFE);
        else {
            sky_fix_blank((uint8_t)(4u + i * 2u), 26u, 2u);
            sky_fix_blank((uint8_t)(4u + i * 2u), 27u, 2u);
        }
    }

    if (cells > 10u) cells = 10u;
    for (i = 0u; i < 10u; i++) {
        sky_infix((uint8_t)(14u + i * 2u), 26u,
                  (uint8_t)(i < cells ? SKY_INFIX_ENERGY : SKY_INFIX_ENERGY_MT));
    }
}

static void NEOGEO_USER sky_hud_boss(void)
{
    char bar[26];
    uint8_t n = sky_stage_boss_bar();
    uint8_t i;

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
    if (sky_count_kind(SKY_KIND_PSHOT) >= SKY_MAX_PSHOTS) return;

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
    int16_t  x, y;

    if (!s_player || !s_player->active) return;

    x = s_player->x;
    y = s_player->y;
    if (joy & JOY_LEFT)  x = (int16_t)(x - p->speed);
    if (joy & JOY_RIGHT) x = (int16_t)(x + p->speed);
    if (joy & JOY_UP)    y = (int16_t)(y - p->speed);
    if (joy & JOY_DOWN)  y = (int16_t)(y + p->speed);

    ng_char_set_pos(s_player,
                    (int16_t)NG_CLAMP(x, SKY_FIELD_LEFT, SKY_FIELD_RIGHT),
                    (int16_t)NG_CLAMP(y, SKY_FIELD_TOP + 8, SKY_FIELD_BOTTOM));

    /* Auto-fire on hold, like the arcade board: A is a trigger, not a
     * typing test. */
    if (joy & (BUTTON_A | BUTTON_B)) sky_fire();
    if (s_fire_cd) s_fire_cd--;

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
        if (!a || !a->active) continue;

        for (j = 0u; j < nmarks; j++) {
            NGCharacter *b = chars_at(marks[j]);
            if (!b || !b->active) continue;
            if (!ng_rect_hit(ng_char_body_rect(a), ng_char_body_rect(b))) continue;

            ng_char_damage(b, 1u);
            if (b->hp == 0u) {
                int16_t bx = b->x, by = b->y;
                uint8_t was_boss = (uint8_t)(b->kind == SKY_KIND_BOSS);
                ng_chars_remove(b);
                sky_spawn_blast(bx, by);
                s_score += was_boss ? 5000u : 150u;
                playSFX(SOUND_SFX_5);
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

/* Wait `frames`, or until a button lands.  1 = the player pressed. */
static uint8_t NEOGEO_USER sky_hold(uint16_t frames)
{
    uint16_t t;
    for (t = 0u; t < frames; t++) {
        sky_frame();
        if (sky_joy_pressed() & (BUTTON_A | BUTTON_B | BUTTON_C | BUTTON_D)) return 1u;
    }
    return 0u;
}

static void NEOGEO_USER sky_title_card(void)
{
    uint16_t t;

    sky_scene_begin();
    sky_bg_select(SKY_BG_MOUNTAIN);

    sky_puts(15u,  7u, "SKY  LANCE",            SKY_PAL_TITLE);
    sky_puts( 9u,  9u, "NEO GEO 2D  SORTIE 01", SKY_PAL_BODY);
    sky_puts(11u, 20u, "PRESS A TO SORTIE",     SKY_PAL_SCORE);
    sky_puts( 9u, 22u, "JOYSTICK MOVE   A FIRE", SKY_PAL_BODY);

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
                    plane[i]->scale_x = sel ? SKY_SCALE_PLAYER : SKY_SCALE_ROSTER;
                    plane[i]->scale_y = plane[i]->scale_x;
                    plane[i]->sprite_dirty = 1u;
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

    s_player = sky_spawn(SKY_KIND_PLAYER, k_pilot[s_pick % SKY_PILOTS].plane,
                         SKY_FIELD_X + SKY_FIELD_W / 2, SKY_FIELD_BOTTOM - 24,
                         SKY_SCALE_PLAYER, NG_RENDER_BAND_PLAYER);
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
    sky_puts(16u, 12u, tag, SKY_PAL_TITLE);

    for (;;) {
        sky_frame();

        sky_bg_advance((uint8_t)(1u + (stage >> 2)));
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
            if (!banner) sky_fix_blank(13u, 12u, 14u);
        }

        if (cleared) {
            sky_puts(13u, 12u, "STAGE CLEAR", SKY_PAL_SCORE);
            if (!sky_hold(180u)) { /* let the last blasts play out */ }
            return 1u;
        }
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

    soundSceneReset();
    sky_music(SOUND_TRACK_A);
    (void)sky_select();

    for (;;) {
        /* TRACK_E is reserved for the eyecatcher, so the stage beds walk
         * an explicit table rather than an arithmetic run. */
        static const uint8_t k_stage_track[4] = {
            SOUND_TRACK_B, SOUND_TRACK_C, SOUND_TRACK_D, SOUND_TRACK_F
        };
        sky_music(k_stage_track[s_stage & 3u]);
        if (!sky_fly_stage(s_stage)) break;
        s_stage = (uint8_t)((s_stage + 1u) % SKY_STAGE_COUNT);
        /* A cleared stage tops the energy back up but never the lives -
         * the run has to stay finite. */
        s_energy = SKY_ENERGY_MAX;
    }

    sky_music(SOUND_TRACK_H);
    sky_game_over();
    soundStopAll();
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
