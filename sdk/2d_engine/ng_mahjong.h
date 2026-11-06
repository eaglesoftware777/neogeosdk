#ifndef NG_MAHJONG_H
#define NG_MAHJONG_H
/*
 * ng_mahjong: a deterministic Mahjong core (v1.7.7 genre engine).
 * Selected with GAME_GENRE_ENGINE = mahjong in a game's game.mk.
 *
 * Two unrelated games share the tiles and nothing else, so they have
 * separate contexts and calls:
 *
 *   NGMahjong       four-seat Mahjong under one named ruleset,
 *                   NGM_RULES_HK_OLD_STYLE ("Hong Kong Old Style", the
 *                   subset documented in docs/GENRE_ENGINES_V1.7.7.md):
 *                   wall, deal, draws and discards, chow / pung / kong
 *                   claims with their priorities, self-drawn and discard
 *                   wins, faan scoring and payments, optional flowers,
 *                   exhaustive draws, dealer rotation, and CPU seats.
 *   NGMahjongSol    Mahjong solitaire: a layout of stacked tiles, free-tile
 *                   rules, pair removal, hints, undo, shuffles, and deals
 *                   that are solvable by construction.
 *
 * Neither touches video RAM or the sound driver: the game draws tiles with
 * the SDK's sprites and FIX layer from the context and the events it
 * drains. Everything lives in the caller's context: no heap, no floating
 * point, no hidden globals. Recursion is bounded (hand analysis looks at
 * most four sets deep). Each context has its own seeded generator, so a
 * seed and the same commands replay a game exactly.
 *
 * CPU seats decide from what that seat may see (NGMahjongView: its own
 * hand, every discard, meld and flower, the wall count), never from other
 * hands or the wall order. ng_mahjong_cpu_choose() is public, so a game can
 * also offer the CPU's choice as a hint to a human seat.
 *
 * Not implemented (documented limits): robbing a kong, seven pairs,
 * Japanese riichi rules (yaku, furiten, dora), Chinese Official scoring,
 * and dead-wall variants.
 */
#include <stdint.h>
#include "ng_defs.h"
#ifdef __cplusplus
extern "C" {
#endif

/* ---- tiles ------------------------------------------------------------- */
/* Kinds: 0-8 characters 1-9, 9-17 dots 1-9, 18-26 bamboo 1-9,
 * 27-30 winds east, south, west, north, 31-33 dragons red, green, white,
 * 34-37 flowers 1-4, 38-41 seasons 1-4. */
#define NGM_KINDS          34u     /* kinds that make hands */
#define NGM_KINDS_ALL      42u
#define NGM_CHARACTERS     0u
#define NGM_DOTS           9u
#define NGM_BAMBOO         18u
#define NGM_EAST           27u
#define NGM_SOUTH          28u
#define NGM_WEST           29u
#define NGM_NORTH          30u
#define NGM_RED            31u
#define NGM_GREEN          32u
#define NGM_WHITE          33u
#define NGM_FLOWER         34u
#define NGM_SEASON         38u
#define NGM_NO_TILE        0xFFu
#define NGM_IS_SUIT(k)     ((k) < 27u)
#define NGM_IS_HONOR(k)    ((k) >= 27u && (k) < 34u)
#define NGM_IS_BONUS(k)    ((k) >= 34u && (k) < 42u)
#define NGM_SUIT_OF(k)     ((k) < 9u ? 0u : (k) < 18u ? 1u : (k) < 27u ? 2u : 3u)
#define NGM_RANK(k)        ((uint8_t)((k) < 27u ? (k) % 9u + 1u : 0u))   /* 1..9, 0 for honours */

/* ---- results ----------------------------------------------------------- */
enum {
    NGM_OK = 0,
    NGM_ERR_ARG = -1,
    NGM_ERR_TURN = -2,      /* not this seat's decision now */
    NGM_ERR_ILLEGAL = -3,   /* not allowed with this hand */
    NGM_ERR_STATE = -4,     /* the hand or game is over */
    NGM_ERR_LAYOUT = -5,    /* solitaire layout or deal rejected */
    NGM_ERR_SAVE = -6
};

/* ---- four-seat game ---------------------------------------------------- */
#define NGM_SEATS          4u
#define NGM_HAND_MAX       14u
#define NGM_MELDS_MAX      4u
#define NGM_DISCARDS_MAX   40u
#define NGM_WALL_MAX       144u
#define NGM_EVENT_QUEUE    32u
#define NGM_HUMAN          0xFFu   /* ng_mahjong_set_cpu(): this seat is a player */

enum { NGM_RULES_HK_OLD_STYLE = 1 };

typedef struct {
    uint8_t ruleset;        /* NGM_RULES_HK_OLD_STYLE */
    uint8_t flowers;        /* 1: play the eight bonus tiles */
    uint8_t min_faan;       /* smallest hand that may win (0..3 usual) */
    uint8_t limit_faan;     /* faan cap, also the limit hands' value (8..13) */
    uint8_t rounds;         /* prevailing winds to play: 1 east only .. 4 */
    uint8_t pad;
    uint16_t base_points;   /* points for a 0-faan hand */
    int32_t start_score;    /* each seat's score at the start */
} NGMahjongRules;

/* Melds */
enum { NGM_MELD_CHOW = 1, NGM_MELD_PUNG, NGM_MELD_KONG, NGM_MELD_KONG_CONCEALED };

typedef struct {
    uint8_t type;           /* NGM_MELD_* */
    uint8_t kind;           /* lowest tile of a chow, the tile of the others */
    uint8_t from;           /* seat it was claimed from (own seat if none) */
    uint8_t claimed;        /* the claimed tile (NGM_NO_TILE if none) */
} NGMahjongMeld;

typedef struct {
    uint8_t tiles[NGM_HAND_MAX];     /* concealed tiles, sorted */
    uint8_t count;
    uint8_t nmelds;
    NGMahjongMeld melds[NGM_MELDS_MAX];
    uint8_t flowers;                 /* bit n: bonus tile 34 + n */
    uint8_t ndiscards;
    uint8_t discards[NGM_DISCARDS_MAX];
    uint8_t discard_taken[(NGM_DISCARDS_MAX + 7u) / 8u];   /* claimed by someone */
} NGMahjongHand;

/* Phases (NGMahjong.phase). */
enum {
    NGM_PH_START = 0,       /* call ng_mahjong_update to deal */
    NGM_PH_TURN,            /* seat `turn` decides: discard, self-drawn win or kong */
    NGM_PH_CLAIM,           /* the other seats may claim the last discard */
    NGM_PH_HAND_OVER,       /* see `result`; ng_mahjong_next_hand() goes on */
    NGM_PH_GAME_OVER
};

/* Actions for ng_mahjong_command() and bits for ng_mahjong_wants(). */
enum {
    NGM_ACT_DISCARD = 1,    /* arg: tile kind */
    NGM_ACT_TSUMO,          /* win on the tile just drawn */
    NGM_ACT_KONG_SELF,      /* arg: kind; four concealed, or the fourth to an exposed pung */
    NGM_ACT_PASS,           /* claim nothing */
    NGM_ACT_CHOW,           /* arg: 0 discard is the lowest, 1 the middle, 2 the highest tile */
    NGM_ACT_PUNG,
    NGM_ACT_KONG,           /* three in hand and the discard */
    NGM_ACT_RON             /* win on the discard */
};
#define NGM_CAN_DISCARD    0x001u
#define NGM_CAN_TSUMO      0x002u
#define NGM_CAN_KONG_SELF  0x004u
#define NGM_CAN_PASS       0x008u
#define NGM_CAN_CHOW_LOW   0x010u
#define NGM_CAN_CHOW_MID   0x020u
#define NGM_CAN_CHOW_HIGH  0x040u
#define NGM_CAN_PUNG       0x080u
#define NGM_CAN_KONG       0x100u
#define NGM_CAN_RON        0x200u

/* Scoring patterns (NGMahjongResult.patterns). */
#define NGM_PAT_SELF_DRAWN      0x00000001u  /* 1 */
#define NGM_PAT_CONCEALED       0x00000002u  /* 1 */
#define NGM_PAT_ALL_CHOWS       0x00000004u  /* 1 */
#define NGM_PAT_ALL_PUNGS       0x00000008u  /* 3 */
#define NGM_PAT_HALF_FLUSH      0x00000010u  /* 3 */
#define NGM_PAT_FULL_FLUSH      0x00000020u  /* 7 */
#define NGM_PAT_DRAGON_PUNG     0x00000040u  /* 1 each */
#define NGM_PAT_SEAT_WIND       0x00000080u  /* 1 */
#define NGM_PAT_ROUND_WIND      0x00000100u  /* 1 */
#define NGM_PAT_SMALL_DRAGONS   0x00000200u  /* 5, instead of the dragon pungs */
#define NGM_PAT_BIG_DRAGONS     0x00000400u  /* 8, instead of the dragon pungs */
#define NGM_PAT_SMALL_WINDS     0x00000800u  /* 10 */
#define NGM_PAT_BIG_WINDS       0x00001000u  /* limit */
#define NGM_PAT_ALL_HONORS      0x00002000u  /* 10 */
#define NGM_PAT_THIRTEEN        0x00004000u  /* limit: thirteen orphans */
#define NGM_PAT_NINE_GATES      0x00008000u  /* limit */
#define NGM_PAT_ALL_TERMINALS   0x00010000u  /* 10: only 1s, 9s */
#define NGM_PAT_LAST_TILE       0x00020000u  /* 1: on the wall's last tile */
#define NGM_PAT_KONG_DRAW       0x00040000u  /* 1: on a kong's replacement tile */
#define NGM_PAT_NO_FLOWERS      0x00080000u  /* 1 (flowers in play) */
#define NGM_PAT_OWN_FLOWER      0x00100000u  /* 1 each (flowers in play) */
#define NGM_PAT_HEAVENLY        0x00200000u  /* limit: dealer wins on the deal */

typedef struct {
    uint8_t winner;         /* seat, or NGM_NO_TILE for an exhaustive draw */
    uint8_t from;           /* the discarder, or the winner for a self-drawn win */
    uint8_t tile;           /* the winning tile */
    uint8_t faan;           /* capped at the rules' limit */
    uint32_t patterns;
    int32_t delta[NGM_SEATS];     /* score change this hand */
} NGMahjongResult;

/* What one seat may see. */
typedef struct {
    uint8_t seat, turn, phase, dealer;
    uint8_t seat_wind, round_wind;     /* NGM_EAST.. */
    uint8_t last_discard, discarder;   /* the tile up for claims */
    uint8_t drawn;                     /* the tile this seat just drew (NGM_NO_TILE) */
    uint8_t wall_left;
    uint16_t wants;                    /* NGM_CAN_* for this seat now */
    uint8_t counts[NGM_KINDS_ALL];     /* own concealed tiles by kind */
    uint8_t seen[NGM_KINDS];           /* tiles this seat can see: own hand, every discard and meld */
    uint8_t nmelds;                    /* own melds */
    NGMahjongMeld melds[NGM_MELDS_MAX];
    uint8_t min_faan;
} NGMahjongView;

enum {
    NGM_EV_NONE = 0,
    NGM_EV_DEAL,            /* a = dealer */
    NGM_EV_DRAW,            /* seat, tile (tile only for the drawing seat's eyes) */
    NGM_EV_FLOWER,          /* seat, tile set aside */
    NGM_EV_DISCARD,         /* seat, tile */
    NGM_EV_CLAIM,           /* seat, tile, a = NGM_MELD_*, b = from */
    NGM_EV_KONG_SELF,       /* seat, tile, a = NGM_MELD_KONG or _CONCEALED */
    NGM_EV_TURN,            /* seat: its decision is wanted */
    NGM_EV_CLAIM_WINDOW,    /* tile: claims are open */
    NGM_EV_WIN,             /* seat, tile, a = from, b = faan */
    NGM_EV_EXHAUSTED,       /* the wall ran out */
    NGM_EV_GAME_OVER
};

typedef struct {
    uint8_t type, seat, tile, a, b, pad;
} NGMahjongEvent;

typedef struct {
    NGMahjongRules rules;
    NGMahjongHand hand[NGM_SEATS];
    int32_t score[NGM_SEATS];
    uint8_t cpu[NGM_SEATS];            /* CPU level 0..2, or NGM_HUMAN */
    uint8_t wall[NGM_WALL_MAX];
    uint8_t wall_size, wall_head, wall_tail;   /* draws from head, replacements from tail */
    uint8_t wall_left;
    uint8_t phase, turn, dealer, round_wind, hand_no, dealer_moves;
    uint8_t last_discard, discarder, drawn, kong_draw;
    uint8_t first_turn;                /* nobody has claimed or discarded yet */
    uint8_t claim_want[NGM_SEATS];     /* NGM_ACT_* chosen in the claim window (0: waiting) */
    uint8_t claim_arg[NGM_SEATS];
    NGMahjongResult result;
    uint32_t seed, rng;
    uint16_t turns;                    /* discards this hand */
    uint8_t ev_head, ev_count;
    uint16_t ev_dropped;
    NGMahjongEvent ev[NGM_EVENT_QUEUE];
} NGMahjong;

#define NGM_SAVE_SIZE   1024u

/* Default rules: HK Old Style, no flowers, 0 minimum faan, 10 limit,
 * one round (east), 1 point base, 1000 start score. */
void     NEOGEO_USER ng_mahjong_rules_default(NGMahjongRules *rules);
int      NEOGEO_USER ng_mahjong_init(NGMahjong *m, const NGMahjongRules *rules, uint32_t seed);
/* Seat controller: CPU level 0 (easy), 1, 2 (careful) or NGM_HUMAN.
 * All seats start as CPUs; seat 0 is usually the player. */
void     NEOGEO_USER ng_mahjong_set_cpu(NGMahjong *m, uint8_t seat, uint8_t level);
/* One step: deals, draws, CPU decisions and claim resolution. Stops when a
 * human seat's decision is wanted (ng_mahjong_wants() is non-zero for it),
 * at the end of a hand, or after one discard has been resolved. */
void     NEOGEO_USER ng_mahjong_update(NGMahjong *m);
int      NEOGEO_USER ng_mahjong_command(NGMahjong *m, uint8_t seat, uint8_t action, uint8_t arg);
uint16_t NEOGEO_USER ng_mahjong_wants(const NGMahjong *m, uint8_t seat);
/* Start the next hand after NGM_PH_HAND_OVER (dealer kept on a dealer
 * win or a draw). Returns NGM_ERR_STATE once the last round is done. */
int      NEOGEO_USER ng_mahjong_next_hand(NGMahjong *m);
uint8_t  NEOGEO_USER ng_mahjong_event(NGMahjong *m, NGMahjongEvent *out);
void     NEOGEO_USER ng_mahjong_view(const NGMahjong *m, uint8_t seat, NGMahjongView *view);
/* The CPU's choice for a seat, from its view only: an NGM_ACT_* and arg. */
void     NEOGEO_USER ng_mahjong_cpu_choose(const NGMahjongView *view, uint8_t level, uint8_t *action, uint8_t *arg);
uint8_t  NEOGEO_USER ng_mahjong_seat_wind(const NGMahjong *m, uint8_t seat);

/* Hand analysis, for any 14 (or 13 + melds) tiles given as counts by kind. */
/* Shanten: tiles from ready (0 = one tile from winning, -1 = a winning hand). */
int8_t   NEOGEO_USER ng_mahjong_shanten(const uint8_t counts[NGM_KINDS], uint8_t melds);
/* 1 if counts (with melds already exposed) make four sets and a pair, or thirteen orphans. */
uint8_t  NEOGEO_USER ng_mahjong_is_win(const uint8_t counts[NGM_KINDS], uint8_t melds);
/* Score a winning hand for seat in the current game: fills r->faan and r->patterns. */
uint8_t  NEOGEO_USER ng_mahjong_score(const NGMahjong *m, uint8_t seat, uint8_t tile, uint8_t self_drawn,
                                      NGMahjongResult *r);
/* Every tile accounted for: wall, hands, melds, discards, flowers. */
uint8_t  NEOGEO_USER ng_mahjong_tiles_ok(const NGMahjong *m);
uint32_t NEOGEO_USER ng_mahjong_hash(const NGMahjong *m);
uint16_t NEOGEO_USER ng_mahjong_save(const NGMahjong *m, uint8_t *buf, uint16_t size);
int      NEOGEO_USER ng_mahjong_load(NGMahjong *m, const uint8_t *buf, uint16_t size);

/* ---- solitaire ----------------------------------------------------------- */
#define NGMS_MAX_TILES  144u
#define NGMS_NONE       0xFFu
#define NGMS_ROWS       32u     /* positions' y must be below this */

/* A tile position in half-tile units: a tile covers x..x+1, y..y+1 at height z. */
typedef struct { uint8_t x, y, z; } NGMahjongPos;

enum { NGMS_PLAYING = 0, NGMS_CLEARED, NGMS_STUCK };

enum {
    NGMS_EV_NONE = 0,
    NGMS_EV_SELECT,         /* a = position */
    NGMS_EV_DESELECT,       /* a = position */
    NGMS_EV_PAIR,           /* a, b = the two positions removed */
    NGMS_EV_MISMATCH,       /* a, b: not a pair (the new one is selected) */
    NGMS_EV_BLOCKED,        /* a: not a free tile */
    NGMS_EV_UNDO,           /* a, b put back */
    NGMS_EV_SHUFFLE,
    NGMS_EV_CLEARED,
    NGMS_EV_STUCK           /* no free pair left */
};

typedef struct { uint8_t type, a, b, pad; } NGMahjongSolEvent;

typedef struct {
    const NGMahjongPos *pos;           /* layout (ROM or caller RAM), n positions */
    uint8_t n;
    uint8_t kind[NGMS_MAX_TILES];      /* tile at each position */
    uint8_t gone[(NGMS_MAX_TILES + 7u) / 8u];
    uint8_t left;                      /* tiles still on the layout */
    uint8_t selected;                  /* NGMS_NONE */
    uint8_t status;                    /* NGMS_* */
    uint8_t undo_n;
    uint8_t undo[NGMS_MAX_TILES];      /* removed positions, in pairs */
    uint8_t solution[NGMS_MAX_TILES];  /* a removal order that clears this deal */
    /* Kept as tiles go and come back, so a free-tile test is a lookup: the
     * tiles still on top of each position, and still beside it (left in the
     * low four bits, right in the high four). */
    uint8_t cover[NGMS_MAX_TILES];
    uint8_t side[NGMS_MAX_TILES];
    /* The positions by row, and where each row starts in that list: a tile
     * can only touch or lie under tiles of its own and the next rows. */
    uint8_t by_row[NGMS_MAX_TILES];
    uint8_t row_first[NGMS_ROWS + 1u];
    uint16_t pairs_removed, shuffles, undos;
    uint32_t seed, rng;
    uint8_t ev_head, ev_count;
    NGMahjongSolEvent ev[16];
} NGMahjongSol;

/* Write the classic 144-tile turtle layout into out[144]; returns 144. */
uint8_t  NEOGEO_USER ng_mjsol_layout_turtle(NGMahjongPos *out);
/* Deal a layout (n even, at most 144) with a seed: every deal can be
 * cleared (tiles are laid by removing free pairs from the full layout).
 * Fails with NGM_ERR_LAYOUT if the layout can't be dealt that way. */
int      NEOGEO_USER ng_mjsol_init(NGMahjongSol *s, const NGMahjongPos *pos, uint8_t n, uint32_t seed);
uint8_t  NEOGEO_USER ng_mjsol_is_free(const NGMahjongSol *s, uint8_t i);
/* After setting pos, n or gone by hand (an editor, a test): make the
 * row index and the counts the free-tile test reads again. The calls here
 * keep them. */
void     NEOGEO_USER ng_mjsol_recount(NGMahjongSol *s);
uint8_t  NEOGEO_USER ng_mjsol_match(uint8_t a, uint8_t b);   /* same kind, or two flowers, or two seasons */
/* Select a tile: picks it, or removes it with the selected one if they pair. */
int      NEOGEO_USER ng_mjsol_select(NGMahjongSol *s, uint8_t i);
int      NEOGEO_USER ng_mjsol_undo(NGMahjongSol *s);
uint8_t  NEOGEO_USER ng_mjsol_hint(const NGMahjongSol *s, uint8_t *a, uint8_t *b);
uint8_t  NEOGEO_USER ng_mjsol_free_pairs(const NGMahjongSol *s);
/* Re-deal the tiles still on the layout over their positions (solvable). */
int      NEOGEO_USER ng_mjsol_shuffle(NGMahjongSol *s);
uint8_t  NEOGEO_USER ng_mjsol_event(NGMahjongSol *s, NGMahjongSolEvent *out);
uint32_t NEOGEO_USER ng_mjsol_hash(const NGMahjongSol *s);

#ifdef __cplusplus
}
#endif
#endif
