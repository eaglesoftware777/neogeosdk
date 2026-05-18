/*
 * demo_combat.c — Scene 7: Impact feedback system showcase
 *
 * Demonstrates: hitstop freeze, red palette flash, camera shake,
 * hit spark particles, escalating impact levels.
 *
 * https://eaglesoftware.biz
 */

#include "demo_combat.h"
#include "demo.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "sdk/2d_engine/ng_engine.h"
#include "sdk/2d_engine/ng_feedback.h"
#include "sdk/2d_engine/ng_camera.h"
#include "sdk/2d_engine/ng_palette_fx.h"
#include "sdk/2d_engine/ng_particles.h"
#include "sdk/2d_engine/ng_sprite_group.h"
#include "sdk/2d_engine/ng_chars.h"
#include "sdk/2d_engine/ng_npcs.h"
#include "sdk/2d_engine/ng_border_constraints.h"
#include "sdk/2d_engine/ng_game_events.h"
#include "sdk/2d_engine/ng_joystick.h"
#include "sdk/2d_engine/ng_defs.h"
#include <stdint.h>

void NEOGEO_USER clearFix(void);
void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundPlayGameLoop(uint8_t music_track);
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v);
void NEOGEO_USER playSFX(uint8_t n);

/* Attacker: warrior walk/idle = screen 11, slot 1 */
#define ATKER_SCREEN   20u   /* attack pose */
#define ATKER_SLOT      1u

/* Defender: NPC = cat_01, slot 20 */
#define DEFDR_SCREEN  110u
#define DEFDR_SLOT     20u

/* Base palette slots */
#define ATKER_PAL  DEMO_SCREEN_PALETTE(ATKER_SCREEN)
#define DEFDR_PAL  DEMO_SCREEN_PALETTE(DEFDR_SCREEN)

static const uint16_t s_atker_pal[16] = {
    0x0000u, 0x7FFFu, 0x4F00u, 0x2422u, 0x3747u, 0x7551u,
    0x7001u, 0x7011u, 0x4e82u, 0x2a82u, 0x5341u, 0x3113u,
    0x1448u, 0x1b55u, 0x6FF0u, 0x30FFu
};

/* SFX hook for ng_feedback */
static void NEOGEO_USER combat_sfx_hook(uint16_t id)
{
    if (id < 32u) playSFX((uint8_t)id);
}

/* NPC tile constants */
#define COMBAT_NPC_FRAME_COUNT 12u
#define COMBAT_NPC_FIRST_SCREEN 110u
#define COMBAT_NPC_SCREEN(n) ((uint8_t)(COMBAT_NPC_FIRST_SCREEN + ((uint8_t)(n) % COMBAT_NPC_FRAME_COUNT)))
#define COMBAT_NPC_TILE(n)  DEMO_SCREEN_TILE(COMBAT_NPC_SCREEN(n))
#define COMBAT_NPC_PAL(n)   DEMO_SCREEN_PALETTE(COMBAT_NPC_FIRST_SCREEN)
#define COMBAT_NPC_STRIPS   12u
#define COMBAT_NPC_ROWS     15u
#define COMBAT_NPC_STRIDE   16u
#define COMBAT_NPC_OFFSET_Y (-240)

/* ------------------------------------------------------------------ */
/*  Sub-scene: hitbox visualisation                                      */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER combat_hitbox_visual(void)
{
    NGCharacter *attacker;
    NGCharacter *defender;
    NGCamera cam;
    uint16_t t;
    uint8_t  def_hp;

    clearFix();
    demo_fix_puts(2u, 0u, "HITBOX VISUALISATION", 2u);
    demo_fix_puts(2u, 1u, "BODY RECT OVERLAY  DAMAGE  HP BAR", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    ng_camera_init(&cam);
    ng_camera_set_bounds(&cam, 0, 0, 0, 0);
    ng_camera_snap(&cam, 0, 0);

    ng_feedback_init();
    ng_feedback_set_sfx_hook(combat_sfx_hook);
    ng_particles_init();

    demo_load_screen_palette(11u);
    demo_load_screen_palette(COMBAT_NPC_FIRST_SCREEN);
    ng_chars_init();

    attacker = chars_add(0u, 80, 180);
    if (attacker) {
        ng_char_set_sprite(attacker, 0u, 6u, 10u,
                           DEMO_SCREEN_TILE(11u), DEMO_SCREEN_PALETTE(11u));
        ng_char_set_tile_stride(attacker, 16u);
        ng_char_set_body(attacker, -24, -160, 48, 160);
        attacker->sprite_offset_y = -160;
        attacker->scale_x = 0xFFu;
        attacker->scale_y = 0xFFu;
        attacker->hp     = 10u;
        attacker->max_hp = 10u;
    }

    defender = chars_add(0u, 220, 180);
    if (defender) {
        ng_char_set_sprite(defender, 0u, COMBAT_NPC_STRIPS, COMBAT_NPC_ROWS,
                           COMBAT_NPC_TILE(0u), COMBAT_NPC_PAL(0u));
        ng_char_set_tile_stride(defender, COMBAT_NPC_STRIDE);
        ng_char_set_body(defender, -20, -90, 40, 90);
        defender->sprite_offset_y = COMBAT_NPC_OFFSET_Y;
        defender->scale_x = 0x80u;
        defender->scale_y = 0x80u;
        defender->hp     = 10u;
        defender->max_hp = 10u;
    }

    def_hp = 10u;

    for (t = 0u; t < 300u; t++) {
        uint8_t i;
        char bar[14];

        if ((t % 90u) == 0u && t > 0u && attacker && defender) {
            if (def_hp > 0u) {
                def_hp--;
                ng_char_damage(defender, 1u);
                ng_impact_event(NG_IMPACT_MEDIUM, DEFDR_PAL, s_atker_pal,
                                &cam, (uint16_t)SOUND_SFX_IMPACT_HIT,
                                220, 112, 0u, 0x40u);
            }
        }

        ng_chars_draw();
        ng_camera_update(&cam, 0, 0, 0);
        ng_feedback_update();
        ng_palette_fx_update();
        ng_particles_update();
        ng_particles_draw(40u, 40u);

        /* HP bar */
        bar[0] = '[';
        for (i = 0u; i < 10u; i++) {
            bar[1u + i] = (i < def_hp) ? '#' : ' ';
        }
        bar[11] = ']';
        bar[12] = '\0';
        demo_fix_puts(13u, 24u, bar, (def_hp > 5u) ? 2u : 1u);

        if (demo_frame()) break;
    }

    ng_chars_init();
    ng_particles_init();
}

/* ------------------------------------------------------------------ */
/*  Sub-scene: border constraints                                         */
/* ------------------------------------------------------------------ */
static uint8_t s_bc_triggered;
static uint16_t s_bc_event_id;
static uint16_t s_bc_msg_timer;

static void NEOGEO_USER combat_bc_handler(const NGGameEvent *e)
{
    s_bc_triggered = 1u;
    s_bc_event_id  = e->id;
    s_bc_msg_timer = 30u;
}

static void NEOGEO_USER combat_border_trigger(void)
{
    NGBorderConstraint triggers[2];
    NGCharacter *walker;
    int16_t      wx;
    uint16_t     t;
    NGGameEvent  ev;

    clearFix();
    demo_fix_puts(2u, 0u, "BORDER CONSTRAINTS", 2u);
    demo_fix_puts(2u, 1u, "INVISIBLE TRIGGER VOLUMES  EVENTS", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    demo_fix_puts(10u, 5u, "T1", 1u);
    demo_fix_puts(25u, 5u, "T2", 1u);

    triggers[0].x        = 80;
    triggers[0].y        = 0;
    triggers[0].w        = 16;
    triggers[0].h        = 224;
    triggers[0].event_id = 1u;
    triggers[0].a        = 0u;
    triggers[0].b        = 0u;
    triggers[0].once     = 0u;
    triggers[0].used     = 0u;

    triggers[1].x        = 200;
    triggers[1].y        = 0;
    triggers[1].w        = 16;
    triggers[1].h        = 224;
    triggers[1].event_id = 2u;
    triggers[1].a        = 0u;
    triggers[1].b        = 0u;
    triggers[1].once     = 0u;
    triggers[1].used     = 0u;

    ng_border_constraints_init();
    ng_border_constraints_load(triggers, 2u);
    ng_game_events_set_handler(combat_bc_handler);

    demo_load_screen_palette(COMBAT_NPC_FIRST_SCREEN);
    ng_chars_init();
    walker = chars_add(0u, 0, 180);
    if (walker) {
        ng_char_set_sprite(walker, 0u, COMBAT_NPC_STRIPS, COMBAT_NPC_ROWS,
                           COMBAT_NPC_TILE(0u), COMBAT_NPC_PAL(0u));
        ng_char_set_tile_stride(walker, COMBAT_NPC_STRIDE);
        ng_char_set_body(walker, -20, -90, 40, 90);
        walker->sprite_offset_y = COMBAT_NPC_OFFSET_Y;
        walker->scale_x = 0x80u;
        walker->scale_y = 0x80u;
    }

    s_bc_triggered = 0u;
    s_bc_msg_timer = 0u;
    wx = 0;

    for (t = 0u; t < 300u; t++) {
        if (walker) {
            walker->x = wx;
            ng_border_constraints_update();
            while (ng_game_events_count() > 0u) {
                ng_game_events_read(&ev);
            }
        }

        wx = (int16_t)(wx + 2);
        if (wx > 320) wx = 0;

        ng_chars_draw();

        if (s_bc_msg_timer > 0u) {
            char buf[24];
            buf[0]  = 'T';
            buf[1]  = 'R';
            buf[2]  = 'I';
            buf[3]  = 'G';
            buf[4]  = 'G';
            buf[5]  = 'E';
            buf[6]  = 'R';
            buf[7]  = '!';
            buf[8]  = ' ';
            buf[9]  = 'E';
            buf[10] = 'V';
            buf[11] = ':';
            buf[12] = (char)('0' + (uint8_t)s_bc_event_id);
            buf[13] = '\0';
            demo_fix_puts(12u, 10u, buf, 2u);
            s_bc_msg_timer--;
        } else {
            demo_fix_puts(12u, 10u, "             ", 0u);
        }

        if (demo_frame()) break;
    }

    ng_game_events_set_handler((void*)0u);
    ng_chars_init();
}

/* ------------------------------------------------------------------ */
/*  Sub-scene: NPC basic patrol                                          */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER combat_npc_basic(void)
{
    NGNpc *npcs[3];
    NGCharacter *c;
    uint8_t i;
    uint16_t t;
    static const int16_t npc_x[3] = { 40, 140, 240 };

    clearFix();
    demo_fix_puts(2u, 0u, "NPC PATROL / THINK LOOP", 2u);
    demo_fix_puts(2u, 1u, "3 NPCS  PATROL BOUNDS  THINK EVERY 8F", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    demo_load_screen_palette(COMBAT_NPC_FIRST_SCREEN);
    ng_chars_init();
    ng_npcs_init();

    for (i = 0u; i < 3u; i++) {
        npcs[i] = npc_spawn(0u, 0u, npc_x[i], 180);
        if (npcs[i]) {
            c = npc_char(npcs[i]);
            if (c) {
                ng_char_set_sprite(c, 0u, COMBAT_NPC_STRIPS, COMBAT_NPC_ROWS,
                                   COMBAT_NPC_TILE(i * 2u), COMBAT_NPC_PAL(i * 2u));
                ng_char_set_tile_stride(c, COMBAT_NPC_STRIDE);
                c->sprite_offset_y = COMBAT_NPC_OFFSET_Y;
                c->scale_x = 0x80u;
                c->scale_y = 0x80u;
            }
            ng_npc_set_home(npcs[i], npc_x[i], 180);
            ng_npc_set_patrol_bounds(npcs[i],
                (int16_t)(npc_x[i] - 60), (int16_t)(npc_x[i] + 60),
                150, 200);
            ng_npc_set_think(npcs[i], ng_npc_think_patrol, 8u);
        }
    }

    for (t = 0u; t < 300u; t++) {
        char buf[8];
        uint8_t cnt = ng_npc_count();

        buf[0] = 'N';
        buf[1] = 'P';
        buf[2] = 'C';
        buf[3] = ':';
        buf[4] = (char)('0' + cnt);
        buf[5] = '\0';
        demo_fix_puts(30u, 3u, buf, 1u);

        for (i = 0u; i < 3u; i++) {
            if (npcs[i]) {
                c = npc_char(npcs[i]);
                if (c) {
                    uint8_t anim = (uint8_t)((t / 12u) % 12u);
                    uint16_t tile = COMBAT_NPC_TILE(anim);
                    uint8_t pal = COMBAT_NPC_PAL(anim);
                    demo_load_screen_palette(COMBAT_NPC_FIRST_SCREEN);
                    if (c->sprite_tile != tile || c->palette != pal) {
                        c->sprite_tile = tile;
                        c->palette = pal;
                        c->sprite_dirty = 1u;
                    }
                    {
                        char dbuf[4];
                        dbuf[0] = (c->vx_fp > 0) ? '>' : (c->vx_fp < 0 ? '<' : '-');
                        dbuf[1] = '\0';
                        demo_fix_puts((uint8_t)(3u + i * 10u), 23u, dbuf, 2u);
                    }
                }
            }
        }

        ng_npcs_update();
        ng_chars_draw();
        if (demo_frame()) break;
    }

    ng_chars_init();
    ng_npcs_init();
}

/* ------------------------------------------------------------------ */
/*  Sub-scene: advanced NPC boss phases                                  */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER combat_npc_advanced(void)
{
    NGNpc *boss;
    NGCharacter *bc;
    NGCamera cam;
    uint16_t t;
    uint8_t  phase;
    uint16_t phase_timer;

    static const char *const s_phase_names[4] = {
        "PATROL ", "AGGRO  ", "ATTACK ", "RETREAT"
    };

    clearFix();
    demo_fix_puts(2u, 0u, "MULTI-PHASE BOSS NPC", 2u);
    demo_fix_puts(2u, 1u, "PATROL > AGGRO > ATTACK > RETREAT", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    ng_camera_init(&cam);
    ng_camera_set_bounds(&cam, 0, 0, 0, 0);
    ng_camera_snap(&cam, 0, 0);

    ng_feedback_init();
    ng_feedback_set_sfx_hook(combat_sfx_hook);
    ng_particles_init();

    demo_load_screen_palette(COMBAT_NPC_FIRST_SCREEN);
    ng_chars_init();
    ng_npcs_init();

    boss = npc_spawn(0u, 0u, 160, 160);
    if (boss) {
        bc = npc_char(boss);
        if (bc) {
            ng_char_set_sprite(bc, 0u, COMBAT_NPC_STRIPS, COMBAT_NPC_ROWS,
                               COMBAT_NPC_TILE(0u), COMBAT_NPC_PAL(0u));
            ng_char_set_tile_stride(bc, COMBAT_NPC_STRIDE);
            bc->sprite_offset_y = COMBAT_NPC_OFFSET_Y;
            bc->scale_x = 0xB0u;
            bc->scale_y = 0xB0u;
            bc->hp     = 10u;
            bc->max_hp = 10u;
        }
        ng_npc_set_home(boss, 160, 160);
        ng_npc_set_patrol_bounds(boss, 80, 240, 140, 190);
        ng_npc_set_think(boss, ng_npc_think_patrol, 8u);
    }

    phase       = 0u;
    phase_timer = 0u;

    for (t = 0u; t < 400u; t++) {
        if (phase_timer >= 90u) {
            phase       = (uint8_t)((phase + 1u) & 3u);
            phase_timer = 0u;

            if (phase == 2u && boss) {
                ng_impact_event(NG_IMPACT_MEDIUM, COMBAT_NPC_PAL(0u), s_atker_pal,
                                &cam, (uint16_t)SOUND_SFX_IMPACT_HIT,
                                160, 112, 0u, 0x40u);
            }
        }
        phase_timer++;

        if (boss) {
            bc = npc_char(boss);
            if (bc) {
                uint8_t anim = (uint8_t)((t / 12u) % 12u);
                uint16_t tile = COMBAT_NPC_TILE(anim);
                uint8_t pal = COMBAT_NPC_PAL(anim);
                demo_load_screen_palette(COMBAT_NPC_FIRST_SCREEN);
                if (bc->sprite_tile != tile || bc->palette != pal) {
                    bc->sprite_tile = tile;
                    bc->palette = pal;
                    bc->sprite_dirty = 1u;
                }
            }
        }

        demo_fix_puts(2u, 3u, "PHASE:", 0u);
        demo_fix_puts(9u, 3u, s_phase_names[phase], 2u);

        ng_npcs_update();
        ng_chars_draw();
        ng_camera_update(&cam, 0, 0, 0);
        ng_feedback_update();
        ng_palette_fx_update();
        ng_particles_update();
        ng_particles_draw(40u, 40u);

        if (demo_frame()) break;
    }

    ng_chars_init();
    ng_npcs_init();
    ng_particles_init();
}

/* ------------------------------------------------------------------ */
/*  Sub-scene: special moves attack pipeline                             */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER combat_special_moves(void)
{
    NGCharacter *attacker;
    NGCharacter *defender;
    NGCamera cam;
    uint16_t t;
    uint8_t  show_label;
    uint8_t  label_timer;

    clearFix();
    demo_fix_puts(2u, 0u, "SPECIAL MOVES / FULL ATTACK PIPELINE", 2u);
    demo_fix_puts(2u, 1u, "HITBOX + FEEDBACK + PARTICLES + SFX", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    ng_camera_init(&cam);
    ng_camera_set_bounds(&cam, 0, 0, 0, 0);
    ng_camera_snap(&cam, 0, 0);

    ng_feedback_init();
    ng_feedback_set_sfx_hook(combat_sfx_hook);
    ng_particles_init();

    demo_load_screen_palette(11u);
    demo_load_screen_palette(COMBAT_NPC_FIRST_SCREEN);
    ng_chars_init();

    attacker = chars_add(0u, 80, 180);
    if (attacker) {
        ng_char_set_sprite(attacker, 0u, 6u, 10u,
                           DEMO_SCREEN_TILE(11u), DEMO_SCREEN_PALETTE(11u));
        ng_char_set_tile_stride(attacker, 16u);
        ng_char_set_body(attacker, -24, -160, 48, 160);
        attacker->sprite_offset_y = -160;
        attacker->scale_x = 0xFFu;
        attacker->scale_y = 0xFFu;
        attacker->hit_x = 0;
        attacker->hit_y = -80;
        attacker->hit_w = 60;
        attacker->hit_h = 40;
    }

    defender = chars_add(0u, 220, 180);
    if (defender) {
        ng_char_set_sprite(defender, 0u, COMBAT_NPC_STRIPS, COMBAT_NPC_ROWS,
                           COMBAT_NPC_TILE(0u), COMBAT_NPC_PAL(0u));
        ng_char_set_tile_stride(defender, COMBAT_NPC_STRIDE);
        ng_char_set_body(defender, -20, -90, 40, 90);
        defender->sprite_offset_y = COMBAT_NPC_OFFSET_Y;
        defender->scale_x = 0x80u;
        defender->scale_y = 0x80u;
    }

    show_label  = 0u;
    label_timer = 0u;

    for (t = 0u; t < 300u; t++) {
        if ((t % 80u) == 40u && attacker && defender) {
            NGRect hr = ng_char_hit_rect(attacker);
            NGRect dr = ng_char_body_rect(defender);
            if (ng_rect_hit(hr, dr)) {
                ng_impact_event(NG_IMPACT_HEAVY, DEFDR_PAL, s_atker_pal,
                                &cam, (uint16_t)SOUND_SFX_IMPACT_HIT,
                                220, 100, 0u, 0x40u);
                ng_spawn_slash_trail(220, 100, 1u, 0u, 0x40u);
                ng_spawn_hit_spark(220, 100, 0u, 0x40u);
                show_label  = 1u;
                label_timer = 20u;
            }
        }

        if (label_timer > 0u) {
            demo_fix_puts(13u, 5u, "SPECIAL MOVE!", 2u);
            label_timer--;
        } else if (show_label) {
            demo_fix_puts(13u, 5u, "             ", 0u);
            show_label = 0u;
        }

        ng_chars_draw();
        ng_camera_update(&cam, 0, 0, 0);
        ng_feedback_update();
        ng_palette_fx_update();
        ng_particles_update();
        ng_particles_draw(40u, 40u);

        if (demo_frame()) break;
    }

    ng_chars_init();
    ng_particles_init();
}

/* ------------------------------------------------------------------ */
/*  Sub-scene: joystick module live example                               */
/* ------------------------------------------------------------------ */
static uint16_t s_joy_last_event;

static void NEOGEO_USER combat_joy_event_handler(const NGGameEvent *e)
{
    if (!e) return;
    if (e->id >= NG_EVENT_JOY_JUMP && e->id <= NG_EVENT_JOY_SPECIAL_DP) {
        s_joy_last_event = e->id;
    }
}

static void NEOGEO_USER combat_joy_put_event(uint16_t ev)
{
    switch (ev) {
        case NG_EVENT_JOY_JUMP:
            demo_fix_puts(2u, 5u, "EVENT: JUMP      ", 2u);
            break;
        case NG_EVENT_JOY_FIRE_LIGHT:
            demo_fix_puts(2u, 5u, "EVENT: LIGHT HIT ", 1u);
            break;
        case NG_EVENT_JOY_FIRE_HEAVY:
            demo_fix_puts(2u, 5u, "EVENT: HEAVY HIT ", 1u);
            break;
        case NG_EVENT_JOY_HIT:
            demo_fix_puts(2u, 5u, "EVENT: BASIC HIT ", 0u);
            break;
        case NG_EVENT_JOY_SPECIAL_QCF:
            demo_fix_puts(2u, 5u, "EVENT: SPECIAL QCF", 2u);
            break;
        case NG_EVENT_JOY_SPECIAL_DP:
            demo_fix_puts(2u, 5u, "EVENT: SPECIAL DP ", 2u);
            break;
        default:
            demo_fix_puts(2u, 5u, "EVENT: ---       ", 0u);
            break;
    }
}

static void NEOGEO_USER combat_joystick_example(void)
{
    NGCharacter *player;
    NGJoystickCharConfig cfg;
    uint16_t t;

    clearFix();
    demo_fix_puts(2u, 0u, "JOYSTICK INPUT EXAMPLE", 2u);
    demo_fix_puts(2u, 1u, "LEFT/RIGHT MOVE  A JUMP  B/C ATTACK", 1u);
    demo_fix_puts(2u, 2u, "QCF+B OR DP+C -> SPECIAL EVENT", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    demo_load_screen_palette(11u);
    ng_chars_init();
    ng_physics_init();
    ng_game_events_init();
    ng_game_events_set_handler(combat_joy_event_handler);
    ng_joystick_init();

    ng_physics_add_solid(24, 188, 272, 8, 0u);
    demo_fix_puts(2u, 24u, "====================================", 0u);

    player = chars_add(0u, 96, 188);
    if (player) {
        ng_char_set_sprite(player, 0u, 6u, 10u,
                           DEMO_SCREEN_TILE(11u), DEMO_SCREEN_PALETTE(11u));
        ng_char_set_tile_stride(player, 16u);
        ng_char_set_body(player, -20, -158, 40, 158);
        player->sprite_offset_y = -160;
        player->scale_x = 0xFFu;
        player->scale_y = 0xFFu;
        ng_physics_attach(player, (uint16_t)(NG_PHYSICS_GRAVITY | NG_PHYSICS_SOLIDS));
    }

    cfg = *ng_joy_default_char_config();
    cfg.jump_button = BUTTON_A;
    cfg.light_button = BUTTON_B;
    cfg.heavy_button = BUTTON_C;
    cfg.hit_button = BUTTON_D;

    s_joy_last_event = 0u;
    combat_joy_put_event(0u);

    for (t = 0u; t < 420u; t++) {
        const NGJoystickState *js;
        ng_joystick_update();
        js = ng_joystick_state();

        if (player) {
            ng_joy_control_character(player, &cfg);
        }

        ng_physics_update_pre();
        ng_chars_update();
        ng_physics_resolve();
        ng_game_events_update();
        ng_chars_draw();

        combat_joy_put_event(s_joy_last_event);
        {
            char buf[24];
            uint8_t dir = js ? js->dir : 5u;
            uint8_t jump_hold = ng_joy_held_frames(BUTTON_A);
            uint8_t atk_hold = ng_joy_held_frames(BUTTON_B);
            buf[0] = 'D'; buf[1] = 'I'; buf[2] = 'R'; buf[3] = ':'; buf[4] = ' ';
            buf[5] = (char)('0' + (dir % 10u));
            buf[6] = ' '; buf[7] = 'J'; buf[8] = ':'; buf[9] = (char)('0' + ((jump_hold / 10u) % 10u));
            buf[10] = (char)('0' + (jump_hold % 10u));
            buf[11] = ' '; buf[12] = 'B'; buf[13] = ':'; buf[14] = (char)('0' + ((atk_hold / 10u) % 10u));
            buf[15] = (char)('0' + (atk_hold % 10u));
            buf[16] = ' '; buf[17] = ' '; buf[18] = ' '; buf[19] = ' '; buf[20] = ' '; buf[21] = ' '; buf[22] = ' '; buf[23] = '\0';
            demo_fix_puts(2u, 7u, buf, 0u);
        }

        if (demo_frame()) break;
    }

    ng_game_events_set_handler(0);
    ng_physics_clear_solids();
    ng_chars_init();
}

/* ------------------------------------------------------------------ */
/*  Combat sub-scene: escalating hit sequence                            */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER combat_hit_sequence(void)
{
    NGCamera cam;
    uint8_t  hit;
    uint16_t t;

    clearFix();
    demo_fix_puts(2u, 0u, "IMPACT FEEDBACK SYSTEM", 2u);
    demo_fix_puts(2u, 1u, "HITSTOP + SHAKE + FLASH + PARTICLES", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    ng_camera_init(&cam);
    ng_camera_set_bounds(&cam, 0, 0, 0, 0);
    ng_camera_snap(&cam, 0, 0);

    ng_feedback_init();
    ng_feedback_set_sfx_hook(combat_sfx_hook);

    ng_particles_init();

    /* Draw both characters once (static for hitstop demo) */
    demo_load_screen_palette(11u);
    demo_draw_sprite_screen(11u, ATKER_SLOT,  60, (int16_t)(-34), 16u, 16u, 0xFFu, 0xFFu);
    demo_load_screen_palette(COMBAT_NPC_FIRST_SCREEN);
    demo_draw_sprite_screen(DEFDR_SCREEN, DEFDR_SLOT, 200, (int16_t)(-60), COMBAT_NPC_STRIPS, COMBAT_NPC_ROWS, 0x70u, 0x70u);

    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);

    for (t = 0u; t < 360u; t++) {
        /* Trigger a hit at frames 60, 120, 180, 240 — escalating */
        if ((t == 60u) || (t == 120u) || (t == 180u) || (t == 240u)) {
            hit = (uint8_t)((t / 60u) - 1u);  /* 0=light, 1=medium, 2=heavy, 3=boss */
            if (hit > NG_IMPACT_BOSS) hit = NG_IMPACT_BOSS;

            ng_impact_event(hit, DEFDR_PAL, s_atker_pal,
                            &cam,
                            (uint16_t)SOUND_SFX_IMPACT_HIT,
                            200, 112, 0u, 0x40u);

            {
                const char *labels[4] = {
                    "LIGHT IMPACT  HITSTOP 3F",
                    "MEDIUM IMPACT  SHAKE 6F",
                    "HEAVY IMPACT  SHAKE 8F",
                    "BOSS HIT  MAX SHAKE 12F"
                };
                demo_fix_puts(2u, 3u, labels[hit], (uint8_t)(hit + 1u));
            }
        }

        /* Update systems */
        ng_camera_update(&cam, 0, 0, 0);
        ng_particles_update();
        ng_particles_draw(40u, 40u);
        ng_feedback_update();
        ng_palette_fx_update();

        if (demo_frame()) break;
    }

    ng_particles_init();
}

/* ------------------------------------------------------------------ */
/*  Public: combat scene                                                 */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_combat_run(void)
{
    demo_clear_scene();
    soundSceneReset();
    soundSetADPCMAVolume(0x3Cu);

    combat_hitbox_visual();
    combat_border_trigger();
    combat_npc_basic();
    combat_npc_advanced();
    combat_special_moves();
    combat_joystick_example();
    combat_hit_sequence();

    soundStopAll();
    demo_clear_scene();
}
