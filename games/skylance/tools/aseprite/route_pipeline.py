"""Preserve exact native terrain pixels in editable indexed Aseprite documents."""
import json,subprocess
import numpy as np
from PIL import Image
from .aircraft_pipeline import ROOT,SCRIPTS,exe,path

def roundtrip(name,image,indices,palettes,mapping,sectors):
    global_indices=(mapping.repeat(16,0).repeat(16,1)*16+indices).astype(np.uint8)
    colors=np.zeros((256,3),np.uint8);colors[:len(palettes)*16]=palettes.reshape(-1,3)
    (ROOT/f'{name}_indices.bin').write_bytes(global_indices.tobytes())
    document={'width':image.width,'height':image.height,'palette':colors.tolist(),'sectors':list(sectors),'tile_palette_banks':mapping.tolist(),'native_index_format':'global index = tile bank * 16 + hardware pixel index'}
    (ROOT/f'{name}_document.json').write_text(json.dumps(document,indent=2)+'\n')
    program=exe();subprocess.run([program,'-b','--script-param','root='+path(ROOT,program),'--script-param','name='+name,'--script-param','scripts='+path(SCRIPTS,program),'--script',path(SCRIPTS/'save_route.lua',program)],check=True)
    if (ROOT/f'{name}_editor_indices.bin').read_bytes()!=global_indices.tobytes():raise ValueError(f'{name}: saved Aseprite terrain indices changed')
    with Image.open(ROOT/f'{name}_editor.png') as im:
        if im.mode!='P' or not np.array_equal(np.array(im),global_indices):raise ValueError(f'{name}: editor PNG changed pixels')
        if im.getpalette()!=colors.reshape(-1).tolist():raise ValueError(f'{name}: editor palette was not saved explicitly')
        rgba=im.convert('RGBA')
        if not np.array_equal(np.array(rgba),np.array(image)):raise ValueError(f'{name}: editor RGBA differs from authored terrain')
    return global_indices & 15,palettes,mapping
