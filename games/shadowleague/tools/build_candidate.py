"""Build independent AES/MVS candidate packages and retain matching debug ELF files.
Run in an isolated checkout. Does not install, publish, commit or push anything.
"""
import argparse,hashlib,json,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
GAME=ROOT/'games/shadowleague'

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--platform',choices=['aes','mvs','both'],default='both')
 ap.add_argument('--reuse-assets',action='store_true',help='Code-only rebuild after unchanged C/S/M/V assets have passed a full build')
 ap.add_argument('--output-root',type=Path,default=ROOT/'dist/candidates/shadowleague/rebirth-banked')
 ap.add_argument('--label',default='rebirth')
 args=ap.parse_args()
 if not args.label or any(c not in 'abcdefghijklmnopqrstuvwxyz0123456789-' for c in args.label):ap.error('Invalid artifact label')
 out=GAME/'build/elf'/args.label;out.mkdir(parents=True,exist_ok=True)
 destination=args.output_root.resolve()
 platforms=['aes','mvs'] if args.platform=='both' else [args.platform]
 for p in platforms:
  targets=['781-p1.p1','hash'] if args.reuse_assets else ['all','hash']
  subprocess.run(['make','GAME=shadowleague','PLATFORM='+p]+targets,cwd=ROOT,check=True)
  shutil.copy2(ROOT/'out/game',out/(p+'-'+args.label+'-game'))
  subprocess.run([sys.executable,'tools/make_neo.py','--game','shadowleague','--game-id','781','--platform',p,'--name','Shadow League Fighting','--output-root',str(destination)],cwd=ROOT,check=True)
  package=destination/p/'roms/shadowleague'
  for rom in package.glob('781-*'):
   assert rom.read_bytes()==(ROOT/'roms/shadowleague'/rom.name).read_bytes()
 subprocess.run([sys.executable,'tools/make_neo.py','--game','shadowleague','--manifest','--output-root',str(destination)],cwd=ROOT,check=True)
 records={}
 for p in platforms:
  records[p]={rom.name:hashlib.sha256(rom.read_bytes()).hexdigest() for rom in (destination/p/'roms/shadowleague').glob('781-*')}
 (GAME/'build/candidate-banked-sha256.json').write_text(json.dumps(records,indent=2)+'\n')
if __name__=='__main__':main()
