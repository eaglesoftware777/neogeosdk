#ifndef NG_ACTIONS_H
#define NG_ACTIONS_H

#include "ng_defs.h"

struct NGCharacter;

/*
 * NGActionCmd — one step in an animation script.
 *
 * Scripts are const arrays terminated by END() or LOOP().
 * The engine processes one command per frame pass; FRAME() commands consume
 * multiple frames (the `time` argument is the hold duration in ticks).
 *
 * Quick reference:
 *   FRAME(tile, ticks)   — display tileBase `tile` for `ticks` frames
 *   PALETTE(bank)        — switch to palette bank `bank`
 *   FLIP(hflip, vflip)   — set mirror flags (0 or 1)
 *   SCALE(sx, sy)        — set scale (0xFF = full size)
 *   MOVE(dx, dy)         — instant position offset
 *   SPEED(vx, vy)        — set fixed-point velocity (pixels/frame)
 *   WAIT(ticks)          — pause script for `ticks` frames (no tile change)
 *   SFX(id)              — fire ADPCM-A sample via the registered SFX hook
 *   MUSIC(id)            — start music track via the registered music hook
 *   FX(kind, dx, dy)     — call the FX hook at (c->x+dx, c->y+dy)
 *   HITBOX(x,y,w,h)      — enable a hit-stun box relative to the character
 *   CLEAR_HITBOX()       — remove the hit-stun box
 *   EVENT(id,a,b)        — fire a game event with payload (a, b)
 *   GOTO(action_id)      — jump to another registered action script
 *   LOOP()               — restart the current script from the beginning
 *   END()                — stop the script; character holds its last frame
 */
typedef struct {
    uint8_t cmd;
    int16_t a;
    int16_t b;
    int16_t c;
    int16_t d;
} NGActionCmd;

enum {
    ACT_END = 0,
    ACT_LOOP,
    ACT_FRAME,
    ACT_WAIT,
    ACT_MOVE,
    ACT_SPEED,
    ACT_SCALE,
    ACT_PALETTE,
    ACT_FLIP,
    ACT_SFX,
    ACT_MUSIC,
    ACT_FX,
    ACT_HITBOX,
    ACT_CLEAR_HITBOX,
    ACT_EVENT,
    ACT_GOTO
};

#define FRAME(tile,time)      { ACT_FRAME,       (int16_t)(tile), (int16_t)(time), 0, 0 }
#define WAIT(time)            { ACT_WAIT,         (int16_t)(time), 0, 0, 0 }
#define MOVE(dx,dy)           { ACT_MOVE,         (int16_t)(dx),   (int16_t)(dy), 0, 0 }
#define SPEED(vx,vy)          { ACT_SPEED,        (int16_t)(vx),   (int16_t)(vy), 0, 0 }
#define SCALE(sx,sy)          { ACT_SCALE,        (int16_t)(sx),   (int16_t)(sy), 0, 0 }
#define PALETTE(pal)          { ACT_PALETTE,      (int16_t)(pal),  0, 0, 0 }
#define FLIP(x,y)             { ACT_FLIP,         (int16_t)(x),    (int16_t)(y), 0, 0 }
#define SFX(id)               { ACT_SFX,          (int16_t)(id),   0, 0, 0 }
#define MUSIC(id)             { ACT_MUSIC,        (int16_t)(id),   0, 0, 0 }
#define FX(kind,x,y)          { ACT_FX,           (int16_t)(kind), (int16_t)(x), (int16_t)(y), 0 }
#define HITBOX(x,y,w,h)       { ACT_HITBOX,       (int16_t)(x),    (int16_t)(y), (int16_t)(w), (int16_t)(h) }
#define CLEAR_HITBOX()        { ACT_CLEAR_HITBOX, 0, 0, 0, 0 }
#define EVENT(id,a,b)         { ACT_EVENT,        (int16_t)(id),   (int16_t)(a), (int16_t)(b), 0 }
#define GOTO(action)          { ACT_GOTO,         (int16_t)(action), 0, 0, 0 }
#define LOOP()                { ACT_LOOP,         0, 0, 0, 0 }
#define END()                 { ACT_END,          0, 0, 0, 0 }

typedef void(*NGActionSfxHook)(uint16_t id);
typedef void(*NGActionMusicHook)(uint16_t id);
typedef void(*NGActionFxHook)(uint16_t kind, int16_t x, int16_t y);

void NEOGEO_USER ng_actions_init(void);
void NEOGEO_USER ng_actions_register(uint16_t action_id, const NGActionCmd *script);
const NGActionCmd* actions_get(uint16_t action_id);
void NEOGEO_USER ng_actions_set_sound_hooks(NGActionSfxHook sfx_hook, NGActionMusicHook music_hook);
void NEOGEO_USER ng_actions_set_fx_hook(NGActionFxHook fx_hook);
void NEOGEO_USER ng_char_action(struct NGCharacter *c, uint16_t action_id);
void NEOGEO_USER ng_actions_update(struct NGCharacter *c);

#endif
