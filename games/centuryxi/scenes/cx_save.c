/*
 * cx_save.c -- Century XI: what stays between games.
 *
 * The block in docs/saves.md: on an arcade board the system ROM keeps it in
 * backup RAM; on a console a memory card keeps it, and with no card the game
 * plays on and simply forgets. It holds the options, the championship records
 * and one Road to 2030 in progress. Everything read back is checked before use:
 * a damaged or foreign block is ignored, never trusted.
 */
#include "cx_render.h"
#include "cx_tournament.h"

#define CX_SAVE_VERSION 5u                /* 5: the road keeps scorers, bookings and suspensions */
#define CX_SAVE_TITLE   "CENTURY XI          "

typedef struct {
    CxOptions options;
    uint8_t road_live;            /* 1: road holds a tournament to go back to */
    uint8_t pad;
    uint16_t played, won;
    uint8_t titles[CX_SQUADS];    /* championships per nation */
    CxTournament road;
} CxSaveData;

typedef char cx_save_fits[(sizeof(CxSaveData) + 14u <= 0x400u) ? 1 : -1];

static uint8_t s_card[NG_CARD_FILE_SIZE(sizeof(CxSaveData))];

static CxSaveData *NEOGEO_USER data(void)
{
    if (!ng_save_valid(CX_SAVE_VERSION, sizeof(CxSaveData))) ng_save_format(CX_SAVE_VERSION, sizeof(CxSaveData));
    return (CxSaveData *)ng_save_data();
}

static void NEOGEO_USER seal(void)
{
    ng_save_commit();
    if (!ng_sys_is_mvs()) (void)ng_card_save(CX_SAVE_TITLE, s_card, sizeof(s_card));
}

void NEOGEO_USER cx_save_load(void)
{
    const CxSaveData *d;
    if (!ng_sys_is_mvs()) (void)ng_card_load(CX_SAVE_VERSION, sizeof(CxSaveData), s_card, sizeof(s_card));
    if (!ng_save_valid(CX_SAVE_VERSION, sizeof(CxSaveData))) return;
    d = (const CxSaveData *)ng_save_data();
    if (d->options.level <= 7u && d->options.var <= CX_VAR_CHALLENGE && d->options.rules <= CX_RULES_MODERN && d->options.classic <= 1u && d->options.length < 5u && d->options.extra <= CX_ET_NONE) {
        cx_options = d->options;
        /* a save from the three-level days: easy, normal and hard become levels 2, 4 and 6 */
        if (cx_options.levels8 != CX_LEVELS8) {
            cx_options.level = (uint8_t)(cx_options.level <= 2u ? 1u + cx_options.level * 2u : 3u);
            cx_options.levels8 = CX_LEVELS8;
        }
    }
}

void NEOGEO_USER cx_save_write(void)
{
    cx_options.levels8 = CX_LEVELS8;
    data()->options = cx_options;
    seal();
}

uint8_t NEOGEO_USER cx_road_saved(void)
{
    const CxSaveData *d;
    if (!ng_save_valid(CX_SAVE_VERSION, sizeof(CxSaveData))) return 0;
    d = (const CxSaveData *)ng_save_data();
    return (uint8_t)(d->road_live == 1u && d->road.completed <= 64u && d->road.human < CX_SQUADS);
}

void NEOGEO_USER cx_road_store(const CxTournament *t)
{
    CxSaveData *d = data();
    /* in assembly: at -O2 a C byte copy between two buffers came out one byte off */
    ng_copy_bytes((volatile uint8_t *)&d->road, (const volatile uint8_t *)t, sizeof(*t));
    d->road_live = 1;
    seal();
}

void NEOGEO_USER cx_road_fetch(CxTournament *t)
{
    ng_copy_bytes((volatile uint8_t *)t, (const volatile uint8_t *)&((const CxSaveData *)ng_save_data())->road, sizeof(*t));
}

void NEOGEO_USER cx_road_finish(uint8_t team, uint8_t won)
{
    CxSaveData *d = data();
    d->road_live = 0;
    if (d->played < 60000u) d->played++;
    if (won) {
        if (d->won < 60000u) d->won++;
        if (team < CX_SQUADS && d->titles[team] < 99u) d->titles[team]++;
    }
    seal();
}

/* Everything kept goes: options, records and a Road to 2030 in progress (OPTIONS, ERASE
 * SAVED DATA). The block is formatted empty and sealed, on the card too. */
void NEOGEO_USER cx_save_erase(void)
{
    ng_save_format(CX_SAVE_VERSION, sizeof(CxSaveData));
    seal();
}
