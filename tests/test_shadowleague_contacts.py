"""Compiler contact tables use each active drawing's reach, never pose unions."""
import sys,unittest,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'games/shadowleague/tools'))
from fighter_cook import Fighter,along_limb,art

class ContactCompilerTests(unittest.TestCase):
    def test_distinct_active_poses_keep_distinct_rectangles(self):
        f=Fighter(ROOT/'games/shadowleague/fighters/kenji.json')
        active=[fr for fr in f.data['anims']['super'] if len(fr)>2 and 'active' in fr[2]]
        boxes=[]
        for fr in active:
            sprite=f.sprite_names.index(fr[0])
            if f.authored is not None:
                source=f.authored[sprite]['hit'].get('near_fist',[])
                self.assertTrue(source,'Active authored punch needs its own contact region')
                boxes.extend(along_limb(b) for b in source)
            else:boxes.append(along_limb(art.hitbox(f.data['build'],f.joints[sprite],'near_fist')))
        self.assertGreater(len({tuple(b) for b in boxes}),1)
        text=f.c_tables('probe')
        for b in boxes:self.assertIn('{'+', '.join(str(v) for v in b)+'}',text)
        self.assertIn('probe_hit_sets, probe_frame_hits',text)
    def test_all_compiled_frames_have_a_contact_index(self):
        import re
        for name in ('kenji','jack','amara'):
            f=Fighter(ROOT/'games/shadowleague/fighters'/f'{name}.json');text=f.c_tables('probe')
            match=re.search(r'probe_frame_hits\[\] = \{([0-9,]+)\}',text)
            self.assertIsNotNone(match)
            indexes=[int(n) for n in match[1].split(',')]
            expected=[fr for anim in f.anim_names for fr in f.data['anims'][anim]]
            self.assertEqual(len(indexes),len(expected))
            for i,fr in enumerate(expected):
                if len(fr)<3 or 'active' not in fr[2]:self.assertEqual(indexes[i],0)
if __name__=='__main__':unittest.main()
