"""V5 pacing, collision candidates and indexed-environment contracts."""
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

import numpy as np

ROOT=Path(__file__).resolve().parents[1]
GAME=ROOT/'games/maiya'


def module(name):
    spec=importlib.util.spec_from_file_location(name,GAME/'tools'/(name+'.py'))
    result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result)
    return result


class StageTests(unittest.TestCase):
    def test_campaign_has_five_contiguous_beats_and_safe_guard_floors(self):
        levels=module('levels');_,stages=levels.load()
        self.assertEqual(len(stages),12)
        self.assertEqual({s['arcade']['gimmick'] for s in stages},set(range(12)))
        for stage in stages:
            a=stage['arcade'];self.assertTrue(all(x<y for x,y in zip([0]+a['ends'],a['ends'])))
            self.assertEqual(a['ends'][-1],stage['width'])
            self.assertGreaterEqual(a['x'],a['ends'][1]+96)
            self.assertLessEqual(a['x'],a['ends'][2]-96)

    def test_bad_pacing_is_rejected_before_export(self):
        levels=module('levels');source=json.loads((levels.LEVELS/'01_emerald_forest.json').read_text())
        for field in ('overlap','order','bank','enemy'):
            bad=json.loads(json.dumps(source))
            if field=='overlap':bad['arcade']['phases'][1]['start']+=1
            elif field=='order':bad['arcade']['phases'][3]['name']='intro'
            elif field=='bank':bad['arcade']['platform']=33
            else:bad['arcade']['miniboss']['enemy']='unknown'
            with self.subTest(field=field),tempfile.TemporaryDirectory() as tmp:
                p=Path(tmp)/'bad.json';p.write_text(json.dumps(bad))
                names={'MG_E_':levels.defines(GAME/'scenes/maiya_levels.h','MG_E_')}
                with self.assertRaises(levels.LevelError):levels.Stage(p,names,levels.caps()).build()


class ArtworkTests(unittest.TestCase):
    def test_all_native_scenery_tiles_use_one_embedded_palette_bank(self):
        art=module('arcade_art')
        for stage,cols in enumerate(art.COLS):
            image=art.panorama(stage,cols*16)
            self.assertEqual(image.mode,'P');self.assertEqual(image.info['transparency'],0)
            pixels=np.asarray(image)
            self.assertFalse(np.any(pixels%16==0))
            for y in range(0,224,16):
                self.assertEqual(len(np.unique(pixels[y:y+16]//16)),1)
            self.assertGreater(len(image.getcolors()),32)
            mid=art.midground(stage)
            self.assertEqual(mid.size,(32,112));self.assertTrue(np.any(np.asarray(mid)==0))


@unittest.skipUnless(shutil.which('cc'),'host C compiler required')
class RuntimeTests(unittest.TestCase):
    def test_grid_has_no_false_negatives_and_arcade_math_is_bounded(self):
        source=r'''
#include <assert.h>
#include "sdk/2d_engine/ng_spatial_grid.h"
#include "games/maiya/scenes/maiya_arcade.h"
int main(void) {
 NGSpatialGrid g;int x,y,i;int boxes[8][4];
 MGArcadeProfile a={{640,1280,1920,3200,3840},1600,1,8,0,0,0};
 ng_spatial_grid_reset(&g,-32,-64);
 for(i=0;i<8;++i) {
  boxes[i][0]=i*43-40;boxes[i][1]=i*23-45;
  boxes[i][2]=boxes[i][0]+57;boxes[i][3]=boxes[i][1]+74;
  ng_spatial_grid_insert(&g,(unsigned char)i,boxes[i][0],boxes[i][1],boxes[i][2],boxes[i][3]);
 }
 for(y=-64;y<256;y+=3) for(x=-32;x<352;x+=3) {
  unsigned short result=ng_spatial_grid_query(&g,x,y,x+1,y+1);
  for(i=0;i<8;++i) if(x>=boxes[i][0] && x<boxes[i][2] && y>=boxes[i][1] && y<boxes[i][3]) assert(result&(1u<<i));
 }
 assert(!ng_spatial_grid_query(&g,500,0,510,10));
 assert(!ng_spatial_grid_query(&g,0,0,0,10));
 for(x=0;x<3840;++x) assert(mg_arcade_phase(&a,x)<5);
 assert(mg_arcade_phase(&a,639)==0 && mg_arcade_phase(&a,640)==1);
 for(x=0;x<1024;++x) assert(mg_arcade_sway(x)>=-8 && mg_arcade_sway(x)<=8);
 assert(mg_arcade_multiplier(255)==10 && mg_arcade_multiplier(0)==1);
 assert(mg_arcade_rank(8,5,0)==2 && mg_arcade_rank(8,1,0)==0);
 assert(mg_arcade_boss_contact(0,160,0,192,0)==2);
 assert(mg_arcade_boss_contact(36,160,0,192,0)==1);
 assert(mg_arcade_boss_contact(0,70,0,192,0)==0);
 assert(mg_arcade_boss_contact(0,130,0,192,4)==2);
 return 0;
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            c=Path(tmp)/'test.c';exe=Path(tmp)/'test';c.write_text(source)
            subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-I'+str(ROOT),str(c),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)
