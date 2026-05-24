#ifndef SOUND_IDS_H
#define SOUND_IDS_H

/*
 * Sound track IDs — MUST match the leading integer in each MML / SSG
 * filename under games/<game>/sound/.  All filenames follow a fully
 * generic pattern (N_<kind>_example_<letter>.mml) so the canonical
 * SDK identifier is the LETTER suffix.  Indexed TRACK_N aliases
 * (1-based) are provided for code that prefers numeric track refs.
 */

/* ---------------------------------------------------------------- */
/*  MUSIC tracks — games/<game>/sound/mml/<N>_mml_example_<x>.mml    */
/* ---------------------------------------------------------------- */
enum {
	SOUND_MUSIC_A = 0,  /* 0_mml_example_a.mml */
	SOUND_MUSIC_B = 1,  /* 1_mml_example_b.mml */
	SOUND_MUSIC_C = 2,  /* 2_mml_example_c.mml */
	SOUND_MUSIC_D = 3,  /* 3_mml_example_d.mml */
	SOUND_MUSIC_E = 4,  /* 4_mml_example_e.mml */
	SOUND_MUSIC_F = 5,  /* 5_mml_example_f.mml */
	SOUND_MUSIC_G = 6,  /* 6_mml_example_g.mml */
	SOUND_MUSIC_H = 7   /* 7_mml_example_h.mml */
};

#define SOUND_MUSIC_TRACK_1  SOUND_MUSIC_A
#define SOUND_MUSIC_TRACK_2  SOUND_MUSIC_B
#define SOUND_MUSIC_TRACK_3  SOUND_MUSIC_C
#define SOUND_MUSIC_TRACK_4  SOUND_MUSIC_D
#define SOUND_MUSIC_TRACK_5  SOUND_MUSIC_E
#define SOUND_MUSIC_TRACK_6  SOUND_MUSIC_F
#define SOUND_MUSIC_TRACK_7  SOUND_MUSIC_G
#define SOUND_MUSIC_TRACK_8  SOUND_MUSIC_H
#define SOUND_MUSIC_TRACK_COUNT  8

/* ---------------------------------------------------------------- */
/*  SFX (ADPCM-A short triggers) — sound/samples/in_wav_a/<N+1>.wav  */
/* ---------------------------------------------------------------- */
enum {
	SOUND_SFX_1  = 0,    /*  1.wav */
	SOUND_SFX_2  = 1,    /*  2.wav */
	SOUND_SFX_3  = 2,    /*  3.wav */
	SOUND_SFX_4  = 3,    /*  4.wav */
	SOUND_SFX_5  = 4,    /*  5.wav */
	SOUND_SFX_6  = 5,    /*  6.wav */
	SOUND_SFX_7  = 6,    /*  7.wav */
	SOUND_SFX_8  = 7,    /*  8.wav */
	SOUND_SFX_9  = 8,    /*  9.wav */
	SOUND_SFX_10 = 9,    /* 10.wav */
	SOUND_SFX_11 = 10,   /* 11.wav */
	SOUND_SFX_12 = 11    /* 12.wav */
};
#define SOUND_SFX_COUNT  12

/* ---------------------------------------------------------------- */
/*  TRACKs (ADPCM-B streams) — sound/samples/in_wav_b/<N+1>.wav        */
/*                                                                   */
/*  TRACK N corresponds to file (N+1).wav.  TRACK 4 (5.wav) is RESERVED  */
/*  for the eyecatcher screen — soundPlayGameLoop never selects it.  */
/* ---------------------------------------------------------------- */
enum {
	SOUND_TRACK_A = 0,    /* 1.wav */
	SOUND_TRACK_B = 1,    /* 2.wav */
	SOUND_TRACK_C = 2,    /* 3.wav */
	SOUND_TRACK_D = 3,    /* 4.wav */
	SOUND_TRACK_E = 4,    /* 5.wav — RESERVED (eyecatcher) */
	SOUND_TRACK_F = 5,    /* 6.wav */
	SOUND_TRACK_G = 6,    /* 7.wav */
	SOUND_TRACK_H = 7,    /* 8.wav */
	SOUND_TRACK_I = 8     /* 9.wav */
};

#define SOUND_TRACK_COUNT       9
#define SOUND_TRACK_FIRST_FREE  0
#define SOUND_TRACK_LAST_FREE   8

/* ---------------------------------------------------------------- */
/*  FM patches — games/<game>/sound/fm/<N>_fm_example_<x>.mml        */
/* ---------------------------------------------------------------- */
enum {
	SOUND_FM_A = 0,    /* 0_fm_example_a.mml */
	SOUND_FM_B = 1,    /* 1_fm_example_b.mml */
	SOUND_FM_C = 2,    /* 2_fm_example_c.mml */
	SOUND_FM_D = 3,    /* 3_fm_example_d.mml */
	SOUND_FM_E = 4,    /* 4_fm_example_e.mml */
	SOUND_FM_F = 5,    /* 5_fm_example_f.mml */
	SOUND_FM_G = 6,    /* 6_fm_example_g.mml */
	SOUND_FM_H = 7     /* 7_fm_example_h.mml */
};

#define SOUND_FM_TRACK_1  SOUND_FM_A
#define SOUND_FM_TRACK_2  SOUND_FM_B
#define SOUND_FM_TRACK_3  SOUND_FM_C
#define SOUND_FM_TRACK_4  SOUND_FM_D
#define SOUND_FM_TRACK_5  SOUND_FM_E
#define SOUND_FM_TRACK_6  SOUND_FM_F
#define SOUND_FM_TRACK_7  SOUND_FM_G
#define SOUND_FM_TRACK_8  SOUND_FM_H
#define SOUND_FM_TRACK_COUNT  8

/* ---------------------------------------------------------------- */
/*  SSG tracks — games/<game>/sound/ssg/<N>_ssg_example_<x>.mml      */
/* ---------------------------------------------------------------- */
enum {
	SOUND_SSG_A = 0,    /* 0_ssg_example_a.mml */
	SOUND_SSG_B = 1,    /* 1_ssg_example_b.mml */
	SOUND_SSG_C = 2,    /* 2_ssg_example_c.mml */
	SOUND_SSG_D = 3     /* 3_ssg_example_d.mml */
};

#define SOUND_SSG_TRACK_1  SOUND_SSG_A
#define SOUND_SSG_TRACK_2  SOUND_SSG_B
#define SOUND_SSG_TRACK_3  SOUND_SSG_C
#define SOUND_SSG_TRACK_4  SOUND_SSG_D
#define SOUND_SSG_TRACK_COUNT  4

/* ---------------------------------------------------------------- */
/*  Voice cues (ADPCM-A long voice samples)                          */
/* ---------------------------------------------------------------- */
enum {
	SOUND_VOICE_1 = 1,
	SOUND_VOICE_2 = 2
};

#endif
