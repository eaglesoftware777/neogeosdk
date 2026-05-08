#ifndef NG_ACTIONS_H
#define NG_ACTIONS_H

#include "ng_defs.h"

struct NGCharacter;

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

#define FRAME(tile,time)      { ACT_FRAME, tile, time, 0, 0 }
#define WAIT(time)            { ACT_WAIT, time, 0, 0, 0 }
#define MOVE(dx,dy)           { ACT_MOVE, dx, dy, 0, 0 }
#define SPEED(vx,vy)          { ACT_SPEED, vx, vy, 0, 0 }
#define SCALE(sx,sy)          { ACT_SCALE, sx, sy, 0, 0 }
#define PALETTE(pal)          { ACT_PALETTE, pal, 0, 0, 0 }
#define FLIP(x,y)             { ACT_FLIP, x, y, 0, 0 }
#define SFX(id)               { ACT_SFX, id, 0, 0, 0 }
#define MUSIC(id)             { ACT_MUSIC, id, 0, 0, 0 }
#define FX(kind,x,y)          { ACT_FX, kind, x, y, 0 }
#define HITBOX(x,y,w,h)       { ACT_HITBOX, x, y, w, h }
#define CLEAR_HITBOX()        { ACT_CLEAR_HITBOX, 0, 0, 0, 0 }
#define EVENT(id,a,b)         { ACT_EVENT, id, a, b, 0 }
#define GOTO(action)          { ACT_GOTO, action, 0, 0, 0 }
#define LOOP()                { ACT_LOOP, 0, 0, 0, 0 }
#define END()                 { ACT_END, 0, 0, 0, 0 }

typedef void (*NGActionSfxHook)(uint16_t id);
typedef void (*NGActionMusicHook)(uint16_t id);
typedef void (*NGActionFxHook)(uint16_t kind, int16_t x, int16_t y);

void actions_init(void);
void actions_register(uint16_t action_id, const NGActionCmd *script);
const NGActionCmd* actions_get(uint16_t action_id);
void actions_set_sound_hooks(NGActionSfxHook sfx_hook, NGActionMusicHook music_hook);
void actions_set_fx_hook(NGActionFxHook fx_hook);
void char_action(struct NGCharacter *c, uint16_t action_id);
void actions_update(struct NGCharacter *c);

#endif
