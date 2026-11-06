"""Small original native disc fixtures; no firmware or commercial game data."""
import copy
from pathlib import Path


def program():
    data=bytearray(0x204)
    data[0x100:0x108]=b'NEO-GEO\x02'
    data[0x122:0x128]=bytes.fromhex('4ef900000200')
    data[0x200:]=bytes.fromhex('4e7160fe')
    return bytes(data)


def package(folder):
    folder=Path(folder)
    data={'HELLO.PRG':program(),'BOOT.SPR':bytes(128),'FONT.FIX':bytes(32),
          'SOUND.Z80':bytes(2),'CUE.PCM':bytes(256)}
    for name,raw in data.items(): (folder/name).write_bytes(raw)
    manifest={'format':'neogeosdk-cd-1','game':'hello','groups':{'boot':{'depends':[]}},
              'files':[{'name':n,'source':n,'group':'boot'} for n in data]}
    return copy.deepcopy(manifest),data
