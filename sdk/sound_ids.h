#ifndef SOUND_IDS_H
#define SOUND_IDS_H

/*
 * Sound track IDs — these MUST match the numeric prefix on each
 * music/SSG/FM data file under games/<game>/sound/.  Files are picked
 * up by the build pipeline using the leading integer in the filename,
 * so the enum below is the SDK-side name for that index.
 *
 * If you add a new track, save it as N_name.mml where N is the next
 * free index, and add the matching constant here at value N.
 */

/* ---------------------------------------------------------------- */
/*  MUSIC tracks — games/<game>/sound/mml/<N>_mml_example_*.mml      */
/* ---------------------------------------------------------------- */
enum {
	SOUND_MUSIC_SAMURAI_GAME_LOOP    = 0,  /* 0_mml_example_a.mml */
	SOUND_MUSIC_SAMURAI_BATTLE_LOOP  = 1,  /* 1_mml_example_b.mml */
	SOUND_MUSIC_SAMURAI_NIGHT_SCENE  = 2,  /* 2_mml_example_c.mml */
	SOUND_MUSIC_WARRIOR_BATTLE       = 3,  /* 3_mml_example_d.mml */
	SOUND_MUSIC_EAGLE_FANFARE        = 4,  /* 4_mml_example_e.mml */
	SOUND_MUSIC_BOSS_TENSION         = 5,  /* 5_mml_example_f.mml */
	SOUND_MUSIC_SHOP_JINGLE          = 6,  /* 6_mml_example_g.mml */
	SOUND_MUSIC_ENDING_CREDITS       = 7   /* 7_mml_example_h.mml */
};

/* Legacy alias kept for older demo code that still references the
 * old name — points to the same audio data as SAMURAI_NIGHT_SCENE. */
#define SOUND_MUSIC_SAMURAI_ENDING_SCENE  SOUND_MUSIC_SAMURAI_NIGHT_SCENE

/*
 * Generic track IDs — TRACK_1..TRACK_8 are friendlier when you just
 * want "the Nth music track" without remembering which samurai bed it
 * is.  Indices are 1-based here to match human counting and the
 * future filename convention (mml_1.mml, mml_2.mml ...).
 */
#define SOUND_MUSIC_TRACK_1  SOUND_MUSIC_SAMURAI_GAME_LOOP
#define SOUND_MUSIC_TRACK_2  SOUND_MUSIC_SAMURAI_BATTLE_LOOP
#define SOUND_MUSIC_TRACK_3  SOUND_MUSIC_SAMURAI_NIGHT_SCENE
#define SOUND_MUSIC_TRACK_4  SOUND_MUSIC_WARRIOR_BATTLE
#define SOUND_MUSIC_TRACK_5  SOUND_MUSIC_EAGLE_FANFARE
#define SOUND_MUSIC_TRACK_6  SOUND_MUSIC_BOSS_TENSION
#define SOUND_MUSIC_TRACK_7  SOUND_MUSIC_SHOP_JINGLE
#define SOUND_MUSIC_TRACK_8  SOUND_MUSIC_ENDING_CREDITS
#define SOUND_MUSIC_TRACK_COUNT 8

/* ---------------------------------------------------------------- */
/*  SFX (ADPCM-A short triggers) — sound/samples/in_wav_a/<N+1>.wav  */
/* ---------------------------------------------------------------- */
enum {
	SOUND_SFX_COIN_CHIME    = 0,
	SOUND_SFX_START_SLASH   = 1,
	SOUND_SFX_TITLE_GONG    = 2,
	SOUND_SFX_INTRO_TAIKO   = 3,
	SOUND_SFX_FOOTSTEP      = 4,
	SOUND_SFX_SHORT_SHOUT   = 5,
	SOUND_SFX_BLADE_WHOOSH  = 6,
	SOUND_SFX_IMPACT_HIT    = 7,
	SOUND_SFX_STRING_PHRASE = 8,
	SOUND_SFX_LOW_DRUM      = 9,
	SOUND_SFX_READY_VOICE   = 10,
	SOUND_SFX_ATTACK_VOICE  = 11
};

/* ---------------------------------------------------------------- */
/*  Beds (ADPCM-B streams) — sound/samples/in_wav_b/<N+1>.wav        */
/* ---------------------------------------------------------------- */
/*
 * Bed N corresponds to file (N+1).wav in sound/samples/in_wav_b/.
 * The first 5 beds have semantic names; beds 5..8 are extra slots
 * for scene-music rotation (added when 6.wav..9.wav were dropped in).
 *
 * SOUND_BED_EYECATCHER (4 → 5.wav) is RESERVED for the eyecatcher
 * scene — soundPlayGameLoop excludes it from its rotation pool.
 */
enum {
	SOUND_BED_TITLE_THEME  = 0,   /* 1.wav */
	SOUND_BED_STAGE_ONE    = 1,   /* 2.wav */
	SOUND_BED_STAGE_TWO    = 2,   /* 3.wav */
	SOUND_BED_ENDING_THEME = 3,   /* 4.wav */
	SOUND_BED_EYECATCHER   = 4,   /* 5.wav (reserved) */
	SOUND_BED_SCENE_F      = 5,   /* 6.wav */
	SOUND_BED_SCENE_G      = 6,   /* 7.wav */
	SOUND_BED_SCENE_H      = 7,   /* 8.wav */
	SOUND_BED_SCENE_I      = 8    /* 9.wav */
};

#define SOUND_BED_COUNT      9
#define SOUND_BED_FIRST_FREE 0    /* lowest bed safe for scene rotation */
#define SOUND_BED_LAST_FREE  8    /* highest bed (inclusive) */

/* ---------------------------------------------------------------- */
/*  FM patches — games/<game>/sound/fm/<N>_fm_example_*.mml          */
/* ---------------------------------------------------------------- */
enum {
	SOUND_FM_MENU_MAJOR     = 0,  /* 0_fm_example_a.mml */
	SOUND_FM_SAMURAI_MINOR  = 1,  /* 1_fm_example_b.mml */
	SOUND_FM_ATTRACT_FAST   = 2,  /* 2_fm_example_c.mml */
	SOUND_FM_DUEL_SUSPENSE  = 3,  /* 3_fm_example_d.mml */
	SOUND_FM_BASS_MOTIF     = 4,  /* 4_fm_example_e.mml */
	SOUND_FM_VICTORY_JINGLE = 5,  /* 5_fm_example_f.mml */
	SOUND_FM_GAMEOVER_DARK  = 6,  /* 6_fm_example_g.mml */
	SOUND_FM_PATCH_SHOWCASE = 7   /* 7_fm_example_h.mml */
};

#define SOUND_FM_TRACK_1  SOUND_FM_MENU_MAJOR
#define SOUND_FM_TRACK_2  SOUND_FM_SAMURAI_MINOR
#define SOUND_FM_TRACK_3  SOUND_FM_ATTRACT_FAST
#define SOUND_FM_TRACK_4  SOUND_FM_DUEL_SUSPENSE
#define SOUND_FM_TRACK_5  SOUND_FM_BASS_MOTIF
#define SOUND_FM_TRACK_6  SOUND_FM_VICTORY_JINGLE
#define SOUND_FM_TRACK_7  SOUND_FM_GAMEOVER_DARK
#define SOUND_FM_TRACK_8  SOUND_FM_PATCH_SHOWCASE
#define SOUND_FM_TRACK_COUNT 8

/* ---------------------------------------------------------------- */
/*  SSG tracks — games/<game>/sound/ssg/<N>_ssg_example_*.mml        */
/* ---------------------------------------------------------------- */
enum {
	SOUND_SSG_MENU_LOOP    = 0,  /* 0_ssg_example_a.mml */
	SOUND_SSG_ARCADE_ALERT = 1,  /* 1_ssg_example_b.mml */
	SOUND_SSG_INSERT_COIN  = 2,  /* 2_ssg_example_c.mml */
	SOUND_SSG_WARRIOR_RIFF = 3   /* 3_ssg_example_d.mml */
};

/* Legacy alias — was index 0 in the old enum; map to the closest
 * real SSG track so attract / battle-sting code paths still compile. */
#define SOUND_SSG_BATTLE_STING  SOUND_SSG_ARCADE_ALERT

#define SOUND_SSG_TRACK_1  SOUND_SSG_MENU_LOOP
#define SOUND_SSG_TRACK_2  SOUND_SSG_ARCADE_ALERT
#define SOUND_SSG_TRACK_3  SOUND_SSG_INSERT_COIN
#define SOUND_SSG_TRACK_4  SOUND_SSG_WARRIOR_RIFF
#define SOUND_SSG_TRACK_COUNT 4

/* ---------------------------------------------------------------- */
/*  Voice cues (ADPCM-A long voice samples)                          */
/* ---------------------------------------------------------------- */
enum {
	SOUND_VOICE_GET_READY = 1,
	SOUND_VOICE_ATTACK    = 2
};

#endif
