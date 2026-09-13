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
#define PAT_SURFACE  3u   /* terrain-bound vehicle, no airborne chase   */

/* How long a PAT_HOLD enemy keeps its station before resuming the descent,
 * in frames.  Bounded so a stage cannot stall waiting for it to leave. */
#define SKY_HOLD_FRAMES  300u

/* Frames the director will wait for leftover enemies once the waves are spent
 * before sending the boss in regardless. */
#define SKY_BOSS_WAIT_MAX  420u

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
    { SKY_E_MISSILE_BOAT,  7u, SKY_SCALE_ENEMY, PAT_SURFACE,16u, 120u,  200u },
    { SKY_E_TANK,          6u, SKY_SCALE_ENEMY, PAT_SURFACE,16u, 140u,  120u },
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
    uint8_t  has_boss;       /* 1 if stage ends with boss battle */
    uint8_t  roster[4];      /* indices into k_enemy */
    uint8_t  waves;          /* squadrons before boss / stage clear */
    const char *boss_name;
    const char *subtitle;
} SkyStage;

static const SkyStage k_stage[SKY_STAGE_COUNT] = {
    { SKY_BG_MOUNTAIN, SKY_B_RED_FORTRESS, 48u, 5000u, 1u, {0u,1u,2u,1u}, 5u,
      "CRIMSON KEEP", "01 / 07  MOUNTAIN DAWN" },
    { SKY_BG_COAST, SKY_B_BATTLESHIP, 60u, 6000u, 1u, {4u,1u,4u,6u}, 5u,
      "IRON TIDE", "02 / 07  HARBOR ASSAULT" },
    { SKY_BG_MOUNTAIN, SKY_B_GOLD_CORE, 64u, 6500u, 1u, {5u,2u,0u,6u}, 6u,
      "SOL CORE", "03 / 07  VALLEY CONVOY" },
    { SKY_BG_OPEN_SEA, SKY_B_STEALTH, 68u, 7000u, 1u, {2u,4u,1u,6u}, 6u,
      "NIGHT RAZOR", "04 / 07  REEF INTERCEPT" },
    { SKY_BG_MOUNTAIN, SKY_B_HELI_CARRIER, 76u, 7500u, 1u, {3u,5u,3u,6u}, 6u,
      "ROTOR NEST", "05 / 07  FOREST OUTPOST" },
    { SKY_BG_MOUNTAIN, SKY_B_TANK_FORT, 84u, 8000u, 1u, {5u,2u,5u,6u}, 7u,
      "EARTH HAMMER", "06 / 07  ARMORED COLUMN" },
    { SKY_BG_COAST, SKY_B_CATHEDRAL, 96u, 10000u, 1u, {4u,2u,3u,6u}, 7u,
      "CRIMSON CITADEL", "07 / 07  FINAL APPROACH" }
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
static uint16_t s_boss_wait;
static uint8_t  s_boss_hp_max;
static uint16_t s_kills;
static uint16_t s_tick;

uint8_t NEOGEO_USER sky_stage_bg(uint8_t stage)
{
    return k_stage[stage % SKY_STAGE_COUNT].bg;
}

uint8_t NEOGEO_USER sky_stage_has_boss(uint8_t stage)
{
    return k_stage[stage % SKY_STAGE_COUNT].has_boss;
}

const char * NEOGEO_USER sky_stage_boss_name(uint8_t stage)
{
    return k_stage[stage % SKY_STAGE_COUNT].boss_name;
}

const char * NEOGEO_USER sky_stage_subtitle(uint8_t stage)
{
    return k_stage[stage % SKY_STAGE_COUNT].subtitle;
}

uint8_t NEOGEO_USER sky_stage_boss_active(void) { return s_boss_on; }
uint16_t NEOGEO_USER sky_stage_kills(void)      { return s_kills; }

uint16_t NEOGEO_USER sky_stage_defeated(const NGCharacter *c)
{
    if (!c) return 0u;
    if (c->kind == SKY_KIND_BOSS) return k_stage[s_stage].boss_score;
    if (c->kind != SKY_KIND_ENEMY) return 0u;
    s_kills++;
    return k_enemy[c->data2 % ENEMY_COUNT].score;
}

void NEOGEO_USER sky_stage_begin(uint8_t stage)
{
    const SkyStage *st = &k_stage[stage % SKY_STAGE_COUNT];

    s_stage       = (uint8_t)(stage % SKY_STAGE_COUNT);
    s_waves_left  = st->waves;
    s_wave_timer  = 90u;
    s_squad_left  = 0u;
    s_squad_gap   = 0u;
    s_boss_on     = 0u;
    s_boss_wait   = 0u;
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
    c = sky_spawn(SKY_KIND_BLAST, SKY_SHOT_RING, x, y, SKY_BURST_SCALE(0), NG_RENDER_BAND_FX);
    if (c) c->data0 = 0u;
    return c;
}

static void NEOGEO_USER sky_spawn_eshot(int16_t x, int16_t y, int16_t vx, int16_t vy)
{
    NGCharacter *s;

    if (sky_count_kind(SKY_KIND_ESHOT) >= SKY_MAX_ESHOTS) return;
    s = sky_spawn(SKY_KIND_ESHOT, SKY_SHOT_ORB, x, y, SKY_SCALE_PROJECTILE, NG_RENDER_BAND_FX);
    if (!s) return;
    ng_char_set_speed(s, vx, vy);
    ng_char_set_body(s, -3, -3, 6, 6);
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

static uint8_t NEOGEO_USER sky_spawn_enemy(uint8_t type, int16_t x, int16_t y, int16_t side)
{
    const SkyEnemy *e = &k_enemy[type % ENEMY_COUNT];
    NGCharacter *c;

    if (sky_count_kind(SKY_KIND_ENEMY) >= SKY_MAX_ENEMIES) return 0u;

    c = sky_spawn(SKY_KIND_ENEMY, e->asset, x, y, e->scale, NG_RENDER_BAND_ENEMY);
    if (!c) return 0u;

    c->hp     = (uint8_t)(e->hp + (s_stage >> 1));
    c->max_hp = c->hp;
    c->data0  = e->pattern;
    c->data1  = 0u;
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
    return 1u;
}

static void NEOGEO_USER sky_spawn_boss(void)
{
    const SkyStage *st = &k_stage[s_stage % SKY_STAGE_COUNT];
    NGCharacter *c;
    if (!st->has_boss) return;
    c = sky_spawn(SKY_KIND_BOSS, st->boss,
                  SKY_FIELD_X + SKY_FIELD_W / 2, -40,
                  SKY_SCALE_BOSS, NG_RENDER_BAND_ENEMY);
    if (!c) return;

    c->hp     = st->boss_hp;
    c->max_hp = st->boss_hp;
    s_boss_hp_max = st->boss_hp;
    c->data0  = 0u;    /* 0 = entering, 1 = engaged */
    c->data1  = 0u;    /* burst timer               */
    c->data2  = 0u;    /* volley phase              */
    ng_char_set_speed(c, 0, 1);
    s_boss_on = 1u;
    playSFX(SOUND_SFX_12); /* warning siren */
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
        /* Weave, but drift toward the player's column while doing it, so a
         * dive threatens where the player actually is instead of whichever
         * lane it happened to spawn in. */
        int32_t chase = (px > c->x) ? (NG_FP_ONE / 3)
                      : (px < c->x) ? -(NG_FP_ONE / 3) : 0;
        ng_char_set_speed_fp(c, (int32_t)sway * (NG_FP_ONE / 12) + chase,
                             c->vy_fp);
        break;
    }
    case PAT_STRAFE:
        /* Turn around at the field edge instead of flying off it. */
        if ((c->x <= SKY_FIELD_LEFT && c->vx_fp < 0) ||
            (c->x >= SKY_FIELD_RIGHT && c->vx_fp > 0)) {
            c->vx_fp = -c->vx_fp;
        }
        break;
    case PAT_HOLD:
        /* Descend to a firing line, then hold it while sliding after the
         * player, so sitting directly underneath one is no longer safe.
         *
         * The hold is deliberately not forever.  A holder that never leaves
         * also never stops counting as a live enemy, and the stage waits on
         * that count reaching zero before it sends in the boss - so a single
         * survivor parked on its line would stall the stage indefinitely. */
        if (c->y > SKY_FIELD_TOP + 40) {
            if (c->data1 < SKY_HOLD_FRAMES) {
                int32_t track = (px > c->x) ? (NG_FP_ONE / 2)
                              : (px < c->x) ? -(NG_FP_ONE / 2) : 0;
                ng_char_set_speed_fp(c, track, 0);
            } else {
                /* Time up: break station and fly out of the bottom. */
                ng_char_set_speed_fp(c, 0,
                                     ((int32_t)e->speed << NG_FP_SHIFT) / 16);
            }
        }
        break;
    case PAT_SURFACE:
        ng_char_set_speed(c, 0, 1);
        break;
    default:
        break;
    }

    if (e->fire_odds) {
        /* Fixed warm-up: no offscreen or point-blank surprise volleys. */
        uint16_t interval = (uint16_t)(e->fire_odds - s_stage * 6u);
        if (interval < 54u) interval = 54u;
        if (c->y >= SKY_FIELD_TOP + 12 && c->y < py - 16 &&
            (c->data1 % interval) == 0u) {
            sky_fire_aimed(c->x, (int16_t)(c->y + 8), px, py,
                           (int16_t)(2 + (s_stage >= 3u)));
        }
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
        if (c->y >= SKY_FIELD_TOP + 44) {
            c->data0 = 1u;
            ng_char_set_speed(c, 1, 0);
        }
        return;
    }

    if (c->x <= left)  ng_char_set_speed(c,  1, 0);
    if (c->x >= right) ng_char_set_speed(c, -1, 0);

    c->data1++;
    {
        uint16_t interval = (uint16_t)(108u - s_stage * 6u);
        int16_t muzzle = (int16_t)(c->y + 36);
        if (c->hp * 2u < c->max_hp) interval = (uint16_t)(interval * 3u / 4u);
        if (c->data1 >= interval) {
            c->data1 = 0u;
            c->data2++;
            switch (s_stage) {
            case 0u: /* Opening fan leaves wide escape lanes. */
                sky_spawn_eshot(c->x, muzzle, -2, 2);
                sky_spawn_eshot(c->x, muzzle, 0, 3);
                sky_spawn_eshot(c->x, muzzle, 2, 2);
                break;
            case 1u: /* Two naval guns converge on different points. */
                sky_fire_aimed(c->x - 28, muzzle, px - 16, py, 3);
                sky_fire_aimed(c->x + 28, muzzle, px + 16, py, 3);
                break;
            case 2u: /* The rotating core alternates the safe side. */
                sky_spawn_eshot(c->x, muzzle, (c->data2 & 1u) ? -3 : 3, 2);
                sky_spawn_eshot(c->x, muzzle, (c->data2 & 1u) ? -1 : 1, 3);
                break;
            case 3u:
                if (c->data2 & 1u) sky_fire_aimed(c->x, muzzle, px, py, 4);
                else {
                    sky_spawn_eshot(c->x - 24, muzzle, 0, 3);
                    sky_spawn_eshot(c->x + 24, muzzle, 0, 3);
                }
                break;
            case 4u: /* Carrier fan deliberately leaves the centre open. */
                sky_spawn_eshot(c->x - 16, muzzle, -2, 3);
                sky_spawn_eshot(c->x - 16, muzzle, -1, 3);
                sky_spawn_eshot(c->x + 16, muzzle, 1, 3);
                sky_spawn_eshot(c->x + 16, muzzle, 2, 3);
                break;
            case 5u:
                sky_fire_aimed(c->x + ((c->data2 & 1u) ? -24 : 24), muzzle, px, py, 4);
                sky_spawn_eshot(c->x, muzzle, 0, 2);
                break;
            default:
                if (c->data2 & 1u) {
                    int16_t d;
                    for (d = -2; d <= 2; d++) sky_spawn_eshot(c->x, muzzle, d, 3);
                } else {
                    sky_fire_aimed(c->x - 24, muzzle, px - 16, py, 3);
                    sky_fire_aimed(c->x + 24, muzzle, px + 16, py, 3);
                }
                break;
            }
        }
    }
}

static void NEOGEO_USER sky_blast_step(NGCharacter *c)
{
    /* Bound the expansion before converting to an 8-bit hardware scale.
     * Adding to full scale wrapped to zero and displaced the old anchor. */
    c->data0++;
    if (c->data0 >= SKY_BURST_STEPS * 3u) {
        ng_chars_remove(c);
        return;
    }
    if ((c->data0 % 3u) == 0u)
        sky_bind(c, SKY_SHOT_RING, SKY_BURST_SCALE(c->data0 / 3u), NG_RENDER_BAND_FX);
}

void NEOGEO_USER sky_stage_effects_tick(void)
{
    uint8_t i;
    for (i = 0u; i < NG_MAX_CHARS; i++) {
        NGCharacter *c = chars_at(i);
        if (c && c->active && c->kind == SKY_KIND_BLAST) sky_blast_step(c);
    }
}

/* ------------------------------------------------------------------ */
/*  Wave director                                                       */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER sky_release_next(void)
{
    int16_t x;

    if (!s_squad_left) return;
    if (s_squad_gap) { s_squad_gap--; return; }

    if (s_squad_type == 5u) {
        x = SKY_FIELD_X + 68; /* the valley road */
    } else if (s_squad_type == 4u) {
        x = (int16_t)(SKY_FIELD_X + 112 + (s_squad_left & 1u) * 72);
    } else if (k_enemy[s_squad_type % ENEMY_COUNT].pattern == PAT_STRAFE) {
        x = (int16_t)(s_squad_side > 0 ? SKY_FIELD_LEFT : SKY_FIELD_RIGHT);
    } else {
        /* Spread the squadron across the arena on a fixed lane so the
         * formation reads as a formation rather than as noise. */
        uint8_t lane = (uint8_t)(s_squad_left & 3u);
        x = (int16_t)(SKY_FIELD_LEFT + 24 + lane * ((SKY_FIELD_W - 72) / 3));
    }

    if (!sky_spawn_enemy(s_squad_type, x, (int16_t)(SKY_FIELD_TOP - 24),
                        (int16_t)(s_squad_side > 0 ? 1 : -1))) return;
    s_squad_left--;
    s_squad_gap = (uint16_t)((s_squad_type == 4u || s_squad_type == 5u) ? 48u : 24u);
}

static void NEOGEO_USER sky_start_squad(void)
{
    const SkyStage *st = &k_stage[s_stage];

    /* Every sortie includes its advertised surface and aircraft squadrons. */
    s_squad_type = st->roster[(st->waves - s_waves_left) & 3u];
    s_squad_side = (uint8_t)(sky_rand() & 1u);
    s_squad_left = (uint8_t)((s_squad_type == 4u || s_squad_type == 5u) ? 2u : 3u);
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
        case SKY_KIND_PICKUP:
            c->data1++;
            if (c->y > SKY_FIELD_BOTTOM + 20 || c->data1 > 480u) ng_chars_remove(c);
            break;
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
        if (!k_stage[s_stage % SKY_STAGE_COUNT].has_boss) {
            cleared = 1u;
            return cleared;
        }
        s_boss_wait = 0u;
        sky_spawn_boss();
    } else if (s_boss_wait >= SKY_BOSS_WAIT_MAX) {
        if (!k_stage[s_stage % SKY_STAGE_COUNT].has_boss) {
            cleared = 1u;
            return cleared;
        }
        s_boss_wait = 0u;
        sky_spawn_boss();
    } else {
        s_boss_wait++;
    }

    return 0u;
}
