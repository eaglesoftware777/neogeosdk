python createromdb.py
python romdbimgimport.py 10
rem python romdbfiximport.py
rem python fixtiles.py
python romtiles.py
python genmapfile.py 10
python genmapdb.py
python genscreens.py 10 16 16 16
rem call romfx.bat
call romts.bat
