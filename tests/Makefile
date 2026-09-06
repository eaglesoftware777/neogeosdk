CC ?= gcc
CXX ?= g++
CFLAGS = -I../sdk -I../sdk/2d_engine
CXXFLAGS = -I../sdk -I../sdk/2d_engine_plus

.PHONY: all test clean

all: test_c test_cpp

test_c: sprite_render_test.c ../sdk/2d_engine/ng_sprite_group.c ../sdk/2d_engine/ng_sprite_window.c ../sdk/2d_engine/ng_vram.c
	$(CC) -o $@ $(CFLAGS) $^

test_cpp: sprite_render_test.c ../sdk/2d_engine_plus/ng_sprite_group.cpp ../sdk/2d_engine_plus/ng_sprite_window.cpp ../sdk/2d_engine_plus/ng_vram.cpp
	$(CXX) -o $@ $(CXXFLAGS) $^

test: all
	./test_c
	./test_cpp

clean:
	rm -f test_c test_cpp
