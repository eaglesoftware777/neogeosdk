#include "demo_sound.h"
#include "demo_screen.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include <stdint.h>

void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundStopMusic(void);
void NEOGEO_USER soundCancelFade(void);
void NEOGEO_USER soundSetFMVolume(uint8_t v);
void NEOGEO_USER soundSetSSGVolume(uint8_t v);
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v);
void NEOGEO_USER soundSetADPCMBVolume(uint8_t v);
void NEOGEO_USER playFMTrack(uint8_t n);
void NEOGEO_USER playSSGTrack(uint8_t n);
void NEOGEO_USER soundSetSSGPreset(uint8_t preset);
void NEOGEO_USER playSFX(uint8_t n);
void NEOGEO_USER playSFXB(uint8_t n);
void NEOGEO_USER playVoiceCue(uint8_t n);
void NEOGEO_USER soundApplyMix(uint8_t a, uint8_t b, uint8_t s, uint8_t f);
void NEOGEO_USER waitVbl(void);
void NEOGEO_USER clearFix(void);
void NEOGEO_USER fixtext_out(uint16_t x, uint16_t y, char *mess, short pal);

static void NEOGEO_USER demo_sound_vbl(uint8_t frames)
{
    uint8_t i;
    for (i = 0; i < frames; i++) waitVbl();
}

static void NEOGEO_USER demo_sound_reset(void)
{
    soundCancelFade();
    soundStopAll();
    demo_sound_vbl(6);
    soundSceneReset();
    demo_sound_vbl(8);
    soundCancelFade();
}

static uint8_t NEOGEO_USER demo_sound_hold(uint16_t frames)
{
    return demo_wait_frames_or_a(frames);
}

static void NEOGEO_USER demo_sound_line(uint8_t row, const char *kind, const char *name, uint8_t pal)
{
    demo_fix_puts(2, row, "                                    ", 0);
    demo_fix_puts(2, row, kind, pal);
    demo_fix_puts(12, row, name, pal);
}

static uint8_t NEOGEO_USER demo_sound_play_a(uint8_t row, const char *name, uint8_t pal, uint8_t sample)
{
    demo_sound_line(row, "ADPCM-A", name, pal);
    playSFX(sample);
    return demo_sound_hold(72);
}

void NEOGEO_USER showSoundDemo(void)
{
    demo_clear_scene();
    demo_sound_reset();
    demo_scene_caption("YM2610 SOUND DEMO", "CHANNELS ARE LABELED BEFORE PLAY", "A ADVANCES / START IS IGNORED HERE");
    demo_sound_hold(24);

    soundSetFMVolume(0x0C);
    soundSetSSGVolume(0x00);
    soundSetADPCMAVolume(0x00);
    soundSetADPCMBVolume(0x00);
    soundCancelFade();

    demo_sound_line(7, "FM", "ATTRACT FAST", 1);
    playFMTrack(SOUND_FM_ATTRACT_FAST);
    if (demo_sound_hold(120)) goto sound_done;

    demo_sound_line(8, "FM", "DUEL SUSPENSE", 2);
    soundStopMusic();
    demo_sound_vbl(4);
    playFMTrack(SOUND_FM_DUEL_SUSPENSE);
    if (demo_sound_hold(120)) goto sound_done;

    demo_sound_line(9, "FM", "VICTORY JINGLE", 0);
    soundStopMusic();
    demo_sound_vbl(4);
    playFMTrack(SOUND_FM_VICTORY_JINGLE);
    if (demo_sound_hold(100)) goto sound_done;

    demo_clear_scene();
    demo_sound_reset();
    demo_scene_caption("YM2610 SOUND DEMO", "SSG / PSG OSCILLATORS", "TEXT FIRST THEN SOUND");
    soundSetSSGVolume(0x0F);
    soundSetFMVolume(0x00);
    demo_sound_hold(16);

    demo_sound_line(7, "SSG", "ARCADE ALERT", 1);
    playSSGTrack(SOUND_SSG_ARCADE_ALERT);
    soundSetSSGPreset(1);
    if (demo_sound_hold(100)) goto sound_done;

    demo_sound_line(8, "SSG", "MENU LOOP", 2);
    playSSGTrack(SOUND_SSG_MENU_LOOP);
    if (demo_sound_hold(100)) goto sound_done;

    demo_sound_line(9, "SSG", "INSERT COIN", 0);
    playSSGTrack(SOUND_SSG_INSERT_COIN);
    soundSetSSGPreset(0);
    if (demo_sound_hold(96)) goto sound_done;

    demo_clear_scene();
    demo_sound_reset();
    demo_scene_caption("YM2610 SOUND DEMO", "ADPCM-A 6CH SAMPLES", "SAMPLES ARE SHORT AND NAMED");
    soundSetADPCMAVolume(0x3F);
    soundSetADPCMBVolume(0x00);
    demo_sound_hold(16);

    if (demo_sound_play_a(7,  "INTRO TAIKO",   1, SOUND_SFX_INTRO_TAIKO)) goto sound_done;
    if (demo_sound_play_a(8,  "TITLE GONG",    2, SOUND_SFX_TITLE_GONG)) goto sound_done;
    if (demo_sound_play_a(9,  "BLADE WHOOSH",  1, SOUND_SFX_BLADE_WHOOSH)) goto sound_done;
    if (demo_sound_play_a(10, "IMPACT HIT",    2, SOUND_SFX_IMPACT_HIT)) goto sound_done;
    if (demo_sound_play_a(11, "STRING PHRASE", 1, SOUND_SFX_STRING_PHRASE)) goto sound_done;
    if (demo_sound_play_a(12, "LOW DRUM",      2, SOUND_SFX_LOW_DRUM)) goto sound_done;
    if (demo_sound_play_a(13, "COIN CHIME",    1, SOUND_SFX_COIN_CHIME)) goto sound_done;
    if (demo_sound_play_a(14, "SHORT SHOUT",   2, SOUND_SFX_SHORT_SHOUT)) goto sound_done;

    demo_clear_scene();
    demo_sound_reset();
    demo_scene_caption("YM2610 SOUND DEMO", "ADPCM-B BED + VOICES", "MUSIC BED WITH VOICE OVERLAY");
    soundSetADPCMBVolume(0xC0);
    soundSetADPCMAVolume(0x3F);
    demo_sound_hold(16);

    demo_sound_line(7, "ADPCM-B", "STAGE TWO BED", 1);
    playSFXB(SOUND_BED_STAGE_TWO);
    if (demo_sound_hold(120)) goto sound_done;

    demo_sound_line(9, "VOICE", "GET READY", 2);
    playVoiceCue(SOUND_VOICE_GET_READY);
    if (demo_sound_hold(84)) goto sound_done;

    demo_sound_line(11, "VOICE", "ATTACK", 1);
    playVoiceCue(SOUND_VOICE_ATTACK);
    if (demo_sound_hold(84)) goto sound_done;

    demo_clear_scene();
    demo_sound_reset();
    demo_scene_caption("YM2610 SOUND DEMO", "FULL MIX", "FM + SSG + ADPCM-A + ADPCM-B");
    soundApplyMix(0x30, 0xC0, 0x08, 0x08);
    demo_sound_vbl(4);
    playSFXB(SOUND_BED_STAGE_ONE);
    playFMTrack(SOUND_FM_BASS_MOTIF);
    playSSGTrack(SOUND_SSG_INSERT_COIN);
    soundSetSSGPreset(1);
    demo_sound_line(7, "BED", "STAGE ONE", 1);
    demo_sound_line(8, "FM", "BASS MOTIF", 2);
    demo_sound_line(9, "SSG", "INSERT COIN", 1);
    if (demo_sound_hold(120)) goto sound_done;
    demo_sound_line(11, "ADPCM-A", "BLADE WHOOSH", 2);
    playSFX(SOUND_SFX_BLADE_WHOOSH);
    demo_sound_hold(96);

sound_done:
    /* Do not leave fade state active for the next demo section. */
    soundCancelFade();
    demo_sound_vbl(4);
    soundStopAll();
    demo_sound_vbl(8);
    soundSceneReset();
    demo_sound_vbl(8);
    soundCancelFade();
    demo_clear_scene();
}

void NEOGEO_USER demo_sound_showcase(void)
{
    showSoundDemo();
    demo_clear_scene();
}
