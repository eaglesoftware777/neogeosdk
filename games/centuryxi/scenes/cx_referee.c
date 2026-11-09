/*
 * cx_referee.c -- Century XI: the people round the match, and what they say.
 *
 * All parody. The referee is pompous, the linesman terrified, the coach beside
 * himself, and everybody has an opinion. None of this touches the simulation, it
 * only reads what happened, so the talk has its own little dice and a replayed match
 * stays the same match. Lines stay inside the screen's safe columns.
 */
#include "cx_render.h"

#define U(v) ((int16_t)((v) >> 4))
#define TALK_ROW 10u                 /* over the pitch: on row 26 it sat on the stand's boards */

static uint16_t s_dice;
static uint8_t s_talk_t;
static uint8_t s_coach_t, s_coach_f, s_lines_t;
static int16_t s_lines_x;

static const char *const k_foul[] = { "REF: NO KARATE PLEASE!", "REF: THAT WAS A LEG, NOT A BALL!", "PLAYER: I NEVER TOUCHED HIM!", "PLAYER: HE FELL ON MY BOOT!", "FAN: SEND HIM TO THE MOON!" };
static const char *const k_yellow[] = { "REF: NAUGHTY BOY!", "REF: ONE MORE AND NO DINNER!", "PLAYER: MY MOTHER IS WATCHING!", "REF: I WRITE YOUR NAME. SPELL IT." };
static const char *const k_red[] = { "REF: GO HOME!", "REF: BATH TIME!", "PLAYER: BUT I BROUGHT ORANGES!", "COACH: HE WAS MY BEST MAN!" };
static const char *const k_dive[] = { "REF: NICE ACTING!", "REF: THE OSCARS ARE NEXT DOOR!", "FAN: HE DIED! NO, HE IS FINE!", "PLAYER: I FELT A BREEZE!" };
static const char *const k_hurt[] = { "PHYSIO: MAGIC SPONGE!", "PLAYER: TELL MY WIFE I LOVE HER!", "PHYSIO: IT IS ONLY A SCRATCH!", "PLAYER: I CAN SEE THE LIGHT!" };
static const char *const k_offside[] = { "LINESMAN: SORRY! SORRY! SORRY!", "LINESMAN: I SAW IT, WITH GLASSES!", "STRIKER: I WAS LEVEL, I SWEAR!", "COACH: BUY NEW GLASSES!" };
static const char *const k_save[] = { "KEEPER: NOT TODAY!", "KEEPER: MY GLOVES ARE MAGIC!", "KEEPER: TOO EASY! MY KNEE HURTS!", "FAN: WHAT A CAT!" };
static const char *const k_miss[] = { "COACH: MY GRANDMA SCORES THAT!", "COACH: WHY?! WHY?! WHY?!", "FAN: I WANT MY TICKET BACK!", "STRIKER: THE GRASS MOVED!" };
static const char *const k_goal[] = { "COACH: I TAUGHT HIM THAT!", "FAN: BEST DAY OF MY LIFE!", "KEEPER: THE SUN WAS IN MY EYES!", "COACH: PAY THAT MAN!" };
static const char *const k_own[] = { "COACH: WRONG WAY! WRONG WAY!", "DEFENDER: THE BALL HAD OTHER IDEAS", "KEEPER: WE ARE ON THE SAME SIDE!" };
static const char *const k_hand[] = { "PLAYER: MY ARM WAS SLEEPING!", "REF: FOOTBALL. FOOT. BALL.", "PLAYER: IT JUMPED AT ME!" };
static const char *const k_nutmeg[] = { "FAN: THROUGH THE LEGS!", "DEFENDER: MY MOTHER SAW THAT!", "FAN: OLE!" };
static const char *const k_argue[] = { "PLAYER: ARE YOU BLIND, REF?!", "REF: I HAVE TWO EYES. ONE WORKS.", "REF: NO TALKING!", "PLAYER: WHO IS PAYING YOU?!" };
static const char *const k_sub[] = { "COACH: GO ON, AND RUN THIS TIME!", "SUB: FINALLY! MY LEGS WERE COLD!", "COACH: DO NOT TOUCH THE BALL." };
static const char *const k_keeper[] = { "REF: HANDS OFF THE KEEPER!", "KEEPER: HE TRIED TO EAT MY BALL!", "REF: THE KEEPER IS NOT A PINATA!", "REF: LEAVE HIM ALONE. YELLOW!" };
static const char *const k_wall[] = { "REF: BACK! NINE METRES!", "REF: I SAID BACK, BOYS!", "WALL: WE ARE BACK! (WE ARE NOT)", "REF: ONE MORE STEP. AND ONE MORE." };
static const char *const k_fumble[] = { "FAN: BUTTER FINGERS!", "KEEPER: IT WAS WET!", "COACH: NOT THE GLOVES AGAIN!" };

#define PICK(list) say_one(list, (uint8_t)(sizeof(list) / sizeof(list[0])))

static void NEOGEO_USER say_one(const char *const *list, uint8_t n)
{
    s_dice = (uint16_t)(s_dice * 25173u + 13849u);
    cx_clear_row(TALK_ROW);
    cx_puts_center(TALK_ROW, list[(uint16_t)(s_dice >> 8) % n], CX_PAL_WHITE);
    s_talk_t = 150;
}

/* Who has the whistle today. */
void NEOGEO_USER cx_ref_intro(const CxWorld *w)
{
    static const char *const k_who[3] = { "REF TODAY: MR TANAKA, HE NAPS A LOT", "REF TODAY: MR SATO, FAIR AND SHORT", "REF TODAY: MR KAMINARI, LOVES CARDS" };
    cx_clear_row(TALK_ROW);
    cx_puts_center(TALK_ROW, k_who[w->ref_style], CX_PAL_GOLD);
    s_talk_t = 200;
}

static uint8_t s_goal_said;        /* this goal's line has been said */

void NEOGEO_USER cx_talk_reset(void)
{
    s_talk_t = 0;
    s_coach_t = 0;
    s_lines_t = 0;
    s_dice = (uint16_t)(s_dice + 1u);
}

void NEOGEO_USER cx_talk_events(const CxWorld *w)
{
    uint32_t ev = w->ev;
    s_dice = (uint16_t)(s_dice + w->frame);
    if (w->phase == CX_P_FREEKICK && w->fk_wall_n && w->phase_t == 10u) { PICK(k_wall); return; }
    if ((ev & CX_EV_FOUL) && w->keeper_foul) PICK(k_keeper);
    else if (w->phase == CX_P_GOAL && w->phase_t >= 140u && !s_goal_said) {
        s_goal_said = 1;
        if (w->goal_kind == CX_G_OWN) PICK(k_own); else PICK(k_goal);
    }
    else if (ev & CX_EV_GOAL) { s_goal_said = 0; return; }   /* said once the GOAL words have gone (above) */
    else if (ev & CX_EV_HANDBALL) PICK(k_hand);
    else if (ev & CX_EV_CARD) { if (w->card_red) PICK(k_red); else PICK(k_yellow); }
    else if (ev & CX_EV_DIVE) PICK(k_dive);
    else if (ev & CX_EV_INJURY) PICK(k_hurt);
    else if (ev & CX_EV_OFFSIDE) PICK(k_offside);
    else if (ev & CX_EV_FOUL) { if (s_dice & 1024u) PICK(k_foul); else PICK(k_argue); }
    else if (ev & CX_EV_FUMBLE) PICK(k_fumble);
    else if (ev & CX_EV_SAVE) PICK(k_save);
    else if (ev & (CX_EV_MISS | CX_EV_POST)) PICK(k_miss);
    else if (ev & CX_EV_NUTMEG) PICK(k_nutmeg);
    else if (ev & CX_EV_SUB) PICK(k_sub);
}

void NEOGEO_USER cx_talk_step(void)
{
    if (s_talk_t && !--s_talk_t) cx_clear_row(TALK_ROW);
}

/* ---------------------------------------------------------------- the bench */
void NEOGEO_USER cx_coach_pop(uint8_t frame, uint8_t frames)
{
    s_coach_f = frame;
    s_coach_t = frames;
}

void NEOGEO_USER cx_coach_events(const CxWorld *w)
{
    uint32_t ev = w->ev;
    uint8_t mine = (uint8_t)(w->human[0] ? 0u : 1u);
    if (ev & CX_EV_GOAL) {
        uint8_t ours = (uint8_t)((w->scorer != CX_NOBODY && (w->scorer >= 11u) == mine) != (w->goal_kind == CX_G_OWN));
        cx_coach_pop(ours ? CX_CO_CO_JUMP : CX_CO_CO_TEAR, 170);
    } else if (ev & CX_EV_CARD) cx_coach_pop(CX_CO_CO_ARGUE, 110);
    else if (ev & (CX_EV_MISS | CX_EV_POST)) cx_coach_pop(CX_CO_CO_DESPAIR, 90);
    else if (ev & CX_EV_FOUL) cx_coach_pop(CX_CO_CO_SHOUT, 80);
    else if (ev & CX_EV_SAVE) cx_coach_pop(CX_CO_CO_CHEW, 80);
    else if (ev & CX_EV_SUB) cx_coach_pop(CX_CO_CO_SHOUT, 90);
    else if (ev & CX_EV_FUMBLE) cx_coach_pop(CX_CO_CO_KICK, 100);
}

void NEOGEO_USER cx_coach_step(uint16_t frame, uint8_t hold)
{
    /* he waits for the GOAL words to finish: they share the screen's sprites per line */
    static uint8_t shown;           /* frames he has been up: his box waits for his picture */
    if (!s_coach_t || hold) { cx_spr_hide(&cx_coach); cx_spr_hide(&cx_inset); shown = 0; return; }
    s_coach_t--;
    /* his picture's tiles can take a blank longer to land than the box's: the box first
     * showed empty (a blue square in play tests) */
    if (shown < 2u) shown++;
    else cx_spr_show(&cx_inset, SPR_INSET, CX_ART_INSET, 0, 232, 118, 0);
    /* despair and tears shake a little */
    cx_spr_show(&cx_coach, SPR_COACH, CX_ART_COACH, s_coach_f,
                (int16_t)(240 + ((frame >> 2) & 1u)), (int16_t)(208 - CX_COACH_AY), 0);
}

/* ---------------------------------------------------------------- the line */
void NEOGEO_USER cx_linesman_events(const CxWorld *w)
{
    if (w->ev & CX_EV_OFFSIDE) {
        s_lines_t = 120;
        s_lines_x = U(w->ball.x);
    }
}

void NEOGEO_USER cx_linesman_step(uint16_t frame)
{
    int16_t x, y;
    uint16_t s;
    if (!s_lines_t) { cx_spr_hide(&cx_lines); return; }
    s_lines_t--;
    cx_project(s_lines_x, -8, &x, &y, &s);
    x = (int16_t)(x - cx_camx);
    y = (int16_t)(y - cx_camy);
    /* he must be seen even if the camera has gone: he stands at the top of the screen */
    if (y < 76) y = 76;
    if (x < 40) x = 40;
    if (x > 280) x = 280;
    cx_spr_show(&cx_lines, SPR_LINES, CX_ART_LINES, (frame >> 3) & 1u ? CX_LN_LN_PANIC : CX_LN_LN_FLAG,
                (int16_t)(x - CX_LINES_AX), (int16_t)(y - CX_LINES_AY), 0);
}

/* ---------------------------------------------------------------- the foul */
/* Play has stopped, and the referee, a great man, has something to show for himself. A foul
 * is not always this much of an event: every second one or so. */
void NEOGEO_USER cx_foul_scene(const CxWorld *w)
{
    static const uint8_t k_gag[] = { CX_HR_REF_SANDWICH, CX_HR_REF_BOOK, CX_HR_REF_FLAT, CX_HR_REF_BLOW, CX_HR_REF_DIZZY, CX_HR_REF_IGNORE, CX_HR_REF_SHOCK, CX_HR_REF_CAT,
                                     CX_HR_REF_FLEE, CX_HR_REF_JIG, CX_HR_REF_LACE, CX_HR_REF_RICE, CX_HR_REF_WATCH, CX_HR_REF_YAWN, CX_HR_REF_SNEEZE, CX_HR_REF_CRY, CX_HR_REF_SALUTE };
    static const char *const k_said[] = { "REF: WRONG POCKET! ONE MOMENT.", "REF: LET ME CHECK THE RULES...", "REF: WHO PUT THAT GRASS THERE?!", "REF: PHEEEEEEEEP!", "REF: WHO HIT ME?",
                                          "REF: I CANNOT HEAR YOU!", "REF: MY MOUSTACHE!", "REF: THIS IS MY CAT. HE IS ANGRY.", "REF: HELP! SECURITY!", "REF: WHAT A WHISTLE THAT WAS!",
                                          "REF: ONE MOMENT, MY LACE.", "REF: MMF! FOUL! MMF!", "REF: I AM NOT PAID BY THE HOUR.", "REF: IS IT LUNCH YET?", "REF: AH... AH... CHOO!",
                                          "REF: NOBODY LIKES ME!", "REF: THE LAW IS THE LAW!" };
    uint16_t f, pressed;
    uint8_t g;
    (void)w;
    s_dice = (uint16_t)(s_dice * 25173u + 13849u);
#ifdef CX_PERF
    if (cx_test_gag) s_dice = 0;
#endif
    if (s_dice & 0x0100u) return;
    g = (uint8_t)((uint16_t)(s_dice >> 7) % (uint8_t)sizeof(k_gag));
#ifdef CX_PERF
    if (cx_test_gag) { g = (uint8_t)(cx_test_gag - 1u); cx_test_gag = 0; }
#endif
    cx_clear_row(TALK_ROW);
    cx_puts_center(TALK_ROW, k_said[g], CX_PAL_WHITE);
    for (f = 0; f < 90u; f++) {
        cx_hero_show(&cx_hero, SPR_HERO, CX_ART_HEROR, k_gag[g], 112, (int16_t)(96 + ((f >> 3) & 1u)), 0, BANK_REF);
        (void)cx_input(&pressed);
        if (f > 16u && (pressed & (CX_A | CX_B | CX_C | START1))) break;
    }
    cx_spr_hide(&cx_hero);
    cx_clear_row(TALK_ROW);
}

/* ---------------------------------------------------------------- the cards */
/* A manga moment: the referee, enormous, the card, and the guilty man. The match waits;
 * a button cuts it short. */
void NEOGEO_USER cx_card_scene(const CxWorld *w)
{
    uint8_t man = w->card_man, red = w->card_red;
    uint16_t art = cx_man_art(man) == CX_ART_KEEPER ? (uint16_t)CX_ART_HERO0 : (uint16_t)(CX_ART_HERO0 + (cx_man_art(man) - CX_ART_FIG0));
    uint16_t f, pressed, n = red ? 130u : 80u;
    char name[12];
    cx_man_name(w->team[man >= 11u], w->m[man].squad, name);
    cx_say_card(red);
    cx_say_man_later(w, man, red ? 112u : 94u);      /* "Yellow card!" ... and who */
    cx_clear_row(6);
    cx_clear_row(7);
    cx_clear_row(8);
    cx_clear_row(9);
    cx_puts_center(6, red ? "RED CARD!" : "YELLOW CARD", red ? CX_PAL_RED : CX_PAL_GOLD);
    cx_puts_center(7, name, CX_PAL_WHITE);
    for (f = 0; f < n; f++) {
        cx_hero_show(&cx_hero, SPR_HERO, CX_ART_HEROR, CX_HR_REF_CARD, 16, 96, 0, BANK_REF);
        cx_spr_show(&cx_bigcard, SPR_BIGCARD, CX_ART_CARDS, red, 30, (int16_t)(92 + ((f >> 3) & 1u)), 0);
        cx_hero_show(&cx_hero2, SPR_HERO2, (uint16_t)art,
                     red ? (f > 60u ? CX_H_WALK_CRY : CX_H_KNEEL) : ((f >> 4) & 1u ? CX_H_COMPLAIN : CX_H_DISBELIEF),
                     (int16_t)(red && f > 60u ? 196 + (f - 60u) * 2u : 196), 96, 1, (uint8_t)(BANK_MEN + man));
        (void)cx_input(&pressed);
        cx_voice_tick();
        if (f > 20u && (pressed & (CX_A | CX_B | CX_C | START1))) break;
    }
    cx_spr_hide(&cx_hero);
    cx_spr_hide(&cx_hero2);
    cx_spr_hide(&cx_bigcard);
    cx_clear_row(6);
    cx_clear_row(7);
}
