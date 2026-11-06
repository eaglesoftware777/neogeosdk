GAME_NAME  = Seven Wonders Puzzles
GAME_ID    = 782
GAME_SCENES = sw_draw sw_game sw_levels
GAME_ART_BUILDER = games/sevenwonders/tools/art.py
# Levels are authored in levels/levels.json; this writes the C tables first.
GAME_LEVEL_BUILDER = games/sevenwonders/tools/levels.py
GAME_OPTIMIZE = -O2
GAME_ENGINE_EXCLUDE = ng_particles
# Palette fades in the vertical blank, sprite writes in the vertical blank,
# and a queue for sound commands (sdk/neogeo.h), as the other SDK games.
GAME_ENGINE_DEFINES = -DNG_PALFX_SCREEN=1 -DNG_VRAM_DEFER=1 -DNG_SOUND_QUEUE=1
# The board logic: matching, cascades and launched bubbles (sdk/2d_engine/ng_puzzle.c).
GAME_GENRE_ENGINE = puzzle
