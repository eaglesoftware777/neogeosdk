GAME_NAME  = No Signal Z Alpha
GAME_ID    = 784
GAME_SCENES = ns_draw ns_game ns_data
GAME_ART_BUILDER = games/nosignal/tools/art.py
GAME_OPTIMIZE = -O2
GAME_ENGINE_EXCLUDE = ng_particles
# Palette fades in the vertical blank, sprite writes in the vertical blank,
# and a queue for sound commands (sdk/neogeo.h), as the other SDK games.
GAME_ENGINE_DEFINES = -DNG_PALFX_SCREEN=1 -DNG_VRAM_DEFER=1 -DNG_SOUND_QUEUE=1
# The street, the crowd, grabs, throws and the scooter (sdk/2d_engine/ng_beatemup.c).
GAME_GENRE_ENGINE = beatemup
