/*
 * ml_world.h -- Maiya & Luna: the world tables' types and the generated ids.
 *
 * The world itself (rooms, people, puzzles, items, dialogue) is written in
 * the world files (world/) and turned into scenes/ml_world_data.h by tools/world.py; this
 * header declares what that data holds beyond the RPG engine's own tables.
 */
#ifndef ML_WORLD_H
#define ML_WORLD_H
#include <stdint.h>
#include "sdk/2d_engine/ng_rpg.h"
#include "games/maiyaluna/scenes/ml_world_ids.h"

enum { ML_CAT_FLOWER = 0, ML_CAT_HISTORY, ML_CAT_DIAGRAM, ML_CAT_PHOTO, ML_CAT_SPECIMEN, ML_CAT_WONDER, ML_CATS };

typedef struct { uint16_t title; uint8_t restored_bg, restored_music, lit, pad; } MLRoomMeta;
typedef struct { uint16_t title, text; } MLItemText;
typedef struct { uint8_t category, pad; uint16_t title; } MLCollectible;
typedef struct { uint16_t name; uint8_t face, pad; } MLSpeaker;
/* A chapter: where it starts, the counter of its quest stages, the wonder
 * component it gives, where the party goes after its restoration, whether
 * the sisters travel together in it, and the texts of its cards; its
 * notebook goals are ml_goals[journey][goal_first..]. */
typedef struct {
    uint8_t number, quest;
    uint16_t title, place, reward;
    uint8_t start_room, start_x, start_y, start_face;
    uint8_t back_room, back_x, back_y, back_face;
    uint8_t wonder, goal_first, goals, together;   /* together: both sisters travel in it */
    uint16_t restore_title, restore_text, end_text;
} MLChapter;
typedef struct { uint8_t stage, pad; uint16_t text; } MLGoal;   /* shown once the quest counter reaches stage */

/* The sisters: Maiya climbs and moves heavy things, Luna fits through narrow
 * gaps and works fine mechanisms. Both weigh 2 (a block weighs 3). */
#define ML_MAIYA 0u
#define ML_LUNA  1u
static const NGRpgMemberDef ml_party[2] = {
    { ML_MAIYA, NGR_AB_CLIMB | NGR_AB_PUSH, 24, 6, 2 },
    { ML_LUNA,  NGR_AB_NARROW | NGR_AB_FINE, 22, 6, 2 },
};

/* One world per journey (ML_J_MAIYA, ML_J_LUNA: who set out): the same rooms,
 * each with that journey's objects, exits and triggers (tools/worldlib.py). */
extern const NGRpgWorld ml_worlds[ML_JOURNEYS];
extern const NGRpgRoomDef ml_rooms[ML_JOURNEYS][ML_ROOMS];
extern const MLRoomMeta ml_room_meta[ML_ROOMS];
extern const uint8_t *const ml_room_looks[ML_JOURNEYS][ML_ROOMS];
extern const uint16_t *const ml_room_hints[ML_JOURNEYS][ML_ROOMS];
extern const NGRpgItemDef ml_items[ML_ITEMS];
extern const MLItemText ml_item_text[ML_JOURNEYS][ML_ITEMS];
extern const MLCollectible ml_collect[ML_COLLECT];
extern const NGRpgDialogueNode ml_dialogue[ML_NODES];
extern const MLSpeaker ml_speakers[ML_SPEAKERS];
extern const MLChapter ml_chapters[ML_JOURNEYS][ML_CHAPTERS];
extern const MLGoal ml_goals[ML_JOURNEYS][ML_GOALS];
extern const char *const ml_text[ML_TEXTS];

#endif
