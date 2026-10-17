GAME_NAME  = Shadow League Fighting
GAME_ID    = 781
GAME_SCENES = slf_draw slf_game slf_data slf_rom
GAME_P_BUILDER = games/shadowleague/tools/banked_rom.py
GAME_ART_BUILDER = games/shadowleague/tools/art.py
GAME_OPTIMIZE = -O2
GAME_ENGINE_EXCLUDE = ng_particles
GAME_ENGINE_DEFINES = -DNG_PALFX_SCREEN=1 -DNG_VRAM_DEFER=1 -DNG_SOUND_QUEUE=1 -DNG_FIGHT=1
# Music streams (ADPCM-B) at 21 kHz: sixteen tracks and the voices fit the 8 MiB V ROM.
export ADPCMB_RATE = 21000
