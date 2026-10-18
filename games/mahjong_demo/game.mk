GAME_NAME  = Mahjong Demo
GAME_ID    = 785
GAME_SCENES = mj_draw mj_table mj_sol mj_game
GAME_ART_BUILDER = games/mahjong_demo/tools/art.py
GAME_OPTIMIZE = -O2
GAME_ENGINE_EXCLUDE = ng_particles
# Palette fades in the vertical blank, sprite writes in the vertical blank,
# and a queue for sound commands (sdk/neogeo.h), as the other SDK games.
GAME_ENGINE_DEFINES = -DNG_PALFX_SCREEN=1 -DNG_VRAM_DEFER=1 -DNG_SOUND_QUEUE=1
# Four-seat Mahjong and Mahjong solitaire (sdk/2d_engine/ng_mahjong.c).
GAME_GENRE_ENGINE = mahjong
