GAME_NAME = Maiya: Super Nature Girl
GAME_ID = 780
GAME_SCENES = maiya_game
GAME_SOUND_FROM = maiya
GAME_EXTRA_INCLUDES = -Igames/maiya -Igames/maiya/artbox
GAME_ART_BUILDER = games/maiya/tools/art.py
# Her stages are authored as JSON in games/maiya/levels/ (docs/levels.md).
GAME_LEVEL_BUILDER = games/maiya/tools/levels.py

# Maiya draws her own sparks and dust (MGSpark), so the engine particle
# pool is left out: no per-frame update, no pool RAM.
GAME_ENGINE_EXCLUDE = ng_particles

# Her fades lift every palette bank she uses together, and every colour
# reaches palette RAM in the vertical blank (ng_palette_fx.h, the screen).
GAME_ENGINE_DEFINES = -DNG_PALFX_SCREEN=1

# Her frame doesn't fit in one vertical blank at -O0: the engine and her
# scene are built at -O2 (the start-up sources stay at -O0).
GAME_OPTIMIZE = -O2
