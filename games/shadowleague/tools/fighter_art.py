"""Skeletal pixel-art renderer for Shadow League fighters.

A pose places the hip and torso, then reaches each fist and foot to a target
with two-bone inverse kinematics. Body parts are shaded capsules (three tones
lit from the upper front) with a one-pixel cel outline, drawn far side
first. The same joints give each frame's hurtboxes and the attacking limb's
hitbox, so collision always matches the drawing.

Coordinates face right: x forward, y down; the anchor is the point between
the feet on the floor. Palette indices: 0 clear, 1 outline, 2-4 skin,
5-7 material A, 8-10 material B, 11-13 material C, 14 white, 15 accent.
"""

import math
import numpy as np

W, H = 112, 128
ANCHOR = (56, 125)
LIGHT = np.array([0.45, -0.89])
RAMPS = {"skin": (2, 3, 4), "A": (5, 6, 7), "B": (8, 9, 10), "C": (11, 12, 13),
         "white": (14, 14, 14), "accent": (15, 15, 15)}

POSE_KEYS = ("hip", "torso", "head", "near_fist", "far_fist", "near_foot", "far_foot",
             "near_elbow", "far_elbow", "near_knee", "far_knee", "fist")


def resolve(poses, name, seen=()):
    """A pose with its `base` chain applied."""
    if name in seen:
        raise ValueError(f"pose cycle at {name}")
    pose = poses[name]
    out = dict(resolve(poses, pose["base"], seen + (name,))) if "base" in pose else {}
    for key, value in pose.items():
        if key != "base":
            out[key] = value
    return out


def ik(root, target, a, b, bend):
    """Joint and end point reaching from root toward target; bend picks the side."""
    root = np.asarray(root, float)
    target = np.asarray(target, float)
    d = target - root
    dist = float(np.hypot(*d))
    if dist < 1e-6:
        d, dist = np.array([0.0, 1.0]), 1e-6
    u = d / dist
    if dist >= a + b - 0.01:
        joint = root + u * a
        return joint, root + u * (a + b)
    if dist < abs(a - b) + 0.5:
        dist = abs(a - b) + 0.5
    x = (a * a - b * b + dist * dist) / (2 * dist)
    y = math.sqrt(max(a * a - x * x, 0.0))
    n = np.array([-u[1], u[0]])
    if np.dot(n, bend) < 0:
        n = -n
    joint = root + u * x + n * y
    return joint, root + u * dist


class Canvas:
    def __init__(self):
        self.idx = np.zeros((H, W), np.uint8)
        ys, xs = np.mgrid[0:H, 0:W]
        self.px = np.stack([xs + 0.5, ys + 0.5], axis=-1)

    def _paint(self, mask, tone, ramp):
        if not mask.any():
            return
        lo, mid, hi = RAMPS[ramp]
        tones = np.where(tone > 0.33, hi, np.where(tone < -0.28, lo, mid)).astype(np.uint8)
        edge = mask & ~(np.roll(mask, 1, 0) & np.roll(mask, -1, 0) & np.roll(mask, 1, 1) & np.roll(mask, -1, 1))
        self.idx[mask] = tones[mask]
        self.idx[edge] = 1

    def capsule(self, p0, p1, r0, r1, ramp):
        p0, p1 = np.asarray(p0, float), np.asarray(p1, float)
        d = p1 - p0
        length2 = float(np.dot(d, d)) or 1e-6
        rel = self.px - p0
        t = np.clip((rel @ d) / length2, 0.0, 1.0)
        closest = p0 + t[..., None] * d
        off = self.px - closest
        dist = np.hypot(off[..., 0], off[..., 1])
        r = r0 + (r1 - r0) * t
        mask = dist <= r
        normal = off / np.maximum(r, 0.5)[..., None]
        self._paint(mask, normal @ LIGHT, ramp)

    def ellipse(self, c, rx, ry, ramp, light=True):
        off = (self.px - np.asarray(c, float)) / np.array([rx, ry])
        dist = np.hypot(off[..., 0], off[..., 1])
        mask = dist <= 1.0
        self._paint(mask, (off @ LIGHT) if light else np.zeros(mask.shape), ramp)
        return mask

    def dot(self, x, y, index):
        x, y = int(round(x)), int(round(y))
        if 0 <= x < W and 0 <= y < H:
            self.idx[y, x] = index


def skeleton(build, pose):
    """Joint positions (canvas pixels) for a resolved pose."""
    ax, ay = ANCHOR
    leg = build["thigh"] + build["shin"]
    hip = np.array([ax + pose["hip"][0], ay - build["ankle"] - leg + pose["hip"][1]], float)
    t = math.radians(pose["torso"])
    up = np.array([math.sin(t), -math.cos(t)])
    neck = hip + up * build["torso"]
    shoulder = hip + up * (build["torso"] - build["shoulder_drop"])
    h = math.radians(pose["torso"] + pose["head"])
    head = neck + np.array([math.sin(h), -math.cos(h)]) * (build["neck"] + build["head_r"])
    j = {"hip": hip, "neck": neck, "head": head, "up": up}
    for side, dx, dy in (("near", 0.0, 0.0), ("far", -build["depth"], -1.0)):
        s = shoulder + np.array([dx, dy])
        hp = hip + np.array([dx, 0.0])
        fist = s + np.asarray(pose[f"{side}_fist"], float)
        elbow_bend = pose.get(f"{side}_elbow", [-0.6, 0.8])
        j[f"{side}_shoulder"] = s
        j[f"{side}_elbow"], j[f"{side}_fist"] = ik(s, fist, build["upper_arm"], build["forearm"], elbow_bend)
        foot = np.array([ax + pose[f"{side}_foot"][0], ay - build["ankle"] + pose[f"{side}_foot"][1]], float)
        knee_bend = pose.get(f"{side}_knee", [1.0, -0.2])
        j[f"{side}_hip"] = hp
        j[f"{side}_knee"], j[f"{side}_ankle"] = ik(hp, foot, build["thigh"], build["shin"], knee_bend)
    return j


def draw_leg(c, build, j, side, mat):
    hip, knee, ankle = j[f"{side}_hip"], j[f"{side}_knee"], j[f"{side}_ankle"]
    grounded = ankle[1] >= ANCHOR[1] - build["ankle"] - 1.5
    shin_dir = (ankle - knee) / max(np.hypot(*(ankle - knee)), 1e-6)
    if grounded:
        toe = ankle + np.array([build["foot"], build["ankle"] - 1.0])
    else:
        side_n = np.array([shin_dir[1], -shin_dir[0]])
        if side_n[0] < 0:
            side_n = -side_n
        toe = ankle + (shin_dir * 0.55 + side_n * 0.83) * build["foot"]

    # Thigh with quadriceps contour
    thigh_scale = mat.get("thigh_scale", 1.0)
    c.capsule(hip, knee, build["thigh_r"] * thigh_scale, build["knee_r"] * thigh_scale, mat["thigh"])

    # Kneecap patella accent
    patella = knee + (ankle - knee) * 0.12
    c.ellipse(patella, build["knee_r"] * 0.6, build["knee_r"] * 0.6, mat["shin"])

    # Shin / calf contour
    shin_scale = mat.get("shin_scale", 1.0)
    if mat.get("shin_split"):
        mid = knee + (ankle - knee) * mat["shin_split"]
        c.capsule(knee, mid, build["knee_r"] * 0.95, build["ankle_r"] * 1.1, mat["shin"])
        c.capsule(mid, ankle, build["ankle_r"] * 1.15, build["ankle_r"] * 1.1, mat["boot"])
    else:
        # Tapered calf to ankle
        mid = knee + (ankle - knee) * 0.4
        c.capsule(knee, mid, build["knee_r"] * shin_scale * 1.08, build["ankle_r"] * shin_scale * 1.15, mat["shin"])
        c.capsule(mid, ankle, build["ankle_r"] * shin_scale * 1.15, build["ankle_r"] * shin_scale, mat["shin"])

    # Ankle wraps (Muay Lao / kickboxing)
    if mat.get("ankle_wrap"):
        wrap_mid = ankle - (ankle - knee) * 0.18
        c.capsule(wrap_mid, ankle, build["ankle_r"] * 1.18, build["ankle_r"] * 1.12, mat["ankle_wrap"])

    # Baggy martial arts trousers flare (abadá)
    if mat.get("abada"):
        cuff_l = ankle + np.array([-build["ankle_r"] * 1.5, 0])
        cuff_r = ankle + np.array([build["ankle_r"] * 1.5, 0])
        c.capsule(knee + (ankle - knee) * 0.5, ankle, build["ankle_r"] * 1.7, build["ankle_r"] * 1.4, mat["shin"])

    # Foot
    c.capsule(ankle, toe, build["ankle_r"], build["toe_r"], mat["foot"])
    if mat.get("foot") == "skin":
        # Bare foot toes definition
        toe_tip = toe + shin_dir * 1.5
        c.dot(toe_tip[0], toe_tip[1], 3)

    # Trunks / shorts
    if mat.get("trunks"):
        t_end = hip + (knee - hip) * mat["trunks"]
        c.capsule(hip, t_end, build["thigh_r"] * 1.15, build["thigh_r"] * 1.08, mat.get("shorts", mat["thigh"]))


def draw_arm(c, build, j, side, mat, fist_kind):
    s, e, f = j[f"{side}_shoulder"], j[f"{side}_elbow"], j[f"{side}_fist"]
    arm_dir = (f - e) / max(np.hypot(*(f - e)), 1e-6)
    up_dir = (e - s) / max(np.hypot(*(e - s)), 1e-6)

    # Upper arm with deltoid cap
    c.ellipse(s, build["arm_r"] * 1.15, build["arm_r"] * 1.15, mat["upper_arm"])
    c.capsule(s, e, build["arm_r"], build["elbow_r"], mat["upper_arm"])

    # Forearm
    c.capsule(e, f, build["elbow_r"] * 0.95, build["wrist_r"], mat["forearm"])

    # Sleeve
    sleeve = mat.get("sleeve", 0.0)
    if sleeve:
        c.capsule(s, s + (e - s) * sleeve, build["arm_r"] * 1.35, build["elbow_r"] * 1.4, mat.get("sleeve_mat", mat["upper_arm"]))

    # Armor pauldrons (ATLAS)
    if mat.get("armor"):
        c.ellipse(s - up_dir * 1.5, build["arm_r"] * 1.4, build["arm_r"] * 1.2, mat["armor"])

    # Wrist wraps (kaad chuek / athletic tape)
    if mat.get("wrist_wrap"):
        w_start = f - arm_dir * (build["wrist_r"] * 1.8)
        c.capsule(w_start, f, build["wrist_r"] * 1.2, build["wrist_r"] * 1.15, mat["wrist_wrap"])

    # Articulated hands
    fr = build["fist_r"]
    ortho_dir = np.array([-arm_dir[1], arm_dir[0]])
    if ortho_dir[0] < 0:
        ortho_dir = -ortho_dir

    if fist_kind == "open":
        # Open palm with extended fingers
        c.ellipse(f, fr * 0.85, fr * 0.85, mat["fist"])
        tip = f + arm_dir * (fr * 1.3)
        c.capsule(f, tip, fr * 0.75, fr * 0.45, mat["fist"])
        # Thumb flare
        thumb = f + (arm_dir * 0.5 + ortho_dir * 0.85) * (fr * 0.9)
        c.capsule(f, thumb, fr * 0.5, fr * 0.35, mat["fist"])
    elif fist_kind == "knife":
        # Karate shuto / Taekwondo spear-hand
        c.ellipse(f, fr * 0.75, fr * 0.75, mat["fist"])
        tip = f + arm_dir * (fr * 1.6)
        c.capsule(f, tip, fr * 0.7, fr * 0.3, mat["fist"])
    elif fist_kind == "claw":
        # Grappling / tiger claw: hooked fingers
        c.ellipse(f, fr * 0.9, fr * 0.9, mat["fist"])
        claw_tip = f + (arm_dir * 0.9 + ortho_dir * 0.6) * (fr * 1.1)
        c.capsule(f, claw_tip, fr * 0.6, fr * 0.3, mat["fist"])
    else:
        # Clenched fist (fist / closed)
        c.ellipse(f, fr, fr * 1.05, mat["fist"])
        knuckle = f + arm_dir * (fr * 0.65)
        c.dot(knuckle[0], knuckle[1], 1)


def draw_head(c, build, j, mat, pose):
    head, up = j["head"], j["up"]
    r = build["head_r"]
    c.capsule(j["neck"], head, build["neck_r"], build["neck_r"], "skin")
    c.ellipse(head, r, r * 1.08, "skin")

    # Hair style selection
    hair_style = mat.get("hair_style", "default")
    hair_mat = mat.get("hair", "C")
    lo, mid, hi = RAMPS[hair_mat]

    # Base head inside mask
    inside = np.hypot((c.px[..., 0] - head[0]) / (r + 1.2), (c.px[..., 1] - head[1]) / (r * 1.08 + 1.2)) <= 1.0

    if hair_style == "visor":
        # Robotic helmet / sensor visor (ATLAS)
        helmet = inside & ((c.px[..., 1] - head[1]) < r * 0.45)
        c.idx[helmet] = RAMPS[mat.get("armor", "A")][1]
        # Glowing visor scanner slit
        visor_band = inside & (np.abs(c.px[..., 1] - head[1]) <= 1.8) & ((c.px[..., 0] - head[0]) >= -r * 0.3)
        c.idx[visor_band] = RAMPS.get("accent", (15, 15, 15))[1]
        c.dot(head[0] + r * 0.5, head[1], 14)  # white gleam
        # Audio transducer ear pod
        c.ellipse(head + np.array([-r * 0.7, 0]), 2.2, 3.0, mat.get("armor", "A"))
    else:
        # Standard / spiky / braids / topknot hair
        hair_low = mat.get("hair_low", 0.0)
        hair = (c.px[..., 0] - head[0]) * 0.55 + (c.px[..., 1] - head[1]) * 1.0 < -r * 0.15 - hair_low
        back = c.px[..., 0] - head[0] < -r * 0.25
        mask = inside & (hair | back)

        tone = ((c.px - head) / r) @ LIGHT
        c.idx[mask] = np.where(tone[mask] > 0.45, hi, np.where(tone[mask] < -0.2, lo, mid))

        # Distinct hair silhouettes
        if hair_style == "braids":
            # Long cascading dreadlock braids trailing behind neck and back
            b1 = head + np.array([-r * 0.6, r * 0.3])
            b2 = head + np.array([-r * 0.8, r * 1.4])
            c.capsule(b1, b2, 2.0, 1.4, hair_mat)
            b3 = head + np.array([-r * 0.3, r * 0.4])
            b4 = head + np.array([-r * 0.5, r * 1.6])
            c.capsule(b3, b4, 1.8, 1.2, hair_mat)
        elif hair_style == "topknot":
            # Mongolian warrior topknot / shonmage atop cranium
            knot_c = head + np.array([-r * 0.1, -r * 1.15])
            c.ellipse(knot_c, 2.5, 3.2, hair_mat)
            c.capsule(knot_c, knot_c + np.array([-3.0, 3.5]), 1.4, 1.0, hair_mat)
        elif hair_style == "mongkhon":
            # Sacred Muay Lao circlet headband and trailing tail
            circlet = inside & (np.abs((c.px[..., 1] - (head[1] - r * 0.42))) < 1.6)
            c.idx[circlet] = RAMPS[mat.get("band", "white")][1]
            m_tail = head + np.array([-r - 2.5, -r * 0.35])
            c.capsule(m_tail, m_tail + np.array([-6.0, 5.0]), 1.6, 1.1, mat.get("band", "white"))
        elif hair_style == "spiky":
            # Spiky lock silhouettes extending outwards
            sp1 = head + np.array([-r * 0.3, -r * 0.9])
            c.capsule(sp1, sp1 + np.array([-3.5, -4.0]), 2.0, 0.8, hair_mat)
            sp2 = head + np.array([-r * 0.7, -r * 0.4])
            c.capsule(sp2, sp2 + np.array([-4.5, -2.5]), 1.8, 0.7, hair_mat)

        edge = mask & ~(np.roll(mask, 1, 0) & np.roll(mask, -1, 0) & np.roll(mask, 1, 1) & np.roll(mask, -1, 1))
        c.idx[edge & ~(np.roll(inside, 1, 1) & np.roll(inside, -1, 1) & np.roll(inside, 1, 0) & np.roll(inside, -1, 0))] = 1

    # Standard headband
    if mat.get("band") and hair_style != "mongkhon":
        band = inside & (np.abs((c.px[..., 1] - (head[1] - r * 0.38))) < 1.3)
        c.idx[band] = RAMPS[mat["band"]][1]
        tail = head + np.array([-r - 2.0, -r * 0.3])
        c.capsule(tail, tail + np.array([-5.0, 3.0 + (pose.get("tail", 0))]), 1.4, 1.0, mat["band"])

    # Face details: eye (sclera + pupil), brow ridge, jaw contour
    ex, ey = head[0] + r * 0.45, head[1] - r * 0.05
    c.dot(ex, ey, 14)        # white sclera
    c.dot(ex + 1, ey, 1)     # dark pupil
    c.dot(ex, ey - 2, 1)     # brow line
    c.dot(ex + 1, ey - 2, 1)
    c.dot(head[0] + r * 0.75, head[1] + r * 0.45, 2)  # mouth/jaw
    c.dot(head[0] + r * 0.98, head[1] + r * 0.05, 3)  # nose bridge


def draw_torso(c, build, j, mat):
    hip, neck = j["hip"], j["neck"]
    c.capsule(hip, neck - j["up"] * 2.0, build["waist_r"], build["chest_r"], mat["torso"])

    # Anatomical muscular contouring (pectoral plates and abdominal grid)
    if mat.get("muscle") or mat.get("open_chest") or mat["torso"] == "skin":
        chest = hip + j["up"] * (build["torso"] * 0.65)
        # Pectoral muscle plate accents
        c.dot(chest[0] + 3, chest[1] + 2, 2)
        c.dot(chest[0] + 4, chest[1] + 2, 2)
        c.dot(chest[0] + 2, chest[1] + 3, 2)
        c.dot(chest[0] + 5, chest[1] + 3, 2)
        # Rectus abdominis six-pack grid
        for k in range(3):
            a = hip + j["up"] * (7 + k * 4.5)
            c.dot(a[0] + 3, a[1], 2)
            c.dot(a[0] + 4, a[1], 2)
            c.dot(a[0] + 5, a[1] + 0.5, 3)

    # Gi / Dobok V-neck lapels
    if mat.get("gi_lapel"):
        lapel_mat = mat["gi_lapel"]
        v_pt = hip + j["up"] * (build["torso"] * 0.45)
        sh_near = neck + np.array([build["chest_r"] * 0.6, -1.0])
        sh_far = neck + np.array([-build["chest_r"] * 0.6, -1.0])
        c.capsule(sh_near, v_pt, 2.2, 1.8, lapel_mat)
        c.capsule(sh_far, v_pt, 2.0, 1.6, lapel_mat)

    # Wrestling vest / zodog
    if mat.get("vest"):
        vest_mat = mat["vest"]
        v_neck = neck - j["up"] * 2.0
        c.capsule(v_neck, hip + j["up"] * (build["torso"] * 0.5), build["chest_r"] * 1.08, build["waist_r"] * 1.05, vest_mat)
        # Open chest cutout revealing muscular chest
        c.capsule(v_neck, hip + j["up"] * (build["torso"] * 0.55), build["chest_r"] * 0.5, build["waist_r"] * 0.4, "skin")

    # Cybernetic armor plating (ATLAS)
    if mat.get("armor"):
        cuirass_mat = mat["armor"]
        c_top = neck - j["up"] * 2.0
        c.capsule(c_top, hip + j["up"] * (build["torso"] * 0.4), build["chest_r"] * 1.06, build["waist_r"] * 1.03, cuirass_mat)
        # Glowing chest reactor / power core
        core_pos = hip + j["up"] * (build["torso"] * 0.6)
        c.ellipse(core_pos, 2.4, 2.4, "accent")
        c.dot(core_pos[0], core_pos[1], 14)  # white bright core

    # Open chest V-neck cut
    if mat.get("open_chest") and not mat.get("vest"):
        v = neck - j["up"] * 9.0
        c.capsule(neck - j["up"] * 2.5, v, 2.6, 0.6, "skin")

    # Martial arts belt / obi
    if mat.get("belt"):
        c.capsule(hip - j["up"] * 1.5 + np.array([-build["waist_r"] + 1, 0]),
                  hip + j["up"] * 1.5 + np.array([build["waist_r"] - 1, 0]), 1.9, 1.9, mat["belt"])
        c.capsule(hip + np.array([build["waist_r"] - 2, 1]), hip + np.array([build["waist_r"] + 2, 8]), 1.3, 1.0, mat["belt"])

    # Waistband
    if mat.get("waistband"):
        c.capsule(hip + np.array([-build["waist_r"], -2]), hip + np.array([build["waist_r"], -2]), 2.2, 2.2, mat["waistband"])

    # Capoeira cordão / sash ties
    if mat.get("abada_cord"):
        cord_mat = mat["abada_cord"]
        cord_hip = hip + np.array([build["waist_r"] * 0.6, 0])
        c.capsule(cord_hip, cord_hip + np.array([2.0, 10.0]), 1.2, 0.9, cord_mat)


def render(build, mat, pose):
    c = Canvas()
    j = skeleton(build, pose)
    draw_arm(c, build, j, "far", mat, pose.get("fist", "closed"))
    draw_leg(c, build, j, "far", mat)
    draw_torso(c, build, j, mat)
    draw_leg(c, build, j, "near", mat)
    draw_head(c, build, j, mat, pose)
    draw_arm(c, build, j, "near", mat, pose.get("fist", "closed"))
    if pose.get("lying"):
        c.idx = np.rot90(c.idx, 1 if pose["lying"] > 0 else -1)[:H, :W] if False else c.idx
    return c.idx, j


def _box(x0, y0, x1, y1):
    """Canvas-pixel bounds to an engine box (x from anchor, y above floor)."""
    ax, ay = ANCHOR
    return [int(round(x0 - ax)), int(round(ay - y1)), int(round(x1 - x0)), int(round(y1 - y0))]


def hurtboxes(build, j, attacking=None):
    head, r = j["head"], build["head_r"]
    xs = [head[0] - r, head[0] + r, j["hip"][0] - build["waist_r"], j["hip"][0] + build["waist_r"],
          j["neck"][0] - build["chest_r"], j["neck"][0] + build["chest_r"]]
    top = head[1] - r
    upper = _box(min(xs) + 2, top + 1, max(xs) - 2, j["hip"][1])
    pts = [j[k] for k in ("near_knee", "far_knee", "near_ankle", "far_ankle", "hip")]
    lx = [p[0] for p in pts]
    ly = [p[1] for p in pts]
    lower = _box(min(lx) - 3, min(ly), max(lx) + 3, max(ly) + build["ankle"])
    boxes = [upper, lower]
    if attacking:
        tip = j[attacking]
        base = j[attacking.replace("fist", "elbow").replace("ankle", "knee")]
        boxes.append(_box(min(tip[0], base[0]) - 2, min(tip[1], base[1]) - 2, max(tip[0], base[0]) + 2, max(tip[1], base[1]) + 2))
    return boxes


def hitbox(build, j, limb):
    tip = j[limb]
    base = j[limb.replace("fist", "elbow").replace("ankle", "knee")]
    reach = build["fist_r"] + 3 if "fist" in limb else build["toe_r"] + 6
    x0 = min(tip[0], base[0] + (tip[0] - base[0]) * 0.45) - 3
    x1 = max(tip[0], base[0] + (tip[0] - base[0]) * 0.45) + reach
    y0 = min(tip[1], base[1] + (tip[1] - base[1]) * 0.45) - reach
    y1 = max(tip[1], base[1] + (tip[1] - base[1]) * 0.45) + reach
    return _box(x0, y0, x1, y1)


def ramp_colors(palette):
    """16 RGB triples from a fighter palette description."""
    out = np.zeros((16, 3), np.uint8)
    out[1] = palette.get("outline", (18, 14, 22))
    for name, (lo, mid, hi) in RAMPS.items():
        if name in palette and lo != hi:
            out[lo], out[mid], out[hi] = palette[name]
    out[14] = palette.get("white", (240, 238, 232))
    out[15] = palette.get("accent", (90, 30, 30))
    return out
