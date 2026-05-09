cd artbox
python3 createromdb.py
python3 romdbimgimport.py
IMG_COUNT=$(ls in/*.png 2>/dev/null | wc -l)
echo "Art pipeline: $IMG_COUNT images"
#python3 romdbfiximport.py
#python3 fixtiles.py
python3 romtiles.py
python3 genmapfile.py $IMG_COUNT
python3 genmapdb.py
python3 genscreens.py $IMG_COUNT 16 16 16
python3 sync_main_screens.py
#./romfx.sh
./romts.sh
cd ..
