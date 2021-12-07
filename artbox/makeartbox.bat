cd artbox
py -2.7 createromdb.py
py -2.7 romdbimgimport.py 10
rem py -2.7 romdbfiximport.py
rem py -2.7 fixtiles.py
py -2.7 romtiles.py
py -2.7 genmapfile.py 10
py -2.7 genmapdb.py
py -2.7 genscreens.py 10 16 16 16
rem call  romfx.bat
call  romts.bat
cd ..