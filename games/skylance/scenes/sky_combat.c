#include "sky_combat.h"
#ifdef SKY_HOST_TEST
#define NEOGEO_USER
#else
#include "sdk/macro.h"
#endif

int8_t NEOGEO_USER sky_roll_step(SkyRoll *roll, int8_t direction, uint8_t boost)
{
    int16_t target, delta, step;
    if (!direction || direction != roll->direction) roll->held = 0u;
    roll->direction = direction;
    if (direction && roll->held < 120u) roll->held++;
    /* Gentle taps, then a 45-degree sustained bank in about one second. */
    target = (int16_t)(240 + (roll->held > 12u ? (roll->held - 12u) * 12 : 0));
    if (boost && roll->held > 12u) target += 60;
    if (target > 720) target = 720;
    target = (int16_t)(target * direction);
    delta = (int16_t)(target - roll->angle);
    step = (int16_t)(delta / 6);
    if (!step && delta) step = delta > 0 ? 1 : -1;
    if (step > 32) step = 32;
    if (step < -32) step = -32;
    roll->angle = (int16_t)(roll->angle + step);
    return (int8_t)((roll->angle + (roll->angle < 0 ? -120 : 120)) / 240);
}

uint8_t NEOGEO_USER sky_special_damage(uint8_t pilot, uint8_t boss)
{
    static const uint8_t damage[3][2] = {{12u, 6u}, {10u, 6u}, {14u, 7u}};
    return damage[pilot % 3u][boss != 0u];
}

uint8_t NEOGEO_USER sky_special_stage_damage(uint8_t pilot, uint8_t stage, uint8_t boss)
{
    if (stage > 6u) stage = 6u;
    return (uint8_t)(sky_special_damage(pilot, boss) + stage * (boss ? 2u : 3u));
}

uint8_t NEOGEO_USER sky_is_command_target(uint8_t type)
{
    return (uint8_t)(type == 7u || type == 8u);
}

uint16_t NEOGEO_USER sky_hull_damage(uint16_t health, uint16_t damage)
{
    return damage >= health ? 0u : (uint16_t)(health - damage);
}

int16_t NEOGEO_USER sky_flight_axis(int16_t velocity, int8_t direction,
                                   uint8_t pilot, uint8_t boost)
{
    static const int16_t top[3] = {768, 704, 832};
    static const int16_t acceleration[3] = {96, 72, 80};
    int16_t target = (int16_t)(top[pilot % 3u] * direction);
    int16_t delta, step = direction ? acceleration[pilot % 3u] : 112;
    if (boost) target = (int16_t)(target * 3 / 2);
    delta = (int16_t)(target - velocity);
    if (delta > step) delta = step;
    if (delta < -step) delta = (int16_t)-step;
    return (int16_t)(velocity + delta);
}

void NEOGEO_USER sky_resources_init(SkyResources *r)
{
    r->power = r->shields = r->turbo = r->boosting = 0u;
    r->fuel = SKY_FUEL_MAX;
    r->cores = 1u;
    r->chain = r->extends = r->special = r->special_pulses = 0u;
    r->idle_recovery = r->chain_time = 0u;
}

void NEOGEO_USER sky_resources_tick(SkyResources *r, uint8_t boost)
{
    r->boosting = (uint8_t)(boost && r->fuel);
    /* Fuel is the mission countdown, independent of stick/afterburner use. */
    if (++r->idle_recovery >= 60u) {
        r->idle_recovery = 0u;
        if (r->fuel) r->fuel--;
    }
    if (r->chain_time) r->chain_time--;
    else r->chain = 0u;
}

uint8_t NEOGEO_USER sky_resources_collect(SkyResources *r, uint8_t type,
                                         uint8_t *missiles, uint8_t *lives,
                                         uint8_t *energy)
{
    if (type >= SKY_PICK_COUNT) return 0u;
    switch (type) {
    case SKY_PICK_POWER: if (r->power < SKY_POWER_MAX) r->power++; break;
    case SKY_PICK_MISSILE: *missiles = *missiles > 6u ? 9u : *missiles + 3u; break;
    case SKY_PICK_SHIELD: if (r->shields < 2u) r->shields++; break;
    case SKY_PICK_FUEL: r->fuel = r->fuel > SKY_FUEL_MAX - 30u ? SKY_FUEL_MAX : r->fuel + 30u; break;
    case SKY_PICK_TURBO: r->turbo = 1u; break;
    case SKY_PICK_CORE: if (r->cores < 3u) r->cores++; break;
    case SKY_PICK_LIFE: if (*lives < 6u) (*lives)++; break;
    case SKY_PICK_REPAIR: *energy = *energy > 60u ? 100u : *energy + 40u; break;
    default: break;
    }
    return 1u;
}

void NEOGEO_USER sky_resources_death(SkyResources *r)
{
    if (r->power) r->power--;
    r->shields = r->special = r->special_pulses = r->boosting = 0u;
    r->chain = 0u;
}

uint8_t NEOGEO_USER sky_radar_contains(int16_t px, int16_t py, int16_t rx, int16_t ry)
{
    int16_t dx = (int16_t)(px-rx), dy = (int16_t)(py-ry);
    if (dx < -30 || dx > 30 || dy < -30 || dy > 30) return 0u;
    return (uint8_t)(dx*dx + dy*dy <= 900);
}

uint8_t NEOGEO_USER sky_special_begin(SkyResources *r)
{
    if (!r->cores || r->special) return 0u;
    r->cores--;
    r->special = SKY_SPECIAL_FRAMES;
    r->special_pulses = 0u;
    return 1u;
}

uint8_t NEOGEO_USER sky_special_tick(SkyResources *r)
{
    if (!r->special) return 0u;
    r->special--;
    if ((r->special % 15u) == 0u) {
        r->special_pulses++;
        return 1u;
    }
    return 0u;
}

uint8_t NEOGEO_USER sky_weapon_pattern(uint8_t pilot, uint8_t power, SkyShot shots[3])
{
    uint8_t i, count;
    if (power > SKY_POWER_MAX) power = SKY_POWER_MAX;
    pilot %= 3u;
    count = (uint8_t)(power >= 3u ? 3u : power ? 2u : 1u);
    for (i = 0u; i < count; i++) {
        shots[i].x = (int8_t)(count == 1u ? 0 : count == 2u ? (i ? 8 : -8) : (i - 1) * 12);
        shots[i].vx = (int8_t)(pilot == 1u ? (count == 1u ? 0 : count == 2u ? (i ? 1 : -1) : (i - 1) * 2) : 0);
        shots[i].damage = (uint8_t)(1u + (power >= 2u) + (power == 4u) + (pilot == 2u && power >= 2u));
        shots[i].art = (uint8_t)(pilot * 5u + power);
    }
    return count;
}

uint8_t NEOGEO_USER sky_weapon_gap(uint8_t pilot, uint8_t power)
{
    static const uint8_t gaps[3][5] = {{9,9,8,8,7},{11,11,10,10,9},{12,11,11,10,10}};
    return gaps[pilot % 3u][power > 4u ? 4u : power];
}

uint32_t NEOGEO_USER sky_score_add(uint32_t score, uint32_t amount)
{
    if (score >= SKY_SCORE_MAX) return SKY_SCORE_MAX;
    return amount > SKY_SCORE_MAX - score ? SKY_SCORE_MAX : score + amount;
}

uint32_t NEOGEO_USER sky_star_score(SkyResources *r)
{
    if (r->chain < 8u) r->chain++;
    r->chain_time = 240u;
    return 100u * r->chain;
}

void NEOGEO_USER sky_score_extends(SkyResources *r, uint32_t score, uint8_t *lives)
{
    static const uint32_t thresholds[3] = {20000u, 60000u, 120000u};
    while (r->extends < 3u && score >= thresholds[r->extends]) {
        r->extends++;
        if (*lives < 6u) (*lives)++;
    }
}
