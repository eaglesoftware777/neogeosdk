/*
 * cx_var.c -- Century XI: the video review.
 *
 * A side asks, or (in automatic mode) the record calls it in. Everything shown comes from
 * that record: which man, which call, and the answer, already known when the first frame
 * is drawn. There are no dice here and no reversal for the joke. The tiny referee and the
 * worried player are the fun; the decision is the truth of what happened.
 */
#include "cx_render.h"

/* Under the monitor, on the dark wall, so its picture stays clear and the words stay legible. */
static void NEOGEO_USER caption(const CxWorld *w)
{
    char name[32], who[12];
    uint8_t man = w->review_man, n = 0, i = 0;
    static const char k_asked[] = " ASK FOR VAR";
    cx_puts_center(12, "VIDEO REVIEW", CX_PAL_GOLD);
    if (w->var == CX_VAR_CHALLENGE) {
        cx_squad_name(w->team[w->asked], name);
        while (name[n]) n++;
        while (k_asked[i]) name[n++] = k_asked[i++];
        name[n] = 0;
        cx_puts_center(13, name, CX_PAL_CYAN);
    }
    cx_puts_center(14, w->review == 4u ? "CHECKING FOR OFFSIDE" : (w->review == 1u ? "CHECKING THE GOAL" : "CHECKING THE FOUL"), CX_PAL_WHITE);
    if (man != CX_NOBODY) {
        cx_man_name(w->team[man >= 11u], w->m[man].squad, who);
        cx_puts_center(15, who, CX_PAL_CYAN);
    }
    cx_puts_center(26, "REF: ONE MOMENT, IT IS BUFFERING", CX_PAL_WHITE);
}

void NEOGEO_USER cx_var_show(const CxWorld *w)
{
    uint16_t t, full = (uint16_t)(cx_options.var == CX_VAR_FULL ? 110u : 70u), pressed;
    uint8_t band = 2u, per = CX_REF_FRAMES;
    uint8_t man = w->review_man != CX_NOBODY ? w->review_man : (uint8_t)(w->asked * 11u + 1u);
    uint16_t art = cx_man_art(man);
    uint8_t fper = (uint8_t)(art == CX_ART_KEEPER ? CX_KEEP_FRAMES : CX_FIG_FRAMES);
    const char *verdict = w->review_keep ? "THE CALL STANDS" : (w->review == 4u ? "OFFSIDE  NO GOAL" : "OVERTURNED  NO FOUL");
    cx_screen_clear();
    cx_picture_show(CX_ART_VAR_ROOM);
    cx_side_palettes(w);
    cx_say_review();
    caption(w);
    for (t = 0; t < full; t++) {
        uint8_t frame = (t >> 4) & 1u ? CX_R_REF_PEER : CX_R_REF_MOP;
        cx_figure_show(&cx_ref, SPR_MEN + 110u, CX_ART_REF, (uint8_t)(band * per + frame), 96, 196, 0, BANK_REF);
        cx_figure_show(&cx_men[0], SPR_MEN, art, (uint8_t)(band * fper + ((t >> 5) & 1u ? CX_F_VAR_WAIT : CX_F_DISBELIEF)), 236, 196, 1,
                       (uint8_t)(BANK_MEN + man));
        (void)cx_input(&pressed);
        if (t > 20u && (pressed & (CX_A | CX_B | START1))) break;
    }
    cx_clear_row(14);
    cx_clear_row(26);
    cx_puts_center(14, verdict, w->review_keep ? CX_PAL_GOLD : CX_PAL_RED);
    cx_figure_show(&cx_ref, SPR_MEN + 110u, CX_ART_REF, (uint8_t)(band * per + (w->review_keep ? CX_R_REF_HIPS : CX_R_REF_SIDE)), 96, 196, 0, BANK_REF);
    cx_figure_show(&cx_men[0], SPR_MEN, art, (uint8_t)(band * fper + (w->review_keep ? CX_F_THUMBS : CX_F_COMPLAIN)), 236, 196, 1, (uint8_t)(BANK_MEN + man));
    cx_wait(50);
    cx_screen_clear();
}
