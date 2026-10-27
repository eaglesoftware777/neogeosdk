/* Exercise the production directory/IPL loader against a bounded host bus. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eagle_cd.h"

volatile uint32_t eagle_cd_frames, eagle_cd_sectors, eagle_cd_last_lba;
volatile uint16_t eagle_cd_phase;
volatile int16_t eagle_cd_error;
static uint8_t program[0x100000], upload[0x100000], registers[512];
static uint8_t *disc;
static size_t disc_size;
static unsigned upload_writes;

uint8_t *eagle_cd_host_reg8(uintptr_t address)
{
    if (address < sizeof(program)) return program+address;
    if (address >= 0xE00000 && address < 0xF00000) {
        ++upload_writes;
        return upload+address-0xE00000;
    }
    if (address >= 0xFF0000 && address < 0xFF0200) return registers+address-0xFF0000;
    fprintf(stderr, "unbounded bus access: %lx\n", (unsigned long)address);
    abort();
}
int eagle_cd_read_begin(uint32_t lba)
{ return lba < disc_size/2048 ? 0 : -5; }
int eagle_cd_read_sector(uint32_t lba, uint8_t *out)
{
    if (lba >= disc_size/2048) return -5;
    memcpy(out,disc+(size_t)lba*2048,2048);
    ++eagle_cd_sectors;
    return 0;
}
int eagle_cd_read_next(uint32_t lba, uint8_t *out)
{ return eagle_cd_read_sector(lba,out); }
int eagle_cd_drive_pause(void) { return 0; }
void eagle_cd_decoder_enable(uint8_t enabled) { (void)enabled; }

int main(int argc, char **argv)
{
    FILE *f;
    long length;
    int mounted, loaded = -999;
    if (argc != 2 || !(f=fopen(argv[1],"rb"))) return 2;
    if (fseek(f,0,SEEK_END) || (length=ftell(f)) <= 0 || length > 2*1024*1024) return 2;
    rewind(f);
    disc_size=(size_t)length;
    disc=malloc(disc_size);
    if (!disc || fread(disc,1,disc_size,f)!=disc_size) return 2;
    fclose(f);
    mounted=eagle_cd_mount();
    if (!mounted) loaded=eagle_cd_load_ipl();
    printf("%d,%d,%u,%u\n",mounted,loaded,(unsigned)eagle_cd_files_loaded,upload_writes);
    free(disc);
    return 0;
}
