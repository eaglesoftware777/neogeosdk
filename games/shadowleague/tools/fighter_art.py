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

    def paint_smooth(self, mask, tone, ramp):
        if not mask.any():
            return
        lo, mid, hi = RAMPS[ramp]
        tones = np.where(tone > 0.30, hi, np.where(tone < -0.22, lo, mid)).astype(np.uint8)
        self.idx[mask] = tones[mask]

    def outline_mask(self, mask, color=1):
        if not mask.any():
            return
        edge = mask & ~(np.roll(mask, 1, 0) & np.roll(mask, -1, 0) & np.roll(mask, 1, 1) & np.roll(mask, -1, 1))
        self.idx[edge] = color

    def capsule_mask(self, p0, p1, r0, r1):
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
        tone = normal @ LIGHT
        return mask, tone

    def ellipse_mask(self, c, rx, ry):
        off = (self.px - np.asarray(c, float)) / np.array([rx, ry])
        dist = np.hypot(off[..., 0], off[..., 1])
        mask = dist <= 1.0
        tone = off @ LIGHT
        return mask, tone

    def capsule(self, p0, p1, r0, r1, ramp, outline=True):
        mask, tone = self.capsule_mask(p0, p1, r0, r1)
        self.paint_smooth(mask, tone, ramp)
        if outline:
            self.outline_mask(mask)

    def ellipse(self, c, rx, ry, ramp, light=True, outline=True):
        mask, tone = self.ellipse_mask(c, rx, ry)
        self.paint_smooth(mask, tone if light else np.zeros(mask.shape), ramp)
        if outline:
            self.outline_mask(mask)
        return mask

    def dot(self, x, y, index):
        x, y = int(round(x)), int(round(y))
        if 0 <= x < W and 0 <= y < H:
            self.idx[y, x] = index

    def line(self, p0, p1, index):
        p0, p1 = np.asarray(p0, float), np.asarray(p1, float)
        dist = int(math.ceil(np.hypot(*(p1 - p0)))) or 1
        for step in range(dist + 1):
            pt = p0 + (p1 - p0) * (step / dist)
            self.dot(pt[0], pt[1], index)


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

    thigh_scale = mat.get("thigh_scale", 1.0)
    shin_scale = mat.get("shin_scale", 1.0)
    thigh_m, thigh_t = c.capsule_mask(hip, knee, build["thigh_r"] * thigh_scale * 1.15, build["knee_r"] * thigh_scale * 1.15)
    shin_m, shin_t = c.capsule_mask(knee, ankle, build["knee_r"] * shin_scale * 1.15, build["ankle_r"] * shin_scale * 1.25)
    foot_m, foot_t = c.capsule_mask(ankle, toe, build["ankle_r"], build["toe_r"])

    if mat.get("trunks"):
        t_end = hip + (knee - hip) * mat["trunks"]
        trunks_m, trunks_t = c.capsule_mask(hip, t_end, build["thigh_r"] * 1.25, build["thigh_r"] * 1.18)
        skin_m = (thigh_m & ~trunks_m) | shin_m
        skin_t = np.where(shin_m, shin_t, thigh_t)
        c.paint_smooth(skin_m, skin_t, "skin")
        c.outline_mask(skin_m)
        c.paint_smooth(trunks_m, trunks_t, mat.get("shorts", mat["thigh"]))
        stripe_mat = mat.get("waistband", "white")
        c.line(hip + np.array([build["thigh_r"] * 0.8, 0]),
               t_end + np.array([build["thigh_r"] * 0.7, 0]), RAMPS.get(stripe_mat, (14, 14, 14))[1])
        c.outline_mask(trunks_m)
    else:
        leg_m = thigh_m | shin_m
        leg_t = np.where(shin_m, shin_t, thigh_t)
        c.paint_smooth(leg_m, leg_t, mat["thigh"])
        knee_fwd = np.array([-shin_dir[1], shin_dir[0]])
        c.line(knee - knee_fwd * 3, knee + knee_fwd * 3 + shin_dir * 1.5, RAMPS[mat["thigh"]][0])
        c.line(ankle - knee_fwd * 3, ankle + knee_fwd * 3, RAMPS[mat["thigh"]][0])
        if mat.get("abada"):
            cuff_m, cuff_t = c.capsule_mask(knee + (ankle - knee) * 0.5, ankle, build["ankle_r"] * 1.7, build["ankle_r"] * 1.4)
            c.paint_smooth(cuff_m, cuff_t, mat["shin"])
        c.outline_mask(leg_m)

    if mat.get("shin_split"):
        boot_start = knee + (ankle - knee) * mat["shin_split"]
        boot_m, boot_t = c.capsule_mask(boot_start, ankle, build["ankle_r"] * 1.25, build["ankle_r"] * 1.15)
        c.paint_smooth(boot_m, boot_t, mat["boot"])
        c.outline_mask(boot_m)
    if mat.get("ankle_wrap"):
        wrap_mid = ankle - (ankle - knee) * 0.18
        wrap_m, wrap_t = c.capsule_mask(wrap_mid, ankle, build["ankle_r"] * 1.2, build["ankle_r"] * 1.15)
        c.paint_smooth(wrap_m, wrap_t, mat["ankle_wrap"])
        c.outline_mask(wrap_m)

    c.paint_smooth(foot_m, foot_t, mat["foot"])
    if mat.get("foot") == "skin":
        c.dot(toe[0], toe[1], 4)
        c.dot(toe[0] - 1, toe[1], 3)
    elif mat.get("vest"):
        c.line(toe, toe + np.array([3.0, -2.5]), RAMPS[mat["boot"]][1])
    c.outline_mask(foot_m)


def draw_arm(c, build, j, side, mat, fist_kind):
    s, e, f = j[f"{side}_shoulder"], j[f"{side}_elbow"], j[f"{side}_fist"]
    arm_dir = (f - e) / max(np.hypot(*(f - e)), 1e-6)
    up_dir = (e - s) / max(np.hypot(*(e - s)), 1e-6)

    delt_m, delt_t = c.ellipse_mask(s, build["arm_r"] * 1.3, build["arm_r"] * 1.3)
    u_arm_m, u_arm_t = c.capsule_mask(s, e, build["arm_r"] * 1.05, build["elbow_r"] * 1.05)
    fore_m, fore_t = c.capsule_mask(e, f, build["elbow_r"] * 0.95, build["wrist_r"])

    sleeve = mat.get("sleeve", 0.0)
    if sleeve:
        sl = s + (e - s) * sleeve
        sl_m, sl_t = c.capsule_mask(s, sl, build["arm_r"] * 1.4, build["elbow_r"] * 1.45)
        c.paint_smooth(delt_m | sl_m, np.where(sl_m, sl_t, delt_t), mat.get("sleeve_mat", mat["upper_arm"]))
        c.line(sl + np.array([-build["elbow_r"] * 1.2, 0]), sl + np.array([build["elbow_r"] * 1.2, 0]), RAMPS[mat["upper_arm"]][0])
        fore_vis_m, fore_vis_t = c.capsule_mask(sl, f, build["elbow_r"] * 0.95, build["wrist_r"])
        c.paint_smooth(fore_vis_m, fore_vis_t, mat["forearm"])
        c.outline_mask(delt_m | sl_m | fore_vis_m)
    else:
        arm_m = delt_m | u_arm_m | fore_m
        c.paint_smooth(arm_m, np.where(fore_m, fore_t, np.where(u_arm_m, u_arm_t, delt_t)), mat["upper_arm"])
        c.dot(e[0], e[1], RAMPS[mat["upper_arm"]][2])
        c.outline_mask(arm_m)

    if mat.get("band") and mat.get("torso") == "skin":
        p_arm = s + (e - s) * 0.38
        p_m, _ = c.ellipse_mask(p_arm, build["arm_r"] * 1.25, build["arm_r"] * 0.9)
        c.paint_smooth(p_m, np.zeros(p_m.shape), mat.get("band", "white"))
        c.line(p_arm, p_arm + np.array([-4.0, 5.5]), RAMPS[mat.get("band", "white")][1])
    if mat.get("armor"):
        p_m, p_t = c.ellipse_mask(s - up_dir * 1.5, build["arm_r"] * 1.4, build["arm_r"] * 1.2)
        c.paint_smooth(p_m, p_t, mat["armor"])
        c.dot(s[0] - up_dir[0] * 1.5, s[1] - up_dir[1] * 1.5, 15)
        c.outline_mask(p_m)
    if mat.get("wrist_wrap"):
        w_start = f - arm_dir * (build["wrist_r"] * 1.8)
        w_m, w_t = c.capsule_mask(w_start, f, build["wrist_r"] * 1.25, build["wrist_r"] * 1.15)
        c.paint_smooth(w_m, w_t, mat["wrist_wrap"])
        c.outline_mask(w_m)
    if mat.get("boot") and mat.get("fist") == "B":
        w_lace = f - arm_dir * (build["fist_r"] * 0.85)
        c.line(w_lace, f - arm_dir * (build["fist_r"] * 0.35), 14)

    fr = build["fist_r"]
    ortho_dir = np.array([-arm_dir[1], arm_dir[0]])
    if ortho_dir[0] < 0: ortho_dir = -ortho_dir

    if fist_kind == "open":
        fist_m, fist_t = c.ellipse_mask(f, fr * 0.85, fr * 0.85)
        tip = f + arm_dir * (fr * 1.3)
        fingers_m, fingers_t = c.capsule_mask(f, tip, fr * 0.75, fr * 0.45)
        c.paint_smooth(fist_m | fingers_m, np.where(fingers_m, fingers_t, fist_t), mat["fist"])
        c.outline_mask(fist_m | fingers_m)
    elif fist_kind == "knife":
        fist_m, fist_t = c.ellipse_mask(f, fr * 0.75, fr * 0.75)
        tip = f + arm_dir * (fr * 1.6)
        shuto_m, shuto_t = c.capsule_mask(f, tip, fr * 0.7, fr * 0.3)
        c.paint_smooth(fist_m | shuto_m, np.where(shuto_m, shuto_t, fist_t), mat["fist"])
        c.outline_mask(fist_m | shuto_m)
    elif fist_kind == "claw":
        fist_m, fist_t = c.ellipse_mask(f, fr * 0.9, fr * 0.9)
        claw_tip = f + (arm_dir * 0.9 + ortho_dir * 0.6) * (fr * 1.1)
        claw_m, claw_t = c.capsule_mask(f, claw_tip, fr * 0.6, fr * 0.3)
        c.paint_smooth(fist_m | claw_m, np.where(claw_m, claw_t, fist_t), mat["fist"])
        c.outline_mask(fist_m | claw_m)
    else:
        fist_m, fist_t = c.ellipse_mask(f, fr * 1.05, fr * 1.1)
        c.paint_smooth(fist_m, fist_t, mat["fist"])
        c.line(f + arm_dir * (fr * 0.5) - np.array([0, 2]), f + arm_dir * (fr * 0.5) + np.array([0, 2]), 1)
        c.outline_mask(fist_m)


def draw_head(c, build, j, mat, pose):
    head, up = j["head"], j["up"]
    r = build["head_r"]
    neck_m, _ = c.capsule_mask(j["neck"], head, build["neck_r"], build["neck_r"])
    c.idx[neck_m] = RAMPS["skin"][1]
    head_m, head_t = c.ellipse_mask(head, r * 0.95, r * 1.1)
    c.paint_smooth(head_m, head_t, "skin")

    hair_style = mat.get("hair_style", "default")
    hair_mat = mat.get("hair", "C")
    lo_h, mid_h, hi_h = RAMPS[hair_mat]

    if hair_style == "visor":
        helmet = head_m & ((c.px[..., 1] - head[1]) < r * 0.45)
        c.idx[helmet] = RAMPS[mat.get("armor", "A")][1]
        visor_band = head_m & (np.abs(c.px[..., 1] - head[1]) <= 1.8) & ((c.px[..., 0] - head[0]) >= -r * 0.3)
        c.idx[visor_band] = RAMPS.get("accent", (15, 15, 15))[1]
        c.dot(head[0] + r * 0.5, head[1], 14)
    else:
        hair_cap = head_m & ((c.px[..., 1] - head[1]) < -r * 0.05) & ((c.px[..., 0] - head[0]) < r * 0.35)
        c.idx[hair_cap] = mid_h
        hair_back = head_m & ((c.px[..., 0] - head[0]) < -r * 0.25)
        c.idx[hair_back] = lo_h

        if hair_style == "spiky":
            for sp_dx, sp_dy in [(-r * 0.2, -r * 1.25), (-r * 0.6, -r * 1.1), (-r * 0.9, -r * 0.6), (-r * 0.9, -r * 0.1)]:
                c.line(head + np.array([-2, -4]), head + np.array([sp_dx, sp_dy]), hi_h)
                c.line(head + np.array([-1, -3]), head + np.array([sp_dx - 1, sp_dy + 1]), lo_h)
        elif hair_style == "topknot":
            knot_c = head + np.array([-r * 0.1, -r * 1.15])
            c.ellipse(knot_c, 2.5, 3.2, hair_mat)
            c.line(knot_c, knot_c + np.array([-3.0, 3.5]), lo_h)
        elif hair_style == "braids":
            b1 = head + np.array([-r * 0.6, r * 0.3])
            c.line(b1, b1 + np.array([-3.0, 8.0]), hi_h)
            b2 = head + np.array([-r * 0.3, r * 0.4])
            c.line(b2, b2 + np.array([-2.0, 9.0]), mid_h)

    if mat.get("band") and hair_style != "mongkhon":
        band_mat = mat["band"]
        lo_bd, mid_bd, hi_bd = RAMPS[band_mat]
        band_y = head[1] - r * 0.25
        band_m = head_m & (np.abs(c.px[..., 1] - band_y) <= 1.5) & ((c.px[..., 0] - head[0]) > -r * 0.8)
        c.idx[band_m] = mid_bd
        band_hi_m = head_m & (c.px[..., 1] >= band_y - 1.5) & (c.px[..., 1] < band_y - 0.5)
        c.idx[band_hi_m] = hi_bd
        tail_k = head + np.array([-r * 0.75, -r * 0.25])
        c.line(tail_k, tail_k + np.array([-6.0, 3.0]), hi_bd)
        c.line(tail_k + np.array([0, 1]), tail_k + np.array([-7.0, 5.0]), mid_bd)
        c.line(tail_k + np.array([0, 2]), tail_k + np.array([-6.0, 7.0]), lo_bd)
    elif hair_style == "mongkhon":
        circlet = head_m & (np.abs(c.px[..., 1] - (head[1] - r * 0.38)) <= 1.6)
        c.idx[circlet] = RAMPS[mat.get("band", "white")][1]
        m_tail = head + np.array([-r - 2.5, -r * 0.35])
        c.line(m_tail, m_tail + np.array([-6.0, 5.0]), RAMPS[mat.get("band", "white")][1])

    eye_x, eye_y = head[0] + r * 0.35, head[1] - r * 0.05
    c.dot(eye_x, eye_y, 14)
    c.dot(eye_x + 1, eye_y, 1)
    c.line(np.array([eye_x - 1, eye_y - 2]), np.array([eye_x + 2, eye_y - 2]), 1)
    c.dot(head[0] + r * 0.75, head[1] + r * 0.05, 3)
    c.dot(head[0] + r * 0.7, head[1] + r * 0.15, 1)
    c.line(np.array([head[0] + r * 0.35, head[1] + r * 0.45]), np.array([head[0] + r * 0.65, head[1] + r * 0.45]), 1)
    c.dot(head[0] + r * 0.5, head[1] + r * 0.55, 2)

    c.outline_mask(head_m)


def draw_torso(c, build, j, mat):
    hip, neck = j["hip"], j["neck"]
    up = j["up"]
    torso_m, torso_t = c.capsule_mask(hip, neck - up * 2.0, build["waist_r"] * 1.05, build["chest_r"] * 1.1)
    c.paint_smooth(torso_m, torso_t, mat["torso"])

    if mat.get("muscle") or mat.get("open_chest") or mat["torso"] == "skin":
        chest = hip + up * (build["torso"] * 0.65)
        c.dot(chest[0] + 3, chest[1] + 2, 2)
        c.dot(chest[0] + 4, chest[1] + 2, 2)
        c.dot(chest[0] + 2, chest[1] + 3, 2)
        c.dot(chest[0] + 5, chest[1] + 3, 2)
        for k in range(3):
            a = hip + up * (7 + k * 4.5)
            c.dot(a[0] + 3, a[1], 2)
            c.dot(a[0] + 4, a[1], 2)
            c.dot(a[0] + 5, a[1] + 0.5, 3)

    if mat.get("gi_lapel"):
        lapel_mat = mat["gi_lapel"]
        lo_l, mid_l, hi_l = RAMPS[lapel_mat]
        v_apex = hip + up * (build["torso"] * 0.42)
        sh_l = neck + np.array([-build["chest_r"] * 0.55, -1.0])
        sh_r = neck + np.array([build["chest_r"] * 0.55, -1.0])
        c.line(sh_l, v_apex + np.array([3.0, 3.0]), hi_l)
        c.line(sh_l + np.array([1, 0]), v_apex + np.array([4.0, 3.0]), mid_l)
        c.line(sh_r, v_apex - np.array([2.0, -2.0]), mid_l)
        c.line(sh_r + np.array([-1, 0]), v_apex - np.array([3.0, -2.0]), lo_l)
        c.dot(neck[0], neck[1] + 2, 3)
        c.dot(neck[0] + 1, neck[1] + 3, 3)
        c.dot(v_apex[0] - 1, v_apex[1] - 3, 2)
        skirt_m, skirt_t = c.capsule_mask(hip + np.array([0, 1]), hip + np.array([1, 10]), build["waist_r"] * 1.15, build["waist_r"] * 1.1)
        c.paint_smooth(skirt_m, skirt_t, mat["torso"])

    if mat.get("vest"):
        vest_m, vest_t = c.capsule_mask(hip + up * (build["torso"] * 0.5), neck - up * 1.5, build["waist_r"] * 1.08, build["chest_r"] * 1.15)
        c.paint_smooth(vest_m, vest_t, mat["vest"])

    if mat.get("armor"):
        cuirass_mat = mat["armor"]
        c_top = neck - up * 2.0
        c.capsule(c_top, hip + up * (build["torso"] * 0.4), build["chest_r"] * 1.06, build["waist_r"] * 1.03, cuirass_mat)
        core_pos = hip + up * (build["torso"] * 0.6)
        c.ellipse(core_pos, 2.4, 2.4, "accent")
        c.dot(core_pos[0], core_pos[1], 14)

    if mat.get("waistband"):
        belt_mat = mat["waistband"]
        lo_b, mid_b, hi_b = RAMPS[belt_mat]
        belt_l = hip + np.array([-build["waist_r"] * 1.1, -3])
        belt_r = hip + np.array([build["waist_r"] * 1.1, -1])
        belt_m, _ = c.capsule_mask(belt_l, belt_r, 2.6, 2.6)
        c.idx[belt_m] = mid_b
        c.line(belt_l + np.array([0, -1]), belt_r + np.array([0, -1]), hi_b)
        c.line(belt_l + np.array([0, 2]), belt_r + np.array([0, 2]), lo_b)
        knot = hip + np.array([2.0, -1.0])
        c.ellipse_mask(knot, 2.2, 2.0)
        c.dot(knot[0], knot[1], hi_b)
        c.dot(knot[0] + 1, knot[1], lo_b)
        c.line(knot, knot + np.array([2.0, 11.0]), mid_b)
        c.line(knot + np.array([1, 0]), knot + np.array([3.0, 11.0]), lo_b)
        c.dot(knot[0] + 2.0, knot[1] + 10.0, 15)
        c.dot(knot[0] + 2.0, knot[1] + 11.0, 15)

    if mat.get("abada_cord"):
        cord_mat = mat["abada_cord"]
        cord_hip = hip + np.array([build["waist_r"] * 0.6, 0])
        c.line(cord_hip, cord_hip + np.array([2.0, 10.0]), RAMPS[cord_mat][1])

    c.outline_mask(torso_m)


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
