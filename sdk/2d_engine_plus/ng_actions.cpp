#include "ng_actions.hpp"
#include "ng_chars.hpp"
#include "ng_game_events.hpp"

/* --- ActionRegistry singleton --- */

ActionRegistry& ActionRegistry::instance()
{
    static ActionRegistry ar;
    return ar;
}

/* --- ActionRegistry public methods --- */

void ActionRegistry::init()
{
    uint16_t i;
    for (i = 0; i < NG_MAX_ACTIONS; i++) table[i] = nullptr;
    sfx_hook   = nullptr;
    music_hook = nullptr;
    fx_hook    = nullptr;
}

void ActionRegistry::registerScript(uint16_t action_id, const NGActionCmd *script)
{
    if (action_id < NG_MAX_ACTIONS) table[action_id] = script;
}

const NGActionCmd* ActionRegistry::get(uint16_t action_id) const
{
    if (action_id >= NG_MAX_ACTIONS) return 0;
    return table[action_id];
}

void ActionRegistry::setSoundHooks(NGActionSfxHook sfx, NGActionMusicHook music)
{
    sfx_hook   = sfx;
    music_hook = music;
}

void ActionRegistry::setFxHook(NGActionFxHook fx)
{
    fx_hook = fx;
}

void ActionRegistry::update(NGCharacter *c)
{
    const NGActionCmd *script;
    uint8_t safety;

    if (!c) return;

    if (c->action_timer > 0) { c->action_timer--; return; }

    script = get(c->action);
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
            if (c->sprite_tile != (uint16_t)cmd.a) {
                c->sprite_tile  = (uint16_t)cmd.a;
                c->sprite_dirty = 1;
            }
            c->action_timer = (uint16_t)cmd.b;
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
            if (c->palette != (uint8_t)cmd.a) {
                c->palette      = (uint8_t)cmd.a;
                c->sprite_dirty = 1;
            }
            break;

        case ACT_FLIP:
            if (c->flip_x != (uint8_t)cmd.a || c->flip_y != (uint8_t)cmd.b) {
                c->flip_x = (uint8_t)cmd.a;
                c->flip_y = (uint8_t)cmd.b;
                c->sprite_dirty = 1;
            }
            break;

        case ACT_SFX:
            if (sfx_hook) sfx_hook((uint16_t)cmd.a);
            break;

        case ACT_MUSIC:
            if (music_hook) music_hook((uint16_t)cmd.a);
            break;

        case ACT_FX:
            if (fx_hook) fx_hook((uint16_t)cmd.a, (int16_t)(c->x + cmd.b), (int16_t)(c->y + cmd.c));
            break;

        case ACT_HITBOX:
            c->hit_x = cmd.a; c->hit_y = cmd.b;
            c->hit_w = cmd.c; c->hit_h = cmd.d;
            break;

        case ACT_CLEAR_HITBOX:
            c->hit_w = 0; c->hit_h = 0;
            break;

        case ACT_EVENT:
            ng_game_events_send((uint16_t)cmd.a, (uint16_t)cmd.b, (uint16_t)cmd.c, 0);
            break;

        case ACT_GOTO:
            ng_char_action(c, (uint16_t)cmd.a);
            break;

        default:
            return;
        }
    }
}

/* --- extern "C" wrappers --- */

extern "C" {

void NEOGEO_USER ng_actions_init(void)
{
    ActionRegistry::instance().init();
}

void NEOGEO_USER ng_actions_register(uint16_t action_id, const NGActionCmd *script)
{
    ActionRegistry::instance().registerScript(action_id, script);
}

const NGActionCmd* actions_get(uint16_t action_id)
{
    return ActionRegistry::instance().get(action_id);
}

void NEOGEO_USER ng_actions_set_sound_hooks(NGActionSfxHook sfx_hook, NGActionMusicHook music_hook)
{
    ActionRegistry::instance().setSoundHooks(sfx_hook, music_hook);
}

void NEOGEO_USER ng_actions_set_fx_hook(NGActionFxHook fx_hook)
{
    ActionRegistry::instance().setFxHook(fx_hook);
}

void NEOGEO_USER ng_char_action(NGCharacter *c, uint16_t action_id)
{
    if (!c) return;
    if (c->action == action_id) return;
    c->action       = action_id;
    c->action_pos   = 0;
    c->action_timer = 0;
    c->hit_w        = 0;
    c->hit_h        = 0;
}

void NEOGEO_USER ng_actions_update(NGCharacter *c)
{
    ActionRegistry::instance().update(c);
}

} /* extern "C" */
