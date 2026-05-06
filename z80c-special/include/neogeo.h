#ifndef Z80CC_NEOGEO_H
#define Z80CC_NEOGEO_H

#define Z80_PORT_68K_CMD 0x00
#define Z80_PORT_YM_A_ADDR 0x04
#define Z80_PORT_YM_A_DATA 0x05
#define Z80_PORT_YM_B_ADDR 0x06
#define Z80_PORT_YM_B_DATA 0x07
#define Z80_PORT_68K_STATUS 0x0c

#define YM_A(reg, value) ym_a((reg), (value))
#define YM_B(reg, value) ym_b((reg), (value))
#define Z80_READY() out(Z80_PORT_68K_STATUS, 1)
#define Z80_BUSY() out(Z80_PORT_68K_STATUS, 0)

#define ADPCMB_FORCE_STOP 0x01
#define ADPCMB_KEY_ON 0x80
#define ADPCM_STEREO 0xc0

#endif
