/* Exercise real deferred rendering against the host VRAM fixture. */
static void advance_raster(void);
#define NG_TEST_VRAM_WRITE() advance_raster()
#define main sprite_immediate_test_main
#include "sprite_render_test.c"
#undef main
#include "../sdk/2d_engine/ng_sprite_group.c"

uint16_t ng_vram_test_line = 240u;
static unsigned words_per_line, raster_words;
static void advance_raster(void)
{
    if (words_per_line && ++raster_words >= words_per_line) {
        raster_words = 0u;
        ng_vram_test_line = (uint16_t)((ng_vram_test_line + 1u) % 264u);
    }
}
void ng_render_queue_flush(void) {}
void ng_fix_commit(void) {}

static unsigned pending(void)
{
    return ngsg_high_n + ngsg_norm_n + ngsg_low_n;
}

static void test_pending_rebind_is_unique(void)
{
    NGSpriteGroup g;
    unsigned frame;
    ngsg_queue_drop();
    memset(ram, 0, sizeof(ram));
    ng_vram_test_line = 100u; /* No upload budget. */
    for (frame = 0; frame < 200; frame++) {
        ng_sprite_group_init(&g, 96u, 3u, 3u, (uint16_t)(100 + frame), 20u);
        ng_sprite_group_flush(&g);
        assert(pending() == 1u);
        assert(g.dirty & NG_SGF_QUEUED);
    }
    ng_vram_test_line = 240u;
    ng_vram_commit();
    assert(pending() == 0u);
    assert(map_tile(96, 0) == 299u);
    assert(act_of(96) == 3u);
}

static void test_reused_hidden_window_restores_stationary_owner(void)
{
    NGCharacter *a, *b;
    ngsg_queue_drop();
    memset(ram, 0, sizeof(ram));
    ng_chars_init();
    a = chars_add(1u, 40, 40);
    b = chars_add(2u, 80, 80);
    ng_char_set_sprite(a, 0u, 3u, 3u, 100u, 20u);
    ng_char_set_sprite(b, 0u, 3u, 3u, 200u, 21u);
    ng_chars_draw();
    ng_vram_commit();
    assert(b->sprite_first == 99u);

    /* Hide the old owner, relocate the survivor, then reuse that old
     * uploaded window before its deferred hide has reached hardware. */
    ng_chars_remove(a);
    ng_chars_draw();
    a = chars_add(1u, 40, 40);
    ng_char_set_sprite(a, 0u, 3u, 3u, 300u, 20u);
    ng_chars_draw();
    ng_vram_commit();
    assert(act_of(a->sprite_first) == 3u);
    assert(act_of(b->sprite_first) == 3u);
    assert(ram[SCB2_ADDR + b->sprite_first] == 0x0fffu);
    assert((ram[SCB3_ADDR + b->sprite_first + 1] & 0x40u) != 0u);
    assert(map_tile(b->sprite_first, 0) == 200u);
}

static void test_aged_write_keeps_membership_until_compaction(void)
{
    NGSpriteGroup g;
    ngsg_queue_drop();
    ng_sprite_group_init(&g, 96u, 4u, 4u, 400u, 20u);
    ng_sprite_group_flush(&g);
    ngsg_head[1] = &g;
    ngsg_head_age[1] = 3u;
    ngsg_commit_aged();
    /* The aged upload used the remaining blank; its list is not compacted. */
    ng_vram_test_line = 100u;
    ngsg_commit_list(ngsg_norm, &ngsg_norm_n);
    assert(pending() == 1u);
    assert(g.dirty & NG_SGF_QUEUED);
    ng_sprite_group_set_tile_base(&g, 500u);
    ng_sprite_group_flush(&g);
    assert(pending() == 1u);
    ng_vram_test_line = 240u;
    ng_vram_commit();
    assert(pending() == 0u);
    assert(map_tile(96, 0) == 500u);
}

/* A group still listed (the blank ran out) is uploaded at once -- a shadow
 * shown again -- then changes and flushes before the next commit. It must
 * stay listed once: a second entry is written twice and fills the list. */
static void test_upload_keeps_membership(void)
{
    NGSpriteGroup g;
    ngsg_queue_drop();
    ng_sprite_group_init(&g, 96u, 3u, 3u, 600u, 20u);
    ng_sprite_group_flush(&g);
    ng_vram_test_line = 100u;
    ngsg_commit_list(ngsg_norm, &ngsg_norm_n);
    assert(pending() == 1u);
    ng_vram_test_line = 240u;
    ng_sprite_group_upload(&g);
    ng_sprite_group_set_tile_base(&g, 700u);
    ng_sprite_group_flush(&g);
    assert(pending() == 1u);
    ng_vram_commit();
    assert(pending() == 0u);
    assert(map_tile(96, 0) == 700u);
}

/* A creature ahead of the player in the draw order leaves while the blank
 * has no room for uploads. Sorted, the player moves down into new slots
 * that stay unwritten -- hidden -- until a later blank; pinned, she keeps
 * her slots and stays on screen. */
static uint16_t player_after_creature_leaves(uint8_t pin)
{
    NGCharacter *creature, *player;
    uint16_t first;
    ngsg_queue_drop();
    memset(ram, 0, sizeof(ram));
    ng_chars_init();
    creature = chars_add(1u, 40, 40);
    player = chars_add(0u, 120, 80);
    ng_char_set_sprite(creature, 0u, 3u, 3u, 100u, 20u);
    ng_char_set_sprite(player, 0u, 5u, 4u, 400u, 21u);
    ng_char_set_priority(creature, NG_RENDER_BAND_ENEMY, 0);
    ng_char_set_priority(player, NG_RENDER_BAND_PLAYER, 0);
    if (pin) ng_char_pin_sprite(player, (uint16_t)(NG_SPR_CHAR_LAST + 1u - 5u));
    ng_vram_test_line = 240u;
    ng_chars_draw();
    ng_vram_commit();
    first = player->sprite_first;
    assert(act_of(first) == 4u && map_tile(first, 0) == 400u);
    assert(act_of(creature->sprite_first) == 3u);
    if (pin) assert(first == NG_SPR_CHAR_LAST + 1u - 5u && creature->sprite_first < first);
    else assert(first > creature->sprite_first);

    ng_chars_remove(creature);
    ng_vram_test_line = 100u;   /* no room in this blank */
    ng_chars_draw();
    ng_vram_commit();
    return (uint16_t)(act_of(player->sprite_first) == 4u && map_tile(player->sprite_first, 0) == 400u &&
                      player->sprite_first == first);
}

static void test_pinned_player_stays_when_a_creature_leaves(void)
{
    assert(!player_after_creature_leaves(0u));   /* sorted: she moves and blinks out */
    assert(player_after_creature_leaves(1u));    /* pinned: same slots, still drawn */
}

static void test_deferred_large_rom_map_keeps_latest_address(void)
{
    NGSpriteGroup g;
    static const uint8_t banks[2] = {64u, 67u};
    ngsg_queue_drop(); memset(ram, 0, sizeof(ram));
    ng_vram_test_line = 100u;
    ng_sprite_group_init(&g, 300u, 1u, 2u, 10u, 7u);
    ng_sprite_group_set_palette_map(&g, banks);
    ng_sprite_group_set_tile_address(&g, 0x1FFFEu);
    ng_sprite_group_flush(&g);ng_vram_commit();
    assert(pending() == 1u && ram[300u * 64u] == 0u);
    ng_sprite_group_set_tile_address(&g, 0x1FFFFu);
    ng_sprite_group_flush(&g);assert(pending() == 1u);
    ng_vram_test_line = 240u;ng_vram_commit();
    assert(pending() == 0u);
    assert(ram[300u * 64u] == 0xFFFFu && ram[300u * 64u + 1u] == 0x4010u);
    assert(ram[300u * 64u + 2u] == 0u && ram[300u * 64u + 3u] == 0x4320u);
    assert(ram[300u * 64u + 5u] == NG_SPRITE_BLANK_ATTR);
}

static void test_aged_content_obeys_deadline(void)
{
    NGSpriteGroup g;
    unsigned before;
    ngsg_queue_drop();
    ng_sprite_group_init(&g, 96u, 4u, 4u, 900u, 20u);
    ng_sprite_group_flush(&g);
    ngsg_head[1] = &g; ngsg_head_age[1] = 9u;
    before = writes;
    ng_vram_test_line = 100u;
    ngsg_commit_aged();
    assert(writes == before && pending() == 1u);
    ng_vram_test_line = 7u;
    ngsg_commit_aged();
    assert(writes == before && (g.dirty & NG_SGF_DIRTY_TILE));
    /* Being old must not let a full map start in a two-line tail. */
    ng_vram_test_line = NG_VRAM_DEADLINE - 2u;
    ngsg_commit_aged();
    assert(writes == before && pending() == 1u);
    assert(ngsg_head[1] == &g && (g.dirty & NG_SGF_DIRTY_TILE));
    ng_vram_test_line = 240u;
    ng_vram_commit();
    assert(pending() == 0u && map_tile(96, 0) == 900u);
    ngsg_head[1] = &g; ngsg_head_age[1] = 9u;
    ngsg_queue_drop();
    assert(!ngsg_head[1] && !ngsg_head_age[1]);
}

static void test_bounded_removal_and_slot_reuse(void)
{
    NGSpriteGroup g;
    unsigned before;
    ngsg_queue_drop(); memset(ram, 0, sizeof(ram));
    ng_vram_test_line = 240u;
    ng_sprite_group_init(&g, 96u, 40u, 1u, 1000u, 20u);
    ng_sprite_group_upload(&g);
    before = writes;
    ng_vram_test_line = 100u;
    ng_sprite_group_hide_queued(&g);
    assert(writes == before && !g.readyRows && !g.visible);
    ng_sprite_group_init(&g, 120u, 3u, 3u, 1100u, 21u);
    ng_sprite_group_flush(&g);
    ng_vram_commit();
    assert(writes == before && pending() == 1u);
    /* A simulated tight blank permits one eight-strip hide batch. The
     * replacement belongs to the part still waiting, so it cannot publish. */
    words_per_line = 24u; raster_words = 0u;
    ng_vram_test_line = 4u;
    ng_vram_commit();
    assert(ngsg_hide_any && ngsg_waiting_hide(&g));
    assert(writes == before + 24u && map_tile(120, 0) != 1100u);
    words_per_line = 0u;
    ng_vram_test_line = 240u;
    ng_vram_commit();
    assert(!ngsg_hide_any && pending() == 0u);
    assert(map_tile(120, 0) == 1100u && act_of(120) == 3u);
    before = writes;
    ng_vram_commit();
    assert(writes == before); /* No old hide can erase the replacement. */
}

static void test_cancel_drops_owned_streaming_columns(void)
{
    NGSpriteGroup a, b;
    ngsg_queue_drop();
    ng_sprite_group_init(&a, 96u, 3u, 3u, 1200u, 20u);
    ng_sprite_group_init(&b, 100u, 3u, 3u, 1300u, 21u);
    ngsg_list_strip(&a, 0u, 19u);
    ngsg_list_strip(&b, 0u, 20u);
    ngsg_list_strip(&a, 1u, 21u);
    ng_sprite_group_flush(&a);
    ng_sprite_group_cancel(&a);
    assert(ngsg_strips_n == 1u && ngsg_strips[0].g == &b);
    assert(pending() == 1u && (a.dirty & NG_SGF_QUEUED));
    ngsg_queue_drop();
}

static void test_hidden_map_staging_keeps_live_pose(void)
{
    NGSpriteGroup live, next;
    uint8_t column = 0u;
    unsigned before;
    ngsg_queue_drop(); memset(ram, 0, sizeof(ram));
    ng_vram_test_line = 240u;
    ng_sprite_group_init(&live, 96u, 8u, 8u, 2000u, 20u);
    ng_sprite_group_upload(&live);
    ng_sprite_group_init(&next, 176u, 8u, 8u, 3000u, 21u);
    ng_sprite_group_set_flip(&next, 1u, 0u);
    before = writes;
    ng_vram_test_line = NG_VRAM_DEADLINE - 7u;
    assert(!ng_sprite_group_stage_map(&next, &column));
    assert(column == 0u && writes == before);
    assert(!ng_sprite_group_present_staged(&next));
    words_per_line = 24u; raster_words = 0u;
    ng_vram_test_line = 240u;
    while (!ng_sprite_group_stage_map(&next, &column)) {
        assert(column < next.strips && column > 0u);
        assert(act_of(live.firstSprite) == 8u);
        assert(act_of(next.firstSprite) == 0u);
        assert(!ng_sprite_group_present_staged(&next));
        ng_vram_test_line = 240u;
    }
    assert(act_of(next.firstSprite) == 0u && act_of(live.firstSprite) == 8u);
    assert(map_tile(176u, 0u) == 3007u);
    assert(map_tile(183u, 7u) == 3056u);
    assert(map_tile(176u, 8u) == NG_SPRITE_BLANK_TILE);
    before = writes;
    ng_vram_test_line = NG_VRAM_DEADLINE - 2u;
    assert(!ng_sprite_group_present_staged(&next) && writes == before);
    words_per_line = 0u; ng_vram_test_line = 240u;
    assert(ng_sprite_group_present_staged(&next));
    ng_sprite_park_off_range(live.firstSprite, live.strips);
    assert(act_of(live.firstSprite) == 0u && act_of(next.firstSprite) == 8u);
    assert(next.readyRows == 8u && !next.dirty);
}

static void test_staging_detaches_previous_queue_owner(void)
{
    NGSpriteGroup g, other;
    uint8_t column = 0u;
    ngsg_queue_drop(); memset(ram, 0, sizeof(ram));
    ng_sprite_group_init(&g, 96u, 8u, 8u, 2000u, 20u);
    ng_sprite_group_init(&other, 176u, 8u, 8u, 3000u, 21u);
    ng_sprite_group_flush(&g); ng_sprite_group_flush(&other);
    ngsg_list_strip(&g, 0u, 2u);
    ngsg_head[1] = &g; ngsg_head_age[1] = 3u;
    ng_sprite_group_detach(&g);
    assert(pending() == 1u && ngsg_norm[0] == &other);
    assert(!g.dirty && !ngsg_strips_n && !ngsg_head[1]);
    ng_sprite_group_init(&g, 96u, 8u, 8u, 4000u, 20u);
    ng_vram_test_line = 240u;
    assert(ng_sprite_group_stage_map(&g, &column));
    /* A generic commit must not enable this descriptor while its new
     * owner is still assembling a multi-group presentation. */
    ng_vram_commit();
    assert(!act_of(96u) && act_of(176u) == 8u);
    assert(ng_sprite_group_present_staged(&g));
    assert(ng_sprite_stage_retire_range(176u, 8u));
    assert(act_of(96u) == 8u && !act_of(176u));
    assert(!(ram[SCB3_ADDR + 177u] & 0x40u));
}

static void test_whole_bank_cutover_is_bounded(void)
{
    uint16_t scale[80], pos[80], x[80];
    unsigned i, before;
    memset(ram, 0, sizeof(ram));
    for (i = 0u; i < 80u; i++) {
        scale[i] = 0x0FFFu; pos[i] = i ? 0x48u : 0xA008u; x[i] = 0x4000u;
        ram[SCB3_ADDR + 77u + i] = pos[i];
    }
    before = writes;
    /* A short safe tail must leave BOTH banks untouched. */
    ng_vram_test_line = 249u;
    assert(!ng_sprite_stage_swap(77u, 80u, 157u, 80u, scale, pos, x));
    assert(writes == before && act_of(77u) == 8u && !act_of(157u));
    words_per_line = 24u; raster_words = 0u; ng_vram_test_line = 240u;
    assert(ng_sprite_stage_swap(77u, 80u, 157u, 80u, scale, pos, x));
    assert(ng_vram_window_open() && !act_of(77u) && act_of(157u) == 8u);
    for (i = 0u; i < 80u; i++) {
        assert(!(ram[SCB3_ADDR + 77u + i] & 0x7Fu));
        assert(ram[SCB2_ADDR + 157u + i] == scale[i]);
        assert(ram[SCB3_ADDR + 157u + i] == pos[i]);
        assert(ram[SCB4_ADDR + 157u + i] == x[i]);
    }
    before = writes;
    assert(!ng_sprite_stage_swap(77u, 80u, 100u, 80u, scale, pos, x));
    assert(writes == before); /* Overlap cannot retire live staging data. */
    words_per_line = 0u;
}

static void test_staged_map_crosses_64k_tiles_without_wrapping(void)
{
    NGSpriteGroup g;
    uint8_t column = 0u;
    unsigned before;
    ngsg_queue_drop(); memset(ram, 0, sizeof(ram));
    ng_sprite_group_init(&g, 96u, 2u, 2u, 0xFFFFu, 21u);
    ng_sprite_group_set_flip(&g, 1u, 0u);
    before = writes; ng_vram_test_line = 261u; /* Eleven lines, wide path needs twelve. */
    assert(!ng_sprite_group_stage_map(&g, &column));
    assert(!column && writes == before);
    ng_vram_test_line = 240u;
    assert(ng_sprite_group_stage_map(&g, &column));
    assert(map_tile(96u, 0u) == 0u && map_tile(96u, 1u) == 2u);
    assert(ram[(96u << 6) + 1u] == (uint16_t)((21u << 8) | 0x11u));
    assert(map_tile(97u, 0u) == 0xFFFFu && map_tile(97u, 1u) == 1u);
    assert(ram[(97u << 6) + 1u] == (uint16_t)((21u << 8) | 1u));
    assert(ram[(97u << 6) + 3u] == (uint16_t)((21u << 8) | 0x11u));
    assert(map_tile(96u, 2u) == NG_SPRITE_BLANK_TILE);
    assert(!act_of(96u));
}

static void test_move_only_character_model_lifecycle(void)
{
    NGCharacter *c;
    uint16_t first;
    unsigned before;
    ngsg_queue_drop(); memset(ram, 0, sizeof(ram));
    ng_vram_test_line = 240u;
    ng_chars_init();
    c = chars_add(1u, 50, 60);
    ng_char_set_sprite(c, 0u, 1u, 1u, 100u, 20u);
    ng_char_set_render_mode(c, NG_CHAR_RENDER_MOVE_ONLY);
    ng_chars_draw(); ng_vram_commit();
    first = c->sprite_first;
    assert(map_tile(first, 0u) == 100u && act_of(first) == 1u);
    /* A stationary frame does no hardware work. Movement needs controls,
     * not another map; poison one map word to detect accidental reupload. */
    before = writes;
    ng_chars_draw(); ng_vram_commit();
    assert(writes == before);
    ram[first * 64u] = 0x1234u;
    c->x += 3; c->sprite_offset_x = 2;
    ng_chars_draw(); ng_vram_commit();
    assert(ram[SCB4_ADDR + first] == setSCB4(55u));
    assert(map_tile(first, 0u) == 0x1234u && writes > before);

    /* Setters and explicit raw invalidation still update the whole model. */
    ng_char_set_sprite(c, 0u, 1u, 1u, 200u, 21u);
    ng_chars_draw(); ng_vram_commit();
    assert(map_tile(first, 0u) == 200u);
    assert((ram[first * 64u + 1u] >> 8) == 21u);
    c->sprite_tile = 300u; c->flip_y = 1u; c->sprite_dirty = 1u;
    ng_chars_draw(); ng_vram_commit();
    assert(map_tile(first, 0u) == 300u);
    assert(ram[first * 64u + 1u] & 2u);

    c->visible = 0u;
    ng_chars_draw(); ng_vram_commit(); assert(!act_of(first));
    c->visible = 1u;
    ng_chars_draw(); ng_vram_commit();
    assert(act_of(first) == 1u && map_tile(first, 0u) == 300u);

    /* Recycled pool entries default to FULL: legacy raw writes work. */
    ng_chars_remove(c);
    c = chars_add(2u, 50, 60);
    ng_char_set_sprite(c, 0u, 1u, 1u, 400u, 22u);
    ng_chars_draw(); ng_vram_commit();
    c->sprite_tile = 500u;
    ng_chars_draw(); ng_vram_commit();
    assert(map_tile(c->sprite_first, 0u) == 500u);
}

static void test_small_actor_reserves_a_bounded_commit(void)
{
    NGSpriteGroup actor, bulk;
    unsigned before;
    ngsg_queue_drop(); memset(ram, 0, sizeof(ram));
    ng_vram_test_line = 100u;
    ng_sprite_group_init(&bulk, 1u, 32u, 12u, 1000u, 20u);
    ng_sprite_group_init(&actor, 219u, 5u, 4u, 100u, 4u);
    ng_sprite_group_set_priority(&actor, NG_SG_PRIO_HIGH);
    ng_sprite_group_flush(&bulk); ng_sprite_group_flush(&actor);
    before = writes;
    assert(!ng_sprite_group_commit_small(&actor)); /* active display */
    assert(writes == before && pending() == 2u);
    ng_vram_test_line = 7u;
    assert(!ng_sprite_group_commit_small(&actor)); /* insufficient whole-map budget */
    assert(writes == before && pending() == 2u);
    ng_vram_test_line = 240u; ng_vram_busy = 1u;
    assert(!ng_sprite_group_commit_small(&actor));
    ng_vram_busy = 0u;
    words_per_line = 16u; raster_words = 0u;
    assert(ng_sprite_group_commit_small(&actor));
    assert(pending() == 1u && ng_vram_busy == 0u && !(actor.dirty & NG_SGF_QUEUED));
    assert(act_of(219u) == 4u && map_tile(219u, 0u) == 100u);
    assert(map_tile(223u, 3u) == 119u);
    assert(ng_vram_test_line >= 240u || ng_vram_test_line < 16u);
    before = writes;
    assert(!ng_sprite_group_commit_small(&bulk)); /* oversized actors use normal/staged path */
    assert(writes == before && pending() == 1u);
    ng_vram_commit(); /* bulk cannot displace the already complete actor */
    assert(writes == before && pending() == 1u);
    assert(ng_sprite_group_commit_small(&actor)); /* no duplicate hardware write */
    assert(writes == before);
    words_per_line = 0u; ngsg_queue_drop();
    ng_sprite_group_set_scale(&actor, 255u, 239u);
    ng_vram_test_line = 240u;
    assert(ng_sprite_group_commit_small(&actor));
    assert(actor.readyRows == 0u); /* scaled maps are complete without the full-scale move shortcut */
    before = writes;
    assert(ng_sprite_group_commit_small(&actor) && writes == before);
    ng_vram_test_line = 100u;
    ng_sprite_group_init(&actor, 219u, 5u, 4u, 0u, 4u);
    ng_sprite_group_set_tile_address(&actor, 0x10000u);
    ng_sprite_group_flush(&actor); ng_vram_test_line = 240u; before = writes;
    assert(!ng_sprite_group_commit_small(&actor) && writes == before && pending() == 1u);
    ng_sprite_group_set_tile_address(&actor, 0xFFF0u); /* window crosses into the wide writer */
    assert(!ng_sprite_group_commit_small(&actor) && writes == before && pending() == 1u);
    ngsg_queue_drop();
}

static void test_small_actor_respects_hides_and_stream_ownership(void)
{
    NGSpriteGroup g;
    unsigned before;
    ngsg_queue_drop(); memset(ram, 0, sizeof(ram));
    ng_vram_test_line = 100u;
    ng_sprite_group_init(&g, 219u, 5u, 4u, 100u, 4u);
    ng_sprite_group_flush(&g);
    ng_sprite_hide_range_queued(219u, 5u);
    ng_vram_test_line = 240u; before = writes;
    assert(!ng_sprite_group_commit_small(&g));
    assert(writes == before && pending() == 1u);
    ng_vram_commit(); /* queued hide precedes replacement */
    assert(act_of(219u) == 4u);
    ng_vram_test_line = 100u;
    ng_sprite_group_set_strip_column(&g, 0u, 2u);
    ng_vram_test_line = 240u; before = writes;
    assert(!ng_sprite_group_commit_small(&g)); /* even a column-only update remains pending */
    assert(writes == before && ngsg_strips_n == 1u);
    ng_sprite_group_set_tile_base(&g, 200u);
    ng_sprite_group_flush(&g);
    ng_vram_test_line = 240u; before = writes;
    assert(!ng_sprite_group_commit_small(&g));
    assert(writes == before && ngsg_strips_n == 1u && pending() == 1u);
    ngsg_queue_drop();
}

static void test_character_small_commit_consumes_only_its_published_group(void)
{
    NGCharacter *c;
    uint16_t first;
    unsigned before;
    ngsg_queue_drop(); memset(ram, 0, sizeof(ram));
    ng_vram_test_line = 100u; ng_chars_init();
    c = chars_add(1u, 50, 60);
    ng_char_set_sprite(c, 0u, 5u, 4u, 100u, 4u);
    assert(!ng_char_commit_small(c)); /* draw has not published ownership */
    ng_chars_draw(); first = c->sprite_first;
    ng_vram_test_line = 240u;
    assert(ng_char_commit_small(c));
    assert(act_of(first) == 4u && map_tile(first, 0u) == 100u && pending() == 0u);
    before = writes; ng_vram_commit(); assert(writes == before);
    ng_vram_test_line = 100u; c->sprite_tile = 120u; c->sprite_dirty = 1u; ng_chars_draw();
    ng_chars_remove(c); ng_vram_test_line = 240u; before = writes;
    assert(!ng_char_commit_small(c) && writes == before);
    ng_vram_commit(); assert(act_of(first) == 0u);
    ngsg_queue_drop();
}

int main(void)
{
    test_pending_rebind_is_unique();
    test_reused_hidden_window_restores_stationary_owner();
    test_aged_write_keeps_membership_until_compaction();
    test_pinned_player_stays_when_a_creature_leaves();
    test_upload_keeps_membership();
    test_deferred_large_rom_map_keeps_latest_address();
    test_aged_content_obeys_deadline();
    test_bounded_removal_and_slot_reuse();
    test_cancel_drops_owned_streaming_columns();
    test_hidden_map_staging_keeps_live_pose();
    test_staging_detaches_previous_queue_owner();
    test_whole_bank_cutover_is_bounded();
    test_staged_map_crosses_64k_tiles_without_wrapping();
    test_move_only_character_model_lifecycle();
    test_small_actor_reserves_a_bounded_commit();
    test_small_actor_respects_hides_and_stream_ownership();
    test_character_small_commit_consumes_only_its_published_group();
    puts("deferred membership, deadline, bounded hides and replacement ownership: PASS");
    return 0;
}
