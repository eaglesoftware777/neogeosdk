"""Native-size contact sheets of the actual C-ROM reconstructed previews."""
from pathlib import Path
from PIL import Image, ImageDraw

ART = Path(__file__).resolve().parents[1] / "artbox/generated"


def main():
    panels = [("BANKING", [f"p{i}_plane{s}" for i in (1, 2, 3) for s in ("_left", "_left_mid", "_left_soft", "", "_right_soft", "_right_mid", "_right")], 7, 80, 62),
              ("ENEMIES", ["drone", "fighter", "interceptor", "heli", "missile_boat", "tank", "bomber", "sam", "radar_truck", "submarine", "aa_bunker", "radar_site"], 6, 80, 62),
              ("BOSSES", ["castle", "gold_core", "heli_carrier", "battleship", "red_fortress", "stealth", "tank_fort"], 4, 128, 128),
              ("PILOTS", [f"p{i}_pilot" for i in (1, 2, 3)], 3, 96, 112),
              ("PICKUPS", [f"pickup_{s}" for s in ("power", "missile", "shield", "fuel", "turbo", "core", "life", "star", "repair")], 9, 40, 36),
              ("WEAPONS", [f"weapon_{i}_{p}" for i in range(3) for p in range(5)], 5, 40, 36),
              ("HOSTILE_WEAPONS", [f"hostile_{i}" for i in range(8)], 8, 48, 40),
              ("MANEUVERS", [f"maneuver_{i}_{p}" for i in range(3) for p in range(4)], 4, 80, 62),
              ("COCKPITS", [f"cockpit_{i}" for i in range(3)], 3, 80, 128),
              ("ATTITUDE", [f"attitude_{i}" for i in range(7)], 7, 48, 48)]
    panels += [("DEFENDERS", [f"miniboss_{i}" for i in range(3)],3,80,64),
               ("COUNTERMEASURES",["chaff","radar_zone_0","radar_zone_1"],3,80,80)]
    for title, names, columns, width, height in panels:
        sheet = Image.new("RGB", (columns * width, ((len(names) + columns - 1) // columns) * height), "#263540")
        draw = ImageDraw.Draw(sheet)
        for i, name in enumerate(names):
            image = Image.open(ART / f"{name}.png").convert("RGBA")
            x, y = i % columns * width, i // columns * height
            sheet.paste(image, (x + (width - image.width) // 2, y + 12), image)
            draw.text((x + 1, y), name.replace("weapon_", "").replace("pickup_", "")[:12], fill="white")
        sheet.save(ART / f"diagnostic_{title.lower()}.png")


if __name__ == "__main__":
    main()
