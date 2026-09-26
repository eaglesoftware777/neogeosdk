GAME_NAME = Maiya: Super Nature Girl
GAME_ID = 780
GAME_SCENES = maiya_game
GAME_SOUND_FROM = maiya
GAME_EXTRA_INCLUDES = -Igames/maiya -Igames/maiya/artbox
GAME_ART_BUILDER = games/maiya/tools/art.py

# Maiya draws her own sparks and dust (MGSpark), so the engine particle
# pool is left out: no per-frame update, no pool RAM.
GAME_ENGINE_EXCLUDE = ng_particles
