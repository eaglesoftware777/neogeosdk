GAME_NAME  = NeoGeo SDK Demo PLUS
GAME_ID    = 778

# demo_plus is the demo, run on the C++ engine.  It exists to show that the
# same game code produces the same ROM against either engine, so it shares the
# demo's scenes, artwork, main.c and user.c rather than keeping a second copy
# that would drift.  The only thing that differs is which engine gets linked.
USE_2D_PLUS = 1

# Scene sources come from games/demo/scenes.  The Makefile looks in this game's
# own scenes directory first, so a file dropped into games/demo_plus/scenes
# with a matching name overrides the demo's without touching this list.
GAME_SCENES_FROM = demo
GAME_SCENES = demo demo_intro demo_sprites demo_camera demo_palette demo_particles demo_depth demo_sound demo_fix demo_combat demo_stress demo_title demo_render demo_screen demo_2d_engine demo_3d demo_unified

# Source artwork comes from the demo's artbox.  The tables, palettes and the
# 778-* C/S ROMs are still generated into games/demo_plus/artbox under this
# game's own ID; only the input PNGs are shared.
GAME_ART_FROM = demo

# Music, FM patches and ADPCM samples come from the demo too, so the flow's
# cues land on the same track and SFX numbers.  778-m1.m1 and 778-v1.v1 are
# still built for this game.
GAME_SOUND_FROM = demo

# The shared games/demo/main.c and games/demo/user.c include headers by their
# demo-relative names, so put games/demo and its artbox on the include path.
# games/demo_plus/artbox comes first in CFLAGS, so this game's own generated
# tables win and the demo's are only the fallback.
GAME_EXTRA_INCLUDES = -Igames/demo -Igames/demo/artbox

# games/demo_plus/scenes/demo_plus_main.c is a standalone three-scene smoke
# test for sdk/2d_engine_plus.  It defines its own demo_run_full_flow, so it
# cannot be built alongside the demo scenes above; to run it instead, replace
# GAME_SCENES with "demo_plus_main" and drop GAME_SCENES_FROM.
