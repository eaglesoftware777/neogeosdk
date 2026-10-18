GAME_NAME  = Maiya & Luna
GAME_ID    = 783
GAME_SCENES = ml_draw ml_game ml_world
GAME_ART_BUILDER = games/maiyaluna/tools/art.py
# The world (rooms, people, puzzles, dialogue) is written in world/*.py; this writes the C tables first.
GAME_LEVEL_BUILDER = games/maiyaluna/tools/world.py
GAME_OPTIMIZE = -O2
GAME_ENGINE_EXCLUDE = ng_particles
# Palette fades in the vertical blank, sprite writes in the vertical blank,
# and a queue for sound commands (sdk/neogeo.h), as the other SDK games.
GAME_ENGINE_DEFINES = -DNG_PALFX_SCREEN=1 -DNG_VRAM_DEFER=1 -DNG_SOUND_QUEUE=1
# Rooms, the party, puzzles, inventory and dialogue (sdk/2d_engine/ng_rpg.c).
GAME_GENRE_ENGINE = rpg
