#ifndef SLF_SETTINGS_H
#define SLF_SETTINGS_H
#include <stdint.h>
#define SLF_SDK_VERSION "1.7.7"
#define SLF_DEFAULT_DIFFICULTY 3u
static inline uint8_t slf_difficulty_from_dip(uint8_t value) { return value < 7u ? (uint8_t)(value + 1u) : SLF_DEFAULT_DIFFICULTY; }
static inline uint8_t slf_seconds_from_dip(uint8_t value) { return value == 0u ? 30u : value == 2u ? 90u : 60u; }
#endif
