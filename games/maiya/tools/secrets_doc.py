#!/usr/bin/env python3
"""Write docs/maiya_secrets.md, the guide to every hidden place and hidden
item in Maiya's valleys, from the stage files (games/maiya/levels/*.json).

Run it again whenever a stage file changes:
    python3 games/maiya/tools/secrets_doc.py
"""
from __future__ import annotations

import json
from pathlib import Path

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
OUT = ROOT / "docs" / "maiya_secrets.md"

ROAD = 192          # the road's surface
SCREEN = 320        # one screen of valley

# How far above the surface she leaves a thing can hang (its bottom, px)
# and still be caught, from the game's jump speeds and gravity plus her
# reach: a standing jump, a running jump, the high leap (kneel, then
# Up + A), and the sky lily's second jump on top of a leap.
REACH = ((88, "a jump"), (100, "a running jump (hold B)"), (140, "the high leap (kneel, then Up + A)"),
         (190, "the sky lily's second jump (Up + A in the air)"))

LANDMARK = {"forest": "the ancient forest tree", "falls": "the cliff of the falls", "coast": "the great sea rock",
            "autumn": "the great autumn oak", "ice": "the ice peak", "worldtree": "the World Tree's trunk",
            "works": "the old works' tower, overgrown", "reef": "the coral rock", "mountain": "the silver mountain",
            "baobab": "the great baobab", "citadel": "the citadel's tower"}

SECRET = {"rose": "a golden rose", "gem": "a sun seed", "chest": "a golden rose"}

PICKUP = {
    "life": "an extra life",
    "lily": "a sky lily (for a while, Up + A in the air jumps once more)",
    "charm": "the elder's charm (read it and he tells you where the hidden vault is)",
    "bloom": "a bloom bud (one more Secret Art charge)",
    "veil": "a mist veil (nothing can touch her for a while)",
    "spring": "a spring bud (she jumps the canopy)",
    "swift": "a swift wind leaf (she runs light)",
    "might": "a thorn might berry (her strike bites harder)",
    "crown": "a thorn crown (her throw grows)",
    "thorns": "a thorn sheaf (two thorns at a time)",
    "spread": "a petal fan (three thorns at once)",
    "pierce": "a golden seed (her thorn goes through)",
    "gale": "a wind leaf (her thorn comes back)",
    "critter": "a trapped forest friend to free",
}
# Coins and flowers are everywhere and not listed one by one.
COMMON = ("gold", "silver", "flower")

ART = {"blossom": "Rose Blossom Storm", "rain": "Purifying Rain", "sun": "Sunflare Dance",
       "frost": "Frost Petal Storm", "gale": "Leaf Gale"}
ART_VALLEYS: dict[str, list[str]] = {}


def title(name: str) -> str:
    small = {"of", "the", "and"}
    words = name.lower().split()
    return " ".join(w if k and w in small else w.capitalize() for k, w in enumerate(words))


def either(names: list[str]) -> str:
    return names[0] if len(names) == 1 else ", ".join(names[:-1]) + " or " + names[-1]


def screen(x: int, width: int) -> str:
    return f"screen {x // SCREEN + 1} of {max(1, (width + SCREEN - 1) // SCREEN)}"


def surface_below(x: int, y: int, size: int, platforms) -> tuple[int, bool]:
    """The surface under something `size` px wide at (x, y): the nearest
    ledge below any of it (its y, True), else the road (192, False)."""
    best = None
    for p in platforms:
        if p["x"] <= x + size and x <= p["x"] + p["w"] and p["y"] >= y:
            if best is None or p["y"] < best:
                best = p["y"]
    return (best, True) if best is not None else (ROAD, False)


def where(x: int, y: int, size: int, d: dict) -> str:
    """Where a thing of `size` px, top-left at (x, y), sits and what it takes."""
    width = d["gate_x"] + 112 if d.get("gate_x") else d["width"]
    if d.get("mechanic") == "flight":
        return f"{screen(x, width)}, " + ("high in" if y < 70 else ("in the middle of" if y < 130 else "low in")) + " the sky"
    base, ledge = surface_below(x, y, size, d["platforms"])
    gap = base - (y + size)
    place = screen(x, width)
    tier = " in the upper tier" if y < 0 else ""
    if gap <= 12:
        on = "on a ledge" if ledge else "on the road"
        return f"{place}, {on}{tier}"
    what = "the ledge" if ledge else "the road"
    need = next((how for h, how in REACH if gap <= h), "a way up from a higher ledge nearby")
    return f"{place}{tier}, {gap} px above {what} below: {need}"


def valley(n: int, d: dict) -> list[str]:
    lines = []
    # the road ends with the gate's landmark (192 px from gate - 80)
    width = d["gate_x"] + 112 if d.get("gate_x") else d["width"]
    art = d["art"]
    flight = d.get("mechanic") == "flight"
    lines.append(f"## {n}. {title(d['name'])}")
    lines.append("")
    lines.append(f"{max(1, (width + SCREEN - 1) // SCREEN)} screens long. "
                 f"Guardian: {title(d['guardian']['name'])}. "
                 f"Secret Art: **{ART[art['kind']]}**"
                 + (" (known from the start)." if art["kind"] == "blossom" else
                    " (learned from the spirit orb in the hidden vault of "
                    + either(["the " + v for v in ART_VALLEYS[art["kind"]] if v != "Sky Road"]) + ")."))
    lines.append("")
    if flight:
        lines.append("A flight on the eagle's back: no road, no gate, no key and no vault. "
                     "Everything here floats in the open sky.")
        lines.append("")
    else:
        key = d["key"]
        lines.append(f"- **Sun Key** (opens the gate): {where(key['x'], key['y'], 16, d)}.")
        gate = d.get("gate_x", 0)
        if gate:
            lines.append(f"- **The gate**: the very end of the road, in the foot of {LANDMARK[d['landmark']]}; "
                         "nothing lies past it. Without the key it stays shut; through it, the guardian.")
        h = d["hideout"]
        spot = "in the upper tier" if h["y"] < 0 else ("on the road" if h["y"] == ROAD else "on a ledge")
        orb = ""
        if art["kind"] != "blossom":
            orb = (f" Its first treasure is the **spirit orb of the {ART[art['kind']]}**, "
                   "if she hasn't learned it yet.")
        lines.append(f"- **Hidden vault**: {screen(h['x'], width)}, {spot} "
                     f"(x {h['x']}). The elder's clue: \"{h['hint']}\". Kneel on the spot "
                     "(hold Down) for half a second: ten seconds of gold, silver, a flower "
                     f"and a life.{orb} The spot glints now and then; after the charm, it sparkles.")
    secrets = d.get("secrets", [])
    if secrets:
        lines.append(f"- **Secrets** ({len(secrets)}; each is 1,000 points, counts for the rank, "
                     "and lets her lift a pool of poison):")
        for s in sorted(secrets, key=lambda s: s["x"]):
            lines.append(f"  - {SECRET[s['type']]}: {where(s['x'], s['y'], 32, d)}.")
    special = [p for p in d.get("pickups", []) if p["kind"] not in COMMON]
    if special:
        lines.append("- **Hidden and special items**:")
        for p in sorted(special, key=lambda p: p["x"]):
            lines.append(f"  - {PICKUP.get(p['kind'], p['kind'])}: {where(p['x'], p['y'], 32, d)}.")
    coins = [p for p in d.get("pickups", []) if p["kind"] in COMMON]
    high = [p for p in coins if p["y"] < 150]
    if high:
        lines.append(f"- **Coins and flowers up high**: "
                     + "; ".join(f"{p['kind']} on {screen(p['x'], width)}" + (" (upper tier)" if p["y"] < 0 else "")
                                 for p in sorted(high, key=lambda p: p["x"])) + ".")
    if d.get("upper"):
        climbs = [v for v in d.get("vines", []) if v["top"] < 0]
        how = ", ".join(screen(v["x"], width) for v in climbs) or "the tall climbs"
        lines.append(f"- **Upper tier**: a second level above the screen, reached by the tall climb on {how}.")
    toxic = [z for z in d.get("hazards", []) if z["type"] == "toxic"]
    if toxic:
        lines.append("- **Poison pools she can lift** (stand by one with a secret found, press Up): "
                     + ", ".join(screen(z["x"], width) for z in toxic) + ".")
    rescues = d.get("rescues", [])
    if rescues:
        lines.append("- **Captives to free**: " + ", ".join(
            f"the {r['who']} on {screen(r['x'], width)}" for r in rescues) + ".")
    npcs = d.get("npcs", [])
    if npcs:
        lines.append("- **What the villagers tell her**: " + "; ".join(
            f"\"{v['line']}\" ({screen(v['x'], width)})" for v in npcs) + ".")
    lines.append("")
    return lines


def main() -> None:
    stages = [json.loads(p.read_text()) for p in sorted((GAME / "levels").glob("*.json"))]
    for d in stages:
        ART_VALLEYS.setdefault(d["art"]["kind"], []).append(title(d["name"]))
    out = [
        "# Maiya: every hidden place and hidden item",
        "",
        "Written by `games/maiya/tools/secrets_doc.py` from the stage files; run it",
        "again after a stage changes. \"Screen 4 of 16\" counts screens from the",
        "valley's start; heights are how far above the ledge or road below a thing",
        "hangs, and what it takes to reach it.",
        "",
        "## How things hide",
        "",
        "- **The Sun Key** hangs just above a ledge. The Ancient Nature Gate at the",
        "  valley's end stays shut without it.",
        "- **Secrets** (golden roses and sun seeds) hang in hard places: high ledges,",
        "  behind falls, in an upper tier. Each is worth 1,000 points and counts for",
        "  the valley's rank, and once she carries one she can lift a pool of poison:",
        "  stand by it and press Up.",
        "- **The elder's charm** is a scroll lying somewhere in the valley. Take it and",
        "  the elder tells her where the valley's hidden vault is; its spot then",
        "  sparkles instead of glinting now and then.",
        "- **The hidden vault**: kneel (hold Down) on its spot for half a second. For",
        "  ten seconds, treasure rains on two shelves: gold, silver, a flower and a",
        "  life. Once a valley.",
        "- **Spirit orbs and the Secret Arts** (D). She knows the Rose Blossom Storm",
        "  from the start. The other four are learned from a spirit orb, the first",
        "  treasure of the hidden vault in a valley of their kind:",
    ]
    for kind in ("rain", "sun", "frost", "gale"):
        out.append(f"  - **{ART[kind]}**: " + ", ".join(ART_VALLEYS.get(kind, [])) + ".")
    out += [
        "  In a valley she uses its own art once she knows it, the rose storm until",
        "  then.",
        "- **Reaching high places**: a running jump (hold B) goes higher than a",
        "  standing one; kneel, then Up + A, for the high leap; with a **sky lily**,",
        "  Up + A in the air jumps once more. Lilies sit before the high places",
        "  that need them.",
        "- **Upper tiers**: some valleys have a second level above the screen,",
        "  reached by a tall climb (press Up at the vine, ladder or rope).",
        "- **Villagers** pass on hints; so do the elder's words before each valley.",
        "",
    ]
    for n, d in enumerate(stages, 1):
        out += valley(n, d)
    OUT.write_text("\n".join(out).rstrip() + "\n")
    print(f"secrets_doc: {len(stages)} valleys -> {OUT.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
