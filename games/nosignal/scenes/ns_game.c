/*
 * ns_game.c -- No Signal Z Alpha: the flow of a game.
 *
 * Attract: the title, the premise, how to play. On an arcade board a credit
 * goes straight into the story; a console first offers its modes (the story,
 * stage select, challenge, the gallery). Choose one
 * hero (exactly one person plays), then stage by stage: a short scene, the
 * street, the boss's entrance, the boss, the boss's confession. Out of lives:
 * RECONNECT? (a credit on an arcade board, free on a console) or NO SIGNAL.
 *
 * Controls: the stick walks along and across the street; A attacks (a chain
 * of four), B jumps (A in the air kicks), C grabs and throws, D is the
 * hero's special (it costs a little battery); a double tap runs; down + A
 * sweeps. Walking over food, a gold sneaker or (empty-handed) a weapon picks
 * it up; walking onto the scooter rides it, B (or C) jumps off. On a console
 * START pauses.
 */
#include "ns.h"
#include "sdk/ng_system.h"
#include "games/nosignal/artbox/generated/ns_assets.h"
#include "games/nosignal/artbox/generated/ns_faces.h"

static NGBeatEmUp s_b;
static uint8_t s_hero, s_lives;
static uint32_t s_total;          /* score carried from stage to stage */
static uint16_t s_bark_cool;
static uint8_t s_boss = NGB_NONE;
enum { MODE_STORY = 0, MODE_SELECT, MODE_CHALLENGE, MODE_GALLERY, MODES };
#define CHALLENGE_FRAMES (120u * 60u)
static uint8_t s_mode;

static const NGHowToPlay k_howto = {
    {'H', 'T', 'P', '1'}, 12, 10, "NO SIGNAL Z ALPHA",
    {
        "STICK  WALK THE STREET",
        "TAP TWICE  RUN",
        "A  PUNCH  (A A A A: COMBO)",
        "B  JUMP   (A IN THE AIR)",
        "C  GRAB / THROW",
        "D  SPECIAL (COSTS BATTERY)",
        "DOWN + A  SWEEP",
        "WALK OVER ITEMS: PICK UP",
        "SCOOTER: WALK ON, B: HOP OFF",
        "",
        "GET THE PHONE BACK.",
        "ONE PLAYER. NO CAP.",
    }
};

static uint8_t NEOGEO_USER dip(uint8_t n) { return ng_dip_option(n); }

/* ======================================================================
 * barks: short lines over the action, never too often
 * ==================================================================== */
static const char *const k_ko_barks[] = { "NO CAP!", "YOU'RE COOKED!", "MASSIVE W", "RATIO.", "TOUCH GRASS!", "SKILL ISSUE", "L + BOZO" };
static const char *const k_hurt_barks[] = { "BRUH.", "THAT'S NEGATIVE AURA", "NOT THE FIT!", "CHAT, IS THIS REAL?" };
static const uint8_t k_ko_voice[] = { NS_V_NOCAP, NS_V_COOKED, NS_V_W, NS_V_W, NS_V_NOCAP, NS_V_COOKED, NS_V_W };

/* the chosen hero's voice line v */
static uint8_t NEOGEO_USER voice(uint8_t v) { return (uint8_t)(NS_SFX_VOICE + 4u * s_hero + v); }

static void NEOGEO_USER bark(const char *text, uint8_t sfx, const NGBFighter *f)
{
    int16_t x, y;
    if (s_bark_cool || dip(3)) return;
    x = (int16_t)((f->x - s_b.camera_x) >> 8);
    y = (int16_t)((f->y - f->z) >> 8);
    ns_bark(text, (int16_t)(x - 24), (int16_t)(y - 116));
    if (sfx != 0xFFu) ns_sfx(sfx);
    s_bark_cool = 240;
}

/* ======================================================================
 * scenes: a list of lines over the street or a picture
 * ==================================================================== */
static void NEOGEO_USER scene(const NSLine *lines)
{
    uint8_t i;
    if (!lines) return;
    for (i = 0; lines[i].text; i++) {
        uint8_t who = lines[i].speaker == NS_SPK_HERO ? ns_hero_info[s_hero].speaker : lines[i].speaker;
        ns_dialog(who, lines[i].text);
        ns_wait_button(600);
        if (ns_start_pending() && !NEO_REGISTER8(NS_START_FLAG)) break;
    }
    ns_dialog_hide();
}

/* ======================================================================
 * title, attract
 * ==================================================================== */
/* the title's prompt: under the heroes when they are in the picture */
#define PROMPT_ROW (NS_TITLE_PAINTED ? 24u : 13u)

static void NEOGEO_USER copyright(void)
{
    ns_puts_c(26, NS_VERSION_TEXT, NS_TXT_GREY);
    ns_puts_c(27, "(C) 2026 EAGLE SOFTWARE", NS_TXT_WHITE);
}

static void NEOGEO_USER title_art(void)
{
    ns_picture(NS_ART_TITLE);
    ns_music(NS_MUS_TITLE);
    ns_logo(1, 32, 16);
#if !NS_TITLE_PAINTED
    ns_hero_card(0, NS_ZOE, HF_STANCE, 8, 100, 0);
    ns_hero_card(1, NS_JAY, HF_STANCE, 96, 96, 0);
    ns_hero_card(2, NS_KAI, HF_STANCE, 184, 100, 1);
#endif
    copyright();
    ns_reveal();
}

void NEOGEO_USER ns_title_screen(void)
{
    uint16_t countdown = 15u * 60u, t = 0;
    title_art();
    for (;;) {
        if (NEO_REGISTER8(NS_START_FLAG)) break;
        t++;
        if (!ng_sys_is_mvs()) {
            ns_puts_c(PROMPT_ROW, (t & 32u) ? "PRESS START" : "           ", NS_TXT_WHITE);
            if (ns_start_pending()) break;
        } else if (read_p1credit() > 0) {
            ns_puts_c(PROMPT_ROW, "PRESS 1P START", NS_TXT_WHITE);
            ns_putn(18, PROMPT_ROW + 1u, (uint32_t)(countdown / 60u), 2, NS_TXT_GOLD);
            if (countdown) countdown--;
            else ns_auto_start();
        } else {
            ns_puts_c(PROMPT_ROW, (t & 32u) ? "INSERT COIN" : "           ", NS_TXT_WHITE);
        }
        ns_frame();
    }
    ns_sfx(NS_SFX_START);
}

static void NEOGEO_USER own_how_to_play(uint16_t frames)
{
    uint8_t i;
    ns_picture(NS_ART_MENU);
    ns_card(1);
    ns_puts_c(4, "HOW TO PLAY", NS_TXT_GOLD);
    for (i = 0; i < k_howto.lines; i++) ns_puts(6, (uint8_t)(7u + i), k_howto.line[i], i < 9u ? NS_TXT_WHITE : NS_TXT_SKY);
    ns_reveal();
    while (frames--) {
        ns_frame();
        if (ns_start_pending() && !NEO_REGISTER8(NS_START_FLAG)) return;
        if (NEO_REGISTER8(NS_START_FLAG) && (ns_pressed() & (NS_BUTTONS | NS_START))) return;
    }
}

static const char *const k_premise[] = {
    "ONE PHONE.",
    "TWO YEARS OF VOICE NOTES.",
    "ONE GUY ON A MOTORBIKE.",
    "ZERO CHILL.",
};

void NEOGEO_USER ns_run_attract(void)
{
    for (;;) {
        uint16_t t;
        uint8_t i;
        title_art();
        for (t = 0; t < 420u; t++) {
            ns_puts_c(PROMPT_ROW, (t & 32u) ? (ng_sys_is_mvs() ? "INSERT COIN" : "PRESS START") : "           ", NS_TXT_WHITE);
            ns_frame();
            if (ns_start_pending()) return;
        }
        ns_picture(NS_ART_MENU);
        ns_music(NS_MUS_SCENE);
        ns_reveal();
        for (i = 0; i < 4u; i++) {
            ns_puts_c((uint8_t)(9u + 3u * i), k_premise[i], i == 3u ? NS_TXT_PINK : NS_TXT_WHITE);
            ns_wait(80);
            if (ns_start_pending()) return;
        }
        ns_wait(120);
        if (ng_sys_is_eaglebios()) {
            ns_fade_out();
            ng_sys_howtoplay(&k_howto);
        } else {
            own_how_to_play(480);
        }
        if (ns_start_pending()) return;
    }
}

/* ======================================================================
 * hero select
 * ==================================================================== */
/* the pictures stand on row 20; the chosen hero's lines go under their feet */
#define CARD_Y 36
static uint8_t NEOGEO_USER hero_select(void)
{
    uint8_t sel = 0, i, shown = 0;
    uint16_t t = 15u * 60u;
    ns_picture(NS_ART_MENU);
    ns_plate(1);                 /* the hero's lines on a plate, not over the lit windows */
    ns_music(NS_MUS_SELECT);
    ns_puts_c(2, "CHOOSE YOUR HERO", NS_TXT_GOLD);
    ns_puts_c(3, "(ONE PLAYER. ONE HERO. NO CAP.)", NS_TXT_GREY);
    for (;;) {
        for (i = 0; i < NS_HEROES; i++)
            ns_hero_card(i, i, i == sel ? HF_WIN : HF_STANCE, (int16_t)(8 + 104 * i), CARD_Y, 0);
        ns_clear_text(21, 27);
        ns_puts_c(21, ns_hero_info[sel].name, NS_TXT_GOLD);
        ns_puts_c(22, ns_hero_info[sel].nick, NS_TXT_PINK);
        ns_puts_c(24, ns_hero_info[sel].style, NS_TXT_WHITE);
        ns_puts_c(25, ns_hero_info[sel].special, NS_TXT_SKY);
        if (!shown) { ns_reveal(); shown = 1; }
        for (;;) {
            ns_frame();
#ifdef NS_AUTOPLAY_HERO
            if (sel != NS_AUTOPLAY_HERO) { sel = NS_AUTOPLAY_HERO; break; }    /* a test build plays its own hero */
#endif
            if (t) t--;
            ns_putn(36, 2, (uint32_t)(t / 60u), 2, NS_TXT_GOLD);
            if (ns_pressed() & NS_LEFT) { sel = (uint8_t)(sel ? sel - 1u : NS_HEROES - 1u); ns_sfx(NS_SFX_SELECT); break; }
            if (ns_pressed() & NS_RIGHT) { sel = (uint8_t)(sel + 1u < NS_HEROES ? sel + 1u : 0u); ns_sfx(NS_SFX_SELECT); break; }
            if ((ns_pressed() & (NS_BUTTONS | NS_START)) || !t) {
                ns_sfx(NS_SFX_START);
                ns_hero_card(sel, sel, HF_WIN, (int16_t)(8 + 104 * sel), CARD_Y, 0);
                ns_puts_c(27, sel == NS_JAY ? "LET'S COOK." : sel == NS_ZOE ? "MAIN CHARACTER ENTERS." : "AURA: MAXIMUM.", NS_TXT_GOLD);
                ns_wait(70);
                return sel;
            }
        }
    }
}

/* ======================================================================
 * a stage
 * ==================================================================== */
enum { STAGE_CLEAR = 0, STAGE_OVER, STAGE_QUIT };

static uint8_t NEOGEO_USER boss_index(void)
{
    uint8_t i;
    for (i = 1; i < NGB_FIGHTERS; i++)
        if (s_b.f[i].alive && s_b.f[i].def && s_b.f[i].def->ai == NGB_AI_BOSS) return i;
    return NGB_NONE;
}

/* 1 to play on */
static uint8_t NEOGEO_USER reconnect(void)
{
    uint8_t secs = 10;
    uint16_t t = 0;
    ns_dialog_hide();
    ns_card(1);
    ns_clear_text(4, 25);
    ns_puts_c(7, "CONNECTION LOST", NS_TXT_RED);
    ns_puts_c(11, "RECONNECT?", NS_TXT_GOLD);
    for (;;) {
        ns_frame();
        ns_putn(19, 14, secs, 2, NS_TXT_WHITE);
        if (ng_sys_is_mvs()) ns_puts_c(18, read_p1credit() > 0 ? "PRESS START" : "INSERT COIN", NS_TXT_WHITE);
        else ns_puts_c(18, "PRESS START", NS_TXT_WHITE);
        if (ns_pressed() & (NS_START | NS_A)) {
            if (ns_spend_credit()) { ns_sfx(NS_SFX_START); ns_card(0); ns_clear_text(4, 25); return 1; }
        }
        if (ns_pressed() & NS_BUTTONS) t = (uint16_t)(t + 30u);
        if (++t >= 60u) { t = 0; if (!secs) break; secs--; }
    }
    ns_clear_text(4, 25);
    ns_puts_c(11, "NO SIGNAL", NS_TXT_RED);
    ns_music(NS_MUS_OVER);
    ns_sfx(NS_SFX_NO_SIGNAL);
    ns_wait(240);
    return 0;
}

/* What a boss says as it enters its second and third phases. */
typedef struct { const NGBFighterDef *def; const char *line[2]; uint8_t sfx[2]; } BossTalk;
static const BossTalk k_boss_talk[] = {
    { &ns_rico, { "FINE. WE RIDE.", "OK I'M PANICKING" }, { NS_SFX_RICO_RIDE, NS_SFX_RICO_LAUGH } },
    { &ns_bigflex, { "MY ARMOUR! THAT WAS A RENTAL!", "NOW I'M ANGRY. AND ALSO SWOLE." }, { NS_SFX_CRASH, NS_SFX_ROAR } },
    { &ns_zerox, { "I FEEL... SPARKLY", "I FORGOT HOW TO LOSE!" }, { NS_SFX_GAS, NS_SFX_GAS } },
    { &ns_m08, { "M-08... UNSTABLE...", "M-08... VERY... UPSET" }, { NS_SFX_ALARM, NS_SFX_ROAR } },
    { &ns_behemoth, { "ITS ARMOUR FELL OFF. IT LOOKS... COLD?", "GRAAAAH!!" }, { NS_SFX_CRASH, NS_SFX_ROAR } },
    { &ns_proxy, { "REINFORCEMENTS: SUMMONED. THEY HAVE MORE AURA.", "YOU ARE COOKED. SOURCE: ME." }, { NS_SFX_ALARM, NS_SFX_SERVO } },
    { &ns_overmind, { "DEPLOYING DRONES. DELETING YOU.", "CORE... EXPOSED... DO NOT LOOK AT IT" }, { NS_SFX_ALARM, NS_SFX_ALARM } },
    { &ns_archivist, { "YOU CRACKED MY ARMOUR. HOW... ORGANIC.", "ENOUGH. BEHOLD WHAT I TRULY AM." }, { NS_SFX_WARBLE, NS_SFX_WARBLE } },
};
/* the infected get better when they go down */
static const char *const k_wake[] = { "WHERE AM I?", "WHY AM I IN A HI-VIS VEST?", "IS IT MONDAY?", "I FEEL... BETTER?" };

static void NEOGEO_USER boss_phase(const NGBEvent *e)
{
    const NGBFighter *f = &s_b.f[e->a];
    uint8_t i;
    if (e->c < 1u || e->c > 2u) return;
    for (i = 0; i < sizeof(k_boss_talk) / sizeof(k_boss_talk[0]); i++) {
        if (k_boss_talk[i].def != f->def) continue;
        s_bark_cool = 0;
        bark(k_boss_talk[i].line[e->c - 1u], k_boss_talk[i].sfx[e->c - 1u], f);
        break;
    }
    /* General Proxy and Overmind Prime call in drones from both sides */
    if ((f->def == &ns_proxy || f->def == &ns_overmind) && e->c == 1u) {
        ng_beatemup_spawn(&s_b, &ns_drone, 0, 160, 1);
        ng_beatemup_spawn(&s_b, &ns_drone, 0, 190, 0);
    }
}

/* The sound of the blow that just landed. A swing of a held weapon (move
 * 0xFE) and a throw's recovery (no move) are not rows of the fighter's table. */
static uint8_t NEOGEO_USER blow_sound(const NGBFighter *a)
{
    if (a->state != NGB_ST_ATTACK || a->move == NGB_NONE || !a->def) return NS_SFX_PUNCH;
    if (a->move == 0xFEu) {
        if (a->weapon == NGB_NONE || !s_b.stage->nweapons) return NS_SFX_HEAVY;
        return s_b.stage->weapons[s_b.prop[a->weapon].contents].move->sound;
    }
    return a->move < a->def->nmoves ? a->def->moves[a->move].sound : NS_SFX_PUNCH;
}

/* dust at someone's feet (landing, a dash) */
static void NEOGEO_USER dust(const NGBFighter *f)
{
    ns_effect((int16_t)((f->x - s_b.camera_x) >> 8), (int16_t)((f->y >> 8) - 6), NS_FX_DUST);
}

/* something picked up: a sparkle where it lay */
static void NEOGEO_USER sparkle(uint8_t prop)
{
    const NGBProp *pr = &s_b.prop[prop < NGB_MAX_PROPS ? prop : 0u];
    ns_effect((int16_t)((pr->x - s_b.camera_x) >> 8), (int16_t)((pr->y >> 8) - 14), NS_FX_HIT);
}

static void NEOGEO_USER handle(const NGBEvent *e)
{
    const NGBFighter *p = &s_b.f[NGB_PLAYER];
    switch (e->type) {
    case NGB_EV_HIT: {
        const NGBFighter *t = &s_b.f[e->b], *a = &s_b.f[e->a];
        int16_t x = (int16_t)(((t->x + a->x) >> 9) - (s_b.camera_x >> 8));
        int16_t y = (int16_t)(((t->y - t->z) >> 8) - 52);
        ns_effect(x, y, e->c >= 8u ? NS_FX_BIG : NS_FX_HIT);
        ns_sfx(blow_sound(a));
        if (e->c >= 10u) ns_shake(6);
        if (e->a == NGB_PLAYER && e->b != NGB_PLAYER && t->def && t->def->ai != NGB_AI_BOSS) ns_focus(e->b, t->def);
        if (e->b == NGB_PLAYER) {
            ns_sfx(NS_SFX_HURT);
            if (!(ns_rand() & 3u)) bark(k_hurt_barks[ns_rand() & 3u], voice(NS_V_BRUH), p);
        }
        break;
    }
    case NGB_EV_KO:
        ns_sfx(NS_SFX_KO);
        if (s_b.f[e->b].def && s_b.f[e->b].def->ai == NGB_AI_BOSS) ns_shake(8);
        if (s_b.f[e->b].def == &ns_runner || s_b.f[e->b].def == &ns_bloater) {
            if (!(ns_rand() & 1u)) bark(k_wake[ns_rand() & 3u], 0xFFu, &s_b.f[e->b]);
        } else if (!(ns_rand() & 1u)) {
            uint8_t k = (uint8_t)(ns_rand() & 7u);
            if (k >= 7u) k = 0;
            bark(k_ko_barks[k], voice(k_ko_voice[k]), p);
        }
        break;
    case NGB_EV_KNOCKDOWN: {
        const NGBFighter *t = &s_b.f[e->b];
        ns_sfx(NS_SFX_FALL);
        ns_effect((int16_t)((t->x - s_b.camera_x) >> 8), (int16_t)((t->y >> 8) - 6), NS_FX_DUST);
        break;
    }
    case NGB_EV_GRAB: ns_sfx(NS_SFX_GRAB); break;
    case NGB_EV_THROW: ns_sfx(NS_SFX_THROW); if (e->a == NGB_PLAYER) ns_throw_pose(); break;
    case NGB_EV_TELEGRAPH: {
        const NGBFighter *t = &s_b.f[e->a];
        int16_t x = (int16_t)((t->x - s_b.camera_x) >> 8);
        int16_t y = (int16_t)(((t->y - t->z) >> 8) - 104);
        ns_mark(x, y);
        break;
    }
    case NGB_EV_SPECIAL:
        ns_sfx(NS_SFX_SPECIAL);
        ns_banner(ns_hero_info[s_hero].special, NS_TXT_PINK, 60);
        break;
    case NGB_EV_JUMP: ns_sfx(NS_SFX_JUMP); break;
    case NGB_EV_LAND: if (e->a < NGB_FIGHTERS) dust(&s_b.f[e->a]); break;
    case NGB_EV_DASH: ns_sfx(NS_SFX_WHOOSH); dust(p); break;
    /* walked over or taken with C: always said, a sparkle where it lay */
    case NGB_EV_PICKUP: ns_sfx(NS_SFX_PICKUP); sparkle(e->b); s_bark_cool = 0; bark("WEAPON UNLOCKED", 0xFFu, p); break;
    case NGB_EV_FOOD: ns_sfx(NS_SFX_FOOD); sparkle(e->b); s_bark_cool = 0; bark("SNACK DIFF. +BATTERY", 0xFFu, p); break;
    case NGB_EV_SCORE_ITEM: ns_sfx(NS_SFX_PICKUP); sparkle(e->b); s_bark_cool = 0; bark("DRIP ACQUIRED", 0xFFu, p); break;
    case NGB_EV_BREAK: ns_sfx(NS_SFX_BREAK); break;
    case NGB_EV_WEAPON_BREAK: ns_sfx(NS_SFX_BREAK); bark("IT'S GIVING... BROKEN", 0xFFu, p); break;
    case NGB_EV_MOUNT: ns_sfx(NS_SFX_SCOOTER); s_bark_cool = 0; bark("SCOOTER ARC UNLOCKED", 0xFFu, p); break;
    case NGB_EV_RAM: ns_sfx(NS_SFX_CRASH); ns_shake(4); break;
    case NGB_EV_VEHICLE_BREAK: ns_sfx(NS_SFX_CRASH); ns_shake(6); s_bark_cool = 0; bark("RIP SCOOTER. 2026-2026.", 0xFFu, p); break;
    case NGB_EV_SHOT: ns_sfx(NS_SFX_PHONE); break;
    case NGB_EV_WAVE_CLEAR: break;
    case NGB_EV_GO: ns_sfx(NS_SFX_GO); ns_banner("GO >>>", NS_TXT_GOLD, 90); break;
    case NGB_EV_BOSS_PHASE: ns_shake(8); boss_phase(e); break;
    case NGB_EV_PLAYER_DOWN: s_bark_cool = 0; bark("BRUH.", voice(NS_V_BRUH), p); break;
    case NGB_EV_RESPAWN: ns_banner("RECONNECTED", NS_TXT_MINT, 90); ns_hud(&s_b, s_hero, 1); ns_respawn_blink(); break;
    default: break;
    }
}

#ifdef NS_AUTOPLAY
/* Test builds: a plain player that lines up with the nearest enemy, closes
 * in, faces it and punches, and spends the special when crowded. */
static uint16_t NEOGEO_USER autopilot(void)
{
    const NGBFighter *p = &s_b.f[NGB_PLAYER];
    int16_t px = (int16_t)(p->x >> 8), py = (int16_t)(p->y >> 8), best_d = 0x7FFF, dx = 0, dy = 0;
    uint8_t i, near = 0;
    uint16_t in = 0;
    for (i = 1; i < NGB_FIGHTERS; i++) {
        const NGBFighter *e = &s_b.f[i];
        int16_t ex, ey, d;
        if (!e->alive || e->state == NGB_ST_DEAD) continue;
        ex = (int16_t)((e->x >> 8) - px);
        ey = (int16_t)((e->y >> 8) - py);
        d = (int16_t)((ex < 0 ? -ex : ex) + 2 * (ey < 0 ? -ey : ey));
        if (d < 70) near++;
        if (d < best_d) { best_d = d; dx = ex; dy = ey; }
    }
    /* the bot is no fighter: a slow mend lets a test run reach the ending */
    if (p->hp && p->hp < p->def->max_hp && !(s_b.tick & 31u)) s_b.f[NGB_PLAYER].hp++;
    if (p->state == NGB_ST_RIDE && !(s_b.tick & 127u)) return NS_B;      /* off the scooter now and then */
    if (best_d == 0x7FFF) return (uint16_t)(s_b.locked ? 0u : NS_RIGHT);
    if (dy > 4) in |= NS_DOWN;
    else if (dy < -4) in |= NS_UP;
    if (dx > 34) in |= NS_RIGHT;
    else if (dx < -34) in |= NS_LEFT;
    else if ((dx < 0) != (p->facing != 0)) in |= dx < 0 ? NS_LEFT : NS_RIGHT;
    if (dx > -44 && dx < 44 && dy > -8 && dy < 8 && (s_b.tick & 4u)) in |= NS_A;
    if (near >= 3u && p->hp > 20u && !(s_b.tick & 63u)) in |= NS_D;
    return in;
}
#endif

/* ======================================================================
 * the save block: what the player has reached and the best scores
 * ==================================================================== */
#define SAVE_VERSION 1u
#define SAVE_SIZE    16u
enum { SV_CLEARED = 3, SV_REACHED = 4, SV_BEST = 5, SV_CHALLENGE = 9 };   /* byte offsets */
#define CLEARED_ANY  0x01u                                                /* bit 1 + hero: cleared with that hero */

static uint8_t *NEOGEO_USER save(void) { return (uint8_t *)ng_save_data(); }
static uint8_t NEOGEO_USER save_ok(void) { return (uint8_t)(ng_save_capacity() >= SAVE_SIZE); }

static void NEOGEO_USER save_init(void)
{
    uint8_t *d, i;
    if (!save_ok() || ng_save_valid(SAVE_VERSION, SAVE_SIZE)) return;
    ng_save_format(SAVE_VERSION, SAVE_SIZE);
    d = save();
    for (i = 0; i < SAVE_SIZE; i++) d[i] = 0;
    d[0] = 'N'; d[1] = 'S'; d[2] = 1;
    ng_save_commit();
}

static uint32_t NEOGEO_USER get32(uint8_t at)
{
    const uint8_t *d = save() + at;
    if (!save_ok()) return 0;
    return ((uint32_t)d[0] << 24) | ((uint32_t)d[1] << 16) | ((uint32_t)d[2] << 8) | d[3];
}

static void NEOGEO_USER put32(uint8_t at, uint32_t v)
{
    uint8_t *d = save() + at;
    d[0] = (uint8_t)(v >> 24); d[1] = (uint8_t)(v >> 16); d[2] = (uint8_t)(v >> 8); d[3] = (uint8_t)v;
}

static uint8_t NEOGEO_USER cleared(void) { return save_ok() ? save()[SV_CLEARED] : 0u; }
static uint8_t NEOGEO_USER reached(void) { return save_ok() ? save()[SV_REACHED] : 0u; }

static void NEOGEO_USER note_reached(uint8_t stage)
{
    if (!save_ok() || save()[SV_REACHED] >= stage) return;
    save()[SV_REACHED] = stage;
    ng_save_commit();
}

static void NEOGEO_USER note_score(uint8_t at, uint32_t score)
{
    if (!save_ok() || score <= get32(at)) return;
    put32(at, score);
    ng_save_commit();
}

/* ======================================================================
 * a stage
 * ==================================================================== */
static NGBStage s_stage;          /* the stage as played: its crowd follows the difficulty setting */

/* Field by field (no struct copy: GCC's -O2 68000 copy loops are not to be trusted). */
static const NGBStage *NEOGEO_USER stage_for(const NGBStage *st)
{
    uint8_t diff = dip(1);
    s_stage.length = st->length; s_stage.y_min = st->y_min; s_stage.y_max = st->y_max; s_stage.view_w = st->view_w;
    s_stage.waves = st->waves; s_stage.props = st->props; s_stage.enemies = st->enemies; s_stage.weapons = st->weapons;
    s_stage.nwaves = st->nwaves; s_stage.nprops = st->nprops; s_stage.nenemies = st->nenemies; s_stage.nweapons = st->nweapons;
    s_stage.max_attackers = st->max_attackers; s_stage.max_active = st->max_active; s_stage.flags = st->flags;
    s_stage.camera_max = st->camera_max; s_stage.vehicle_max = st->vehicle_max; s_stage.vehicle_accel = st->vehicle_accel;
    s_stage.vehicle_durability = st->vehicle_durability; s_stage.vehicle_ram = st->vehicle_ram;
    if (diff == 0u) s_stage.max_attackers = 1;                          /* easy: one at a time */
    if (diff >= 2u) s_stage.max_attackers = 3;                          /* hard: three */
    if (diff >= 3u && s_stage.max_active < NGB_MAX_ENEMIES) s_stage.max_active++;   /* expert: and more of them */
    return &s_stage;
}

static uint8_t NEOGEO_USER play_stage(uint8_t n)
{
    const NSStageInfo *si = &ns_stages[n];
    const NGBStage *st = stage_for(si->stage);
    uint8_t lives = (uint8_t)(dip(0) + 1u), boss_seen = 0, pause = 0, secs = 0xFFu;
    uint16_t clock = CHALLENGE_FRAMES;
    s_boss = NGB_NONE;
    ng_beatemup_init(&s_b, st, &si->heroes[s_hero], s_lives, ((uint32_t)ns_rand() << 16) | ns_rand() | 1u);
    s_b.score = s_total;                       /* the engine adds this stage's points to it */
    ns_stage_show(&s_b, si, s_hero);
    ns_music(si->music);
    if (s_mode != MODE_CHALLENGE) scene(si->intro[s_hero]);
    ns_hud(&s_b, s_hero, 1);
    ns_banner(s_mode == MODE_CHALLENGE ? "CHALLENGE: 2 MINUTES" : si->title,
              NS_TXT_GOLD, 120);
    ns_puts_c(13, si->place, NS_TXT_WHITE);
    for (;;) {
        NGBEvent e;
        uint16_t held, btn;
        uint8_t dirs;
        ns_frame();
        if (!ng_sys_is_mvs() && (ns_pressed() & NS_START)) {
            pause ^= 1u;
            ns_banner(pause ? "PAUSED (TOUCH GRASS?)" : "", NS_TXT_GOLD, pause ? 0xFFFFu : 1u);
        }
        if (pause) continue;
        held = ns_held();
#ifdef NS_AUTOPLAY
        held = autopilot();
#endif
        dirs = (uint8_t)(held & 0x0Fu);
        btn = (uint16_t)(held & (NS_A | NS_B | NS_C | NS_D));
        ng_beatemup_input(&s_b, dirs, (uint8_t)btn);
        ng_beatemup_update(&s_b);
        if (s_bark_cool) s_bark_cool--;
        if (s_mode == MODE_CHALLENGE) {
            uint16_t c = clock, t = 0;
            if (clock) clock--;
            while (c >= 60u) { c = (uint16_t)(c - 60u); t++; }
            if ((uint8_t)t != secs) { secs = (uint8_t)t; ns_puts(31, 3, "TIME", NS_TXT_SKY); ns_putn(36, 3, secs, 3, secs < 10u ? NS_TXT_RED : NS_TXT_WHITE); }
            if (!clock) { s_total = s_b.score; return STAGE_CLEAR; }
        }
        while (ng_beatemup_event(&s_b, &e)) {
            if (e.type == NGB_EV_WAVE && st->waves[e.c].flags & NGB_WAVE_BOSS && !boss_seen) {
                boss_seen = 1;
                ns_stage_draw(&s_b);
                ns_music(si->boss_music);
                ns_banner("WARNING: MAIN VILLAIN ENERGY", NS_TXT_RED, 100);
                ns_wait(60);
                if (s_mode != MODE_CHALLENGE) scene(si->boss[s_hero]);
                ns_hud(&s_b, s_hero, 1);
                continue;
            }
            if (e.type == NGB_EV_GAME_OVER) {
                if (reconnect()) {
                    ng_beatemup_continue(&s_b, lives);
                    ns_hud(&s_b, s_hero, 1);
                    continue;
                }
                s_total = s_b.score;
                return STAGE_OVER;
            }
            if (e.type == NGB_EV_STAGE_CLEAR) {
                uint8_t t;
                ns_boss_bar(&s_b, NGB_NONE, 0);
                for (t = 0; t < 60u; t++) { ns_frame(); ng_beatemup_update(&s_b); ns_stage_draw(&s_b); }
                ns_music(NS_MUS_CLEAR);
                if (s_mode != MODE_CHALLENGE) scene(si->outro[s_hero]);
                s_total = s_b.score;
                s_lives = s_b.lives;
                return STAGE_CLEAR;
            }
            handle(&e);
        }
        s_boss = boss_index();
        ns_stage_draw(&s_b);
        ns_hud(&s_b, s_hero, 0);
        ns_boss_bar(&s_b, s_boss, si->boss_name);
    }
}

static void NEOGEO_USER stage_tally(uint8_t n)
{
    ns_card(1);
    ns_clear_text(4, 25);
    ns_puts_c(6, s_mode == MODE_CHALLENGE ? "TIME!" : ns_stages[n].title, NS_TXT_GOLD);
    ns_puts_c(8, s_mode == MODE_CHALLENGE ? "LET'S SEE THAT SCORE." : "CLEAR. MASSIVE W.", NS_TXT_WHITE);
    ns_puts(10, 12, "SCORE", NS_TXT_SKY);
    ns_putn(18, 12, s_b.score, 9, NS_TXT_WHITE);
    ns_puts(10, 14, "BATTERY", NS_TXT_SKY);
    ns_putn(18, 14, s_b.f[NGB_PLAYER].hp, 3, NS_TXT_MINT);
    ns_puts(10, 16, "SIGNAL", NS_TXT_SKY);
    ns_putn(18, 16, s_b.lives, 3, NS_TXT_WHITE);
    ns_wait_button(400);
    ns_card(0);
}

static const char *const k_credits[] = {
    "NO SIGNAL Z ALPHA",
    "",
    "A GAME BY EAGLE SOFTWARE",
    "MADE WITH THE NEOGEOSDK",
    "AND ITS BEAT-'EM-UP ENGINE",
    "",
    "VOICES: PIPER, LIBRITTS-R VOICE",
    "(KOIZUMI ET AL., 2023, CC BY 4.0)",
    "",
    "THANK YOU FOR PLAYING.",
    "CALL SOMEONE YOU MISS.",
};

static void NEOGEO_USER ending(void)
{
    static const uint8_t pics[NS_HEROES] = { NS_ART_END_JAY, NS_ART_END_ZOE, NS_ART_END_KAI };
    uint8_t i;
    ns_picture(pics[s_hero]);
    ns_music(NS_MUS_ENDING);
    ns_reveal();
    scene(ns_ending[s_hero]);
    ns_wait(60);
    /* the last gag */
    ns_sfx(NS_SFX_ALARM);
    ns_card(1);
    ns_clear_text(4, 25);
    ns_puts_c(11, "! STORAGE ALMOST FULL !", NS_TXT_RED);
    ns_puts_c(13, "DELETE SOME VOICE NOTES?", NS_TXT_WHITE);
    ns_wait(150);
    ns_card(0);
    ns_clear_text(4, 25);
    ns_sfx(voice(NS_V_BRUH));
    scene(ns_epilogue[s_hero]);
    /* credits */
    ns_picture(NS_ART_MENU);
    ns_music(NS_MUS_CREDITS);
    ns_reveal();
    ns_card(1);
    for (i = 0; i < sizeof(k_credits) / sizeof(k_credits[0]); i++)
        ns_puts_c((uint8_t)(5u + i), k_credits[i], i == 0u ? NS_TXT_GOLD : i >= 9u ? NS_TXT_PINK : NS_TXT_WHITE);
    ns_puts_c(19, "SCORE", NS_TXT_SKY);
    ns_putn(15, 20, s_b.score, 9, NS_TXT_GOLD);
    ns_wait_button(1200);
}

/* ======================================================================
 * the modes
 * ==================================================================== */
static uint8_t NEOGEO_USER mode_open(uint8_t m)
{
    if (m == MODE_SELECT) return (uint8_t)(cleared() & CLEARED_ANY);
    return 1;
}

/* A console's menu of modes. An arcade board has one: a credit plays the story. */
static uint8_t NEOGEO_USER mode_menu(void)
{
    static const char *const name[MODES] = { "STORY ARCADE", "STAGE SELECT", "CHALLENGE", "GALLERY" };
    static const char *const note[MODES] = { "EIGHT STAGES. ONE PHONE.", "ANY STAGE, ANY TIME.",
                                             "TWO MINUTES. HIGH SCORE.", "EVERYONE YOU MET." };
    uint8_t sel = 0, i;
    uint16_t idle = 0;
    if (ng_sys_is_mvs()) return MODE_STORY;
    ns_picture(NS_ART_MENU);
    ns_music(NS_MUS_SELECT);
    ns_puts_c(3, "PICK A MODE", NS_TXT_GOLD);
    ns_reveal();
    for (;;) {
        for (i = 0; i < MODES; i++) {
            ns_clear_text((uint8_t)(7u + 2u * i), (uint8_t)(7u + 2u * i));
            ns_puts_c((uint8_t)(7u + 2u * i), name[i], !mode_open(i) ? NS_TXT_GREY : i == sel ? NS_TXT_GOLD : NS_TXT_WHITE);
        }
        ns_clear_text(19, 22);
        ns_puts_c(19, mode_open(sel) ? note[sel] : "LOCKED: CLEAR THE STORY FIRST", mode_open(sel) ? NS_TXT_SKY : NS_TXT_RED);
        if (sel == MODE_STORY && get32(SV_BEST)) { ns_puts(12, 21, "BEST", NS_TXT_GREY); ns_putn(17, 21, get32(SV_BEST), 9, NS_TXT_WHITE); }
        if (sel == MODE_CHALLENGE && get32(SV_CHALLENGE)) { ns_puts(12, 21, "BEST", NS_TXT_GREY); ns_putn(17, 21, get32(SV_CHALLENGE), 9, NS_TXT_WHITE); }
        for (;;) {
            ns_frame();
#ifdef NS_AUTOPLAY
            return MODE_STORY;
#endif
            if (ns_pressed()) idle = 0;
            else if (++idle >= 30u * 60u) return MODE_STORY;     /* nobody choosing: the story */
            if (ns_pressed() & NS_UP) { sel = (uint8_t)(sel ? sel - 1u : MODES - 1u); ns_sfx(NS_SFX_SELECT); break; }
            if (ns_pressed() & NS_DOWN) { sel = (uint8_t)(sel + 1u < MODES ? sel + 1u : 0u); ns_sfx(NS_SFX_SELECT); break; }
            if (ns_pressed() & (NS_A | NS_START)) {
                if (mode_open(sel)) { ns_sfx(NS_SFX_START); return sel; }
                ns_sfx(NS_SFX_HURT);
            }
        }
    }
}

static uint8_t NEOGEO_USER stage_menu(void)
{
    uint8_t sel = 0, i;
    ns_picture(NS_ART_MENU);
    ns_puts_c(3, "STAGE SELECT", NS_TXT_GOLD);
    ns_reveal();
    for (;;) {
        for (i = 0; i < NS_STAGES; i++) {
            ns_clear_text((uint8_t)(6u + 2u * i), (uint8_t)(6u + 2u * i));
            ns_puts(8, (uint8_t)(6u + 2u * i), ns_stages[i].title, i == sel ? NS_TXT_GOLD : NS_TXT_GREY);
            ns_puts(17, (uint8_t)(6u + 2u * i), ns_stages[i].place, i == sel ? NS_TXT_WHITE : NS_TXT_GREY);
        }
        for (;;) {
            ns_frame();
            if (ns_pressed() & NS_UP) { sel = (uint8_t)(sel ? sel - 1u : NS_STAGES - 1u); ns_sfx(NS_SFX_SELECT); break; }
            if (ns_pressed() & NS_DOWN) { sel = (uint8_t)(sel + 1u < NS_STAGES ? sel + 1u : 0u); ns_sfx(NS_SFX_SELECT); break; }
            if (ns_pressed() & (NS_A | NS_START)) { ns_sfx(NS_SFX_START); return sel; }
        }
    }
}

static void NEOGEO_USER gallery(void)
{
    uint8_t sel = 0, shown = 0xFFu, frame = 0, t = 0;
    uint8_t far = reached();
    ns_picture(NS_ART_MENU);
    ns_music(NS_MUS_SCENE);
    ns_puts_c(1, "GALLERY", NS_TXT_GOLD);
    ns_puts_c(27, "LEFT/RIGHT: WHO  B: BACK", NS_TXT_GREY);
    ns_reveal();
    for (;;) {
        const NSGalleryEntry *g = &ns_gallery[sel];
        uint8_t open = (uint8_t)(g->stage <= far || (cleared() & CLEARED_ANY));
        if (shown != sel) {
            shown = sel;
            ns_cards_hide();
            ns_clear_text(20, 25);
            if (open) {
                ns_show_fighter(g->art, 0, (int16_t)(160 - 64), 40);
                ns_puts_c(21, g->name, NS_TXT_GOLD);
                ns_puts_c(23, g->bio, NS_TXT_WHITE);
            } else {
                ns_puts_c(12, "???", NS_TXT_GREY);
                ns_puts_c(21, "NOT MET YET", NS_TXT_GREY);
                ns_puts_c(23, "KEEP PLAYING THE STORY", NS_TXT_GREY);
            }
        }
        ns_frame();
        if (open && ++t >= 12u) { t = 0; frame = (uint8_t)(frame == 0u ? 1u : frame == 1u ? 2u : 0u); ns_show_fighter(g->art, frame, (int16_t)(160 - 64), 40); }
        if (ns_pressed() & NS_LEFT) { sel = (uint8_t)(sel ? sel - 1u : ns_gallery_n - 1u); ns_sfx(NS_SFX_SELECT); }
        if (ns_pressed() & NS_RIGHT) { sel = (uint8_t)(sel + 1u < ns_gallery_n ? sel + 1u : 0u); ns_sfx(NS_SFX_SELECT); }
        if (ns_pressed() & (NS_B | NS_START)) { ns_cards_hide(); return; }
    }
}

static void NEOGEO_USER new_run(void)
{
    s_bark_cool = 0;
    s_total = 0;
    s_lives = (uint8_t)(dip(0) + 1u);
}

void NEOGEO_USER ns_run(void)
{
    uint8_t n;
    save_init();
    if (dip(2) == 0) own_how_to_play(600);
    s_mode = mode_menu();
    if (s_mode == MODE_GALLERY) { gallery(); return; }
    n = s_mode == MODE_SELECT ? stage_menu() : 0u;
    s_hero = hero_select();
    new_run();
    if (s_mode == MODE_CHALLENGE) {
        play_stage(0);
        stage_tally(0);
        note_score(SV_CHALLENGE, s_total);
        return;
    }
    if (s_mode == MODE_SELECT) {
        if (play_stage(n) == STAGE_CLEAR) stage_tally(n);
        return;
    }
    for (; n < NS_STAGES; n++) {
        note_reached((uint8_t)(n + 1u));
        if (play_stage(n) == STAGE_OVER) {
            note_score(SV_BEST, s_total);
            return;
        }
        stage_tally(n);
    }
    if (save_ok()) { save()[SV_CLEARED] = (uint8_t)(save()[SV_CLEARED] | CLEARED_ANY | (2u << s_hero)); ng_save_commit(); }
    note_score(SV_BEST, s_total);
    ending();
}
