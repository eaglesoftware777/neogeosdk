#ifndef NEOGEO_H
#define NEOGEO_H

#include <stdint.h>

#include "macro.h"
#include "sound_ids.h"

/*
 * Sprite control block helpers.
 *
 * Use these to build the SCB values consumed by vram_sprite() and the sprite
 * movement helpers.
 */
uint16_t setSCB2(uint16_t x_shrink, uint16_t y_shrink);
uint16_t setSCB3(uint16_t y_pos, uint16_t sticky_flag, uint16_t height_factor);
uint16_t setSCB4(uint16_t x_pos);
uint16_t setSCB1_2(uint16_t pal_offset, uint16_t tile_offset, uint16_t bit3_autoanim,
                   uint16_t bit2_autoanim, uint16_t vflip, uint16_t hflip);
uint16_t setFIXDATA(uint16_t palette_index, uint16_t tile_number);

/*
 * Palette and VRAM helpers.
 *
 * These functions work directly against the Neo Geo VRAM and palette memory
 * layout.
 */
void setBACKDROP(uint16_t backdrop_color);
void load_palettes(uint16_t *palette, uintptr_t palette_offset);
void vram_init(uint16_t start, uint16_t vram_inc);
void setpal(uint16_t *pal_tile, uint16_t t0, uint16_t t1, uint16_t t2, uint16_t t3, uint16_t t4,
            uint16_t t5, uint16_t t6, uint16_t t7, uint16_t t8, uint16_t t9, uint16_t t10,
            uint16_t t11, uint16_t t12, uint16_t t13, uint16_t t14, uint16_t t15);
void vram_SCB1(uint16_t *scb1_tiles, uint16_t *scb1_attrs, uint8_t tiles_number);
void vram_sfix(uint16_t vram_inc, uint16_t fix_addr, uint16_t fix_data);
void vram_sfix1(uint16_t fix_data);
void vram_SCB234(uint16_t scb_addr, uint16_t scb_value);
void vram_sprite(uint16_t vram_start, uint16_t vram_inc, uint16_t vram_offset, uint16_t *scb1_tiles,
                 uint16_t *scb1_attrs, uint16_t tiles_number, uint16_t scb2, uint16_t scb3,
                 uint16_t scb4);
void vram_sprite_mvx(uint16_t vram_offset, uint16_t scb4);
void vram_sprite_mvy(uint16_t vram_offset, uint16_t scb3);

/*
 * FIX layer and HUD helpers.
 *
 * The FIX plane is the text/UI layer. These functions are used for strings,
 * numbers, BIOS message status, and direct tile writes.
 */
void fixtext_out(uint16_t x, uint16_t y, char *text, short pal);
void fixtext_out1(uint16_t x, uint16_t y, uint16_t *tiles, short pal, int count);
void fixtext_out2(uint16_t x, uint16_t y, uint16_t a, uint16_t b, uint16_t c, uint16_t mod,
                  uint16_t *tiles, short pal, int count);
void fix_svalue1(uint16_t x, uint16_t y, uint16_t value, short pal, uint16_t offset);
void fix_svalue(uint16_t x, uint16_t y, uint16_t value, short pal);
void display_digit(uint16_t x, uint16_t y, uint32_t value, short pal, uint16_t offset);
void displayCreditP1(void);
void displayCreditP2(void);
void mess_outtest(void);
void setsfix(void);
void setBIOSMESSBusy(void);
void setBIOSMESSReady(void);

/*
 * Timing and housekeeping helpers.
 *
 * These are small utility calls for frame pacing, RAM clear, watchdog service,
 * and simple blocking delays.
 */
void clearRAM(void);
void clearSprs(void);
void clearFix(void);
void waitVbl(void);
void cycle10ms(void);
void cycle1s(void);
void cyclexms1(int ticks_10ms);
void cyclexs(int seconds);
void cyclexms(int milliseconds);
void kickWatchDog(void);
void sleep1FFF(void);

/*
 * Input and status helpers.
 *
 * poll_joystick() reads the BIOS-managed controller state, while read_joy1()
 * reads the hardware input ports directly.
 */
uint16_t poll_joystick(void);
uint16_t poll_joystick_edge(void);
int read_p1credit(void);
uint16_t read_joy1(void);

/* Low-level sound command helpers. */
void playSoundtest(uint16_t index);
void soundCommand(uint8_t command);
void isZ80Ready(void);

/* Core sound control. */
void soundInit(void);
void soundReset(void);
void soundStopAll(void);
void soundStopMusic(void);
void soundCancelFade(void);
void soundSceneReset(void);

/* Music and sample playback. */
void playMusic(uint8_t track);
void playSFX(uint8_t sample);
void playSFXB(uint8_t sample);

/* FM sequencer. */
void playFMDebug(void);
void playFMTrack(uint8_t track);
void soundSetFMVolume(uint8_t volume);

/* Standalone SSG sequencer. */
void playSSGTrack(uint8_t track);
void soundSetSSGPreset(uint8_t preset);
void playInsertCoinSSG(void);

/* Voice and cue helpers. */
void playVoiceCue(uint8_t cue);
void playGetReadyVoice(void);
void playAttackVoice(void);
void playCoinThenReady(void);

/* SSG-synthesised voice cues (driver $50 / $51 / $52). */
void playVoiceGetReady(void);
void playVoiceLetsGo(void);
void playVoiceGameOver(void);

/* YM2610 chip-level controls. */
void soundSetADPCMBPan(uint8_t pan);    /* $C0 stereo, $80 L, $40 R, $00 mute */
void soundFMSetLFO(uint8_t rate_enable);/* bit3 enable, bits0-2 rate */
void soundSetSSGNoise(uint8_t period);  /* 5-bit noise period */

/* Fades. */
void soundFadeOut(void);
void soundFadeIn(void);
void soundFadeOutSpeed(uint8_t speed);
void soundFadeInSpeed(uint8_t speed);

/* Runtime volume and tempo controls. */
void soundSetTempo(uint8_t tempo);
void soundSetADPCMAVolume(uint8_t volume);
void soundSetADPCMBVolume(uint8_t volume);
void soundSetSSGVolume(uint8_t volume);
void soundApplyMix(uint8_t adpcma_vol, uint8_t adpcmb_vol, uint8_t ssg_vol, uint8_t fm_vol);

/* High-level scene helpers used by the demo flow. */
void soundPlayDemoFM(uint8_t fm_track);
void soundPlayTitleMusic(uint8_t music_track);
void soundPlayGameLoop(uint8_t music_track);

#endif
