"""Archive an owned MAME capture losslessly and export an audible review clip."""
import argparse,hashlib,json,re,subprocess
from pathlib import Path

def run(args):return subprocess.check_output(args,stderr=subprocess.STDOUT,text=True)

def decoded_hashes(path):
    result={}
    for name,options in [('video',['-map','0:v:0','-pix_fmt','rgb24']),('audio',['-map','0:a:0','-c:a','pcm_s16le'])]:
        result[name]=run(['ffmpeg','-v','error','-nostdin','-i',str(path),*options,'-f','hash','-hash','sha256','-']).strip()
    return result

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--capture',type=Path,required=True);ap.add_argument('--archive',type=Path,required=True)
    ap.add_argument('--preview',type=Path,required=True);ap.add_argument('--start',type=float,default=23)
    ap.add_argument('--duration',type=float,default=16)
    args=ap.parse_args();raw=args.capture/'cartridge.avi';proof=args.capture/'archive-verification.json'
    if raw.exists():
        if not args.archive.exists():
            args.archive.parent.mkdir(parents=True,exist_ok=True)
            subprocess.run(['ffmpeg','-v','error','-nostdin','-i',str(raw),'-map','0:v','-map','0:a','-c:v','ffv1','-level','3','-g','1','-c:a','flac',str(args.archive)],check=True)
        before=decoded_hashes(raw);after=decoded_hashes(args.archive)
        if before!=after:raise ValueError('Archive changed decoded pixels or PCM audio')
        proof.write_text(json.dumps({'original':before,'lossless':after,'archive':str(args.archive),
                         'archive_sha256':hashlib.sha256(args.archive.read_bytes()).hexdigest()},indent=2)+'\n')
        raw.unlink()
    else:
        q=json.loads(proof.read_text())
        if q['archive_sha256']!=hashlib.sha256(args.archive.read_bytes()).hexdigest():raise ValueError('Archive identity changed')
    source=json.loads(run(['ffprobe','-v','error','-show_entries','format=duration:stream=codec_type,width,height,sample_rate,channels','-of','json',str(args.archive)]))
    video=next(s for s in source['streams'] if s['codec_type']=='video')
    if (video['width'],video['height'])!=(320,224):raise ValueError('Expected native Neo Geo capture')
    if float(source['format']['duration'])<args.start+args.duration:raise ValueError('Review clip exceeds the capture')
    if args.preview.exists():raise ValueError('Refusing to replace an existing review clip')
    subprocess.run(['ffmpeg','-v','error','-nostdin','-ss',str(args.start),'-i',str(args.archive),'-t',str(args.duration),
                    '-vf','scale=1280:896:flags=neighbor,pad=1920:1080:(ow-iw)/2:(oh-ih)/2:color=0x080c14,setsar=1',
                    '-c:v','libx264','-preset','fast','-crf','16','-pix_fmt','yuv420p','-c:a','aac','-b:a','192k',
                    '-movflags','+faststart',str(args.preview)],check=True)
    volume=run(['ffmpeg','-hide_banner','-nostdin','-i',str(args.preview),'-vn','-af','volumedetect','-f','null','-'])
    match=re.search(r'mean_volume: ([-0-9.]+) dB',volume)
    if not match or float(match[1]) < -60:raise ValueError('Review clip has no audible cartridge sound')
    result={'preview':str(args.preview),'sha256':hashlib.sha256(args.preview.read_bytes()).hexdigest(),
            'source_archive':str(args.archive),'capture_manifest':str(args.capture/'run-manifest.json'),
            'start_seconds':args.start,'duration_seconds':args.duration,'native_pixels':[320,224],
            'preview_pixels':[1920,1080],'scaling':'4x nearest neighbour, padded','mean_audio_dbfs':float(match[1])}
    args.preview.with_suffix('.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))

if __name__=='__main__':main()
