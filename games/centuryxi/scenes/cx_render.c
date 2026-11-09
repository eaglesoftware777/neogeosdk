/*
 * cx_render.c -- Century XI: drawing the match.
 *
 * The ground is one panorama streamed into a ring of 21 strips, scrolling with
 * the ball. World points reach the screen through cx_project(), the same
 * perspective the ground was painted with (tools/arena.py), so a man's feet are
 * where the markings say. Men are figures in palette roles: bank 32+i dresses
 * man i in his nation's kit and his own skin and hair, so every national side
 * shares one set of frames. Each figure comes in three native sizes; a man takes
 * the size for his depth, drawn by hand-reduced frames, not by shrinking.
 *
 * The men and the referee do not go through the sprite-group queue. Each has a slot of
 * five hardware sprites in depth order, and everything a slot should show is worked out
 * during the frame and written straight to video memory as one piece, first thing in the
 * vertical blank. Queued, a changed man could take his new position in one blank and his
 * new look in the next, so men trading depth order flickered when they were close. And a
 * pose only uses the strips and rows it fills (cx_frames.h), so a crowd costs the
 * hardware's 96-sprites-a-line far less. The fifth sprite is his shirt number.
 */
#include "cx_render.h"
#include "games/centuryxi/artbox/generated/cx_crowd.h"
#include "games/centuryxi/artbox/generated/cx_assets.h"
#include "games/centuryxi/artbox/generated/cx_proj.h"
#include "games/centuryxi/artbox/generated/cx_frames.h"

typedef char cx_btn_ok[(CX_A == BUTTON_A && CX_B == BUTTON_B && CX_C == BUTTON_C &&
                        CX_D == BUTTON_D && CX_LEFT == JOY_LEFT && CX_RIGHT == JOY_RIGHT &&
                        CX_UP == JOY_UP && CX_DOWN == JOY_DOWN &&
                        CX_START == START1) ? 1 : -1];

#define STRIP_STICKY  0x0040u
#define CAM_MAX_X     (CX_PIC_W - 320)
#define CAM_MAX_Y     (CX_PIC_H - 224)
#define U(v)          ((int16_t)((v) >> 4))
#define BANDS         3u
#define BAND_MID      232u           /* scale at the far/middle join, /256 */
#define BAND_NEAR     257u
#define BAND_SLACK    5u
#define CX_SHADOW_WORD 0x0242u      /* a dark, cool green: the grass in shade */

/* Nothing here has an initial value: the game's RAM is cleared at boot and .data
 * is not copied from ROM, so a static's initialiser would silently be zero. */
CxOptions cx_options;
static uint16_t s_prev[2];

/* The five drawn directions (side, down-right, down, up-right, up) for dirs 0-7. */
static const uint8_t k_dir_slot[8] = {0, 1, 2, 1, 0, 3, 4, 3};
static const uint8_t k_dir_flip[8] = {0, 0, 0, 1, 1, 1, 0, 0};

static NGSpriteGroup s_bg[2];        /* the stands' ring and the pitch's */
static uint8_t s_shown[RING], s_ring, s_bg_live;
static uint16_t s_left;
static int16_t s_chain_x[2], s_chain_y;
/* The background chains' position words for the next blank, written together and
 * first (cx_bg_commit). Through the engine's queue they could be split across two
 * blanks when the window ran short, and a chain half moved put a block of the
 * pitch off the screen for a frame: black columns, flicker on every pan. */
typedef struct { uint16_t slot, scb3, scb4; } CxBgPos;
static CxBgPos s_bgpos[6];
static uint8_t s_bgpos_n;
static void NEOGEO_USER bg_pos(uint16_t slot, uint16_t scb3, uint16_t scb4)
{
    uint8_t k;
    for (k = 0; k < s_bgpos_n; k++)
        if (s_bgpos[k].slot == slot) { s_bgpos[k].scb3 = scb3; s_bgpos[k].scb4 = scb4; return; }
    if (s_bgpos_n < 6u) { s_bgpos[s_bgpos_n].slot = slot; s_bgpos[s_bgpos_n].scb3 = scb3; s_bgpos[s_bgpos_n].scb4 = scb4; s_bgpos_n++; }
}

static void NEOGEO_USER bg_commit(void)
{
    volatile uint16_t *vp = (volatile uint16_t *)VRAM_ADDR;
    uint8_t k;
    if (!s_bgpos_n) return;
    vp[2] = 0x200;                                  /* SCB3 to SCB4: 0x200 apart */
    for (k = 0; k < s_bgpos_n; k++) {
        vp[0] = (uint16_t)(SCB3_ADDR + s_bgpos[k].slot);
        vp[1] = s_bgpos[k].scb3;
        vp[1] = s_bgpos[k].scb4;
    }
    s_bgpos_n = 0;
}
CxSpr cx_men[CX_MEN], cx_ref, cx_ball_s, cx_shadow, cx_cursor[2], cx_banner, cx_portrait, cx_trophy, cx_plate, cx_logo, cx_goal,
      cx_card, cx_flag[2], cx_hero, cx_hero2, cx_inset, cx_coach, cx_lines, cx_bigcard, cx_ballz, cx_word[3];
int16_t cx_camx, cx_camy;
static int16_t s_rx, s_ry;            /* the referee, in pitch units */
static uint8_t s_ranim, s_rdir;
#define SLOTS   23u                   /* 22 men and the referee */
#define SLOT_W  5u
#define REF_I   22u
static uint8_t s_art[CX_MEN];         /* each man's figure asset */
static uint8_t s_band[SLOTS];
static uint8_t s_shirt[CX_MEN];       /* the digit tile on his back */
/* Each slot's figure (the men by number, the referee last), worked out with the kits
 * rather than every frame: its poses' spans, first tile and poses a band, and the tile of
 * the number on his back (0: none). Per frame they cost a thirtieth of the 68000's time. */
static const uint8_t (*s_fig_sp[SLOTS])[4];
static uint16_t s_fig_tile[SLOTS], s_fig_num[SLOTS];
static uint8_t s_fig_per[SLOTS];
static uint8_t s_fig_build[SLOTS];      /* 1 tall: a size up; 2 slight: a size down (the referee: 0) */

/* A man's slot: what the frame wants, and (SLOTS further on) what video memory has.
 * slot_set() works out the final sprite words while the game runs, so the blank only
 * compares and copies: the first 16 bytes compare as four longs, and an unchanged man
 * costs four compares. nyw and nxw are 0 when he shows no number. */
typedef struct {
    uint16_t tile, num;                 /* his pose's first tile, the shirt digit's */
    uint16_t yw, nyw;                   /* SCB3: his first strip's y and height, the number's */
    uint16_t xw, nxw;                   /* SCB4: their x */
    uint8_t bank, flip, vis, strips;    /* strips on the have side: the sticky words written */
    uint8_t c0, c1, r0, rows;           /* the strips and rows his pose fills */
    int16_t x, y;                       /* where his frame's corner is, to move an old pose */
} CxSlot;
static CxSlot s_slot[2u * SLOTS];
#define s_want (s_slot)
#define s_have (s_slot + SLOTS)
typedef uint32_t __attribute__((__may_alias__)) CxL32;
#define SLOT_L(p, i) (((const CxL32 *)(const void *)(p))[i])
static uint8_t s_slot_next;           /* the slot this blank's new poses start from */
static uint8_t s_slots_live;
static uint16_t s_stands;             /* this ground's stands asset */
static int16_t s_ref_ux, s_ref_uy;
static uint8_t s_ref_frame, s_ref_flip, s_ref_on;

/* ------------------------------------------------------------- helpers */
/* The frame's wait: the vertical blank, then every queued sprite and FIX
 * write committed inside it (NG_VRAM_DEFER), so nothing is drawn half-written. */
static void NEOGEO_USER slots_commit(void);
#ifdef CX_PERF
uint8_t cx_dbg_strips;           /* (measurement builds) background columns a blank behind */
#endif

void NEOGEO_USER cx_frame_wait(void)
{
    /* A late frame may leave the BIOS flag set from an earlier blank. Wait
     * for the next blank so queued writes have a real transfer window. */
    NEO_REGISTER(USER_WORKRAM) = 0;
    waitVbl();
    bg_commit();                     /* the background's chains first, whole: never half moved */
    ng_vram_commit_strips_first();   /* and its new columns: a column behind shows old pitch (a ghost net) */
    CX_PERF_MARK(6);
    slots_commit();
    CX_PERF_MARK(8);
    ng_vram_commit();
    CX_PERF_MARK(7);
#ifdef CX_PERF
    cx_dbg_strips = ng_vram_strips_waiting();       /* background columns a blank behind */
#endif
}


/* All the men, whole, in the blank: a man's new pose is written complete or not at all.
 *
 * The blank is shared: ng_vram_commit() runs straight after this with a deadline
 * (NG_VRAM_DEADLINE), and whatever it can't start waits, so if the men took the
 * whole window the pitch's strip columns, the goal and the big figures would never
 * be written (a crowded goalmouth: the picture stopped scrolling, the goal
 * vanished). So a new pose is written only while SLOT_ROOM lines are left; the
 * rest keep their old pose one more blank, moved with their feet so nobody lags
 * the pitch, and the next blank starts with them. Position words are written only
 * when they change: the man's first strip and his shirt number are 4 sprites apart,
 * so with VRAM_INC at 4 one address serves both. A sticky strip takes its x from
 * the strip before it, so its SCB4 word is never needed. */
#define SLOT_ROOM 16u
#ifdef CX_PERF
static uint8_t s_dbg_new, s_dbg_pos;
#endif
/* A sprite word's 9-bit position field moved by d (y fields count up the screen). */
#define POS_MOVE(w, d) ((uint16_t)(((uint16_t)(((w) >> 7) + (d)) & 0x1FFu) << 7 | ((w) & 0x7Fu)))

static void NEOGEO_USER slots_commit(void)
{
    /* the port in an address register: (An) writes take 8 cycles, absolute ones 16 */
    volatile uint16_t *vp = (volatile uint16_t *)VRAM_ADDR;
    CxSlot *n;
    uint16_t first;
    uint8_t i, k, room = 1, late = 0xFFu;
    if (!s_slots_live) return;
    __asm__ volatile ("" : "+a"(vp));
#define VA vp[0]
#define VD vp[1]
#define VI vp[2]
    k = s_slot_next;
    n = &s_want[k];
    first = (uint16_t)(SPR_MEN + k * SLOT_W);
    for (i = SLOTS; i; i--) {
        CxSlot *h = n + SLOTS;
        if (!n->vis) {
            if (h->vis) {
                VI = 1;
                VA = (uint16_t)(SCB3_ADDR + first);
                VD = 0; VD = 0; VD = 0; VD = 0; VD = 0;
                h->vis = 0;
                h->strips = 0;
            }
        } else if (SLOT_L(n, 0) != SLOT_L(h, 0) || SLOT_L(n, 1) != SLOT_L(h, 1) ||
                   SLOT_L(n, 2) != SLOT_L(h, 2) || SLOT_L(n, 3) != SLOT_L(h, 3)) {
            uint8_t look = (uint8_t)(!h->vis || h->tile != n->tile || h->bank != n->bank || h->flip != n->flip);
            /* A slot that was empty, or held another man (the depth order changed), must
             * be written now: keeping its old pose would show nobody, or another man in
             * this man's place for a frame (a red player flashing blue). Only the same
             * man's next pose may wait a blank. */
            uint8_t must = (uint8_t)(!h->vis || h->bank != n->bank);
            if (look && room && ng_vram_lines_left() < SLOT_ROOM) room = 0;
            if (look && !room && !must) {
                if (late == 0xFFu) late = k;
                if (h->vis) {
                    /* the old pose one more blank, moved with his feet */
                    int16_t dx = (int16_t)(n->x - h->x), dy = (int16_t)(h->y - n->y);
                    h->x = n->x;
                    h->y = n->y;
                    h->yw = POS_MOVE(h->yw, dy);
                    h->xw = POS_MOVE(h->xw, dx);
                    if (h->nyw) { h->nyw = POS_MOVE(h->nyw, dy); h->nxw = POS_MOVE(h->nxw, dx); }
                    VI = 4;
                    VA = (uint16_t)(SCB3_ADDR + first); VD = h->yw; VD = h->nyw;
                    VA = (uint16_t)(SCB4_ADDR + first); VD = h->xw; VD = h->nxw;
                }
            } else {
                if (look) {
                    uint16_t attr = (uint16_t)(((uint16_t)n->bank << 8) | n->flip);
                    uint16_t base = (uint16_t)(n->tile + (uint16_t)n->r0 * 4u);
                    uint16_t addr = (uint16_t)(first << 6);
                    uint8_t j;
#ifdef CX_PERF
                    s_dbg_new++;
#endif
                    VI = 1;
                    for (j = 0; j < n->strips; j++) {
                        uint16_t tile = (uint16_t)(base + (n->flip ? n->c1 - j : n->c0 + j));
                        uint8_t r;
                        VA = addr;
                        for (r = n->rows; r; r--) { VD = tile; VD = attr; tile = (uint16_t)(tile + 4u); }
                        addr = (uint16_t)(addr + 64u);
                    }
                    if (n->strips != h->strips) {
                        VA = (uint16_t)(SCB3_ADDR + first + 1u);
                        VD = (uint16_t)(n->strips > 1u ? STRIP_STICKY : 0u);
                        VD = (uint16_t)(n->strips > 2u ? STRIP_STICKY : 0u);
                        VD = (uint16_t)(n->strips > 3u ? STRIP_STICKY : 0u);
                    }
                }
                if (n->num && (look || h->num != n->num)) {
                    VI = 1;
                    VA = (uint16_t)((first + 4u) << 6);
                    VD = n->num;
                    VD = (uint16_t)((uint16_t)n->bank << 8);
                }
#ifdef CX_PERF
                s_dbg_pos++;
#endif
                VI = 4;
                VA = (uint16_t)(SCB3_ADDR + first); VD = n->yw; VD = n->nyw;
                VA = (uint16_t)(SCB4_ADDR + first); VD = n->xw; VD = n->nxw;
                *h = *n;
            }
        }
        if (++k == SLOTS) { k = 0; n = s_want; first = SPR_MEN; }
        else { n++; first = (uint16_t)(first + SLOT_W); }
    }
    if (late != 0xFFu) s_slot_next = late;
    VI = 1;
#undef VA
#undef VD
#undef VI
#ifdef CX_PERF
    cx_perf[9] = (uint16_t)(s_dbg_new | ((uint16_t)s_dbg_pos << 8));
    s_dbg_new = s_dbg_pos = 0;
#endif
}

/* The slots become the men's: full size, nothing showing yet. */
static void NEOGEO_USER slots_begin(void)
{
    volatile uint16_t *va = (volatile uint16_t *)VRAM_ADDR, *vd = (volatile uint16_t *)VRAM_RW;
    uint16_t i;
    NEO_REGISTER(VRAM_INC) = 1;
    *va = (uint16_t)(SCB2_ADDR + SPR_MEN);
    for (i = 0; i < SLOTS * SLOT_W; i++) *vd = 0x0FFFu;
    *va = (uint16_t)(SCB3_ADDR + SPR_MEN);
    for (i = 0; i < SLOTS * SLOT_W; i++) *vd = 0;
    for (i = 0; i < SLOTS; i++) {
        s_want[i].vis = s_have[i].vis = 0;
        s_have[i].strips = 0;                 /* as just written: no sticky strips */
    }
    s_slot_next = 0;
    s_slots_live = 1;
}

static const NGArtAsset *NEOGEO_USER asset(uint16_t id) { return &cx_art[id - 1u]; }

static void NEOGEO_USER load_asset_pal(uint16_t id)
{
    uint8_t idx = (uint8_t)(id - 1u);
    uint8_t n = cx_art_banks[idx];
    uint8_t bank = cx_art[idx].palette_bank;
    uint8_t k;
    for (k = 0; k < n; k++)
        load_palettes((uint16_t *)(cx_art_pal[idx] + 16u * k), (uintptr_t)(PALETTES + ((uint16_t)(bank + k) << 5)));
}

void NEOGEO_USER cx_puts_center(uint8_t y, const char *s, uint8_t pal)
{
    uint8_t n = 0;
    while (s[n]) n++;
    ng_fix_puts((uint8_t)((40u - n) / 2u), y, s, pal);
}

void NEOGEO_USER cx_clear_row(uint8_t y)
{
    ng_fix_puts(0, y, "                                        ", CX_PAL_WHITE);
}

void NEOGEO_USER cx_put_2d(uint8_t x, uint8_t y, uint16_t value, uint8_t pal)
{
    char buf[3];
    uint8_t tens = 0;
    while (value >= 100u) value = (uint16_t)(value - 100u);
    while (value >= 10u) { value = (uint16_t)(value - 10u); tens++; }
    buf[0] = (char)('0' + tens);
    buf[1] = (char)('0' + value);
    buf[2] = 0;
    ng_fix_puts(x, y, buf, pal);
}

uint16_t NEOGEO_USER cx_input(uint16_t *pressed)
{
    /* Edges from this frame against the last: the BIOS's change bytes keep the
     * START that began the game set, so a menu would see it pressed again. */
    uint16_t joy;
    cx_frame_wait();
    joy = poll_joystick();
    *pressed = (uint16_t)(joy & (uint16_t)~s_prev[0]);
    s_prev[0] = joy;
    return joy;
}

/* The second stick, read after cx_input in the same frame. */
uint16_t NEOGEO_USER cx_input2(uint16_t *pressed)
{
    uint16_t joy = (uint16_t)(NEO_REGISTER8(BIOS_P2CURRENT) | ((uint16_t)(poll_joystick() & 0x0400u) >> 2));
    *pressed = (uint16_t)(joy & (uint16_t)~s_prev[1]);
    s_prev[1] = joy;
    return joy;
}

void NEOGEO_USER cx_wait(uint16_t n)
{
    while (n--) cx_frame_wait();
}

/* Waits up to `frames` for a button after a short pause; answers what was pressed. */
uint16_t NEOGEO_USER cx_any_press(uint16_t frames)
{
    uint16_t f, pressed;
    for (f = 0; f < frames; f++) {
        (void)cx_input(&pressed);
        if (f > 30u && (pressed & (CX_A | CX_B | CX_C | START1))) return pressed;
    }
    return 0;
}

/* ----------------------------------------------------------- sprites */
void NEOGEO_USER cx_spr_hide(CxSpr *s)
{
    if (s->vis) {
        ng_sprite_group_set_visible(&s->g, 0);
        ng_sprite_group_flush(&s->g);
        s->vis = 0;
    }
}

/* Frame `frame` of asset `art` with its top-left at (x, y); bank 0: the asset's own colours. */
void NEOGEO_USER cx_spr_show_bank(CxSpr *s, uint16_t slot, uint16_t art, uint8_t frame, int16_t x, int16_t y, uint8_t flip, uint8_t bank)
{
    const NGArtAsset *a = asset(art);
    uint16_t per = (uint16_t)((uint16_t)a->strips * a->rows);
    uint16_t tile = (uint16_t)(a->tile_base + per * frame);
    if (!s->bound || s->art != art) {
        /* a bigger asset's strips must not linger; one the same size is simply
         * rewritten in place (hiding first would blink for a frame) */
        if (s->bound && (a->strips < s->g.strips || a->rows < s->g.heightTiles)) cx_spr_hide(s);
        ng_sprite_group_init(&s->g, slot, a->strips, a->rows, a->tile_base, a->palette_bank);
        s->bound = 1;
        s->art = art;
        s->tile = 0xFFFFu;
        s->vis = 0;
    }
    if (s->tile != tile || s->flip != flip || s->bank != bank || !s->vis) {
        ng_sprite_group_set_tile_base(&s->g, tile);
        if (bank) {
            ng_sprite_group_set_palette_map(&s->g, 0);
            ng_sprite_group_set_palette(&s->g, bank);
        } else {
            ng_sprite_group_set_palette_map(&s->g, a->tile_palettes + per * frame);
        }
        ng_sprite_group_set_flip(&s->g, flip, 0);
        ng_sprite_group_set_pos(&s->g, x, y);
        ng_sprite_group_set_visible(&s->g, 1);
        ng_sprite_group_mark_dirty(&s->g, NG_SGF_DIRTY_ALL);
        ng_sprite_group_flush(&s->g);
        s->tile = tile;
        s->flip = flip;
        s->bank = bank;
        s->x = x;
        s->y = y;
        s->vis = 1;
    } else if (s->x != x || s->y != y) {
        ng_sprite_group_set_pos(&s->g, x, y);
        ng_sprite_group_flush(&s->g);
        s->x = x;
        s->y = y;
    }
}

/* A picture shown at a fraction of its size (scale 0..255, 255 whole), shrunk about
 * the middle of where it stands whole (x, y: its top-left at full size). The width
 * is shared out exactly across its strips (ng_sprite_group exact width), so a
 * caption zooming in grows a pixel at a time instead of a strip's worth. */
void NEOGEO_USER cx_spr_show_zoom(CxSpr *s, uint16_t slot, uint16_t art, uint8_t frame, int16_t x, int16_t y, uint8_t scale)
{
    const NGArtAsset *a = asset(art);
    int16_t fw = (int16_t)(a->strips * 16u), fh = (int16_t)(a->rows * 16u);
    int16_t w = (int16_t)ng_sprite_group_exact_width(a->strips, scale);
    int16_t h = (int16_t)(((uint16_t)a->rows * 16u * ((uint16_t)scale + 1u)) >> 8);
    cx_spr_show(s, slot, art, frame, (int16_t)(x + (fw - w) / 2), (int16_t)(y + (fh - h) / 2), 0);
    ng_sprite_group_set_exact_width(&s->g, 1);
    ng_sprite_group_set_scale(&s->g, scale, scale);
    if (s->g.dirty & NG_SGF_DIRTY_SHRINK) ng_sprite_group_flush(&s->g);
}

void NEOGEO_USER cx_spr_show(CxSpr *s, uint16_t slot, uint16_t art, uint8_t frame, int16_t x, int16_t y, uint8_t flip)
{
    cx_spr_show_bank(s, slot, art, frame, x, y, flip, 0);
}

/* A figure from its feet at screen (x, y); a mirror turns round the anchor. */
void NEOGEO_USER cx_figure_show(CxSpr *s, uint16_t slot, uint16_t art, uint8_t frame, int16_t x, int16_t y, uint8_t flip, uint8_t bank)
{
    cx_spr_show_bank(s, slot, art, frame, (int16_t)(x - (flip ? (int16_t)(CX_MAN_W - CX_MAN_AX) : CX_MAN_AX)),
                     (int16_t)(y - CX_MAN_AY), flip, bank);
}

static void NEOGEO_USER sprites_reset(void)
{
    uint8_t i;
    for (i = 0; i < CX_MEN; i++) cx_men[i].bound = cx_men[i].vis = 0;
    cx_ref.bound = cx_ball_s.bound = cx_shadow.bound = cx_cursor[0].bound = cx_cursor[1].bound = 0;
    cx_banner.bound = cx_portrait.bound = cx_trophy.bound = cx_plate.bound = cx_logo.bound = cx_goal.bound = cx_card.bound = 0;
    cx_flag[0].bound = cx_flag[1].bound = cx_hero.bound = cx_hero2.bound = cx_inset.bound = cx_coach.bound = cx_lines.bound = 0;
    cx_bigcard.bound = cx_ballz.bound = cx_word[0].bound = cx_word[1].bound = cx_word[2].bound = 0;
    cx_ref.vis = cx_ball_s.vis = cx_shadow.vis = cx_cursor[0].vis = cx_cursor[1].vis = 0;
    cx_banner.vis = cx_portrait.vis = cx_trophy.vis = cx_plate.vis = cx_logo.vis = cx_goal.vis = cx_card.vis = 0;
    cx_flag[0].vis = cx_flag[1].vis = cx_hero.vis = cx_hero2.vis = cx_inset.vis = cx_coach.vis = cx_lines.vis = 0;
    cx_bigcard.vis = cx_ballz.vis = cx_word[0].vis = cx_word[1].vis = cx_word[2].vis = 0;
}

void NEOGEO_USER cx_screen_clear(void)
{
    cx_frame_wait();
    s_slots_live = 0;
    clearSprs();
    ng_sprite_hide_all();
    ng_fix_clear();
    sprites_reset();
    s_bg_live = 0;
}

void NEOGEO_USER cx_init_video(void)
{
    uint16_t id;
    s_prev[0] = s_prev[1] = 0xFFFFu;     /* whatever is held now is not a new press */
    s_slots_live = 0;
    clearSprs();
    ng_fix_init();
    ng_fix_set_ascii_base(0xD00u);
    for (id = CX_ART_GOAL; id <= CX_ART_COUNT; id++) load_asset_pal(id);
    sprites_reset();
    s_bg_live = 0;
}

void NEOGEO_USER cx_picture_show(uint16_t art)
{
    const NGArtAsset *a = asset(art);
    load_asset_pal(art);
    ng_sprite_group_init(&s_bg[0], SPR_BG, a->strips, a->rows, a->tile_base, a->palette_bank);
    ng_sprite_group_set_palette_map(&s_bg[0], a->tile_palettes);
    ng_sprite_group_set_pos(&s_bg[0], 0, 0);
    ng_sprite_group_set_visible(&s_bg[0], 1);
    ng_sprite_group_mark_dirty(&s_bg[0], NG_SGF_DIRTY_ALL);
    ng_sprite_group_flush(&s_bg[0]);
}

/* ------------------------------------------------------------- dress */
void NEOGEO_USER cx_man_palette(uint8_t bank, uint8_t nation, uint8_t kit, uint8_t skin, uint8_t hair, uint8_t keeper)
{
    uint16_t p[16];
    const uint16_t *k = cx_kit_words(nation, keeper ? 2u : kit);
    const uint16_t *sk = cx_skin_words(skin);
    const uint16_t *h = cx_hair_words(hair);
    uint8_t i;
    p[0] = 0;
    p[1] = 0x0112u;                                   /* the outline, a warm near-black */
    for (i = 0; i < 3u; i++) { p[2u + i] = sk[i]; p[5u + i] = h[i]; }
    for (i = 0; i < 7u; i++) p[8u + i] = k[i];
    p[13] = CX_SHADOW_WORD;                           /* the ground under him, not his kit */
    p[15] = 0x7FFFu;
    load_palettes(p, (uintptr_t)(PALETTES + ((uint16_t)bank << 5)));
}

void NEOGEO_USER cx_ref_palette(uint8_t bank)
{
    /* black shirt and shorts, black socks with a touch of grey */
    static const uint16_t kit[7] = {0x0000u, 0x0223u, 0x0556u, 0x0000u, 0x0223u, 0x0001u, 0x0334u};
    uint16_t p[16];
    const uint16_t *sk = cx_skin_words(0);
    const uint16_t *h = cx_hair_words(0);
    uint8_t i;
    p[0] = 0;
    p[1] = 0x0112u;
    for (i = 0; i < 3u; i++) { p[2u + i] = sk[i]; p[5u + i] = h[i]; }
    for (i = 0; i < 7u; i++) p[8u + i] = kit[i];
    p[13] = CX_SHADOW_WORD;
    p[15] = 0x7FFFu;
    load_palettes(p, (uintptr_t)(PALETTES + ((uint16_t)bank << 5)));
}

void NEOGEO_USER cx_load_art_palette(uint16_t art) { load_asset_pal(art); }

uint8_t NEOGEO_USER cx_man_art(uint8_t man) { return s_art[man]; }

static void NEOGEO_USER fig_set(uint8_t i, uint8_t art, uint16_t num)
{
    s_fig_sp[i] = cx_spans[art - CX_ART_FIG0];
    s_fig_tile[i] = asset(art)->tile_base;
    s_fig_per[i] = (uint8_t)(art == CX_ART_KEEPER ? CX_KEEP_FRAMES : (art == CX_ART_REF ? CX_REF_FRAMES : CX_FIG_FRAMES));
    s_fig_num[i] = num;
}

void NEOGEO_USER cx_side_palettes(const CxWorld *w)
{
    uint8_t i;
    uint8_t kit[2];
    kit[0] = cx_kit_for(w->team[0], w->team[1]);
    kit[1] = cx_kit_for(w->team[1], w->team[0]);
    for (i = 0; i < CX_MEN; i++) {
        const CxMan *m = &w->m[i];
        uint8_t t = (uint8_t)(i >= 11u);
        cx_man_palette((uint8_t)(BANK_MEN + i), w->team[t], kit[t], m->skin, m->hair, (uint8_t)(m->role == 0));
        s_art[i] = (uint8_t)(m->role == 0 ? CX_ART_KEEPER : CX_ART_FIG0 + m->fig);
        /* his number, in a colour his shirt shows: dark on a light shirt, white on a dark one */
        s_shirt[i] = (uint8_t)(m->squad + (cx_kit_ink(w->team[t], m->role == 0 ? 2u : kit[t]) ? 23u : 0u));
        fig_set(i, s_art[i], (uint16_t)(asset(CX_ART_DIGITS)->tile_base + s_shirt[i]));
        s_fig_build[i] = m->build;
    }
    fig_set(REF_I, CX_ART_REF, 0);
    cx_ref_palette(BANK_REF);
}

/* ------------------------------------------------------------- projection */
void NEOGEO_USER cx_project(int16_t ux, int16_t uy, int16_t *px, int16_t *py, uint16_t *s8)
{
    int16_t y = uy;
    uint16_t i, s;
    if (y < CX_PROJ_Y_LO) y = CX_PROJ_Y_LO;
    if (y > CX_PROJ_Y_HI) y = CX_PROJ_Y_HI;
    i = (uint16_t)(y - CX_PROJ_Y_LO);
    s = cx_proj_s[i];
    *py = cx_proj_py[i];
    *s8 = s;
    *px = (int16_t)(CX_PCX + (int16_t)(((int32_t)(int16_t)(ux - CX_LEN / 2) * (int16_t)s) >> 8));
}

/* ------------------------------------------------------------- pitch */
/* A live crowd: each ground's stand palettes have pairs of entries only the crowd wears
 * (tools/art.py crowd_swaps); swapping one pair now and then moves shirts and scarves
 * about the terraces, quicker when the crowd is up. Palette words, written directly. */
static uint8_t s_crowd_ground, s_crowd_k, s_crowd_t;

void NEOGEO_USER cx_crowd_step(uint8_t excited)
{
    volatile uint16_t *pal;
    const uint8_t *pr;
    uint16_t a, b;
    uint8_t g = (uint8_t)(s_crowd_ground & 7u), n = cx_crowd_n[g];
    if (!n || !s_bg_live) return;
    if (++s_crowd_t < (uint8_t)(excited ? 2u : 5u)) return;
    s_crowd_t = 0;
    if (s_crowd_k >= n) s_crowd_k = 0;
    pr = cx_crowd_pair[g][s_crowd_k++];
    pal = (volatile uint16_t *)(uintptr_t)(PALETTES + (uint32_t)(asset(s_stands)->palette_bank + pr[0]) * 32u);
    a = pal[pr[1]];
    b = pal[pr[2]];
    pal[pr[1]] = b;
    pal[pr[2]] = a;
}

/* The day's two sides in the stands: the supporters painted in key colours (tools/arena.py)
 * wear their side's shirt and wave its flag, the home end on the left, the away on the right. */
void NEOGEO_USER cx_crowd_kits(uint8_t home, uint8_t away)
{
    uint16_t col[6];
    uint8_t set, k;
    col[0] = cx_kit_words(home, 0)[1];
    col[1] = cx_nation_flag(home, 0);
    col[2] = cx_nation_flag(home, 1);
    col[3] = cx_kit_words(away, 0)[1];
    col[4] = cx_nation_flag(away, 0);
    col[5] = cx_nation_flag(away, 1);
    for (set = 0; set < 2u; set++) {
        uint8_t s = (uint8_t)(set ? 8u : (s_crowd_ground & 7u));
        for (k = 0; k < cx_crowd_key_n[s]; k++) {
            const uint8_t *e = cx_crowd_key[s][k];
            ((volatile uint16_t *)(uintptr_t)(PALETTES + (uint32_t)e[0] * 32u))[e[1]] = col[e[2] < 6u ? e[2] : 0u];
        }
    }
}

void NEOGEO_USER cx_pitch_load(uint8_t stadium)
{
    uint8_t i, k;
    s_crowd_ground = (uint8_t)(stadium & 7u);
    s_crowd_k = s_crowd_t = 0;
    s_stands = (uint16_t)(CX_ART_STANDS0 + (stadium & 7u));
    load_asset_pal(s_stands);
    load_asset_pal(CX_ART_PITCH);
    for (k = 0; k < 2u; k++) {
        const NGArtAsset *a = asset(k ? CX_ART_PITCH : s_stands);
        NGSpriteGroup *g = &s_bg[k];
        ng_sprite_group_init(g, (uint16_t)(SPR_BG + k * RING), RING, a->rows, a->tile_base, a->palette_bank);
        ng_sprite_group_set_tile_stride(g, a->strips);
        ng_sprite_group_set_palette_map(g, a->tile_palettes);
        ng_sprite_group_set_pos(g, 0, 0);
        ng_sprite_group_set_visible(g, 1);
        ng_sprite_group_mark_dirty(g, NG_SGF_DIRTY_ALL);
        ng_sprite_group_flush(g);
    }
    for (i = 0; i < RING; i++) s_shown[i] = i;
    s_ring = 0;
    s_left = 0xFFFFu;
    s_chain_x[0] = s_chain_x[1] = 0x7FFF;
    s_chain_y = 0x7FFF;
    s_bgpos_n = 0;
    s_rx = CX_LEN / 2 + 40;
    s_ry = 200;
    /* The first write of the two rings is too big for one blank and is spread
     * over several. Streaming a column before it ends would be overwritten by
     * the rest of it, so the picture stays parked until it has finished. */
    cx_wait(8);
    s_bg_live = 1;
}

/* A ring of 21 strips: strip i shows picture column left + ((i - ring) mod 21),
 * placed as two chains (ring..20 at the left of the screen, 0..ring-1 after);
 * the chains' height moves the whole picture up and down. The stands' ring and
 * the pitch's do this together. They never cross a scanline together, so the
 * ground costs 21 of the 96 sprites a line can show. */
void NEOGEO_USER cx_pitch_draw(int16_t camx, int16_t camy)
{
    uint16_t left = (uint16_t)((uint16_t)camx >> 4), c;
    int16_t x0 = (int16_t)(left * 16u) - camx, x1;
    uint8_t ring, k, i, r;
    if (!s_bg_live) return;
    if (left != s_left) {
        const NGArtAsset *a = asset(CX_ART_PITCH);
        c = left;
        while (c >= RING) c = (uint16_t)(c - RING);
        ring = (uint8_t)c;
        if (ring != s_ring) {
            if (s_ring) {
                bg_pos((uint16_t)(SPR_BG + s_ring), STRIP_STICKY, 0);
                bg_pos((uint16_t)(SPR_BG + RING + s_ring), STRIP_STICKY, 0);
            }
            s_ring = ring;
            s_chain_x[0] = s_chain_x[1] = 0x7FFF;
        }
        for (k = 0, i = ring; k < RING; k++) {
            uint16_t col16 = (uint16_t)(left + k);
            uint8_t col = (uint8_t)(col16 < a->strips ? col16 : a->strips - 1u);
            if (s_shown[i] != col) {
                s_shown[i] = col;
                ng_sprite_group_set_strip_column(&s_bg[0], i, col);
                ng_sprite_group_set_strip_column(&s_bg[1], i, col);
            }
            if (++i == RING) i = 0;
        }
        s_left = left;
    }
    if (camy != s_chain_y) { s_chain_y = camy; s_chain_x[0] = s_chain_x[1] = 0x7FFF; }
    x1 = (int16_t)(x0 + (int16_t)((RING - s_ring) * 16u));
    for (r = 0; r < 2u; r++) {
        const NGArtAsset *a = asset(r ? CX_ART_PITCH : s_stands);
        uint16_t first = (uint16_t)(SPR_BG + r * RING);
        uint16_t top = (uint16_t)(496 + camy - (r ? CX_STANDS_H : 0));
        uint16_t scb3 = (uint16_t)(((top & 0x1FFu) << 7) | a->rows);
        if (x0 != s_chain_x[0]) bg_pos((uint16_t)(first + s_ring), scb3, (uint16_t)(((uint16_t)x0 & 0x1FFu) << 7));
        if (s_ring && x1 != s_chain_x[1]) bg_pos(first, scb3, (uint16_t)(((uint16_t)x1 & 0x1FFu) << 7));
    }
    s_chain_x[0] = x0;
    s_chain_x[1] = x1;
}

void NEOGEO_USER cx_camera(const CxWorld *w, uint8_t snap)
{
    int16_t px, py, tx, ty;
    uint16_t s;
    /* the aim view at a set piece: between the ball and the target the player is moving */
    if (w->aim_on) {
        int16_t ax, ay, bx2, by2;
        cx_project(w->aim_x, w->aim_y, &ax, &ay, &s);
        cx_project(U(w->ball.x), U(w->ball.y), &bx2, &by2, &s);
        tx = (int16_t)((ax + bx2) / 2 - 160);
        ty = (int16_t)((ay + by2) / 2 - 112);
        if (tx < 0) tx = 0;
        if (tx > CAM_MAX_X) tx = CAM_MAX_X;
        if (ty < 0) ty = 0;
        if (ty > CAM_MAX_Y) ty = CAM_MAX_Y;
        cx_camx = (int16_t)(cx_camx + cx_div((int16_t)(tx - cx_camx), 6));
        cx_camy = (int16_t)(cx_camy + cx_div((int16_t)(ty - cx_camy), 6));
        return;
    }
    /* after a goal the camera goes with the scorer and his celebration, not the ball */
    if (w->phase == CX_P_GOAL && w->scorer != CX_NOBODY && w->phase_t > 20u) {
        cx_project(U(w->m[w->scorer].x), U(w->m[w->scorer].y), &px, &py, &s);
        tx = (int16_t)(px - 160);
        ty = (int16_t)(py - 140);
        if (tx < 0) tx = 0;
        if (tx > CAM_MAX_X) tx = CAM_MAX_X;
        if (ty < 0) ty = 0;
        if (ty > CAM_MAX_Y) ty = CAM_MAX_Y;
        cx_camx = (int16_t)(cx_camx + cx_div((int16_t)(tx - cx_camx), 8));
        cx_camy = (int16_t)(cx_camy + cx_div((int16_t)(ty - cx_camy), 8));
        return;
    }
    cx_project(U(w->ball.x), U(w->ball.y), &px, &py, &s);
    tx = (int16_t)(px - 160 + U(w->ball.vx) * 4);
    /* a high ball: the camera looks up with it, so it is not a shadow with no ball */
    {
        int16_t h = U(w->ball.h);
        if (h > 100) h = 100;
        ty = (int16_t)(py - 124 - (h > 0 ? (int16_t)(h - (h >> 2)) : 0));
    }
    if (tx < 0) tx = 0;
    if (tx > CAM_MAX_X) tx = CAM_MAX_X;
    if (ty < 0) ty = 0;
    if (ty > CAM_MAX_Y) ty = CAM_MAX_Y;
    if (snap) { cx_camx = tx; cx_camy = ty; return; }
    /* eased, and never more than 12 pixels a frame across (6 up or down): a column of the
     * background is 16 wide, so a frame never asks for two new ones in one blank */
    {
        int16_t dx = cx_div((int16_t)(tx - cx_camx), 6), dy = cx_div((int16_t)(ty - cx_camy), 6);
        if (dx > 12) dx = 12;
        if (dx < -12) dx = -12;
        if (dy > 6) dy = 6;
        if (dy < -6) dy = -6;
        cx_camx = (int16_t)(cx_camx + dx);
        cx_camy = (int16_t)(cx_camy + dy);
    }
}

/* ------------------------------------------------------------- the men */
/* The frame for man i: band-free index; the mirror in *flip. */
static uint8_t NEOGEO_USER man_frame(const CxWorld *w, uint8_t i, uint8_t *flip)
{
    const CxMan *m = &w->m[i];
    uint8_t keeper = (uint8_t)(m->role == 0);
    uint8_t t = (uint8_t)(i >= 11u);
    uint8_t slot = k_dir_slot[m->dir & 7u];
    uint8_t keeper_flip = (uint8_t)!cx_attacks_right(w, t);      /* the keeper faces up the pitch */
    *flip = k_dir_flip[m->dir & 7u];
    switch (m->state) {
    case CX_M_KICK:
        if (keeper) { *flip = keeper_flip; return CX_K_GK_PUNT; }
        if (slot >= 3u) return CX_F_KICK_B2;
        if (m->kick == CX_K_PASS) return CX_F_PASS;
        if (m->kick == CX_K_LONG) return CX_F_LOB;
        return m->timer > 7u ? CX_F_KICK_SHORT : CX_F_KICK_STRIKE;
    case CX_M_WIND:
        return CX_F_POWER;
    case CX_M_VOLLEY:
        return CX_F_VOLLEY;
    case CX_M_BIKE:
        return m->timer > 14u ? CX_F_BIKE1 : CX_F_BIKE2;
    case CX_M_FLY:
        return CX_F_DIVE_HEAD;
    case CX_M_ROLL:
        return (m->timer >> 3) & 1u ? CX_F_ROLL1 : CX_F_ROLL2;
    case CX_M_ARGUE:
        return (m->timer >> 4) & 1u ? CX_F_ARGUE : CX_F_COMPLAIN;
    case CX_M_SLIDE:
        if (slot == 2u) return CX_F_SLIDE_F;
        return m->timer > 14u ? CX_F_SLIDE1 : CX_F_SLIDE2;
    case CX_M_TACKLE:
        return CX_F_POKE;
    case CX_M_DOWN:
        if (keeper) return CX_K_GK_LIE;
        return m->timer > 26u ? CX_F_FALL : (m->timer > 8u ? CX_F_DOWN : CX_F_GETUP);
    case CX_M_HEAD:
        return m->timer > 7u ? CX_F_HEAD_JUMP : CX_F_HEAD_HIT;
    case CX_M_CELEB: {
        /* each nation has its own way of showing it */
        uint8_t going = (uint8_t)(m->vx || m->vy), beat = (uint8_t)((m->timer >> 3) & 1u);
        uint8_t c = w->celeb;
        /* the men who join him each do their own: a team's celebration is a crowd, not a drill */
        if (i != w->scorer) {
            static const uint8_t k_group[6] = { CX_C_RUN, CX_C_DANCE, CX_C_SAMBA, CX_C_SCREAM, CX_C_HIPS, CX_C_SKY };
            c = k_group[(uint8_t)(w->celeb + i) % 6u];
        }
        /* on his way to the crowd, arms out; the nation's own when he gets there */
        if (going && c != CX_C_KNEE && c != CX_C_SKATE) return CX_F_CELEB_RUN;
        switch (c) {
        case CX_C_RUN: return going ? CX_F_CELEB_RUN : CX_F_CELEB_JUMP;
        case CX_C_KNEE: return going ? CX_F_CELEB_RUN : CX_F_CELEB_KNEE;
        case CX_C_SHIRT: return CX_F_CELEB_SHIRT;
        case CX_C_BABY: *flip = beat; return CX_F_CELEB_BABY;
        case CX_C_DANCE: return beat ? CX_F_CELEB_DANCE : CX_F_SAMBA1;
        case CX_C_HIPS: return going ? CX_F_CELEB_RUN : (beat ? CX_F_HIP1 : CX_F_HIP2);
        case CX_C_BELLY: return going ? CX_F_CELEB_RUN : CX_F_BELLY;
        case CX_C_FLIP: return (m->timer & 63u) > 40u ? CX_F_FLIP : CX_F_CELEB_JUMP;
        case CX_C_SAMBA: return beat ? CX_F_SAMBA1 : CX_F_SAMBA2;
        case CX_C_PRAY: return CX_F_PRAY;
        case CX_C_SCREAM: return CX_F_SCREAM;
        case CX_C_SKY: return CX_F_SKY;
        case CX_C_SKATE: return CX_F_SKATE;
        default: return CX_F_CELEB_JUMP;
        }
    }
    case CX_M_THROW:
        return CX_F_THROW1;
    case CX_M_THROWN:
        return CX_F_THROW2;
    case CX_M_DIVE:
        *flip = keeper_flip;
        if (m->dir == 6u) return m->timer > 18u ? CX_K_GK_UP1 : CX_K_GK_UP2;
        return m->timer > 18u ? CX_K_GK_NEAR1 : CX_K_GK_NEAR2;
    case CX_M_HOLD:
        *flip = keeper_flip;
        return CX_K_GK_HOLD;
    case CX_M_BEATEN:
        return CX_F_SKID;
    case CX_M_FEINT:
        return (m->timer >> 2) & 1u ? CX_F_FEINT1 : CX_F_FEINT2;
    case CX_M_SAD:
        if (keeper) { *flip = keeper_flip; return CX_K_GK_SIT; }
        return (i & 1u) ? CX_F_KNEEL : CX_F_DISBELIEF;
    default:
        break;
    }
    if (m->vx || m->vy) {
        /* turning back on himself he digs in; otherwise his legs follow the ground he covers */
        if (((m->vx > 8 && m->gx < -8) || (m->vx < -8 && m->gx > 8))) return CX_F_SKID;
        return (uint8_t)(slot * 5u + 1u + (m->anim & 3u));
    }
    if (keeper && w->phase == CX_P_PLAY) {
        int16_t dx = (int16_t)(U(w->ball.x) - U(m->x));
        if (dx > -420 && dx < 420) { *flip = keeper_flip; return CX_K_GK_READY; }
    }
    return (uint8_t)(slot * 5u);
}

/* Depth order, nearest the camera last. A man changing places costs two slots rewritten whole
 * in the blank. Four pixels of slack keep two men at one depth from swapping back and forth. */
static uint8_t s_order[SLOTS], s_order_live;
/* on screen last frame. A man must go further out to leave than to come in: at the edge,
 * a man in for one frame and out the next shuffled the order of every slot after his
 * (each rewritten whole, more than a blank holds) and back again */
static uint8_t s_seen[SLOTS];

void NEOGEO_USER cx_men_reset(void)
{
    uint8_t i;
    s_order_live = 0;
    for (i = 0; i < SLOTS; i++) s_seen[i] = 0;
    slots_begin();
}

static uint8_t NEOGEO_USER band_of(uint8_t i, uint16_t s)
{
    uint8_t b = s_band[i];
    if (b == 0u && s >= BAND_MID + BAND_SLACK) b = 1;
    else if (b == 1u && s < BAND_MID - BAND_SLACK) b = 0;
    else if (b == 1u && s >= BAND_NEAR + BAND_SLACK) b = 2;
    else if (b == 2u && s < BAND_NEAR - BAND_SLACK) b = 1;
    s_band[i] = b;
    return b;
}

#ifdef CX_RENDER_CHECK
/* (check builds) the drawing as it was, run beside the new one each frame */
static void NEOGEO_USER slot_set(uint8_t k, uint16_t art, uint8_t frame, int16_t x, int16_t y, uint8_t flip, uint8_t bank, uint16_t num)
{
    CxSlot *s = &s_want[k];
    const uint8_t *sp = cx_spans[art - CX_ART_FIG0][frame];
    const NGArtAsset *a = asset(art);
    uint8_t c0 = (uint8_t)(sp[0] >> 4), c1 = (uint8_t)(sp[0] & 15u), r0 = (uint8_t)(sp[1] >> 4);
    int16_t fx = (int16_t)(x - (flip ? (int16_t)(CX_MAN_W - CX_MAN_AX) : CX_MAN_AX));
    int16_t fy = (int16_t)(y - CX_MAN_AY);
    int16_t sx = (int16_t)(fx + (flip ? (int16_t)((3 - c1) << 4) : (int16_t)(c0 << 4)));
    int16_t sy = (int16_t)(fy + (int16_t)(r0 << 4));
    s->tile = (uint16_t)(a->tile_base + (uint16_t)frame * 20u);
    s->bank = bank;
    s->flip = flip;
    s->x = fx;
    s->y = fy;
    s->c0 = c0;
    s->c1 = c1;
    s->r0 = r0;
    s->rows = (uint8_t)((sp[1] & 15u) - r0 + 1u);
    s->strips = (uint8_t)(c1 - c0 + 1u);
    s->yw = (uint16_t)((((uint16_t)(496 - sy) & 0x1FFu) << 7) | s->rows);
    s->xw = (uint16_t)(((uint16_t)sx & 0x1FFu) << 7);
    s->num = 0;
    s->nyw = s->nxw = 0;
    if (num && sp[3]) {
        /* on his shirt, the right way round even when he is mirrored */
        int16_t nx = (int16_t)(fx + (flip ? CX_MAN_W - 1 - sp[2] : sp[2]) - 8);
        int16_t ny = (int16_t)(fy + sp[3] - 8);
        s->num = num;
        s->nyw = (uint16_t)((((uint16_t)(496 - ny) & 0x1FFu) << 7) | 1u);
        s->nxw = (uint16_t)(((uint16_t)nx & 0x1FFu) << 7);
    }
    s->vis = 1;
}

static void NEOGEO_USER men_render_old(const int16_t *ux, const int16_t *uy, const uint8_t *frame, const uint8_t *flip)
{
    int16_t sy[SLOTS], sx[SLOTS];
    uint8_t vis[SLOTS], bands[SLOTS], i, k;
    uint16_t digits = asset(CX_ART_DIGITS)->tile_base;
    if (!s_order_live) { for (i = 0; i < SLOTS; i++) s_order[i] = i; s_order_live = 1; }
    for (i = 0; i < SLOTS; i++) {
        int16_t px, py, wx = i == REF_I ? s_ref_ux : ux[i], wy = i == REF_I ? s_ref_uy : uy[i];
        uint16_t s;
        /* a man nobody can see sorts to the end: at depth 0 he sat among the men at the
         * top of the screen, and every time one walked out of range the order shuffled
         * and slots changed hands (men vanishing or changing shirts for a frame) */
        if (wx == -32000 || (i == REF_I && !s_ref_on)) { vis[i] = 0; s_seen[i] = 0; sy[i] = 32000; continue; }
        cx_project(wx, wy, &px, &py, &s);
        sx[i] = (int16_t)(px - cx_camx);
        sy[i] = (int16_t)(py - cx_camy);
        bands[i] = band_of(i, s);
        if (s_seen[i]) vis[i] = (uint8_t)(sx[i] >= -96 && sx[i] <= 416 && sy[i] >= -48 && sy[i] <= 330);
        else vis[i] = (uint8_t)(sx[i] >= -48 && sx[i] <= 368 && sy[i] >= -8 && sy[i] <= 300);
        s_seen[i] = vis[i];
        if (!vis[i]) sy[i] = 32000;
    }
    /* insertion sort: the list is nearly in order from the last frame */
    for (k = 1; k < SLOTS; k++) {
        uint8_t a = s_order[k], j = k;
        while (j > 0u && sy[s_order[j - 1u]] > sy[a] + 4) { s_order[j] = s_order[j - 1u]; j--; }
        s_order[j] = a;
    }
    for (k = 0; k < SLOTS; k++) {
        i = s_order[k];
        if (!vis[i]) { s_want[k].vis = 0; continue; }
        if (i == REF_I) {
            slot_set(k, CX_ART_REF, (uint8_t)(bands[i] * CX_REF_FRAMES + s_ref_frame), sx[i], sy[i], s_ref_flip, BANK_REF, 0);
        } else {
            uint8_t per = (uint8_t)(s_art[i] == CX_ART_KEEPER ? CX_KEEP_FRAMES : CX_FIG_FRAMES);
            slot_set(k, s_art[i], (uint8_t)(bands[i] * per + frame[i]), sx[i], sy[i], flip[i], (uint8_t)(BANK_MEN + i), (uint16_t)(digits + s_shirt[i]));
        }
    }
}
#endif

/* Slot s from figure i's pose (band included) with its frame's corner at x, y. The final
 * sprite words are worked out here, in frame time, so the blank only compares and copies. */
static inline __attribute__((always_inline)) void slot_fill(CxSlot *s, uint8_t i, uint8_t frame, int16_t x, int16_t y,
                                                            uint8_t flip, uint8_t bank)
{
    const uint8_t *sp = s_fig_sp[i][frame];
    uint8_t c0 = (uint8_t)(sp[0] >> 4), c1 = (uint8_t)(sp[0] & 15u), r0 = (uint8_t)(sp[1] >> 4);
    uint8_t rows = (uint8_t)((sp[1] & 15u) - r0 + 1u);
    uint16_t f = frame, num = s_fig_num[i];
    int16_t fx = (int16_t)(x - (flip ? (int16_t)(CX_MAN_W - CX_MAN_AX) : CX_MAN_AX));
    int16_t fy = (int16_t)(y - CX_MAN_AY);
    int16_t sx = (int16_t)(fx + (flip ? (int16_t)((3 - c1) << 4) : (int16_t)(c0 << 4)));
    int16_t sy = (int16_t)(fy + (int16_t)(r0 << 4));
    s->tile = (uint16_t)(s_fig_tile[i] + (f << 4) + (f << 2));         /* 20 tiles a pose */
    s->bank = bank;
    s->flip = flip;
    s->x = fx;
    s->y = fy;
    s->c0 = c0;
    s->c1 = c1;
    s->r0 = r0;
    s->rows = rows;
    s->strips = (uint8_t)(c1 - c0 + 1u);
    s->yw = (uint16_t)((((uint16_t)(496 - sy) & 0x1FFu) << 7) | rows);
    s->xw = (uint16_t)(((uint16_t)sx & 0x1FFu) << 7);
    s->num = 0;
    s->nyw = s->nxw = 0;
    if (num && sp[3]) {
        /* on his shirt, the right way round even when he is mirrored */
        int16_t nx = (int16_t)(fx + (flip ? CX_MAN_W - 1 - sp[2] : sp[2]) - 8);
        int16_t ny = (int16_t)(fy + sp[3] - 8);
        s->num = num;
        s->nyw = (uint16_t)((((uint16_t)(496 - ny) & 0x1FFu) << 7) | 1u);
        s->nxw = (uint16_t)(((uint16_t)nx & 0x1FFu) << 7);
    }
    s->vis = 1;
}

/* Where figure i stands on the screen, his size band, and whether he is on it. A man
 * nobody can see sorts to the end: at depth 0 he sat among the men at the top of the
 * screen, and every time one walked out of range the order shuffled and slots changed
 * hands (men vanishing or changing shirts for a frame). */
static inline __attribute__((always_inline)) void fig_place(uint8_t i, int16_t wx, int16_t wy, int16_t *sx, int16_t *sy,
                                                            uint8_t *vis, uint8_t *band)
{
    int16_t px, py, x, y;
    uint16_t s;
    uint8_t v;
    cx_project(wx, wy, &px, &py, &s);
    x = (int16_t)(px - cx_camx);
    y = (int16_t)(py - cx_camy);
    *band = band_of(i, s);
    if (s_seen[i]) v = (uint8_t)(x >= -96 && x <= 416 && y >= -48 && y <= 330);
    else v = (uint8_t)(x >= -48 && x <= 368 && y >= -8 && y <= 300);
    s_seen[i] = v;
    *vis = v;
    *sx = x;
    *sy = v ? y : 32000;
}

/* The men from their pitch positions (units), frames and mirrors: live play and the replay
 * draw through here alike. A man off the pitch has x -32000. */
static void NEOGEO_USER men_render_new(const int16_t *ux, const int16_t *uy, const uint8_t *frame, const uint8_t *flip)
{
    int16_t sy[SLOTS], sx[SLOTS];
    uint8_t vis[SLOTS], bands[SLOTS], i, k;
    CxSlot *s;
    if (!s_order_live) { for (i = 0; i < SLOTS; i++) s_order[i] = i; s_order_live = 1; }
    for (i = 0; i < CX_MEN; i++) {
        if (ux[i] == -32000) { vis[i] = 0; s_seen[i] = 0; sy[i] = 32000; continue; }
        fig_place(i, ux[i], uy[i], &sx[i], &sy[i], &vis[i], &bands[i]);
    }
    if (!s_ref_on || s_ref_ux == -32000) { vis[REF_I] = 0; s_seen[REF_I] = 0; sy[REF_I] = 32000; }
    else fig_place(REF_I, s_ref_ux, s_ref_uy, &sx[REF_I], &sy[REF_I], &vis[REF_I], &bands[REF_I]);
    /* insertion sort: the list is nearly in order from the last frame (32000 + 4 still fits) */
    for (k = 1; k < SLOTS; k++) {
        uint8_t a = s_order[k], j = k;
        int16_t lim = (int16_t)(sy[a] + 4);
        while (j > 0u && sy[s_order[j - 1u]] > lim) { s_order[j] = s_order[j - 1u]; j--; }
        s_order[j] = a;
    }
    for (k = 0, s = s_want; k < SLOTS; k++, s++) {
        uint8_t f, fl, bank, b;
        i = s_order[k];
        if (!vis[i]) { s->vis = 0; continue; }
        if (i == REF_I) { f = s_ref_frame; fl = s_ref_flip; bank = BANK_REF; }
        else { f = frame[i]; fl = flip[i]; bank = (uint8_t)(BANK_MEN + i); }
        b = bands[i];
        /* a tall man is drawn a size up, a slight one a size down: the depth bands are the
         * figure's sizes, so a big centre-half stands a head over the men round him */
        if (s_fig_build[i] == 1u && b < 2u) b++;
        else if (s_fig_build[i] == 2u && b > 0u) b--;
        if (b) f = (uint8_t)(f + (b == 1u ? s_fig_per[i] : (uint8_t)(s_fig_per[i] << 1)));
        slot_fill(s, i, f, sx[i], sy[i], fl, bank);
    }
}

#ifdef CX_RENDER_CHECK
uint16_t cx_render_checks, cx_render_diffs;
static uint8_t s_chk_order[SLOTS], s_chk_seen[SLOTS], s_chk_band[SLOTS], s_chk_live;
static CxSlot s_chk_want[SLOTS];
static void NEOGEO_USER men_render(const int16_t *ux, const int16_t *uy, const uint8_t *frame, const uint8_t *flip)
{
    uint8_t o[SLOTS], sn[SLOTS], bd[SLOTS], live = s_order_live, k, bad = 0;
    for (k = 0; k < SLOTS; k++) { o[k] = s_order[k]; sn[k] = s_seen[k]; bd[k] = s_band[k]; }
    men_render_old(ux, uy, frame, flip);
    for (k = 0; k < SLOTS; k++) {
        s_chk_order[k] = s_order[k]; s_chk_seen[k] = s_seen[k]; s_chk_band[k] = s_band[k]; s_chk_want[k] = s_want[k];
        s_order[k] = o[k]; s_seen[k] = sn[k]; s_band[k] = bd[k];
    }
    s_chk_live = s_order_live;
    s_order_live = live;
    men_render_new(ux, uy, frame, flip);
    if (s_chk_live != s_order_live) bad = 1;
    for (k = 0; k < SLOTS; k++) {
        const CxSlot *a = &s_chk_want[k], *b = &s_want[k];
        if (s_chk_order[k] != s_order[k] || s_chk_seen[k] != s_seen[k] || s_chk_band[k] != s_band[k] || a->vis != b->vis) bad = 1;
        else if (a->vis && (a->tile != b->tile || a->num != b->num || a->yw != b->yw || a->nyw != b->nyw || a->xw != b->xw ||
                            a->nxw != b->nxw || a->bank != b->bank || a->flip != b->flip || a->strips != b->strips ||
                            a->c0 != b->c0 || a->c1 != b->c1 || a->r0 != b->r0 || a->rows != b->rows || a->x != b->x || a->y != b->y)) bad = 1;
    }
    cx_render_checks++;
    if (bad) cx_render_diffs++;
}
#else
#define men_render men_render_new
#endif

/* ------------------------------------------------------------- replay */
#define REPLAY_N 96                 /* samples, one every other frame: about three seconds */
typedef struct { int16_t x, y; uint8_t frame, flags; } CxRepMan;        /* flags: 1 flip */
typedef struct { CxRepMan m[CX_MEN]; int16_t bx, by, bh; uint8_t spin, pad; } CxRepFrame;
static CxRepFrame s_rep[REPLAY_N];
static uint8_t s_rep_head, s_rep_count;

void NEOGEO_USER cx_replay_reset(void) { s_rep_head = s_rep_count = 0; }
uint8_t NEOGEO_USER cx_replay_ready(void) { return (uint8_t)(s_rep_count > 20u); }

void NEOGEO_USER cx_men_draw(const CxWorld *w, uint8_t record)
{
    int16_t ux[CX_MEN], uy[CX_MEN];
    uint8_t fr[CX_MEN], fl[CX_MEN], i;
    CxRepFrame *r = &s_rep[s_rep_head];
    CxRepMan *rm = r->m;
    const CxMan *m = w->m;
    int16_t mid = (int16_t)(cx_camx + 160 - CX_PCX + CX_LEN / 2);     /* the screen's centre, near enough in world x */
    int16_t left = (int16_t)(mid - 340), right = (int16_t)(mid + 340);
    for (i = 0; i < CX_MEN; i++, m++) {
        int16_t x = U(m->x), y = U(m->y);
        uint8_t f = 0, flp = 0;
        /* well off the screen: nothing to draw, and the replay will not look there */
        if (m->state == CX_M_OFF || x < left || x > right) x = -32000;
        else f = man_frame(w, i, &flp);
        ux[i] = x;
        uy[i] = y;
        fr[i] = f;
        fl[i] = flp;
        if (record) {
            rm->x = x;
            rm->y = y;
            rm->frame = f;
            rm->flags = flp;
            rm++;
        }
    }
    if (record) {
        r->bx = U(w->ball.x); r->by = U(w->ball.y); r->bh = U(w->ball.h); r->spin = w->ball.spin;
        if (++s_rep_head >= REPLAY_N) s_rep_head = 0;
        if (s_rep_count < REPLAY_N) s_rep_count++;
    }
    men_render(ux, uy, fr, fl);
}

/* The referee keeps a few strides off the ball, on the far side of play; at a stoppage he
 * hurries to it. He goes through the depth order with the men. */
void NEOGEO_USER cx_ref_update(const CxWorld *w)
{
    int16_t bx = U(w->ball.x), by = U(w->ball.y);
    int16_t tx = (int16_t)(bx + (bx < CX_LEN / 2 ? 150 : -150)), ty = (int16_t)(by - 130);
    int16_t dx, dy;
    uint8_t moving = 0, frame, flip, stop = (uint8_t)(w->phase == CX_P_STOP);
    int16_t step = (int16_t)(stop ? 3 : 2);
    if (stop) { tx = (int16_t)(bx + 30); ty = (int16_t)(by - 24); }
    if (ty < 30) ty = (int16_t)(by + (stop ? 24 : 130));
    dx = (int16_t)(tx - s_rx);
    dy = (int16_t)(ty - s_ry);
    if (dx > 24 || dx < -24 || dy > 24 || dy < -24) {
        if (dx > 1) s_rx = (int16_t)(s_rx + step); else if (dx < -1) s_rx = (int16_t)(s_rx - step);
        if (dy > 1) s_ry = (int16_t)(s_ry + step - 1); else if (dy < -1) s_ry = (int16_t)(s_ry - step + 1);
        s_rdir = (uint8_t)(dx >= 0 ? (dy > 24 ? 1u : (dy < -24 ? 7u : 0u)) : (dy > 24 ? 3u : (dy < -24 ? 5u : 4u)));
        s_ranim++;
        moving = 1;
    }
    flip = k_dir_flip[s_rdir];
    if (moving) frame = (uint8_t)(k_dir_slot[s_rdir] * 5u + 1u + ((s_ranim >> 2) & 3u));
    else if (w->phase != CX_P_PLAY && w->phase_t < 50u) { frame = CX_R_REF_WHISTLE; flip = 0; }
    /* after a foul he stands with his hands on his hips waving the complaints away */
    else if (stop || ((w->phase == CX_P_FREEKICK || w->phase == CX_P_PENALTY) && w->phase_t < 80u)) { frame = (w->phase_t >> 4) & 1u ? CX_R_REF_WAVE : CX_R_REF_HIPS; flip = 0; }
    else if (w->adv_t) { frame = CX_R_REF_PLAY_ON; flip = (uint8_t)(!cx_attacks_right(w, w->adv_team)); }
    else frame = (uint8_t)(k_dir_slot[s_rdir] * 5u);
    s_ref_ux = s_rx;
    s_ref_uy = s_ry;
    s_ref_frame = frame;
    s_ref_flip = flip;
    s_ref_on = 1;
}

void NEOGEO_USER cx_ball_at(int16_t bx, int16_t by, int16_t h, uint8_t spin)
{
    int16_t px, py, x, ground, lift;
    uint16_t s;
    uint8_t f = (uint8_t)((spin >> 2) & 3u);
    cx_project(bx, by, &px, &py, &s);
    x = (int16_t)(px - cx_camx - 8);
    ground = (int16_t)(py - cx_camy);
    if (x < -16 || x > 336 || ground < -16 || ground > 260) { cx_spr_hide(&cx_ball_s); cx_spr_hide(&cx_shadow); return; }
    lift = (int16_t)(cx_mul((int16_t)(cx_mul(h, CX_Z_SCALE) >> 8), (int16_t)s) >> 8);
    cx_spr_show(&cx_shadow, SPR_SHADOW, CX_ART_SHADOW, 0, x, (int16_t)(ground - 9), 0);
    cx_spr_show(&cx_ball_s, SPR_BALL, CX_ART_BALL, f, x, (int16_t)(ground - 13 - lift), 0);
}

void NEOGEO_USER cx_ball_draw(const CxWorld *w)
{
    /* The keeper's hold pose already contains the ball in his hands. */
    if (w->ball.owner < CX_MEN && w->m[w->ball.owner].role == 0 &&
        w->m[w->ball.owner].state == CX_M_HOLD) {
        cx_spr_hide(&cx_ball_s);
        cx_spr_hide(&cx_shadow);
        return;
    }
    cx_ball_at(U(w->ball.x), U(w->ball.y), U(w->ball.h), w->ball.spin);
}

/* The near post and the bar of whichever goal is on screen, over the men. */
void NEOGEO_USER cx_goals_draw(uint8_t bulge)
{
    int16_t left = (int16_t)(CX_GOAL_X - cx_camx);
    int16_t right = (int16_t)(CX_PIC_W - CX_GOAL_X - CX_GOAL_W - cx_camx);
    int16_t y = (int16_t)(CX_GOAL_Y - cx_camy);
    if (left > -CX_GOAL_W && left < 320) cx_spr_show(&cx_goal, SPR_GOAL, CX_ART_GOAL, bulge, left, y, 0);
    else if (right > -CX_GOAL_W && right < 320) cx_spr_show(&cx_goal, SPR_GOAL, CX_ART_GOAL, bulge, right, y, 1);
    else cx_spr_hide(&cx_goal);
}

void NEOGEO_USER cx_cursors_draw(const CxWorld *w)
{
    uint8_t t;
    for (t = 0; t < 2u; t++) {
        uint8_t c = w->ctrl[t];
        int16_t x, y;
        uint16_t s;
        if (!w->human[t] || c == CX_NOBODY || w->phase == CX_P_GOAL || w->phase >= CX_P_HALF) { cx_spr_hide(&cx_cursor[t]); continue; }
        /* in the aim view the arrow marks the target instead of the man */
        if (w->aim_on && t == w->restart_team) cx_project(w->aim_x, w->aim_y, &x, &y, &s);
        else cx_project(U(w->m[c].x), U(w->m[c].y), &x, &y, &s);
        x = (int16_t)(x - cx_camx - 8);
        y = (int16_t)(y - cx_camy - (int16_t)(s >> 2) - 8 - ((w->frame >> 3) & 1u));
        if (x < -16 || x > 336 || y < -16 || y > 240) { cx_spr_hide(&cx_cursor[t]); continue; }
        cx_spr_show(&cx_cursor[t], (uint16_t)(SPR_CURSOR + t), CX_ART_CURSOR, (uint8_t)(t * 2u + ((w->frame >> 4) & 1u)), x, y, 0);
    }
}

/* The last seconds before the goal again, slower, under the REPLAY banner. */
void NEOGEO_USER cx_replay(void)
{
    /* the last moments only, at half speed: a replay must not cost the match its pace */
    uint8_t n = (uint8_t)(s_rep_count > 60u ? 60u : s_rep_count), i, k, step;
    uint8_t first = (uint8_t)((s_rep_head + REPLAY_N - n) % REPLAY_N);
    cx_spr_hide(&cx_cursor[0]);
    cx_spr_hide(&cx_cursor[1]);
    cx_spr_hide(&cx_portrait);
    cx_spr_show(&cx_banner, SPR_BANNER, CX_ART_BANNER, BANNER_REPLAY, 80, 40, 0);
    for (i = 0; i < n; i++) {
        const CxRepFrame *r = &s_rep[(first + i) % REPLAY_N];
        int16_t ux[CX_MEN], uy[CX_MEN];
        uint8_t fr[CX_MEN], fl[CX_MEN];
        for (k = 0; k < CX_MEN; k++) {
            ux[k] = r->m[k].x; uy[k] = r->m[k].y; fr[k] = r->m[k].frame; fl[k] = (uint8_t)(r->m[k].flags & 1u);
        }
        for (step = 0; step < 2u; step++) {
            int16_t px, py, tx, ty;
            uint16_t s, pressed;
            cx_project(r->bx, r->by, &px, &py, &s);
            tx = (int16_t)(px - 160);
            ty = (int16_t)(py - 124);
            (void)cx_input(&pressed);
            if (tx < 0) tx = 0;
            if (tx > CAM_MAX_X) tx = CAM_MAX_X;
            if (ty < 0) ty = 0;
            if (ty > CAM_MAX_Y) ty = CAM_MAX_Y;
            cx_camx = (int16_t)(cx_camx + (tx - cx_camx) / 4);
            cx_camy = (int16_t)(cx_camy + (ty - cx_camy) / 4);
            cx_pitch_draw(cx_camx, cx_camy);
            men_render(ux, uy, fr, fl);
            cx_ball_at(r->bx, r->by, r->bh, r->spin);
            cx_goals_draw(0);
            if (pressed & (START1 | CX_A)) { i = n; break; }     /* the player can skip it */
        }
    }
    s_rep_count = 0;
    cx_spr_hide(&cx_banner);
}

void NEOGEO_USER cx_flag_show(uint8_t which, uint8_t nation, int16_t x, int16_t y)
{
    cx_spr_show(&cx_flag[which], (uint16_t)(SPR_FLAG + which * 2u), CX_ART_FLAGS, nation, x, y, 0);
}

void NEOGEO_USER cx_hero_show(CxSpr *s, uint16_t slot, uint16_t art, uint8_t frame, int16_t x, int16_t y, uint8_t flip, uint8_t bank)
{
    cx_spr_show_bank(s, slot, art, frame, x, y, flip, bank);
}
