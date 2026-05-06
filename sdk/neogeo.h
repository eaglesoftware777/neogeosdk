#ifndef NEOGEO_H
#define NEOGEO_H

#include <stdint.h>
#include "macro.h"
#include "sound_ids.h"

/* Core sound control */
void soundInit(void);
void soundReset(void);
void soundStopAll(void);
void soundStopMusic(void);
void soundCancelFade(void);
void soundSceneReset(void);

/* Music / effects playback */
void playMusic(uint8_t track);
void playSFX(uint8_t sample);
void playSFXB(uint8_t sample);

/* FM sequencer */
void playFMDebug(void);
void playFMTrack(uint8_t track);
void soundSetFMVolume(uint8_t volume);

/* Standalone SSG sequencer */
void playSSGTrack(uint8_t track);
void soundSetSSGPreset(uint8_t preset);
void playInsertCoinSSG(void);

/* Voice / cue helpers */
void playVoiceCue(uint8_t cue);
void playGetReadyVoice(void);
void playAttackVoice(void);
void playCoinThenReady(void);

/* Fades */
void soundFadeOut(void);
void soundFadeIn(void);
void soundFadeOutSpeed(uint8_t speed);
void soundFadeInSpeed(uint8_t speed);

/* Runtime volume / tempo */
void soundSetTempo(uint8_t tempo);
void soundSetADPCMAVolume(uint8_t volume);
void soundSetADPCMBVolume(uint8_t volume);
void soundSetSSGVolume(uint8_t volume);

/* Higher-level scene helpers */
void soundPlayDemoFM(uint8_t fm_track);
void soundPlayTitleMusic(uint8_t music_track);
void soundPlayGameLoop(uint8_t music_track);

/* Debug raw command helper */
void playSoundtest(uint16_t index);

/* Low-level sound command helpers */
void soundCommand(uint8_t command);
void isZ80Ready(void);

/* Input */
int read_joy1(void);

#endif
