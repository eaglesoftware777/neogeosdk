/* Exercise real deferred rendering against the host VRAM fixture. */
#define main sprite_immediate_test_main
#include "sprite_render_test.c"
#undef main
#include "../sdk/2d_engine/ng_sprite_group.c"

uint16_t ng_vram_test_line = 240u;
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

int main(void)
{
    test_pending_rebind_is_unique();
    test_reused_hidden_window_restores_stationary_owner();
    test_aged_write_keeps_membership_until_compaction();
    test_pinned_player_stays_when_a_creature_leaves();
    test_upload_keeps_membership();
    puts("deferred rebind membership, hidden window reuse, pinned slots and immediate uploads: PASS");
    return 0;
}
