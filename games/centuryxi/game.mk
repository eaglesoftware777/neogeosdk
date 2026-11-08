GAME_NAME  = Century XI: Road to 2030
GAME_ID    = 787
GAME_SCENES = cx_match cx_ball cx_player cx_keeper cx_ai cx_rules cx_nations cx_tournament cx_render cx_show cx_menu cx_audio cx_var cx_referee cx_save
GAME_ART_BUILDER = games/centuryxi/tools/art.py
# the ball, the lines, offside and the championship: sdk/2d_engine/ng_soccer.c
GAME_GENRE_ENGINE = soccer
GAME_OPTIMIZE = -O2
# sprite and FIX writes are queued and committed in the vertical blank (frame_wait);
# sound commands are queued too, never waited for (a slow answer from the driver held
# a frame for up to a whole frame)
GAME_ENGINE_DEFINES = -DNG_VRAM_DEFER=1 -DNG_SOUND_QUEUE=1
