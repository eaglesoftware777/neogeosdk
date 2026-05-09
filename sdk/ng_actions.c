#include "ng_actions.h"
#include "ng_chars.h"
#include "ng_game_events.h"

static const NGActionCmd *ng_action_table[NG_MAX_ACTIONS];
static NGActionSfxHook ng_action_sfx_hook;
static NGActionMusicHook ng_action_music_hook;
static NGActionFxHook ng_action_fx_hook;

void NEOGEO_USER actions_init(void)
{
    uint16_t i;
    for (i = 0; i < NG_MAX_ACTIONS; i++) ng_action_table[i] = 0;
    ng_action_sfx_hook = 0;
    ng_action_music_hook = 0;
    ng_action_fx_hook = 0;
}

void NEOGEO_USER actions_register(uint16_t action_id, const NGActionCmd *script)
{
    if (action_id >= NG_MAX_ACTIONS) return;
    ng_action_table[action_id] = script;
}

const NGActionCmd* NEOGEO_USER actions_get(uint16_t action_id)
{
    if (action_id >= NG_MAX_ACTIONS) return 0;
    return ng_action_table[action_id];
}

void NEOGEO_USER actions_set_sound_hooks(NGActionSfxHook sfx_hook, NGActionMusicHook music_hook)
{
    ng_action_sfx_hook = sfx_hook;
    ng_action_music_hook = music_hook;
}

void NEOGEO_USER actions_set_fx_hook(NGActionFxHook fx_hook)
{
    ng_action_fx_hook = fx_hook;
}

void NEOGEO_USER char_action(NGCharacter *c, uint16_t action_id)
{
    if (!c) return;
    if (c->action == action_id) return;
    c->action = action_id;
    c->action_pos = 0;
    c->action_timer = 0;
    c->hit_w = 0;
    c->hit_h = 0;
}

void NEOGEO_USER actions_update(NGCharacter *c)
{
    const NGActionCmd *script;
    uint8_t safety;

    if (!c) return;
    if (c->action_timer > 0) {
        c->action_timer--;
        return;
    }

    script = actions_get(c->action);
    if (!script) return;

    safety = 16;
    while (safety--) {
        NGActionCmd cmd = script[c->action_pos++];

        switch (cmd.cmd) {
            case ACT_END:
                c->action_pos = 0;
                return;

            case ACT_LOOP:
                c->action_pos = 0;
                break;

            case ACT_FRAME:
                c->sprite_tile = (uint16_t)cmd.a;
                c->action_timer = (uint16_t)cmd.b;
                c->sprite_dirty = 1;
                return;

            case ACT_WAIT:
                c->action_timer = (uint16_t)cmd.a;
                return;

            case ACT_MOVE:
                c->x_fp += NG_TO_FP(cmd.a);
                c->y_fp += NG_TO_FP(cmd.b);
                c->x = NG_FROM_FP(c->x_fp);
                c->y = NG_FROM_FP(c->y_fp);
                break;

            case ACT_SPEED:
                c->vx_fp = NG_TO_FP(cmd.a);
                c->vy_fp = NG_TO_FP(cmd.b);
                break;

            case ACT_SCALE:
                c->scale_x = (uint8_t)cmd.a;
                c->scale_y = (uint8_t)cmd.b;
                break;

            case ACT_PALETTE:
                c->palette = (uint8_t)cmd.a;
                c->sprite_dirty = 1;
                break;

            case ACT_FLIP:
                c->flip_x = (uint8_t)cmd.a;
                c->flip_y = (uint8_t)cmd.b;
                c->sprite_dirty = 1;
                break;

            case ACT_SFX:
                if (ng_action_sfx_hook) ng_action_sfx_hook((uint16_t)cmd.a);
                break;

            case ACT_MUSIC:
                if (ng_action_music_hook) ng_action_music_hook((uint16_t)cmd.a);
                break;

            case ACT_FX:
                if (ng_action_fx_hook) ng_action_fx_hook((uint16_t)cmd.a, (int16_t)(c->x + cmd.b), (int16_t)(c->y + cmd.c));
                break;

            case ACT_HITBOX:
                c->hit_x = cmd.a;
                c->hit_y = cmd.b;
                c->hit_w = cmd.c;
                c->hit_h = cmd.d;
                break;

            case ACT_CLEAR_HITBOX:
                c->hit_w = 0;
                c->hit_h = 0;
                break;

            case ACT_EVENT:
                game_events_send((uint16_t)cmd.a, (uint16_t)cmd.b, (uint16_t)cmd.c, 0);
                break;

            case ACT_GOTO:
                char_action(c, (uint16_t)cmd.a);
                break;

            default:
                return;
        }
    }
}
