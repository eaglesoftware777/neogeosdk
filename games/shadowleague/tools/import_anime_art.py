"""Convert authentic anime feature film reference artwork into Neo-Geo native indexed assets.

Processes:
1. 80x96 presentation portraits for showcase fighters (Kenji, Jack, Amara)
2. 32x32 select screen portraits and head icons
3. 512x224 master arena backgrounds (Dojo, Bangkok, Brooklyn, Chimera Core)
4. 352x224 cinema cards (Title, Summons, Kenji, Jack, Amara, Protocol, Challenge)
"""

import json
from pathlib import Path
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
BRAIN_DIR = Path.home() / '.antigravity_workspace' / 'refs'
if not BRAIN_DIR.exists():
    BRAIN_DIR = list((Path.home()).glob('.*/*cli/brain/*'))[0] if list((Path.home()).glob('.*/*cli/brain/*')) else Path('.')
CONCEPT_KIT = Path("/mnt/c/neogeo/concept_kit/ShadowLeague_Anime_PixelArt_Concept_Kit")
SDK_ROOT = ROOT.parents[1]

import sys
sys.path.insert(0, str(ROOT / 'tools'))
sys.path.insert(0, str(SDK_ROOT / 'artbox'))
from native_art_io import indexed, palette_tiles
from palette_banks import fit_palette, quantize, palette_words

def quantize_indexed_16(rgb_img, mask=None):
    """Fit a single 16-color palette (index 0 transparent, indices 1..15 fitted)."""
    arr = np.array(rgb_img)
    h, w = arr.shape[:2]
    if mask is None:
        mask = np.ones((h, w), dtype=bool)
    
    fg_pixels = arr[mask]
    if len(fg_pixels) == 0:
        pal15 = np.zeros((15, 3), dtype=np.uint8)
    else:
        pal15 = fit_palette(fg_pixels, colors=15)
        
    full_pal = np.zeros((16, 3), dtype=np.uint8)
    full_pal[1:] = pal15
    
    diffs = arr[:, :, None, :] - pal15[None, None, :, :]
    dists = (diffs ** 2).sum(axis=3)
    nearest15 = dists.argmin(axis=2) + 1
    
    indexed_img = np.zeros((h, w), dtype=np.uint8)
    indexed_img[mask] = nearest15[mask]
    
    out_im = Image.fromarray(indexed_img, mode='P')
    flat_pal = np.zeros(768, dtype=np.uint8)
    flat_pal[:48] = full_pal.flatten()
    out_im.putpalette(flat_pal.tolist())
    return out_im, full_pal

def quantize_to_fixed_pal(rgb_img, pal16, mask=None):
    """Map RGB pixels to the exact fixed 16-color palette (index 0 transparent)."""
    arr = np.array(rgb_img)
    h, w = arr.shape[:2]
    if mask is None:
        mask = np.ones((h, w), dtype=bool)
    
    # We map to indices 1..15 of pal16
    diffs = arr[:, :, None, :] - pal16[None, None, 1:16, :]
    dists = (diffs ** 2).sum(axis=3)
    nearest15 = dists.argmin(axis=2) + 1
    
    indexed_img = np.zeros((h, w), dtype=np.uint8)
    indexed_img[mask] = nearest15[mask]
    
    out_im = Image.fromarray(indexed_img, mode='P')
    flat_pal = np.zeros(768, dtype=np.uint8)
    flat_pal[:48] = pal16[:16].flatten()
    out_im.putpalette(flat_pal.tolist())
    return out_im

def create_cinema_card_indexed(canvas_rgb, out_path, limit=8):
    """Quantize a 352x224 RGB image into an indexed PNG meeting palette_tiles <= 8 banks."""
    rgba = np.asarray(canvas_rgb.convert('RGBA'))
    indices, banks, assignment = quantize(rgba, bank_limit=limit)
    num_banks = len(banks)
    
    global_colors = np.zeros((256, 3), dtype=np.uint8)
    source = np.zeros((224, 352), dtype=np.uint8)
    
    for b in range(num_banks):
        for i in range(1, 16):
            global_idx = b * 15 + i
            global_colors[global_idx] = banks[b, i]
            
    for ty in range(14):
        for tx in range(22):
            b = assignment[ty, tx]
            tile_ind = indices[ty*16:(ty+1)*16, tx*16:(tx+1)*16]
            tile_src = np.zeros((16, 16), dtype=np.uint8)
            mask = tile_ind > 0
            tile_src[mask] = b * 15 + tile_ind[mask]
            source[ty*16:(ty+1)*16, tx*16:(tx+1)*16] = tile_src
            
    im = Image.fromarray(source, mode='P')
    im.putpalette(global_colors.flatten().tolist())
    im.save(out_path, transparency=0)
    
    # Verify lossless roundtrip through native_art_io
    src_read, cols_read = indexed(out_path, (352, 224))
    local, pt_banks, mapping = palette_tiles(src_read, cols_read, limit=limit)
    decoded = np.zeros((224, 352, 3), np.uint8)
    for y in range(0, 224, 16):
        for x in range(0, 352, 16):
            decoded[y:y+16, x:x+16] = pt_banks[mapping[y//16, x//16]][local[y:y+16, x:x+16]]
    expected = cols_read[src_read].copy()
    expected[src_read == 0] = 0
    if not np.array_equal(decoded, expected):
        raise ValueError(f"{out_path}: roundtrip verification failed!")
    print(f"Verified cinema card {out_path.name}: {len(pt_banks)} palette banks")

def update_presentation_portraits():
    """Convert and write 80x96 presentation portraits and update project.json."""
    portraits_dir = ROOT / "assets/authored/rebirth/portraits"
    portraits_dir.mkdir(parents=True, exist_ok=True)
    
    specs = [
        ("kenji", BRAIN_DIR / "kenji_arata_portrait_1792087395250.jpg", (80, 100, 80 + 720, 100 + 864), 55),
        ("jack", BRAIN_DIR / "jack_mercer_bust_portrait_1792087619121.jpg", (80, 100, 80 + 720, 100 + 864), 55),
        ("amara", BRAIN_DIR / "amara_silva_bust_portrait_clean_1792087819029.jpg", (80, 100, 80 + 720, 100 + 864), 55),
    ]
    
    project_path = portraits_dir / "project.json"
    project_data = json.loads(project_path.read_text()) if project_path.exists() else {"images": {}}
    
    for who, img_path, crop_box, bg_thresh in specs:
        ref = Image.open(img_path)
        crop = ref.crop(crop_box).resize((80, 96), Image.Resampling.LANCZOS)
        arr = np.array(crop)
        bg_sample = np.concatenate([arr[:12, :12], arr[:12, -12:]], axis=0)
        bg_mean = bg_sample.mean(axis=(0, 1))
        dist = np.linalg.norm(arr.astype(float) - bg_mean, axis=2)
        
        mask = np.ones((96, 80), dtype=bool)
        for y in range(96):
            for x in range(80):
                if y < 65 and (x < 18 or x > 62):
                    if dist[y, x] < bg_thresh: mask[y, x] = False
                if y < 35 and (x < 25 or x > 55):
                    if dist[y, x] < bg_thresh + 15: mask[y, x] = False
                if y < 14 and dist[y, x] < bg_thresh + 25:
                    mask[y, x] = False
                    
        out_im, pal = quantize_indexed_16(crop, mask)
        out_file = portraits_dir / f"{who}.png"
        out_im.save(out_file, transparency=0)
        
        # Verify
        data, colors = indexed(out_file, (80, 96))
        assert data.max() <= 15
        words = palette_words(colors[:16])
        assert len(words) == 16
        print(f"Verified portrait {who}: 80x96, {data.max()+1} colors")
        
        project_data["images"][who] = {
            "file": f"{who}.png",
            "canvas": [80, 96],
            "colors": colors[:16].tolist()
        }
        
    project_path.write_text(json.dumps(project_data, indent=2) + "\n")
    print("Updated rebirth/portraits/project.json")

def update_select_icons():
    """Update 32x32 select icons and head icons for showcase fighters using their exact normal palette."""
    import fighter_cook
    fighters = {f.id: f for f in fighter_cook.load_all()}
    
    specs = [
        ("kenji", BRAIN_DIR / "kenji_arata_portrait_1792087395250.jpg", (200, 150, 700, 650)),
        ("jack", BRAIN_DIR / "jack_mercer_bust_portrait_1792087619121.jpg", (200, 150, 700, 650)),
        ("amara", BRAIN_DIR / "amara_silva_bust_portrait_clean_1792087819029.jpg", (200, 150, 700, 650)),
    ]
    
    for who, img_path, crop_box in specs:
        ref = Image.open(img_path)
        crop = ref.crop(crop_box).resize((32, 32), Image.Resampling.LANCZOS)
        normal_pal, _ = fighters[who].palettes()
        
        # 32x32 border mask (corners transparent)
        mask32 = np.ones((32, 32), dtype=bool)
        mask32[0, :3] = mask32[0, -3:] = False
        mask32[1, :2] = mask32[1, -2:] = False
        mask32[2, 0] = mask32[2, -1] = False
        
        out_im32 = quantize_to_fixed_pal(crop, normal_pal, mask32)
        
        # Save to fighter authored directory
        fighter_dir = ROOT / f"assets/authored/fighters/{who}"
        if fighter_dir.exists():
            out_im32.save(fighter_dir / "portrait-small.png", transparency=0)
            out_im32.save(fighter_dir / "head.png", transparency=0)
            
        # Save to assets/authored/portraits/
        p_dir = ROOT / "assets/authored/portraits"
        p_dir.mkdir(parents=True, exist_ok=True)
        out_im32.save(p_dir / f"{who}.png", transparency=0)
        
        # 21x23 head icon
        crop21 = crop.crop((5, 4, 26, 27)).resize((21, 23), Image.Resampling.LANCZOS)
        mask21 = np.ones((23, 21), dtype=bool)
        mask21[0, :2] = mask21[0, -2:] = False
        out_im21 = quantize_to_fixed_pal(crop21, normal_pal, mask21)
        h_dir = ROOT / "assets/authored/heads"
        h_dir.mkdir(parents=True, exist_ok=True)
        out_im21.save(h_dir / f"{who}.png", transparency=0)
        print(f"Updated select icons for {who} with exact normal palette")

def update_arena_masters():
    """Update the 4 arena master PNGs (512x224 RGB) with authentic anime feature art."""
    authored_dir = ROOT / "assets/authored"
    arenas = [
        ("dojo", BRAIN_DIR / "arata_mountain_dojo_dawn_1792088251183.jpg"),
        ("bangkok", BRAIN_DIR / "bangkok_night_stadium_arena_1792088256377.jpg"),
        ("brooklyn", BRAIN_DIR / "brooklyn_underground_boxing_gym_1792088263264.jpg"),
        ("chimera", BRAIN_DIR / "veil_core_stage_bg_1792088266377.jpg"),
    ]
    for s_name, path in arenas:
        ref = Image.open(path)
        # Rescale cleanly using Lanczos to exactly 512x224
        scaled = ref.resize((512, 224), Image.Resampling.LANCZOS).convert("RGB")
        target_file = authored_dir / f"{s_name}-master.png"
        scaled.save(target_file)
        print(f"Updated master arena {target_file.name}: 512x224 RGB")

def update_cinema_cards():
    """Update title and story cards in rebirth/cinema/."""
    cinema_dir = ROOT / "assets/authored/rebirth/cinema"
    cinema_dir.mkdir(parents=True, exist_ok=True)
    
    # 1. Title Screen: 320x224 draft centered in 352x224 canvas
    title_draft_path = CONCEPT_KIT / "production_drafts/title/SLF_title_320x224_RGB.png"
    if title_draft_path.exists():
        draft = Image.open(title_draft_path)
        canvas = Image.new("RGB", (352, 224), (0, 0, 0))
        canvas.paste(draft, (16, 0))
        create_cinema_card_indexed(canvas, cinema_dir / "title.png", limit=8)
        
    # 2. Summons: Neo Tokyo 2099
    intro_ref_path = CONCEPT_KIT / "references/intro_scene_reference.png"
    if intro_ref_path.exists():
        ref = Image.open(intro_ref_path).resize((352, 224), Image.Resampling.LANCZOS)
        create_cinema_card_indexed(ref, cinema_dir / "summons.png", limit=8)
        
    # 3. Kenji Story Card: Kenji waist up with Arata Dojo background
    kenji_waist = Image.open(BRAIN_DIR / "kenji_arata_portrait_belt_1792087455330.jpg")
    dojo_bg = Image.open(BRAIN_DIR / "arata_mountain_dojo_dawn_1792088251183.jpg").resize((352, 224), Image.Resampling.LANCZOS)
    kw_crop = kenji_waist.resize((180, 224), Image.Resampling.LANCZOS)
    # Composite Kenji onto right half of Dojo bg
    card_k = dojo_bg.copy()
    card_k.paste(kw_crop, (150, 0))
    create_cinema_card_indexed(card_k, cinema_dir / "kenji.png", limit=8)
    
    # 4. Jack Story Card: Jack waist up with Brooklyn Gym background
    jack_waist = Image.open(BRAIN_DIR / "jack_mercer_waist_up_portrait_1792087689323.jpg")
    brooklyn_bg = Image.open(BRAIN_DIR / "brooklyn_underground_boxing_gym_1792088263264.jpg").resize((352, 224), Image.Resampling.LANCZOS)
    jw_crop = jack_waist.resize((180, 224), Image.Resampling.LANCZOS)
    card_j = brooklyn_bg.copy()
    card_j.paste(jw_crop, (150, 0))
    create_cinema_card_indexed(card_j, cinema_dir / "jack.png", limit=8)
    
    # 5. Amara Story Card: Amara waist up with Bangkok Stadium background
    amara_waist = Image.open(BRAIN_DIR / "amara_silva_waist_up_1792087696409.jpg")
    bangkok_bg = Image.open(BRAIN_DIR / "bangkok_night_stadium_arena_1792088256377.jpg").resize((352, 224), Image.Resampling.LANCZOS)
    aw_crop = amara_waist.resize((180, 224), Image.Resampling.LANCZOS)
    card_a = bangkok_bg.copy()
    card_a.paste(aw_crop, (150, 0))
    create_cinema_card_indexed(card_a, cinema_dir / "amara.png", limit=8)
    
    # 6. Protocol Card: Project Chimera Veil Core
    chimera_bg = Image.open(BRAIN_DIR / "veil_core_stage_bg_1792088266377.jpg").resize((352, 224), Image.Resampling.LANCZOS)
    create_cinema_card_indexed(chimera_bg, cinema_dir / "protocol.png", limit=8)
    
    # 7. Challenge Card: 3v3 team combat
    combat_ref = CONCEPT_KIT / "references/combat_keyframe_reference.png"
    if combat_ref.exists():
        ref = Image.open(combat_ref).resize((352, 224), Image.Resampling.LANCZOS)
        create_cinema_card_indexed(ref, cinema_dir / "challenge.png", limit=8)

def main():
    print("=== Processing Presentation Portraits ===")
    update_presentation_portraits()
    print("=== Processing Select Icons ===")
    update_select_icons()
    print("=== Processing Arena Masters ===")
    update_arena_masters()
    print("=== Processing Cinema Story Cards ===")
    update_cinema_cards()
    print("=== All Anime Art Assets Imported Successfully ===")

if __name__ == '__main__':
    main()
