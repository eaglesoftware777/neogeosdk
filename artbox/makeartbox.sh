cd artbox
python3 createromdb.py
python3 romdbimgimport.py 10
#python3 romdbfiximport.py
#python3 fixtiles.py
python3 romtiles.py
python3 genmapfile.py 10
python3 genmapdb.py
python3 genscreens.py 10 16 16 16
python3 sync_main_screens.py
#./romfx.sh
./romts.sh
cd ..
