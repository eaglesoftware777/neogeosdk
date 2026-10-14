"""Small exact-pixel colour edits on the accepted walk/run text maps.

Do not reassemble the accepted drawings with the older author_maps.py: the
reviewed maps contain subsequent hand edits. No geometry, pose, palette, head,
tunic, outline or timing changes. Aseprite builds the edited maps with putPixel.
"""
import json
from pathlib import Path

ART = Path(__file__).resolve().parents[2] / 'assets/authored'
# Hand-selected light pixels (y, first x, last x), before upper-body bob.
ARMS = {
    ('back', 0): [(25,33,33),(33,29,29),(34,29,29),(35,30,30)],
    ('back', 1): [(25,34,34),(34,31,31),(35,32,32)],
    ('back', 2): [(25,34,34),(35,35,35),(36,35,35)],
    ('front', 0): [(27,57,57),(29,51,51),(30,53,53),(31,52,52)],
    ('front', 1): [(27,50,50),(31,52,52),(32,53,53),(33,54,54)],
    ('front', 2): [(27,49,49),(33,49,49),(34,49,49),(37,52,52)],
}
# Knee/thigh highlight clusters. The existing warm shade remains on the rim,
# with boots untouched; lighting is on the same physical limb in both halves.
LEGS = {
    'walk_contact': {
        'far_leg': [(44,46,47),(45,46,48),(46,46,48),(47,46,48),(48,46,47)],
        'near_leg': [(43,33,36),(44,33,36),(45,31,36),(46,31,33),(47,31,34),(48,31,32)],
    },
    'walk_down': {
        'far_leg': [(44,43,44),(45,43,45),(46,43,45),(47,43,45),(48,43,44)],
        'near_leg': [(42,33,36),(43,33,36),(44,31,36),(45,31,33),(46,31,34),(47,31,32)],
    },
    'walk_passing': {
        'far_leg': [(43,38,40),(44,38,40),(45,38,40),(46,39,41),(47,39,41),(48,38,40)],
        'near_leg': [(44,39,39),(45,38,40),(46,38,39),(47,39,39)],
    },
    'walk_up': {
        'far_leg': [(43,40,42),(44,39,41),(45,39,42),(46,37,40),(47,37,40),(48,37,38)],
        'near_leg': [(42,40,41),(43,40,42),(44,40,42),(45,40,42),(46,40,41)],
    },
    'run_contact': {
        'far_leg': [(44,45,47),(45,45,48),(46,46,48),(47,46,47),(48,46,47)],
        'near_leg': [(44,34,37),(45,32,37),(46,32,36),(47,33,35),(48,32,34),(49,32,32)],
    },
    'run_down': {
        'far_leg': [(43,43,43),(44,44,46),(45,44,46),(46,45,46),(47,44,46),(48,44,46),(49,45,45)],
        'near_leg': [(42,36,37),(43,36,37),(44,36,37),(45,33,34),(46,31,33),(47,33,35)],
    },
    'run_push': {
        'far_leg': [(43,33,34),(44,32,33),(45,31,32),(46,31,32),(47,30,31),(48,30,31)],
        'near_leg': [(43,43,43),(44,44,44),(45,44,44),(46,43,43),(47,42,42)],
    },
    'run_flight': {
        'far_leg': [(41,34,36),(42,30,35),(43,31,35),(44,31,34),(45,30,33),(46,30,31)],
        'near_leg': [(41,44,45),(42,44,45),(43,46,48),(44,46,49),(45,46,46)],
    },
    'stop': {
        'far_leg': [(46,39,39),(47,38,40),(48,39,40),(49,39,39)],
        'near_leg': [(43,39,41),(44,39,39),(45,39,39),(46,39,39),(47,39,39),(48,39,41)],
    },
}


def main():
    edits = []
    for pose in json.loads((ART/'poses.json').read_text()):
        name = pose['name']
        for layer in ('far_arm','near_arm','far_leg','near_leg'):
            path = ART/'maps'/f'{name}__{layer}.txt'
            rows = [list(row) for row in path.read_text().splitlines()]
            if len(rows)!=64 or any(len(row)!=80 for row in rows):
                raise ValueError(f'{path}: expected 80x64 pixel map')
            if layer.endswith('arm'):
                xs = [x for row in rows for x,c in enumerate(row) if c!='.']
                side = 'back' if min(xs)<40 else 'front'
                spans = [(y+pose['bob'],lo,hi) for y,lo,hi in ARMS[side,pose['swing']]]
            else:
                key = name[:-2] if pose['half'] else name
                # The accepted B half exchanges the physical limb layers.
                source_layer = layer
                if pose['half']=='b':
                    source_layer = 'near_leg' if layer=='far_leg' else 'far_leg'
                spans = LEGS[key][source_layer]
            for y,lo,hi in spans:
                for x in range(lo,hi+1):
                    # Only the existing skin shade is touched. Whole-pixel
                    # outlines, boot colours and transparent margins stay exact.
                    if rows[y][x]=='9':
                        rows[y][x]='6'
                        edits.append(dict(pose=name,layer=layer,x=x,y=y,before=9,after=6))
            path.write_text('\n'.join(''.join(row) for row in rows)+'\n')
    if edits:
        (ART/'skin-polish.json').write_text(json.dumps({'method':'exact colour-only pixel edits','edits':edits},indent=2)+'\n')
    print(f'Skin polish: {len(edits)} exact shade-to-highlight pixel edits; geometry unchanged')


if __name__=='__main__':
    main()
