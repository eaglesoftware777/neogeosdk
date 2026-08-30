/*
 * sky_stage.c — Sky Lance enemy tables, wave director and boss AI.
 *
 * The shape of a stage follows the arcade vertical shooter it is modelled
 * on: squadrons fly in on a fixed pattern rather than trickling in at
 * random, each squadron is one enemy type so the player can read it at a
 * glance, and the stage ends with a single named boss that has to be
 * fought at close range.  The randomness is confined to which squadron
 * comes next and where it enters.
 *
 * https://eaglesoftware.biz
 */

#include "sky_stage.h"
#include "sky_draw.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "sdk/2d_engine/ng_defs.h"

void NEOGEO_USER playSFX(uint8_t n);

/* ------------------------------------------------------------------ */
/*  Enemy roster                                                        */
/* ------------------------------------------------------------------ */
/* Flight patterns.  Stored per-enemy in NGCharacter.data0. */
#define PAT_DIVE     0u   /* straight down, weaving                      */
#define PAT_STRAFE   1u   /* crosses the screen, drifting down slowly    */
#define PAT_HOLD     2u   /* descends to a firing line and sits there    */

typedef struct {
    uint8_t asset;
    uint8_t hp;
    uint8_t scale;
    uint8_t pattern;
    uint8_t speed;        /* pixels per 16 frames along the main axis */
    uint8_t fire_odds;    /* 1-in-N per frame, 0 = never shoots        */
    uint16_t score;
} SkyEnemy;

#define ENEMY_COUNT 7u
static const SkyEnemy k_enemy[ENEMY_COUNT] = {
    /* asset,             hp, scale,           pattern,  spd, odds, score */
    { SKY_E_DRONE,         1u, SKY_SCALE_SMALL, PAT_DIVE,   34u,   0u,   50u },
    { SKY_E_FIGHTER,       3u, SKY_SCALE_ENEMY, PAT_DIVE,   24u, 150u,  100u },
    { SKY_E_INTERCEPTOR,   4u, SKY_SCALE_ENEMY, PAT_STRAFE, 30u, 130u,  150u },
    { SKY_E_HELI,          5u, SKY_SCALE_ENEMY, PAT_HOLD,   18u,  90u,  180u },
    { SKY_E_MISSILE_BOAT,  7u, SKY_SCALE_ENEMY, PAT_HOLD,   14u,  80u,  200u },
    { SKY_E_TANK,          6u, SKY_SCALE_ENEMY, PAT_HOLD,   12u, 100u,  120u },
    { SKY_E_BOMBER,        8u, SKY_SCALE_ENEMY, PAT_DIVE,   16u, 110u,  250u }
};

/* ------------------------------------------------------------------ */
/*  Stage table                                                         */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t  bg;
    uint8_t  boss;
    uint8_t  boss_hp;
    uint16_t boss_score;
    uint8_t  roster[4];      /* indices into k_enemy      */
    uint8_t  waves;          /* squadrons before the boss */
    const char *boss_name;
} SkyStage;

static const SkyStage k_stage[SKY_STAGE_COUNT] = {
    { SKY_BG_MOUNTAIN, SKY_B_RED_FORTRESS, 60u, 5000u, {0u,1u,1u,2u}, 5u, "CRIMSON KEEP" },
    { SKY_BG_COAST,    SKY_B_BATTLESHIP,   70u, 6000u, {1u,2u,4u,4u}, 5u, "IRON TIDE"    },
    { SKY_BG_MOUNTAIN, SKY_B_GOLD_CORE,    65u, 4500u, {0u,2u,3u,6u}, 6u, "SOL CORE"     },
    { SKY_BG_COAST,    SKY_B_STEALTH,      75u, 5500u, {1u,2u,2u,6u}, 6u, "NIGHT RAZOR"  },
    { SKY_BG_MOUNTAIN, SKY_B_HELI_CARRIER, 70u, 4800u, {3u,3u,1u,6u}, 6u, "ROTOR NEST"   },
    { SKY_BG_COAST,    SKY_B_TANK_FORT,    85u, 6200u, {5u,5u,2u,6u}, 7u, "EARTH HAMMER" },
    { SKY_BG_MOUNTAIN, SKY_B_CATHEDRAL,    95u, 8000u, {2u,3u,6u,6u}, 7u, "SPIRE GOD"    }
};

/* ------------------------------------------------------------------ */
/*  Director state                                                      */
/* ------------------------------------------------------------------ */
static uint8_t  s_stage;
static uint8_t  s_waves_left;
static uint16_t s_wave_timer;
static uint8_t  s_squad_left;      /* enemies still to release this squadron */
static uint8_t  s_squad_type;
static uint8_t  s_squad_side;
static uint16_t s_squad_gap;
static uint8_t  s_boss_on;
static uint8_t  s_boss_hp_max;
static uint16_t s_kills;
static uint16_t s_tick;

uint8_t NEOGEO_USER sky_stage_bg(uint8_t stage)
{
    return k_stage[stage % SKY_STAGE_COUNT].bg;
}

const char * NEOGEO_USER sky_stage_boss_name(uint8_t stage)
{
    return k_stage[stage % SKY_STAGE_COUNT].boss_name;
}

uint8_t NEOGEO_USER sky_stage_boss_active(void) { return s_boss_on; }
uint16_t NEOGEO_USER sky_stage_kills(void)      { return s_kills; }

void NEOGEO_USER sky_stage_begin(uint8_t stage)
{
    const SkyStage *st = &k_stage[stage % SKY_STAGE_COUNT];

    s_stage       = (uint8_t)(stage % SKY_STAGE_COUNT);
    s_waves_left  = st->waves;
    s_wave_timer  = 90u;
    s_squad_left  = 0u;
    s_squad_gap   = 0u;
    s_boss_on     = 0u;
    s_boss_hp_max = 1u;
    s_kills       = 0u;
    s_tick        = 0u;
}

/*
 * Count live objects of one kind.
 *
 * This walks the whole NG_MAX_CHARS array on purpose.  ng_chars_count()
 * returns how many slots are ACTIVE, not the highest slot in use, so
 * `for (i = 0; i < ng_chars_count(); i++) chars_at(i)` silently skips
 * the tail of the array as soon as any slot in the middle is freed.
 */
uint8_t NEOGEO_USER sky_count_kind(uint8_t kind)
{
    uint8_t i, total = 0u;
    for (i = 0u; i < NG_MAX_CHARS; i++) {
        NGCharacter *c = chars_at(i);
        if (c && c->active && c->kind == kind) total++;
    }
    return total;
}

/* ------------------------------------------------------------------ */
/*  Spawns                                                              */
/* ------------------------------------------------------------------ */
NGCharacter * NEOGEO_USER sky_spawn_blast(int16_t x, int16_t y)
{
    NGCharacter *c;

    if (sky_count_kind(SKY_KIND_BLAST) >= SKY_MAX_BLASTS) return 0;
    /*
     * There is no explosion frame in this art set, so the enemy shot
     * ring doubles as one: spawned at the kill point, it expands over
     * six frames via SCB2 and is then dropped.  data0 carries the age.
     */
    c = sky_spawn(SKY_KIND_BLAST, SKY_SHOT_RING, x, y, SKY_SCALE_SMALL, NG_RENDER_BAND_FX);
    if (c) c->data0 = 0u;
    return c;
}

static void NEOGEO_USER sky_spawn_eshot(int16_t x, int16_t y, int16_t vx, int16_t vy)
{
    NGCharacter *s;

    if (sky_count_kind(SKY_KIND_ESHOT) >= SKY_MAX_ESHOTS) return;
    s = sky_spawn(SKY_KIND_ESHOT, SKY_SHOT_ORB, x, y, SKY_SCALE_SMALL, NG_RENDER_BAND_FX);
    if (!s) return;
    ng_char_set_speed(s, vx, vy);
    ng_char_set_body(s, -5, -5, 10, 10);
}

/* Fire one orb from (x,y) towards the player, at a fixed speed. */
static void NEOGEO_USER sky_fire_aimed(int16_t x, int16_t y,
                                       int16_t px, int16_t py, int16_t speed)
{
    int16_t dx = (int16_t)(px - x);
    int16_t dy = (int16_t)(py - y);
    int16_t adx = (int16_t)(dx < 0 ? -dx : dx);
    int16_t ady = (int16_t)(dy < 0 ? -dy : dy);
    int16_t mag = (int16_t)(adx > ady ? adx + (ady >> 1) : ady + (adx >> 1));

    /* Octagonal distance instead of a square root: within ~6% of the
     * true length, and it keeps the whole aiming path in 16-bit ints. */
    if (mag < 16) mag = 16;
    sky_spawn_eshot(x, y,
                    (int16_t)((int32_t)dx * speed / mag),
                    (int16_t)((int32_t)dy * speed / mag));
}

static void NEOGEO_USER sky_spawn_enemy(uint8_t type, int16_t x, int16_t y, int16_t side)
{
    const SkyEnemy *e = &k_enemy[type % ENEMY_COUNT];
    NGCharacter *c;

    if (sky_count_kind(SKY_KIND_ENEMY) >= SKY_MAX_ENEMIES) return;

    c = sky_spawn(SKY_KIND_ENEMY, e->asset, x, y, e->scale, NG_RENDER_BAND_ENEMY);
    if (!c) return;

    c->hp     = (uint8_t)(e->hp + (s_stage >> 1));
    c->max_hp = c->hp;
    c->data0  = e->pattern;
    c->data1  = (uint16_t)(sky_rand() & 63u);      /* weave phase */
    c->data2  = type;

    switch (e->pattern) {
    case PAT_STRAFE:
        ng_char_set_speed_fp(c,
                             (int32_t)side * ((int32_t)e->speed << NG_FP_SHIFT) / 16,
                             ((int32_t)e->speed << NG_FP_SHIFT) / 48);
        break;
    case PAT_HOLD:
        ng_char_set_speed_fp(c, 0, ((int32_t)e->speed << NG_FP_SHIFT) / 16);
        break;
    default:
        ng_char_set_speed_fp(c, 0, ((int32_t)e->speed << NG_FP_SHIFT) / 16);
        break;
    }
}

static void NEOGEO_USER sky_spawn_boss(void)
{
    const SkyStage *st = &k_stage[s_stage];
    NGCharacter *c = sky_spawn(SKY_KIND_BOSS, st->boss,
                               SKY_FIELD_X + SKY_FIELD_W / 2, -40,
                               SKY_SCALE_BOSS, NG_RENDER_BAND_ENEMY);
    if (!c) return;

    c->hp     = st->boss_hp;
    c->max_hp = st->boss_hp;
    s_boss_hp_max = st->boss_hp;
    c->data0  = 0u;    /* 0 = entering, 1 = engaged */
    c->data1  = 0u;    /* burst timer               */
    ng_char_set_speed(c, 0, 1);
    s_boss_on = 1u;
    playSFX(SOUND_SFX_2);
}

uint8_t NEOGEO_USER sky_stage_boss_bar(void)
{
    uint8_t i;

    for (i = 0u; i < NG_MAX_CHARS; i++) {
        NGCharacter *c = chars_at(i);
        if (c && c->active && c->kind == SKY_KIND_BOSS) {
            if (!s_boss_hp_max) return 0u;
            return (uint8_t)(((uint16_t)c->hp * 24u) / s_boss_hp_max);
        }
    }
    return 0u;
}

/* ------------------------------------------------------------------ */
/*  Per-enemy behaviour                                                 */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER sky_enemy_step(NGCharacter *c, int16_t px, int16_t py)
{
    const SkyEnemy *e = &k_enemy[c->data2 % ENEMY_COUNT];

    c->data1++;

    switch (c->data0) {
    case PAT_DIVE: {
        /* Triangle-wave weave: cheaper than a sine table and, at this
         * amplitude, indistinguishable on a 320-px screen. */
        uint8_t phase = (uint8_t)(c->data1 & 63u);
        int16_t sway  = (int16_t)((phase < 32u) ? ((int16_t)phase - 16)
                                                : (47 - (int16_t)phase));
        ng_char_set_speed_fp(c, (int32_t)sway * (NG_FP_ONE / 12), c->vy_fp);
        break;
    }
    case PAT_STRAFE:
        /* Turn around at the field edge instead of flying off it. */
        if (c->x < SKY_FIELD_LEFT || c->x > SKY_FIELD_RIGHT) {
            c->vx_fp = -c->vx_fp;
        }
        break;
    case PAT_HOLD:
        /* Descend to a firing line, then hold station and shoot. */
        if (c->y > SKY_FIELD_TOP + 40) {
            ng_char_set_speed(c, 0, 0);
        }
        break;
    default:
        break;
    }

    if (e->fire_odds && (sky_rand() % e->fire_odds) == 0u) {
        sky_fire_aimed(c->x, (int16_t)(c->y + 8), px, py,
                       (int16_t)(2 + (s_stage >> 1)));
    }

    /* Off the bottom: gone, no explosion, no score. */
    if (c->y > SKY_FIELD_BOTTOM + 40) ng_chars_remove(c);
}

static void NEOGEO_USER sky_boss_step(NGCharacter *c, int16_t px, int16_t py)
{
    const int16_t left  = SKY_FIELD_X + 56;
    const int16_t right = SKY_FIELD_X + SKY_FIELD_W - 56;

    if (c->data0 == 0u) {
        /* Entry: slide down to the top of the arena, then engage. */
        if (c->y >= SKY_FIELD_TOP + 28) {
            c->data0 = 1u;
            ng_char_set_speed(c, 1, 0);
        }
        return;
    }

    if (c->x <= left)  ng_char_set_speed(c,  1, 0);
    if (c->x >= right) ng_char_set_speed(c, -1, 0);

    c->data1++;
    /*
     * Two attacks, alternating: a spread of five orbs across the arena,
     * and a single aimed shot that punishes the player for parking under
     * the boss.  The interval tightens as the boss loses health.
     */
    {
        uint16_t interval = (uint16_t)(96u - (uint16_t)(s_stage * 6u));
        if (c->hp * 2u < c->max_hp) interval = (uint16_t)(interval >> 1);
        if (interval < 24u) interval = 24u;

        if ((c->data1 % interval) == 0u) {
            int16_t i;
            for (i = -2; i <= 2; i++) {
                sky_spawn_eshot(c->x, (int16_t)(c->y + 24), (int16_t)(i * 2), 3);
            }
        } else if ((c->data1 % (uint16_t)(interval >> 1)) == 0u) {
            sky_fire_aimed(c->x, (int16_t)(c->y + 24), px, py, 4);
        }
    }
}

static void NEOGEO_USER sky_blast_step(NGCharacter *c)
{
    /* Expand and fade out.  Six frames is short enough that several can
     * overlap on a busy screen without eating the sprite budget. */
    c->data0++;
    c->scale_x = (uint8_t)(SKY_SCALE_SMALL + (uint8_t)(c->data0 * 16u));
    c->scale_y = c->scale_x;
    c->sprite_dirty = 1u;
    if (c->data0 >= 6u) ng_chars_remove(c);
}

/* ------------------------------------------------------------------ */
/*  Wave director                                                       */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER sky_release_next(void)
{
    const SkyStage *st = &k_stage[s_stage];
    int16_t x;

    if (!s_squad_left) return;
    if (s_squad_gap) { s_squad_gap--; return; }

    if (k_enemy[s_squad_type % ENEMY_COUNT].pattern == PAT_STRAFE) {
        x = (int16_t)(s_squad_side > 0 ? SKY_FIELD_LEFT : SKY_FIELD_RIGHT);
    } else {
        /* Spread the squadron across the arena on a fixed lane so the
         * formation reads as a formation rather than as noise. */
        uint8_t lane = (uint8_t)(s_squad_left & 3u);
        x = (int16_t)(SKY_FIELD_LEFT + 24 + lane * ((SKY_FIELD_W - 72) / 3));
    }

    sky_spawn_enemy(s_squad_type, x, (int16_t)(SKY_FIELD_TOP - 24),
                    (int16_t)(s_squad_side > 0 ? 1 : -1));
    s_squad_left--;
    s_squad_gap = 18u;
    NG_UNUSED(st);
}

static void NEOGEO_USER sky_start_squad(void)
{
    const SkyStage *st = &k_stage[s_stage];

    s_squad_type = st->roster[sky_rand() & 3u];
    s_squad_side = (uint8_t)(sky_rand() & 1u);
    s_squad_left = (uint8_t)(3u + (sky_rand() % 2u));
    s_squad_gap  = 0u;
    if (s_waves_left) s_waves_left--;
}

uint8_t NEOGEO_USER sky_stage_tick(int16_t player_x, int16_t player_y)
{
    uint8_t i;
    uint8_t cleared = 0u;

    s_tick++;

    /* --- per-object behaviour ------------------------------------- */
    for (i = 0u; i < NG_MAX_CHARS; i++) {
        NGCharacter *c = chars_at(i);
        if (!c || !c->active) continue;

        switch (c->kind) {
        case SKY_KIND_ENEMY: sky_enemy_step(c, player_x, player_y); break;
        case SKY_KIND_BOSS:  sky_boss_step(c, player_x, player_y);  break;
        case SKY_KIND_BLAST: sky_blast_step(c);                     break;
        case SKY_KIND_PSHOT:
            if (c->y < SKY_FIELD_TOP - 24) ng_chars_remove(c);
            break;
        case SKY_KIND_ESHOT:
            if (c->y > SKY_FIELD_BOTTOM + 24 || c->y < SKY_FIELD_TOP - 24 ||
                c->x < SKY_FIELD_X - 16 || c->x > SKY_FIELD_X + SKY_FIELD_W + 16) {
                ng_chars_remove(c);
            }
            break;
        default: break;
        }
    }

    /* --- wave scheduling ------------------------------------------ */
    if (s_boss_on) {
        if (!sky_count_kind(SKY_KIND_BOSS)) {
            s_boss_on = 0u;
            cleared = 1u;
        }
        return cleared;
    }

    if (s_squad_left) {
        sky_release_next();
    } else if (s_wave_timer) {
        s_wave_timer--;
    } else if (s_waves_left) {
        sky_start_squad();
        s_wave_timer = (uint16_t)(150u - (uint16_t)(s_stage * 10u));
    } else if (!sky_count_kind(SKY_KIND_ENEMY)) {
        sky_spawn_boss();
    }

    return 0u;
}
