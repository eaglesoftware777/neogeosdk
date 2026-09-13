#ifndef DRIVER_DEFS_H
#define DRIVER_DEFS_H

#define DRIVER_READY_VALUE             0x01

#define DRIVER_FIFO_MASK               0x1F
#define DRIVER_ADPCMA_CHANNEL_COUNT    6
#define DRIVER_FM_CHANNEL_COUNT        4
#define DRIVER_FM_SILENCE_TL_WRITES    0x0D

#define DRIVER_PORT_COMMAND            0x00
#define DRIVER_PORT_STATUS             0x0C
#define DRIVER_PORT_NMI_ENABLE         0x08
#define DRIVER_PORT_NMI_DISABLE        0x18

#define DRIVER_CMD_SET_ADPCMA_VOLUME   0x05
#define DRIVER_CMD_SET_ADPCMB_VOLUME   0x06
#define DRIVER_CMD_SET_SSG_VOLUME      0x07
#define DRIVER_CMD_FADE_OUT            0x0A
#define DRIVER_CMD_STOP_ADPCMA         0x0C
#define DRIVER_CMD_STOP_ADPCMB         0x0D
#define DRIVER_CMD_SET_TEMPO           0x0E
#define DRIVER_CMD_STOP_MUSIC          0x0F
#define DRIVER_CMD_CANCEL_FADE         0x11
#define DRIVER_CMD_FADE_IN             0x12
#define DRIVER_CMD_SET_FM_VOLUME       0x13
#define DRIVER_CMD_SET_SSG_PRESET      0x14
#define DRIVER_CMD_PLAY_FM_PREFIX      0x31
#define DRIVER_CMD_PLAY_SSG_PREFIX     0x32

#define DRIVER_PARAM_NONE              0
#define DRIVER_PARAM_TEMPO             1
#define DRIVER_PARAM_ADPCMA_VOLUME     2
#define DRIVER_PARAM_ADPCMB_VOLUME     3
#define DRIVER_PARAM_SSG_VOLUME        4
#define DRIVER_PARAM_FADE_OUT          5
#define DRIVER_PARAM_FADE_IN           6
#define DRIVER_PARAM_FM_TRACK          7
#define DRIVER_PARAM_FM_VOLUME         8
#define DRIVER_PARAM_SSG_TRACK         9
#define DRIVER_PARAM_SSG_PRESET        10

#define DRIVER_DEFAULT_TEMPO_FRAMES    3
#define DRIVER_DEFAULT_SSG_VOLUME      0x0A
#define DRIVER_DEFAULT_MUSIC_VOLUME    0x0F
#define DRIVER_DEFAULT_ADPCMA_VOLUME   0x3F
#define DRIVER_DEFAULT_ADPCMB_VOLUME   0xA8

#define DRIVER_SSG_SILENT_MIXER        0x3F
#define DRIVER_SSG_SILENT_LEVEL        0x00
#define DRIVER_ADPCMA_MASTER_REG       0x01
#define DRIVER_ADPCMB_VOLUME_REG       0x1B
#define DRIVER_FM_KEY_REG              0x28
#define DRIVER_FM_KEY_CH1              0x01
#define DRIVER_FM_KEY_ALL_OPERATORS    0xF1

#define MUSIC_BASE                     0x20
#define SFX_A_BASE                     0x40
#define SFX_B_BASE                     0x80
#define SSG_CMD_PRESET                 DRIVER_CMD_SET_SSG_PRESET
#define SSG_CMD_PLAY                   DRIVER_CMD_PLAY_SSG_PREFIX

// Performance macros using compiler builtins for "closer to ASM" code generation
#define WRITE_A(r, v) write_a(r, v)
#define WRITE_B(r, v) write_b(r, v)
#define FORCE_A(r, v) force_a(r, v)
#define FORCE_B(r, v) force_b(r, v)

#endif
