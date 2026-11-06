"""Residency safety tests. Runtime executable overlays remain unsupported."""
import copy
from pathlib import Path
import tempfile
import unittest
from cd_fixtures import package,program
from tools.neocd.build_disc import validate_manifest,source_file


class ResidencyTests(unittest.TestCase):
    def test_pinned_resources_and_exclusive_scene_banks(self):
        with tempfile.TemporaryDirectory() as d:
            f=Path(d);m,_=package(f);(f/'NEXT.SPR').write_bytes(bytes(128))
            for group in ('level1','level2'):
                m['groups'][group]={'depends':['boot']}
                m['files'].append(dict(name=group.upper()+'.SPR',source='NEXT.SPR',group=group,bank=1))
            self.assertEqual(len(validate_manifest(m,f)[0]),7)
            m['groups']['level2']['depends'].append('level1')
            with self.assertRaises(ValueError):validate_manifest(m,f)

    def test_boot_collision_and_executable_overlay_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            f=Path(d);m,_=package(f);(f/'NEXT.SPR').write_bytes(bytes(128))
            m['groups']['level']={'depends':['boot']}
            m['files'].append(dict(name='NEXT.SPR',source='NEXT.SPR',group='level'))
            with self.assertRaises(ValueError):validate_manifest(m,f)
            m['files'][-1]=dict(name='NEXT.PRG',source='NEXT.PRG',group='level',offset=0x80000)
            (f/'NEXT.PRG').write_bytes(program())
            with self.assertRaises(ValueError):validate_manifest(m,f)

    def test_invalid_dependency_graph(self):
        with tempfile.TemporaryDirectory() as d:
            f=Path(d);original,_=package(f)
            for groups in ({'boot':{'depends':['missing']}},{'boot':{'depends':['boot']}},
                           {'boot':{},'scene':{}},{'boot':{'depends':'scene'}},
                           {'boot':{'depends':[]},'scene':{'depends':[['boot']]}}):
                m=copy.deepcopy(original);m['groups']=groups
                with self.subTest(groups=groups),self.assertRaises(ValueError):validate_manifest(m,f)

    def test_integrity_and_source_sandbox(self):
        with tempfile.TemporaryDirectory() as d:
            f=Path(d);m,_=package(f)
            for field,value in (('sha256','0'*64),('size',1),('size',True)):
                bad=copy.deepcopy(m);bad['files'][0][field]=value
                with self.subTest(field=field,value=value),self.assertRaises(ValueError):validate_manifest(bad,f)
            for name in ('../outside.SPR',str(f/'BOOT.SPR')):
                with self.assertRaises(ValueError):source_file(f,name)


if __name__=='__main__':unittest.main()
