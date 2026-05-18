/*
 * ng_palette_fx.hpp — Palette effects system.
 *
 * All writes go through ng_rq_palette_upload() → posted at VBlank.
 * NeoGeo colour: bit15=dark, [14:10]=R, [9:5]=G, [4:0]=B.
 * Index 0 is always transparent.
 */

#ifndef NG_PALETTE_FX_HPP
#define NG_PALETTE_FX_HPP

#include "ng_defs.hpp"
#include "ng_render_queue.hpp"

#define NG_PALFX_MAX_SLOTS   8

#define NG_PALFX_NONE        0
#define NG_PALFX_FADE_IN     1
#define NG_PALFX_FADE_OUT    2
#define NG_PALFX_FLASH_WHITE 3
#define NG_PALFX_FLASH_RED   4
#define NG_PALFX_FLASH_BLUE  5
#define NG_PALFX_PULSE       6
#define NG_PALFX_CYCLE       7

struct NGPalFxSlot {
    uint8_t         active;
    uint8_t         palette_slot;
    uint8_t         fx_type;
    uint8_t         timer;
    uint8_t         duration;
    uint8_t         cycle_start;
    uint8_t         cycle_end;
    uint8_t         cycle_pos;
    const uint16_t *base_pal;
    uint16_t        work_pal[16];
};

struct NGPaletteAsset {
    uint16_t asset_id;
    uint8_t  palette_slot;
    uint16_t colors[16];
};

/*
 * PaletteFxSystem — singleton owning the FX slot pool.
 */
class PaletteFxSystem {
public:
    static PaletteFxSystem& instance();

    void NEOGEO_USER init();
    void NEOGEO_USER update();

    void NEOGEO_USER uploadBase(uint8_t palette_slot, const uint16_t *pal);
    void NEOGEO_USER fadeIn(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration);
    void NEOGEO_USER fadeOut(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration);
    void NEOGEO_USER flashWhite(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration);
    void NEOGEO_USER flashRed(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration);
    void NEOGEO_USER flashBlue(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration);
    void NEOGEO_USER pulse(uint8_t palette_slot, const uint16_t *base_pal, uint8_t period);
    void NEOGEO_USER cycle(uint8_t palette_slot, const uint16_t *base_pal, uint8_t start, uint8_t end);
    void NEOGEO_USER stop(uint8_t palette_slot);
    uint8_t NEOGEO_USER active(uint8_t palette_slot) const;

private:
    PaletteFxSystem() {}

    NGPalFxSlot slots[NG_PALFX_MAX_SLOTS];

    NGPalFxSlot* findOrAlloc(uint8_t palette_slot);

    static uint8_t  scaleChannel(uint8_t v, uint8_t factor);
    static void     blendTo(uint16_t *out, const uint16_t *base,
                            uint8_t tr, uint8_t tg, uint8_t tb, uint8_t blend);
    static void     scalePal(uint16_t *out, const uint16_t *base, uint8_t brightness);
};

#ifdef __cplusplus
extern "C" {
#endif

void    NEOGEO_USER ng_palette_fx_init(void);
void    NEOGEO_USER ng_palette_fx_update(void);
void    NEOGEO_USER ng_palette_load_bank(uint8_t palette_slot, const uint16_t *pal);
uint8_t NEOGEO_USER ng_palette_load_asset(const NGPaletteAsset *assets,
                                          uint16_t count,
                                          uint16_t asset_id);
void    NEOGEO_USER ng_palfx_upload_base(uint8_t palette_slot, const uint16_t *pal);
void    NEOGEO_USER ng_palfx_fade_in(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration);
void    NEOGEO_USER ng_palfx_fade_out(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration);
void    NEOGEO_USER ng_palfx_flash_white(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration);
void    NEOGEO_USER ng_palfx_flash_red(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration);
void    NEOGEO_USER ng_palfx_flash_blue(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration);
void    NEOGEO_USER ng_palfx_pulse(uint8_t palette_slot, const uint16_t *base_pal, uint8_t period);
void    NEOGEO_USER ng_palfx_cycle(uint8_t palette_slot, const uint16_t *base_pal, uint8_t start, uint8_t end);
void    NEOGEO_USER ng_palfx_stop(uint8_t palette_slot);
uint8_t NEOGEO_USER ng_palfx_active(uint8_t palette_slot);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
