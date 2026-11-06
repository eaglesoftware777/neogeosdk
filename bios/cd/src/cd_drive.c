/* Original CD transport. Register/packet contracts are documented in the
 * hardware ledger. No firmware or emulator implementation is embedded. */
#include "eagle_cd.h"

volatile uint16_t eagle_cd_phase;
volatile int16_t eagle_cd_error;
volatile uint32_t eagle_cd_sectors, eagle_cd_last_lba;
uint8_t eagle_cd_reply[10];
static uint8_t last_track;

static uint8_t decoder_read(uint8_t reg)
{
    ECD_REG8(0xFF0101) = reg;
    return ECD_REG8(0xFF0103);
}

static void decoder_write(uint8_t reg, uint8_t value)
{
    ECD_REG8(0xFF0101) = reg;
    ECD_REG8(0xFF0103) = value;
}

static int clock_wait(uint8_t high)
{
    uint16_t tries = 10000;
    do {
        if ((ECD_REG8(0xFF0161) & 0x10u) == high) return 0;
        ECD_REG8(0x300001) = 0;
    } while (--tries);
    return -110;
}

static int receive_packet(void)
{
    uint8_t i, sum = 5;
    for (i = 0; i < 10u; ++i) {
        ECD_REG8(0xFF0165) = 2;
        if (clock_wait(0)) return -110;
        eagle_cd_reply[i] = ECD_REG8(0xFF0161) & 15u;
        sum += eagle_cd_reply[i];
        ECD_REG8(0xFF0165) = 3;
        if (clock_wait(0x10)) return -110;
    }
    return (sum & 15u) == 15u ? 0 : -5;
}

int eagle_cd_drive_command(uint8_t *command)
{
    uint8_t i, sum = 5;
    int result;
    for (i = 0; i < 9u; ++i) sum += command[i];
    command[9] = (uint8_t)(~sum & 15u);
    for (i = 0; i < 10u; ++i) {
        ECD_REG8(0xFF0163) = command[i];
        ECD_REG8(0xFF0165) = 0;
        ECD_REG8(0xFF0165) = 1;
        ECD_REG8(0x300001) = 0;
    }
    /* Serial commands and responses are separate transactions. Drain the
     * previous response, then accept a checksummed response to this request. */
    (void)receive_packet();
    result = receive_packet();
    return result;
}

int eagle_cd_drive_pause(void)
{
    uint8_t command[10] = {6,0,0,0,0,0,0,0,0,0};
    return eagle_cd_drive_command(command);
}

void eagle_cd_decoder_enable(uint8_t enabled)
{
    decoder_write(10, enabled ? 0x84u : 0u);
}

int eagle_cd_drive_init(void)
{
    uint8_t command[10] = {2,0,0,4,0,0,0,0,0,0};
    /* LC8953 sector-to-DRAM transfer program. These register words are
     * recorded in the public MAME CD hardware driver; program the actual
     * controller rather than relying only on an emulator's mode heuristic. */
    static const uint16_t dma_program[9] = {
        0xFFC5,0xFCF5,0xA6F6,0x2602,0xFDFF,0xFC2D,0xFCF5,0x8492,0xDA92
    };
    uint16_t i;
    uint32_t began;
    int result;
    eagle_cd_phase = 1;
    ECD_REG8(0xFF016D) = 0;
    ECD_REG8(0xFF0181) = 0;
    ECD_REG8(0xFF0165) = 3;
    ECD_REG8(0xFF0165) = 1;
    ECD_REG16(0xFF0002) = 0;
    ECD_REG8(0xFF000F) = 0x38;
    ECD_REG8(0xFF0181) = 1;
    decoder_write(15, 0);
    decoder_write(1, 0x22); /* decoder interrupt plus data output */
    decoder_write(10, 0x84); /* decode Mode 1 sectors into external buffer */
    decoder_write(11, 0);
    ECD_REG8(0xFF0061) = 0;
    for (i = 0; i < 9u; ++i) ECD_REG16(0xFF007Eul+i*2u) = dma_program[i];
    result = eagle_cd_drive_command(command);
    if (result) return result;
    began = eagle_cd_frames;
    while (eagle_cd_reply[1] != 4u || !(eagle_cd_reply[4] | eagle_cd_reply[5])) {
        if (eagle_cd_frames-began > 180u) return -2;
        eagle_cd_wait();
        result = receive_packet();
        if (result) return result;
    }
    last_track = (uint8_t)(eagle_cd_reply[4]*10u+eagle_cd_reply[5]);
    if (!last_track || last_track > 99u) return -22;
    eagle_cd_phase = 2;
    return 0;
}

int eagle_cd_read_begin(uint32_t lba)
{
    uint32_t msf = lba + 150u;
    uint16_t minute = (uint16_t)(msf / 4500u);
    uint16_t second = (uint16_t)((msf % 4500u) / 75u);
    uint16_t frame = (uint16_t)(msf % 75u);
    uint8_t command[10] = {3,0,0,0,0,0,0,0,0,0};
    if (minute > 99u) return -34;
    eagle_cd_decoder_enable(1);
    command[2] = minute / 10u; command[3] = minute % 10u;
    command[4] = second / 10u; command[5] = second % 10u;
    command[6] = frame / 10u; command[7] = frame % 10u;
    (void)decoder_read(15); /* clear an earlier decode notification */
    return eagle_cd_drive_command(command);
}

int32_t eagle_cd_cdda(uint32_t word)
{
    uint8_t action = (uint8_t)((word >> 8) & 0x3Fu);
    uint8_t track = (uint8_t)word;
    uint8_t command[10] = {2,0,0,5,0,0,0,0,0,0};
    uint8_t i;
    int result;
    if (action == 2u) return eagle_cd_drive_pause();
    if (action == 3u) { command[0] = 7; command[3] = 0; return eagle_cd_drive_command(command); }
    if (action > 1u) return -95;
    if ((track >> 4) > 9u || (track & 15u) > 9u || !track ||
        (track >> 4)*10u+(track & 15u) > last_track) return -22;
    command[4] = track >> 4; command[5] = track & 15u;
    result = eagle_cd_drive_command(command);
    if (result) return result;
    if (eagle_cd_reply[1] != 5u || (eagle_cd_reply[6] & 8u)) return -22;
    command[0] = 3; command[3] = 0;
    for (i = 2; i < 8u; ++i) command[i] = eagle_cd_reply[i];
    command[6] &= 7u;
    eagle_cd_decoder_enable(0);
    return eagle_cd_drive_command(command);
}

static uint16_t from_bcd(uint8_t value)
{
    return (uint16_t)((value >> 4) * 10u + (value & 15u));
}

int eagle_cd_read_next(uint32_t lba, uint8_t *sector)
{
    uint32_t start = eagle_cd_frames;
    for (;;) {
        uint16_t pointer;
        uint32_t actual;
        uint8_t minute, second, frame;
        ECD_REG8(0x300001) = 0;
        if (eagle_cd_frames - start > 180u) return -110;
        if (decoder_read(1) & 0x20u) continue;
        minute = decoder_read(4); second = decoder_read(5); frame = decoder_read(6);
        actual = ((uint32_t)from_bcd(minute) * 60u + from_bcd(second)) * 75u + from_bcd(frame);
        if (actual < 150u) { (void)decoder_read(15); continue; }
        actual -= 150u;
        if (actual < lba) { (void)decoder_read(15); continue; }
        if (actual != lba) return -5;
        pointer = decoder_read(8);
        pointer |= (uint16_t)decoder_read(9) << 8;
        pointer = (uint16_t)((pointer + 4u) & 0x7FFFu);
        decoder_write(2, 0xFF); decoder_write(3, 7);
        decoder_write(4, (uint8_t)pointer); decoder_write(5, (uint8_t)(pointer >> 8));
        decoder_write(6, 0xFF);
        ECD_REG32(0xFF0064) = (uint32_t)(uintptr_t)sector;
        ECD_REG32(0xFF0070) = 1024;
        ECD_REG16(0xFF007E) = 0xFFC5;
        ECD_REG8(0xFF0061) = 0x40;
        (void)decoder_read(15);
        ++eagle_cd_sectors;
        eagle_cd_last_lba = actual;
        return 0;
    }
}

int eagle_cd_read_sector(uint32_t lba, uint8_t *sector)
{
    int result = eagle_cd_read_begin(lba);
    if (result) return result;
    return eagle_cd_read_next(lba, sector);
}
