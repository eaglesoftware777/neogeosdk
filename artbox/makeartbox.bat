cd artbox
py createromdb.py
py romdbimgimport.py
py gen_sprite_meta.py
rem py romdbfiximport.py
rem py fixtiles.py
py romtiles.py
for /f %%i in ('py count_assets.py') do set IMG_COUNT=%%i
echo Art pipeline: %IMG_COUNT% images
py genmapfile.py %IMG_COUNT%
py genmapdb.py
py genscreens.py %IMG_COUNT% 16 16 16
py sync_main_screens.py
rem call  romfx.bat
call  romts.bat
cd ..
