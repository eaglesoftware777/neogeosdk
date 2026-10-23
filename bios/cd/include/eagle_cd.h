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
#ifdef ECD_HOST_TEST
/* Test-only bus boundary: production builds always use native volatile I/O. */
uint8_t *eagle_cd_host_reg8(uintptr_t address);
#define ECD_REG8(a) (*eagle_cd_host_reg8((uintptr_t)(a)))
#else
#define ECD_REG8(a) (*(volatile uint8_t *)(uintptr_t)(a))
#endif
#define ECD_REG16(a) (*(volatile uint16_t *)(uintptr_t)(a))
#define ECD_REG32(a) (*(volatile uint32_t *)(uintptr_t)(a))
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
extern volatile uint16_t eagle_cd_phase;
extern volatile int16_t eagle_cd_error;
extern volatile uint32_t eagle_cd_sectors;
extern volatile uint32_t eagle_cd_last_lba;
extern uint8_t eagle_cd_reply[10];
int eagle_cd_drive_init(void);
int eagle_cd_read_begin(uint32_t lba);
int eagle_cd_read_next(uint32_t lba, uint8_t *sector);
int eagle_cd_read_sector(uint32_t lba, uint8_t *sector);
int eagle_cd_drive_command(uint8_t *command);
int eagle_cd_drive_pause(void);
void eagle_cd_decoder_enable(uint8_t enabled);
extern uint8_t eagle_cd_sector_buffer[2048];
extern volatile uint16_t eagle_cd_files_loaded;
int eagle_cd_mount(void);
int eagle_cd_load_ipl(void);
int32_t eagle_cd_load_list(const uint8_t *list);
int32_t eagle_cd_cdda(uint32_t command);
void eagle_cd_enter_user(void);
void eagle_cd_next_user(void);
#endif
