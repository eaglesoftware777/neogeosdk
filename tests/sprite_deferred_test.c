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
    puts("deferred membership, deadline, bounded hides and replacement ownership: PASS");
    return 0;
}
