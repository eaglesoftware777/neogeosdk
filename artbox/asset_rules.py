#!/usr/bin/env python3

import configparser
import fnmatch
import json
import os


SCRIPT_ROOT = os.path.dirname(os.path.abspath(__file__))
DATA_ROOT = os.environ.get("ARTBOX_DATA_DIR", SCRIPT_ROOT)
CFG_PATH = os.path.join(DATA_ROOT, "assets.cfg")
if not os.path.isfile(CFG_PATH):
    CFG_PATH = os.path.join(SCRIPT_ROOT, "assets.cfg")
MANIFEST_PATH = os.path.join(DATA_ROOT, "assets_manifest.json")
OUT_SRT_PATH = os.path.join(DATA_ROOT, "out.srt")

# Canonical category order — determines tile/palette assignment order
CATEGORY_ORDER = [
    "backgrounds",
    "characters",
    "effects",
    "eyecatcher",
    "npc",
    "screens",
    "titles",
]


def _rule_value(section, key, default):
    if key not in section:
        return default
    return section.get(key, fallback=default)


def load_rules(cfg_path=CFG_PATH):
    cfg = configparser.ConfigParser()
    cfg.optionxform = str
    cfg.read(cfg_path, encoding="utf-8")

    rules = []
    for section_name in cfg.sections():
        if not section_name.startswith("rule:"):
            continue
        section = cfg[section_name]
        rules.append(
            {
                "name": section_name[5:],
                "match_category": _rule_value(section, "match_category", "").strip().lower(),
                "pattern": _rule_value(section, "pattern", "*.png"),
                "mode": _rule_value(section, "mode", "screen").strip().lower(),
                "category": _rule_value(section, "category", "background").strip().lower(),
                "fit": _rule_value(section, "fit", "contain").strip().lower(),
                "anchor": _rule_value(section, "anchor", "center").strip().lower(),
                "target_width": int(_rule_value(section, "target_width", "256")),
                "target_height": int(_rule_value(section, "target_height", "256")),
                "dither": _rule_value(section, "dither", "ordered").strip().lower(),
                "contrast": float(_rule_value(section, "contrast", "1.0")),
                "saturation": float(_rule_value(section, "saturation", "1.0")),
                "sharpen_radius": float(_rule_value(section, "sharpen_radius", "0.0")),
                "sharpen_percent": int(_rule_value(section, "sharpen_percent", "0")),
                "sharpen_threshold": int(_rule_value(section, "sharpen_threshold", "0")),
                "kmeans_samples": int(_rule_value(section, "kmeans_samples", "4096")),
                "kmeans_iters": int(_rule_value(section, "kmeans_iters", "16")),
                # Outer-ring halo strip (sprite-mode only).  Off by default;
                # opt-in per category so legitimate glow / highlight sprites
                # don't lose their bright outlines.
                "halo_strip": _rule_value(section, "halo_strip", "false")
                                  .strip().lower() in ("1", "true", "yes", "on"),
                "halo_luma_threshold": int(_rule_value(section,
                                                       "halo_luma_threshold",
                                                       "220")),
                "note": _rule_value(section, "note", "").strip(),
            }
        )
    return rules


def _collect_subdir_files(in_dir):
    """Return [(category, filename, full_path)] sorted by CATEGORY_ORDER then filename."""
    result = []

    # Files directly in in_dir (legacy flat layout)
    try:
        flat = sorted(
            f for f in os.listdir(in_dir)
            if f.lower().endswith(".png") and os.path.isfile(os.path.join(in_dir, f))
        )
        for f in flat:
            result.append(("", f, os.path.join(in_dir, f)))
    except FileNotFoundError:
        pass

    # Subdirectory files in CATEGORY_ORDER, then alpha-by-filename within each
    present = set(
        d for d in os.listdir(in_dir)
        if os.path.isdir(os.path.join(in_dir, d))
    )
    ordered = [d for d in CATEGORY_ORDER if d in present]
    # Append any subdirs not in CATEGORY_ORDER at the end, sorted
    ordered += sorted(d for d in present if d not in CATEGORY_ORDER)

    for subdir in ordered:
        subpath = os.path.join(in_dir, subdir)
        files = sorted(
            f for f in os.listdir(subpath) if f.lower().endswith(".png")
        )
        for f in files:
            result.append((subdir, f, os.path.join(subpath, f)))

    return result


def list_asset_files(in_dir):
    """Legacy shim — returns sorted flat filenames (for old callers)."""
    return sorted(f for f in os.listdir(in_dir) if f.lower().endswith(".png"))


def match_rule(name, rules, category=""):
    """
    Match rule by category first (match_category field), then by filename pattern.
    Falls back to a plain pattern match if no category-specific rule exists.
    """
    # Priority 1: exact category + matching filename pattern
    if category:
        for rule in rules:
            if rule["match_category"] == category.lower():
                if fnmatch.fnmatch(name, rule["pattern"]):
                    return dict(rule)
        # Priority 2: exact category with wildcard pattern
        for rule in rules:
            if rule["match_category"] == category.lower():
                return dict(rule)

    # Priority 3: legacy filename-only pattern match (no category constraint)
    for rule in rules:
        if not rule["match_category"] and fnmatch.fnmatch(name, rule["pattern"]):
            return dict(rule)

    return {
        "name": "fallback",
        "match_category": "",
        "pattern": "*.png",
        "mode": "screen",
        "category": "background",
        "fit": "contain",
        "anchor": "center",
        "target_width": 256,
        "target_height": 256,
        "dither": "floyd",
        "contrast": 1.0,
        "saturation": 1.0,
        "sharpen_radius": 0.0,
        "sharpen_percent": 0,
        "sharpen_threshold": 0,
        "kmeans_samples": 8192,
        "kmeans_iters": 25,
        "halo_strip": False,
        "halo_luma_threshold": 220,
        "note": "",
    }


def build_asset_specs(in_dir="in", cfg_path=CFG_PATH):
    entries = _collect_subdir_files(in_dir)
    rules   = load_rules(cfg_path)
    specs   = []

    for db_index, (subdir, name, full_path) in enumerate(entries):
        rule = match_rule(name, rules, category=subdir)
        screen_id = db_index + 1
        spec = {
            "db_index": db_index,
            "screen_id": screen_id,
            "name": name,
            "subdir": subdir,
            "path": full_path,
            "rule_name": rule["name"],
            "mode": rule["mode"],
            "category": subdir if subdir else rule["category"],
            "fit": rule["fit"],
            "anchor": rule["anchor"],
            "target_width": rule["target_width"],
            "target_height": rule["target_height"],
            "dither": rule["dither"],
            "contrast": rule["contrast"],
            "saturation": rule["saturation"],
            "sharpen_radius": rule["sharpen_radius"],
            "sharpen_percent": rule["sharpen_percent"],
            "sharpen_threshold": rule["sharpen_threshold"],
            "kmeans_samples": rule["kmeans_samples"],
            "kmeans_iters": rule["kmeans_iters"],
            "halo_strip": rule["halo_strip"],
            "halo_luma_threshold": rule["halo_luma_threshold"],
            "note": rule["note"],
            "tile_base": db_index * 256,
            "tile_reserved_count": 256,
            "tile_reserved_last": (db_index * 256) + 255,
            "palette_bank": 0x10 + db_index,
            "full_codegen": 1 if rule["mode"] == "screen" else 0,
            "transparent_zero": 1 if rule["mode"] == "sprite" else 0,
            "palette_has_zero": 1 if rule["mode"] == "sprite" else 0,
        }
        specs.append(spec)
    return specs


def save_manifest(specs, manifest_path=MANIFEST_PATH):
    with open(manifest_path, "w", encoding="utf-8") as handle:
        json.dump(specs, handle, indent=2)


def load_manifest(manifest_path=MANIFEST_PATH):
    with open(manifest_path, "r", encoding="utf-8") as handle:
        return json.load(handle)


def write_out_srt(specs, out_path=OUT_SRT_PATH):
    with open(out_path, "w", encoding="utf-8") as handle:
        for spec in specs:
            handle.write(f"[{spec['screen_id']:03d}] {spec.get('subdir','')}/{spec['name']}\n")
            handle.write(
                f"mode={spec['mode']} fit={spec['fit']} anchor={spec['anchor']} rule={spec['rule_name']}\n"
            )
            handle.write(f"category={spec.get('category', 'background')}\n")
            handle.write(
                f"source={spec['source_width']}x{spec['source_height']} "
                f"canvas={spec['canvas_width']}x{spec['canvas_height']} "
                f"content={spec['content_width']}x{spec['content_height']} "
                f"offset=({spec['content_left']},{spec['content_top']})\n"
            )
            handle.write(
                f"screen_id={spec['screen_id']} palette_bank={spec['palette_bank']} "
                f"tile_base={spec['tile_base']} reserved_last={spec['tile_reserved_last']} "
                f"used_cols={spec['used_tile_cols']} used_rows={spec['used_tile_rows']} "
                f"start_col={spec['used_tile_col_start']} start_row={spec['used_tile_row_start']} "
                f"x_pad={spec['content_left_mod']} y_pad={spec['content_top_mod']} "
                f"used_tiles={spec['used_tile_count']} strips={spec['sprite_strips']} "
                f"active_rows={spec['sprite_active_rows']}\n"
            )
            if spec.get("note"):
                handle.write(f"note={spec['note']}\n")
            handle.write("\n")
