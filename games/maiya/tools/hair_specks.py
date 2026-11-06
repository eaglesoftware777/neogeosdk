"""Stray light specks inside Maiya's hair, in hero-palette indices.

Her drawings were reduced from painted art, which left single cream/white
highlight pixels (slots 6, 14) and rose-pink pixels (slot 12) scattered in
her hair. On Maiya's gold hair they read as speckle; Luna wears the same
pixels with her own bank, where those slots stay light and show as dots on
her brown hair. A speck is an isolated light pixel with at least three
gold/yellow neighbours, no skin-shadow, green or rose-red neighbour, and at
most one other light pixel beside it -- so eyes, faces, hands, knees and
boot shine are left alone. Highlights become hair yellow (8), pink specks
the hair's olive shade (13).
"""
import numpy as np

HAIR = (2, 7, 8, 13)          # gold, brown-orange, yellow, olive
NOT_HAIR = (3, 4, 9, 10, 15)  # rose red, greens (leaves, eyes, tunic), skin shadow
LIGHT = (6, 12, 14)


def hair_specks(a):
    """{(y, x): replacement index} for every speck in an indexed frame."""
    out = {}
    h, w = a.shape
    for y, x in zip(*np.nonzero(np.isin(a, LIGHT))):
        nb = a[max(0, y - 1):y + 2, max(0, x - 1):x + 2]
        gold = int(np.isin(nb, (2, 8)).sum())
        light = int(np.isin(nb, LIGHT).sum()) - 1
        if gold >= 3 and light <= 1 and np.isin(nb, HAIR).sum() >= 4 and not np.isin(nb, NOT_HAIR).any():
            out[(int(y), int(x))] = 8 if a[y, x] in (6, 14) else 13
    return out


def smooth_hair(a):
    """A copy of the indexed frame with its hair specks recoloured."""
    a = np.array(a, copy=True)
    for (y, x), v in hair_specks(a).items():
        a[y, x] = v
    return a
