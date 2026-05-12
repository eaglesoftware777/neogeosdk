#include "ng_audio/ng_audio.h"
#include "ng_fix/ng_fix.h"
#include "neogeo.h"

void NEOGEO_USER ngaudio_scene_reset(const NGAudioMix *mix)
{
    soundSceneReset();
    if (mix) soundApplyMix(mix->adpcma, mix->adpcmb, mix->ssg, mix->fm);
}

void NEOGEO_USER ngaudio_stop_scene(uint8_t fade_speed, uint16_t hold_frames)
{
    uint16_t i;

    if (fade_speed) soundFadeOutSpeed(fade_speed);
    for (i = 0; i < hold_frames; i++) waitVbl();
    soundStopAll();
}

void NEOGEO_USER ngaudio_label_sfx(uint8_t row, const char *label, uint8_t pal, uint8_t sfx)
{
    ngfix_clear_line(row);
    ngfix_puts(2, row, label, pal);
    playSFX(sfx);
}

void NEOGEO_USER ngaudio_label_music(uint8_t row, const char *label, uint8_t pal)
{
    ngfix_clear_line(row);
    ngfix_puts(2, row, label, pal);
}
