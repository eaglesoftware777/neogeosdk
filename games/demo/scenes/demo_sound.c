/*
 * demo_sound.c — Scene 10: YM2610 audio engine showcase
 *
 * Demonstrates: FM tracks, SSG oscillators, ADPCM-A samples,
 * ADPCM-B TRACK, voice cues, full mix.
 *
 * https://eaglesoftware.biz
 */

#include "demo_sound.h"
#include "demo.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include <stdint.h>

void NEOGEO_USER waitVbl(void);
void NEOGEO_USER clearFix(void);
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

static void NEOGEO_USER snd_vbl(uint8_t frames)
{
    uint8_t i;
    for (i = 0u; i < frames; i++) waitVbl();
}

static void NEOGEO_USER snd_reset(void)
{
    soundCancelFade();
    soundStopAll();
    snd_vbl(6u);
    soundSceneReset();
    snd_vbl(8u);
    soundCancelFade();
}

static void NEOGEO_USER snd_label(uint8_t row, const char *kind, const char *name, uint8_t pal)
{
    demo_fix_puts(2u, row, "                                    ", 0u);
    demo_fix_puts(2u, row, kind, pal);
    demo_fix_puts(12u, row, name, pal);
}

/* ------------------------------------------------------------------ */
/*  Public: sound scene                                                  */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_sound_run(void)
{
    demo_clear_scene();
    snd_reset();

    /* -- FM Tracks -- */
    demo_caption("YM2610 SOUND DEMO", "FM SYNTHESIS  OPN2 CHANNELS", "FM = 4 CHANNELS  FULL POLYPHONY");
    snd_vbl(24u);

    soundSetFMVolume(0x0Cu);
    soundSetSSGVolume(0x00u);
    soundSetADPCMAVolume(0x00u);
    soundSetADPCMBVolume(0x00u);
    soundCancelFade();

    snd_label(7u, "FM", "ATTRACT FAST", 1u);
    playFMTrack(SOUND_FM_C);
    if (demo_wait(120u)) goto done;

    snd_label(8u, "FM", "DUEL SUSPENSE", 2u);
    soundStopMusic();
    snd_vbl(4u);
    playFMTrack(SOUND_FM_D);
    if (demo_wait(120u)) goto done;

    snd_label(9u, "FM", "VICTORY JINGLE", 0u);
    soundStopMusic();
    snd_vbl(4u);
    playFMTrack(SOUND_FM_F);
    if (demo_wait(100u)) goto done;

    /* -- SSG Tracks -- */
    demo_clear_scene();
    snd_reset();
    demo_caption("YM2610 SOUND DEMO", "SSG / PSG OSCILLATORS", "3 SQUARE-WAVE CHANNELS");
    soundSetSSGVolume(0x0Fu);
    soundSetFMVolume(0x00u);
    snd_vbl(16u);

    snd_label(7u, "SSG", "ARCADE ALERT", 1u);
    playSSGTrack(SOUND_SSG_B);
    soundSetSSGPreset(1u);
    if (demo_wait(100u)) goto done;

    snd_label(8u, "SSG", "MENU LOOP", 2u);
    playSSGTrack(SOUND_SSG_A);
    if (demo_wait(100u)) goto done;

    snd_label(9u, "SSG", "INSERT COIN", 0u);
    playSSGTrack(SOUND_SSG_C);
    soundSetSSGPreset(0u);
    if (demo_wait(96u)) goto done;

    /* -- ADPCM-A Samples -- */
    demo_clear_scene();
    snd_reset();
    demo_caption("YM2610 SOUND DEMO", "ADPCM-A 6CH SAMPLES", "DELTA-SIGMA PCM  ~18.5 KHZ");
    soundSetADPCMAVolume(0x3Fu);
    soundSetADPCMBVolume(0x00u);
    snd_vbl(16u);

    snd_label(7u,  "ADPCM-A", "INTRO TAIKO",   1u); playSFX(SOUND_SFX_4);   if (demo_wait(72u)) goto done;
    snd_label(8u,  "ADPCM-A", "TITLE GONG",    2u); playSFX(SOUND_SFX_3);    if (demo_wait(72u)) goto done;
    snd_label(9u,  "ADPCM-A", "BLADE WHOOSH",  1u); playSFX(SOUND_SFX_7);  if (demo_wait(72u)) goto done;
    snd_label(10u, "ADPCM-A", "IMPACT HIT",    2u); playSFX(SOUND_SFX_8);    if (demo_wait(72u)) goto done;
    snd_label(11u, "ADPCM-A", "STRING PHRASE", 1u); playSFX(SOUND_SFX_9); if (demo_wait(72u)) goto done;
    snd_label(12u, "ADPCM-A", "LOW DRUM",      2u); playSFX(SOUND_SFX_10);      if (demo_wait(72u)) goto done;
    snd_label(13u, "ADPCM-A", "COIN CHIME",    1u); playSFX(SOUND_SFX_1);    if (demo_wait(72u)) goto done;
    snd_label(14u, "ADPCM-A", "SHORT SHOUT",   2u); playSFX(SOUND_SFX_6);   if (demo_wait(72u)) goto done;

    /* -- ADPCM-B TRACK + Voices -- */
    demo_clear_scene();
    snd_reset();
    demo_caption("YM2610 SOUND DEMO", "ADPCM-B TRACK + VOICE CUES", "STREAMING MUSIC TRACK  ~55 KHZ");
    soundSetADPCMBVolume(0xC0u);
    soundSetADPCMAVolume(0x3Fu);
    snd_vbl(16u);

    snd_label(7u,  "ADPCM-B", "STAGE TWO TRACK", 1u);
    playSFXB(SOUND_TRACK_C);
    if (demo_wait(120u)) goto done;

    snd_label(9u,  "VOICE", "GET READY", 2u);
    playVoiceCue(SOUND_VOICE_1);
    if (demo_wait(84u)) goto done;

    snd_label(11u, "VOICE", "ATTACK", 1u);
    playVoiceCue(SOUND_VOICE_2);
    if (demo_wait(84u)) goto done;

    /* -- Full Mix -- */
    demo_clear_scene();
    snd_reset();
    demo_caption("YM2610 SOUND DEMO", "FULL MIX", "FM + SSG + ADPCM-A + ADPCM-B");
    soundApplyMix(0x30u, 0xC0u, 0x08u, 0x08u);
    snd_vbl(4u);
    playSFXB(SOUND_TRACK_B);
    playFMTrack(SOUND_FM_E);
    playSSGTrack(SOUND_SSG_C);
    soundSetSSGPreset(1u);
    snd_label(7u, "TRACK",     "STAGE ONE",   1u);
    snd_label(8u, "FM",      "BASS MOTIF",  2u);
    snd_label(9u, "SSG",     "INSERT COIN", 1u);
    if (demo_wait(120u)) goto done;
    snd_label(11u, "ADPCM-A", "BLADE WHOOSH", 2u);
    playSFX(SOUND_SFX_7);
    demo_wait(96u);

done:
    soundCancelFade();
    snd_vbl(4u);
    soundStopAll();
    snd_vbl(8u);
    soundSceneReset();
    snd_vbl(8u);
    soundCancelFade();
    demo_clear_scene();
}

/* Legacy compat */
void NEOGEO_USER showSoundDemo(void)
{
    demo_sound_run();
}
