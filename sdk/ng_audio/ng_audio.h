#ifndef SDK_NG_AUDIO_NG_AUDIO_H
#define SDK_NG_AUDIO_NG_AUDIO_H

#include <stdint.h>
#include "macro.h"

#ifndef NEOGEO_USER
#define NEOGEO_USER
#endif

typedef struct {
    uint8_t adpcma;
    uint8_t adpcmb;
    uint8_t ssg;
    uint8_t fm;
} NGAudioMix;

void NEOGEO_USER ngaudio_scene_reset(const NGAudioMix *mix);
void NEOGEO_USER ngaudio_stop_scene(uint8_t fade_speed, uint16_t hold_frames);
void NEOGEO_USER ngaudio_label_sfx(uint8_t row, const char *label, uint8_t pal, uint8_t sfx);
void NEOGEO_USER ngaudio_label_music(uint8_t row, const char *label, uint8_t pal);

#endif
