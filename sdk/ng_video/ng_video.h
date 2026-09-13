#ifndef SDK_NG_VIDEO_NG_VIDEO_H
#define SDK_NG_VIDEO_NG_VIDEO_H

#include <stdint.h>
#include "macro.h"

#ifndef NEOGEO_USER
#define NEOGEO_USER
#endif

/*
 * NeoUniversal video coordinator.
 *
 * This layer owns scene-level video hygiene: FIX captions, backdrop color,
 * sprite-range clearing, generated-screen isolation and transition state.
 * It does not replace the low-level Neo Geo VRAM helpers.  It gives higher
 * layers a single place to begin/end visual scenes so showScreen(), raycaster,
 * 2D engine actors, FIX HUD and eyecatcher code do not fight for the same
 * hardware state.
 */

#define NGVIDEO_SPRITE_TOTAL       384u
#define NGVIDEO_SPRITE_SAFE_LAST   383u

#define NGVIDEO_RANGE_ALL_FIRST    0u
#define NGVIDEO_RANGE_ALL_COUNT    384u
#define NGVIDEO_RANGE_GAME_FIRST   0u
#define NGVIDEO_RANGE_GAME_COUNT   300u
#define NGVIDEO_RANGE_BG_FIRST     300u
#define NGVIDEO_RANGE_BG_COUNT     32u
#define NGVIDEO_RANGE_SHOW_FIRST   0u
#define NGVIDEO_RANGE_SHOW_COUNT   128u
#define NGVIDEO_RANGE_FX_FIRST     0u
#define NGVIDEO_RANGE_FX_COUNT     160u
#define NGVIDEO_RANGE_RAY_FIRST    0u
#define NGVIDEO_RANGE_RAY_COUNT    64u

#define NGVIDEO_FLAG_STOP_SOUND    0x0001u
#define NGVIDEO_FLAG_CLEAR_FIX     0x0002u
#define NGVIDEO_FLAG_CLEAR_SPRITES 0x0004u
#define NGVIDEO_FLAG_BLACK_BACKDROP 0x0008u
#define NGVIDEO_FLAG_DEFAULT       (NGVIDEO_FLAG_CLEAR_FIX | NGVIDEO_FLAG_CLEAR_SPRITES | NGVIDEO_FLAG_BLACK_BACKDROP)
#define NGVIDEO_FLAG_FULL_RESET    (NGVIDEO_FLAG_STOP_SOUND | NGVIDEO_FLAG_DEFAULT)

typedef enum {
    NGVIDEO_OWNER_NONE = 0,
    NGVIDEO_OWNER_SHOWSCREEN,
    NGVIDEO_OWNER_2D_ENGINE,
    NGVIDEO_OWNER_RAYCASTER,
    NGVIDEO_OWNER_FLOORFX,
    NGVIDEO_OWNER_EYECATCHER,
    NGVIDEO_OWNER_USER
} NGVideoOwner;

typedef struct {
    uint16_t first;
    uint16_t count;
    NGVideoOwner owner;
} NGVideoRange;

typedef struct {
    const char *title;
    const char *line1;
    const char *line2;
    uint16_t flags;
    uint16_t sprite_first;
    uint16_t sprite_count;
    NGVideoOwner owner;
} NGVideoSceneDesc;

void NEOGEO_USER ngvideo_init(void);
void NEOGEO_USER ngvideo_hard_clear(void);
void NEOGEO_USER ngvideo_clear_sprites(uint16_t first, uint16_t count);
void NEOGEO_USER ngvideo_clear_fix(void);
void NEOGEO_USER ngvideo_set_backdrop(uint16_t color);
void NEOGEO_USER ngvideo_begin_scene(const NGVideoSceneDesc *desc);
void NEOGEO_USER ngvideo_end_scene(uint16_t clear_sprites);
void NEOGEO_USER ngvideo_caption(const char *title, const char *line1, const char *line2);
void NEOGEO_USER ngvideo_caption_footer(const char *text);
uint8_t NEOGEO_USER ngvideo_advance_requested(void);
uint8_t NEOGEO_USER ngvideo_wait_frames(uint16_t frames);
void NEOGEO_USER ngvideo_register_owner(uint16_t first, uint16_t count, NGVideoOwner owner);
NGVideoOwner NEOGEO_USER ngvideo_owner_for(uint16_t sprite_index);
const NGVideoRange* NEOGEO_USER ngvideo_current_range(void);

#endif
