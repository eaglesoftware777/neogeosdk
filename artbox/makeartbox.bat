cd artbox
py createromdb.py
if errorlevel 1 goto :fail
py romdbimgimport.py
if errorlevel 1 goto :fail
py gen_sprite_meta.py
if errorlevel 1 goto :fail
rem py romdbfiximport.py
rem py fixtiles.py
py romtiles.py
if errorlevel 1 goto :fail
for /f %%i in ('py count_assets.py') do set IMG_COUNT=%%i
echo Art pipeline: %IMG_COUNT% images
py genmapfile.py %IMG_COUNT%
if errorlevel 1 goto :fail
py genmapdb.py
if errorlevel 1 goto :fail
py genscreens.py %IMG_COUNT% 16 16 16
if errorlevel 1 goto :fail
py sync_main_screens.py
if errorlevel 1 goto :fail
rem call  romfx.bat
call  romts.bat
if errorlevel 1 goto :fail
cd ..
goto :eof

:fail
cd ..
exit /b 1
