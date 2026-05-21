/*******
https://eaglesoftware.biz
https://github.com/eaglesoftware777
https://github.com/eaglesoftware777/neogeosdk
******/

#include "sdk/macro.h"
#include "sdk/neogeo.h"
#include <stdint.h>

#include "sdk/2d_engine/ng_bg.h"

/* No artbox screens — helloworld uses FIX text only */
const NGShowScreenFn ng_screen_table[NG_SCREEN_TABLE_MAX] = { 0 };
const uint16_t ng_screen_count = 0;
