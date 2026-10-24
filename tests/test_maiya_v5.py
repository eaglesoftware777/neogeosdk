"""Original Maiya rules, safe layout changes and exact back-contour correction."""
import importlib.util
import hashlib
import re
import sys
from unittest.mock import patch
import numpy as np
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT=Path(__file__).resolve().parents[1]
GAME=ROOT/'games/maiya'
sys.path.insert(0,str(GAME/'tools'))


def module(name):
    spec=importlib.util.spec_from_file_location(name,GAME/'tools'/(name+'.py'))
    result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result)
    return result


class StageTests(unittest.TestCase):
    def test_original_campaign_rules_and_only_explicit_layout_edits(self):
        stages=[json.loads(p.read_text()) for p in sorted((GAME/'levels').glob('*.json'))]
        self.assertEqual(len(stages),12)
        self.assertTrue(all('arcade' not in stage for stage in stages))
        for stage_index,x,current,original in ((6,4944,128,104),(6,5104,88,72),(9,4992,136,120)):
            platform=next(p for p in stages[stage_index]['platforms'] if p['x']==x)
            self.assertEqual(platform['y'],current)
            platform['y']=original
        archer=next(a for a in stages[6]['archers'] if a['x']==5160)
        self.assertEqual(archer['y'],88)
        archer['y']=72
        self.assertEqual(hashlib.sha256(json.dumps(stages,sort_keys=True).encode()).hexdigest(),
                         'fb553734c374c9d182e25d13b5da83fcd1b6cebcc5db99c45af78767fc991b8f')

    def test_experimental_encounters_are_rejected_before_export(self):
        levels=module('levels');source=json.loads((levels.LEVELS/'01_emerald_forest.json').read_text())
        source['arcade']={}
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'bad.json';p.write_text(json.dumps(source))
            with self.assertRaisesRegex(levels.LevelError,'experimental encounters'):
                levels.Stage(p,{},levels.caps()).build()


class ArtworkTests(unittest.TestCase):
    def test_graphics_and_metadata_match_the_approved_v4_reference(self):
        module('locked_art').check_generated()

    def test_only_recorded_back_pixels_change_in_final_banks(self):
        art=module('maiya_posture');spec=art.recipe()
        self.assertEqual([p['frame'] for p in spec['frames']],list(range(60,76)))
        self.assertEqual(sum(len(p['edits']) for p in spec['frames']),90)
        base=[(GAME/'artbox'/f'780-{p}.{p}').read_bytes() for p in ('c1','c2')]
        final=art.banks()
        header=(GAME/'artbox/v4_reference/maiya_assets.h').read_text()
        tiles=[int(v,0) for v in re.findall(r'0x[0-9a-fA-F]+|\d+',re.search(r'mg_hero_tiles\[.*?\].*?\{(.*?)\}',header,re.S)[1])]
        permitted=set()
        for pose in spec['frames']:
            start=tiles[pose['frame']]*64
            permitted.update(range(start,start+1280))
            original=art.decode_image(base[0][start:start+1280],base[1][start:start+1280],80,64,swapped=True)
            result=art.decode_image(final[0][start:start+1280],final[1][start:start+1280],80,64,swapped=True)
            expected=original.copy()
            for x,y,old,new in pose['edits']:
                self.assertTrue(36<=x<=39 and 28<=y<=34)
                self.assertEqual(int(original[y,x]),old)
                expected[y,x]=new
            np.testing.assert_array_equal(result,expected)
            np.testing.assert_array_equal(result[:28],original[:28])
            np.testing.assert_array_equal(result[35:],original[35:])
            with self.assertRaisesRegex(ValueError,'source frame'):
                art.correct_frame(pose['frame'],np.zeros((64,80),dtype=np.uint8))
            bad=json.loads(json.dumps(spec));bad['frames'][pose['frame']-60]['edits'][0][0]=10
            with patch.object(art,'recipe',return_value=bad):
                with self.assertRaisesRegex(ValueError,'permitted back contour'):
                    art.correct_frame(pose['frame'],original)
        for original,result in zip(base,final):
            self.assertEqual(len(original),len(result))
            self.assertFalse(any(a!=b and i not in permitted for i,(a,b) in enumerate(zip(original,result))))

    def test_changed_art_is_rejected(self):
        art=module('locked_art')
        expected=art.verify_reference()['files']['artbox/780-c1.c1']
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'780-c1.c1';path.write_bytes(b'changed')
            with self.assertRaisesRegex(ValueError,'Approved V4 artwork differs'):
                art.verify_file(path,expected)


@unittest.skipUnless(shutil.which('cc'),'host C compiler required')
class RuntimeTests(unittest.TestCase):
    def test_optional_grid_has_no_false_negatives(self):
        source=r'''
#include <assert.h>
#include "sdk/2d_engine/ng_spatial_grid.h"
int main(void) {
 NGSpatialGrid g;int x,y,i;int boxes[8][4];
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
 return 0;
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            c=Path(tmp)/'test.c';exe=Path(tmp)/'test';c.write_text(source)
            subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-I'+str(ROOT),str(c),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)
