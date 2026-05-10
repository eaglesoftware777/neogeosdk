python3 createromdb.py
python3 romdbimgimport.py
python3 gen_sprite_meta.py
#python3 romdbfiximport.py
#python3 fixtiles.py
python3 romtiles.py
IMG_COUNT=$(python3 count_assets.py)
python3 genmapfile.py $IMG_COUNT
python3 genmapdb.py
python3 genscreens.py $IMG_COUNT 16 16 16
#./romfx.sh
./romts.sh
