#!/usr/bin/env python3
"""Pack the approved four-cell atlas into native-size shooter assets only."""

from pathlib import Path

from PIL import Image

GAME = Path(__file__).resolve().parents[1]
DEMO = GAME.parent / "demo" / "artbox"


def main():
    with Image.open(GAME / "artbox/source/shooter_icons.png") as source:
        atlas = source.convert("RGBA")
    if atlas.getchannel("A").getextrema()[0] == 255:
        raise ValueError("The atlas must have real transparency, not a painted background")

    for index, name in enumerate(("life", "missile", "speed", "bullet")):
        x, y = index % 2, index // 2
        cell = atlas.crop((x * atlas.width // 2, y * atlas.height // 2,
                           (x + 1) * atlas.width // 2, (y + 1) * atlas.height // 2))
        bounds = cell.getchannel("A").point(lambda a: 255 if a >= 128 else 0).getbbox()
        if not bounds:
            raise ValueError(f"Empty atlas cell: {name}")
        icon = cell.crop(bounds)
        icon.thumbnail((14, 14), Image.Resampling.NEAREST)
        frame = Image.new("RGBA", (16, 16))
        frame.paste(icon, ((16 - icon.width) // 2, (16 - icon.height) // 2))
        destinations = []
        if name != "bullet":
            destinations += [GAME / f"artbox/in/zzzz_pickups/item_{name}.png",
                             DEMO / f"in/zzz_shooter/item_{name}.png"]
        if name == "life":
            destinations.append(GAME / "artbox/infix/fix_life.png")
        if name == "bullet":
            destinations += [GAME / "artbox/in/characters/sprite_shot_player.png",
                             DEMO / "in/npcs/zzzzzzzzzz_sky_bullet.png"]
        if name == "missile":
            destinations.append(DEMO / "in/zzz_shooter/missile.png")
        for path in destinations:
            path.parent.mkdir(parents=True, exist_ok=True)
            frame.save(path)
            print(path.relative_to(GAME.parent.parent))


if __name__ == "__main__":
    main()
