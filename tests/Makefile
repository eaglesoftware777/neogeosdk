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

all: $(TEST_C) $(TEST_CPP) $(TEST_RAND) $(TEST_TRIG) $(TEST_PAUSE)

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

test: all
	$(RUN)$(TEST_C)
	$(RUN)$(TEST_CPP)
	$(RUN)$(TEST_RAND)
	$(RUN)$(TEST_TRIG)
	$(RUN)$(TEST_PAUSE)
	$(PYTHON) ../tools/gen_trig.py --check ../sdk/2d_engine/ng_trig_table.c

clean:
	-$(RMF) $(TEST_C) $(TEST_CPP) $(TEST_RAND) $(TEST_TRIG) $(TEST_PAUSE)
