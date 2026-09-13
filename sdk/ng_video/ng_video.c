#include "ng_video/ng_video.h"
#include "ng_fix/ng_fix.h"
#include "neogeo.h"

static NGVideoRange ngvideo_range;
static NGVideoOwner ngvideo_owner_map[NGVIDEO_SPRITE_TOTAL];

static uint16_t NEOGEO_USER ngvideo_clamp_count(uint16_t first, uint16_t count)
{
    if (first >= NGVIDEO_SPRITE_TOTAL) return 0;
    if ((uint32_t)first + (uint32_t)count > NGVIDEO_SPRITE_TOTAL)
        return (uint16_t)(NGVIDEO_SPRITE_TOTAL - first);
    return count;
}

void NEOGEO_USER ngvideo_init(void)
{
    uint16_t i;

    ngfix_init();
    ngvideo_range.first = 0;
    ngvideo_range.count = 0;
    ngvideo_range.owner = NGVIDEO_OWNER_NONE;

    for (i = 0; i < NGVIDEO_SPRITE_TOTAL; i++) {
        ngvideo_owner_map[i] = NGVIDEO_OWNER_NONE;
    }
}

void NEOGEO_USER ngvideo_register_owner(uint16_t first, uint16_t count, NGVideoOwner owner)
{
    uint16_t i;
    count = ngvideo_clamp_count(first, count);

    for (i = 0; i < count; i++) {
        ngvideo_owner_map[first + i] = owner;
    }

    ngvideo_range.first = first;
    ngvideo_range.count = count;
    ngvideo_range.owner = owner;
}

NGVideoOwner NEOGEO_USER ngvideo_owner_for(uint16_t sprite_index)
{
    if (sprite_index >= NGVIDEO_SPRITE_TOTAL) return NGVIDEO_OWNER_NONE;
    return ngvideo_owner_map[sprite_index];
}

const NGVideoRange* NEOGEO_USER ngvideo_current_range(void)
{
    return &ngvideo_range;
}

void NEOGEO_USER ngvideo_clear_sprites(uint16_t first, uint16_t count)
{
    uint16_t i;

    count = ngvideo_clamp_count(first, count);
    for (i = 0; i < count; i++) {
        vram_SCB234((uint16_t)(SCB3_ADDR + first + i), 0);
        if ((first + i) < NGVIDEO_SPRITE_TOTAL)
            ngvideo_owner_map[first + i] = NGVIDEO_OWNER_NONE;
    }
}

void NEOGEO_USER ngvideo_clear_fix(void)
{
    clearFix();
    ngfix_cache_invalidate();
}

void NEOGEO_USER ngvideo_set_backdrop(uint16_t color)
{
    setBACKDROP(color);
}

void NEOGEO_USER ngvideo_hard_clear(void)
{
    soundStopAll();
    ngvideo_clear_fix();
    clearSprs();
    ngvideo_clear_sprites(0, NGVIDEO_SPRITE_TOTAL);
    ngvideo_set_backdrop(BLACK);
    waitVbl();
    ngvideo_clear_sprites(0, NGVIDEO_SPRITE_TOTAL);
    waitVbl();
}

void NEOGEO_USER ngvideo_caption_footer(const char *text)
{
    ngfix_clear_line(27);
    if (text) ngfix_puts(25, 27, text, 1);
}

void NEOGEO_USER ngvideo_caption(const char *title, const char *line1, const char *line2)
{
    ngvideo_clear_fix();
    if (title) ngfix_puts(2, 1, title, 0);
    if (line1) ngfix_puts(2, 3, line1, 1);
    if (line2) ngfix_puts(2, 5, line2, 2);
    ngvideo_caption_footer("A: NEXT");
}

void NEOGEO_USER ngvideo_begin_scene(const NGVideoSceneDesc *desc)
{
    uint16_t flags = NGVIDEO_FLAG_DEFAULT;
    uint16_t first = 0;
    uint16_t count = NGVIDEO_SPRITE_TOTAL;
    NGVideoOwner owner = NGVIDEO_OWNER_USER;

    if (desc) {
        flags = desc->flags;
        first = desc->sprite_first;
        count = desc->sprite_count ? desc->sprite_count : NGVIDEO_SPRITE_TOTAL;
        owner = desc->owner;
    }

    if (flags & NGVIDEO_FLAG_STOP_SOUND) soundStopAll();
    if (flags & NGVIDEO_FLAG_CLEAR_FIX) ngvideo_clear_fix();
    if (flags & NGVIDEO_FLAG_CLEAR_SPRITES) {
        clearSprs();
        ngvideo_clear_sprites(first, count);
    }
    if (flags & NGVIDEO_FLAG_BLACK_BACKDROP) ngvideo_set_backdrop(BLACK);

    ngvideo_register_owner(first, count, owner);

    waitVbl();
    if (desc) ngvideo_caption(desc->title, desc->line1, desc->line2);
}

void NEOGEO_USER ngvideo_end_scene(uint16_t clear_sprites)
{
    if (clear_sprites) {
        ngvideo_clear_sprites(ngvideo_range.first, ngvideo_range.count);
    }
    ngvideo_clear_fix();
    ngvideo_set_backdrop(BLACK);
    waitVbl();
}

uint8_t NEOGEO_USER ngvideo_advance_requested(void)
{
    uint8_t p1_change = NEO_REGISTER8(BIOS_P1CHANGE);

    /*
     * Demo scene advance is deliberately tied only to the A-button edge.
     * Do not use BIOS_START_FLAG here: after START_GAME the BIOS start flag
     * can remain asserted, which makes all timed scenes immediately skip and
     * look like a fast rewind.
     */
    if (p1_change & (uint8_t)(1u << CNT_A)) return 1;
    return 0;
}

uint8_t NEOGEO_USER ngvideo_wait_frames(uint16_t frames)
{
    uint16_t i;

    for (i = 0; i < frames; i++) {
        waitVbl();
        if (ngvideo_advance_requested()) {
            waitVbl();
            return 1;
        }
    }
    return 0;
}
