#ifndef NG_PLATFORM_H
#define NG_PLATFORM_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum { NG_PLATFORM_MVS_ID=1, NG_PLATFORM_AES_ID=2, NG_PLATFORM_CD_ID=3 } NGPlatformId;
typedef struct {
    uint16_t abi_version;
    uint8_t platform;
    uint8_t has_cd_drive;
    uint8_t has_cdda;
    uint8_t has_battery_save;
    uint8_t has_adpcm_a;
    uint8_t has_adpcm_b;
    uint32_t program_bytes;
    uint32_t sprite_bytes;
    uint32_t fix_bytes;
    uint32_t pcm_bytes;
    uint32_t z80_bytes;
} NGPlatformCaps;
/* Physical capabilities are distinct from which services passed runtime QA. */
const NGPlatformCaps *ng_platform_capabilities(void);
#ifdef __cplusplus
}
#endif
#endif
