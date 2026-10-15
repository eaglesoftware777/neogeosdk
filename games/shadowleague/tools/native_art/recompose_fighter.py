"""Export edited fighter layers without running the initial drawing recipes.

All poses and palette banks are validated in staging before any flat PNG changes.
Collision rectangles and animation timing remain explicitly authored metadata.
"""
import argparse
import json
import os
import shutil
import sys
import tempfile
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from native_art_io import indexed,fighter_source

ORDER=('far_arm','far_leg','torso','head_hair','near_leg','near_arm')

def export(folder,pose=None):
    folder=Path(folder).resolve()
    manifest=json.loads((folder/'manifest.json').read_text())
    names=list(manifest['frames'])
    if pose is not None and pose not in names:raise ValueError('Unknown pose: '+pose)
    selected=[pose] if pose else names
    with tempfile.TemporaryDirectory(prefix='shadowleague-pixel-export-') as temporary:
        stage=Path(temporary)
        (stage/'manifest.json').write_text(json.dumps(manifest))
        # Copy only files referenced by the manifest, retaining nested paths.
        files={frame['file'] for frame in manifest['frames'].values()}
        files.update(path for frame in manifest['frames'].values() for path in frame['layers'].values())
        for name in files:
            if Path(name).is_absolute() or '..' in Path(name).parts:raise ValueError('Source must be relative to fighter directory')
            source=(folder/name).resolve()
            if not source.is_relative_to(folder):raise ValueError('Source escapes fighter directory')
            dest=stage/name;dest.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(source,dest)
        for name in selected:
            frame=manifest['frames'][name]
            result=np.zeros((160,160),np.uint8);colors=None
            for layer in ORDER:
                data,pal=indexed(stage/frame['layers'][layer],(160,160))
                if colors is not None and not np.array_equal(colors,pal):raise ValueError(name+': layer palette mismatch')
                colors=pal;result[data>0]=data[data>0]
            im=Image.fromarray(result,'P');im.putpalette(colors.ravel().tolist())
            im.save(stage/frame['file'],transparency=0)
        # Includes all poses: a new color cannot silently overflow a fighter bank.
        fighter_source(stage,names)
        for name in selected:
            relative=manifest['frames'][name]['file'];dest=folder/relative
            fd,pending=tempfile.mkstemp(prefix='.export-',dir=dest.parent)
            try:
                with os.fdopen(fd,'wb') as out:out.write((stage/relative).read_bytes())
                os.replace(pending,dest)
            finally:
                if os.path.exists(pending):os.unlink(pending)
    print(f'{folder.name}: {len(selected)} exact layer composites exported; collision metadata preserved')

if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('folder',type=Path);ap.add_argument('--pose',help='Export only this named drawing')
    args=ap.parse_args();export(args.folder,args.pose)
