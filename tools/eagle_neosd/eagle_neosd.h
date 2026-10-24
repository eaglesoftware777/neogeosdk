#ifndef EAGLE_NEOSD_H
#define EAGLE_NEOSD_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EAGLE_NEOSD_VERSION "1.0.0"

/* NeoSD File Format Constants */
#define NEO_MAGIC "NEO"
#define NEO_VERSION 1
#define NEO_HEADER_SIZE 4096
#define NEO_NAME_LEN 33
#define NEO_MANUFACTURER_LEN 17
#define NEO_RESERVED_SIZE 4002

/* Six ROM regions required by NeoSD */
enum {
    NEO_REG_P = 0,
    NEO_REG_S = 1,
    NEO_REG_M = 2,
    NEO_REG_V1 = 3,
    NEO_REG_V2 = 4,
    NEO_REG_C = 5,
    NEO_REGIONS_COUNT = 6
};

/* NeoSD Game Genres */
enum {
    NEO_GENRE_OTHER = 0,
    NEO_GENRE_ACTION = 1,
    NEO_GENRE_BEATEMUP = 2,
    NEO_GENRE_SPORTS = 3,
    NEO_GENRE_DRIVING = 4,
    NEO_GENRE_PLATFORMER = 5,
    NEO_GENRE_MAHJONG = 6,
    NEO_GENRE_SHOOTER = 7,
    NEO_GENRE_QUIZ = 8,
    NEO_GENRE_FIGHTING = 9,
    NEO_GENRE_PUZZLE = 10
};

#pragma pack(push, 1)
typedef struct {
    uint8_t  magic[3];                  /* 'N', 'E', 'O' */
    uint8_t  version;                   /* 0x01 */
    uint32_t p_size;                    /* 68000 Program ROM size in bytes */
    uint32_t s_size;                    /* S1 Fix Layer ROM size in bytes */
    uint32_t m_size;                    /* Z80 Sound CPU ROM size in bytes */
    uint32_t v1_size;                   /* ADPCM-A Audio ROM size in bytes */
    uint32_t v2_size;                   /* ADPCM-B Audio ROM size in bytes */
    uint32_t c_size;                    /* Interleaved Sprite C-ROM size */
    uint32_t year;                      /* Release Year */
    uint32_t genre;                     /* Genre ID (0..10) */
    uint32_t screenshot;                /* Screenshot index / reserved (0) */
    uint32_t ngh;                       /* NGH Game ID (e.g. 780, 777) */
    char     name[NEO_NAME_LEN];        /* Title string, ASCII null-padded */
    char     manufacturer[NEO_MANUFACTURER_LEN]; /* Studio name, ASCII null-padded */
    uint8_t  reserved[NEO_RESERVED_SIZE]; /* Zero padding to 4096 bytes */
} NeoSDHeader;
#pragma pack(pop)

/* Memory buffer for a single ROM region */
typedef struct {
    uint8_t *data;
    size_t   size;
    size_t   capacity;
} NeoSDBuffer;

/* Conversion parameters */
typedef struct {
    const char *name;
    const char *manufacturer;
    uint32_t    year;
    uint32_t    genre;
    uint32_t    ngh;
    int         swap_p;       /* -1: auto, 0: keep as-is, 1: force byte-swap */
    int         verbose;
} NeoSDOptions;

/* ROM set collection for packing */
typedef struct {
    NeoSDBuffer reg[NEO_REGIONS_COUNT];
    NeoSDOptions opt;
} NeoSDRomSet;

/* API Functions */
void neosd_header_init(NeoSDHeader *hdr);
const char *neosd_genre_to_string(uint32_t genre);
uint32_t neosd_string_to_genre(const char *name);

void neosd_buffer_init(NeoSDBuffer *buf);
void neosd_buffer_free(NeoSDBuffer *buf);
int  neosd_buffer_append(NeoSDBuffer *buf, const uint8_t *src, size_t len);
int  neosd_buffer_load_file(NeoSDBuffer *buf, const char *filepath);

int  neosd_interleave_c(NeoSDBuffer *c_out, const uint8_t *c1, size_t c1_len, const uint8_t *c2, size_t c2_len);
int  neosd_deinterleave_c(const uint8_t *c_in, size_t c_len, NeoSDBuffer *c1_out, NeoSDBuffer *c2_out);

int  neosd_pack_files(const char *out_path, const NeoSDRomSet *romset);
int  neosd_pack_directory(const char *dir_path, const char *out_path, const NeoSDOptions *opt);
int  neosd_unpack(const char *neo_path, const char *out_dir, int verbose);
int  neosd_inspect(const char *neo_path, FILE *out_stream);

#ifdef __cplusplus
}
#endif

#endif /* EAGLE_NEOSD_H */
