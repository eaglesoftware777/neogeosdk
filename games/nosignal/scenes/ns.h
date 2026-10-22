/*
 * ns.h -- No Signal Z Alpha: a one-player street brawler on the SDK's
 * beat-'em-up engine (sdk/2d_engine/ng_beatemup.c).
 *
 * Someone on a motorbike snatched the hero's phone, and on it are years of
 * messages from someone far away. ns_game.c runs the game (title, hero
 * select, the stages and their scenes, continue, ending); ns_draw.c draws
 * it and plays its sounds; ns_data.c holds the fighters, moves and stages.
 * Exactly one person plays: player 2's controls are never read.
 */
#ifndef NS_H
#define NS_H
#include <stdint.h>
#include "sdk/neogeo.h"
#include "sdk/ng_system.h"
#include "sdk/2d_engine/ng_beatemup.h"

#define NS_START_FLAG   0x100700u
#define NS_VERSION_TEXT "V2 CANDIDATE"

#define NS_UP      0x0001u
#define NS_DOWN    0x0002u
#define NS_LEFT    0x0004u
#define NS_RIGHT   0x0008u
#define NS_A       0x0010u
#define NS_B       0x0020u
#define NS_C       0x0040u
#define NS_D       0x0080u
#define NS_START   0x0100u
#define NS_BUTTONS (NS_A | NS_B | NS_C | NS_D)
#define NS_DIRS    (NS_UP | NS_DOWN | NS_LEFT | NS_RIGHT)

enum { NS_TXT_WHITE = 0, NS_TXT_GOLD, NS_TXT_PINK, NS_TXT_SKY, NS_TXT_MINT, NS_TXT_RED, NS_TXT_GREY,
       NS_BAR_FULL, NS_BAR_HALF, NS_BAR_LOW, NS_BAR_ENEMY, NS_BAR_BOSS };   /* FIX palettes */

/* heroes */
enum { NS_JAY = 0, NS_ZOE, NS_KAI, NS_HEROES };
/* who speaks in scenes (and whose portrait shows): tools/art.py's SPEAKERS order */
enum {
    NS_SPK_JAY = 0, NS_SPK_ZOE, NS_SPK_KAI, NS_SPK_RICO, NS_SPK_THIEF, NS_SPK_BIKER, NS_SPK_SKATER,
    NS_SPK_BIGFLEX, NS_SPK_ZEROX, NS_SPK_M08, NS_SPK_BEHEMOTH, NS_SPK_PROXY, NS_SPK_OVERMIND, NS_SPK_ARCHIVIST,
    NS_SPK_NARRATOR, NS_SPEAKERS
};
#define NS_SPK_HERO 0xFFu        /* a scene line spoken by whichever hero is playing */

/* A fighter's pictures: frame numbers in its sheet. Heroes have 30 frames,
 * the street crew 10, the boss 20 (see tools/art.py). */
enum {
    HF_STANCE = 0, HF_IDLE, HF_WALK1, HF_WALK2, HF_WALK3, HF_WALK4, HF_RUN1, HF_RUN2, HF_JUMP_UP, HF_JUMP_DOWN,
    HF_KEY2, HF_JAB, HF_CROSS, HF_UPPER, HF_KICK, HF_JUMPKICK, HF_SWEEP, HF_DASH, HF_GRAB, HF_THROW,
    HF_KEY3, HF_HURT, HF_HURT_LOW, HF_KNOCKED, HF_DOWN, HF_GETUP, HF_SPECIAL1, HF_SPECIAL2, HF_WIN, HF_RIDE,
    HF_COUNT
};
enum { EF_STANCE = 0, EF_WALK1, EF_WALK2, EF_WALK3, EF_WINDUP, EF_ATTACK, EF_HURT, EF_KNOCKED, EF_DOWN, EF_GETUP, EF_COUNT };
enum {  /* Rico's second sheet follows his first */
    BF_STANCE2 = 10, BF_THROW_PHONE, BF_TAUNT, BF_BIKE1, BF_BIKE2, BF_BIKE_FALL, BF_DIZZY, BF_LAUGH, BF_LUNGE, BF_DEFEAT, BF_COUNT
};
/* The other bosses' second sheets: frame 10 + i is that sheet's figure i
 * (tools/import_sheets.py), named in ns_data.c beside their moves. */
#define NS_SHEET2(i)  ((uint8_t)(10u + (i)))
#define NS_SHEET3(i)  ((uint8_t)(20u + (i)))

/* effects: ADPCM-A samples (sound/samples/in_wav_a) */
enum {
    NS_SFX_COIN = 0, NS_SFX_START, NS_SFX_SELECT, NS_SFX_PUNCH, NS_SFX_KICK, NS_SFX_HEAVY, NS_SFX_WHOOSH,
    NS_SFX_HURT, NS_SFX_FALL, NS_SFX_GRAB, NS_SFX_THROW, NS_SFX_SPECIAL, NS_SFX_BREAK, NS_SFX_PICKUP,
    NS_SFX_FOOD, NS_SFX_SCOOTER, NS_SFX_CRASH, NS_SFX_PHONE, NS_SFX_BIKE, NS_SFX_GO, NS_SFX_KO,
    NS_SFX_TEXT, NS_SFX_JUMP, NS_SFX_LAND,
    NS_SFX_VOICE,                         /* 4 lines a hero: bruh, no cap, you're cooked, massive W */
    NS_SFX_RICO_RIDE = NS_SFX_VOICE + 4 * NS_HEROES, NS_SFX_RICO_LAUGH, NS_SFX_NO_SIGNAL,
    NS_SFX_ZAP, NS_SFX_LASER, NS_SFX_ROAR, NS_SFX_SERVO, NS_SFX_WARBLE, NS_SFX_ALARM, NS_SFX_GAS, NS_SFX_STOMP,
    NS_SFX_COUNT
};
enum { NS_V_BRUH = 0, NS_V_NOCAP, NS_V_COOKED, NS_V_W };
/* music: ADPCM-B tracks (file n + 1; from 4 on n + 2: 5.wav is the eye-catcher's) */
enum {
    NS_MUS_TITLE = 0, NS_MUS_SELECT, NS_MUS_STAGE1, NS_MUS_BOSS, NS_MUS_CLEAR, NS_MUS_OVER, NS_MUS_SCENE, NS_MUS_ENDING,
    NS_MUS_STAGE2, NS_MUS_STAGE3, NS_MUS_STAGE4, NS_MUS_STAGE5, NS_MUS_STAGE6, NS_MUS_STAGE7, NS_MUS_STAGE8,
    NS_MUS_FINAL, NS_MUS_CREDITS
};

/* ns_data.c ------------------------------------------------------------------ */
typedef struct {
    const char *name, *nick, *style, *special;
    uint8_t speaker;
} NSHeroInfo;
extern const NSHeroInfo ns_hero_info[NS_HEROES];
extern const NGBFighterDef ns_heroes[NS_HEROES];
extern const NGBFighterDef ns_heroes_space[NS_HEROES];      /* stage 8: low gravity, higher jumps */
extern const NGBFighterDef ns_thief, ns_biker, ns_skater, ns_rico;
extern const NGBFighterDef ns_guard, ns_drone, ns_runner, ns_bloater, ns_android, ns_robodog, ns_walker, ns_alien;
extern const NGBFighterDef ns_bigflex, ns_zerox, ns_m08, ns_behemoth, ns_proxy, ns_overmind, ns_archivist;

/* How a fighter is drawn: its sheet (NS_ART_F_*), and for a boss whose
 * pictures change with its phase, the standard EF_ frames per phase. */
typedef struct {
    const NGBFighterDef *def;
    uint8_t art;
    uint8_t lift;                             /* drawn this many pixels up (hovering drones) */
    const uint8_t (*phase_frames)[EF_COUNT];   /* NULL: EF_ frames in every phase */
    const char *name;                         /* on the enemy health bar */
} NSLook;
extern const NSLook ns_looks[];
extern const uint8_t ns_nlooks;

typedef struct { uint8_t speaker; const char *text; } NSLine;
typedef struct {
    const NGBStage *stage;
    const char *title, *place;
    uint8_t music, boss_music;
    uint8_t bg;                           /* NS_ART_STAGE<n> */
    uint8_t crew[6];                      /* fighter art the stage loads the colours of (0 ends) */
    const char *boss_name;
    const NSLine *intro[NS_HEROES];       /* lists end with text 0 */
    const NSLine *boss[NS_HEROES];
    const NSLine *outro[NS_HEROES];
    const NGBFighterDef *heroes;          /* the heroes' data on this stage (low gravity...) */
} NSStageInfo;
#define NS_STAGES       8u
extern const NSStageInfo ns_stages[NS_STAGES];
extern const NSLine *const ns_ending[NS_HEROES];
extern const NSLine *const ns_epilogue[NS_HEROES];
extern const NGBStage ns_boss_rush[NS_STAGES];

/* The gallery: shown once the player has reached its stage (0: always). */
typedef struct { uint8_t art, stage; const char *name, *bio; } NSGalleryEntry;
extern const NSGalleryEntry ns_gallery[];
extern const uint8_t ns_gallery_n;

/* ns_draw.c ------------------------------------------------------------------ */
void     NEOGEO_USER ns_vblank(void);
void     NEOGEO_USER ns_presentation_init(void);
void     NEOGEO_USER ns_fix_palettes(void);
uint16_t NEOGEO_USER ns_frame(void);
uint16_t NEOGEO_USER ns_held(void);
uint16_t NEOGEO_USER ns_pressed(void);
uint16_t NEOGEO_USER ns_rand(void);
void     NEOGEO_USER ns_wait(uint16_t frames);
uint8_t  NEOGEO_USER ns_wait_button(uint16_t frames);
void     NEOGEO_USER ns_puts(uint8_t x, uint8_t y, const char *text, uint8_t pal);
void     NEOGEO_USER ns_puts_c(uint8_t y, const char *text, uint8_t pal);
void     NEOGEO_USER ns_putn(uint8_t x, uint8_t y, uint32_t value, uint8_t digits, uint8_t pal);
void     NEOGEO_USER ns_text_block(uint8_t x, uint8_t y, const char *text, uint8_t pal);
void     NEOGEO_USER ns_clear_text(uint8_t y0, uint8_t y1);
void     NEOGEO_USER ns_fade_out(void);
void     NEOGEO_USER ns_picture(uint8_t art);
void     NEOGEO_USER ns_reveal(void);
void     NEOGEO_USER ns_logo(uint8_t show, int16_t x, int16_t y);
void     NEOGEO_USER ns_card(uint8_t show);
void     NEOGEO_USER ns_hero_card(uint8_t slot, uint8_t hero, uint8_t frame, int16_t x, int16_t y, uint8_t flip);
void     NEOGEO_USER ns_cards_hide(void);
void     NEOGEO_USER ns_show_fighter(uint8_t art, uint8_t frame, int16_t x, int16_t y);   /* the gallery */
void     NEOGEO_USER ns_stage_show(const NGBeatEmUp *b, const NSStageInfo *si, uint8_t hero);
void     NEOGEO_USER ns_stage_draw(const NGBeatEmUp *b);
void     NEOGEO_USER ns_effect(int16_t x, int16_t y, uint8_t kind);
void     NEOGEO_USER ns_respawn_blink(void);       /* the hero blinks a moment after coming back */
enum { NS_FX_HIT = 0, NS_FX_BIG, NS_FX_DUST };
void     NEOGEO_USER ns_hud(const NGBeatEmUp *b, uint8_t hero, uint8_t full);
void     NEOGEO_USER ns_boss_bar(const NGBeatEmUp *b, uint8_t boss, const char *name);
/* the enemy the player just hit: its name and health on the second line for a while */
void     NEOGEO_USER ns_focus(uint8_t fighter, const NGBFighterDef *def);
void     NEOGEO_USER ns_bark(const char *text, int16_t x, int16_t y);
void     NEOGEO_USER ns_bark_tick(void);
void     NEOGEO_USER ns_banner(const char *text, uint8_t pal, uint16_t frames);
void     NEOGEO_USER ns_dialog(uint8_t speaker, const char *text);
void     NEOGEO_USER ns_dialog_hide(void);
void     NEOGEO_USER ns_music(uint8_t track);
void     NEOGEO_USER ns_sfx(uint8_t sfx);
void     NEOGEO_USER ns_throw_pose(void);

/* ns_game.c ------------------------------------------------------------------ */
void     NEOGEO_USER ns_title_screen(void);
void     NEOGEO_USER ns_run_attract(void);
void     NEOGEO_USER ns_run(void);

/* user.c ---------------------------------------------------------------------- */
uint8_t  NEOGEO_USER ns_start_pending(void);
void     NEOGEO_USER ns_auto_start(void);
uint8_t  NEOGEO_USER ns_spend_credit(void);

#endif
