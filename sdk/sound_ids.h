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
/*  MUSIC tracks — games/<game>/sound/mml/<N>_*.mml                  */
/* ---------------------------------------------------------------- */
enum {
	SOUND_MUSIC_SAMURAI_GAME_LOOP    = 0,  /* 0_samurai_game_loop    */
	SOUND_MUSIC_SAMURAI_BATTLE_LOOP  = 1,  /* 1_samurai_battle_loop  */
	SOUND_MUSIC_SAMURAI_NIGHT_SCENE  = 2,  /* 2_samurai_night_scene  */
	SOUND_MUSIC_WARRIOR_BATTLE       = 3,  /* 3_warrior_battle       */
	SOUND_MUSIC_EAGLE_FANFARE        = 4,  /* 4_eagle_fanfare        */
	SOUND_MUSIC_BOSS_TENSION         = 5,  /* 5_boss_tension         */
	SOUND_MUSIC_SHOP_JINGLE          = 6,  /* 6_shop_jingle          */
	SOUND_MUSIC_ENDING_CREDITS       = 7   /* 7_ending_credits       */
};

/* Legacy alias kept for older demo code that still references the
 * old name — points to the same audio data as SAMURAI_NIGHT_SCENE. */
#define SOUND_MUSIC_SAMURAI_ENDING_SCENE  SOUND_MUSIC_SAMURAI_NIGHT_SCENE

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
enum {
	SOUND_BED_TITLE_THEME  = 0,
	SOUND_BED_STAGE_ONE    = 1,
	SOUND_BED_STAGE_TWO    = 2,
	SOUND_BED_ENDING_THEME = 3,
	SOUND_BED_EYECATCHER   = 4
};

/* ---------------------------------------------------------------- */
/*  FM patches — games/<game>/sound/fm/<N>_*.mml                     */
/* ---------------------------------------------------------------- */
enum {
	SOUND_FM_MENU_MAJOR     = 0,  /* 0_menu_major     */
	SOUND_FM_SAMURAI_MINOR  = 1,  /* 1_samurai_minor  */
	SOUND_FM_ATTRACT_FAST   = 2,  /* 2_attract_fast   */
	SOUND_FM_DUEL_SUSPENSE  = 3,  /* 3_duel_suspense  */
	SOUND_FM_BASS_MOTIF     = 4,  /* 4_bass_motif     */
	SOUND_FM_VICTORY_JINGLE = 5,  /* 5_victory_jingle */
	SOUND_FM_GAMEOVER_DARK  = 6,  /* 6_gameover_dark  */
	SOUND_FM_PATCH_SHOWCASE = 7   /* 7_patch_showcase */
};

/* ---------------------------------------------------------------- */
/*  SSG tracks — games/<game>/sound/ssg/<N>_*.mml                    */
/* ---------------------------------------------------------------- */
enum {
	SOUND_SSG_MENU_LOOP    = 0,  /* 0_square_menu  */
	SOUND_SSG_ARCADE_ALERT = 1,  /* 1_arcade_alert */
	SOUND_SSG_INSERT_COIN  = 2   /* 2_insert_coin  */
};

/* Legacy alias — was index 0 in the old enum; map to the closest
 * real SSG track so attract / battle-sting code paths still compile. */
#define SOUND_SSG_BATTLE_STING  SOUND_SSG_ARCADE_ALERT

/* ---------------------------------------------------------------- */
/*  Voice cues (ADPCM-A long voice samples)                          */
/* ---------------------------------------------------------------- */
enum {
	SOUND_VOICE_GET_READY = 1,
	SOUND_VOICE_ATTACK    = 2
};

#endif
