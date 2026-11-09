/*
 * cx_show.c -- Century XI: a match from the first whistle to the last.
 *
 * Runs the simulation a step a frame and shows it: the board, the captions that
 * say why something happened, half time, the review, the shoot-out.
 */
#include "cx_render.h"
#include "sdk/2d_engine/ng_join.h"
#ifdef CX_PERF
#include "sdk/2d_engine/ng_devmenu.h"
#include "sdk/2d_engine/ng_task.h"
#include "sdk/2d_engine/ng_fixscript.h"
/* QA builds: a self-test of the SDK's tasks and FIX scripts during a match. One
 * task counts every second frame; the other types a line and ends. */
uint16_t cx_perf_task[3];
static uint16_t s_task_a[128], s_task_b[192];
static void NEOGEO_USER task_count(void *arg)
{
    (void)arg;
    for (;;) { cx_perf_task[0]++; ng_task_sleep(2); }
}
static void NEOGEO_USER task_type(void *arg)
{
    /* typed on the bottom row, clear of the score box, and wiped */
    (void)arg;
    ng_fixscript_play("{w30}{30,26}{p2}{t2}TASK OK{w60}{30,26}{b8,1}", 0);
    cx_perf_task[1] = 1;
}
#endif
#include "games/centuryxi/artbox/generated/cx_hud.h"

#define U(v) ((int16_t)((v) >> 4))

uint8_t cx_result[2];
static CxWorld s_w;
#ifdef CX_PERF
#include "cx_internal.h"
volatile uint16_t cx_perf[10];
volatile uint8_t cx_test_card;       /* a QA script sets it: 1 a yellow, 2 a red, 3 a foul on the man on the ball */
volatile uint8_t cx_test_gag;         /* and this picks the referee's turn, plus one */
#endif

static const char *const k_stage[4] = { "FRIENDLY", "GROUP STAGE", "KNOCK-OUT", "THE FINAL" };
static const char *const k_why[7] = { "", "PERFECT TIMING!", "HE WAS COMMITTED!", "CLASS!", "TOO CLOSE!", "TOO EARLY!", "HE READ IT!" };

static const char *const k_kind[12] = { "GOAL", "WHAT A STRIKE!", "TAP IN!", "HEADER!", "DIVING HEADER!", "VOLLEY!", "OVERHEAD KICK!",
                                         "CHIPPED THE KEEPER!", "FREE KICK!", "PENALTY!", "REBOUND!", "OWN GOAL!" };

/* ---------------------------------------------------------------- the board
 * The score as television showed it in 1994, all on the FIX layer (no sprites): a navy
 * box with each side's flag, code and score in tall digits; the match clock with the
 * added time beside it, red through the last minute and the added time; under the box
 * the mood bar in the two shirts' colours; and a short status line that comes and goes.
 * Rows 3 to 5; captions start at row 6. */
#define HUD_Y      3u
#define CLOCK_X   27u
#define BAR_X      4u
#define BAR_CELLS 17u
#define PILL_X    25u
#define PILL_TEXT  9u

typedef struct {
    uint16_t clock, acc;          /* the clock as last counted, and its sixtieths */
    uint8_t min, sec, shown_sec, shown_min, added, warn, bar, mental[2], status, status_t, last_min;
} CxHud;
static CxHud s_hud;

static void NEOGEO_USER hud_block(uint8_t x, uint8_t y, const uint16_t *t, uint8_t w, uint8_t h, uint8_t pal)
{
    uint8_t i, j;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++) ng_fix_put_tile((uint8_t)(x + i), (uint8_t)(y + j), t[j * w + i], pal);
}

static void NEOGEO_USER hud_digit(uint8_t x, uint8_t d, uint8_t pal)
{
    hud_block(x, HUD_Y, d < 12u ? cx_hud_digit[d] : cx_hud_blank, 1, 2, pal);
}

/* the box's palette with the two shirts in it, the flags', and the warning one */
static void NEOGEO_USER hud_palettes(const CxWorld *w)
{
    uint16_t p[16];
    const uint16_t *k0 = cx_kit_words(w->team[0], cx_kit_for(w->team[0], w->team[1]));
    const uint16_t *k1 = cx_kit_words(w->team[1], cx_kit_for(w->team[1], w->team[0]));
    uint8_t i, pal;
    for (pal = 0; pal < 3u; pal++) {
        for (i = 0; i < 16u; i++) p[i] = cx_hud_pal[pal][i];
        if (pal != 1u) {
            p[CX_HUD_T0] = k0[1]; p[CX_HUD_T0S] = k0[0];
            p[CX_HUD_T1] = k1[1]; p[CX_HUD_T1S] = k1[0];
        }
        load_palettes(p, (uintptr_t)(PALETTES + ((uint16_t)(CX_HUD_PAL_BOX + pal) << 5)));
    }
}

static void NEOGEO_USER hud_names(const CxWorld *w)
{
    uint8_t a = w->team[0], b = w->team[1];
    cx_spr_hide(&cx_plate);
    hud_palettes(w);
    hud_block(3, HUD_Y, cx_hud_cap[0], 1, 2, CX_HUD_PAL_BOX);
    hud_block(4, HUD_Y, cx_hud_flag[a], 3, 2, CX_HUD_PAL_FLAG);
    hud_block(7, HUD_Y, cx_hud_code[a], 3, 2, CX_HUD_PAL_BOX);
    hud_block(10, HUD_Y, cx_hud_blank, 1, 2, CX_HUD_PAL_BOX);
    hud_block(16, HUD_Y, cx_hud_blank, 1, 2, CX_HUD_PAL_BOX);
    hud_block(17, HUD_Y, cx_hud_code[b], 3, 2, CX_HUD_PAL_BOX);
    hud_block(20, HUD_Y, cx_hud_flag[b], 3, 2, CX_HUD_PAL_FLAG);
    hud_block(23, HUD_Y, cx_hud_cap[1], 1, 2, CX_HUD_PAL_BOX);
    hud_digit(13, 11, CX_HUD_PAL_BOX);
    /* the clock's own box */
    hud_block(CLOCK_X, HUD_Y, cx_hud_cap[0], 1, 2, CX_HUD_PAL_BOX);
    hud_block((uint8_t)(CLOCK_X + 8u), HUD_Y, cx_hud_cap[1], 1, 2, CX_HUD_PAL_BOX);
    ng_fix_put_tile(BAR_X, (uint8_t)(HUD_Y + 2u), cx_hud_bar[0], CX_HUD_PAL_BOX);
    ng_fix_put_tile((uint8_t)(BAR_X + BAR_CELLS + 1u), (uint8_t)(HUD_Y + 2u), cx_hud_bar[1], CX_HUD_PAL_BOX);
    s_hud.shown_sec = s_hud.shown_min = s_hud.added = s_hud.warn = s_hud.bar = 0xFFu;
    s_hud.mental[0] = w->mental[0];
    s_hud.mental[1] = w->mental[1];
    s_hud.status = 0;
    s_hud.status_t = 1;           /* clears the status line at the next draw */
}

/* The status line: a little picture and up to nine letters, for a few seconds. */
enum { ST_NONE, ST_INJURY, ST_TEN, ST_ATTACK, ST_DEFEND, ST_BALANCE, ST_LAST, ST_ADDED, ST_GOLDEN, ST_JOIN };
static void NEOGEO_USER hud_status(uint8_t what, uint8_t frames)
{
    static const char *const k_text[] = { "", "INJURY", "TEN MEN", "ATTACK!", "DEFEND!", "BALANCED", "LAST MIN!", "STOPPAGE", "GOLDEN G", "P2 JOINS!" };
    static const uint8_t k_icon[] = { 0, 0, 1, 3, 4, 3, 2, 2, 2 };
    const char *t = k_text[what];
    uint8_t i, j;
    s_hud.status = what;
    s_hud.status_t = frames;
    ng_fix_put_tile(PILL_X, (uint8_t)(HUD_Y + 2u), cx_hud_pill[0], CX_HUD_PAL_BOX);
    ng_fix_put_tile((uint8_t)(PILL_X + 1u), (uint8_t)(HUD_Y + 2u), cx_hud_icon[k_icon[what]], CX_HUD_PAL_BOX);
    for (i = 0; i < PILL_TEXT; i++) {
        char c = *t ? *t++ : ' ';
        for (j = 0; cx_hud_chars[j] && cx_hud_chars[j] != c; j++) {}
        ng_fix_put_tile((uint8_t)(PILL_X + 2u + i), (uint8_t)(HUD_Y + 2u), cx_hud_char[cx_hud_chars[j] ? j : 0], CX_HUD_PAL_BOX);
    }
    ng_fix_put_tile((uint8_t)(PILL_X + 2u + PILL_TEXT), (uint8_t)(HUD_Y + 2u), cx_hud_pill[1], CX_HUD_PAL_BOX);
}

static void NEOGEO_USER hud_draw(const CxWorld *w, uint8_t *shown_score, uint16_t *shown_time)
{
    uint8_t half_min = (uint8_t)(w->half < 2u ? 45u : 15u);
    uint8_t base = (uint8_t)(w->half == 0u ? 0u : (w->half == 1u ? 45u : (w->half == 2u ? 90u : 105u)));
    uint8_t warn, t;
    /* the score: the home side's right-aligned against the dash, the away side's left */
    if (shown_score[0] != w->score[0]) {
        uint8_t v = w->score[0];
        shown_score[0] = v;
        hud_digit(11, v >= 10u ? (uint8_t)(v / 10u) : 12u, CX_HUD_PAL_BOX);
        hud_digit(12, (uint8_t)(v % 10u), CX_HUD_PAL_BOX);
    }
    if (shown_score[1] != w->score[1]) {
        uint8_t v = w->score[1];
        shown_score[1] = v;
        hud_digit(14, v >= 10u ? (uint8_t)(v / 10u) : (uint8_t)(v % 10u), CX_HUD_PAL_BOX);
        hud_digit(15, v >= 10u ? (uint8_t)(v % 10u) : 12u, CX_HUD_PAL_BOX);
    }
    /* the clock, counted on without a division a frame: sixty seconds to a shown minute */
    if (*shown_time == 0xFFFFu) { *shown_time = 0; s_hud.shown_sec = s_hud.shown_min = s_hud.added = s_hud.warn = s_hud.bar = 0xFFu; }
    if (w->clock < s_hud.clock) { s_hud.clock = 0; s_hud.acc = 0; s_hud.min = s_hud.sec = 0; s_hud.last_min = 0; }
    while (s_hud.clock < w->clock) {
        s_hud.clock++;
        s_hud.acc = (uint16_t)(s_hud.acc + 60u);
        while (s_hud.acc >= w->fpm) {
            s_hud.acc = (uint16_t)(s_hud.acc - w->fpm);
            if (++s_hud.sec >= 60u) { s_hud.sec = 0; s_hud.min++; }
        }
    }
    warn = (uint8_t)(s_hud.min + 1u >= half_min || w->added);
    if (warn != s_hud.warn) { s_hud.warn = warn; s_hud.shown_sec = s_hud.shown_min = 0xFFu; }
    if (s_hud.shown_min != s_hud.min) {
        uint8_t m = (uint8_t)((base + s_hud.min) % 100u), pal = warn ? CX_HUD_PAL_WARN : CX_HUD_PAL_BOX;
        s_hud.shown_min = s_hud.min;
        hud_digit((uint8_t)(CLOCK_X + 1u), (uint8_t)(m / 10u), pal);
        hud_digit((uint8_t)(CLOCK_X + 2u), (uint8_t)(m % 10u), pal);
        hud_digit((uint8_t)(CLOCK_X + 3u), 10, pal);
    }
    if (s_hud.shown_sec != s_hud.sec) {
        uint8_t pal = warn ? CX_HUD_PAL_WARN : CX_HUD_PAL_BOX;
        s_hud.shown_sec = s_hud.sec;
        hud_digit((uint8_t)(CLOCK_X + 4u), (uint8_t)(s_hud.sec / 10u), pal);
        hud_digit((uint8_t)(CLOCK_X + 5u), (uint8_t)(s_hud.sec % 10u), pal);
    }
    /* the warning clock flashes: its ink swapped in the palette, no cell rewritten */
    if (warn && (w->frame & 7u) == 0u) {
        volatile uint16_t *pw = (volatile uint16_t *)(PALETTES + ((uint16_t)CX_HUD_PAL_WARN << 5));
        uint8_t on = (uint8_t)((w->frame >> 3) & 1u);
        pw[CX_HUD_INK] = on ? cx_hud_pal[0][CX_HUD_INK] : cx_hud_pal[2][CX_HUD_INK];
        pw[CX_HUD_INK2] = on ? cx_hud_pal[0][CX_HUD_INK2] : cx_hud_pal[2][CX_HUD_INK2];
    }
    if (s_hud.added != w->added) {
        s_hud.added = w->added;
        hud_block((uint8_t)(CLOCK_X + 6u), HUD_Y, w->added ? cx_hud_plus[w->added > 9u ? 9u : w->added] : cx_hud_blank, w->added ? 2u : 1u, 2, CX_HUD_PAL_BOX);
        if (!w->added) hud_block((uint8_t)(CLOCK_X + 7u), HUD_Y, cx_hud_blank, 1, 2, CX_HUD_PAL_BOX);
        else hud_status(ST_ADDED, 180);
    }
    if (s_hud.min + 1u == half_min && !s_hud.last_min && !w->added) { s_hud.last_min = 1; hud_status(ST_LAST, 150); }
    /* the mood bar: the share of the momentum, in pixels over the bar's 17 cells */
    if ((w->frame & 15u) == 0u) {
        uint16_t m0 = w->momentum[0], m1 = w->momentum[1];
        uint8_t px = (uint8_t)(m0 + m1 ? (uint16_t)(m0 * (BAR_CELLS * 8u) / (m0 + m1)) : BAR_CELLS * 4u);
        if (px != s_hud.bar) {
            uint8_t c, full = (uint8_t)(px >> 3), rem = (uint8_t)(px & 7u);
            s_hud.bar = px;
            for (c = 0; c < BAR_CELLS; c++) {
                uint16_t tile = c < full ? cx_hud_bar[2] : cx_hud_bar[3];
                if (c == full && rem) tile = cx_hud_bar[rem <= 2u ? 4u : (rem <= 4u ? 5u : 6u)];
                ng_fix_put_tile((uint8_t)(BAR_X + 1u + c), (uint8_t)(HUD_Y + 2u), tile, CX_HUD_PAL_BOX);
            }
        }
    }
    /* the status line: the physio's on, a side down to ten, a coach's new orders */
    if (w->injured != CX_NOBODY && w->m[w->injured].state == CX_M_ROLL && s_hud.status != ST_INJURY) hud_status(ST_INJURY, 120);
    if ((w->ev & CX_EV_CARD) && w->card_red) hud_status(ST_TEN, 160);
    for (t = 0; t < 2u; t++) {
        if (s_hud.mental[t] != w->mental[t]) {
            if (s_hud.mental[t] != 0xFFu && w->phase == CX_P_PLAY)
                hud_status(w->mental[t] == 2u ? ST_ATTACK : (w->mental[t] == 0u ? ST_DEFEND : ST_BALANCE), 140);
            s_hud.mental[t] = w->mental[t];
        }
    }
    if (s_hud.status_t && !--s_hud.status_t) {
        uint8_t i;
        s_hud.status = 0;
        for (i = 0; i < PILL_TEXT + 3u; i++) ng_fix_puts((uint8_t)(PILL_X + i), (uint8_t)(HUD_Y + 2u), " ", CX_PAL_WHITE);
    }
}

static void NEOGEO_USER phase_caption(const CxWorld *w)
{
    const char *s = 0;
    uint8_t human = w->human[w->restart_team];
    cx_clear_row(6);
    cx_clear_row(25);
    switch (w->phase) {
    case CX_P_KICKOFF:  s = "KICK-OFF"; break;
    case CX_P_THROW:    s = "THROW-IN"; break;
    case CX_P_GOALKICK: s = "GOAL KICK"; break;
    case CX_P_CORNER:   s = "CORNER KICK"; break;
    case CX_P_FREEKICK: s = "FREE KICK"; break;
    case CX_P_PENALTY:  s = "PENALTY!"; break;
    default: break;
    }
    if (s) {
        cx_puts_center(6, s, CX_PAL_GOLD);
        if (human && w->phase == CX_P_FREEKICK) cx_puts_center(25, "A PASS B SHOOT C LOB D AIM VIEW", CX_PAL_WHITE);
        else if (human && w->phase == CX_P_CORNER) cx_puts_center(25, "A SHORT B NEAR C FAR D AIM VIEW", CX_PAL_WHITE);
        else if (human && w->phase == CX_P_THROW) cx_puts_center(25, "A FEET  B DOWN THE LINE  C LONG", CX_PAL_WHITE);
        else if (human && w->phase != CX_P_PENALTY) cx_puts_center(25, "A PASS   C LOB", CX_PAL_WHITE);
    }
}

/* The caption for what just happened, and why. */
/* The commentator's line: a player's name on what he just did, "NAME" then the words. */
static uint8_t NEOGEO_USER named(const CxWorld *w, uint8_t man, const char *before, const char *after, char *out)
{
    char name[12];
    uint8_t n = 0, k;
    if (man >= CX_MEN) return 0;
    cx_man_name(w->team[man >= 11u], w->m[man].squad, name);
    for (k = 0; before[k] && n < 30u; k++) out[n++] = before[k];
    for (k = 0; name[k] && n < 30u; k++) out[n++] = name[k];
    for (k = 0; after[k] && n < 36u; k++) out[n++] = after[k];
    out[n] = 0;
    return 1;
}

static void NEOGEO_USER calls(const CxWorld *w, uint8_t *call_t)
{
    const char *s = 0;
    char line[40];
    uint8_t pal = CX_PAL_CYAN;
    const CxBall *b = &w->ball;
    /* the big moments with a name in them */
    if ((w->ev & CX_EV_SAVE) && b->last < CX_MEN && w->m[b->last].role == 0 && named(w, b->last, "SAVED BY ", "!", line)) s = line;
    else if ((w->ev & CX_EV_SPECIAL) && b->kind == CX_G_BIKE && named(w, b->striker, "", " OVERHEAD KICK!", line)) { s = line; pal = CX_PAL_GREEN; }
    else if ((w->ev & CX_EV_SPECIAL) && b->kind == CX_G_VOLLEY && named(w, b->striker, "", " ON THE VOLLEY!", line)) { s = line; pal = CX_PAL_GREEN; }
    else if ((w->ev & CX_EV_SPECIAL) && b->kind == CX_G_DIVING && named(w, b->striker, "DIVING HEADER ", "!", line)) { s = line; pal = CX_PAL_GREEN; }
    else if ((w->ev & CX_EV_SHOT) && b->kind == CX_G_HEADER && named(w, b->striker, "", " HEADS FOR GOAL!", line)) s = line;
    else if ((w->ev & CX_EV_SHOT) && (b->kind == CX_G_SHOT || b->kind == CX_G_LONG) && named(w, b->striker, "", " SHOOTS!", line)) s = line;
    else if ((w->ev & CX_EV_THROUGH) && named(w, b->last, "THROUGH BALL BY ", "", line)) s = line;
    else if ((w->ev & CX_EV_ONETWO) && named(w, b->last, "ONE-TWO, ", "!", line)) { s = line; pal = CX_PAL_GREEN; }
    if (s) {
        cx_clear_row(8);
        cx_puts_center(8, s, pal);
        *call_t = 70;
        return;
    }
    if (w->ev & CX_EV_SAVE) s = "GREAT SAVE!";
    else if (w->ev & CX_EV_POST) s = "OFF THE POST!";
    else if (w->ev & CX_EV_OFFSIDE) { s = "OFFSIDE!"; pal = CX_PAL_RED; }
    else if (w->ev & CX_EV_HANDBALL) { s = "HAND BALL!"; pal = CX_PAL_RED; }
    else if (w->ev & CX_EV_DIVE) { s = "DIVE!"; pal = CX_PAL_RED; }
    else if (w->ev & CX_EV_ADV) s = "ADVANTAGE!";
    else if (w->ev & CX_EV_FOUL) { s = "FOUL!"; pal = CX_PAL_GOLD; }
    else if (w->ev & CX_EV_NUTMEG) { s = "NUTMEG!"; pal = CX_PAL_GREEN; }
    else if (w->ev & CX_EV_FEINT) {
        s = k_why[w->duel_why];
        pal = (w->duel_res == CX_DUEL_BEAT || w->duel_res == CX_DUEL_STEP) ? CX_PAL_GREEN : CX_PAL_RED;
    } else if (w->ev & CX_EV_ADDED) {
        char plus[] = "ADDED TIME +0";
        plus[12] = (char)('0' + w->added);
        cx_clear_row(8);
        cx_puts_center(8, plus, CX_PAL_GOLD);
        *call_t = 120;
        return;
    }
    if (s) {
        cx_clear_row(8);
        cx_puts_center(8, s, pal);
        *call_t = 70;
    } else if (*call_t && !--*call_t) {
        cx_clear_row(8);
    }
}

/* The man on the ball with a defender square in front: tell the player he can try it. */
static void NEOGEO_USER duel_prompt(const CxWorld *w, uint8_t *shown)
{
    uint8_t on = (uint8_t)(w->duel_a != CX_NOBODY && w->human[w->duel_a >= 11u] && w->ctrl[w->duel_a >= 11u] == w->duel_a);
    if (on != *shown) {
        *shown = on;
        if (on) cx_puts_center(25, "A+B FEINT", CX_PAL_GREEN);
        else cx_clear_row(25);
    }
}

static void NEOGEO_USER match_screen(CxWorld *w, uint8_t stage)
{
    cx_screen_clear();
    setBACKDROP(BLACK);
    cx_side_palettes(w);
    cx_pitch_load(w->stadium);
    cx_crowd_kits(w->team[0], w->team[1]);
    cx_men_reset();
    cx_camera(w, 1);
    hud_names(w);
    (void)stage;
}

/* A player winding up a shot: a power gauge under his half of the screen, drawn only
 * when it changes length, and gone the moment the shot is struck. */
static uint8_t s_gauge[2];
static void NEOGEO_USER shot_gauge(const CxWorld *w)
{
    static const char k_bar[] = "##########";
    uint8_t t;
    for (t = 0; t < 2u; t++) {
        uint8_t n = (uint8_t)(w->human[t] && w->charge[t] ? 1u + (w->charge[t] * 9u) / CX_CHARGE_MAX : 0u);
        uint8_t x = (uint8_t)(t ? 23u : 5u);
        if (n == s_gauge[t]) continue;
        ng_fix_clear_rect(x, 26, 16, 1, CX_PAL_WHITE);
        if (n) {
            ng_fix_puts(x, 26, "POWER", CX_PAL_WHITE);
            ng_fix_puts((uint8_t)(x + 6u), 26, k_bar + (10u - n), (uint8_t)(n >= 8u ? CX_PAL_RED : (n >= 5u ? CX_PAL_GOLD : CX_PAL_GREEN)));
        }
        s_gauge[t] = n;
    }
}

/* The GOAL words wait just under the screen between goals, written: their first
 * full write (with the padding a shrinkable picture needs) costs more than a busy
 * vertical blank has, so it is done once at kick-off and a goal only moves them. */
#define WORD_PARK_Y 260
static void NEOGEO_USER park_word(uint8_t i)
{
    cx_spr_show_zoom(&cx_word[i], (uint16_t)(SPR_WORD + i * 16u), CX_ART_GOALWORD, 0, 32, WORD_PARK_Y, 255);
}

/* GOAL! GOAL! GOAL!: the three words zoom in one after another as the announcer
 * shouts them and land with a shake, over the players' own celebration. */
static void NEOGEO_USER goal_words(const CxWorld *w)
{
    static const uint8_t k_at[3] = {16, 34, 52};
    uint16_t t = w->phase_t;
    uint8_t i;
    for (i = 0; i < 3u; i++) {
        if (t >= k_at[i] && t < 124u) {
            int16_t age = (int16_t)(t - k_at[i]);
            if (age < 8) {
                /* it zooms in from a third of its size, then lands with a shake */
                cx_spr_show_zoom(&cx_word[i], (uint16_t)(SPR_WORD + i * 16u), CX_ART_GOALWORD, 0, 32, (int16_t)(28 + i * 58),
                                 (uint8_t)(80 + age * 25));
            } else {
                int16_t x = (int16_t)(32 + (age < 16 ? (age & 1 ? 6 : -6) : 0));
                cx_spr_show_zoom(&cx_word[i], (uint16_t)(SPR_WORD + i * 16u), CX_ART_GOALWORD, 0, x, (int16_t)(28 + i * 58), 255);
            }
        } else {
            park_word(i);
        }
    }
}

/* The goal's kind and the scorer, one line under the REPLAY banner while it runs. Printed
 * at the goal, two rows sat over the coach's words and the stand's boards. */
static void NEOGEO_USER goal_caption(const CxWorld *w)
{
    char line[40];
    uint8_t sc = w->scorer, n = 0, k;
    const char *kind = k_kind[w->goal_kind];
    cx_clear_row(24);
    cx_clear_row(25);
    if (sc == CX_NOBODY) return;
    while (kind[n] && n < 20u) { line[n] = kind[n]; n++; }
    line[n++] = ' ';
    cx_man_name(w->team[sc >= 11u], w->m[sc].squad, line + n);
    for (k = n; line[k] && k < 38u; k++) { }
    line[k] = 0;
    cx_puts_center(9, line, CX_PAL_GOLD);
}

/* ------------------------------------------------------------- the 3D penalty */
/* From behind the spot, looking at the goal: the taker from the back, the keeper facing
 * us and the ball shrinking as it flies. Where it goes: stick left, centre or right
 * (the column), up for high. The keeper dives a column. Answers 1 for a goal. */
#define PEN_GOAL_L  96
#define PEN_GOAL_R  224
#define PEN_LINE_Y  134
#define PEN_BAR_Y   72

static uint8_t NEOGEO_USER pen_view(const CxWorld *w, uint8_t taker, uint8_t keeper, uint8_t kicker_human, uint8_t keeper_human,
                                    uint8_t kicker_pad, uint32_t *dice)
{
    static const int16_t k_col[3] = {160, 118, 202};        /* centre, left, right */
    uint8_t art = cx_man_art(taker);
    uint16_t hero = art == CX_ART_KEEPER ? CX_ART_HERO0 : (uint16_t)(CX_ART_HERO0 + (art - CX_ART_FIG0));
    uint8_t col = 0, high = 0, dive = 0, scored, t, kband = (uint8_t)(2u * CX_KEEP_FRAMES);
    uint8_t kdive = 0xFFu, khigh = 0, early = 0;      /* a player's keeper: the dive he committed to */
    uint16_t f, pressed, p2, joy, joy2;
    int16_t tx, ty, kx = 160;
    uint32_t x = *dice;
    cx_screen_clear();
    cx_picture_show(CX_ART_PEN_VIEW);
    cx_music_penalty();
    cx_puts_center(4, "PENALTY", CX_PAL_GOLD);
    if (kicker_human) cx_puts_center(27, "LEFT RIGHT UP AIM  A OR B SHOOT", CX_PAL_WHITE);
    else if (keeper_human) cx_puts_center(27, "STICK MOVES  A DIVES  UP FOR HIGH", CX_PAL_WHITE);
    for (f = 0; f < 300u; f++) {
        joy = cx_input(&pressed);
        joy2 = cx_input2(&p2);
        if (kicker_pad) { uint16_t q = joy; joy = joy2; joy2 = q; q = pressed; pressed = p2; p2 = q; }
        /* a player's keeper shuffles along his line and may commit early: the taker sees it */
        if (keeper_human) {
            if ((joy2 & CX_LEFT) && kx > 140) kx--;
            if ((joy2 & CX_RIGHT) && kx < 180) kx++;
            if (kdive == 0xFFu && (p2 & (CX_A | CX_B))) {
                kdive = (uint8_t)((joy2 & CX_LEFT) ? 1u : ((joy2 & CX_RIGHT) ? 2u : 0u));
                khigh = (uint8_t)((joy2 & CX_UP) != 0);
                early = 1;
            }
        }
        cx_figure_show(&cx_men[1], SPR_MEN + 4u, CX_ART_KEEPER, (uint8_t)(kband + CX_K_GK_READY_F),
                       (int16_t)(kx + ((f >> 4) & 1u ? 2 : -2)), PEN_LINE_Y, 0, (uint8_t)(BANK_MEN + keeper));
        cx_hero_show(&cx_hero, SPR_HERO, hero, CX_H_STAND_B, 40, 112, 0, (uint8_t)(BANK_MEN + taker));
        cx_spr_show(&cx_ballz, SPR_BALLZ, CX_ART_BALLZ, 0, 144, 186, 0);
        if (kicker_human) {
            col = (uint8_t)((joy & CX_LEFT) ? 1u : ((joy & CX_RIGHT) ? 2u : 0u));
            high = (uint8_t)((joy & CX_UP) != 0);
            cx_spr_show(&cx_cursor[0], SPR_CURSOR, CX_ART_CURSOR, (uint8_t)((f >> 4) & 1u), (int16_t)(k_col[col] - 8),
                        (int16_t)(high ? PEN_BAR_Y + 10 : PEN_LINE_Y - 34), 0);
        }
        if (f < 30u) continue;
        if (kicker_human && (pressed & (CX_A | CX_B))) break;
        if (!kicker_human && f > 100u) break;
    }
    cx_spr_hide(&cx_cursor[0]);
    /* the CPU's choices come from the dice, the player's from the stick */
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    if (!kicker_human) { col = (uint8_t)((x >> 4) % 3u); high = (uint8_t)((x >> 9) & 1u); }
    /* the run-up: a player's keeper can still pick his side until the ball is struck */
    for (f = 0; f < 26u; f++) {
        uint16_t q1, j1 = cx_input(&q1);              /* the frame's wait, and both sticks */
        joy2 = cx_input2(&p2);
        if (kicker_pad) { joy2 = j1; p2 = q1; }
        cx_hero_show(&cx_hero, SPR_HERO, hero, f < 16u ? CX_H_KICK_B1 : CX_H_KICK_B2, (int16_t)(40 + (int16_t)(f < 20u ? f * 3u : 60u)), (int16_t)(112 - (f >> 2)), 0, (uint8_t)(BANK_MEN + taker));
        if (keeper_human) {
            if ((joy2 & CX_LEFT) && kx > 140) kx--;
            if ((joy2 & CX_RIGHT) && kx < 180) kx++;
            if (kdive == 0xFFu && (p2 & (CX_A | CX_B))) {
                kdive = (uint8_t)((joy2 & CX_LEFT) ? 1u : ((joy2 & CX_RIGHT) ? 2u : 0u));
                khigh = (uint8_t)((joy2 & CX_UP) != 0);
                if (f < 12u) early = 1;
            }
            cx_figure_show(&cx_men[1], SPR_MEN + 4u, CX_ART_KEEPER, (uint8_t)(kband + CX_K_GK_READY_F), kx, PEN_LINE_Y, 0, (uint8_t)(BANK_MEN + keeper));
        }
    }
    if (keeper_human) {
        /* not committed: the way the stick points as it is struck. Shuffled off centre, he
         * covers that side better and leaves the other open */
        if (kdive == 0xFFu) {
            dive = (uint8_t)((joy2 & CX_LEFT) ? 1u : ((joy2 & CX_RIGHT) ? 2u : 0u));
            khigh = (uint8_t)((joy2 & CX_UP) != 0);
        } else dive = kdive;
        /* gone too soon, the CPU's taker sees it half the time and puts it the other way */
        if (!kicker_human && early && dive == col && ((x >> 21) & 1u)) col = (uint8_t)(dive == 1u ? 2u : (dive == 2u ? 1u : 1u + ((x >> 22) & 1u)));
    } else dive = (uint8_t)((x >> 13) % 3u);
    *dice = x;
    scored = cx_pen_roll(col, dive, w->m[taker].shot, cx_squad_stat(w->team[keeper >= 11u], 5), dice);
    if (keeper_human) {
        /* the right side but the wrong height: a high one over a low dive goes in more often */
        if (!scored && col == dive && high != khigh && ((x >> 25) & 1u)) scored = 1;
        /* stepped across to the side it went, he gets a hand to it more often */
        if (scored && col == dive && ((col == 1u && kx < 154) || (col == 2u && kx > 166)) && ((x >> 27) & 3u) == 0) scored = 0;
    }
    tx = k_col[col];
    ty = high ? PEN_BAR_Y + 12 : PEN_LINE_Y - 20;
    if (!scored && col != dive) { tx = (int16_t)(col == 1u ? PEN_GOAL_L - 18 : (col == 2u ? PEN_GOAL_R + 18 : tx)); ty = (int16_t)(PEN_BAR_Y - 14); }
    cx_sfx_kick();
    /* the flight: the ball shrinks away, the keeper goes */
    for (t = 0; t < 24u; t++) {
        int16_t bx = (int16_t)(160 + (tx - 160) * (int16_t)t / 24);
        int16_t by = (int16_t)(206 + (ty - 206) * (int16_t)t / 24 - (int16_t)(t < 12u ? t : 24u - t));
        uint8_t size = (uint8_t)(t < 4u ? 0u : (t < 9u ? 1u : (t < 14u ? 2u : (t < 20u ? 3u : 4u))));
        uint8_t kf = CX_K_GK_READY_F;
        int16_t ky = PEN_LINE_Y;
        cx_frame_wait();
        if (dive) { kx = (int16_t)(kx + (dive == 1u ? -3 : 3)); kf = t < 6u ? CX_K_GK_READY_F : CX_K_GK_FLAT; ky = (int16_t)(PEN_LINE_Y - 8 - (khigh && t >= 6u ? 10 : 0)); }
        else if (high || khigh) { kf = CX_K_GK_HOP; ky = (int16_t)(PEN_LINE_Y - 6); }
        cx_figure_show(&cx_men[1], SPR_MEN + 4u, CX_ART_KEEPER, (uint8_t)(kband + kf), kx, ky, (uint8_t)(dive == 1u), (uint8_t)(BANK_MEN + keeper));
        cx_spr_show(&cx_ballz, SPR_BALLZ, CX_ART_BALLZ, size, (int16_t)(bx - 16), (int16_t)(by - 16), 0);
    }
    cx_clear_row(4);
    cx_clear_row(27);
    cx_puts_center(4, scored ? "GOAL!" : (col == dive ? "SAVED!" : "MISSED!"), scored ? CX_PAL_GOLD : CX_PAL_RED);
    if (scored) cx_sfx_goal(); else if (col == dive) cx_sfx_save();
    cx_figure_show(&cx_men[1], SPR_MEN + 4u, CX_ART_KEEPER, (uint8_t)(kband + (scored ? CX_K_GK_SIT : CX_K_GK_HOP)), kx, PEN_LINE_Y, (uint8_t)(dive == 1u), (uint8_t)(BANK_MEN + keeper));
    cx_hero_show(&cx_hero, SPR_HERO, hero, scored ? CX_H_CELEB_JUMP : CX_H_DISBELIEF, 112, 112, 0, (uint8_t)(BANK_MEN + taker));
    cx_spr_hide(&cx_ballz);
    cx_wait(60);
    return scored;
}

/* ------------------------------------------------------------- shoot-out */
/* Five each, then one each until it splits. Nothing is forced: it ends when the numbers do. */
static void NEOGEO_USER so_board(const CxShootout *so, uint8_t team, uint8_t foe)
{
    char a[4], b[4];
    cx_squad_code(team, a);
    cx_squad_code(foe, b);
    ng_fix_puts(4, 2, a, CX_PAL_GOLD);
    ng_fix_puts(33, 2, b, CX_PAL_CYAN);
    cx_put_2d(8, 2, so->score[0], CX_PAL_WHITE);
    cx_put_2d(30, 2, so->score[1], CX_PAL_WHITE);
}

static uint8_t NEOGEO_USER shootout(uint8_t team, uint8_t foe, uint8_t humans)
{
    CxShootout so;
    CxWorld *w = &s_w;
    uint32_t dice = 787u + team * 33u + foe + w->frame;
    cx_shootout_init(&so, 0, dice);
    cx_side_palettes(w);
    while (!so.over) {
        uint8_t t = so.turn;
        uint8_t taker = (uint8_t)(t * 11u + 10u - (so.taken[t] % 10u));
        uint8_t keeper = (uint8_t)((t ^ 1u) * 11u);
        uint8_t scored = pen_view(w, taker, keeper, (uint8_t)((humans >> t) & 1u), (uint8_t)((humans >> (t ^ 1u)) & 1u), t, &dice);
        cx_shootout_record(&so, scored);
        so_board(&so, team, foe);
        cx_wait(50);
    }
    cx_wait(60);
    return so.winner == 0u;
}

uint8_t NEOGEO_USER cx_penalties(uint8_t team, uint8_t foe, uint8_t humans)
{
    uint8_t r;
    cx_init(&s_w, team, foe, humans, 0);
    r = shootout(team, foe, humans);
    return r;
}

/* ------------------------------------------------------------- tactics and subs */
static void NEOGEO_USER tactics_screen(CxWorld *w, uint8_t t)
{
    static const char *const k_form[4] = { "4-4-2", "4-3-3", "3-5-2", "5-3-2" };
    static const char *const k_mind[3] = { "PARK THE BUS", "BALANCED", "ALL OUT ATTACK" };
    static const char *const k_press[2] = { "SIT BACK", "HIGH PRESS" };
    uint8_t row = 0, dirty = 1;
    uint16_t pressed;
    ng_fix_clear();
    for (;;) {
        (void)cx_input(&pressed);
        if (pressed & CX_UP) { row = row ? (uint8_t)(row - 1u) : 2u; dirty = 1; }
        if (pressed & CX_DOWN) { row = (uint8_t)(row + 1u); if (row > 2u) row = 0; dirty = 1; }
        if (pressed & (CX_LEFT | CX_RIGHT)) {
            if (row == 0u) cx_set_shape(w, t, (uint8_t)(w->form[t] + 1u));
            else if (row == 1u) w->tactic[t] = (uint8_t)((w->tactic[t] + 1u) % 3u);
            else w->press[t] ^= 1u;
            cx_sfx_menu();
            dirty = 1;
        }
        if (dirty) {
            dirty = 0;
            cx_puts_center(7, "TACTICS", CX_PAL_GOLD);
            cx_clear_row(11); cx_clear_row(13); cx_clear_row(15);
            ng_fix_puts(7, 11, "SHAPE", row == 0u ? CX_PAL_GOLD : CX_PAL_WHITE);
            ng_fix_puts(19, 11, k_form[w->form[t]], CX_PAL_CYAN);
            ng_fix_puts(7, 13, "MENTALITY", row == 1u ? CX_PAL_GOLD : CX_PAL_WHITE);
            ng_fix_puts(19, 13, k_mind[w->tactic[t]], CX_PAL_CYAN);
            ng_fix_puts(7, 15, "PRESSING", row == 2u ? CX_PAL_GOLD : CX_PAL_WHITE);
            ng_fix_puts(19, 15, k_press[w->press[t]], CX_PAL_CYAN);
            cx_puts_center(20, "LEFT RIGHT CHANGE  A DONE", CX_PAL_WHITE);
        }
        if (pressed & (CX_A | CX_B | START1)) return;
    }
}

static void NEOGEO_USER subs_screen(CxWorld *w, uint8_t t)
{
    uint8_t row = 1, bench = 0, dirty = 1, k, stage = 0;
    uint16_t pressed;
    char name[12];
    ng_fix_clear();
    for (;;) {
        (void)cx_input(&pressed);
        if (stage == 0u) {
            if (pressed & CX_UP) { row = row > 1u ? (uint8_t)(row - 1u) : 10u; dirty = 1; }
            if (pressed & CX_DOWN) { row = row < 10u ? (uint8_t)(row + 1u) : 1u; dirty = 1; }
            if (pressed & CX_A) { stage = 1; dirty = 1; continue; }
        } else {
            if (pressed & CX_UP) { bench = bench ? (uint8_t)(bench - 1u) : 4u; dirty = 1; }
            if (pressed & CX_DOWN) { bench = (uint8_t)((bench + 1u) % 5u); dirty = 1; }
            if (pressed & CX_A) {
                uint8_t ok = cx_sub_request(w, t, row, bench);
                cx_clear_row(24);
                cx_puts_center(24, ok ? "HE COMES ON AT THE NEXT STOPPAGE" : "NO SUBSTITUTIONS LEFT", ok ? CX_PAL_GREEN : CX_PAL_RED);
                cx_wait(70);
                return;
            }
        }
        if (dirty) {
            dirty = 0;
            cx_clear_row(5);
            cx_puts_center(5, stage ? "WHO COMES ON?" : "WHO COMES OFF?", CX_PAL_GOLD);
            for (k = 1; k <= 10u; k++) {
                const CxMan *m = &w->m[t * 11u + k];
                uint8_t y = (uint8_t)(6u + k * 2u - 1u);
                cx_clear_row(y);
                cx_man_name(w->team[t], m->squad, name);
                ng_fix_puts(5, y, name, stage == 0u && row == k ? CX_PAL_GOLD : (m->state == CX_M_OFF ? CX_PAL_RED : CX_PAL_WHITE));
                cx_put_2d(16, y, (uint16_t)(m->sta * 99u / 255u), m->sta < 96u ? CX_PAL_RED : CX_PAL_GREEN);
                if (k <= 5u) {
                    cx_man_name(w->team[t], (uint8_t)(10u + k), name);
                    ng_fix_puts(24, y, name, stage == 1u && bench == k - 1u ? CX_PAL_GOLD : CX_PAL_CYAN);
                }
            }
            ng_fix_puts(5, 6, "ON THE PITCH  FIT", CX_PAL_GREEN);
            ng_fix_puts(24, 6, "BENCH", CX_PAL_GREEN);
            cx_clear_row(27);
            cx_puts_center(27, "UP DOWN CHOOSE  A OK  B BACK", CX_PAL_WHITE);
        }
        if (pressed & CX_B) return;
    }
}

/* Answers 1 to leave the match. */
static uint8_t NEOGEO_USER pause_menu(CxWorld *w, uint8_t stage)
{
    static const char *const k_item[4] = { "RESUME", "TACTICS", "SUBSTITUTION", "QUIT MATCH" };
    uint8_t row = 0, dirty = 1, k, t = (uint8_t)(w->human[0] ? 0u : 1u);
    uint16_t pressed;
    for (;;) {
        (void)cx_input(&pressed);
        if (pressed & CX_UP) { row = row ? (uint8_t)(row - 1u) : 3u; dirty = 1; }
        if (pressed & CX_DOWN) { row = (uint8_t)((row + 1u) & 3u); dirty = 1; }
        if (dirty) {
            dirty = 0;
            cx_puts_center(9, "PAUSE", CX_PAL_GOLD);
            for (k = 0; k < 4u; k++) {
                cx_clear_row((uint8_t)(12u + k * 2u));
                cx_puts_center((uint8_t)(12u + k * 2u), k_item[k], k == row ? CX_PAL_GOLD : CX_PAL_WHITE);
            }
        }
        if (pressed & START1) break;
        if (pressed & CX_A) {
            if (row == 0u) break;
            if (row == 3u) return 1;
            if (row == 1u) tactics_screen(w, t); else subs_screen(w, t);
            ng_fix_clear();
            hud_names(w);
            dirty = 1;
        }
    }
    ng_fix_clear();
    hud_names(w);
    (void)stage;
    return 0;
}

static void NEOGEO_USER cancel_words(void)
{
    uint8_t i;
    for (i = 0; i < 3u; i++) park_word(i);
}

/* ------------------------------------------------------------- a match */
uint8_t cx_match_round;
uint8_t cx_match_on;
/* "Start has been pressed" (user.c). It lives in work RAM. In backup RAM at
 * D00100 the write was lost on the system ROMs that write-protect it while
 * the game runs (the US arcade one), so Start took a credit and never began
 * a game; a console has no backup RAM there at all. */
volatile uint8_t cx_start_latch;
static uint16_t s_blank_seen;      /* the blank count when the last frame's ticks were run */
static uint8_t s_catch_run;        /* the last frame paid a late frame's tick and drew nothing */
static uint8_t s_late_run;         /* late frames in a row */
static uint8_t s_heavy;            /* even an undrawn frame was late: ticks are paid with the drawing */
static uint8_t s_since_late;       /* frames since the last late one (set at the start: .data is not loaded) */
static uint8_t s_aim_shown;        /* the aim view's hint is on row 25 */
static uint8_t s_keep_shown;       /* and a player's keeper's, while it is in his hands */
static uint8_t s_att_shown;        /* and what C does near their goal: 1 the chip, 2 the cross */
static uint8_t s_att_t;            /* frames before that hint may change again */
static uint8_t s_goal_n, s_goal_team[24], s_goal_squad[24];     /* this match's scorers, for the championship */

/* The k-th goal of the last match: the scorer's nation and squad number (own goals are not
 * listed). Answers 0 past the last. */
uint8_t NEOGEO_USER cx_match_goal(uint8_t k, uint8_t *team, uint8_t *squad)
{
    if (k >= s_goal_n) return 0;
    *team = s_goal_team[k];
    *squad = s_goal_squad[k];
    return 1;
}

/* The last match, side 0 (a player's side in the championship): the man with this squad
 * number booked (1), sent off (2), or neither (0). */
uint8_t NEOGEO_USER cx_match_card(uint8_t squad)
{
    uint8_t k;
    for (k = 0; k < 11u; k++) {
        const CxMan *m = &s_w.m[k];
        if (m->squad != squad) continue;
        if (m->state == CX_M_OFF && m->card) return 2;
        return (uint8_t)(m->card ? 1u : 0u);
    }
    return 0;
}

/* The match is over: a player who joined it leaves with it. */
static void NEOGEO_USER match_over(void)
{
    if (cx_match_on == 3u) ng_join_leave(2);
    cx_match_on = 0;
}

uint8_t NEOGEO_USER cx_match(uint8_t team, uint8_t foe, uint8_t stadium, uint8_t stage, uint8_t humans)
{
    static const uint8_t k_minutes[5] = {3, 5, 7, 10, 14};
    CxWorld *w = &s_w;
    uint8_t shown_phase = 0xFFu, won, banner = 0xFFu, duel_shown = 0, bulge = 0;
    uint8_t shown_score[2] = {0xFFu, 0xFFu};
    uint16_t shown_time = 0xFFFFu, help = 300;
    uint8_t call_t = 0, replayed = 0, var_prompt = 0;
    uint32_t dice = 2030u + team;
    cx_init(w, team, foe, humans, (uint8_t)(stadium & 7u));
    cx_set_length(w, k_minutes[cx_options.length < 5u ? cx_options.length : 1u], cx_options.extra);
    w->classic_controls = cx_options.classic;
    w->rules = cx_options.rules;
    w->var = cx_options.rules == CX_RULES_MODERN ? cx_options.var : CX_VAR_OFF;
    w->level = (uint8_t)(cx_options.level & 7u);
    w->round = cx_match_round;
    w->et = (uint8_t)(stage >= 2u);
    cx_talk_reset();
    cx_replay_reset();
    match_screen(w, stage);
    cx_puts_center(7, k_stage[stage & 3u], CX_PAL_WHITE);
    cx_puts_center(27, cx_options.classic ? "A PASS  B SHOOT  C LOB  A+B TRICK" : "A PASS B SHOOT C LOB D RUN A+B TRICK", CX_PAL_WHITE);
    cx_music_match();
    {
        uint8_t i;
        for (i = 0; i < 3u; i++) park_word(i);      /* written now, while the blank has room */
    }
    cx_ref_intro(w);
    /* a second player may walk up and take the CPU's side (the operator can say no) */
#ifdef CX_PERF
    {
        NGDevConfig dc = {0};
        dc.x = 8; dc.y = 8; dc.pal = CX_PAL_WHITE; dc.pal_hi = CX_PAL_GOLD;
        dc.hold = CX_A | CX_B | CX_C; dc.press = CX_D;
        ng_dev_init(&dc);
        ng_task_reset();
        cx_perf_task[0] = cx_perf_task[1] = 0;
        (void)ng_task_spawn(task_count, 0, s_task_a, sizeof s_task_a);
        (void)ng_task_spawn(task_type, 0, s_task_b, sizeof s_task_b);
    }
#endif
    s_blank_seen = *(volatile uint16_t *)(USER_WORKRAM + 36);
    s_since_late = 255u;
    s_late_run = s_heavy = s_catch_run = 0;
    s_gauge[0] = s_gauge[1] = 0;
    s_keep_shown = s_aim_shown = s_att_shown = 0;
    s_goal_n = 0;
    cx_match_on = (uint8_t)((humans & 3u) == 1u && (!ng_sys_is_mvs() || ng_dip_option(2) == 0u) ? 1u : CX_MATCH_NO_JOIN);
    for (;;) {
        uint16_t pressed, pressed2, joy = cx_input(&pressed), joy2 = cx_input2(&pressed2);
        uint8_t catch_up;
        if (cx_match_on == 1u && ng_join_poll() == 2u) {
            w->human[1] = 1;
            cx_match_on = 3;
            hud_status(ST_JOIN, 180);
            cx_sfx_menu();
        }
        if ((pressed & START1) && w->phase == CX_P_PLAY) {
            if (pause_menu(w, stage)) { match_over(); return 2; }
            shown_score[0] = shown_score[1] = 0xFFu;
            shown_time = 0xFFFFu;
            continue;
        }
        /* A frame that missed its blank owes the match a tick. Paid with the picture left as
         * it was (catch_up 1), two ticks and no drawing fit in one frame: one picture is
         * dropped and the next is on time. Paid with the drawing as well (catch_up 2), the
         * frame is late again, and the next: 30 pictures a second, which is only right when
         * even the undrawn frame was late (a crowded goalmouth), and then every eighth late
         * frame runs one tick to see whether the load has passed. Never two undrawn in a row.
         * A late frame on its own (none in the half second before) is let go: paid, its
         * picture stood three frames and then jumped three ticks, a hitch anyone could see,
         * and the undrawn frame was often late itself; let go, the picture stands two frames
         * and the match runs a thirtieth slower at the very most. Late frames in a run are
         * paid, so the match never crawls when the picture can't keep up. */
        {
            uint16_t now = *(volatile uint16_t *)(USER_WORKRAM + 36);
            uint16_t owed = (uint16_t)(now - s_blank_seen);
            s_blank_seen = now;
            catch_up = 0;
            if (owed >= 2u && w->phase != CX_P_OVER) {
                if (s_since_late < 30u) {
                    s_late_run++;
                    if (s_catch_run) s_heavy = 1;       /* paid without drawing, and late all the same */
                    if (!s_heavy) catch_up = 1;
                    else if ((s_late_run & 7u) != 0u) catch_up = 2;
                }
                s_since_late = 0;
            } else {
                s_late_run = 0;
                s_heavy = 0;
                if (s_since_late < 255u) s_since_late++;
            }
            s_catch_run = (uint8_t)(catch_up == 1u);
        }
        if (catch_up != 1u) {
            /* draw first, in the vertical blank, then think: the pitch's new
             * columns land before the beam reaches them (no tearing) */
            CX_PERF_MARK(0);
            cx_pitch_draw(cx_camx, cx_camy);
            CX_PERF_MARK(1);
            cx_ref_update(w);
            cx_men_draw(w, (uint8_t)(w->phase == CX_P_PLAY && (w->frame & 1u)));
            CX_PERF_MARK(2);
            cx_ball_draw(w);
            cx_goals_draw(bulge);
            cx_cursors_draw(w);
            hud_draw(w, shown_score, &shown_time);
            cx_coach_step(w->frame, (uint8_t)(w->phase == CX_P_GOAL && w->phase_t < 140u));
            cx_linesman_step(w->frame);
            cx_talk_step();
            if (w->phase == CX_P_GOAL) goal_words(w);
            shot_gauge(w);
            {
                uint8_t o = w->ball.owner, kh = (uint8_t)(w->phase == CX_P_PLAY && o != CX_NOBODY && w->m[o].role == 0 &&
                                                           w->m[o].state == CX_M_HOLD && w->human[o >= 11u]);
                if (kh != s_keep_shown) {
                    s_keep_shown = kh;
                    cx_clear_row(25);
                    if (kh) cx_puts_center(25, "A SHORT  B LONG KICK  C LONG THROW", CX_PAL_GOLD);
                }
            }
            if (w->aim_on != s_aim_shown) {
                s_aim_shown = w->aim_on;
                cx_clear_row(25);
                if (w->aim_on) cx_puts_center(25, "STICK AIMS  A PASS B SHOT C LOB", CX_PAL_GOLD);
            }
            /* a player's man on the ball near their goal: what C does there (play tests: the
             * chip and the cross for a header were never found) */
            if (!s_keep_shown && !w->aim_on) {
                uint8_t o = w->ball.owner, hint = 0;
                if (w->phase == CX_P_PLAY && o < CX_MEN && w->m[o].role != 0 && w->human[o >= 11u] && w->ctrl[o >= 11u] == o) {
                    int16_t gx = cx_attacks_right(w, (uint8_t)(o >= 11u)) ? CX_LEN : 0;
                    int16_t range = (int16_t)(gx - U(w->m[o].x)), wide = (int16_t)(U(w->m[o].y) - CX_MID_Y);
                    if (range < 0) range = (int16_t)-range;
                    if (wide < 0) wide = (int16_t)-wide;
                    /* a margin to leave by: at the edge it flipped every frame, and the text queue
                     * dropped letters ("C  HIP T") */
                    int16_t m = (int16_t)(s_att_shown ? 40 : 0);
                    if (range < 360 + m && wide < 160 + m) hint = 1;
                    else if (range < 460 + m && wide >= 170 - m) hint = 2;
                }
                if (s_att_t) s_att_t--;
                if (hint != s_att_shown && !s_att_t) {
                    s_att_shown = hint;
                    s_att_t = 40;
                    cx_clear_row(24);           /* row 25 is the one-on-one's (A+B FEINT): they mixed */
                    if (hint == 1u) cx_puts_center(24, "B SHOOT  C CHIP THE KEEPER", CX_PAL_GOLD);
                    else if (hint == 2u) cx_puts_center(24, "C CROSS  YOUR MAN GOES FOR IT", CX_PAL_GOLD);
                }
            }
            CX_PERF_MARK(3);
        }
#ifdef CX_PERF
        /* the developer's test menu (A+B+C held, D): slow motion, a frozen clock, a
         * free camera, RAM watch and the sound test; the match waits behind it */
        ng_task_run();
        /* the stacks' high-water mark, once a second: each frame it cost six lines the
         * release build doesn't spend, and made test builds read slower than the game */
        if ((w->frame & 63u) == 0u) cx_perf_task[2] = (uint16_t)(ng_task_stack_left(0) | (ng_task_stack_left(1) << 8));
        if (!ng_dev_frame(joy, pressed)) { cx_frame_wait(); continue; }
        {
            uint16_t keep = w->clock;
            cx_tick(w, joy, pressed, joy2, pressed2);
            if (ng_dev.timer_stop) w->clock = keep;
        }
#else
        cx_tick(w, joy, pressed, joy2, pressed2);
#endif
        if (catch_up) {
            uint32_t ev = w->ev;
            cx_tick(w, joy, 0, joy2, 0);         /* the owed tick: the buttons held, nothing newly pressed */
            w->ev |= ev;
        }
        CX_PERF_MARK(4);
        cx_sound_events(w);
        cx_crowd_level(w);
        cx_crowd_step((uint8_t)(w->phase == CX_P_GOAL || w->momentum[0] > 180u || w->momentum[1] > 180u));
        cx_terraces(w);
        cx_talk_events(w);
        cx_coach_events(w);
        cx_linesman_events(w);
        cx_camera(w, 0);
#ifdef CX_PERF
        if (ng_dev.free_cam) { cx_camx = (int16_t)(cx_camx + ng_dev.cam_x); cx_camy = (int16_t)(cx_camy + ng_dev.cam_y); }
#endif
        calls(w, &call_t);
        duel_prompt(w, &duel_shown);
        if (w->ev & CX_EV_SUB) cx_side_palettes(w);
        if (w->ev & CX_EV_GOAL) {
            bulge = 1; replayed = 0; cx_clear_row(6); cx_chant(w->team[w->score[0] > shown_score[0] ? 0 : 1]);
            if (w->scorer < CX_MEN && w->goal_kind != CX_G_OWN && s_goal_n < 24u) {
                s_goal_team[s_goal_n] = w->team[w->scorer >= 11u];
                s_goal_squad[s_goal_n++] = w->m[w->scorer].squad;
            }
        }
        if (w->phase == CX_P_GOAL && w->phase_t == 45u) bulge = 0;
#ifdef CX_PERF
        if (cx_test_card == 3u) { cx_test_card = 0; cx_foul(w, 16, w->ball.owner == 255u ? 9u : w->ball.owner, 1); }
#endif
        /* the whistle has gone: the referee has his moment, then the card */
        if ((w->ev & CX_EV_FOUL) && w->phase == CX_P_STOP) cx_foul_scene(w);
        if (w->ev & CX_EV_CARD) cx_card_scene(w);
        /* a side may ask for the video: the player's C, while the call is fresh */
        {
            uint8_t t, open = 0;
            for (t = 0; t < 2u; t++) {
                if (w->human[t] && cx_var_open(w, t)) {
                    open = 1;
                    if ((t ? pressed2 : pressed) & CX_C) (void)cx_var_challenge(w, t);
                }
            }
            if (open != var_prompt) {
                var_prompt = open;
                cx_clear_row(23);
                if (open) cx_puts_center(23, "C ASK FOR VAR", CX_PAL_GREEN);
            }
        }
#ifdef CX_PERF
        if (cx_test_card) { w->card_man = 16; w->card_red = (uint8_t)(cx_test_card - 1u); cx_test_card = 0; cx_card_scene(w); }
#endif
        if (w->phase == CX_P_GOAL && w->phase_t == 230u && !replayed) {
            replayed = 1;
            cancel_words();
            /* the replay is the goal only: the bench, the talk and the celebration are done.
             * Back from it, the kick-off follows at once (the scorer used to celebrate a
             * second time before it) */
            cx_talk_reset();
            cx_coach_step(w->frame, 1);
            cx_clear_row(10);
            cx_clear_row(8);
            if (cx_replay_ready()) {
                goal_caption(w);
                cx_replay();
                cx_clear_row(9);
                w->phase_t = 250u;
            }
        }
        /* a penalty: the 3D view takes over and tells the match what happened */
        if (w->phase == CX_P_PENALTY && w->phase_t == 36u) {
            uint8_t t = w->restart_team, scored;
            cx_say_penalty();
            cx_wait(45);
            scored = pen_view(w, w->restart_man, (uint8_t)((t ^ 1u) * 11u), w->human[t], w->human[t ^ 1u], t, &dice);
            match_screen(w, stage);
            cx_music_match();
            shown_score[0] = shown_score[1] = 0xFFu;
            shown_time = 0xFFFFu;
            shown_phase = 0xFFu;
            cx_penalty_taken(w, scored);
        }
        if (w->phase == CX_P_VAR && w->phase_t == 2u) {
            cancel_words();
            cx_var_show(w);
            match_screen(w, stage);
            cx_music_match();
            shown_score[0] = shown_score[1] = 0xFFu;
            shown_time = 0xFFFFu;
            shown_phase = 0xFFu;
            w->phase_t = CX_VAR_FRAMES;             /* the record's answer takes effect */
        }
        if (help && !--help) { cx_clear_row(27); cx_clear_row(7); }
        if (w->phase != shown_phase) {
            uint8_t want = 0xFFu;
            shown_phase = w->phase;
            if (w->phase != CX_P_GOAL) { phase_caption(w); cx_clear_row(24); s_att_shown = 0; cancel_words(); }
            if (w->phase == CX_P_HALF) want = BANNER_HALF;
            else if (w->phase == CX_P_FULL) want = BANNER_FULL;
            if (want != banner) {
                banner = want;
                if (want == 0xFFu) cx_spr_hide(&cx_banner);
                else cx_spr_show(&cx_banner, SPR_BANNER, CX_ART_BANNER, want, 80, 72, 0);
            }
        }
        CX_PERF_MARK(5);
        if (w->phase == CX_P_OVER) break;
    }
    cx_spr_hide(&cx_banner);
    cx_clear_row(6);
    cx_clear_row(25);
    match_over();
    cx_result[0] = w->score[0];
    cx_result[1] = w->score[1];
    if (w->score[0] != w->score[1]) won = (uint8_t)(w->score[0] > w->score[1]);
    else if (stage >= 2u) won = shootout(team, foe, humans);
    else won = 3;
    return won;
}

/* ------------------------------------------------------------- training */
/* A lesson a step: each is finished by the event that proves it was done. */
void NEOGEO_USER cx_training(uint8_t team)
{
    static const char *const k_task[6] = { "A PASS TO A TEAM-MATE", "C LOB A LONG PASS", "B SHOOT AT GOAL",
                                           "WIN THE BALL WITH A", "A+B FEINT AT A DEFENDER", "SCORE A GOAL" };
    static const uint16_t k_need[6] = { CX_EV_PASS, CX_EV_PASS, CX_EV_KICK, CX_EV_TACKLE, CX_EV_FEINT, CX_EV_GOAL };
    CxWorld *w = &s_w;
    uint8_t step = 0;
    uint16_t quiet = 0;
    cx_init(w, team, (uint8_t)(team ? 0u : 1u), 1, 2);
    w->classic_controls = cx_options.classic;
    w->rules = cx_options.rules;
    match_screen(w, 0);
    cx_clear_row(7);
    cx_puts_center(7, "TRAINING GROUND", CX_PAL_GOLD);
    cx_puts_center(25, k_task[0], CX_PAL_WHITE);
    while (step < 6u) {
        uint16_t pressed, p2, joy = cx_input(&pressed);
        (void)cx_input2(&p2);
        if (pressed & START1) break;
        cx_pitch_draw(cx_camx, cx_camy);
        cx_ref_update(w);
        cx_men_draw(w, 0);
        cx_ball_draw(w);
        cx_goals_draw(0);
        cx_cursors_draw(w);
        cx_tick(w, joy, pressed, 0, 0);
        cx_sound_events(w);
        cx_camera(w, 0);
        if (quiet) quiet--;
        if (!quiet && (w->ev & k_need[step]) && (step != 1u || (pressed & CX_C)) && (step != 2u || (pressed & CX_B))) {
            step++;
            quiet = 40;
            cx_clear_row(25);
            cx_puts_center(6, "WELL DONE", CX_PAL_GREEN);
            if (step < 6u) cx_puts_center(25, k_task[step], CX_PAL_WHITE);
        }
        if (quiet == 1u) cx_clear_row(6);
    }
    cx_clear_row(25);
    if (step >= 6u) { cx_puts_center(12, "TRAINING COMPLETE", CX_PAL_GOLD); (void)cx_any_press(240); }
}
