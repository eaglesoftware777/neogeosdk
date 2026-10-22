#include "ng_cd.h"
#include "macro.h"
#include "neogeo.h"
extern int32_t ng_cd_bios_cdda(uint32_t command);
static NGCDResult last_audio = NG_CD_UNSUPPORTED;

NGCDResult ng_cd_last_audio_status(void) { return last_audio; }
NGCDResult ng_cd_cdda_play(uint8_t track, uint8_t loop)
{
    if (track<2 || track>99 || loop>1) return NG_CD_INVALID;
#ifdef NG_CD_ENABLE_EXPERIMENTAL_BIOS_IO
    (void)ng_cd_bios_cdda((uint32_t)((loop?0u:1u)<<8) | ((track/10u)<<4) | (track%10u));
#endif
    return NG_CD_UNSUPPORTED;
}
NGCDResult ng_cd_cdda_pause(void) { return NG_CD_UNSUPPORTED; }
NGCDResult ng_cd_cdda_resume(void) { return NG_CD_UNSUPPORTED; }
NGCDResult ng_cd_cdda_stop(void) { return NG_CD_UNSUPPORTED; }

/* The opt-in CD fixture installs its OWN RAM driver through IPL. These
 * events are not commands to an assumed cartridge M1 or an arbitrary BIOS. */
void NEOGEO_USER soundInit(void)
{
    *(volatile uint8_t *)0xFF0183ul = 0;
    *(volatile uint8_t *)0xFF0183ul = 0xFF;
    last_audio=NG_CD_OK;
}
void NEOGEO_USER playSFX(uint8_t sample)
{
    if (sample!=0) { last_audio=NG_CD_UNSUPPORTED;return; }
    *(volatile uint8_t *)0x320000ul=0x40;
    last_audio=NG_CD_OK;
}
