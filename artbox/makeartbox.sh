cd artbox
python3 createromdb.py
python3 romdbimgimport.py
python3 gen_sprite_meta.py
IMG_COUNT=$(python3 count_assets.py)
echo "Art pipeline: $IMG_COUNT images"
#python3 romdbfiximport.py
#python3 fixtiles.py
python3 romtiles.py
python3 genmapfile.py $IMG_COUNT
python3 genmapdb.py
python3 genscreens.py $IMG_COUNT 16 16 16
python3 sync_main_screens.py
python3 gen_eyecatcher.py
#./romfx.sh
./romts.sh
cd ..
