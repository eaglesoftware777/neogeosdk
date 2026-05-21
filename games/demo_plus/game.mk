GAME_NAME  = NeoGeo SDK Demo PLUS
GAME_ID    = 778
GAME_SCENES = demo_plus_main

# demo_plus shares the artbox + asset tables with games/demo.  Tell the
# top-level Makefile to add games/demo and games/demo/artbox to the include
# path so the shared main.c #include "../demo/main.c" finds its headers.
GAME_EXTRA_INCLUDES = -Igames/demo -Igames/demo/artbox
