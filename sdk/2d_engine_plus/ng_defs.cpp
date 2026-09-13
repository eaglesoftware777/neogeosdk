#include "ng_defs.hpp"

extern "C" {

uint8_t NEOGEO_USER ng_rect_hit(NGRect a, NGRect b)
{
    if (static_cast<int16_t>(a.x + a.w) < b.x) return 0;
    if (a.x > static_cast<int16_t>(b.x + b.w)) return 0;
    if (static_cast<int16_t>(a.y + a.h) < b.y) return 0;
    if (a.y > static_cast<int16_t>(b.y + b.h)) return 0;
    return 1;
}

uint16_t NEOGEO_USER ng_abs16(int16_t v)
{
    return (v < 0) ? static_cast<uint16_t>(-v) : static_cast<uint16_t>(v);
}

} /* extern "C" */
