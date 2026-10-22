#include "ng_cd.h"
static const NGPlatformCaps caps = {1,NG_PLATFORM_CD_ID,1,1,1,1,0,
    0x200000ul,0x400000ul,0x20000ul,0x100000ul,0x10000ul};
const NGPlatformCaps *ng_platform_capabilities(void) { return &caps; }
