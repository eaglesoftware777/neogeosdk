/*
 * Eagle Software NeoSD ROM Packager & Unpacker
 * High-performance converter for Neo Geo MVS/AES ROM sets to TerraOnion NeoSD (.neo) format.
 *
 * Dedicated to Eagle Software releases and Neo Geo homebrew/commercial development.
 */

#include "eagle_neosd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <direct.h>
    #define mkdir_portable(path) _mkdir(path)
#else
    #include <dirent.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <unistd.h>
    #define mkdir_portable(path) mkdir(path, 0755)
#endif

/* ------------------------------------------------------------------ */
/*  CRC-32 Implementation (IEEE 802.3)                                */
/* ------------------------------------------------------------------ */
static uint32_t neosd_crc32(const uint8_t *data, size_t length)
{
    static uint32_t table[256];
    static int initialized = 0;
    uint32_t crc = 0xFFFFFFFFu;
    size_t i;

    if (!initialized) {
        uint32_t poly = 0xEDB88320u;
        int n, k;
        for (n = 0; n < 256; n++) {
            uint32_t c = (uint32_t)n;
            for (k = 0; k < 8; k++) {
                if (c & 1) c = poly ^ (c >> 1);
                else       c = c >> 1;
            }
            table[n] = c;
        }
        initialized = 1;
    }

    for (i = 0; i < length; i++) {
        crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

/* ------------------------------------------------------------------ */
/*  Little-Endian Integer Serialization                               */
/* ------------------------------------------------------------------ */
static void put_u32_le(uint8_t *p, uint32_t val)
{
    p[0] = (uint8_t)(val & 0xFF);
    p[1] = (uint8_t)((val >> 8) & 0xFF);
    p[2] = (uint8_t)((val >> 16) & 0xFF);
    p[3] = (uint8_t)((val >> 24) & 0xFF);
}

static uint32_t get_u32_le(const uint8_t *p)
{
    return ((uint32_t)p[0]) |
           (((uint32_t)p[1]) << 8) |
           (((uint32_t)p[2]) << 16) |
           (((uint32_t)p[3]) << 24);
}

/* ------------------------------------------------------------------ */
/*  Genre Translation                                                 */
/* ------------------------------------------------------------------ */
static const char *const genre_names[] = {
    "Other",      /* 0 */
    "Action",     /* 1 */
    "BeatEmUp",   /* 2 */
    "Sports",     /* 3 */
    "Driving",    /* 4 */
    "Platformer", /* 5 */
    "Mahjong",    /* 6 */
    "Shooter",    /* 7 */
    "Quiz",       /* 8 */
    "Fighting",   /* 9 */
    "Puzzle"      /* 10 */
};

const char *neosd_genre_to_string(uint32_t genre)
{
    if (genre < sizeof(genre_names) / sizeof(genre_names[0])) {
        return genre_names[genre];
    }
    return "Other";
}

static int neosd_streq_nocase(const char *a, const char *b)
{
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++;
        b++;
    }
    return *a == *b;
}

uint32_t neosd_string_to_genre(const char *name)
{
    size_t i;
    if (!name) return NEO_GENRE_OTHER;
    if (isdigit((unsigned char)name[0])) {
        uint32_t g = (uint32_t)atoi(name);
        return g <= 10 ? g : NEO_GENRE_OTHER;
    }
    for (i = 0; i < sizeof(genre_names) / sizeof(genre_names[0]); i++) {
        if (neosd_streq_nocase(name, genre_names[i])) return (uint32_t)i;
    }
    return NEO_GENRE_OTHER;
}

/* ------------------------------------------------------------------ */
/*  Buffer Management                                                 */
/* ------------------------------------------------------------------ */
void neosd_buffer_init(NeoSDBuffer *buf)
{
    if (!buf) return;
    buf->data = NULL;
    buf->size = 0;
    buf->capacity = 0;
}

void neosd_buffer_free(NeoSDBuffer *buf)
{
    if (!buf) return;
    if (buf->data) {
        free(buf->data);
        buf->data = NULL;
    }
    buf->size = 0;
    buf->capacity = 0;
}

int neosd_buffer_append(NeoSDBuffer *buf, const uint8_t *src, size_t len)
{
    if (!buf || !src || len == 0) return 0;
    if (buf->size + len > buf->capacity) {
        size_t new_cap = buf->capacity ? buf->capacity * 2 : 65536;
        while (new_cap < buf->size + len) new_cap *= 2;
        uint8_t *new_ptr = (uint8_t *)realloc(buf->data, new_cap);
        if (!new_ptr) return -1;
        buf->data = new_ptr;
        buf->capacity = new_cap;
    }
    memcpy(buf->data + buf->size, src, len);
    buf->size += len;
    return 0;
}

int neosd_buffer_load_file(NeoSDBuffer *buf, const char *filepath)
{
    FILE *f;
    long fsize;
    size_t read_bytes;

    if (!buf || !filepath) return -1;
    f = fopen(filepath, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize < 0) {
        fclose(f);
        return -1;
    }

    if (buf->capacity < (size_t)fsize) {
        uint8_t *new_ptr = (uint8_t *)realloc(buf->data, (size_t)fsize);
        if (!new_ptr && fsize > 0) {
            fclose(f);
            return -1;
        }
        buf->data = new_ptr;
        buf->capacity = (size_t)fsize;
    }

    read_bytes = fread(buf->data, 1, (size_t)fsize, f);
    fclose(f);
    buf->size = read_bytes;
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Sprite C-ROM Interleaving / De-interleaving                       */
/* ------------------------------------------------------------------ */
int neosd_interleave_c(NeoSDBuffer *c_out, const uint8_t *c1, size_t c1_len, const uint8_t *c2, size_t c2_len)
{
    size_t total_pairs;
    size_t out_len;
    size_t i;
    uint8_t *dst;

    if (!c_out || !c1 || !c2) return -1;
    total_pairs = (c1_len > c2_len) ? c1_len : c2_len;
    out_len = total_pairs * 2;

    if (c_out->capacity < c_out->size + out_len) {
        size_t new_cap = c_out->capacity ? c_out->capacity * 2 : out_len + 65536;
        while (new_cap < c_out->size + out_len) new_cap *= 2;
        uint8_t *new_ptr = (uint8_t *)realloc(c_out->data, new_cap);
        if (!new_ptr) return -1;
        c_out->data = new_ptr;
        c_out->capacity = new_cap;
    }

    dst = c_out->data + c_out->size;

    /* Interleave: C1 -> Even bytes (bitplanes 0,1), C2 -> Odd bytes (bitplanes 2,3) */
    for (i = 0; i < total_pairs; i++) {
        dst[2 * i]     = (i < c1_len) ? c1[i] : 0x00;
        dst[2 * i + 1] = (i < c2_len) ? c2[i] : 0x00;
    }

    c_out->size += out_len;
    return 0;
}

int neosd_deinterleave_c(const uint8_t *c_in, size_t c_len, NeoSDBuffer *c1_out, NeoSDBuffer *c2_out)
{
    size_t pairs;
    size_t i;

    if (!c_in || !c1_out || !c2_out) return -1;
    pairs = c_len / 2;

    neosd_buffer_free(c1_out);
    neosd_buffer_free(c2_out);

    c1_out->data = (uint8_t *)malloc(pairs);
    c2_out->data = (uint8_t *)malloc(pairs);
    if (!c1_out->data || !c2_out->data) {
        neosd_buffer_free(c1_out);
        neosd_buffer_free(c2_out);
        return -1;
    }

    c1_out->size = c1_out->capacity = pairs;
    c2_out->size = c2_out->capacity = pairs;

    for (i = 0; i < pairs; i++) {
        c1_out->data[i] = c_in[2 * i];
        c2_out->data[i] = c_in[2 * i + 1];
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Header Setup                                                      */
/* ------------------------------------------------------------------ */
void neosd_header_init(NeoSDHeader *hdr)
{
    if (!hdr) return;
    memset(hdr, 0, sizeof(NeoSDHeader));
    hdr->magic[0] = 'N';
    hdr->magic[1] = 'E';
    hdr->magic[2] = 'O';
    hdr->version  = 0x01;
    hdr->year     = 2026;
    hdr->genre    = NEO_GENRE_ACTION;
    hdr->ngh      = 780;
    strncpy(hdr->name, "Eagle Game", NEO_NAME_LEN - 1);
    strncpy(hdr->manufacturer, "Eagle Software", NEO_MANUFACTURER_LEN - 1);
}

/* ------------------------------------------------------------------ */
/*  Pack RomSet into .neo                                             */
/* ------------------------------------------------------------------ */
int neosd_pack_files(const char *out_path, const NeoSDRomSet *romset)
{
    uint8_t raw_hdr[NEO_HEADER_SIZE];
    FILE *out_file;
    size_t i;
    const char *reg_names[NEO_REGIONS_COUNT] = {
        "P-ROM (68000 Program)",
        "S-ROM (Fix Layer)",
        "M-ROM (Z80 Sound)",
        "V1-ROM (ADPCM-A)",
        "V2-ROM (ADPCM-B)",
        "C-ROM (Interleaved Sprites)"
    };

    if (!out_path || !romset) return -1;

    memset(raw_hdr, 0, NEO_HEADER_SIZE);
    raw_hdr[0] = 'N';
    raw_hdr[1] = 'E';
    raw_hdr[2] = 'O';
    raw_hdr[3] = 0x01;

    /* Sizes */
    put_u32_le(raw_hdr + 4,  (uint32_t)romset->reg[NEO_REG_P].size);
    put_u32_le(raw_hdr + 8,  (uint32_t)romset->reg[NEO_REG_S].size);
    put_u32_le(raw_hdr + 12, (uint32_t)romset->reg[NEO_REG_M].size);
    put_u32_le(raw_hdr + 16, (uint32_t)romset->reg[NEO_REG_V1].size);
    put_u32_le(raw_hdr + 20, (uint32_t)romset->reg[NEO_REG_V2].size);
    put_u32_le(raw_hdr + 24, (uint32_t)romset->reg[NEO_REG_C].size);

    /* Metadata */
    put_u32_le(raw_hdr + 28, romset->opt.year ? romset->opt.year : 2026);
    put_u32_le(raw_hdr + 32, romset->opt.genre);
    put_u32_le(raw_hdr + 36, 0); /* Screenshot */
    put_u32_le(raw_hdr + 40, romset->opt.ngh ? romset->opt.ngh : 780);

    /* Strings */
    if (romset->opt.name && strlen(romset->opt.name)) {
        strncpy((char *)(raw_hdr + 44), romset->opt.name, NEO_NAME_LEN - 1);
    } else {
        strncpy((char *)(raw_hdr + 44), "Eagle Software Game", NEO_NAME_LEN - 1);
    }

    if (romset->opt.manufacturer && strlen(romset->opt.manufacturer)) {
        strncpy((char *)(raw_hdr + 77), romset->opt.manufacturer, NEO_MANUFACTURER_LEN - 1);
    } else {
        strncpy((char *)(raw_hdr + 77), "Eagle Software", NEO_MANUFACTURER_LEN - 1);
    }

    out_file = fopen(out_path, "wb");
    if (!out_file) {
        fprintf(stderr, "Error: Could not open output file '%s' for writing.\n", out_path);
        return -1;
    }

    if (fwrite(raw_hdr, 1, NEO_HEADER_SIZE, out_file) != NEO_HEADER_SIZE) {
        fprintf(stderr, "Error: Failed to write NeoSD header.\n");
        fclose(out_file);
        return -1;
    }

    /* Print header summary */
    printf("\n========================================================\n");
    printf("  EAGLE SOFTWARE NEOSD PACKAGER v%s\n", EAGLE_NEOSD_VERSION);
    printf("========================================================\n");
    printf(" Output File   : %s\n", out_path);
    printf(" Game Name     : %s\n", (char *)(raw_hdr + 44));
    printf(" Manufacturer  : %s\n", (char *)(raw_hdr + 77));
    printf(" Release Year  : %u\n", get_u32_le(raw_hdr + 28));
    printf(" Genre         : %s (%u)\n", neosd_genre_to_string(get_u32_le(raw_hdr + 32)), get_u32_le(raw_hdr + 32));
    printf(" NGH ID        : 0x%04X (%u)\n", get_u32_le(raw_hdr + 40), get_u32_le(raw_hdr + 40));
    printf("--------------------------------------------------------\n");
    printf(" %-27s | %10s | %10s\n", "Region", "Size", "CRC-32");
    printf("--------------------------------------------------------\n");

    /* Write data regions */
    for (i = 0; i < NEO_REGIONS_COUNT; i++) {
        size_t sz = romset->reg[i].size;
        uint32_t crc = sz ? neosd_crc32(romset->reg[i].data, sz) : 0;
        printf(" %-27s | %8zu KB | 0x%08X\n", reg_names[i], sz / 1024, crc);
        if (sz > 0) {
            if (fwrite(romset->reg[i].data, 1, sz, out_file) != sz) {
                fprintf(stderr, "Error writing region %zu to output file.\n", i);
                fclose(out_file);
                return -1;
            }
        }
    }

    long total_size = ftell(out_file);
    fclose(out_file);

    printf("--------------------------------------------------------\n");
    printf(" Total .neo Size: %ld bytes (%.2f MB)\n", total_size, (double)total_size / (1024.0 * 1024.0));
    printf(" Status         : SUCCESSFULLY PACKED!\n");
    printf("========================================================\n\n");

    return 0;
}

/* ------------------------------------------------------------------ */
/*  Scan and pack directory of loose ROMs                             */
/* ------------------------------------------------------------------ */
static void detect_eagle_game_profile(const char *dir_name, NeoSDOptions *opt)
{
    if (!opt->name) {
        if (strstr(dir_name, "maiya") || strstr(dir_name, "780")) {
            opt->name = "Maiya: Super Nature Girl";
            opt->genre = NEO_GENRE_PLATFORMER;
            opt->ngh = 780;
            opt->year = 2026;
            opt->manufacturer = "Eagle Software";
        } else if (strstr(dir_name, "demo") || strstr(dir_name, "777")) {
            opt->name = "NeoGeo SDK Demo";
            opt->genre = NEO_GENRE_ACTION;
            opt->ngh = 777;
            opt->year = 2026;
            opt->manufacturer = "Eagle Software";
        } else {
            opt->name = "NeoGeo Arcade Game";
            opt->genre = NEO_GENRE_ACTION;
            opt->ngh = 780;
            opt->year = 2026;
            opt->manufacturer = "Eagle Software";
        }
    }
}

int neosd_pack_directory(const char *dir_path, const char *out_path, const NeoSDOptions *opt)
{
    NeoSDRomSet rset;
    NeoSDBuffer c1_buf, c2_buf;
    size_t i;
    char path_p1[512] = {0}, path_s1[512] = {0}, path_m1[512] = {0};
    char path_v1[512] = {0}, path_v2[512] = {0};
    char path_c1[512] = {0}, path_c2[512] = {0}, path_c_full[512] = {0};
    int found_c1 = 0, found_c2 = 0;

    for (i = 0; i < NEO_REGIONS_COUNT; i++) neosd_buffer_init(&rset.reg[i]);
    neosd_buffer_init(&c1_buf);
    neosd_buffer_init(&c2_buf);

    if (opt) rset.opt = *opt;
    else memset(&rset.opt, 0, sizeof(NeoSDOptions));

    detect_eagle_game_profile(dir_path, &rset.opt);

#if defined(_WIN32) || defined(_WIN64)
    WIN32_FIND_DATAA fd;
    char search_pattern[512];
    snprintf(search_pattern, sizeof(search_pattern), "%s\\*.*", dir_path);
    HANDLE hFind = FindFirstFileA(search_pattern, &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error: Could not open directory '%s'\n", dir_path);
        return -1;
    }
    do {
        const char *fn = fd.cFileName;
#else
    DIR *d = opendir(dir_path);
    struct dirent *dir;
    if (!d) {
        fprintf(stderr, "Error: Could not open directory '%s'\n", dir_path);
        return -1;
    }
    while ((dir = readdir(d)) != NULL) {
        const char *fn = dir->d_name;
#endif
        char full[512];
        snprintf(full, sizeof(full), "%s/%s", dir_path, fn);

        if (strstr(fn, "-p1.p1") || strstr(fn, "-p1.rom") || strstr(fn, ".p1")) {
            if (!path_p1[0]) snprintf(path_p1, sizeof(path_p1), "%s", full);
        } else if (strstr(fn, "-s1.s1") || strstr(fn, "-s1.rom") || strstr(fn, ".s1")) {
            if (!path_s1[0]) snprintf(path_s1, sizeof(path_s1), "%s", full);
        } else if (strstr(fn, "-m1.m1") || strstr(fn, "-m1.rom") || strstr(fn, ".m1")) {
            if (!path_m1[0]) snprintf(path_m1, sizeof(path_m1), "%s", full);
        } else if (strstr(fn, "-v1.v1") || strstr(fn, "-v1.rom") || strstr(fn, ".v1")) {
            if (!path_v1[0]) snprintf(path_v1, sizeof(path_v1), "%s", full);
        } else if (strstr(fn, "-v2.v2") || strstr(fn, "-v2.rom") || strstr(fn, ".v2")) {
            if (!path_v2[0]) snprintf(path_v2, sizeof(path_v2), "%s", full);
        } else if (strstr(fn, "-c1.c1") || strstr(fn, "-c1.rom") || strstr(fn, ".c1")) {
            snprintf(path_c1, sizeof(path_c1), "%s", full);
            found_c1 = 1;
        } else if (strstr(fn, "-c2.c2") || strstr(fn, "-c2.rom") || strstr(fn, ".c2")) {
            snprintf(path_c2, sizeof(path_c2), "%s", full);
            found_c2 = 1;
        } else if ((strstr(fn, ".c") || strstr(fn, ".crom")) && !strstr(fn, ".c1") && !strstr(fn, ".c2")) {
            snprintf(path_c_full, sizeof(path_c_full), "%s", full);
        }
#if defined(_WIN32) || defined(_WIN64)
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);
#else
    }
    closedir(d);
#endif

    /* Load P-ROM */
    if (path_p1[0]) {
        printf(" Loading P-ROM : %s\n", path_p1);
        neosd_buffer_load_file(&rset.reg[NEO_REG_P], path_p1);

        /* Check NGH in P-ROM header at offset 0x108 if not explicitly set */
        if (!opt || opt->ngh == 0) {
            if (rset.reg[NEO_REG_P].size > 0x10A) {
                uint8_t *b = rset.reg[NEO_REG_P].data;
                /* If byte-swapped MAME format */
                uint16_t detected_ngh = ((uint16_t)b[0x108] << 8) | b[0x109];
                if (detected_ngh != 0) rset.opt.ngh = detected_ngh;
            }
        }
    } else {
        fprintf(stderr, "Warning: No P-ROM found in '%s'\n", dir_path);
    }

    /* Load S-ROM */
    if (path_s1[0]) {
        printf(" Loading S-ROM : %s\n", path_s1);
        neosd_buffer_load_file(&rset.reg[NEO_REG_S], path_s1);
    }

    /* Load M-ROM */
    if (path_m1[0]) {
        printf(" Loading M-ROM : %s\n", path_m1);
        neosd_buffer_load_file(&rset.reg[NEO_REG_M], path_m1);
    }

    /* Load V1-ROM */
    if (path_v1[0]) {
        printf(" Loading V1-ROM: %s\n", path_v1);
        neosd_buffer_load_file(&rset.reg[NEO_REG_V1], path_v1);
    }

    /* Load V2-ROM */
    if (path_v2[0]) {
        printf(" Loading V2-ROM: %s\n", path_v2);
        neosd_buffer_load_file(&rset.reg[NEO_REG_V2], path_v2);
    }

    /* Load C-ROM */
    if (found_c1 && found_c2) {
        printf(" Interleaving C1 & C2:\n");
        printf("   C1 (Even): %s\n", path_c1);
        printf("   C2 (Odd) : %s\n", path_c2);
        neosd_buffer_load_file(&c1_buf, path_c1);
        neosd_buffer_load_file(&c2_buf, path_c2);
        neosd_interleave_c(&rset.reg[NEO_REG_C], c1_buf.data, c1_buf.size, c2_buf.data, c2_buf.size);
        printf("   Interleaved C-ROM Size: %zu KB\n", rset.reg[NEO_REG_C].size / 1024);
    } else if (path_c_full[0]) {
        printf(" Loading pre-interleaved C-ROM: %s\n", path_c_full);
        neosd_buffer_load_file(&rset.reg[NEO_REG_C], path_c_full);
    } else {
        fprintf(stderr, "Warning: No C-ROM found in '%s'\n", dir_path);
    }

    int res = neosd_pack_files(out_path, &rset);

    for (i = 0; i < NEO_REGIONS_COUNT; i++) neosd_buffer_free(&rset.reg[i]);
    neosd_buffer_free(&c1_buf);
    neosd_buffer_free(&c2_buf);

    return res;
}

/* ------------------------------------------------------------------ */
/*  Inspect .neo file                                                 */
/* ------------------------------------------------------------------ */
int neosd_inspect(const char *neo_path, FILE *out_stream)
{
    FILE *f;
    uint8_t hdr[NEO_HEADER_SIZE];
    uint32_t psz, ssz, msz, v1sz, v2sz, csz;
    uint32_t year, genre, screenshot, ngh;
    char name[NEO_NAME_LEN] = {0};
    char manu[NEO_MANUFACTURER_LEN] = {0};
    long fsize;

    if (!out_stream) out_stream = stdout;

    f = fopen(neo_path, "rb");
    if (!f) {
        fprintf(stderr, "Error: Could not open file '%s'\n", neo_path);
        return -1;
    }

    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize < NEO_HEADER_SIZE) {
        fprintf(stderr, "Error: File too small to be a valid .neo file.\n");
        fclose(f);
        return -1;
    }

    if (fread(hdr, 1, NEO_HEADER_SIZE, f) != NEO_HEADER_SIZE) {
        fprintf(stderr, "Error: Could not read .neo header.\n");
        fclose(f);
        return -1;
    }

    fclose(f);

    if (memcmp(hdr, "NEO", 3) != 0) {
        fprintf(stderr, "Error: Missing 'NEO' magic signature at byte 0.\n");
        return -1;
    }

    psz  = get_u32_le(hdr + 4);
    ssz  = get_u32_le(hdr + 8);
    msz  = get_u32_le(hdr + 12);
    v1sz = get_u32_le(hdr + 16);
    v2sz = get_u32_le(hdr + 20);
    csz  = get_u32_le(hdr + 24);

    year       = get_u32_le(hdr + 28);
    genre      = get_u32_le(hdr + 32);
    screenshot = get_u32_le(hdr + 36);
    ngh        = get_u32_le(hdr + 40);

    memcpy(name, hdr + 44, NEO_NAME_LEN - 1);
    memcpy(manu, hdr + 77, NEO_MANUFACTURER_LEN - 1);

    fprintf(out_stream, "\n========================================================\n");
    fprintf(out_stream, "  EAGLE SOFTWARE NEOSD INSPECTOR\n");
    fprintf(out_stream, "========================================================\n");
    fprintf(out_stream, " File Name     : %s\n", neo_path);
    fprintf(out_stream, " File Size     : %ld bytes (%.2f MB)\n", fsize, (double)fsize / (1024.0 * 1024.0));
    fprintf(out_stream, " NeoSD Version : %u\n", hdr[3]);
    fprintf(out_stream, " Game Title    : %s\n", name);
    fprintf(out_stream, " Manufacturer  : %s\n", manu);
    fprintf(out_stream, " Release Year  : %u\n", year);
    fprintf(out_stream, " Genre         : %s (%u)\n", neosd_genre_to_string(genre), genre);
    fprintf(out_stream, " NGH Game ID   : 0x%04X (%u)\n", ngh, ngh);
    fprintf(out_stream, " Screenshot ID : %u\n", screenshot);
    fprintf(out_stream, "--------------------------------------------------------\n");
    fprintf(out_stream, " %-18s | %12s | %16s\n", "ROM Region", "Size", "File Offset");
    fprintf(out_stream, "--------------------------------------------------------\n");
    uint32_t off = NEO_HEADER_SIZE;
    fprintf(out_stream, " %-18s | %10u B | 0x%08X\n", "P-ROM (68000)", psz, off); off += psz;
    fprintf(out_stream, " %-18s | %10u B | 0x%08X\n", "S-ROM (Fix)", ssz, off); off += ssz;
    fprintf(out_stream, " %-18s | %10u B | 0x%08X\n", "M-ROM (Z80)", msz, off); off += msz;
    fprintf(out_stream, " %-18s | %10u B | 0x%08X\n", "V1-ROM (ADPCM-A)", v1sz, off); off += v1sz;
    fprintf(out_stream, " %-18s | %10u B | 0x%08X\n", "V2-ROM (ADPCM-B)", v2sz, off); off += v2sz;
    fprintf(out_stream, " %-18s | %10u B | 0x%08X\n", "C-ROM (Sprites)", csz, off); off += csz;
    fprintf(out_stream, "--------------------------------------------------------\n");
    fprintf(out_stream, " Expected Size : %u bytes (%s)\n", off, (off == (uint32_t)fsize) ? "VALID MATCH" : "SIZE MISMATCH");
    fprintf(out_stream, "========================================================\n\n");

    return 0;
}

/* ------------------------------------------------------------------ */
/*  Unpack .neo to directory                                          */
/* ------------------------------------------------------------------ */
int neosd_unpack(const char *neo_path, const char *out_dir, int verbose)
{
    FILE *f;
    uint8_t hdr[NEO_HEADER_SIZE];
    uint32_t psz, ssz, msz, v1sz, v2sz, csz, ngh;
    char path[512];
    uint8_t *temp_c;

    if (!neo_path || !out_dir) return -1;
    mkdir_portable(out_dir);

    f = fopen(neo_path, "rb");
    if (!f) {
        fprintf(stderr, "Error opening '%s'\n", neo_path);
        return -1;
    }

    if (fread(hdr, 1, NEO_HEADER_SIZE, f) != NEO_HEADER_SIZE || memcmp(hdr, "NEO", 3) != 0) {
        fprintf(stderr, "Error: Invalid .neo file header.\n");
        fclose(f);
        return -1;
    }

    psz  = get_u32_le(hdr + 4);
    ssz  = get_u32_le(hdr + 8);
    msz  = get_u32_le(hdr + 12);
    v1sz = get_u32_le(hdr + 16);
    v2sz = get_u32_le(hdr + 20);
    csz  = get_u32_le(hdr + 24);
    ngh  = get_u32_le(hdr + 40);

    if (verbose) {
        neosd_inspect(neo_path, stdout);
        printf("Extracting ROMs to: %s\n", out_dir);
    }

    /* Extract P-ROM */
    if (psz > 0) {
        uint8_t *buf = (uint8_t *)malloc(psz);
        if (buf && fread(buf, 1, psz, f) == psz) {
            snprintf(path, sizeof(path), "%s/%u-p1.p1", out_dir, ngh);
            FILE *o = fopen(path, "wb");
            if (o) { fwrite(buf, 1, psz, o); fclose(o); }
            if (verbose) printf(" -> Extracted %s (%u KB)\n", path, psz / 1024);
        }
        free(buf);
    }

    /* Extract S-ROM */
    if (ssz > 0) {
        uint8_t *buf = (uint8_t *)malloc(ssz);
        if (buf && fread(buf, 1, ssz, f) == ssz) {
            snprintf(path, sizeof(path), "%s/%u-s1.s1", out_dir, ngh);
            FILE *o = fopen(path, "wb");
            if (o) { fwrite(buf, 1, ssz, o); fclose(o); }
            if (verbose) printf(" -> Extracted %s (%u KB)\n", path, ssz / 1024);
        }
        free(buf);
    }

    /* Extract M-ROM */
    if (msz > 0) {
        uint8_t *buf = (uint8_t *)malloc(msz);
        if (buf && fread(buf, 1, msz, f) == msz) {
            snprintf(path, sizeof(path), "%s/%u-m1.m1", out_dir, ngh);
            FILE *o = fopen(path, "wb");
            if (o) { fwrite(buf, 1, msz, o); fclose(o); }
            if (verbose) printf(" -> Extracted %s (%u KB)\n", path, msz / 1024);
        }
        free(buf);
    }

    /* Extract V1-ROM */
    if (v1sz > 0) {
        uint8_t *buf = (uint8_t *)malloc(v1sz);
        if (buf && fread(buf, 1, v1sz, f) == v1sz) {
            snprintf(path, sizeof(path), "%s/%u-v1.v1", out_dir, ngh);
            FILE *o = fopen(path, "wb");
            if (o) { fwrite(buf, 1, v1sz, o); fclose(o); }
            if (verbose) printf(" -> Extracted %s (%u KB)\n", path, v1sz / 1024);
        }
        free(buf);
    }

    /* Extract V2-ROM */
    if (v2sz > 0) {
        uint8_t *buf = (uint8_t *)malloc(v2sz);
        if (buf && fread(buf, 1, v2sz, f) == v2sz) {
            snprintf(path, sizeof(path), "%s/%u-v2.v2", out_dir, ngh);
            FILE *o = fopen(path, "wb");
            if (o) { fwrite(buf, 1, v2sz, o); fclose(o); }
            if (verbose) printf(" -> Extracted %s (%u KB)\n", path, v2sz / 1024);
        }
        free(buf);
    }

    /* De-interleave C-ROM into c1 and c2 */
    if (csz > 0) {
        temp_c = (uint8_t *)malloc(csz);
        if (temp_c && fread(temp_c, 1, csz, f) == csz) {
            NeoSDBuffer c1, c2;
            neosd_buffer_init(&c1);
            neosd_buffer_init(&c2);
            neosd_deinterleave_c(temp_c, csz, &c1, &c2);

            snprintf(path, sizeof(path), "%s/%u-c1.c1", out_dir, ngh);
            FILE *o1 = fopen(path, "wb");
            if (o1) { fwrite(c1.data, 1, c1.size, o1); fclose(o1); }
            if (verbose) printf(" -> Extracted & De-interleaved %s (%zu KB)\n", path, c1.size / 1024);

            snprintf(path, sizeof(path), "%s/%u-c2.c2", out_dir, ngh);
            FILE *o2 = fopen(path, "wb");
            if (o2) { fwrite(c2.data, 1, c2.size, o2); fclose(o2); }
            if (verbose) printf(" -> Extracted & De-interleaved %s (%zu KB)\n", path, c2.size / 1024);

            neosd_buffer_free(&c1);
            neosd_buffer_free(&c2);
        }
        free(temp_c);
    }

    fclose(f);
    printf("Unpack complete! Output written to: %s\n\n", out_dir);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Main CLI Driver                                                   */
/* ------------------------------------------------------------------ */
static void print_usage(const char *prog)
{
    printf("========================================================\n");
    printf("  Eagle Software NeoSD ROM Packager v%s\n", EAGLE_NEOSD_VERSION);
    printf("========================================================\n");
    printf("Usage: %s [options]\n\n", prog);
    printf("Modes:\n");
    printf("  Pack Directory to .neo:\n");
    printf("    %s -i <rom_dir> -o <game.neo> [options]\n", prog);
    printf("    Example: %s -i roms/maiya -o maiya.neo -n \"Maiya: Super Nature Girl\"\n\n", prog);
    printf("  Inspect .neo File:\n");
    printf("    %s --info <game.neo>\n\n", prog);
    printf("  Extract / Unpack .neo to ROMs:\n");
    printf("    %s -x <game.neo> -o <out_dir>\n\n", prog);
    printf("Options:\n");
    printf("  -i, --input <path>       Input directory of ROMs or .neo file\n");
    printf("  -o, --output <path>      Output .neo file path or extraction directory\n");
    printf("  -n, --name <title>       Game Title (up to 32 chars)\n");
    printf("  -m, --manu <name>        Manufacturer name (default: Eagle Software)\n");
    printf("  -y, --year <year>        Release year (default: 2026)\n");
    printf("  -g, --genre <genre>      Genre: Action, Platformer, BeatEmUp, Shooter, Fighting, etc.\n");
    printf("      --ngh <id>           NGH Game ID (e.g. 780, 777)\n");
    printf("  -v, --info               Inspect and print .neo header information\n");
    printf("  -x, --extract            Extract .neo file into loose ROM files with C de-interleaving\n");
    printf("  -h, --help               Show this help message\n");
    printf("========================================================\n");
}

int main(int argc, char **argv)
{
    const char *input_path = NULL;
    const char *output_path = NULL;
    NeoSDOptions opt;
    int mode_extract = 0;
    int mode_info = 0;
    int i;

    memset(&opt, 0, sizeof(NeoSDOptions));
    opt.year = 2026;
    opt.genre = NEO_GENRE_ACTION;
    opt.manufacturer = "Eagle Software";

    if (argc < 2) {
        print_usage(argv[0]);
        return 0;
    }

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--input") == 0) {
            if (i + 1 < argc) input_path = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) output_path = argv[++i];
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--name") == 0) {
            if (i + 1 < argc) opt.name = argv[++i];
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--manu") == 0) {
            if (i + 1 < argc) opt.manufacturer = argv[++i];
        } else if (strcmp(argv[i], "-y") == 0 || strcmp(argv[i], "--year") == 0) {
            if (i + 1 < argc) opt.year = (uint32_t)atoi(argv[++i]);
        } else if (strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--genre") == 0) {
            if (i + 1 < argc) opt.genre = neosd_string_to_genre(argv[++i]);
        } else if (strcmp(argv[i], "--ngh") == 0) {
            if (i + 1 < argc) opt.ngh = (uint32_t)strtoul(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--info") == 0) {
            mode_info = 1;
            if (i + 1 < argc && argv[i + 1][0] != '-') input_path = argv[++i];
        } else if (strcmp(argv[i], "-x") == 0 || strcmp(argv[i], "--extract") == 0) {
            mode_extract = 1;
            if (i + 1 < argc && argv[i + 1][0] != '-') input_path = argv[++i];
        } else if (!input_path && argv[i][0] != '-') {
            input_path = argv[i];
        }
    }

    if (mode_info) {
        if (!input_path) {
            fprintf(stderr, "Error: Specify a .neo file to inspect.\n");
            return 1;
        }
        return neosd_inspect(input_path, stdout);
    }

    if (mode_extract) {
        if (!input_path) {
            fprintf(stderr, "Error: Specify a .neo file to extract.\n");
            return 1;
        }
        if (!output_path) output_path = "extracted_roms";
        return neosd_unpack(input_path, output_path, 1);
    }

    if (!input_path) {
        fprintf(stderr, "Error: No input directory or .neo file specified.\n");
        print_usage(argv[0]);
        return 1;
    }

    if (!output_path) {
        static char def_out[512];
        snprintf(def_out, sizeof(def_out), "%s.neo", input_path);
        output_path = def_out;
    }

    return neosd_pack_directory(input_path, output_path, &opt);
}
