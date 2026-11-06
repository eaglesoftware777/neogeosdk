#ifndef SKY_COMBAT_H
#define SKY_COMBAT_H
#include <stdint.h>

enum {
    SKY_PICK_POWER, SKY_PICK_MISSILE, SKY_PICK_SHIELD, SKY_PICK_FUEL,
    SKY_PICK_TURBO, SKY_PICK_CORE, SKY_PICK_LIFE, SKY_PICK_STAR,
    SKY_PICK_REPAIR, SKY_PICK_COUNT
};
#define SKY_POWER_MAX 4u
#define SKY_FUEL_MAX 240u
#define SKY_SCORE_MAX 999999u
#define SKY_SPECIAL_FRAMES 90u

typedef struct {
    uint8_t power, fuel, shields, cores, turbo, boosting;
    uint8_t chain, extends, special, special_pulses;
    uint16_t idle_recovery, chain_time;
} SkyResources;

typedef struct {
    int8_t x, vx;
    uint8_t damage, art;
} SkyShot;

typedef struct {
    int16_t angle;
    uint8_t held;
    int8_t direction;
} SkyRoll;

int8_t sky_roll_step(SkyRoll *roll, int8_t direction, uint8_t boost);
uint8_t sky_special_damage(uint8_t pilot, uint8_t boss);
uint8_t sky_special_stage_damage(uint8_t pilot, uint8_t stage, uint8_t boss);
uint8_t sky_is_command_target(uint8_t type);
uint16_t sky_hull_damage(uint16_t health, uint16_t damage);
int16_t sky_flight_axis(int16_t velocity, int8_t direction, uint8_t pilot, uint8_t boost);
uint8_t sky_radar_contains(int16_t px, int16_t py, int16_t rx, int16_t ry);

void sky_resources_init(SkyResources *r);
void sky_resources_tick(SkyResources *r, uint8_t boost);
uint8_t sky_resources_collect(SkyResources *r, uint8_t type,
                              uint8_t *missiles, uint8_t *lives, uint8_t *energy);
void sky_resources_death(SkyResources *r);
uint8_t sky_special_begin(SkyResources *r);
uint8_t sky_special_tick(SkyResources *r);
uint8_t sky_weapon_pattern(uint8_t pilot, uint8_t power, SkyShot shots[3]);
uint8_t sky_weapon_gap(uint8_t pilot, uint8_t power);
uint32_t sky_score_add(uint32_t score, uint32_t amount);
uint32_t sky_star_score(SkyResources *r);
void sky_score_extends(SkyResources *r, uint32_t score, uint8_t *lives);
#endif
