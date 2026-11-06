/* CDDA replaces cartridge ADPCM-B music. SFX/voices use our RAM ADPCM-A driver. */
#include "sdk/neogeo.h"
#include "sdk/platform/cd/ng_cd.h"
#include "sky_cd_audio.h"

extern int32_t ng_cd_bios_cdda(uint32_t command);
volatile uint32_t sky_cd_sfx_calls,sky_cd_music_requests,sky_cd_sound_drops;
volatile uint16_t sky_cd_music_track,sky_cd_sound_reply;
static uint8_t queue[16],head,count,music_pending;
static uint16_t music_word;
static uint32_t music_started;
static uint8_t music_loop;

static void enqueue(uint8_t value)
{
    if(count==16u) {++sky_cd_sound_drops;return;}
    queue[(head+count)&15u]=value;++count;
}

void sky_cd_audio_tick(void)
{
    sky_cd_sound_reply=NEO_REGISTER8(0x320000);
    if(count && sky_cd_sound_reply==1u) {
        NEO_REGISTER8(0x320000)=queue[head];head=(head+1u)&15u;--count;
    }
    if(music_pending) {
        (void)ng_cd_bios_cdda(music_word);
        music_started=ng_cd_frames;music_pending=0u;++sky_cd_music_requests;
    } else if(music_loop && sky_cd_music_track>=2u &&
              ng_cd_frames-music_started>=sky_cd_track_frames[sky_cd_music_track-2u]) {
        (void)ng_cd_bios_cdda(0x0200u);
        (void)ng_cd_bios_cdda(music_word);
        music_started=ng_cd_frames;++sky_cd_music_requests;
    }
}
void NEOGEO_USER soundInit(void)
{
    NEO_REGISTER8(0xFF0183)=0;NEO_REGISTER8(0xFF0183)=0xFF;
    head=count=music_pending=0u;music_loop=1u;sky_cd_music_track=0;
}
void NEOGEO_USER playSFX(uint8_t sample)
{
    if(sample>=SKY_CD_SAMPLE_COUNT) {++sky_cd_sound_drops;return;}
    enqueue((uint8_t)(0x40u+sample));++sky_cd_sfx_calls;
}
void NEOGEO_USER playSFXB(uint8_t track)
{
    uint8_t actual;
    if(track>=SKY_CD_TRACK_COUNT) return;
    actual=(uint8_t)(track+2u);sky_cd_music_track=actual;
    music_word=(uint16_t)(((actual/10u)<<4)|(actual%10u));music_pending=1u;
}
void NEOGEO_USER soundStopAll(void)
{ head=count=0u;enqueue(1u);music_word=0x0200u;music_pending=1u;sky_cd_music_track=0; }
void NEOGEO_USER soundSceneReset(void) { soundStopAll(); }
void NEOGEO_USER soundCancelFade(void) { }
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v) { enqueue((uint8_t)(0x80u|(v&63u))); }
void NEOGEO_USER soundSetADPCMBLoop(uint8_t enabled) { music_loop=(uint8_t)(enabled!=0); }
void NEOGEO_USER soundSetADPCMBVolume(uint8_t v) { (void)v; /* CDDA is not ADPCM-B. */ }
void NEOGEO_USER soundSetSSGVolume(uint8_t v) { (void)v; }
void NEOGEO_USER soundSetFMVolume(uint8_t v) { (void)v; }
