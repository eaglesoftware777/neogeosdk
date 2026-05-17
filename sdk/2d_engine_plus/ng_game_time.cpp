#include "ng_game_time.hpp"
#include "ng_properties.hpp"

extern "C" {


static uint32_t ng_time_frame;
static uint32_t ng_time_stage_frame;
static uint16_t ng_time_second;
static uint8_t ng_time_second_tick;

void NEOGEO_USER ng_game_time_init(void)
{
    ng_time_frame = 0;
    ng_time_stage_frame = 0;
    ng_time_second = 0;
    ng_time_second_tick = 0;
    ng_prop_set(NG_PROP_GROUP_TIME, NG_PROP_TIME_FRAME, 0);
    ng_prop_set(NG_PROP_GROUP_TIME, NG_PROP_TIME_SECOND, 0);
    ng_prop_set(NG_PROP_GROUP_TIME, NG_PROP_TIME_STAGE_FRAME, 0);
}

void NEOGEO_USER ng_game_time_tick(void)
{
    ng_time_frame++;
    ng_time_stage_frame++;
    ng_time_second_tick++;
    if (ng_time_second_tick >= NG_FRAME_RATE) {
        ng_time_second_tick = 0;
        ng_time_second++;
    }

    ng_prop_set(NG_PROP_GROUP_TIME, NG_PROP_TIME_FRAME, (int32_t)ng_time_frame);
    ng_prop_set(NG_PROP_GROUP_TIME, NG_PROP_TIME_SECOND, (int32_t)ng_time_second);
    ng_prop_set(NG_PROP_GROUP_TIME, NG_PROP_TIME_STAGE_FRAME, (int32_t)ng_time_stage_frame);
}

void NEOGEO_USER ng_game_time_reset_stage(void)
{
    ng_time_stage_frame = 0;
    ng_time_second_tick = 0;
    ng_prop_set(NG_PROP_GROUP_TIME, NG_PROP_TIME_STAGE_FRAME, 0);
}

uint32_t NEOGEO_USER ng_game_time_frame(void) { return ng_time_frame; }
uint16_t NEOGEO_USER ng_game_time_second(void) { return ng_time_second; }
uint32_t NEOGEO_USER ng_game_time_stage_frame(void) { return ng_time_stage_frame; }


} /* extern "C" */
