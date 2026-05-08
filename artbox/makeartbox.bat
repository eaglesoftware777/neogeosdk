cd artbox
py createromdb.py
py romdbimgimport.py 10
rem py romdbfiximport.py
rem py fixtiles.py
py romtiles.py
py genmapfile.py 10
py genmapdb.py
py genscreens.py 10 16 16 16
py sync_main_screens.py
rem call  romfx.bat
call  romts.bat
cd ..
