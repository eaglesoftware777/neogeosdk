#include "demo_sound.h"
#include "demo_screen.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include <stdint.h>

void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundStopMusic(void);
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
void NEOGEO_USER soundFadeOutSpeed(uint8_t speed);
void NEOGEO_USER waitVbl(void);
void NEOGEO_USER clearFix(void);
void NEOGEO_USER clearSprs(void);
void NEOGEO_USER fixtext_out(uint16_t x, uint16_t y, char *mess, short pal);

static uint8_t NEOGEO_USER demo_sound_hold(uint16_t frames)
{
    return demo_wait_frames_or_a(frames);
}

static uint8_t NEOGEO_USER demo_sound_play_line(uint8_t row, const char *name, uint8_t pal, uint8_t command)
{
    fixtext_out(2, row, "                                ", pal);
    fixtext_out(2, row, (char *)name, pal);
    playSFX(command);
    return demo_sound_hold(48);
}

void NEOGEO_USER showSoundDemo(void)
{
    demo_clear_scene();
    soundSceneReset();
    demo_scene_caption("YM2610 SOUND DEMO", "FM / SSG / ADPCM-A / ADPCM-B", "EVERY SAMPLE IS NAMED");

    /* FM channels — OPN 4-op FM synthesis. */
    soundSetFMVolume(0x0C);
    soundSetSSGVolume(0x00);
    soundSetADPCMAVolume(0x00);
    soundSetADPCMBVolume(0x00);

    fixtext_out(2, 7, "FM  ATTRACT FAST", 1);
    playFMTrack(SOUND_FM_ATTRACT_FAST);
    if (demo_sound_hold(110)) goto sound_done;

    fixtext_out(2, 8, "FM  DUEL SUSPENSE", 2);
    soundStopMusic();
    playFMTrack(SOUND_FM_DUEL_SUSPENSE);
    if (demo_sound_hold(110)) goto sound_done;

    fixtext_out(2, 9, "FM  VICTORY JINGLE", 0);
    soundStopMusic();
    playFMTrack(SOUND_FM_VICTORY_JINGLE);
    if (demo_sound_hold(90)) goto sound_done;
    soundStopAll();

    /* SSG — PSG oscillator test. */
    demo_clear_scene();
    soundSceneReset();
    demo_scene_caption("YM2610 SOUND DEMO", "SSG / PSG OSCILLATORS", "THREE-VOICE ARCADE TONE");
    soundSetSSGVolume(0x0F);
    soundSetFMVolume(0x00);

    fixtext_out(2, 7, "SSG ARCADE ALERT", 1);
    playSSGTrack(SOUND_SSG_ARCADE_ALERT);
    soundSetSSGPreset(1);
    if (demo_sound_hold(100)) goto sound_done;

    fixtext_out(2, 8, "SSG MENU LOOP", 2);
    playSSGTrack(SOUND_SSG_MENU_LOOP);
    if (demo_sound_hold(100)) goto sound_done;

    fixtext_out(2, 9, "SSG INSERT COIN", 0);
    playSSGTrack(SOUND_SSG_INSERT_COIN);
    soundSetSSGPreset(0);
    if (demo_sound_hold(90)) goto sound_done;
    soundStopAll();

    /* ADPCM-A — sample list with clear names. */
    demo_clear_scene();
    soundSceneReset();
    demo_scene_caption("YM2610 SOUND DEMO", "ADPCM-A 6CH SAMPLES", "NAMED SAMPLE PLAYBACK");
    soundSetADPCMAVolume(0x3F);
    soundSetADPCMBVolume(0x00);

    if (demo_sound_play_line(7,  "ADPCM-A  INTRO TAIKO",      1, SOUND_SFX_INTRO_TAIKO)) goto sound_done;
    if (demo_sound_play_line(8,  "ADPCM-A  TITLE GONG",       2, SOUND_SFX_TITLE_GONG)) goto sound_done;
    if (demo_sound_play_line(9,  "ADPCM-A  BLADE WHOOSH",     1, SOUND_SFX_BLADE_WHOOSH)) goto sound_done;
    if (demo_sound_play_line(10, "ADPCM-A  IMPACT HIT",       2, SOUND_SFX_IMPACT_HIT)) goto sound_done;
    if (demo_sound_play_line(11, "ADPCM-A  STRING PHRASE",    1, SOUND_SFX_STRING_PHRASE)) goto sound_done;
    if (demo_sound_play_line(12, "ADPCM-A  LOW DRUM",         2, SOUND_SFX_LOW_DRUM)) goto sound_done;
    if (demo_sound_play_line(13, "ADPCM-A  COIN CHIME",       1, SOUND_SFX_COIN_CHIME)) goto sound_done;
    if (demo_sound_play_line(14, "ADPCM-A  SHORT SHOUT",      2, SOUND_SFX_SHORT_SHOUT)) goto sound_done;

    /* ADPCM-B — long bed plus voice over. */
    demo_clear_scene();
    soundSceneReset();
    demo_scene_caption("YM2610 SOUND DEMO", "ADPCM-B STREAM + VOICES", "MUSIC BED WITH ADPCM-A OVERLAY");
    soundSetADPCMBVolume(0xBC);
    soundSetADPCMAVolume(0x3F);
    playSFXB(SOUND_BED_STAGE_TWO);
    fixtext_out(2, 7, "ADPCM-B  STAGE TWO BED", 1);
    if (demo_sound_hold(70)) goto sound_done;

    fixtext_out(2, 9, "VOICE CUE  GET READY", 2);
    playVoiceCue(SOUND_VOICE_GET_READY);
    if (demo_sound_hold(90)) goto sound_done;

    fixtext_out(2, 11, "VOICE CUE  ATTACK", 1);
    playVoiceCue(SOUND_VOICE_ATTACK);
    if (demo_sound_hold(90)) goto sound_done;

    /* Full mix — all channels together. */
    demo_clear_scene();
    soundSceneReset();
    demo_scene_caption("YM2610 SOUND DEMO", "FULL MIX", "FM + SSG + ADPCM-A + ADPCM-B");
    soundApplyMix(0x30, 0xB8, 0x08, 0x08);
    playSFXB(SOUND_BED_STAGE_ONE);
    playFMTrack(SOUND_FM_BASS_MOTIF);
    playSSGTrack(SOUND_SSG_INSERT_COIN);
    soundSetSSGPreset(1);
    fixtext_out(2, 7, "BED  STAGE ONE", 1);
    fixtext_out(2, 8, "FM   BASS MOTIF", 2);
    fixtext_out(2, 9, "SSG  INSERT COIN", 1);
    if (demo_sound_hold(60)) goto sound_done;
    fixtext_out(2, 11, "ADPCM-A  BLADE WHOOSH", 2);
    playSFX(SOUND_SFX_BLADE_WHOOSH);
    demo_sound_hold(130);

sound_done:
    soundFadeOutSpeed(8);
    demo_wait_frames_or_a(45);
    soundStopAll();
    demo_clear_scene();
}

void NEOGEO_USER demo_sound_showcase(void)
{
    showSoundDemo();
    demo_clear_scene();
}
