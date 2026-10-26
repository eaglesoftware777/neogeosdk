/*
 * ml_game.c -- Maiya & Luna: the flow of the game.
 *
 * Attract: the title, the prologue over the valley's pictures, how to play.
 * A game: a new journey (who sets out, Maiya or Luna; the prologue, the
 * chapter card) or the saved one, then the valley itself. Each sister travels
 * alone, through the same places with puzzles of her own (the world has one
 * set of rooms per journey, ml_worlds[]); in Chapter VI the other one joins
 * her, and the two can switch. The RPG engine moves them and runs the puzzles, this
 * file turns its events into dialogue, sounds, room changes, notices and the
 * chapter's restoration scene. The notebook (START) holds the bag, the finds,
 * the current goal and saving.
 *
 * Controls: the stick walks; A talks, reads, takes, turns, rings and fits;
 * B is Maiya's whip (it shoos creatures) or Luna's close look (a hint about
 * what she faces); C uses the tool chosen in the notebook; when both sisters
 * travel together, D switches between them, held it makes the other one wait
 * where she stands (or follow again).
 *
 * Saving: the cartridge's save block (an arcade board's backup memory; a
 * console's work memory, copied to a memory card when there is one), at
 * rest points, at the end of a chapter and from the notebook.
 */
#include "ml.h"
#include "sdk/ng_system.h"
#include "games/maiyaluna/artbox/generated/ml_assets.h"

#define SAVE_VERSION  2u           /* 2: the journeys (who set out); a version 1 save is not read */
#define SAVE_SIZE     (NGR_SAVE_SIZE + 4u)

static NGRpg s_g;
uint8_t ml_journey;                 /* ML_J_MAIYA or ML_J_LUNA: who set out (her world, ml_worlds[]) */
static uint8_t s_tool;              /* the item C uses */
static uint8_t s_dlg_node_open;     /* a conversation is on screen */
static uint16_t s_dlg_node;
static uint8_t s_choice, s_choices_up;     /* s_choices_up: this node's choices are on screen */
static uint8_t s_pending;           /* a game event waiting for the conversation to end */
static uint8_t s_quit;
static uint16_t s_d_held;
static uint8_t s_heavy_t;           /* frames Luna has leaned on a heavy stone */
static uint32_t s_shut_told;        /* gates (by object) whose dropping was explained this visit */
static uint8_t s_card_buf[NG_CARD_FILE_SIZE(SAVE_SIZE)];

/* the game's own text for EagleBIOS's HOW TO PLAY screen */
static const NGHowToPlay k_howto = {
    {'H', 'T', 'P', '1'}, 11, 10, "MAIYA & LUNA",
    {
        "STICK  WALK",
        "A  TALK, READ, TAKE, USE",
        "B  MAIYA: WHIP  LUNA: LOOK",
        "C  USE THE CHOSEN TOOL",
        "D  SWITCH SISTERS (TOGETHER)",
        "   HOLD D: WAIT / FOLLOW",
        "START  THE NOTEBOOK",
        "",
        "PLAY MAIYA OR LUNA. MAIYA",
        "CLIMBS AND PUSHES. LUNA FITS",
        "GAPS AND FINE MECHANISMS.",
    }
};

static uint8_t NEOGEO_USER dip(uint8_t n) { return ng_dip_option(n); }

/* ======================================================================
 * saving
 * ==================================================================== */
static uint8_t NEOGEO_USER save_ok(void) { return (uint8_t)(ng_save_capacity() >= SAVE_SIZE); }

static uint8_t NEOGEO_USER saved_game(void)
{
    if (!save_ok()) return 0;
    if (ng_save_valid(SAVE_VERSION, SAVE_SIZE)) return 1;
    /* a console: perhaps on the memory card */
    if (!ng_sys_is_mvs() && ng_card_load(SAVE_VERSION, SAVE_SIZE, s_card_buf, sizeof(s_card_buf)) == NG_CARD_OK)
        return (uint8_t)ng_save_valid(SAVE_VERSION, SAVE_SIZE);
    return 0;
}

static uint8_t NEOGEO_USER save_game(void)
{
    uint8_t *d;
    if (!save_ok()) return 0;
    if (!ng_save_valid(SAVE_VERSION, SAVE_SIZE)) ng_save_format(SAVE_VERSION, SAVE_SIZE);
    d = (uint8_t *)ng_save_data();
    if (ng_rpg_save(&s_g, d, NGR_SAVE_SIZE) != NGR_SAVE_SIZE) return 0;
    d[NGR_SAVE_SIZE] = s_tool;
    d[NGR_SAVE_SIZE + 1u] = 1;
    d[NGR_SAVE_SIZE + 2u] = (uint8_t)(0x80u | ml_journey);   /* who set out */
    d[NGR_SAVE_SIZE + 3u] = 0;
    ng_save_commit();
    if (!ng_sys_is_mvs()) ng_card_save("MAIYA AND LUNA", s_card_buf, sizeof(s_card_buf));
    return 1;
}

/* The journey saved: her world, and the party as it was (one sister, or both
 * in Chapter VI); then the engine's own state. */
static uint8_t NEOGEO_USER load_game(void)
{
    const uint8_t *d;
    uint8_t i;
    if (!saved_game()) return 0;
    d = (const uint8_t *)ng_save_data();
    ml_journey = (d[NGR_SAVE_SIZE + 2u] & 0x80u) ? (uint8_t)(d[NGR_SAVE_SIZE + 2u] & 1u) : ML_J_MAIYA;
    ng_rpg_init(&s_g, &ml_worlds[ml_journey], (((uint32_t)ml_rand() << 16) ^ ml_rand()) | 1u);
    for (i = 0; i < d[7] && i < 2u; i++) ng_rpg_party_add(&s_g, &ml_party[d[8u + 8u * i] & 1u]);
    if (ng_rpg_load(&s_g, d, NGR_SAVE_SIZE) != NGR_OK) return 0;
    s_tool = d[NGR_SAVE_SIZE] < ML_ITEMS ? d[NGR_SAVE_SIZE] : ML_I_LENS;
    return 1;
}

/* ======================================================================
 * title, prologue, how to play
 * ==================================================================== */
static void NEOGEO_USER copyright(void)
{
    ml_puts_c(27, "(C) 2026 EAGLE SOFTWARE", ML_TXT_WHITE);
}

static void NEOGEO_USER title_art(void)
{
    ml_picture(ML_ART_BG_TITLE);
    ml_music(ML_MUS_TITLE);
    ml_logo(1, 32, 8);
    /* (the painting's people stand in the lower half: the text keeps above their heads) */
    ml_puts_c(11, "WONDERFUL ADVENTURES", ML_TXT_WHITE);
    ml_puts_c(12, "IN NATURE FANTASY", ML_TXT_WHITE);
    copyright();
}

void NEOGEO_USER ml_title_screen(void)
{
    uint16_t countdown = 15u * 60u, t = 0;
    title_art();
    for (;;) {
        if (NEO_REGISTER8(ML_START_FLAG)) break;
        t++;
        if (!ng_sys_is_mvs()) {
            ml_puts_c(14, (t & 32u) ? "PRESS START" : "           ", ML_TXT_WHITE);
            if (ml_start_pending()) break;
        } else if (read_p1credit() > 0) {
            ml_puts_c(14, "PRESS 1P START", ML_TXT_WHITE);
            ml_putn(18, 15, (uint32_t)(countdown / 60u), 2, ML_TXT_GOLD);
            if (countdown) countdown--;
            else ml_auto_start();
        } else {
            ml_puts_c(14, (t & 32u) ? "INSERT COIN" : "           ", ML_TXT_WHITE);
        }
        ml_frame();
    }
    ml_sfx(ML_SFX_START);
}

static const char *const k_prologue[] = {
    "AFTER LORD SMOGGAR'S DEFEAT,\nPEACE RETURNED TO THE VALLEY.\nTHE FACTORIES CLOSED AND\nTHE SMOKE BEGAN TO CLEAR.",
    "MAIYA, LUNA AND THEIR LITTLE\nBROTHER SUNBOY HELPED THEIR\nNEIGHBOURS REBUILD HOMES\nAND REPLANT THE FORESTS.",
    "BUT THE LAND HAD NOT FULLY\nRECOVERED. OLD MACHINES\nBLOCKED THE WATERWAYS, AND\nSOME SPRINGS HAD RUN DRY.",
    "IN WILLOWBROOK, BELOW THE\nSACRED FALLS, THE FIELDS\nWERE THIRSTY... AND THE OLD\nWATERMILL STOOD STILL.",
};

/* 1 if it ran to the end */
static uint8_t NEOGEO_USER prologue(uint8_t attract)
{
    uint8_t i;
    ml_chapter_picture(0);
    ml_music(ML_MUS_CHAPTER);
    ml_card(1);
    for (i = 0; i < 4u; i++) {
        ml_clear_text(4, 26);
        ml_puts_c(5, "THE SEVEN LOST WONDERS", ML_TXT_GOLD);
        ml_text_block(6, 10, k_prologue[i], ML_TXT_WHITE);
        if (attract) {
            ml_wait(300);
            if (ml_start_pending()) return 0;
        } else {
            ml_puts_c(24, "A: NEXT", ML_TXT_GREY);
            ml_wait_button(900);
        }
    }
    return 1;
}

static void NEOGEO_USER own_how_to_play(uint16_t frames)
{
    uint8_t i;
    ml_picture(ML_ART_BG_TITLE);
    ml_card(1);
    ml_puts_c(4, "HOW TO PLAY", ML_TXT_GOLD);
    for (i = 0; i < k_howto.lines; i++) ml_puts(6, (uint8_t)(7u + i + (i > 7u ? 1u : 0u)), k_howto.line[i], i < 8u ? ML_TXT_WHITE : ML_TXT_MINT);
    while (frames--) {
        ml_frame();
        if (ml_start_pending() && !NEO_REGISTER8(ML_START_FLAG)) return;
        if (NEO_REGISTER8(ML_START_FLAG) && (ml_pressed() & (ML_BUTTONS | ML_START))) return;
    }
}

void NEOGEO_USER ml_run_attract(void)
{
    for (;;) {
        uint16_t t;
        title_art();
        for (t = 0; t < 480u; t++) {
            ml_puts_c(14, (t & 32u) ? (ng_sys_is_mvs() ? "INSERT COIN" : "PRESS START") : "           ", ML_TXT_WHITE);
            ml_frame();
            if (ml_start_pending()) return;
        }
        if (!prologue(1)) return;
        if (ng_sys_is_eaglebios()) {
            ml_fade_out();
            ng_sys_howtoplay(&k_howto);
        } else {
            own_how_to_play(480);
        }
        if (ml_start_pending()) return;
    }
}

/* ======================================================================
 * cards
 * ==================================================================== */
static const char *const k_roman[7] = { "I", "II", "III", "IV", "V", "VI", "VII" };

static uint8_t NEOGEO_USER chapter_now(void)
{
    uint8_t c = s_g.counters[ML_CTR_CHAPTER];
    return (uint8_t)(c < ML_CHAPTERS ? c : ML_CHAPTERS - 1u);
}

static void NEOGEO_USER chapter_card(uint8_t c)
{
    const MLChapter *ch = &ml_chapters[ml_journey][c];
    char line[24];
    uint8_t i = 0, k = 0;
    const char *w = "CHAPTER ";
    ml_chapter_picture(c);
    ml_music(ML_MUS_CHAPTER);
    while (w[k]) line[i++] = w[k++];
    k = 0;
    while (k_roman[c][k]) line[i++] = k_roman[c][k++];
    line[i] = 0;
    ml_card(1);
    ml_puts_c(8, line, ML_TXT_GOLD);
    ml_puts_c(11, ml_text[ch->title], ML_TXT_WHITE);
    ml_puts_c(14, ml_text[ch->place], ML_TXT_SKY);
    ml_puts_c(22, "A: BEGIN", ML_TXT_GREY);
    ml_wait_button(600);
}

static void NEOGEO_USER enter_room_view(void);
static void NEOGEO_USER handle(const NGRpgEvent *e);

/* Who travels in a chapter: the sister who set out, alone, or both sisters in
 * a chapter they share (the other one joins at her side, index 1, and leaves
 * again after it). */
static void NEOGEO_USER party_for(uint8_t chapter)
{
    if (ml_chapters[ml_journey][chapter].together) {
        if (s_g.nparty < 2u) ng_rpg_party_add(&s_g, &ml_party[ml_journey ^ 1u]);
    } else if (s_g.nparty > 1u) {
        ng_rpg_party_remove(&s_g, 1u);
    }
}

/* where the party goes after a card: a chapter's start, or after its restoration */
static void NEOGEO_USER go_to(uint8_t room, uint8_t x, uint8_t y, uint8_t face)
{
    NGRpgEvent e;
    ng_rpg_enter_room(&s_g, room, x, y, face);
    enter_room_view();
    /* the room's own events (its arrival, a trigger's conversation), as on a new game */
    while (ng_rpg_event(&s_g, &e)) if (e.type != NGR_EV_ROOM) handle(&e);
}

static void NEOGEO_USER restoration(void)
{
    const MLChapter *ch = &ml_chapters[ml_journey][chapter_now()];
    uint8_t i;
    /* the water runs a moment before the picture changes; the sisters cheer */
    ml_member_pose(0, ML_POSE_CHEER);
    ml_member_pose(1, ML_POSE_CHEER);
    for (i = 0; i < 90u; i++) { ml_frame(); ng_rpg_update(&s_g); ml_room_draw(&s_g); }
    { NGRpgEvent e; while (ng_rpg_event(&s_g, &e)) {} }    /* that room's wheels and water, now behind the card */
    ml_chapter_picture(chapter_now());
    ml_music(ML_MUS_RESTORED);
    ml_card(1);
    ml_puts_c(6, ml_text[ch->restore_title], ML_TXT_GOLD);
    ml_text_block(7, 9, ml_text[ch->restore_text], ML_TXT_WHITE);
    ml_puts_c(15, "WONDER COMPONENT FOUND", ML_TXT_SKY);
    ml_puts_c(17, ml_text[ml_item_text[ml_journey][ch->wonder].title], ML_TXT_MINT);
    ml_icon(0, ch->wonder, 152, 148);
    ml_puts_c(22, "A: GO ON", ML_TXT_GREY);
    save_game();
    ml_wait_button(900);
    ml_icons_hide();
    go_to(ch->back_room, ch->back_x, ch->back_y, ch->back_face);    /* back, restored */
}

static void NEOGEO_USER the_end(void);

static void NEOGEO_USER chapter_end(void)
{
    uint8_t c = chapter_now();
    const MLChapter *ch = &ml_chapters[ml_journey][c];
    char line[28];
    uint8_t i = 0, k = 0;
    const char *w = "CHAPTER ";
    while (w[k]) line[i++] = w[k++];
    k = 0;
    while (k_roman[c][k]) line[i++] = k_roman[c][k++];
    w = " COMPLETE";
    k = 0;
    while (w[k]) line[i++] = w[k++];
    line[i] = 0;
    ml_picture(ML_ART_BG_TITLE);
    ml_music(ML_MUS_ENDING);
    ml_card(1);
    ml_puts_c(5, line, ML_TXT_GOLD);
    ml_puts_c(7, ml_text[ch->title], ML_TXT_WHITE);
    ml_text_block(6, 10, ml_text[ch->end_text], ML_TXT_WHITE);
    if (c + 1u < ML_CHAPTERS) ml_puts_c(15, ml_text[ml_chapters[ml_journey][c + 1u].title], ML_TXT_SKY);
    ml_puts_c(19, "YOUR JOURNEY IS SAVED.", ML_TXT_MINT);
    ml_puts_c(22, "A: GO ON", ML_TXT_GREY);
    if (c + 1u < ML_CHAPTERS) {
        s_g.counters[ML_CTR_CHAPTER] = (uint8_t)(c + 1u);
        party_for((uint8_t)(c + 1u));
    }
    save_game();
    ml_wait_button(1200);
    if (c + 1u >= ML_CHAPTERS) { the_end(); return; }
    ch = &ml_chapters[ml_journey][c + 1u];
    chapter_card((uint8_t)(c + 1u));
    go_to(ch->start_room, ch->start_x, ch->start_y, ch->start_face);
}

typedef struct { uint8_t row, pal; const char *text; } MLCredit;
static const MLCredit k_credits[] = {
    { 5, ML_TXT_GOLD, "MAIYA & LUNA" },
    { 7, ML_TXT_WHITE, "WONDERFUL ADVENTURES" },
    { 8, ML_TXT_WHITE, "IN NATURE FANTASY" },
    { 11, ML_TXT_WHITE, "A GAME BY EAGLE SOFTWARE" },
    { 13, ML_TXT_WHITE, "MADE WITH THE NEOGEOSDK" },
    { 15, ML_TXT_WHITE, "AND ITS ADVENTURE ENGINE" },
    { 19, ML_TXT_MINT, "THE VALLEY'S WATER" },
    { 20, ML_TXT_MINT, "RUNS FAIRLY AGAIN." },
    { 22, ML_TXT_MINT, "THANK YOU FOR PLAYING." },
};

/* After the last chapter: the credits, then the restored valley to wander. */
static void NEOGEO_USER the_end(void)
{
    uint8_t i;
    ml_picture(ML_ART_BG_TITLE);
    ml_music(ML_MUS_ENDING);
    ml_card(1);
    for (i = 0; i < sizeof(k_credits) / sizeof(k_credits[0]); i++)
        ml_puts_c(k_credits[i].row, k_credits[i].text, k_credits[i].pal);
    ml_wait_button(1800);
    go_to(ml_chapters[ml_journey][0].back_room, ml_chapters[ml_journey][0].back_x, ml_chapters[ml_journey][0].back_y, ml_chapters[ml_journey][0].back_face);
}

/* ======================================================================
 * the notebook (START)
 * ==================================================================== */
static const char *const k_tabs[4] = { "BAG", "FINDS", "NOTES", "JOURNEY" };
static const char *const k_cats[ML_CATS] = { "FLOWERS", "HISTORY", "DIAGRAMS", "PHOTOGRAPHS", "SPECIMENS", "WONDERS" };
static const uint8_t k_cat_goal[ML_CATS] = { ML_FINDS_FLOWER, ML_FINDS_HISTORY, ML_FINDS_DIAGRAM, ML_FINDS_PHOTO,
                                              ML_FINDS_SPECIMEN, ML_FINDS_WONDER };   /* every one in the world */

static const char *NEOGEO_USER goal_text(void)
{
    const MLChapter *ch = &ml_chapters[ml_journey][chapter_now()];
    uint8_t stage = s_g.counters[ch->quest], i, best = ch->goal_first;
    for (i = 0; i < ch->goals; i++)
        if (ml_goals[ml_journey][ch->goal_first + i].stage <= stage) best = (uint8_t)(ch->goal_first + i);
    return ml_text[ml_goals[ml_journey][best].text];
}

static void NEOGEO_USER notebook_draw(uint8_t tab, uint8_t sel, uint8_t *count)
{
    uint8_t i, row = 0;
    ml_clear_text(3, 26);
    ml_icons_hide();
    for (i = 0; i < 4u; i++) ml_puts((uint8_t)(4u + i * 8u), 4, k_tabs[i], i == tab ? ML_TXT_GOLD : ML_TXT_GREY);
    *count = 0;
    if (tab == 0) {
        for (i = 0; i < NGR_INV_SLOTS && row < 7u; i++) {
            uint8_t it = s_g.inv_item[i];
            if (!s_g.inv_count[i] || it >= ML_ITEMS) continue;
            ml_icon(row, it, 40, (int16_t)(52 + row * 16));
            ml_puts(8, (uint8_t)(7u + row * 2u), ml_text[ml_item_text[ml_journey][it].title], row == sel ? ML_TXT_GOLD : ML_TXT_WHITE);
            if (s_g.inv_count[i] > 1u) ml_putn(28, (uint8_t)(7u + row * 2u), s_g.inv_count[i], 2, ML_TXT_WHITE);
            if (it == s_tool) ml_puts(31, (uint8_t)(7u + row * 2u), "C", ML_TXT_MINT);
            if (row == sel) ml_text_block(5, 23, ml_text[ml_item_text[ml_journey][it].text], ML_TXT_SKY);
            row++;
        }
        *count = row;
        ml_puts(5, 21, "A: CHOOSE FOR C, OR USE IT", ML_TXT_GREY);
    } else if (tab == 1) {
        uint8_t found[ML_CATS];
        for (i = 0; i < ML_CATS; i++) found[i] = 0;
        for (i = 0; i < ML_COLLECT; i++)
            if (ng_rpg_collected(&s_g, i) && ml_collect[i].category < ML_CATS) found[ml_collect[i].category]++;
        for (i = 0; i < ML_CATS; i++) {
            ml_icon(i, (uint8_t)(ML_ITEMS + i), 40, (int16_t)(52 + i * 16));
            ml_puts(8, (uint8_t)(7u + i * 2u), k_cats[i], ML_TXT_WHITE);
            ml_putn(24, (uint8_t)(7u + i * 2u), found[i], 2, ML_TXT_GOLD);
            ml_puts(26, (uint8_t)(7u + i * 2u), "/", ML_TXT_GREY);
            ml_putn(27, (uint8_t)(7u + i * 2u), k_cat_goal[i], 2, ML_TXT_GREY);
        }
        ml_puts(5, 21, "FINDS FILL THE VILLAGE MUSEUM.", ML_TXT_SKY);
    } else if (tab == 2) {
        ml_puts(5, 7, "NOW:", ML_TXT_GOLD);
        ml_text_block(5, 9, goal_text(), ML_TXT_WHITE);
        if (dip(2) == 0 && ng_rpg_count(&s_g, ML_I_DIAGRAM)) {
            ml_puts(5, 15, "SLUICE DIAGRAM:", ML_TXT_GOLD);
            ml_text_block(5, 16, "OPEN THE HIGH SLUICE.\nSHUT THE SPILL. THE\nFLOODGATE RISES ONLY ON\nA DRY YARD.", ML_TXT_SKY);
        }
    } else {
        static const char *const k_sys[3] = { "SAVE THE JOURNEY", "PUT THIS PLACE BACK", "RETURN TO THE TITLE" };
        for (i = 0; i < 3u; i++) ml_puts(8, (uint8_t)(8u + i * 3u), k_sys[i], i == sel ? ML_TXT_GOLD : ML_TXT_WHITE);
        ml_puts(5, 19, "PUT THIS PLACE BACK: MOVED", ML_TXT_GREY);
        ml_puts(5, 20, "STONES AND SWITCHES RESET.", ML_TXT_GREY);
        *count = 3;
    }
}

static void NEOGEO_USER notebook(void)
{
    uint8_t tab = 2, sel = 0, count = 0, redraw = 1;
    ml_sfx(ML_SFX_MENU);
    ml_card(1);
    for (;;) {
        uint16_t p;
        if (redraw) { notebook_draw(tab, sel, &count); redraw = 0; }
        ml_frame();
        p = ml_repeat();
        if (ml_pressed() & (ML_START | ML_B)) break;
        if (p & ML_LEFT) { tab = (uint8_t)((tab + 3u) & 3u); sel = 0; redraw = 1; ml_sfx(ML_SFX_PAGE); }
        if (p & ML_RIGHT) { tab = (uint8_t)((tab + 1u) & 3u); sel = 0; redraw = 1; ml_sfx(ML_SFX_PAGE); }
        if ((p & ML_UP) && sel) { sel--; redraw = 1; }
        if ((p & ML_DOWN) && sel + 1u < count) { sel++; redraw = 1; }
        if (ml_pressed() & ML_A) {
            if (tab == 0 && count) {
                uint8_t i, row = 0, it = 0xFFu;
                for (i = 0; i < NGR_INV_SLOTS; i++) {
                    if (!s_g.inv_count[i] || s_g.inv_item[i] >= ML_ITEMS) continue;
                    if (row++ == sel) { it = s_g.inv_item[i]; break; }
                }
                if (it != 0xFFu) {
                    if (ml_items[it].flags & NGR_I_CONSUME) {
                        if (ng_rpg_use_item(&s_g, it) == NGR_OK) ml_sfx(ML_SFX_REST);
                    } else if (ml_items[it].flags & NGR_I_TOOL) {
                        s_tool = it; ml_sfx(ML_SFX_PICK);
                    }
                    redraw = 1;
                }
            } else if (tab == 3) {
                if (sel == 0) { ml_clear_text(22, 22); ml_puts_c(22, save_game() ? "SAVED." : "NO SAVE MEMORY HERE.", ML_TXT_MINT); ml_sfx(ML_SFX_REST); }
                if (sel == 1) { ng_rpg_reset_room(&s_g); ml_puts_c(22, "PUT BACK AS IT WAS.", ML_TXT_MINT); ml_sfx(ML_SFX_CLICK); }
                if (sel == 2) { s_quit = 1; break; }
            }
        }
    }
    ml_clear_text(3, 26);
    ml_icons_hide();
    ml_card(0);
    ml_hud(&s_g, s_tool, 1);
}

/* ======================================================================
 * play
 * ==================================================================== */
static void NEOGEO_USER show_node(uint16_t node)
{
    const NGRpgDialogueNode *n;
    if (node >= ML_NODES) return;
    n = &ml_dialogue[node];
    s_dlg_node = node;
    s_dlg_node_open = 1;
    s_choice = 0;
    s_choices_up = 0;
    ml_dialog(n->speaker, ml_text[n->text], 1);
    ml_sfx(ML_SFX_TALK);
}

static void NEOGEO_USER dialog_input(uint16_t pressed)
{
    const NGRpgDialogueNode *n = &ml_dialogue[s_dlg_node];
    uint8_t offered = n->nchoices ? ng_rpg_dialogue_offered(&s_g) : 0u, count = 0, i;
    for (i = 0; i < 3u; i++) if ((offered >> i) & 1u) count++;
    if (ml_dialog_typing()) {
        if (pressed & (ML_A | ML_B)) ml_dialog_finish();
        return;
    }
    if (count && !s_choices_up) {            /* the text is all out, by a press or by itself */
        s_choices_up = 1;
        ml_dialog_choices(n, offered, s_choice);
        return;
    }
    if (count) {
        if ((ml_repeat() & (ML_LEFT | ML_UP)) && s_choice) { s_choice--; ml_dialog_choices(n, offered, s_choice); }
        if ((ml_repeat() & (ML_RIGHT | ML_DOWN)) && s_choice + 1u < count) { s_choice++; ml_dialog_choices(n, offered, s_choice); }
    }
    if (pressed & ML_A) {
        ml_sfx(ML_SFX_PAGE);
        if (count) ng_rpg_dialogue_choose(&s_g, s_choice);
        else ng_rpg_dialogue_next(&s_g);
    }
}

static void NEOGEO_USER enter_room_view(void)
{
    s_shut_told = 0;
    uint8_t region_on = (uint8_t)((s_g.restored >> s_g.def->region) & 1u);
    uint8_t mus = region_on ? ml_room_meta[s_g.room].restored_music : (uint8_t)s_g.def->music;
    ml_room_show(&s_g);
    if (mus != ml_music_playing()) ml_music(mus);     /* a card's tune always gives way */
    ml_hud(&s_g, s_tool, 1);
    ml_toast(ml_text[ml_room_meta[s_g.room].title], 0, 0xFFu);
}

/* A gate whose inputs come from a plate, a pan or a balance. */
static uint8_t NEOGEO_USER held_by_weight(uint8_t gate)
{
    const NGRpgObjectDef *g = &s_g.def->objects[gate];
    uint8_t i;
    if (g->type != NGR_O_GATE || !g->in_mask) return 0;
    for (i = 0; i < s_g.def->nobjects; i++) {
        uint8_t t = s_g.def->objects[i].type;
        if ((t == NGR_O_PLATE || t == NGR_O_PAN || t == NGR_O_BALANCE) && (s_g.def->objects[i].out_mask & g->in_mask)) return 1;
    }
    return 0;
}

static void NEOGEO_USER handle(const NGRpgEvent *e)
{
    switch (e->type) {
    case NGR_EV_DIALOGUE: show_node(e->value); break;
    case NGR_EV_DIALOGUE_END:
        s_dlg_node_open = 0;
        ml_dialog_hide();
        ml_hud(&s_g, s_tool, 1);
        break;
    case NGR_EV_EXIT:
        ng_rpg_take_exit(&s_g);
        break;
    case NGR_EV_ROOM: enter_room_view(); break;
    case NGR_EV_LOCKED: if (e->value != 0xFFFFu) ng_rpg_dialogue_start(&s_g, e->value); break;
    case NGR_EV_ITEM:
        ml_sfx(ML_SFX_PICK);
        if (e->a < ML_ITEMS) ml_toast("GOT", ml_text[ml_item_text[ml_journey][e->a].title], e->a);
        break;
    case NGR_EV_FULL: ml_toast("THE BAG IS FULL", 0, 0xFFu); break;
    case NGR_EV_COLLECT:
        ml_sfx(ML_SFX_COLLECT);
        if (e->a < ML_COLLECT) ml_toast("FOUND", ml_text[ml_collect[e->a].title], (uint8_t)(ML_ITEMS + ml_collect[e->a].category));
        break;
    case NGR_EV_OPEN:
        ml_sfx(ML_SFX_GATE);
        /* a gate held up by weight dropping as someone steps off: say so, once a visit */
        if (!e->b && e->a < 32u && !((s_shut_told >> e->a) & 1u) && held_by_weight(e->a)) {
            s_shut_told |= (uint32_t)1u << e->a;
            ml_toast("IT SHUTS AGAIN: SOMEONE MUST STAY ON IT", 0, 0xFFu);
        }
        break;
    case NGR_EV_FOUND: ml_sfx(ML_SFX_SECRET); ml_toast("A HIDDEN PASSAGE!", 0, 0xFFu); break;
    case NGR_EV_PUSH: ml_sfx(ML_SFX_PUSH); break;
    case NGR_EV_FILL: ml_sfx(ML_SFX_THUD); ml_toast("THE STONE FILLS THE GAP", 0, 0xFFu); break;
    case NGR_EV_PLATE: ml_sfx(ML_SFX_CLICK); break;
    case NGR_EV_LEVER: ml_sfx(ML_SFX_LEVER); break;
    case NGR_EV_TONE:
        ml_sfx((uint8_t)(ML_SFX_CHIME0 + (e->b < 3u ? e->b : 0u)));
        ml_bell_rung(e->a);
        break;
    case NGR_EV_SEQ_WRONG: ml_sfx(ML_SFX_WRONG); break;
    case NGR_EV_SEQ_DONE: ml_sfx(ML_SFX_SOLVED); break;
    case NGR_EV_GEARS: if (e->a && !e->b) ml_sfx(ML_SFX_GEARS); if (e->b) ml_sfx(ML_SFX_WRONG); break;
    case NGR_EV_WATER: if (e->b) ml_sfx(ML_SFX_WATER); break;
    case NGR_EV_HURT: ml_sfx(ML_SFX_HURT); break;
    case NGR_EV_DOWN:
        ml_toast("A REST, THEN BACK TO IT", 0, 0xFFu);
        ml_wait(40);
        ng_rpg_respawn(&s_g);
        break;
    case NGR_EV_HIT: ml_sfx(ML_SFX_WHIP); break;
    case NGR_EV_CALMED: ml_sfx(ML_SFX_CALM); ml_toast("IT SCURRIES OFF, CALM", 0, 0xFFu); break;
    case NGR_EV_SWITCH: ml_sfx(ML_SFX_SWITCH); ml_hud(&s_g, s_tool, 0); break;
    case NGR_EV_REST:
        ml_sfx(ML_SFX_REST);
        ml_toast(save_game() ? "RESTED. JOURNEY SAVED." : "RESTED.", 0, 0xFFu);
        break;
    case NGR_EV_GAME:
        s_pending = e->a;
        break;
    default: break;
    }
}

#ifdef ML_AUTOPLAY
/*
 * A test build (ML_AUTOPLAY, see README; with ML_AUTOPLAY_LUNA, Luna's
 * journey) plays the seven chapters by itself through the real game loop, so
 * a script can watch every room, conversation, puzzle, restoration and card
 * in an emulator. The steps are the ones tools/check_world.c takes on the host
 * (it writes them, scenes/ml_autoplay_<journey>.h):
 * they put the leader on a tile (the host check proves she can walk there),
 * then act; conversations are read out at a steady pace.
 */
enum { AP_GO = 0, AP_USE, AP_PUSH, AP_ITEM, AP_LEAD, AP_ORDER, AP_EXIT, AP_IDLE, AP_HOLD, AP_CHOOSE, AP_BOARD,
       AP_UNTIL, AP_ROOM, AP_END };
typedef struct { uint8_t op, room, x, y, arg; } APStep;
static const APStep k_auto[] = {
#ifdef ML_AUTOPLAY_LUNA
#include "games/maiyaluna/scenes/ml_autoplay_luna.h"
#else
#include "games/maiyaluna/scenes/ml_autoplay_maiya.h"
#endif
    { AP_END, 0, 0, 0, 0 },
};
static uint16_t s_ap, s_ap_t;      /* the step, and frames into it (not counting conversations) */
static uint8_t s_ap_pick;          /* a CHOOSE step's pick + 1, for the next choice offered */
static uint8_t s_ap_read;          /* paces A through a conversation */

/* the party member with this id (ML_MAIYA, ML_LUNA); the leader when she isn't there */
static uint8_t NEOGEO_USER ap_member(uint8_t id)
{
    uint8_t i;
    for (i = 0; i < s_g.nparty; i++) if (s_g.party[i].id == id) return i;
    return s_g.leader;
}

/* the leader on (x, y), with those following her (as the host check puts them) */
static void NEOGEO_USER ap_place(uint8_t x, uint8_t y, uint8_t face)
{
    int32_t px = (int32_t)(((uint16_t)x * 16u + 8u) << 8), py = (int32_t)(((uint16_t)y * 16u + 8u) << 8);
    uint8_t i;
    for (i = 0; i < s_g.nparty; i++) {
        if (i != s_g.leader && s_g.party[i].mode == NGR_MODE_WAIT) continue;
        s_g.party[i].x = px;
        s_g.party[i].y = py;
    }
    s_g.party[s_g.leader].facing = face;
}

static uint8_t NEOGEO_USER ap_at(uint8_t x, uint8_t y)
{
    const NGRpgMember *m = &s_g.party[s_g.leader];
    return (uint8_t)((uint8_t)(m->x >> 12) == x && (uint8_t)(m->y >> 12) == y);
}

/* the autopilot's input for this frame: directions, and A when it reads */
static uint16_t NEOGEO_USER autopilot(void)
{
    const APStep *st = &k_auto[s_ap];
    if (s_dlg_node_open) {
        /* at a choice: the CHOOSE step's pick, or the next step's when it is one (the
         * conversation came back to the choice), else the first */
        const NGRpgDialogueNode *n = &ml_dialogue[s_dlg_node];
        uint8_t offered = n->nchoices ? ng_rpg_dialogue_offered(&s_g) : 0u, count = 0, i;
        for (i = 0; i < 3u; i++) if ((offered >> i) & 1u) count++;
        if ((++s_ap_read & 15u) != 0) return 0;
        if (count && !ml_dialog_typing()) {
            if (!s_ap_pick && st->op == AP_CHOOSE) { s_ap_pick = (uint8_t)(st->arg + 1u); s_ap++; s_ap_t = 0; }
            s_choice = (uint8_t)(s_ap_pick && s_ap_pick <= count ? s_ap_pick - 1u : 0u);
            s_ap_pick = 0;
            ml_dialog_choices(n, offered, s_choice);
        }
        return ML_A;
    }
    s_ap_t++;
    if (st->op == AP_END) return 0;
    if (st->op == AP_EXIT && s_g.room != st->room && s_ap_t > 2u) { s_ap++; s_ap_t = 0; return 0; }   /* through */
    if (st->op == AP_ROOM) {
        if (s_g.room == st->room && ap_at(st->x, st->y) && s_ap_t > 30u) { s_ap++; s_ap_t = 0; }
        return 0;
    }
    if (st->op != AP_IDLE && s_g.room != st->room) return 0;     /* a fade, or a scene, still going */
    switch (st->op) {
    case AP_IDLE:
        if (s_ap_t < st->arg) return 0;
        break;
    case AP_HOLD:
        if (s_ap_t <= st->arg) return st->x;
        break;
    case AP_GO:
        if (s_ap_t < ((st->arg & 0x80u) ? 1u : 20u)) return 0;
        ap_place(st->x, st->y, (uint8_t)(st->arg & 0x7Fu));
        break;
    case AP_USE:
        if (s_ap_t < 10u) return 0;
        s_ap++; s_ap_t = 0;
        return ML_A;
    case AP_CHOOSE:
        if (s_ap_t < 10u) return 0;
        s_ap_pick = (uint8_t)(st->arg + 1u);
        s_ap++; s_ap_t = 0;
        return ML_A;
    case AP_ITEM:
        if (s_ap_t < 10u) return 0;
        ng_rpg_use_item(&s_g, st->arg);
        break;
    case AP_LEAD:
        if (s_g.party[s_g.leader].id != st->arg) ng_rpg_switch(&s_g, ap_member(st->arg));
        break;
    case AP_ORDER:
        ng_rpg_order(&s_g, ap_member(st->arg), st->x, 0, 0);
        break;
    case AP_PUSH: {
        /* stand behind the block at (x, y) and lean until it moves */
        int8_t dx = st->arg == NGR_IN_RIGHT ? 1 : st->arg == NGR_IN_LEFT ? -1 : 0;
        int8_t dy = st->arg == NGR_IN_DOWN ? 1 : st->arg == NGR_IN_UP ? -1 : 0;
        uint8_t o = ng_rpg_object_at(&s_g, st->x, st->y);
        uint8_t face = st->arg == NGR_IN_RIGHT ? NGR_FACE_RIGHT : st->arg == NGR_IN_LEFT ? NGR_FACE_LEFT
                     : st->arg == NGR_IN_DOWN ? NGR_FACE_DOWN : NGR_FACE_UP;
        if (s_ap_t == 1u) {
            ap_place((uint8_t)(st->x - dx), (uint8_t)(st->y - dy), face);
            return 0;
        }
        if (o != NGR_NONE && s_ap_t < 120u) return st->arg;
        if (s_ap_t < 140u) return 0;
        break;
    }
    case AP_BOARD: {
        /* on the platform the moment it waits at (x, y) */
        const NGRpgObject *o = &s_g.obj[st->arg];
        if (!((uint8_t)(o->px >> 12) == st->x && (uint8_t)(o->py >> 12) == st->y && o->timer)) return 0;
        ap_place(st->x, st->y, NGR_FACE_DOWN);
        break;
    }
    case AP_UNTIL:
        if (!ap_at(st->x, st->y)) return 0;
        break;
    case AP_EXIT:
        if (s_ap_t == 1u) {
            uint8_t face = st->arg == NGR_IN_RIGHT ? NGR_FACE_RIGHT : st->arg == NGR_IN_LEFT ? NGR_FACE_LEFT
                         : st->arg == NGR_IN_DOWN ? NGR_FACE_DOWN : NGR_FACE_UP;
            ap_place(st->x, st->y, face);
            return 0;
        }
        if (s_ap_t < 200u) return st->arg;
        s_ap_t = 0;                       /* not through: walk up to it again */
        return 0;
    default: break;
    }
    s_ap++;
    s_ap_t = 0;
    return 0;
}
#endif

/* The object the leader faces (NGR_NONE: none). */
static uint8_t NEOGEO_USER faced_object(void)
{
    const NGRpgMember *m = &s_g.party[s_g.leader];
    int8_t dx = 0, dy = 0;
    switch (m->facing) { case NGR_FACE_RIGHT: dx = 1; break; case NGR_FACE_LEFT: dx = -1; break; case NGR_FACE_UP: dy = -1; break; default: dy = 1; break; }
    return ng_rpg_object_at(&s_g, (uint8_t)((m->x >> 12) + dx), (uint8_t)((m->y >> 12) + dy));
}

/* A refused A: say why, so nothing just stays silent. */
static void NEOGEO_USER refused(void)
{
    uint8_t o = faced_object();
    const NGRpgObjectDef *d;
    if (o == NGR_NONE) return;
    d = &s_g.def->objects[o];
    if (d->type == NGR_O_LOCK && d->text != 0xFFFFu) return;          /* it said so itself */
    if (s_g.party[s_g.leader].id == ML_MAIYA && (d->p[6] & 1u)) {
        ml_toast(s_g.nparty > 1u ? "A JOB FOR LUNA'S STEADY HANDS (D)" : "TOO FIDDLY FOR MAIYA'S BIG HANDS", 0, 0xFFu);
        return;
    }
    if (ml_room_hints[ml_journey][s_g.room] && ml_room_hints[ml_journey][s_g.room][o] != 0xFFFFu) { ng_rpg_dialogue_start(&s_g, ml_room_hints[ml_journey][s_g.room][o]); return; }
    ml_toast("NOT YET: SOMETHING IS MISSING", 0, 0xFFu);
}

/* Who stands on a plate or a pan (they hold it down). */
static uint8_t NEOGEO_USER on_weight(uint8_t member)
{
    const NGRpgMember *m = &s_g.party[member];
    uint8_t o = ng_rpg_object_at(&s_g, (uint8_t)(m->x >> 12), (uint8_t)(m->y >> 12));
    return (uint8_t)(o != NGR_NONE && (s_g.def->objects[o].type == NGR_O_PLATE || s_g.def->objects[o].type == NGR_O_PAN));
}

#ifdef ML_TOUR
/* A test build (ML_TOUR) visits every room in turn, a few seconds each, at
 * the spot an exit into it leads to, so an emulator script can photograph
 * each room as the game draws it. */
static void NEOGEO_USER tour_step(void)
{
    static uint16_t t;
    static uint8_t next = 1;
    uint8_t r, e;
    if (++t < 240u) return;
    t = 0;
    for (r = 0; r < ML_ROOMS; r++)
        for (e = 0; e < ml_rooms[ml_journey][r].nexits; e++)
            if (ml_rooms[ml_journey][r].exits[e].room == next) {
                const NGRpgExit *x = &ml_rooms[ml_journey][r].exits[e];
                ng_rpg_enter_room(&s_g, next, x->to_x, x->to_y, x->facing);
                next = (uint8_t)(next + 1u < ML_ROOMS ? next + 1u : 0u);
                return;
            }
    next = (uint8_t)(next + 1u < ML_ROOMS ? next + 1u : 0u);
}
#endif

static void NEOGEO_USER play(void)
{
    s_quit = 0;
    s_d_held = 0;
    for (;;) {
        NGRpgEvent e;
        uint16_t held, pressed;
        ml_frame();
#ifdef ML_TOUR
        tour_step();
        if (s_g.dlg_open) ng_rpg_dialogue_next(&s_g);
#endif
        held = ml_held(); pressed = ml_pressed();
#ifdef ML_AUTOPLAY
        {
            static uint16_t last;
            held = autopilot();
            pressed = (uint16_t)(held & (uint16_t)~last);
            last = held;
        }
#endif
        if (s_dlg_node_open && s_g.dlg_open) {
            dialog_input(pressed);
            ng_rpg_input(&s_g, 0);
        } else {
            ng_rpg_input(&s_g, (uint8_t)(held & ML_DIRS));
            if (pressed & ML_A) {
                int r = ng_rpg_interact(&s_g);
                if (r == NGR_OK) ml_flash_member(s_g.leader);
                else if (r == NGR_ERR_NOPE) refused();
            }
            /* Luna leaning on a stone she can't move */
            if (s_g.party[s_g.leader].id == ML_LUNA && (held & ML_DIRS)) {
                uint8_t o = faced_object();
                if (o != NGR_NONE && s_g.def->objects[o].type == NGR_O_BLOCK && s_g.def->objects[o].p[0]) {
                    if (++s_heavy_t == 40u)
                        ml_toast(s_g.nparty > 1u ? "TOO HEAVY FOR LUNA: MAIYA CAN PUSH IT" : "FAR TOO HEAVY FOR LUNA", 0, 0xFFu);
                } else s_heavy_t = 0;
            } else s_heavy_t = 0;
            if (pressed & ML_B) {
                if (s_g.party[s_g.leader].id == ML_MAIYA) {
                    ml_member_pose(s_g.leader, ML_POSE_WHIP);
                    ml_sfx(ML_SFX_WHIP);
                    ng_rpg_strike(&s_g);
                } else {
                    /* Luna's close look: what she faces */
                    const NGRpgMember *m = &s_g.party[s_g.leader];
                    int8_t dx = 0, dy = 0;
                    uint8_t o;
                    ml_member_pose(s_g.leader, ML_POSE_LOOK);
                    switch (m->facing) { case NGR_FACE_RIGHT: dx = 1; break; case NGR_FACE_LEFT: dx = -1; break; case NGR_FACE_UP: dy = -1; break; default: dy = 1; break; }
                    o = ng_rpg_object_at(&s_g, (uint8_t)((m->x >> 12) + dx), (uint8_t)((m->y >> 12) + dy));
                    if (o != NGR_NONE && ml_room_hints[ml_journey][s_g.room] && ml_room_hints[ml_journey][s_g.room][o] != 0xFFFFu)
                        ng_rpg_dialogue_start(&s_g, ml_room_hints[ml_journey][s_g.room][o]);
                    else ml_toast("NOTHING UNUSUAL HERE", 0, 0xFFu);
                }
            }
            if (pressed & ML_C) {
                if (ng_rpg_use_item(&s_g, s_tool) == NGR_OK) ml_flash_member(s_g.leader);
                else ml_toast("THAT WON'T HELP HERE", 0, 0xFFu);
            }
            /* D (both sisters together): tap to switch, hold to make the other one wait or follow */
            if ((held & ML_D) && s_g.nparty > 1u) {
                if (++s_d_held == 24u) {
                    uint8_t other = (uint8_t)(s_g.leader ^ 1u);
                    if (s_g.party[other].mode == NGR_MODE_WAIT) {
                        ng_rpg_order(&s_g, other, NGR_MODE_FOLLOW, 0, 0);
                        ml_toast(s_g.party[other].id == ML_LUNA ? "LUNA FOLLOWS" : "MAIYA FOLLOWS", 0, 0xFFu);
                    } else {
                        ng_rpg_order(&s_g, other, NGR_MODE_WAIT, 0, 0);
                        ml_toast(s_g.party[other].id == ML_LUNA ? "LUNA WAITS HERE" : "MAIYA WAITS HERE", 0, 0xFFu);
                    }
                    ml_sfx(ML_SFX_CLICK);
                }
            } else {
                if (s_d_held && s_d_held < 24u) {
                    uint8_t was = s_g.leader;
                    if (ng_rpg_switch(&s_g, NGR_NONE) == NGR_OK) {
                        /* the one left on a plate or a pan stays and holds it; otherwise she follows */
                        if (on_weight(was)) {
                            ng_rpg_order(&s_g, was, NGR_MODE_WAIT, 0, 0);
                            ml_toast(s_g.party[was].id == ML_LUNA ? "LUNA STAYS ON IT" : "MAIYA STAYS ON IT", 0, 0xFFu);
                        }
                    }
                }
                s_d_held = 0;
            }
            if (pressed & ML_START) { notebook(); if (s_quit) return; }
        }
        ng_rpg_update(&s_g);
        while (ng_rpg_event(&s_g, &e)) handle(&e);
        if (s_pending && !s_g.dlg_open) {
            uint8_t ev = s_pending;
            s_pending = 0;
            if (ev == ML_EV_RESTORATION) restoration();
            else if (ev == ML_EV_CHAPTER_END) chapter_end();
        }
        ml_room_draw(&s_g);
        ml_hud(&s_g, s_tool, 0);
        ml_toast_tick();
    }
}

static uint8_t NEOGEO_USER start_menu(void)
{
    uint8_t sel = 0, has = saved_game();
    if (!has) return 0;
    ml_picture(ML_ART_BG_TITLE);
    ml_logo(1, 32, 8);
    ml_card(0);
    for (;;) {
        ml_puts_c(12, "NEW JOURNEY", sel == 0 ? ML_TXT_GOLD : ML_TXT_GREY);
        ml_puts_c(14, "CONTINUE THE JOURNEY", sel == 1 ? ML_TXT_GOLD : ML_TXT_GREY);
        ml_frame();
        if (ml_repeat() & (ML_UP | ML_DOWN)) { sel ^= 1u; ml_sfx(ML_SFX_PAGE); }
        if (ml_pressed() & (ML_A | ML_START)) { ml_sfx(ML_SFX_START); return sel; }
    }
}

/* Who sets out: the player travels as one sister. */
static uint8_t NEOGEO_USER choose_sister(void)
{
    static const char *const k_name[2] = { "MAIYA", "LUNA" };
    static const char *const k_what[2][2] = {
        { "BRAVE AND STRONG: SHE CLIMBS IVY,", "PUSHES STONES AND SHOOS CREATURES." },
        { "CLEVER AND CAREFUL: SHE SLIPS", "THROUGH GAPS AND WORKS FINE GEARS." } };
    uint8_t sel = 0, t = 0, frame = 0, redraw = 1;
    ml_picture(ML_ART_BG_TITLE);
    ml_card(1);
    ml_puts_c(4, "WHO SETS OUT?", ML_TXT_GOLD);
    ml_puts_c(25, "LEFT / RIGHT: CHOOSE    A: SET OUT", ML_TXT_GREY);
    for (;;) {
#ifdef ML_AUTOPLAY_LUNA
        sel = ML_J_LUNA;
#endif
        if (redraw & 1u) {
            ml_clear_text(17, 21);
            ml_puts(10, 17, k_name[0], sel == 0 ? ML_TXT_GOLD : ML_TXT_GREY);
            ml_puts(26, 17, k_name[1], sel == 1 ? ML_TXT_GOLD : ML_TXT_GREY);
            ml_puts_c(19, k_what[sel][0], ML_TXT_WHITE);
            ml_puts_c(20, k_what[sel][1], ML_TXT_WHITE);
        }
        if (++t >= 10u) { t = 0; frame = (uint8_t)(frame >= 2u ? 0u : frame + 1u); redraw |= 2u; }
        if (redraw) {                        /* (a person is a whole sprite map: only when her picture changes) */
            ml_person(0, ML_LOOK_MAIYA, sel == 0 ? frame : 0u, 0, 52, 64);
            ml_person(1, ML_LOOK_LUNA, sel == 1 ? frame : 0u, 1, 188, 64);
            redraw = 0;
        }
        ml_frame();
#ifdef ML_AUTOPLAY
        if (ml_pressed() & (ML_A | ML_START)) { ml_person(0, 0, 0, 0, 0, 0); ml_person(1, 0, 0, 0, 0, 0); return sel; }
        continue;
#endif
        if (ml_repeat() & (ML_LEFT | ML_RIGHT)) { sel ^= 1u; redraw = 1; ml_sfx(ML_SFX_PAGE); }
        if (ml_pressed() & (ML_A | ML_START)) {
            ml_sfx(ML_SFX_START);
            ml_person(0, 0, 0, 0, 0, 0);
            ml_person(1, 0, 0, 0, 0, 0);
            return sel;
        }
    }
}

void NEOGEO_USER ml_run(void)
{
    uint8_t cont;
    cont = start_menu();
    s_pending = 0;
    s_dlg_node_open = 0;
    s_tool = ML_I_LENS;
    if (cont && load_game()) {
        /* load put the party in the saved room */
    } else {
        ml_journey = choose_sister();
        ng_rpg_init(&s_g, &ml_worlds[ml_journey], (((uint32_t)ml_rand() << 16) ^ ml_rand()) | 1u);
        ng_rpg_party_add(&s_g, &ml_party[ml_journey]);
        party_for(0);
        if (dip(0) == 0) own_how_to_play(900);
        prologue(0);
        chapter_card(0);
        ng_rpg_enter_room(&s_g, ml_chapters[ml_journey][0].start_room, ml_chapters[ml_journey][0].start_x, ml_chapters[ml_journey][0].start_y,
                          ml_chapters[ml_journey][0].start_face);
    }
    {
        NGRpgEvent e;
        /* the first room's own events (its arrival, a trigger's conversation) */
        enter_room_view();
        while (ng_rpg_event(&s_g, &e)) if (e.type != NGR_EV_ROOM) handle(&e);
    }
    play();
}
