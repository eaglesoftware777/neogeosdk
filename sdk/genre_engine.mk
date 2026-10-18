# Genre engine selector (v1.7.7), read by Makefile and MakefileWin32.mak.
#
# A game picks at most one genre engine with one line in its game.mk:
#
#     GAME_GENRE_ENGINE = puzzle      # sdk/2d_engine/ng_puzzle.c
#     GAME_GENRE_ENGINE = mahjong     # sdk/2d_engine/ng_mahjong.c
#     GAME_GENRE_ENGINE = rpg         # sdk/2d_engine/ng_rpg.c
#     GAME_GENRE_ENGINE = beatemup    # sdk/2d_engine/ng_beatemup.c
#
# The chosen engine is compiled as C (also for a USE_2D_PLUS game, whose own
# sources go through g++ and reach it through the header's extern "C") and
# linked after the core engine objects, which every game keeps as before.
# Left empty, the default, nothing is compiled or linked and the build is the
# one it always was. The game's code and the engine both see
# -DNG_GENRE_ENGINE_<NAME>=1. See docs/GENRE_ENGINES_V1.7.7.md.

GAME_GENRE_ENGINE ?=
NG_GENRE_ALLOWED := puzzle mahjong rpg beatemup
NG_GENRE := $(strip $(GAME_GENRE_ENGINE))

ifneq ($(NG_GENRE),)
  ifneq ($(words $(NG_GENRE)),1)
    $(error GAME_GENRE_ENGINE='$(NG_GENRE)': a game links one genre engine, not $(words $(NG_GENRE)); choose one of: $(NG_GENRE_ALLOWED))
  endif
  ifeq ($(filter $(NG_GENRE),$(NG_GENRE_ALLOWED)),)
    $(error GAME_GENRE_ENGINE='$(NG_GENRE)' is not a genre engine; use one of: $(NG_GENRE_ALLOWED), or leave it empty)
  endif
endif

NG_GENRE_SRC := $(if $(NG_GENRE),sdk/2d_engine/ng_$(NG_GENRE).c)
NG_GENRE_OBJ0 := $(if $(NG_GENRE),out/ng_$(NG_GENRE)0.o)
NG_GENRE_DEFINE_puzzle   := -DNG_GENRE_ENGINE_PUZZLE=1
NG_GENRE_DEFINE_mahjong  := -DNG_GENRE_ENGINE_MAHJONG=1
NG_GENRE_DEFINE_rpg      := -DNG_GENRE_ENGINE_RPG=1
NG_GENRE_DEFINE_beatemup := -DNG_GENRE_ENGINE_BEATEMUP=1
NG_GENRE_DEFINE := $(if $(NG_GENRE),$(NG_GENRE_DEFINE_$(NG_GENRE)))
