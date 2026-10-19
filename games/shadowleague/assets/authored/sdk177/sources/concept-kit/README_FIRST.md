# Shadow League Fighting — Anime Arcade Art Concept Kit

This kit contains a cyberpunk/anime **concept board**, separated review panels, 44 **unassigned** portrait concept crops, arena concept crops, an original 320×224 title-screen draft, two monochrome indexed FIX UI draft sheets, and **44 character-specific production spec directories**.

**Important:** This is **NOT** a completed 44-character sprite library. It does **NOT** contain 44 production-ready model sheets, 2,000+ individually animated final pixel-art frames, indexed C-ROM-verified graphics, or native editable Aseprite projects. Those require further coherent hand-refined character design and animation. Portrait concepts are deliberately **unassigned**, not matched to canon fighter names, because the board did not preserve that roster.

It must not replace the existing working 44-fighter data in the game. Use this material as a separate, non-destructive art direction and proof-of-concept input to an artist/Aseprite production workflow.

## Contents
- `references/complete_concept_board.png` full 1536×1024 visual reference. It contains unapproved typography and should not be used as a game title.
- `references/` cutouts of cinematic character/action/effects/HUD/stage concepts. They remain previews, not tile-ready production sprites.
- `production_drafts/title/SLF_title_320x224_RGB.png` newly composited **correctly named** title screen raster draft.
- `production_drafts/title/SLF_title_320x224_indexed16_preview.png` a simple 16-color preview (not encoded C-ROM).
- `production_drafts/fix/` native 8×8 indexed ASCII / icon draft images (not a complete finished HUD).
- `fighter_roster.csv` names and production statuses.

## Integration advice
1. Preserve all existing project assets and branch changes.
2. Review the design sketches with the existing `assets/authored/profiles.json` identities before assigning art.
3. Draw each character in Aseprite in native indexed pixel art with an actual 16-color bank at a time and stable animation anchors.
4. Rebuild attacks pose-by-pose; never create a full sheet by shrinking or morphing one image.
5. Export lossless native frames and run the existing `fighter_validate.py`, `check_art_v2.py` and cartridge C-ROM roundtrip tests.
6. Test HUD/FIX with the real 8×8 FIX tile constraints and actual arcade gameplay captures.
7. Build the team and assist combat with the existing `ng_fight` mechanics; do not reuse proprietary commercial data.

This kit is a production starting point, **not** a finished art package.
