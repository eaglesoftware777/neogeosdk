#include "ng_cd.h"
#include <stddef.h>
extern int32_t ng_cd_bios_load(const uint8_t *descriptor);

NGCDResult ng_cd_load_file(const char *name, const NGCDRange *r)
{
    uint8_t descriptor[20];
    uint8_t n=0,dot=0,base=0,ext=0;
    uint32_t address;
    NGCDResult result=ng_cd_range_validate(r);
    /* This runtime does not implement executable overlays. Reject PRG
     * rather than permitting LOADFILE to overwrite the live resident code. */
    if (result!=NG_CD_OK) return result;
    if (!name || r->zone==NG_CD_PRG) return NG_CD_UNSUPPORTED;
    while(name[n]) {
        char c=name[n];
        if (n>=12) return NG_CD_INVALID;
        if (c=='.' && !dot && base) dot=1;
        else {
            if (!((c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_')) return NG_CD_INVALID;
            if (dot) { if (++ext>3) return NG_CD_INVALID; }
            else if (++base>8) return NG_CD_INVALID;
        }
        descriptor[n++]=(uint8_t)c;
    }
    if (!dot || !ext) return NG_CD_INVALID;
    {
        static const char extensions[5][4] = {"PRG","FIX","SPR","Z80","PCM"};
        uint8_t i;
        if (ext != 3) return NG_CD_INVALID;
        for (i=0;i<3;i++) if (name[n-3+i]!=extensions[r->zone][i]) return NG_CD_INVALID;
    }
    descriptor[n++]=0;
    descriptor[n++]=(uint8_t)r->bank;
    if (n&1u) descriptor[n++]=0;
    address=r->offset;
    if (r->zone==NG_CD_FIX || r->zone==NG_CD_Z80 || r->zone==NG_CD_PCM) address<<=1;
    descriptor[n++]=(uint8_t)(address>>24);
    descriptor[n++]=(uint8_t)(address>>16);
    descriptor[n++]=(uint8_t)(address>>8);
    descriptor[n++]=(uint8_t)address;
    descriptor[n]=0;
    /* Size is not passed by this BIOS interface: a validated manifest and
     * trusted source-built disc are required. No success is inferred from
     * undocumented BIOS return flags at this unverified backend gate. */
#ifdef NG_CD_ENABLE_EXPERIMENTAL_BIOS_IO
    (void)ng_cd_bios_load(descriptor);
#else
    (void)descriptor;
#endif
    return NG_CD_UNSUPPORTED;
}
