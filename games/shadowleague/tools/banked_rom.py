"""Assemble fixed code and two real one-MiB presentation data banks.

The linker addresses only fixed code. Payload offsets describe the physical
P image; they are copied through the runtime window instead of linked as
permanent pointers. Output is word-swapped for the MAME/NeoSD cartridge.
"""
import argparse, hashlib, json, subprocess, tempfile
from pathlib import Path
ROOT = Path(__file__).resolve().parents[3]
MIB = 0x100000

def assemble(fixed, banks):
    if not fixed or len(fixed) > MIB:
        raise ValueError('Fixed code must fit the first one-MiB P region')
    if len(banks) != 2 or any(not b or len(b) > MIB for b in banks):
        raise ValueError('Exactly two nonempty presentation banks, each at most one MiB')
    raw = fixed.ljust(MIB, b'\xff') + b''.join(b.ljust(MIB,b'\xff') for b in banks)
    swapped = bytearray(raw); swapped[0::2], swapped[1::2] = raw[1::2], raw[0::2]
    return bytes(swapped)

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--elf',type=Path,required=True);ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--objcopy',required=True);a=ap.parse_args()
    folder=ROOT/'games/shadowleague/artbox/generated'
    with tempfile.TemporaryDirectory(prefix='slf-fixed-') as tmp:
        binary=Path(tmp)/'fixed.bin'
        subprocess.run([a.objcopy,'-O','binary',str(a.elf),str(binary)],check=True)
        fixed=binary.read_bytes()
    banks=[(folder/f'presentation-bank-{n}.bin').read_bytes() for n in range(2)]
    result=assemble(fixed,banks);a.output.write_bytes(result)
    report={'mapper':'standard','fixed_code_bytes':len(fixed),'bank_capacity':MIB,
            'payload_bytes':[len(b) for b in banks],'P_bytes':len(result),
            'payload_sha256':[hashlib.sha256(b).hexdigest() for b in banks]}
    (folder/'banked-rom.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Banked P: fixed %d, payloads %s, cartridge %d bytes' % (len(fixed),report['payload_bytes'],len(result)))
if __name__=='__main__':main()
