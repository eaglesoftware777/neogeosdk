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
else
  EXE   :=
  RUN   := ./
  RMF    = rm -f
endif

TEST_C   := test_c$(EXE)
TEST_CPP := test_cpp$(EXE)

C_SRCS   = sprite_render_test.c \
           ../sdk/2d_engine/ng_sprite_group.c \
           ../sdk/2d_engine/ng_sprite_window.c \
           ../sdk/2d_engine/ng_vram.c
CPP_SRCS = sprite_render_test.c \
           ../sdk/2d_engine_plus/ng_sprite_group.cpp \
           ../sdk/2d_engine_plus/ng_sprite_window.cpp \
           ../sdk/2d_engine_plus/ng_vram.cpp

.PHONY: all test clean

all: $(TEST_C) $(TEST_CPP)

$(TEST_C): $(C_SRCS)
	$(CC) -o $@ $(CFLAGS) $(C_SRCS)

$(TEST_CPP): $(CPP_SRCS)
	$(CXX) -o $@ $(CXXFLAGS) $(CPP_SRCS)

test: all
	$(RUN)$(TEST_C)
	$(RUN)$(TEST_CPP)

clean:
	-$(RMF) $(TEST_C) $(TEST_CPP)
