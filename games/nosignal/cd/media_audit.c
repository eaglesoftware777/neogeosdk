/* Separate diagnostic program; never linked into the playable CD. */
#include "sdk/neogeo.h"
#include "games/nosignal/scenes/ns.h"
#include "games/nosignal/cd/platform.h"
#include "nosignal_cd_backgrounds.h"
void ns_cd_setup(void);
volatile uint16_t ns_cd_media_done;
void ng_cd_start(void)
{
    uint8_t i;
    ns_cd_setup();
    for (i=0; i<sizeof(ns_cd_backgrounds)/sizeof(ns_cd_backgrounds[0]); ++i) {
        ns_cd_load_background(ns_cd_backgrounds[i].id);
        ns_frame();
    }
    for (i=0; i<sizeof(ns_cd_heroes)/sizeof(ns_cd_heroes[0]); ++i) {
        ns_cd_load_hero(i);
        ns_frame();
    }
    ns_cd_media_done = 1;
    for (;;) ns_frame();
}
