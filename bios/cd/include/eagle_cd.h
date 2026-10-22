#ifndef EAGLE_CD_H
#define EAGLE_CD_H
#include <stdint.h>
#define ECD_VERSION_MAJOR 1u
#define ECD_VERSION_MINOR 7u
#define ECD_VERSION_PATCH 7u
#define ECD_ROM_BASE 0xC00000ul
#define ECD_ROM_BYTES 0x80000ul
/* This is private to the ORIGINAL firmware, not a stock BIOS RAM map. */
#define ECD_PRIVATE_BEGIN 0x1F0000ul
#define ECD_PRIVATE_END 0x1F4000ul
#define ECD_STACK_TOP 0x1FF000ul
#define ECD_ERR_UNSUPPORTED (-95)
#define ECD_REG8(a) (*(volatile uint8_t *)(uintptr_t)(a))
#define ECD_REG16(a) (*(volatile uint16_t *)(uintptr_t)(a))
extern volatile uint32_t eagle_cd_frames;
extern volatile uint16_t eagle_cd_pad_seen;
void eagle_cd_boot(void);
void eagle_cd_io(void);
void eagle_cd_video_init(void);
void eagle_cd_text(uint8_t x, uint8_t y, const char *text);
void eagle_cd_vblank(void);
void eagle_cd_wait(void);
void eagle_cd_reset(void);
extern const uint8_t eagle_cd_font[];
#endif
