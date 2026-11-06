#include "ng_cd.h"

NGCDResult ng_cd_range_validate(const NGCDRange *r)
{
    uint32_t size, alignment;
    uint16_t banks;
    if (!r || r->zone > NG_CD_PCM) return NG_CD_INVALID;
    switch(r->zone) {
    case NG_CD_PRG: size=0x100000ul;alignment=2;banks=1;break;
    case NG_CD_FIX: size=0x20000ul;alignment=32;banks=1;break;
    case NG_CD_SPR: size=0x100000ul;alignment=128;banks=4;break;
    case NG_CD_Z80: size=0x10000ul;alignment=2;banks=1;break;
    default: size=0x80000ul;alignment=256;banks=2;break;
    }
    if (r->bank>=banks || !r->bytes || r->offset>size || r->bytes>size-r->offset ||
        (r->offset & (alignment-1u)) || (r->bytes & (alignment-1u))) return NG_CD_BOUNDS;
    return NG_CD_OK;
}
