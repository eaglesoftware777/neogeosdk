# Host-side regression tests for the sprite renderers.
#
# These build the engine sources with the host compiler against a stand-in
# VRAM array - no emulator, no cross toolchain.  Run them with:
#
#     make unit-tests                 (from the SDK root, either makefile)
#
# The binaries carry the platform's executable suffix so a tree shared
# between WSL and Windows cannot end up running the other side's build.

CC  ?= gcc
CXX ?= g++
CFLAGS   = -I../sdk -I../sdk/2d_engine
CXXFLAGS = -I../sdk -I../sdk/2d_engine_plus

ifeq ($(OS),Windows_NT)
  EXE   := .exe
  RUN   :=
  RMF    = del /Q
  PYTHON ?= py
else
  EXE   :=
  RUN   := ./
  RMF    = rm -f
  PYTHON ?= python3
endif

TEST_C   := test_c$(EXE)
TEST_CPP := test_cpp$(EXE)
TEST_RAND := test_rand$(EXE)
TEST_TRIG := test_trig$(EXE)
TEST_PAUSE := test_pause$(EXE)
TEST_MOVE := test_move$(EXE)
TEST_SHOOTER := test_shooter$(EXE)
# v1.7.7 genre engines (sdk/2d_engine/ng_<genre>.c), each on its own, and all
# four reached from a C++14 program through their C headers.
TEST_PUZZLE := test_puzzle$(EXE)
TEST_MAHJONG := test_mahjong$(EXE)
TEST_RPG := test_rpg$(EXE)
TEST_BEATEMUP := test_beatemup$(EXE)
TEST_GENRE_CPP := test_genre_cpp$(EXE)
TEST_GENRES := $(TEST_PUZZLE) $(TEST_MAHJONG) $(TEST_RPG) $(TEST_BEATEMUP) $(TEST_GENRE_CPP)
GENRE_CFLAGS = -std=c99 -Wall -Wextra -Werror -Wno-missing-field-initializers $(CFLAGS)

C_SRCS   = sprite_render_test.c \
           ../sdk/2d_engine/ng_chars.c \
           ../sdk/2d_engine/ng_camera.c \
           ../sdk/2d_engine/ng_art_asset.c \
           ../sdk/2d_engine/ng_palette_fx.c \
           ../sdk/2d_engine/ng_sprite_group.c \
           ../sdk/2d_engine/ng_sprite_window.c \
           ../sdk/2d_engine/ng_vram.c
CPP_SRCS = sprite_render_test.c \
           ../sdk/2d_engine_plus/ng_chars.cpp \
           ../sdk/2d_engine_plus/ng_camera.cpp \
           ../sdk/2d_engine_plus/ng_art_asset.cpp \
           ../sdk/2d_engine_plus/ng_palette_fx.cpp \
           ../sdk/2d_engine_plus/ng_sprite_group.cpp \
           ../sdk/2d_engine_plus/ng_sprite_window.cpp \
           ../sdk/2d_engine_plus/ng_vram.cpp

.PHONY: all test clean

all: $(TEST_C) $(TEST_CPP) $(TEST_RAND) $(TEST_TRIG) $(TEST_PAUSE) $(TEST_MOVE) $(TEST_SHOOTER) $(TEST_GENRES)

$(TEST_C): $(C_SRCS) $(wildcard ../sdk/2d_engine/*.h)
	$(CC) -o $@ $(CFLAGS) $(C_SRCS)

$(TEST_CPP): $(CPP_SRCS) $(wildcard ../sdk/2d_engine_plus/*.hpp ../sdk/2d_engine/*.h)
	$(CXX) -o $@ $(CXXFLAGS) $(CPP_SRCS)

$(TEST_RAND): rand_test.c ../sdk/2d_engine/ng_rand.c ../sdk/2d_engine/ng_rand.h
	$(CC) -Wall -o $@ $(CFLAGS) rand_test.c ../sdk/2d_engine/ng_rand.c

TRIG_SRCS = trig_test.c ../sdk/2d_engine/ng_trig.c ../sdk/2d_engine/ng_trig_table.c

$(TEST_TRIG): $(TRIG_SRCS) ../sdk/2d_engine/ng_trig.h
	$(CC) -Wall -o $@ $(CFLAGS) $(TRIG_SRCS) -lm

PAUSE_SRCS = pause_test.c ../sdk/2d_engine/ng_pause.c ../sdk/2d_engine/ng_feedback.c

$(TEST_PAUSE): $(PAUSE_SRCS) ../sdk/2d_engine/ng_pause.h ../sdk/2d_engine/ng_feedback.h
	$(CC) -Wall -o $@ $(CFLAGS) $(PAUSE_SRCS)

$(TEST_MOVE): move_test.c ../sdk/2d_engine/ng_move.c ../sdk/2d_engine/ng_move.h
	$(CC) -Wall -o $@ $(CFLAGS) move_test.c ../sdk/2d_engine/ng_move.c

$(TEST_SHOOTER): shooter_test.c ../sdk/2d_engine/ng_shooter.c ../sdk/2d_engine/ng_shooter.h
	$(CC) -Wall -Wextra -o $@ $(CFLAGS) shooter_test.c ../sdk/2d_engine/ng_shooter.c

$(TEST_PUZZLE): puzzle_test.c ../sdk/2d_engine/ng_puzzle.c ../sdk/2d_engine/ng_puzzle.h $(TRIG_SRCS)
	$(CC) -o $@ $(GENRE_CFLAGS) puzzle_test.c ../sdk/2d_engine/ng_puzzle.c ../sdk/2d_engine/ng_trig.c ../sdk/2d_engine/ng_trig_table.c

$(TEST_MAHJONG): mahjong_test.c ../sdk/2d_engine/ng_mahjong.c ../sdk/2d_engine/ng_mahjong.h
	$(CC) -o $@ $(GENRE_CFLAGS) mahjong_test.c ../sdk/2d_engine/ng_mahjong.c

$(TEST_RPG): rpg_test.c ../sdk/2d_engine/ng_rpg.c ../sdk/2d_engine/ng_rpg.h
	$(CC) -o $@ $(GENRE_CFLAGS) rpg_test.c ../sdk/2d_engine/ng_rpg.c

$(TEST_BEATEMUP): beatemup_test.c ../sdk/2d_engine/ng_beatemup.c ../sdk/2d_engine/ng_beatemup.h
	$(CC) -o $@ $(GENRE_CFLAGS) beatemup_test.c ../sdk/2d_engine/ng_beatemup.c

GENRE_SRCS = ../sdk/2d_engine/ng_puzzle.c ../sdk/2d_engine/ng_mahjong.c ../sdk/2d_engine/ng_rpg.c \
             ../sdk/2d_engine/ng_beatemup.c ../sdk/2d_engine/ng_trig.c ../sdk/2d_engine/ng_trig_table.c
GENRE_OBJS = $(patsubst ../sdk/2d_engine/%.c,genre_%.o,$(GENRE_SRCS))

genre_%.o: ../sdk/2d_engine/%.c
	$(CC) -c -o $@ -std=c99 -Wall -Wextra $(CFLAGS) $<

$(TEST_GENRE_CPP): genre_cpp_test.cpp $(GENRE_OBJS) $(wildcard ../sdk/2d_engine/ng_puzzle.h ../sdk/2d_engine/ng_mahjong.h ../sdk/2d_engine/ng_rpg.h ../sdk/2d_engine/ng_beatemup.h)
	$(CXX) -std=c++14 -Wall -Wextra -Werror -Wno-missing-field-initializers -o $@ $(CFLAGS) genre_cpp_test.cpp $(GENRE_OBJS)

test: all
	$(RUN)$(TEST_C)
	$(RUN)$(TEST_CPP)
	$(RUN)$(TEST_RAND)
	$(RUN)$(TEST_TRIG)
	$(RUN)$(TEST_PAUSE)
	$(RUN)$(TEST_MOVE)
	$(RUN)$(TEST_SHOOTER)
	$(RUN)$(TEST_PUZZLE)
	$(RUN)$(TEST_MAHJONG)
	$(RUN)$(TEST_RPG)
	$(RUN)$(TEST_BEATEMUP)
	$(RUN)$(TEST_GENRE_CPP)
	$(PYTHON) ../tools/gen_trig.py --check ../sdk/2d_engine/ng_trig_table.c

clean:
	-$(RMF) $(TEST_C) $(TEST_CPP) $(TEST_RAND) $(TEST_TRIG) $(TEST_PAUSE) $(TEST_SHOOTER) $(TEST_GENRES) $(GENRE_OBJS)
