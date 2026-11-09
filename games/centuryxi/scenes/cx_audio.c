/*
 * cx_audio.c -- Century XI: what the match sounds like.
 *
 * As on the television of 1994 a match has no music: the crowd has the ADPCM-B
 * channel, swelling when the ball nears a goal, and the announcer, the officials and
 * the kicks ride on ADPCM-A. The screens round the match have the full band, from
 * tools/music.py on ADPCM-B. tools/sfx.py makes sounds 1-9, tools/voices.py the rest:
 * the calls, the chants, the crowd's voices and every man's name.
 */
#include "cx_render.h"

/* sample numbers: the file's number less one */
enum { SFX_KICK, SFX_SHOT, SFX_WHISTLE, SFX_LONG_WHISTLE, SFX_POST, SFX_CATCH, SFX_SLIDE, SFX_OOH, SFX_ROAR,
       V_GOAL3, V_WHAT_A_GOAL, V_HEADER, V_AMAZING, V_OWN_GOAL, V_SAVE, V_CLOSE, V_POST, V_OFFSIDE, V_FOUL, V_YELLOW, V_RED,
       V_PENALTY, V_FREE_KICK, V_CORNER, V_KICK_OFF, V_HALF, V_FULL, V_EXTRA, V_GOLDEN, V_HAND, V_NUTMEG, V_REVIEW, V_SUB,
       V_BLIND, V_MY_LEG, V_COME_ON, V_NO_TALKING, V_OFF_PITCH, V_PLAY_ON, C_RUN, C_WHY, C_YES, CH_OLE, CH_NIPPON, SFX_JINGLE,
       CH_NATION };                  /* then one chant for each nation, in tools/nations.py's order */
#define CH_BOO     (CH_NATION + 32)  /* after the chants: the crowd's boo, then its whistling */
#define CH_SIFFLE  (CH_NATION + 33)
/* then the commentator's calls on the play (tools/voices.py COMMENTARY) */
enum { V_C_SHOT = CH_NATION + 34, V_C_PASSING, V_C_HEADER, V_C_STRIKE, V_C_LONG, V_C_THROUGH, V_C_ONETWO,
       V_C_TACKLE, V_C_BREAK, V_C_DANGER, V_NAME0 };   /* then every shirt name (cx_man_voice) */
enum { B_CROWD, B_TITLE, B_MENU, B_PENALTY, B_CROWD_UP, B_FINAL, B_CHAMPION };

static uint16_t s_quiet;          /* frames until the announcer may speak again */
static uint16_t s_chant;
static uint8_t s_vol;
static uint8_t s_surge;           /* frames of a shot's roar left */
static uint16_t s_crowd_quiet;    /* frames until the crowd may boo or whistle again */
static uint8_t s_pass_team, s_pass_n;   /* passes in a row by one side, for "great passing" */
static uint16_t s_danger_t;       /* frames until "danger here" may be said again */
static uint8_t s_owner = 0xFFu;   /* the man on the ball when the commentator last looked */
static uint8_t s_named = 0xFFu;   /* the last man he named */
static uint8_t s_name_gap;        /* frames until he may name a man on the ball again */
static uint8_t s_later_man = 0xFFu, s_later_t;  /* a name to say once the call before it is done */
static uint8_t s_later_team;

/* "Takoyaki!": the man's own name, the way a commentator follows the ball */
static void NEOGEO_USER name(uint8_t team, uint8_t squad, uint16_t hold)
{
    playSFXWide((uint16_t)(V_NAME0 + cx_man_voice(team, squad)));
    s_quiet = hold;
}

static void NEOGEO_USER name_later(const CxWorld *w, uint8_t man, uint8_t delay)
{
    if (man >= CX_MEN) return;
    s_later_man = w->m[man].squad;
    s_later_team = w->team[man >= 11u];
    s_later_t = delay;
    s_named = man;
}

/* a card: the referee's call, then the man's name (cx_referee.c) */
void NEOGEO_USER cx_say_man_later(const CxWorld *w, uint8_t man, uint8_t delay) { name_later(w, man, delay); }

/* The commentator's clock, once a frame: in play from cx_sound_events, and from the
 * scenes that stop the match (a card) so a name waiting there is still said. */
void NEOGEO_USER cx_voice_tick(void)
{
    if (s_quiet) s_quiet--;
    if (s_name_gap) s_name_gap--;
    if (s_later_t && !--s_later_t) { name(s_later_team, s_later_man, 50); s_name_gap = 90; }
}

static void NEOGEO_USER say(uint8_t line, uint16_t hold)
{
    if (s_quiet) return;
    playSFX(line);
    s_quiet = hold;
}

void NEOGEO_USER cx_sound_events(const CxWorld *w)
{
    uint32_t ev = w->ev;
    int16_t pace = (int16_t)((w->ball.vx < 0 ? -w->ball.vx : w->ball.vx) + (w->ball.vy < 0 ? -w->ball.vy : w->ball.vy));
    cx_voice_tick();
    if (ev & CX_EV_GOAL) {
        playSFX(SFX_ROAR);
        playSFX(SFX_JINGLE);
        s_quiet = 0;
        say(w->goal_kind == CX_G_OWN ? V_OWN_GOAL : V_GOAL3, 170);
        /* "Goal! Goal! Goal!" ... and who */
        if (w->goal_kind != CX_G_OWN) name_later(w, w->scorer, 142);
    } else if (ev & CX_EV_HANDBALL) { playSFX(SFX_WHISTLE); say(V_HAND, 90); }
    else if (ev & CX_EV_SAVE) {
        uint8_t k = w->ball.owner < CX_MEN ? w->ball.owner : w->ball.last;
        playSFX(SFX_CATCH);
        playSFX(SFX_OOH);
        if (!s_quiet && k < CX_MEN && w->m[k].role == 0u) name_later(w, k, 88);
        say(V_SAVE, 90);
    }
    else if (ev & CX_EV_POST) { playSFX(SFX_POST); playSFX(SFX_OOH); say(V_POST, 90); }
    else if (ev & CX_EV_MISS) { playSFX(SFX_OOH); say(V_CLOSE, 90); }
    else if (ev & CX_EV_OFFSIDE) { playSFX(SFX_WHISTLE); say(V_OFFSIDE, 90); }
    else if (ev & CX_EV_DIVE) { say(V_MY_LEG, 80); if (!s_crowd_quiet) { playSFX(CH_BOO); s_crowd_quiet = 240; } }
    else if (ev & CX_EV_INJURY) { playSFX(SFX_WHISTLE); say(V_MY_LEG, 90); }
    else if (ev & CX_EV_FOUL) { playSFX(SFX_WHISTLE); say(V_FOUL, 90); }
    else if (ev & CX_EV_ADDED) { }
    else if ((ev & CX_EV_WHISTLE) && (w->phase == CX_P_HALF || w->phase == CX_P_FULL || w->phase == CX_P_PENS || w->phase == CX_P_ETBREAK)) {
        playSFX(SFX_LONG_WHISTLE);
        s_quiet = 0;
        say(w->phase == CX_P_HALF ? V_HALF : (w->phase == CX_P_ETBREAK ? V_EXTRA : V_FULL), 120);
    } else if (ev & CX_EV_WHISTLE) {
        playSFX(SFX_WHISTLE);
        if (w->phase == CX_P_KICKOFF) say(V_KICK_OFF, 90);
        else if (w->phase == CX_P_FREEKICK) say(V_FREE_KICK, 90);
        else if (w->phase == CX_P_CORNER) say(V_CORNER, 90);
    }
    else if ((ev & CX_EV_SHOT) && !(ev & CX_EV_SPECIAL))
        say(w->ball.kind == CX_G_HEADER ? V_C_HEADER : (pace > 200 ? V_C_STRIKE : V_C_SHOT), 70);
    else if (ev & CX_EV_ONETWO) say(V_C_ONETWO, 70);
    else if (ev & CX_EV_THROUGH) say((w->frame & 1u) ? V_C_THROUGH : V_C_BREAK, 70);
    else if (ev & CX_EV_NUTMEG) say(V_NUTMEG, 90);
    else if (ev & CX_EV_SPECIAL) say(V_AMAZING, 90);
    else if (ev & CX_EV_TACKLE) { playSFX(SFX_SLIDE); if ((w->frame & 3u) == 0u) say(V_C_TACKLE, 70); }
    else if (ev & CX_EV_KICK) {
        playSFX(pace > 90 ? SFX_SHOT : SFX_KICK);
        if ((ev & CX_EV_PASS) && w->ball.last < CX_MEN && w->m[w->ball.last].kick == CX_K_LONG && (w->frame & 1u)) say(V_C_LONG, 70);
    }
    /* a side stringing passes together */
    if ((ev & CX_EV_PASS) && w->ball.last < CX_MEN) {
        uint8_t t = (uint8_t)(w->ball.last >= 11u);
        if (t == s_pass_team) { if (s_pass_n < 255u) s_pass_n++; }
        else { s_pass_team = t; s_pass_n = 1; }
        if (s_pass_n == 6u) say(V_C_PASSING, 80);
    }
    /* "danger here": a side carries it into the other's box, now and then */
    if (s_danger_t) s_danger_t--;
    else if (w->ball.owner < CX_MEN && w->phase == CX_P_PLAY) {
        uint8_t t = (uint8_t)(w->ball.owner >= 11u);
        int16_t x = (int16_t)(w->ball.x >> 4), gx = cx_attacks_right(w, t) ? CX_LEN : 0;
        if ((x - gx < 0 ? gx - x : x - gx) < 260) { say(V_C_DANGER, 80); s_danger_t = 600; }
    }
    if (ev & CX_EV_SUB) say(V_SUB, 90);
    /* the play-by-play: the man who has just got the ball, by name, when the commentator
     * has nothing bigger to say (never the same man twice running, a second apart at least) */
    if (w->ball.owner != s_owner) {
        uint8_t o = w->ball.owner;
        s_owner = o;
        if (o < CX_MEN && o != s_named && w->phase == CX_P_PLAY && !s_quiet && !s_name_gap && !s_later_t) {
            name(w->team[o >= 11u], w->m[o].squad, 46);
            s_named = o;
            s_name_gap = 75;
        }
    }
    /* the stands: a shot lifts them for a second; a card or a foul from behind and they
     * whistle (not every time: a crowd that whistles at everything is noise) */
    if (ev & (CX_EV_SHOT | CX_EV_SPECIAL)) s_surge = 60;
    if (s_crowd_quiet) s_crowd_quiet--;
    else if ((ev & CX_EV_CARD) || ((ev & CX_EV_FOUL) && (w->frame & 3u) == 0u)) { playSFX(CH_SIFFLE); s_crowd_quiet = 300; }
}

void NEOGEO_USER cx_chant(uint8_t nation)
{
    if (nation < 32u) playSFX((uint8_t)(CH_NATION + nation));
    else playSFXWide((uint16_t)(V_NAME0 + cx_chant_voice(nation)));
    s_chant = 0;
}

/* The fans of the side on top sing their own song: after a goal, on a good spell, and every
 * so often just because. Nobody sings over the last song. */
void NEOGEO_USER cx_terraces(const CxWorld *w)
{
    uint8_t t;
    if (++s_chant < 420u) return;
    t = (uint8_t)(w->momentum[1] > w->momentum[0]);
    if (w->momentum[t] > 90u) cx_chant(w->team[t]);
    else if (s_chant > 1200u) cx_chant(w->team[(w->frame >> 6) & 1u]);
}

/* The crowd rises as the ball gets near a goal. */
/* The crowd follows the danger, not the clock: a murmur while the ball goes about the
 * middle, a rising noise as a side gets near the other's goal with it, a roar at a shot,
 * and the whole stadium louder when the match is tight and late, or in extra time. It
 * moves a step at a time, so it swells and settles instead of jumping. */
void NEOGEO_USER cx_crowd_level(const CxWorld *w)
{
    int16_t x = (int16_t)(w->ball.x >> 4);
    int16_t near = x < CX_LEN / 2 ? x : (int16_t)(CX_LEN - x);
    int16_t want, gap = (int16_t)w->score[0] - (int16_t)w->score[1];
    uint8_t attacking = 0;
    if (w->ball.owner != CX_NOBODY) {
        /* the side on the ball going toward the goal the ball is nearer */
        uint8_t t = (uint8_t)(w->ball.owner >= 11u);
        attacking = (uint8_t)(cx_attacks_right(w, t) == (x > CX_LEN / 2));
    }
    want = near < 220 ? (attacking ? 0xB8 : 0x98) : (near < 420 ? (attacking ? 0x88 : 0x70) : 0x50);
    if (gap < 0) gap = (int16_t)-gap;
    if (w->half >= 2u) want = (int16_t)(want + 0x28);                                       /* extra time */
    else if (w->half == 1u && w->clock > (uint16_t)(w->half_frames / 3u * 2u) && gap <= 1) want = (int16_t)(want + 0x18);
    if (s_surge) { s_surge--; if (want < 0xD8) want = 0xD8; }
    if (w->phase == CX_P_GOAL) want = 0xF0;
    if (want > 0xF8) want = 0xF8;
    if ((w->frame & 7u) == 0 && want != s_vol) {
        int16_t v = (int16_t)s_vol;
        v = (int16_t)(v + (want > v ? (want - v > 16 ? 16 : want - v) : -(v - want > 8 ? 8 : v - want)));
        s_vol = (uint8_t)v;
        soundSetADPCMBVolume(s_vol);
    }
}

void NEOGEO_USER cx_sfx_menu(void) { playSSGTrack(0); }
void NEOGEO_USER cx_sfx_kick(void) { playSFX(SFX_SHOT); }
void NEOGEO_USER cx_sfx_goal(void) { playSFX(SFX_ROAR); playSFX(V_WHAT_A_GOAL); }
void NEOGEO_USER cx_sfx_save(void) { playSFX(SFX_CATCH); playSFX(V_SAVE); }
void NEOGEO_USER cx_sfx_whistle(void) { playSFX(SFX_WHISTLE); }
void NEOGEO_USER cx_say_penalty(void) { playSFX(SFX_WHISTLE); playSFX(V_PENALTY); }
void NEOGEO_USER cx_say_card(uint8_t red) { playSFX(red ? V_RED : V_YELLOW); }
void NEOGEO_USER cx_say_review(void) { playSFX(V_REVIEW); }

static void NEOGEO_USER bed(uint8_t track, uint8_t volume)
{
    soundStopAll();
    soundSetADPCMAVolume(0x3Cu);
    soundSetADPCMBVolume(volume);
    soundSetADPCMBLoop(1u);
    playSFXB(track);
    s_vol = volume;
    s_quiet = 0;
}

void NEOGEO_USER cx_music_title(void) { bed(B_TITLE, 0xB0u); }
void NEOGEO_USER cx_music_menu(void) { bed(B_MENU, 0xA8u); }
void NEOGEO_USER cx_music_match(void) { bed(B_CROWD, 0x58u); s_chant = 0; }
void NEOGEO_USER cx_music_penalty(void) { bed(B_PENALTY, 0xA8u); }
void NEOGEO_USER cx_music_final(void) { bed(B_FINAL, 0xB0u); }
void NEOGEO_USER cx_music_champion(void) { bed(B_CHAMPION, 0xB0u); playSFX(SFX_ROAR); }
