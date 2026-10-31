"""Initial native-grid tournament lettering. Normal builds import the PNGs.

Contours are specified at the final pixel size, with separate small and impact
alphabets. This command refuses to overwrite an existing artwork directory.
"""
from pathlib import Path
import json
import numpy as np
from PIL import Image, ImageDraw

OUT = Path(__file__).resolve().parents[2] / "assets/authored/rebirth/announcements"
# Width, outer contours followed by counters. Thirteen-pixel cap height.
SMALL = {
 "A": (12, [[(4,0),(9,0),(12,13),(8,13),(8,10),(4,10),(3,13),(0,13)],[(6,4),(5,7),(8,7),(7,4)]]),
 "B": (11, [[(2,0),(9,0),(11,2),(10,6),(11,8),(10,11),(7,13),(0,13)],[(5,3),(5,5),(8,5),(8,3)],[(4,8),(3,10),(7,10),(8,8)]]),
 "C": (11, [[(5,0),(11,0),(10,3),(6,3),(4,5),(3,9),(5,10),(9,10),(8,13),(3,13),(0,10),(1,5)]]),
 "D": (12, [[(2,0),(9,0),(12,3),(10,10),(6,13),(0,13)],[(5,3),(4,10),(6,10),(8,8),(9,4),(8,3)]]),
 "E": (11, [[(2,0),(11,0),(10,3),(5,3),(5,5),(9,5),(8,8),(4,8),(4,10),(9,10),(8,13),(0,13)]]),
 "F": (11, [[(2,0),(11,0),(10,3),(5,3),(5,5),(9,5),(8,8),(4,8),(3,13),(0,13)]]),
 "G": (12, [[(5,0),(12,0),(11,3),(6,3),(4,5),(3,9),(5,10),(8,10),(8,8),(6,8),(7,5),(12,5),(10,11),(7,13),(3,13),(0,10),(1,5)]]),
 "H": (12, [[(2,0),(5,0),(5,5),(9,5),(10,0),(13,0),(11,13),(8,13),(8,8),(4,8),(3,13),(0,13)]]),
 "I": (6, [[(2,0),(6,0),(4,13),(0,13)]]),
 "K": (12, [[(2,0),(5,0),(5,5),(9,0),(13,0),(7,7),(11,13),(7,13),(4,9),(3,13),(0,13)]]),
 "L": (10, [[(2,0),(6,0),(4,10),(10,10),(9,13),(0,13)]]),
 "M": (15, [[(2,0),(6,0),(9,6),(12,0),(16,0),(14,13),(11,13),(12,6),(8,11),(5,6),(4,13),(0,13)]]),
 "N": (13, [[(2,0),(5,0),(9,7),(10,0),(13,0),(11,13),(8,13),(4,6),(3,13),(0,13)]]),
 "O": (12, [[(5,0),(10,0),(12,3),(11,9),(7,13),(2,13),(0,10),(1,4)],[(5,3),(4,5),(3,9),(4,10),(6,10),(8,8),(9,4),(8,3)]]),
 "P": (12, [[(2,0),(10,0),(12,3),(10,7),(7,9),(4,9),(3,13),(0,13)],[(5,3),(5,6),(7,6),(9,4),(8,3)]]),
 "R": (12, [[(2,0),(10,0),(12,3),(10,7),(8,8),(11,13),(7,13),(4,9),(3,13),(0,13)],[(5,3),(5,6),(7,6),(9,4),(8,3)]]),
 "S": (11, [[(5,0),(11,0),(10,3),(5,3),(4,5),(9,5),(11,7),(9,11),(6,13),(0,13),(1,10),(6,10),(7,8),(3,8),(1,6),(2,3)]]),
 "T": (12, [[(1,0),(13,0),(12,3),(8,3),(6,13),(2,13),(4,3),(0,3)]]),
 "U": (12, [[(2,0),(5,0),(4,9),(5,10),(7,10),(8,9),(10,0),(13,0),(11,10),(7,13),(3,13),(0,10)]]),
 "V": (13, [[(0,0),(4,0),(5,9),(10,0),(14,0),(7,13),(3,13)]]),
 "W": (17, [[(0,0),(4,0),(4,8),(9,1),(12,1),(12,8),(16,0),(20,0),(13,13),(9,13),(9,7),(5,13),(2,13)]]),
 "Y": (12, [[(0,0),(4,0),(6,5),(10,0),(14,0),(8,8),(7,13),(3,13),(4,8)]]),
 "1": (7, [[(3,0),(7,0),(5,10),(7,10),(6,13),(0,13),(1,10),(2,10),(3,4),(1,4),(2,1)]]),
 "2": (11, [[(3,0),(9,0),(11,2),(10,6),(4,10),(10,10),(9,13),(0,13),(1,9),(7,4),(7,3),(2,3)]]),
 "3": (11, [[(2,0),(10,0),(11,2),(9,6),(10,8),(8,12),(5,13),(0,13),(1,10),(6,10),(7,8),(3,8),(4,5),(7,5),(8,3),(1,3)]]),
 "!": (5, [[(2,0),(6,0),(4,8),(1,8)],[(0,11),(3,11),(3,13),(0,13)]]),
 ".": (3, [[(0,10),(3,10),(3,13),(0,13)]]),
}
# Separate native impact contours, not a resized font bitmap.
IMPACT = {
 "F": (21, [[(4,0),(22,0),(21,5),(9,5),(8,9),(18,9),(17,14),(7,14),(6,21),(0,21)]]),
 "I": (10, [[(4,0),(11,0),(7,21),(0,21)]]),
 "G": (24, [[(10,0),(24,0),(23,5),(12,5),(8,9),(6,15),(8,17),(15,17),(16,13),(12,13),(13,9),(23,9),(21,17),(16,21),(6,21),(0,16),(2,7)]]),
 "H": (25, [[(4,0),(11,0),(9,8),(20,8),(21,0),(28,0),(24,21),(17,21),(18,13),(8,13),(6,21),(0,21)]]),
 "T": (26, [[(2,0),(28,0),(27,5),(17,5),(14,21),(7,21),(10,5),(0,5)]]),
 "K": (25, [[(4,0),(11,0),(9,8),(19,0),(27,0),(14,11),(22,21),(14,21),(8,14),(7,21),(0,21)]]),
 "O": (26, [[(10,0),(22,0),(28,5),(25,15),(17,21),(5,21),(0,16),(3,6)],[(12,5),(9,8),(7,14),(9,16),(15,16),(18,13),(20,7),(18,5)]]),
 "!": (8, [[(4,0),(11,0),(7,14),(1,14)],[(0,18),(6,18),(6,21),(0,21)]]),
 ".": (6, [[(0,16),(6,16),(6,21),(0,21)]]),
}


def lean(*shapes):
    """Upright impact contours given the alphabet's four-pixel forward lean."""
    return [[(x + round((21 - y) * 4 / 21), y) for x, y in shape] for shape in shapes]


# The ceremony calls: drawn upright on a seven-pixel stroke, then leaned.
IMPACT.update({
 "A": (25, lean([(5,0),(19,0),(24,5),(24,21),(17,21),(17,16),(7,16),(7,21),(0,21),(0,5)],
                [(9,5),(15,5),(17,7),(17,11),(7,11),(7,7)])),
 "B": (25, lean([(0,0),(19,0),(23,4),(23,8),(21,10),(24,13),(24,18),(21,21),(0,21)],
                [(7,5),(16,5),(16,8),(7,8)], [(7,13),(17,13),(17,16),(7,16)])),
 "C": (23, lean([(6,0),(22,0),(22,5),(9,5),(7,7),(7,14),(9,16),(22,16),(22,21),(6,21),(0,15),(0,6)])),
 "L": (21, lean([(0,0),(7,0),(7,16),(20,16),(20,21),(0,21)])),
 "S": (25, lean([(5,0),(22,0),(22,5),(8,5),(7,6),(7,8),(19,8),(24,12),(24,17),(20,21),(0,21),(0,16),
                 (16,16),(17,15),(17,13),(5,13),(0,9),(0,4)])),
 "U": (25, lean([(0,0),(7,0),(7,16),(17,16),(17,0),(24,0),(24,17),(20,21),(4,21),(0,17)])),
 "1": (16, lean([(3,0),(11,0),(11,16),(15,16),(15,21),(0,21),(0,16),(4,16),(4,7),(0,7),(0,4)])),
 "2": (24, lean([(0,0),(19,0),(23,4),(23,10),(19,13),(8,13),(8,16),(23,16),(23,21),(0,21),(0,12),
                 (4,8),(16,8),(16,5),(0,5)])),
 "3": (24, lean([(0,0),(19,0),(23,4),(23,8),(21,10),(23,13),(23,18),(20,21),(0,21),(0,16),(16,16),
                 (16,13),(6,13),(6,8),(16,8),(16,5),(0,5)])),
})
WORDS = {
 "ROUND1":"ROUND 1", "ROUND2":"ROUND 2", "ROUND3":"ROUND 3", "FINAL":"FINAL ROUND",
 "FIGHT":"FIGHT!", "KO":"K.O.", "TIMEUP":"TIME OVER", "WIN":"VICTORY", "DRAW":"DRAW",
 "VS":"VS", "PERFECT":"PERFECT", "GAMEOVER":"GAME OVER", "COUNTER":"COUNTER",
 "GUARDBREAK":"GUARD BREAK", "CONGRATS":"CONGRATULATIONS", "CHALLENGER":"CHALLENGER!",
 # The referee's ceremony and the league's own calls.
 "COUNT3":"3", "COUNT2":"2", "COUNT1":"1", "CLASH":"CLASH!", "BLACKOUT":"BLACKOUT",
 "UNTOUCHED":"UNTOUCHED", "NIGHT":"THE NIGHT IS YOURS", "RISE":"RISE AGAIN",
}
IMPACT_WORDS = {"FIGHT", "KO", "COUNT3", "COUNT2", "COUNT1", "CLASH", "BLACKOUT"}
PALETTE = [(0,0,0),(20,10,8),(255,240,176),(236,164,48),(160,80,32),(80,32,24),(255,255,224),(48,24,32)]


def word(text, impact=False):
    alphabet = IMPACT if impact else SMALL
    width = 10 + sum(6 if c == " " else alphabet[c][0] + 3 for c in text)
    width = (width + 7) // 8 * 8
    mask = Image.new("1", (width,32 if impact else 24))
    d = ImageDraw.Draw(mask)
    x, top = 4, 3
    for c in text:
        if c == " ":
            x += 6
            continue
        advance, shapes = alphabet[c]
        for n, points in enumerate(shapes):
            # Exclamation's detached dot is a second solid shape.
            d.polygon([(x+a,top+b) for a,b in points], fill=1 if n==0 or c=="!" else 0)
        x += advance + 3
    a = np.asarray(mask)
    result = np.zeros(a.shape, np.uint8)
    for dx,dy,c in [(3,3,7),(2,2,5),(1,1,1),(-1,0,1),(0,-1,1),(1,0,1),(0,1,1)]:
        y0,y1=max(0,dy),min(a.shape[0],a.shape[0]+dy)
        x0,x1=max(0,dx),min(a.shape[1],a.shape[1]+dx)
        shifted=a[y0-dy:y1-dy,x0-dx:x1-dx]
        region=result[y0:y1,x0:x1];region[shifted]=c
    cap=21 if impact else 13
    bands=[6,2,2,3,4,3]
    for y in range(top,top+cap+1):
        result[y,a[y]]=bands[min(5,(y-top)*6//(cap+1))]
    upper=a & ~np.roll(a,1,axis=0)
    result[upper]=6
    im=Image.fromarray(result,"P")
    im.putpalette([v for color in PALETTE for v in color]+[0]*(768-3*len(PALETTE)))
    return im


def main():
    """With no arguments, author the whole set into a new folder. With word
    names, add just those words to the existing set, never replacing one."""
    import sys
    add = sys.argv[1:]
    unknown = [name for name in add if name not in WORDS]
    if unknown:
        raise SystemExit("unknown words: " + " ".join(unknown))
    if add:
        definitions = json.loads((OUT/"project.json").read_text())["images"]
        known = {entry["name"] for entry in definitions}
        taken = [name for name in add if name in known or (OUT/(name.lower()+".png")).exists()]
        if taken:
            raise SystemExit("already authored: " + " ".join(taken))
    else:
        OUT.mkdir(parents=True, exist_ok=False)
        definitions = []
    for name in add or WORDS:
        text = WORDS[name]
        im=word(text,name in IMPACT_WORDS);im.save(OUT/(name.lower()+".png"),transparency=0)
        a=np.asarray(im)
        (OUT/(name.lower()+".txt")).write_text("\n".join("".join("." if i==0 else format(i,"X") for i in row) for row in a)+"\n")
        definitions.append({"name":name,"text":text,"canvas":list(im.size),"file":name.lower()+".png"})
    (OUT/"project.json").write_text(json.dumps({"transparent":0,"palette":PALETTE,"images":definitions},indent=2)+"\n")
    sheet=Image.new("RGB",(640,32*((len(definitions)+1)//2)),(16,18,28))
    for n,entry in enumerate(definitions):
        im=Image.open(OUT/entry["file"]).convert("RGBA");sheet.paste(im,(n%2*320,n//2*32),im)
    sheet.save(OUT/"contact-native.png")
    sheet.resize((1280,sheet.height*2),Image.Resampling.NEAREST).save(OUT/"contact-2x.png")
    print(OUT)


if __name__ == "__main__":
    main()
