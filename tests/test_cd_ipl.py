import unittest
from tools.neocd.formats import Entry,encode_ipl,parse_ipl


class IPLTests(unittest.TestCase):
    def test_physical_bytes_to_bus_offsets(self):
        entries=[Entry('A.PRG',0,2),Entry('A.SPR',3,128),Entry('A.FIX',0,32),
                 Entry('A.Z80',0,2),Entry('A.PCM',1,256)]
        raw=encode_ipl(entries)
        self.assertEqual(raw,b'A.PRG,0,2\r\nA.SPR,3,80\r\nA.FIX,0,40\r\nA.Z80,0,4\r\nA.PCM,1,200\r\n\x1a')
        self.assertEqual(parse_ipl(raw),entries)

    def test_limits(self):
        entries=[Entry('A%02d.SPR'%i) for i in range(33)]
        self.assertEqual(len(parse_ipl(encode_ipl(entries[:32]))),32)
        for bad in ([],entries,[entries[0],entries[0]]):
            with self.subTest(bad=len(bad)),self.assertRaises(ValueError):encode_ipl(bad)

    def test_reject_malformed_records(self):
        for raw in (b'A.SPR,0,0\n\x1a',b'A.SPR,0,0\r\n',b'a.SPR,0,0\r\n\x1a',
                    b'A.SPR,4,0\r\n\x1a',b'A.FIX,0,1\r\n\x1a',b'A.SPR,0,-1\r\n\x1a',
                    b'A.SPR,0,100000\r\n\x1a',b'A.PRG,0,0\r\n\x1a\x1a',
                    b'A.SPR,0,0\r\nA.SPR,0,0\r\n\x1a',b'A.FIX,0,2\r\n\x1a',
                    b'A.JPG,0,0\r\n\x1a',b'\xff.SPR,0,0\r\n\x1a'):
            with self.subTest(raw=raw),self.assertRaises(ValueError):parse_ipl(raw)

    def test_invalid_writer_entries(self):
        for e in (Entry('../A.SPR'),Entry('TOOLONGXX.SPR'),Entry('A.SPR',True),
                  Entry('A.SPR',-1),Entry('A.PCM',2),Entry('A.SPR',0,1)):
            with self.subTest(entry=e),self.assertRaises(ValueError):encode_ipl([e])


if __name__=='__main__':unittest.main()
