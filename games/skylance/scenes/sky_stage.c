/*
 * sky_stage.c — Sky Lance enemy tables, wave director and boss behaviour.
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
#include "sky_combat.h"
#include "games/skylance/artbox/generated/sky_native_assets.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "sdk/2d_engine/ng_defs.h"

void NEOGEO_USER playSFX(uint8_t n);

/* ------------------------------------------------------------------ */
/*  Enemy roster                                                        */
/* ------------------------------------------------------------------ */
/* Flight patterns.  Stored per-enemy in NGCharacter.data0. */
#define PAT_DIVE      0u   /* straight down, weaving                      */
#define PAT_STRAFE    1u   /* crosses the screen, drifting down slowly    */
#define PAT_HOLD      2u   /* descends to a firing line and sits there    */
#define PAT_SURFACE   3u   /* terrain-bound vehicle, no airborne chase   */
#define PAT_SUBMARINE 4u   /* naval submarine: surfaces, fires, dives     */
#define PAT_MINIBOSS  5u
#define SKY_DEFENDER  0x8000u
/* The Crimson Keep's turret sockets, from the hull's pivot. */
#define SKY_KEEP_TURRET_X 27
#define SKY_KEEP_TURRET_Y 13

/* How long a PAT_HOLD enemy keeps its station before resuming the descent,
 * in frames.  Bounded so a stage cannot stall waiting for it to leave. */
#define SKY_HOLD_FRAMES  300u

/* Frames the director will wait for leftover enemies once the waves are spent
 * before sending the boss in regardless. */
#define SKY_BOSS_WAIT_MAX  420u

typedef struct {
    uint16_t asset;
    uint8_t hp;
    uint8_t scale;
    uint8_t pattern;
    uint8_t speed;        /* pixels per 16 frames along the main axis */
    uint8_t fire_odds;    /* 1-in-N per frame, 0 = never shoots        */
    uint16_t score;
} SkyEnemy;

#define ENEMY_COUNT 10u
static const SkyEnemy k_enemy[ENEMY_COUNT] = {
    /* asset,             hp, scale,           pattern,        spd, odds, score */
    { SKY_E_DRONE,         1u, SKY_SCALE_SMALL, PAT_DIVE,      32u,  60u,   50u },
    { SKY_E_FIGHTER,       2u, SKY_SCALE_ENEMY, PAT_DIVE,      24u,  45u,  100u },
    { SKY_E_INTERCEPTOR,   3u, SKY_SCALE_ENEMY, PAT_STRAFE,    28u,  40u,  150u },
    { SKY_E_HELI,          4u, SKY_SCALE_ENEMY, PAT_HOLD,      18u,  36u,  180u },
    { SKY_E_MISSILE_BOAT,  5u, SKY_SCALE_ENEMY, PAT_SURFACE,   16u,  38u,  200u },
    { SKY_E_TANK,          4u, SKY_SCALE_ENEMY, PAT_SURFACE,   16u,  34u,  120u },
    { SKY_E_BOMBER,        6u, SKY_SCALE_ENEMY, PAT_DIVE,      16u,  36u,  250u },
    { SKY_E_SAM,          10u, SKY_SCALE_ENEMY, PAT_SURFACE,    8u,  28u, 1000u },
    { SKY_E_RADAR_SITE,   12u, SKY_SCALE_ENEMY, PAT_SURFACE,    8u,  28u, 1000u },
    { SKY_E_SUB,           6u, SKY_SCALE_ENEMY, PAT_SUBMARINE, 12u,  26u,  400u }
};

/* ------------------------------------------------------------------ */
/*  Stage table                                                         */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t  bg;
    uint8_t  boss;
    uint16_t boss_hp;
    uint16_t boss_score;
    uint8_t  has_boss;       /* 1 if stage ends with boss battle */
    uint8_t  roster[4];      /* indices into k_enemy */
    uint8_t  waves;          /* squadrons before boss / stage clear */
    const char *boss_name;
    const char *subtitle;
    const char *objective;
    const char *intel;
    const char *tactic;
} SkyStage;

static const SkyStage k_stage[SKY_STAGE_COUNT] = {
    { SKY_BG_MOUNTAIN, SKY_B_RED_FORTRESS, 180u, 5000u, 1u, {0u,1u,5u,1u}, 24u,
      "CRIMSON KEEP", "01 / 07  MOUNTAIN DAWN",
      "DESTROY 3 AA RADAR SITES", "BOMBERS & VALLEY TANKS", "ROLL 45 TO EVADE FLAK" },
    { SKY_BG_COAST, SKY_B_BATTLESHIP, 220u, 6000u, 1u, {4u,9u,4u,6u}, 26u,
      "IRON TIDE", "02 / 07  HARBOR ASSAULT",
      "BREAK HARBOR BLOCKADE", "SURFACING SUBS & GUNBOATS", "DIVE STRIKE ON PERISCOPE" },
    { SKY_BG_CONVOY, SKY_B_GOLD_CORE, 260u, 6500u, 1u, {5u,2u,0u,6u}, 28u,
      "SOL CORE", "03 / 07  VALLEY CONVOY",
      "STOP ARMORED SUPPLY LINE", "ARMORED COLUMN & DRONES", "FLANK HIGHWAYS FROM ABOVE" },
    { SKY_BG_OPEN_SEA, SKY_B_STEALTH, 300u, 7000u, 1u, {2u,9u,1u,6u}, 30u,
      "NIGHT RAZOR", "04 / 07  REEF INTERCEPT",
      "INTERCEPT REEF WOLFPACK", "SUBS & STEALTH JET", "WATCH 3-WAY TORPEDO ARCS" },
    { SKY_BG_OUTPOST, SKY_B_HELI_CARRIER, 340u, 7500u, 1u, {3u,5u,3u,6u}, 32u,
      "ROTOR NEST", "05 / 07  FOREST OUTPOST",
      "RECOVER AIRBASE RELAY", "GUNSHIPS & CLOUD AMBUSH", "EVADE AMBUSH FROM CLOUDS" },
    { SKY_BG_ARMORED, SKY_B_TANK_FORT, 380u, 8000u, 1u, {5u,2u,5u,6u}, 34u,
      "EARTH HAMMER", "06 / 07  ARMORED COLUMN",
      "DISARM MOBILE FORTRESS", "HEAVY TANK BATTALIONS", "USE SUPER BOMB ON BASTION" },
    { SKY_BG_CITADEL, SKY_B_CASTLE, 450u, 10000u, 1u, {5u,2u,3u,6u}, 36u,
      "CRIMSON CITADEL", "07 / 07  FINAL APPROACH",
      "SILENCE CITADEL ARRAY", "CITADEL AA MISSILE SILOS", "FOCUS MAIN FIRE ON CORE" }
};

/* ------------------------------------------------------------------ */
/*  Director state                                                      */
/* ------------------------------------------------------------------ */
static uint8_t  s_stage;
static uint8_t  s_fire_art;
static uint8_t  s_fire_boss;       /* the shots now being fired are a boss's */
static uint8_t  s_difficulty;      /* the operator's setting: 0 easy .. 3 expert */
static uint8_t  s_waves_left;
static uint16_t s_wave_timer;
static uint8_t  s_squad_left;      /* enemies still to release this squadron */
static uint8_t  s_squad_type;
static uint8_t  s_squad_side;
static uint16_t s_squad_gap;
static uint8_t  s_boss_on;
static uint16_t s_boss_wait;
static uint16_t s_boss_hp_max;
static uint16_t s_boss_health;
static uint16_t s_kills;
static uint16_t s_tick;
static uint8_t s_targets_destroyed;
static uint8_t s_target_alert;
static uint8_t s_encounter, s_checkpoint, s_guards_to_spawn, s_reinforcements;
static uint8_t s_radar_alert, s_chaff_stock;
static uint16_t s_chaff_time, s_guard_timer;
static volatile uint16_t s_decoyed_missiles;
static uint8_t s_seekers, s_seekers_counting;   /* guided missiles still on the jet */
static uint8_t s_cruise;          /* waves spent: flying on to the boss's arena */
#define SKY_BLAST_QUEUE 10u
static struct { int16_t x, y; uint8_t wait; } s_blast_queue[SKY_BLAST_QUEUE];
static void NEOGEO_USER sky_blast_chain(int16_t x, int16_t y, uint8_t spread, uint8_t count);
static int16_t s_decoy_x, s_decoy_y;
static NGCharacter *s_checkpoint_target, *s_radar_zone;

uint8_t NEOGEO_USER sky_stage_paused(void) { return (uint8_t)(s_encounter != 0u || s_boss_on); }
uint8_t NEOGEO_USER sky_stage_cruising(void) { return (uint8_t)(s_cruise && !sky_stage_paused()); }
uint8_t NEOGEO_USER sky_stage_encounter(void) { return s_encounter; }
uint8_t NEOGEO_USER sky_stage_radar_alert(void) { return s_radar_alert; }
uint8_t NEOGEO_USER sky_stage_chaff_stock(void) { return s_chaff_stock; }
uint8_t NEOGEO_USER sky_stage_seekers(void) { return s_seekers; }

/* NORMAL unless an arcade operator chose otherwise; a console keeps it. */
static uint8_t NEOGEO_USER sky_dip_difficulty(void)
{
    uint8_t o = ng_dip_option(2);
    if (*(const volatile uint8_t *)BIOS_GAME_DIP != 0xFFu) return 1u;
    return o < 4u ? o : 1u;
}

/*
 * Chaff against guided missiles: the chance, in percent, that one release
 * pulls a missile off the jet. Every missile is rolled on its own, so a
 * release can leave some still locked. The first routes field old seekers
 * that fall for it; each route is a little better, from route 4 on they
 * are the newer kind, and a boss's own missiles see through most of it
 * unless the operator chose EASY. Stored in the missile's data2 low byte.
 */
#define SKY_SEEKER_ART       7u
#define SKY_SEEKER_FOOLED    0x100u        /* data2: this release pulled it */
#define SKY_SEEKER_NEWER     3u            /* route index of the newer seekers */

static uint8_t NEOGEO_USER sky_seeker_odds(void)
{
    static const uint8_t base[4]  = {92u, 80u, 68u, 56u};
    static const uint8_t route[4] = { 2u,  3u,  4u,  5u};
    static const uint8_t newer[4] = { 5u, 15u, 20u, 25u};
    static const uint8_t boss[4]  = {85u, 45u, 35u, 25u};
    uint8_t d = (uint8_t)(s_difficulty & 3u);
    int16_t odds;

    if (s_fire_boss) odds = (int16_t)(boss[d] - s_stage);
    else {
        odds = (int16_t)(base[d] - route[d] * s_stage);
        if (s_stage >= SKY_SEEKER_NEWER) odds = (int16_t)(odds - newer[d]);
    }
    return (uint8_t)(odds < 15 ? 15 : odds);
}

/* One roll against odds in percent, from the generator's better high bits. */
static uint8_t NEOGEO_USER sky_seeker_fooled(uint16_t odds)
{
    return (uint8_t)(((uint16_t)((sky_rand() >> 8) * 100u) >> 8) < (odds & 0xFFu));
}

/*
 * Release chaff at (x, y). Returns 0 when none could go out, 1 when every
 * guided missile in the air took the decoy, 2 when one or more stayed on
 * the jet.
 */
uint8_t NEOGEO_USER sky_stage_chaff(int16_t x, int16_t y)
{
    NGCharacter *c;
    uint8_t locked = 0u;
    if (!s_chaff_stock || s_chaff_time) return 0u;
    c = sky_spawn(SKY_KIND_CHAFF, SKY_CHAFF_ART, x, y, SKY_SCALE_FULL, NG_RENDER_BAND_FX);
    if (!c) return 0u;
    s_chaff_stock--;
    s_chaff_time = 120u;
    s_decoy_x = x;
    s_decoy_y = y;
    c->data0 = 120u;
    ng_char_set_speed(c, 0, 0);
    playSFX(SOUND_SFX_8);
    for (NGCharacter *m = chars_at(0u), *end = m + ng_chars_slots_used(); m < end; m++) {
        if (!m->active || m->kind != SKY_KIND_ESHOT || m->data0 != SKY_SEEKER_ART) continue;
        m->data2 &= 0xFFu;
        if (sky_seeker_fooled(m->data2)) m->data2 |= SKY_SEEKER_FOOLED;
        else locked = 1u;
    }
    return (uint8_t)(1u + locked);
}

uint8_t NEOGEO_USER sky_stage_targets(void) { return s_targets_destroyed; }
uint8_t NEOGEO_USER sky_stage_target_alert(void)
{
    uint8_t a = s_target_alert;
    s_target_alert = 0u;
    return a;
}

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

const char * NEOGEO_USER sky_stage_objective(uint8_t stage)
{
    return k_stage[stage % SKY_STAGE_COUNT].objective;
}

const char * NEOGEO_USER sky_stage_intel(uint8_t stage)
{
    return k_stage[stage % SKY_STAGE_COUNT].intel;
}

const char * NEOGEO_USER sky_stage_tactic(uint8_t stage)
{
    return k_stage[stage % SKY_STAGE_COUNT].tactic;
}

uint8_t NEOGEO_USER sky_stage_boss_active(void) { return s_boss_on; }
uint16_t NEOGEO_USER sky_stage_kills(void)      { return s_kills; }

uint16_t NEOGEO_USER sky_stage_defeated(const NGCharacter *c)
{
    if (!c) return 0u;
    if (c->kind == SKY_KIND_MODULE) {
        sky_blast_chain(c->x, c->y, 10u, 2u);
        return 200u;
    }
    if (c->kind == SKY_KIND_BOSS) {
        sky_blast_chain(c->x, c->y, 40u, SKY_BLAST_QUEUE);
        return k_stage[s_stage].boss_score;
    }
    if (c->kind != SKY_KIND_ENEMY) return 0u;
    s_kills++;
    if (c->data0 == PAT_MINIBOSS) {
        sky_blast_chain(c->x, c->y, 24u, 4u);
        return (uint16_t)(1500u + s_stage * 250u);
    }
    if (sky_is_command_target((uint8_t)c->data2)) sky_blast_chain(c->x, c->y, 16u, 3u);
    if (sky_is_command_target((uint8_t)c->data2) && s_targets_destroyed < 3u) s_targets_destroyed++;
    if (c == s_checkpoint_target) {
        s_checkpoint_target = 0;
        if (s_radar_zone) ng_chars_remove(s_radar_zone);
        s_radar_zone = 0;
        s_encounter = s_radar_alert = 0u;
        s_checkpoint++;
        s_target_alert = 3u;
    }
    return k_enemy[c->data2 % ENEMY_COUNT].score;
}

void NEOGEO_USER sky_stage_damage(NGCharacter *c, uint16_t damage)
{
    if (!c) return;
    if (c == s_checkpoint_target && s_encounter != 3u) return;
    if (c->kind == SKY_KIND_BOSS) {
        s_boss_health = sky_hull_damage(s_boss_health, damage);
        /* The shared character ABI keeps an 8-bit normalized hull value.
         * Sky Lance owns the boss's exact health and damage arithmetic. */
        c->hp = s_boss_health ? (uint8_t)(((uint32_t)s_boss_health * 255u +
                            s_boss_hp_max - 1u) / s_boss_hp_max) : 0u;
    } else ng_char_damage(c, (uint8_t)(damage > 255u ? 255u : damage));
}

void NEOGEO_USER sky_stage_begin(uint8_t stage)
{
    const SkyStage *st = &k_stage[stage % SKY_STAGE_COUNT];

    s_stage       = (uint8_t)(stage % SKY_STAGE_COUNT);
    s_targets_destroyed = 0u;
    s_waves_left  = st->waves;
    s_wave_timer  = 240u;
    s_squad_left  = 0u;
    s_squad_gap   = 0u;
    s_boss_on     = 0u;
    s_boss_wait   = 0u;
    s_boss_hp_max = 1u;
    s_boss_health = 0u;
    s_kills       = 0u;
    s_tick        = 0u;
    s_target_alert = 0u;
    s_encounter = s_checkpoint = s_guards_to_spawn = s_reinforcements = 0u;
    s_radar_alert = 0u;
    s_chaff_stock = 6u;
    s_chaff_time = s_guard_timer = 0u;
    for (uint8_t q = 0u; q < SKY_BLAST_QUEUE; q++) s_blast_queue[q].wait = 0u;
    s_cruise = 0u;
    s_difficulty = sky_dip_difficulty();
    s_decoyed_missiles = 0u;
    s_checkpoint_target = s_radar_zone = 0;
}

/*
 * Count live objects of one kind.
 *
 * This walks every slot up to ng_chars_slots_used(), the highest one in
 * use, on purpose.  ng_chars_count() returns how many slots are ACTIVE, so
 * `for (i = 0; i < ng_chars_count(); i++) chars_at(i)` silently skips
 * the tail of the array as soon as any slot in the middle is freed.
 *
 * Every slot walk steps a pointer from chars_at(0): chars_at() per slot
 * costs a call and a multiply by the 108-byte record, and the frame walks
 * the slots a dozen times.
 */
uint8_t NEOGEO_USER sky_count_kind(uint8_t kind)
{
    uint8_t total = 0u;
    for (NGCharacter *c = chars_at(0u), *end = c + ng_chars_slots_used(); c < end; c++) {
        if (c && c->active && c->kind == kind) total++;
    }
    return total;
}

/* ------------------------------------------------------------------ */
/*  Spawns                                                              */
/* ------------------------------------------------------------------ */
/*
 * The explosion: flash, fireball, break-up and smoke, each frame held a
 * little longer than the one before. The painted set has eight frames when
 * the art build brings the last two; the first six keep their old IDs.
 */
#ifdef SKY_EXPLOSION_7
#define SKY_BLAST_FRAMES 8u
static const uint8_t k_blast_end[SKY_BLAST_FRAMES] = {2u, 4u, 7u, 10u, 14u, 18u, 23u, 29u};
#else
#define SKY_BLAST_FRAMES 6u
static const uint8_t k_blast_end[SKY_BLAST_FRAMES] = {2u, 5u, 8u, 12u, 17u, 23u};
#endif

static uint16_t NEOGEO_USER sky_blast_art(uint8_t k)
{
#ifdef SKY_EXPLOSION_7
    if (k >= 6u) return (uint16_t)(SKY_EXPLOSION_6 + (k - 6u));
#endif
    return (uint16_t)(SKY_EXPLOSION_FIRST + k);
}

NGCharacter * NEOGEO_USER sky_spawn_blast(int16_t x, int16_t y)
{
    NGCharacter *c;

    if (sky_count_kind(SKY_KIND_BLAST) >= SKY_MAX_BLASTS) return 0;
    c = sky_spawn(SKY_KIND_BLAST, SKY_EXPLOSION_FIRST, x, y, SKY_SCALE_FULL, NG_RENDER_BAND_FX);
    if (c) c->data0 = c->data1 = 0u;
    return c;
}

/*
 * Blasts still to come: a big kill goes up in a chain across its hull, one
 * blast every few frames, rather than all at once.
 */
static void NEOGEO_USER sky_blast_chain(int16_t x, int16_t y, uint8_t spread, uint8_t count)
{
    uint8_t i, k = 0u;
    uint16_t span = (uint16_t)(spread * 2u + 1u);
    for (i = 0u; i < SKY_BLAST_QUEUE && k < count; i++) {
        if (s_blast_queue[i].wait) continue;
        s_blast_queue[i].x = (int16_t)(x + (int16_t)((sky_rand() >> 8) % span) - (int16_t)spread);
        s_blast_queue[i].y = (int16_t)(y + (int16_t)((sky_rand() >> 8) % span) - (int16_t)spread);
        s_blast_queue[i].wait = (uint8_t)(5u + k * 7u);
        k++;
    }
}

static void NEOGEO_USER sky_blast_queue_step(void)
{
    uint8_t i;
    for (i = 0u; i < SKY_BLAST_QUEUE; i++) {
        if (!s_blast_queue[i].wait || --s_blast_queue[i].wait) continue;
        if (!sky_spawn_blast(s_blast_queue[i].x, s_blast_queue[i].y))
            s_blast_queue[i].wait = 1u;        /* four already burning: next frame */
    }
}

static void NEOGEO_USER sky_spawn_eshot(int16_t x, int16_t y, int16_t vx, int16_t vy)
{
    NGCharacter *s;

    if (sky_count_kind(SKY_KIND_ESHOT) >= SKY_MAX_ESHOTS) return;
    s = sky_spawn(SKY_KIND_ESHOT, (uint8_t)(SKY_HOSTILE_FIRST + s_fire_art),
                  x, y, SKY_SCALE_FULL, NG_RENDER_BAND_FRONT);
    if (!s) return;
    ng_char_set_speed(s, vx, vy);
    ng_char_set_body(s, -3, -3, 6, 6);
    s->data0 = s_fire_art;
    s->data1 = 0u;
    s->data2 = s_fire_art == SKY_SEEKER_ART ? sky_seeker_odds() : 0u;
    if (s->data2 && s_chaff_time && sky_seeker_fooled(s->data2)) s->data2 |= SKY_SEEKER_FOOLED;
    s->flip_y = (uint8_t)(vy > 0);
}

static void NEOGEO_USER sky_spawn_ering(int16_t x, int16_t y, int16_t vx, int16_t vy)
{
    NGCharacter *s;

    if (sky_count_kind(SKY_KIND_ESHOT) >= SKY_MAX_ESHOTS) return;
    s = sky_spawn(SKY_KIND_ESHOT, (uint8_t)(SKY_HOSTILE_FIRST + s_fire_art),
                  x, y, SKY_SCALE_FULL, NG_RENDER_BAND_FRONT);
    if (!s) return;
    ng_char_set_speed(s, vx, vy);
    ng_char_set_body(s, -5, -5, 10, 10);
    s->data0 = s_fire_art;
    s->data1 = 0u;
    s->data2 = s_fire_art == SKY_SEEKER_ART ? sky_seeker_odds() : 0u;
    if (s->data2 && s_chaff_time && sky_seeker_fooled(s->data2)) s->data2 |= SKY_SEEKER_FOOLED;
    s->flip_y = (uint8_t)(vy > 0);
}

NGCharacter * NEOGEO_USER sky_spawn_smoke(int16_t x, int16_t y)
{
    NGCharacter *c;
    if (sky_count_kind(SKY_KIND_SMOKE) >= 2u) return 0;
    c = sky_spawn(SKY_KIND_SMOKE, SKY_SMOKE_FIRST, x, y,
                  SKY_SCALE_FULL, NG_RENDER_BAND_FX);
    if (c) {
        c->data0 = 0u;
        ng_char_set_speed(c, 0, 1);
    }
    return c;
}

static void NEOGEO_USER sky_smoke_step(NGCharacter *c)
{
    if (++c->data0 >= 16u) {
        ng_chars_remove(c);
        return;
    }
    if ((c->data0 & 3u) == 0u)
        sky_bind(c, SKY_SMOKE_FIRST + c->data0 / 4u,
                 SKY_SCALE_FULL, NG_RENDER_BAND_FX);
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

    if(e->pattern==PAT_SURFACE || e->pattern==PAT_SUBMARINE)x-=sky_bg_camera_x();
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
    /* The timeout path can leave holding enemies alive. Finish their
     * ownership before the seven-strip boss enters the scanline budget. */
    for (NGCharacter *e = chars_at(0u), *end = e + ng_chars_slots_used(); e < end; e++) {
        if (e->active && (e->kind == SKY_KIND_ENEMY || e->kind == SKY_KIND_ESHOT))
            ng_chars_remove(e);
    }
    c = sky_spawn(SKY_KIND_BOSS, st->boss,
                  SKY_FIELD_X + SKY_FIELD_W / 2, -40,
                  SKY_SCALE_BOSS, NG_RENDER_BAND_ENEMY);
    if (!c) return;

    c->hp     = 255u;
    c->max_hp = 255u;
    s_boss_hp_max = st->boss_hp;
    s_boss_health = st->boss_hp;
    c->data0  = 0u;    /* 0 = entering, 1 = engaged */
    c->data1  = 0u;    /* burst timer               */
    c->data2  = 0u;    /* volley phase              */
    ng_char_set_speed(c, 0, 1);
    s_boss_on = 1u;
    s_cruise = 0u;
    if(s_stage==0u) {
        /* Shots hit the drawn orange core or its independently owned
         * turrets, rather than a backing rectangle hiding every module. */
        ng_char_set_body(c,-9,0,18,17);
        uint8_t part;
        for(part=0;part<2u;part++) {
            NGCharacter *m=sky_spawn(SKY_KIND_MODULE,SKY_NATIVE_KEEP_TURRET,
                     (int16_t)(c->x+(part?SKY_KEEP_TURRET_X:-SKY_KEEP_TURRET_X)),(int16_t)(c->y+SKY_KEEP_TURRET_Y),SKY_SCALE_FULL,NG_RENDER_BAND_ENEMY);
            if(m) {m->hp=m->max_hp=18u;m->data0=part;ng_char_set_priority(m,NG_RENDER_BAND_ENEMY,1);ng_char_set_body(m,-10,-10,20,20);}
        }
    }
    playSFX(SOUND_SFX_12); /* warning siren */
}

uint8_t NEOGEO_USER sky_stage_boss_bar(void)
{

    for (NGCharacter *c = chars_at(0u), *end = c + ng_chars_slots_used(); c < end; c++) {
        if (c && c->active && c->kind == SKY_KIND_BOSS) {
            if (!s_boss_hp_max) return 0u;
            return (uint8_t)(((uint32_t)s_boss_health * 24u + s_boss_hp_max - 1u) / s_boss_hp_max);
        }
        if (c && c->active && c->kind == SKY_KIND_ENEMY && c->data0 == PAT_MINIBOSS)
            return (uint8_t)(((uint16_t)c->hp * 24u + c->max_hp-1u) / c->max_hp);
    }
    return 0u;
}

/* ------------------------------------------------------------------ */
/*  Per-enemy behaviour                                                 */
/* ------------------------------------------------------------------ */
/* The harbour and open-sea stages: their command targets are warships and
 * their surface craft keep to open water. */
static uint8_t NEOGEO_USER sky_stage_naval(void)
{
    return (uint8_t)(k_stage[s_stage].bg == SKY_BG_COAST || k_stage[s_stage].bg == SKY_BG_OPEN_SEA);
}

/* Open water under a hull 24 pixels wide, every 16 rows from screen y0 to y1. */
static uint8_t NEOGEO_USER sky_water_run(int16_t x, int16_t y0, int16_t y1)
{
    int16_t y;
    for (y = y0; y <= y1; y = (int16_t)(y + 16))
        if (!sky_bg_water((int16_t)(x - 12), y) || !sky_bg_water((int16_t)(x + 12), y)) return 0u;
    return 1u;
}

/* World x for a hull that must stay on water from screen y0 down to the
 * bottom of the field: the wanted lane, else the nearest open one, else -1.
 * Craft ride the terrain, and are sent on down their column to clear the
 * field for an encounter, so the whole column below them must be water. */
static int16_t NEOGEO_USER sky_sea_lane(int16_t want, int16_t y0)
{
    static const int16_t lanes[] = {64, 176, 120, 32, 232, 96, 152, 208, 248};
    int16_t cam = sky_bg_camera_x(), x;
    uint8_t i;
    for (i = 0u; i <= sizeof(lanes) / sizeof(lanes[0]); i++) {
        x = i ? lanes[i - 1u] : want;
        if (sky_water_run((int16_t)(x - cam), y0, SKY_FIELD_BOTTOM)) return x;
    }
    return -1;
}

/* Frame 1 of each enemy's native strip: frames 1-2 idle, 3 damaged,
 * 4-7 destruction. 0 = the enemy has no strip yet. */
static const uint16_t k_enemy_frames[ENEMY_COUNT] = {
    SKY_DRONE_1, SKY_FIGHTER_1, SKY_INTERCEPTOR_1, SKY_HELI_1, 0u,
    SKY_TANK_1, SKY_BOMBER_1, SKY_SAM_1, SKY_RADAR_SITE_1, 0u
};

static uint16_t NEOGEO_USER sky_enemy_frames(const NGCharacter *c)
{
    if (c->data0 == PAT_MINIBOSS) {
        /* The three minibosses take turns, stage by stage. */
        static const uint16_t mini[3] = {SKY_MINIBOSS_0_1, SKY_MINIBOSS_1_1, SKY_MINIBOSS_2_1};
        return mini[s_stage % 3u];
    }
    if ((c->data2 == 7u || c->data2 == 8u) && sky_stage_naval())
        return c->data2 == 7u ? SKY_NAVAL_AA_1 : SKY_NAVAL_RADAR_1;
    return k_enemy_frames[c->data2 % ENEMY_COUNT];
}

#define SKY_WRECK_HOLD 6u

uint8_t NEOGEO_USER sky_stage_wreck(NGCharacter *c)
{
    uint16_t first;
    if (!c) return 0u;
    if (c->kind == SKY_KIND_BOSS) {
        /* The Crimson Keep tears apart and burns out under its blasts. */
        if (s_stage != 0u) return 0u;
        ng_char_set_speed(c, 0, 1);
        c->kind  = SKY_KIND_WRECK;
        c->data0 = 0u;
        c->data1 = (uint16_t)(SKY_RED_FORTRESS_1 + 3u);
        sky_bind(c, c->data1, SKY_SCALE_BOSS, NG_RENDER_BAND_ENEMY);
        return 1u;
    }
    if (c->kind != SKY_KIND_ENEMY) return 0u;
    first = sky_enemy_frames(c);
    if (!first) return 0u;
    /* Aircraft lose way and drop behind; ground units stay on the ground. */
    if (c->data0 != PAT_SURFACE)
        ng_char_set_speed_fp(c, c->vx_fp / 2, NG_FP_ONE / 2);
    c->kind  = SKY_KIND_WRECK;
    c->data0 = 0u;
    c->data1 = (uint16_t)(first + 3u);
    sky_bind(c, c->data1, SKY_SCALE_FULL, NG_RENDER_BAND_ENEMY);
    return 1u;
}

static void NEOGEO_USER sky_wreck_step(NGCharacter *c)
{
    if (++c->data0 >= SKY_WRECK_HOLD * 4u) {
        ng_chars_remove(c);
        return;
    }
    if (!(c->data0 % SKY_WRECK_HOLD))
        sky_bind(c, (uint16_t)(c->data1 + c->data0 / SKY_WRECK_HOLD),
                 SKY_SCALE_FULL, NG_RENDER_BAND_ENEMY);
}

static void NEOGEO_USER sky_enemy_step(NGCharacter *c, int16_t px, int16_t py)
{
    const SkyEnemy *e = &k_enemy[c->data2 % ENEMY_COUNT];
    static const uint8_t weapons[10] = {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 2u, 4u};
    s_fire_art = weapons[c->data2 % ENEMY_COUNT];
    s_fire_boss = 0u;

    c->data1++;
    {
        uint16_t first=sky_enemy_frames(c);
        if(first && !(c->data1&7u)) {
            uint8_t phase=(uint8_t)((c->data1>>3)%3u);
            /* Frame 0 is the legacy drawing's ID, except for the warships,
             * whose whole strip is new and runs on from it. */
            uint16_t base=(c->data2==7u||c->data2==8u)&&sky_stage_naval()?(uint16_t)(first-1u):e->asset;
            uint16_t id=phase?(uint16_t)(first+phase-1u):
                           (c->data0==PAT_MINIBOSS?(uint16_t)(SKY_MINIBOSS_FIRST+s_stage%3u):base);
            if((uint16_t)c->hp*3u<=c->max_hp)id=first+2u;
            sky_bind(c,id,SKY_SCALE_FULL,NG_RENDER_BAND_ENEMY);
        }
    }

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
            if ((c->flags & SKY_DEFENDER) || c->data1 < SKY_HOLD_FRAMES) {
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
        /* Ground sites stay attached to the quarter-pixel terrain scroll. */
        ng_char_set_speed_fp(c, 0, s_encounter == 1u ? NG_FP_ONE :
                                   sky_stage_paused() ? 0 : s_stage==0u?NG_FP_ONE / 2:NG_FP_ONE / 4);
        break;
    case PAT_MINIBOSS:
        if (c->y >= 62) {
            int16_t vx = (c->data1 & 127u) < 64u ? 1 : -1;
            int16_t lo = 48, hi = 240;
            /* Guarding a command target, it patrols the far side and never
             * covers it. */
            if (s_checkpoint_target) {
                if (s_checkpoint_target->x < 144) lo = (int16_t)(s_checkpoint_target->x + 88);
                else hi = (int16_t)(s_checkpoint_target->x - 88);
            }
            if (c->x < lo) vx = 1;
            if (c->x > hi) vx = -1;
            ng_char_set_speed(c, vx, 0);
        }
        if (!(c->data1 % (s_radar_alert ? 32u : 64u))) {
            s_fire_art = (uint8_t)(3u + s_stage % 3u);
            s_fire_boss = 0u;
            sky_fire_aimed(c->x-12, c->y+14, px-12, py, 3);
            sky_fire_aimed(c->x+12, c->y+14, px+12, py, 3);
            sky_spawn_eshot(c->x, c->y+16, 0, 4);
        }
        return;
    case PAT_SUBMARINE:
        /* Surfacing submarine: emerges from ocean depths, leaves water wakes,
         * fires aimed torpedo salvo, then submerges. */
        if (c->data1 < 36u) {
            ng_char_set_speed(c, 0, 1);
        } else if (c->data1 < 140u) {
            ng_char_set_speed(c, 0, 1);
            if (c->data1 == 45u) {
                sky_fire_aimed(c->x, (int16_t)(c->y + 10), px, py, 3);
            }
        } else {
            ng_char_set_speed(c, 0, 2);
        }
        break;
    default:
        break;
    }

    /* Dedicated Anti-Air command targets (AA Radar Sites & Missile Corvettes) */
    if (c->data2 >= 7u && c->data2 <= 8u) {
        if (c->y >= SKY_FIELD_TOP + 8 && c->y <= SKY_FIELD_BOTTOM - 16 &&
            (c->data1 % (s_radar_alert ? 48u : 96u)) == 32u && s_encounter != 3u) {
            sky_fire_aimed(c->x, (int16_t)(c->y + 6), px, py, 2);
            playSFX(SOUND_SFX_7);
        }
    } else if (e->fire_odds) {
        /* Paced arcade bullet firing tailored to each specific enemy unit */
        if (c->y >= SKY_FIELD_TOP + 12 && c->y <= SKY_FIELD_BOTTOM - 20) {
            switch (c->data2) {
            case 0u: /* Drone: High-speed dive; single needle dart at frame 20 */
                if ((c->data1 % 60u) == 20u) {
                    sky_spawn_eshot(c->x, (int16_t)(c->y + 8), 0, 4);
                }
                break;
            case 1u: /* Fighter: Rapid aimed 20mm tracer at frame 26 */
                if ((c->data1 % 72u) == 26u) {
                    sky_fire_aimed(c->x, (int16_t)(c->y + 8), px, py, 3);
                }
                break;
            case 2u: /* Interceptor: 2-way flanking burst with safe center gap at frame 24 */
                if ((c->data1 % 80u) == 24u) {
                    sky_spawn_eshot((int16_t)(c->x - 10), (int16_t)(c->y + 8), -1, 3);
                    sky_spawn_eshot((int16_t)(c->x + 10), (int16_t)(c->y + 8),  1, 3);
                }
                break;
            case 3u: /* Heli: Twin rocket volley at frame 36 */
                if ((c->data1 % 96u) == 36u) {
                    sky_spawn_eshot((int16_t)(c->x - 6), (int16_t)(c->y + 8), 0, 3);
                    sky_spawn_eshot((int16_t)(c->x + 6), (int16_t)(c->y + 8), 0, 3);
                }
                break;
            case 4u: /* Missile Boat: Cruise missile at frame 40 */
                if ((c->data1 % 100u) == 40u) {
                    sky_spawn_eshot(c->x, (int16_t)(c->y + 8), 0, 2);
                }
                break;
            case 5u: /* Tank: Heavy flak shell (ring) at frame 44 */
                if ((c->data1 % 112u) == 44u) {
                    sky_spawn_ering(c->x, (int16_t)(c->y + 8), 0, 2);
                }
                break;
            case 6u: /* Bomber: Carpet bomb expanding ring at frame 50 */
                if ((c->data1 % 100u) == 50u) {
                    sky_spawn_ering(c->x, (int16_t)(c->y + 10), 0, 2);
                }
                break;
            default:
                if (c->data1 == 30u) {
                    sky_fire_aimed(c->x, (int16_t)(c->y + 8), px, py, 3);
                }
                break;
            }
        }
    }

    /* Off the bottom: gone, no explosion, no score. */
    if (c->y > SKY_FIELD_BOTTOM + 40) ng_chars_remove(c);
}

/* Bit 0: the Crimson Keep's left turret still stands; bit 1: the right. */
static uint8_t NEOGEO_USER sky_keep_guns(void)
{
    uint8_t guns = 0u;
    for (NGCharacter *m = chars_at(0u), *end = m + ng_chars_slots_used(); m < end; m++) {
        if (m && m->active && m->kind == SKY_KIND_MODULE && m->hp)
            guns |= (uint8_t)(1u << (m->data0 & 1u));
    }
    return guns;
}

/* Side-to-side pace across the arena; the warship steams at half speed. */
static int32_t NEOGEO_USER sky_boss_pace(void)
{
    return s_stage == 1u ? NG_FP_ONE / 2 : NG_FP_ONE;
}

static void NEOGEO_USER sky_boss_step(NGCharacter *c, int16_t px, int16_t py)
{
    const int16_t left  = SKY_FIELD_X + 56;
    const int16_t right = SKY_FIELD_X + SKY_FIELD_W - 56;
    static const uint8_t weapons[7] = {7u, 4u, 2u, 6u, 3u, 5u, 7u};
    s_fire_art = weapons[s_stage];
    s_fire_boss = 1u;
    if(s_stage==0u && !(s_tick&15u)) {sky_bind(c,
          (uint16_t)(c->hp<85u?SKY_RED_FORTRESS_3:SKY_RED_FORTRESS_1+((s_tick>>4)&1u)),
          SKY_SCALE_BOSS,NG_RENDER_BAND_ENEMY);
        ng_char_set_body(c,-9,0,18,17);
    }

    if (c->data0 == 0u) {
        /* Entry: slide down to the top of the arena, then engage. */
        if (c->y >= SKY_FIELD_TOP + 44) {
            c->data0 = 1u;
            ng_char_set_speed_fp(c, sky_boss_pace(), 0);
        }
        return;
    }

    if (c->x <= left)  ng_char_set_speed_fp(c,  sky_boss_pace(), 0);
    if (c->x >= right) ng_char_set_speed_fp(c, -sky_boss_pace(), 0);

    c->data1++;
    {
        uint16_t interval = (uint16_t)(44u - s_stage * 3u);
        int16_t muzzle = (int16_t)(c->y + 36);
        if (interval < 22u) interval = 22u;
        if (c->hp * 2u < c->max_hp) interval = (uint16_t)(interval * 2u / 3u);
        if (c->data1 >= interval) {
            c->data1 = 0u;
            c->data2++;
            switch (s_stage) {
            case 0u: { /* Each side turret fires its own pair, leaving a lane down
                        * the middle; a destroyed turret falls silent. With both
                        * gone the core's gun fires alone; enraged it adds an aimed
                        * shot and ring flak. */
                uint8_t guns = sky_keep_guns();
                int16_t barrel = (int16_t)(c->y + SKY_KEEP_TURRET_Y + 14);
                if (guns & 1u) {
                    sky_spawn_eshot((int16_t)(c->x - SKY_KEEP_TURRET_X), barrel, -2, 2);
                    sky_spawn_eshot((int16_t)(c->x - SKY_KEEP_TURRET_X), barrel, -1, 3);
                }
                if (guns & 2u) {
                    sky_spawn_eshot((int16_t)(c->x + SKY_KEEP_TURRET_X), barrel, 1, 3);
                    sky_spawn_eshot((int16_t)(c->x + SKY_KEEP_TURRET_X), barrel, 2, 2);
                }
                if (!guns) sky_spawn_eshot(c->x, muzzle, 0, 3);
                if (c->hp * 2u < c->max_hp) {
                    sky_fire_aimed(c->x, muzzle, px, py, 4);
                    sky_spawn_ering(c->x, muzzle, 0, 2);
                }
                break;
            }
            case 1u: /* Two naval guns converge; enraged fires heavy center flak ring */
                sky_fire_aimed(c->x - 28, muzzle, px - 16, py, 3);
                sky_fire_aimed(c->x + 28, muzzle, px + 16, py, 3);
                if (c->hp * 2u < c->max_hp) {
                    sky_spawn_ering(c->x, muzzle, 0, 3);
                    sky_spawn_eshot(c->x, muzzle, 0, 4);
                }
                break;
            case 2u: /* Rotating core alternates sides; enraged fires dual expanding rings */
                sky_spawn_eshot(c->x, muzzle, (c->data2 & 1u) ? -3 : 3, 2);
                sky_spawn_eshot(c->x, muzzle, (c->data2 & 1u) ? -1 : 1, 3);
                if (c->hp * 2u < c->max_hp) {
                    sky_spawn_ering(c->x, muzzle, (c->data2 & 1u) ? -2 : 2, 2);
                    sky_fire_aimed(c->x, muzzle, px, py, 4);
                }
                break;
            case 3u: /* Stealth bomber: aimed bursts and carpet sweeps */
                if (c->data2 & 1u) {
                    sky_fire_aimed(c->x, muzzle, px, py, 4);
                    if (c->hp * 2u < c->max_hp) sky_spawn_ering(c->x, muzzle, 0, 3);
                } else {
                    sky_spawn_eshot(c->x - 24, muzzle, 0, 3);
                    sky_spawn_eshot(c->x + 24, muzzle, 0, 3);
                    if (c->hp * 2u < c->max_hp) {
                        sky_spawn_ering(c->x - 24, muzzle, -1, 2);
                        sky_spawn_ering(c->x + 24, muzzle, 1, 2);
                    }
                }
                break;
            case 4u: /* Carrier fan with flak burst */
                sky_spawn_eshot(c->x - 16, muzzle, -2, 3);
                sky_spawn_eshot(c->x - 16, muzzle, -1, 3);
                sky_spawn_eshot(c->x + 16, muzzle, 1, 3);
                sky_spawn_eshot(c->x + 16, muzzle, 2, 3);
                if (c->hp * 2u < c->max_hp) {
                    sky_fire_aimed(c->x, muzzle, px, py, 4);
                    sky_spawn_ering(c->x, muzzle, 0, 2);
                }
                break;
            case 5u: /* Tank Fort: Heavy mobile fortress cannon barrage */
                sky_fire_aimed(c->x + ((c->data2 & 1u) ? -24 : 24), muzzle, px, py, 4);
                sky_spawn_ering(c->x, muzzle, 0, 2);
                if (c->hp * 2u < c->max_hp) {
                    sky_spawn_eshot(c->x - 20, muzzle, -2, 3);
                    sky_spawn_eshot(c->x + 20, muzzle, 2, 3);
                    sky_spawn_ering(c->x, muzzle, 0, 3);
                }
                break;
            default: /* Castle: final five-way spread with expanding rings */
                if (c->data2 & 1u) {
                    int16_t d;
                    for (d = -2; d <= 2; d++) sky_spawn_eshot(c->x, muzzle, d, 3);
                    if (c->hp * 2u < c->max_hp) sky_spawn_ering(c->x, muzzle, 0, 3);
                } else {
                    sky_fire_aimed(c->x - 24, muzzle, px - 16, py, 3);
                    sky_fire_aimed(c->x + 24, muzzle, px + 16, py, 3);
                    if (c->hp * 2u < c->max_hp) {
                        sky_spawn_ering(c->x - 24, muzzle, -2, 2);
                        sky_spawn_ering(c->x + 24, muzzle, 2, 2);
                    }
                }
                break;
            }
        }
    }
}

static void NEOGEO_USER sky_blast_step(NGCharacter *c)
{
    /* Native-sized frames retain their impact anchor; no SCB2 scale wrap.
     * data0 counts frames, data1 is the picture shown. */
    uint8_t k = (uint8_t)c->data1;
    c->data0++;
    if (c->data0 < k_blast_end[k]) return;
    if (++k >= SKY_BLAST_FRAMES) {
        ng_chars_remove(c);
        return;
    }
    c->data1 = k;
    sky_bind(c, sky_blast_art(k), SKY_SCALE_FULL, NG_RENDER_BAND_FX);
    /* Once it is smoke it falls behind with the ground. */
    if (k == SKY_BLAST_FRAMES / 2u) ng_char_set_speed_fp(c, 0, NG_FP_ONE / 2);
}

void NEOGEO_USER sky_stage_effects_tick(void)
{
    sky_blast_queue_step();
    for (NGCharacter *c = chars_at(0u), *end = c + ng_chars_slots_used(); c < end; c++) {
        if (!c || !c->active) continue;
        if (c->kind == SKY_KIND_BLAST) sky_blast_step(c);
        else if (c->kind == SKY_KIND_WRECK) sky_wreck_step(c);
        else if (c->kind == SKY_KIND_SMOKE) {
            sky_smoke_step(c);
            if (c->active) ng_char_set_pos(c, c->x, (int16_t)(c->y + 1));
        }
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
        x = SKY_FIELD_X + 76; /* world road centre 92, minus the 16px gutter */
    } else if (s_squad_type == 7u) {
        /* AA command targets alternate positions: left bunker, central fortress, right outpost */
        uint8_t tgt_idx = (uint8_t)((k_stage[s_stage].waves - s_waves_left) / 8u);
        x = (int16_t)(48 + (tgt_idx % 3u) * 80);
    } else if (s_squad_type == 8u) {
        /* Naval AA command ships alternate positions across harbor and reefs */
        uint8_t tgt_idx = (uint8_t)((k_stage[s_stage].waves - s_waves_left) / 8u);
        x = (int16_t)(48 + (tgt_idx % 3u) * 80);
    } else if (s_squad_type == 4u || s_squad_type == 9u) {
        x = sky_sea_lane((int16_t)(SKY_FIELD_X + 64 + (s_squad_left & 1u) * 112),
                         (int16_t)(SKY_FIELD_TOP - 24 - 32));
        if (x < 0) {          /* no open water in reach: this hull is skipped */
            s_squad_left--;
            s_squad_gap = 24u;
            return;
        }
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
    if (s_squad_type == 7u || s_squad_type == 8u) {
        s_target_alert = 1u;
        playSFX(SOUND_SFX_12);
    }
    s_squad_left--;
    s_squad_gap = (uint16_t)((s_squad_type == 4u || s_squad_type == 5u || s_squad_type == 9u) ? 48u : 24u);
}

static void NEOGEO_USER sky_start_squad(void)
{
    const SkyStage *st = &k_stage[s_stage];

    /* Every sortie includes its advertised surface and aircraft squadrons. */
    s_squad_type = st->roster[(st->waves - s_waves_left) & 3u];
    s_squad_side = (uint8_t)(sky_rand() & 1u);
    s_squad_left = (uint8_t)((s_squad_type == 4u || s_squad_type == 5u || s_squad_type == 9u) ? 2u : 3u);
    s_squad_gap  = 0u;
    if (s_waves_left) s_waves_left--;
}

static uint8_t NEOGEO_USER sky_spawn_defender(uint8_t mini, uint8_t lane)
{
    uint8_t type = s_radar_alert ? 2u : 3u;
    NGCharacter *c;
    if (sky_count_kind(SKY_KIND_ENEMY) >= SKY_MAX_ENEMIES) return 0u;
    c = sky_spawn(SKY_KIND_ENEMY, mini ? (uint8_t)(SKY_MINIBOSS_FIRST + s_stage % 3u) : k_enemy[type].asset,
                  mini ? (s_checkpoint_target && s_checkpoint_target->x >= 144 ? 56 : 232) : lane ? 240 : 48,
                  -24, SKY_SCALE_FULL, NG_RENDER_BAND_ENEMY);
    if (!c) return 0u;
    c->hp = mini ? (uint8_t)(32u + s_stage * 8u) : (uint8_t)(6u + s_stage);
    c->max_hp = c->hp;
    c->flags = SKY_DEFENDER;
    c->data0 = mini ? PAT_MINIBOSS : PAT_HOLD;
    c->data1 = 0u;
    c->data2 = mini ? 6u : type;
    ng_char_set_speed(c, 0, 1);
    return 1u;
}

static void NEOGEO_USER sky_encounter_step(int16_t px, int16_t py)
{
    uint8_t detected;
    if (!s_encounter && s_checkpoint < 3u && s_waves_left &&
        sky_bg_travel() >= (uint16_t)(512u + s_checkpoint * 512u)) {
        s_encounter = 1u;
        s_squad_left = 0u;
        s_target_alert = 1u;
    }
    if (s_encounter == 1u && !sky_count_kind(SKY_KIND_ENEMY)) {
        uint8_t type = s_checkpoint == 1u ? 8u : 7u;
        uint16_t asset = k_enemy[type].asset;
        int16_t tx = 128;
        if (sky_stage_naval()) {
            /* At sea the command targets are warships, anchored on open water. */
            static const int16_t spots[] = {128, 96, 160, 64, 192, 224, 40};
            uint8_t k;
            asset = type == 7u ? SKY_NAVAL_AA : SKY_NAVAL_RADAR;
            for (k = 0u; k < sizeof(spots) / sizeof(spots[0]); k++)
                if (sky_water_run((int16_t)(spots[k] - sky_bg_camera_x()), 56, 136)) { tx = spots[k]; break; }
        }
        s_checkpoint_target = sky_spawn(SKY_KIND_ENEMY, asset,
                                         (int16_t)(tx-sky_bg_camera_x()), 96, SKY_SCALE_FULL, NG_RENDER_BAND_ENEMY);
        if (!s_checkpoint_target) return;
        s_checkpoint_target->hp = s_checkpoint_target->max_hp = (uint8_t)(16u+s_stage*2u);
        s_checkpoint_target->data0 = PAT_SURFACE;
        s_checkpoint_target->data2 = type;
        s_radar_zone = sky_spawn(SKY_KIND_RADAR, SKY_RADAR_FIRST, s_checkpoint_target->x, 96,
                                 SKY_SCALE_FULL, NG_RENDER_BAND_BACK);
        s_guards_to_spawn = 3u;
        s_reinforcements = 0u;
        s_guard_timer = 0u;
        s_encounter = 2u;
    }
    if (s_encounter != 2u) return;
    detected = sky_radar_contains(px, py, s_checkpoint_target?s_checkpoint_target->x:128, 96);
    if (detected && !s_radar_alert && s_chaff_time <= 90u) {
        s_radar_alert = 1u;
        s_reinforcements = 6u;
        s_target_alert = 4u;
        if (s_radar_zone) sky_bind(s_radar_zone, SKY_RADAR_FIRST+1u,
                                 SKY_SCALE_FULL, NG_RENDER_BAND_BACK);
    }
    if (s_guard_timer) s_guard_timer--;
    else if (s_guards_to_spawn) {
        if (sky_spawn_defender((uint8_t)(s_guards_to_spawn == 3u), s_guards_to_spawn & 1u)) {
            s_guards_to_spawn--;
            s_guard_timer = 40u;
        }
    } else if (s_reinforcements) {
        if (sky_spawn_defender(0u, s_reinforcements & 1u)) {
            s_reinforcements--;
            s_guard_timer = 90u;
        }
    } else if (sky_count_kind(SKY_KIND_ENEMY) == 1u) {
        s_encounter = 3u;
        s_target_alert = 2u;
        if (s_radar_zone) ng_chars_remove(s_radar_zone);
        s_radar_zone = 0;
    }
}

uint8_t NEOGEO_USER sky_stage_tick(int16_t player_x, int16_t player_y)
{
    uint8_t i;
    uint8_t cleared = 0u;

    s_tick++;
    if (s_chaff_time) s_chaff_time--;
    if (!s_boss_on) sky_encounter_step(player_x, player_y);
    if ((s_stage == 0u || s_stage == 2u || s_stage == 5u || s_stage == 6u) &&
        !(s_tick % 420u) && sky_count_kind(SKY_KIND_SCENERY) < 2u && !sky_stage_paused() &&
        sky_bg_travel() + 320u < sky_bg_travel_end()) {     /* the road ends before the arena */
        NGCharacter *car = sky_spawn(SKY_KIND_SCENERY,
            (uint8_t)(SKY_CIVILIAN_FIRST + ((s_tick / 420u) & 1u)),
            (int16_t)(76-sky_bg_camera_x()), -8,
            SKY_SCALE_FULL, NG_RENDER_BAND_BACK);
        if (car) ng_char_set_speed_fp(car, 0, NG_FP_ONE / 2);
    }

    /* --- per-object behaviour ------------------------------------- */
    s_seekers_counting = 0u;
    {
    NGCharacter *c = chars_at(0u), *end = c + ng_chars_slots_used();
    for (i = 0u; c < end; i++, c++) {
        if (!c->active) continue;

        if ((c->kind == SKY_KIND_ENEMY || c->kind == SKY_KIND_BOSS) && c->hp &&
            (uint16_t)c->hp * 3u <= c->max_hp &&
            ((s_tick + (uint16_t)i * 7u) % 24u) == 0u)
            sky_spawn_smoke(c->x, (int16_t)(c->y + 12));

        switch (c->kind) {
        case SKY_KIND_ENEMY: sky_enemy_step(c, player_x, player_y); break;
        case SKY_KIND_BOSS:  sky_boss_step(c, player_x, player_y);  break;
        case SKY_KIND_BLAST: sky_blast_step(c);                     break;
        case SKY_KIND_WRECK: sky_wreck_step(c);                     break;
        case SKY_KIND_SMOKE: sky_smoke_step(c);                     break;
        case SKY_KIND_PICKUP:
            c->data1++;
            if (c->y >= SKY_FIELD_BOTTOM - 20) ng_char_set_speed(c, 0, 0);
            if (c->data1 > 720u) ng_chars_remove(c);
            break;
        case SKY_KIND_SCENERY:
            ng_char_set_speed_fp(c, 0, sky_stage_paused() ? 0 : s_stage==0u?NG_FP_ONE*3/4:NG_FP_ONE/2);
            if (c->y > 224) ng_chars_remove(c);
            break;
        case SKY_KIND_CHAFF:
            if (c->data0) c->data0--;
            else ng_chars_remove(c);
            break;
        case SKY_KIND_PSHOT:
            if (c->y < SKY_FIELD_TOP - 24 || c->x < SKY_FIELD_LEFT - 16 ||
                c->x > SKY_FIELD_RIGHT + 16) ng_chars_remove(c);
            break;
        case SKY_KIND_ESHOT:
            c->data1++;
            /* SAMs guide briefly, then commit to a dodgeable trajectory;
             * one the chaff pulled flies at the decoy instead. */
            if (c->data0 == SKY_SEEKER_ART) {
                uint8_t decoyed = (uint8_t)(s_chaff_time && (c->data2 & SKY_SEEKER_FOOLED));
                if ((c->data1 < 75u || decoyed) && !(c->data1 % 12u)) {
                    int16_t tx = decoyed ? s_decoy_x : player_x;
                    int16_t ty = decoyed ? s_decoy_y : player_y;
                    int16_t vx = (int16_t)NG_CLAMP((tx-c->x)/16, -3, 3);
                    int16_t vy = (int16_t)NG_CLAMP((ty-c->y)/16, -3, 3);
                    if (!vx && !vy) vy = 1;
                    ng_char_set_speed(c, vx, vy);
                }
                if (decoyed &&
                    c->x > s_decoy_x-8 && c->x < s_decoy_x+8 &&
                    c->y > s_decoy_y-8 && c->y < s_decoy_y+8) {
                    ng_chars_remove(c);
                    s_decoyed_missiles++;
                    break;
                }
                if (!decoyed && c->data1 < 75u) s_seekers_counting++;
            }
            if (c->y > SKY_FIELD_BOTTOM + 24 || c->y < SKY_FIELD_TOP - 24 ||
                c->x < SKY_FIELD_X - 16 || c->x > SKY_FIELD_X + SKY_FIELD_W + 16) {
                ng_chars_remove(c);
            }
            break;
        default: break;
        }
    }
    }

    s_seekers = s_seekers_counting;

    /* --- wave scheduling ------------------------------------------ */
    if (s_boss_on) {
        if (!sky_count_kind(SKY_KIND_BOSS)) {
            s_boss_on = 0u;
            cleared = 1u;
        }
        return cleared;
    }

    if (s_encounter) return 0u;

    if (s_squad_left) {
        sky_release_next();
    } else if (s_wave_timer) {
        s_wave_timer--;
    } else if (s_waves_left) {
        sky_start_squad();
        s_wave_timer = (uint16_t)(240u - (uint16_t)(s_stage * 6u));
    } else if (sky_bg_travel() < sky_bg_travel_end()) {
        /* Every boss is met on the route's last page, its open arena:
         * the waves are spent, so the flight cruises there. */
        s_cruise = 1u;
        return 0u;
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

void NEOGEO_USER sky_stage_camera_shift(int16_t delta)
{
    if(!delta)return;
    for (NGCharacter *c = chars_at(0u), *end = c + ng_chars_slots_used(); c < end; c++) {
        if(!c || !c->active)continue;
        if((c->kind==SKY_KIND_ENEMY && (c->data0==PAT_SURFACE || c->data0==PAT_SUBMARINE)) ||
           c->kind==SKY_KIND_RADAR || c->kind==SKY_KIND_SCENERY) {
            c->x_fp-=(int32_t)delta*NG_FP_ONE;
            c->x=NG_FROM_FP(c->x_fp);
        }
    }
}
void NEOGEO_USER sky_stage_parts_update(void)
{
    NGCharacter *boss=0;
    for (NGCharacter *c = chars_at(0u), *end = c + ng_chars_slots_used(); c < end; c++) {
        if(c && c->active && c->kind==SKY_KIND_BOSS) {boss=c;break;}
    }
    for (NGCharacter *c = chars_at(0u), *end = c + ng_chars_slots_used(); c < end; c++) {
        if(!c || !c->active || c->kind!=SKY_KIND_MODULE)continue;
        if(!boss) {ng_chars_remove(c);continue;}
        ng_char_set_pos(c,(int16_t)(boss->x+(c->data0?SKY_KEEP_TURRET_X:-SKY_KEEP_TURRET_X)),
                        (int16_t)(boss->y+SKY_KEEP_TURRET_Y+(boss->data1<3u?2:0)));
        if(!(s_tick&7u)) {
            uint16_t id=c->hp<=6u?SKY_NATIVE_KEEP_TURRET_3:
                (uint16_t)(SKY_NATIVE_KEEP_TURRET_1+((s_tick>>3)&1u));
            sky_bind(c,id,SKY_SCALE_FULL,NG_RENDER_BAND_ENEMY);
            ng_char_set_priority(c,NG_RENDER_BAND_ENEMY,1);ng_char_set_body(c,-10,-10,20,20);
        }
    }
}
