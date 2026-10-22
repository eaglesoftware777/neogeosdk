#ifndef NG_CD_H
#define NG_CD_H
#include <stdint.h>
#include "../ng_platform.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    NG_CD_OK=0, NG_CD_INVALID=-1, NG_CD_BOUNDS=-2, NG_CD_BUSY=-3,
    NG_CD_IO=-4, NG_CD_CHECKSUM=-5, NG_CD_STALE=-6, NG_CD_UNSUPPORTED=-95
} NGCDResult;
typedef enum { NG_CD_PRG=0, NG_CD_FIX=1, NG_CD_SPR=2, NG_CD_Z80=3, NG_CD_PCM=4 } NGCDZone;
typedef struct { uint32_t offset, bytes; uint16_t bank; uint8_t zone; } NGCDRange;
/* All public offsets/counts are physical DATA bytes. BIOS bus units stay
 * confined to assembly/descriptor wrappers. PRG policy excludes stock RAM. */
NGCDResult ng_cd_range_validate(const NGCDRange *range);
NGCDResult ng_cd_load_file(const char *name, const NGCDRange *range);
NGCDResult ng_cd_cdda_play(uint8_t track, uint8_t loop);
NGCDResult ng_cd_cdda_pause(void);
NGCDResult ng_cd_cdda_resume(void);
NGCDResult ng_cd_cdda_stop(void);
NGCDResult ng_cd_last_audio_status(void);
extern volatile uint32_t ng_cd_frames;
void ng_cd_start(void);
void ng_cd_irq(void);
void ng_cd_input_sample(uint16_t current);
void ng_cd_wait_vblank(void);
#ifdef __cplusplus
}
#endif
#endif
