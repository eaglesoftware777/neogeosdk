#!/usr/bin/env python3

import json


with open("assets_manifest.json", "r", encoding="utf-8") as handle:
    assets = json.load(handle)


with open("sprite_meta.h", "w", encoding="utf-8") as out:
    out.write("#ifndef ARTBOX_SPRITE_META_H\n")
    out.write("#define ARTBOX_SPRITE_META_H\n\n")
    out.write("#include <stdint.h>\n\n")
    out.write("typedef struct {\n")
    out.write("    uint16_t tile_base;\n")
    out.write("    uint8_t palette_bank;\n")
    out.write("    uint8_t strips;\n")
    out.write("    uint8_t active_rows;\n")
    out.write("    uint8_t tile_col_start;\n")
    out.write("    uint8_t tile_row_start;\n")
    out.write("    uint8_t x_pad;\n")
    out.write("    uint8_t y_pad;\n")
    out.write("    uint16_t content_width;\n")
    out.write("    uint16_t content_height;\n")
    out.write("    uint8_t mode;\n")
    out.write("    uint8_t category;\n")
    out.write("} NGSpriteAssetMeta;\n\n")
    out.write("#define NG_ASSET_MODE_SCREEN 0\n")
    out.write("#define NG_ASSET_MODE_SPRITE 1\n\n")
    out.write("#define NG_ASSET_CATEGORY_BACKGROUND 0\n")
    out.write("#define NG_ASSET_CATEGORY_MAIN_CHARACTER 1\n")
    out.write("#define NG_ASSET_CATEGORY_OPPONENT 2\n")
    out.write("#define NG_ASSET_CATEGORY_NPC 3\n\n")
    out.write("#define NG_ASSET_META_COUNT %d\n\n" % len(assets))
    out.write("static const NGSpriteAssetMeta g_ng_asset_meta[NG_ASSET_META_COUNT] = {\n")
    def infer_category(spec):
        category_name = spec.get("category")
        if category_name:
            return category_name
        name = spec.get("name", "")
        if name.startswith("z_npc_"):
            return "npc"
        if name.startswith("opponent_"):
            return "opponent"
        if name.startswith("sprite_"):
            return "main_character"
        return "background"

    for spec in assets:
        mode = 1 if spec["mode"] == "sprite" else 0
        category_name = infer_category(spec)
        category = {
            "main_character": 1,
            "opponent": 2,
            "npc": 3,
            "npcs": 3,
            "background": 0,
        }.get(category_name, 0)
        out.write(
            "    { %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d }, /* %s */\n"
            % (
                spec["tile_base"],
                spec["palette_bank"],
                spec["sprite_strips"],
                spec["sprite_active_rows"],
                spec["used_tile_col_start"],
                spec["used_tile_row_start"],
                spec["content_left_mod"],
                spec["content_top_mod"],
                spec["content_width"],
                spec["content_height"],
                mode,
                category,
                spec["name"],
            )
        )
    out.write("};\n\n")
    out.write("#endif\n")
