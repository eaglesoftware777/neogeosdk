#include "eagle_cd.h"

typedef struct { char name[14]; uint32_t lba, size; } CDFile;
typedef struct { uint16_t file; uint8_t zone, bank; uint32_t offset; } CDLoad;
enum { PRG, SPR, FIX, Z80, PCM };
static CDFile files[512];
static CDLoad boot[32];
static uint16_t file_count, boot_count;
static uint32_t volume_sectors;
static uint8_t ipl[1024];
uint8_t eagle_cd_sector_buffer[2048] __attribute__((aligned(2)));
volatile uint16_t eagle_cd_files_loaded;

static uint16_t little16(const uint8_t *p)
{ return (uint16_t)(p[0] | ((uint16_t)p[1] << 8)); }
static uint32_t little32(const uint8_t *p)
{ return p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
static uint32_t big32(const uint8_t *p)
{ return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]; }
static uint8_t program_byte(uint32_t address)
{
#ifdef ECD_HOST_TEST
    return ECD_REG8(address);
#else
    uint8_t value;
    __asm__ volatile ("move.b (%1),%0" : "=d"(value) : "a"(address) : "memory");
    return value;
#endif
}
static int same(const char *a, const char *b)
{
    while (*a && *a == *b) { ++a; ++b; }
    return *a == *b;
}
static char upper(uint8_t c)
{ return (char)(c >= 'a' && c <= 'z' ? c - ('a' - 'A') : c); }
static int name_copy(char *out, const uint8_t *in, uint8_t length)
{
    uint8_t i, dot = 0, ext = 0;
    if (length > 2u && in[length-2u] == ';' && in[length-1u] == '1') length -= 2u;
    if (!length || length > 12u) return -22;
    for (i = 0; i < length; ++i) {
        char c = upper(in[i]);
        if (c == '.') { if (dot || !i || i > 8u) return -22; dot = i; }
        else {
            if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')) return -22;
            if (dot) ++ext;
        }
        out[i] = c;
    }
    out[length] = 0;
    return dot && ext && ext <= 3u ? 0 : -22;
}
static int find(const char *name)
{
    uint16_t i;
    for (i = 0; i < file_count; ++i) if (same(files[i].name, name)) return i;
    return -2;
}

int eagle_cd_mount(void)
{
    uint8_t *s = eagle_cd_sector_buffer;
    uint32_t root, size, sector;
    int result = eagle_cd_read_sector(16, s);
    if (result) return result;
    if (s[0] != 1 || s[1] != 'C' || s[2] != 'D' || s[3] != '0' ||
        s[4] != '0' || s[5] != '1' || s[6] != 1 || little16(s+128) != 2048u ||
        s[130] != 8 || s[131] != 0) return -22;
    volume_sectors = little32(s+80);
    if (volume_sectors != big32(s+84) || volume_sectors < 21u || volume_sectors > 450000ul) return -22;
    root = little32(s+158); size = little32(s+166);
    if (root != big32(s+162) || size != big32(s+170) || root < 18u ||
        !size || size > 65536ul || (size & 2047u) || root >= volume_sectors ||
        size/2048u > volume_sectors-root || !(s[181] & 2u)) return -22;
    file_count = 0;
    for (sector = 0; sector < size/2048u; ++sector) {
        uint16_t pos = 0;
        result = eagle_cd_read_sector(root+sector, s);
        if (result) return result;
        while (pos < 2048u && s[pos]) {
            const uint8_t *r = s+pos;
            uint16_t length = r[0];
            uint32_t lba, bytes;
            if (length < 34u || length > 2048u-pos || r[32] > length-33u || r[1] || r[26] || r[27]) return -22;
            if (r[32] == 1u && r[33] <= 1u) { pos += length; continue; }
            if (r[25] || file_count == 512u) return -22;
            lba = little32(r+2); bytes = little32(r+10);
            if (lba != big32(r+6) || bytes != big32(r+14) || !bytes ||
                lba >= volume_sectors || (bytes+2047ul)/2048ul > volume_sectors-lba ||
                bytes > 0x100000ul) return -22;
            if (name_copy(files[file_count].name, r+33, r[32])) return -22;
            if (find(files[file_count].name) >= 0) return -22;
            files[file_count].lba = lba; files[file_count].size = bytes;
            ++file_count; pos += length;
        }
        /* A zero record terminates the sector, not the directory. Remaining
         * bytes must be padding; never silently hide later file records. */
        while (pos < 2048u) if (s[pos++]) return -22;
    }
    eagle_cd_phase = 4;
    return 0;
}

static int validate_load(CDLoad *load, const char *name, uint8_t bank, uint32_t address)
{
    uint16_t i = 0;
    uint32_t capacity, alignment;
    int index = find(name);
    if (index < 0) return index;
    while (name[i] && name[i] != '.') ++i;
    if (same(name+i, ".PRG")) { load->zone = PRG; capacity = 0x100000ul; alignment = 2; }
    else if (same(name+i, ".SPR")) { load->zone = SPR; capacity = 0x100000ul; alignment = 128; }
    else if (same(name+i, ".FIX")) { load->zone = FIX; capacity = 0x20000ul; alignment = 32; }
    else if (same(name+i, ".Z80")) { load->zone = Z80; capacity = 0x10000ul; alignment = 2; }
    else if (same(name+i, ".PCM")) { load->zone = PCM; capacity = 0x80000ul; alignment = 256; }
    else return -22;
    if (bank >= (load->zone == SPR ? 4u : load->zone == PCM ? 2u : 1u)) return -34;
    if (load->zone >= FIX) { if (address & 1u) return -34; address >>= 1; }
    if (address > capacity || files[index].size > capacity-address ||
        (address & (alignment-1u)) || (files[index].size & (alignment-1u))) return -34;
    load->file = (uint16_t)index; load->bank = bank; load->offset = address;
    return 0;
}

static int load_file(const CDLoad *load)
{
    uint32_t remaining = files[load->file].size, offset = load->offset;
    uint32_t lba = files[load->file].lba;
    uint32_t release = 0;
    int result = eagle_cd_read_begin(lba);
    if (result) return result;
    if (load->zone != PRG) {
        static const uint8_t areas[5] = {0,0,5,4,1};
        static const uint32_t requests[5] = {0,0xFF0121,0xFF0129,0xFF0127,0xFF0123};
        release = requests[load->zone] + 0x20u;
        if (load->zone == Z80) ECD_REG8(0xFF0183) = 0;
        ECD_REG8(requests[load->zone]) = 0;
        ECD_REG8(0xFF0105) = areas[load->zone];
        if (load->zone == SPR) ECD_REG8(0xFF01A1) = load->bank;
        if (load->zone == PCM) ECD_REG8(0xFF01A3) = load->bank;
        ECD_REG8(0xFF016F) = 1;
    }
    while (remaining) {
        uint16_t i, bytes = (uint16_t)(remaining > 2048u ? 2048u : remaining);
        uint32_t target = load->zone == PRG ? offset : load->zone == SPR ?
                          0xE00000ul + offset : 0xE00001ul + offset*2u;
        uint16_t step = load->zone >= FIX ? 2u : 1u;
        result = eagle_cd_read_next(lba, eagle_cd_sector_buffer);
        if (result) break;
        for (i = 0; i < bytes; ++i, target += step) ECD_REG8(target) = eagle_cd_sector_buffer[i];
        remaining -= bytes; offset += bytes; ++lba;
    }
    if (release) { ECD_REG8(release) = 0; ECD_REG8(0xFF016F) = 0; }
    if (result) { (void)eagle_cd_drive_pause(); eagle_cd_decoder_enable(0); }
    if (!result) ++eagle_cd_files_loaded;
    return result;
}

int eagle_cd_load_ipl(void)
{
    int file = find("IPL.TXT"), result;
    uint16_t cursor = 0, length, mask = 0, resident = 0, pass;
    uint32_t resident_size = 0;
    if (file < 0) return file;
    if (files[file].size > sizeof(ipl)) return -34;
    length = (uint16_t)files[file].size;
    result = eagle_cd_read_sector(files[file].lba, eagle_cd_sector_buffer);
    if (result) return result;
    for (cursor = 0; cursor < length; ++cursor) ipl[cursor] = eagle_cd_sector_buffer[cursor];
    cursor = 0; boot_count = 0;
    while (cursor < length && ipl[cursor] != 0x1Au) {
        char name[14]; uint8_t n = 0, bank, digits = 0; uint32_t address = 0;
        uint16_t prior;
        if (boot_count == 32u) return -34;
        while (cursor < length && ipl[cursor] != ',') {
            if (n == 12u) return -22;
            name[n++] = upper(ipl[cursor++]);
        }
        name[n] = 0;
        if (!n || cursor+3u >= length || ipl[cursor++] != ',') return -22;
        bank = (uint8_t)(ipl[cursor++] - '0');
        if (bank > 9u || ipl[cursor++] != ',') return -22;
        while (cursor < length && ipl[cursor] != '\r') {
            uint8_t c = (uint8_t)upper(ipl[cursor++]), value;
            if (c >= '0' && c <= '9') value = c-'0';
            else if (c >= 'A' && c <= 'F') value = c-'A'+10u;
            else return -22;
            if (++digits > 8u) return -34;
            address = (address << 4) | value;
        }
        if (!digits || cursor+1u >= length || ipl[cursor++] != '\r' || ipl[cursor++] != '\n') return -22;
        result = validate_load(&boot[boot_count], name, bank, address);
        if (result) return result;
        for (prior = 0; prior < boot_count; ++prior) {
            const CDLoad *a = &boot[prior], *b = &boot[boot_count];
            if (a->zone == b->zone && a->bank == b->bank &&
                a->offset < b->offset+files[b->file].size && b->offset < a->offset+files[a->file].size) return -22;
        }
        mask |= (uint16_t)(1u << boot[boot_count].zone);
        if (boot[boot_count].zone == PRG) {
            if (address || resident) return -22;
            resident = (uint16_t)(files[boot[boot_count].file].size > 0x200u);
            resident_size = files[boot[boot_count].file].size;
        }
        ++boot_count;
    }
    if (!boot_count || !resident || mask != 31u || cursor+1u != length || ipl[cursor] != 0x1Au) return -22;
    eagle_cd_phase = 5;
    /* Keep the firmware font visible while other memories fill. FIX is a
     * separate non-overlapping zone, so its validated entries can load last. */
    for (pass = 0; pass < 2u; ++pass) {
        if (pass) ECD_REG8(0xFF0115) = 1;
        for (cursor = 0; cursor < boot_count; ++cursor) {
            if ((boot[cursor].zone == FIX) != (pass != 0)) continue;
            result = load_file(&boot[cursor]);
            if (result) return result;
        }
    }
    eagle_cd_drive_pause();
    for (cursor = 0; cursor < 7u; ++cursor)
        if (program_byte(0x100u+cursor) != (uint8_t)"NEO-GEO"[cursor]) return -22;
    {
        uint8_t target[4]; uint32_t entry;
        for (cursor = 0; cursor < 4u; ++cursor) target[cursor] = program_byte(0x124u+cursor);
        entry = big32(target);
        if (program_byte(0x122) != 0x4Eu || program_byte(0x123) != 0xF9u ||
            entry < 0x200u || entry >= resident_size || (entry & 1u)) return -22;
    }
    eagle_cd_phase = 6;
    return 0;
}

int32_t eagle_cd_load_list(const uint8_t *list)
{
    uint16_t count;
    /* Requests must reside in game DRAM. Never follow a caller pointer into
     * firmware, private loader memory or hardware I/O. */
    uintptr_t address = (uintptr_t)list;
    if (address >= 0x100000ul) return -34;
    for (count = 0; count < 32u; ++count) {
        char name[14]; uint8_t n = 0, bank; uint32_t offset; CDLoad load; int result;
        if ((uintptr_t)list >= 0x100000ul) return -34;
        if (!*list) {
            if (count) {
                int result = eagle_cd_drive_pause();
                eagle_cd_decoder_enable(0);
                return result;
            }
            return 0;
        }
        while ((uintptr_t)list < 0x100000ul && *list) {
            if (n == 12u) return -22;
            name[n++] = upper(*list++);
        }
        if ((uintptr_t)list+8u >= 0x100000ul) return -34;
        ++list; name[n] = 0; bank = *list++;
        if ((uintptr_t)list & 1u) ++list;
        offset = big32(list); list += 4;
        result = validate_load(&load, name, bank, offset);
        if (result) return result;
        if (load.zone == PRG) return -95; /* executable overlays are not supported */
        result = load_file(&load);
        if (result) return result;
    }
    return -34;
}
