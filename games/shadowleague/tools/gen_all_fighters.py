"""Generate the 37 new fighter definitions to complete the 44-fighter roster."""

import json
from pathlib import Path
from roster_builder import make_fighter

F_DIR = Path(__file__).resolve().parents[1] / "fighters"

# Helper for standard builds
def b_standard(scale=1.0, heaviness=1.0):
    return {
        "thigh": int(26 * scale), "shin": int(25 * scale), "ankle": 4, "foot": 10,
        "torso": int(31 * scale), "shoulder_drop": 4, "neck": 3, "head_r": 8.8,
        "upper_arm": int(17 * scale), "forearm": int(16 * scale), "depth": 5,
        "thigh_r": round(6.8 * heaviness, 1), "knee_r": round(4.8 * heaviness, 1),
        "ankle_r": round(3.4 * heaviness, 1), "toe_r": 2.5, "arm_r": round(4.4 * heaviness, 1),
        "elbow_r": round(3.6 * heaviness, 1), "wrist_r": round(2.8 * heaviness, 1),
        "fist_r": round(4.0 * heaviness, 1), "waist_r": round(8.5 * heaviness, 1),
        "chest_r": round(11.0 * heaviness, 1), "neck_r": round(3.5 * heaviness, 1)
    }

ROSTER_DATA = [
    # 02 Somchai Rattan (Thailand, Muay Thai)
    {
        "fid": "somchai", "name": "SOMCHAI RATTAN", "origin": "THAILAND",
        "style": "MUAY THAI", "discipline": "MUAY THAI", "stage": "BANGKOK NIGHT STADIUM",
        "life": 1000, "physics": {"walk_fwd": 2.2, "walk_back": 1.7, "dash": 5.2, "backdash": [4.0, 4.2], "jump": [3.2, 10.5], "gravity": 0.54},
        "push": [16, 90, 58], "close_range": 46, "guard_max": 100,
        "build": b_standard(scale=1.0, heaviness=0.98),
        "materials": {"thigh": "A", "shin": "A", "foot": "skin", "torso": "skin", "upper_arm": "skin", "forearm": "skin", "fist": "skin",
                      "trunks": 0.65, "shorts": "A", "waistband": "C", "ankle_wrap": "B", "hair": "C", "hair_style": "mongkhon", "band": "B", "muscle": True},
        "palette": {"skin": [[136, 82, 52], [196, 130, 88], [236, 178, 134]],
                    "A": [[16, 24, 76], [32, 60, 160], [80, 120, 230]],
                    "B": [[150, 20, 24], [220, 40, 44], [254, 120, 110]],
                    "C": [[180, 140, 20], [236, 196, 40], [254, 238, 110]]},
        "alt_palette": {"A": [[130, 18, 24], [200, 36, 42], [248, 96, 90]], "B": [[20, 70, 140], [40, 120, 220], [100, 180, 254]]},
        "specials": [
            {"name": "TIGER ELBOW", "motion": "qcf", "pose_prep": {"base": "stance", "torso": 16, "far_fist": [18, 4]},
             "pose_hit": {"base": "stance", "torso": 24, "near_elbow": [34, -2], "near_fist": [22, 6]}, "limb": "near_fist", "dx": 28},
            {"name": "RISING KNEE", "motion": "dp", "pose_prep": {"base": "crouch", "torso": 10, "near_knee": [16, 10]},
             "pose_hit": {"base": "jump", "torso": -8, "near_knee": [28, -28], "near_foot": [18, 0]}, "limb": "near_ankle", "dx": 18, "dy": 30, "anim_flags": "active|airborne"},
            {"name": "MEKONG SWEEP", "motion": "qcb", "pose_prep": {"base": "crouch", "torso": 32},
             "pose_hit": {"base": "crouch", "torso": 48, "far_foot": [48, -2]}, "limb": "far_ankle", "dx": 14}
        ],
        "super_move": {"name": "SIAM BARRAGE", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 0, "near_fist": [10, 10]},
                       "pose_hit": {"base": "stance", "torso": 28, "near_knee": [34, -20]}, "limb": "near_ankle", "dx": 32},
        "cpu": {"aggression": 80, "range": 50, "jump": 25, "poke": "JAB", "sweep": "SWEEP", "anti_air": "RISING KNEE", "special": "TIGER ELBOW", "rush": "TIGER ELBOW", "throw": "THROW", "super": "SIAM BARRAGE"}
    },

    # 05 Claire Moreau (France, Savate)
    {
        "fid": "claire", "name": "CLAIRE MOREAU", "origin": "FRANCE",
        "style": "SAVATE", "discipline": "SAVATE", "stage": "HISTORIC MARSEILLE HALL",
        "life": 960, "physics": {"walk_fwd": 2.3, "walk_back": 1.8, "dash": 5.3, "backdash": [4.2, 4.3], "jump": [3.4, 10.6], "gravity": 0.53},
        "push": [15, 88, 56], "close_range": 48, "guard_max": 95,
        "build": b_standard(scale=1.02, heaviness=0.92),
        "materials": {"thigh": "A", "shin": "A", "foot": "B", "torso": "A", "upper_arm": "A", "forearm": "skin", "fist": "B",
                      "shin_split": 0.55, "boot": "B", "vest": "C", "hair": "C", "hair_style": "default"},
        "palette": {"skin": [[152, 94, 68], [212, 148, 114], [246, 198, 162]],
                    "A": [[20, 28, 52], [42, 56, 102], [80, 104, 172]],
                    "B": [[130, 20, 26], [198, 38, 44], [242, 88, 88]],
                    "C": [[210, 206, 200], [236, 234, 230], [254, 254, 254]]},
        "alt_palette": {"A": [[120, 22, 28], [180, 40, 48], [230, 84, 88]], "B": [[24, 32, 60], [48, 64, 114], [90, 118, 190]]},
        "specials": [
            {"name": "FOUETTE CHASSE", "motion": "qcf", "pose_prep": {"base": "stance", "torso": -10, "far_foot": [20, -10]},
             "pose_hit": {"base": "stance", "torso": -24, "far_foot": [46, -44]}, "limb": "far_ankle", "dx": 24},
            {"name": "REVERS SPIRALE", "motion": "dp", "pose_prep": {"base": "crouch", "torso": 16},
             "pose_hit": {"base": "jump", "torso": 4, "near_foot": [32, -52]}, "limb": "near_ankle", "dx": 20, "dy": 28, "anim_flags": "active|airborne"},
            {"name": "PIETINEMENT", "motion": "qcb", "pose_prep": {"base": "jump", "torso": 10},
             "pose_hit": {"base": "jump", "torso": 20, "far_foot": [26, 0]}, "limb": "far_ankle", "dx": 18}
        ],
        "super_move": {"name": "BALLET D'ACIER", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": -14},
                       "pose_hit": {"base": "stance", "torso": -26, "far_foot": [50, -50]}, "limb": "far_ankle", "dx": 34},
        "cpu": {"aggression": 72, "range": 68, "jump": 30, "poke": "FOUETTE CHASSE", "sweep": "SWEEP", "anti_air": "REVERS SPIRALE", "special": "FOUETTE CHASSE", "rush": "PIETINEMENT", "throw": "THROW", "super": "BALLET D'ACIER"}
    },

    # 06 Viktor Orlov (Ukraine, Combat Sambo)
    {
        "fid": "viktor", "name": "VIKTOR ORLOV", "origin": "UKRAINE",
        "style": "COMBAT SAMBO", "discipline": "SAMBO", "stage": "INDUSTRIAL TRAINING WAREHOUSE",
        "life": 1040, "physics": {"walk_fwd": 2.0, "walk_back": 1.5, "dash": 4.8, "backdash": [3.8, 4.0], "jump": [3.0, 10.2], "gravity": 0.58},
        "push": [17, 92, 64], "close_range": 48, "guard_max": 110,
        "build": b_standard(scale=1.04, heaviness=1.12),
        "materials": {"thigh": "A", "shin": "A", "foot": "C", "torso": "A", "upper_arm": "A", "forearm": "skin", "fist": "C",
                      "trunks": 0.7, "shorts": "B", "waistband": "C", "shin_split": 0.5, "boot": "C", "gi_lapel": "A", "hair": "C", "hair_style": "default"},
        "palette": {"skin": [[150, 94, 66], [208, 146, 108], [242, 194, 156]],
                    "A": [[20, 36, 96], [38, 70, 172], [76, 120, 240]],
                    "B": [[140, 22, 28], [206, 42, 46], [248, 96, 92]],
                    "C": [[24, 24, 32], [50, 50, 64], [90, 90, 110]]},
        "alt_palette": {"A": [[140, 22, 28], [206, 42, 46], [248, 96, 92]], "B": [[20, 36, 96], [38, 70, 172], [76, 120, 240]]},
        "specials": [
            {"name": "TACTICAL TACKLE", "motion": "qcf", "pose_prep": {"base": "crouch", "torso": 36, "hip": [4, 18]},
             "pose_hit": {"base": "crouch", "torso": 52, "near_fist": [36, 14]}, "limb": "near_fist", "dx": 32},
            {"name": "LEG REAP", "motion": "qcb", "pose_prep": {"base": "stance", "torso": 10},
             "pose_hit": {"base": "crouch", "torso": 40, "far_foot": [44, -2]}, "limb": "far_ankle", "dx": 20},
            {"name": "IRON COUNTER", "motion": "dp", "pose_prep": {"base": "guard", "torso": -4},
             "pose_hit": {"base": "stance", "torso": 32, "near_fist": [30, 4]}, "limb": "near_fist", "dx": 16}
        ],
        "super_move": {"name": "COMMANDO SLAM", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 20, "near_fist": [22, 12]},
                       "pose_hit": {"base": "stance", "torso": -40, "near_fist": [18, -30]}, "limb": "near_fist", "dx": 28},
        "cpu": {"aggression": 75, "range": 45, "jump": 15, "poke": "STRAIGHT", "sweep": "LEG REAP", "anti_air": "IRON COUNTER", "special": "TACTICAL TACKLE", "rush": "TACTICAL TACKLE", "throw": "THROW", "super": "COMMANDO SLAM"}
    },

    # 07 Lin Wei (China, Sanda)
    {
        "fid": "linwei", "name": "LIN WEI", "origin": "CHINA",
        "style": "SANDA", "discipline": "SANDA", "stage": "SHANGHAI ROOFTOP ARENA",
        "life": 990, "physics": {"walk_fwd": 2.2, "walk_back": 1.7, "dash": 5.1, "backdash": [4.0, 4.2], "jump": [3.3, 10.4], "gravity": 0.54},
        "push": [16, 90, 58], "close_range": 46, "guard_max": 100,
        "build": b_standard(scale=1.0, heaviness=0.98),
        "materials": {"thigh": "A", "shin": "A", "foot": "skin", "torso": "B", "upper_arm": "B", "forearm": "skin", "fist": "C",
                      "trunks": 0.65, "shorts": "A", "waistband": "C", "wrist_wrap": "C", "hair": "C", "hair_style": "spiky"},
        "palette": {"skin": [[154, 96, 68], [214, 150, 114], [246, 198, 160]],
                    "A": [[150, 20, 24], [216, 40, 44], [254, 96, 92]],
                    "B": [[22, 22, 28], [46, 46, 56], [84, 84, 102]],
                    "C": [[180, 140, 20], [236, 196, 40], [254, 238, 110]]},
        "alt_palette": {"A": [[20, 36, 96], [40, 72, 172], [80, 124, 240]], "B": [[140, 22, 28], [206, 42, 46], [248, 96, 92]]},
        "specials": [
            {"name": "LIGHTNING JAB-KICK", "motion": "qcf", "pose_prep": {"base": "stance", "torso": 14, "near_fist": [28, 0]},
             "pose_hit": {"base": "stance", "torso": -16, "far_foot": [42, -36]}, "limb": "far_ankle", "dx": 26},
            {"name": "DRAGON CATCH", "motion": "dp", "pose_prep": {"base": "stance", "torso": 8},
             "pose_hit": {"base": "jump", "torso": 16, "near_fist": [26, -24]}, "limb": "near_fist", "dx": 18, "dy": 26, "anim_flags": "active|airborne"},
            {"name": "SWEEPING SHADOW", "motion": "qcb", "pose_prep": {"base": "crouch", "torso": 28},
             "pose_hit": {"base": "crouch", "torso": 50, "far_foot": [46, -2]}, "limb": "far_ankle", "dx": 18}
        ],
        "super_move": {"name": "SANDA TYPHOON", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 10},
                       "pose_hit": {"base": "stance", "torso": -20, "far_foot": [48, -46]}, "limb": "far_ankle", "dx": 32},
        "cpu": {"aggression": 78, "range": 55, "jump": 26, "poke": "LIGHTNING JAB-KICK", "sweep": "SWEEPING SHADOW", "anti_air": "DRAGON CATCH", "special": "LIGHTNING JAB-KICK", "rush": "LIGHTNING JAB-KICK", "throw": "THROW", "super": "SANDA TYPHOON"}
    },

    # 10 Malik Kane (Senegal, Laamb wrestling)
    {
        "fid": "malik", "name": "MALIK KANE", "origin": "SENEGAL",
        "style": "LAAMB WRESTLING", "discipline": "LAAMB", "stage": "SAND WRESTLING ARENA",
        "life": 1080, "physics": {"walk_fwd": 1.9, "walk_back": 1.4, "dash": 4.6, "backdash": [3.6, 3.8], "jump": [2.8, 9.8], "gravity": 0.60},
        "push": [18, 94, 68], "close_range": 52, "guard_max": 115,
        "build": b_standard(scale=1.08, heaviness=1.20),
        "materials": {"thigh": "A", "shin": "A", "foot": "skin", "torso": "skin", "upper_arm": "skin", "forearm": "skin", "fist": "skin",
                      "trunks": 0.5, "shorts": "A", "waistband": "B", "hair": "C", "hair_style": "braids", "muscle": True},
        "palette": {"skin": [[90, 54, 36], [142, 90, 62], [188, 126, 92]],
                    "A": [[150, 110, 20], [216, 168, 36], [250, 218, 96]],
                    "B": [[150, 24, 28], [216, 44, 46], [254, 96, 92]],
                    "C": [[18, 18, 22], [36, 36, 44], [64, 64, 78]]},
        "alt_palette": {"A": [[24, 90, 52], [48, 150, 92], [112, 216, 156]], "B": [[150, 110, 20], [216, 168, 36], [250, 218, 96]]},
        "specials": [
            {"name": "SAHARA CHARGE", "motion": "qcf", "pose_prep": {"base": "crouch", "torso": 38, "hip": [4, 16]},
             "pose_hit": {"base": "stance", "torso": 44, "near_fist": [38, 8]}, "limb": "near_fist", "dx": 36},
            {"name": "THUNDER PALM", "motion": "dp", "pose_prep": {"base": "crouch", "torso": 10},
             "pose_hit": {"base": "stance", "torso": -18, "near_fist": [28, -26], "far_fist": [22, -24]}, "limb": "near_fist", "dx": 18, "dy": 20},
            {"name": "LION SLAM", "motion": "qcb", "pose_prep": {"base": "stance", "torso": 16, "near_fist": [22, 10]},
             "pose_hit": {"base": "stance", "torso": -50, "near_fist": [16, -26]}, "limb": "near_fist", "dx": 16}
        ],
        "super_move": {"name": "DAKAR AVALANCHE", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 20},
                       "pose_hit": {"base": "stance", "torso": 54, "near_fist": [42, 12]}, "limb": "near_fist", "dx": 36},
        "cpu": {"aggression": 82, "range": 42, "jump": 12, "poke": "STRAIGHT", "sweep": "SWEEP", "anti_air": "THUNDER PALM", "special": "SAHARA CHARGE", "rush": "SAHARA CHARGE", "throw": "LION SLAM", "super": "DAKAR AVALANCHE"}
    },

    # 11 Idris Zahra (Morocco, Kickboxing)
    {
        "fid": "idris", "name": "IDRIS ZAHRA", "origin": "MOROCCO",
        "style": "KICKBOXING", "discipline": "KICKBOXING", "stage": "CASABLANCA ROOFTOP CLUB",
        "life": 990, "physics": {"walk_fwd": 2.2, "walk_back": 1.7, "dash": 5.2, "backdash": [4.1, 4.2], "jump": [3.3, 10.4], "gravity": 0.54},
        "push": [16, 90, 58], "close_range": 46, "guard_max": 100,
        "build": b_standard(scale=1.0, heaviness=0.98),
        "materials": {"thigh": "A", "shin": "A", "foot": "skin", "torso": "skin", "upper_arm": "skin", "forearm": "skin", "fist": "B",
                      "trunks": 0.65, "shorts": "A", "waistband": "C", "ankle_wrap": "C", "hair": "C", "hair_style": "spiky", "muscle": True},
        "palette": {"skin": [[146, 92, 64], [206, 142, 104], [242, 192, 154]],
                    "A": [[130, 20, 26], [198, 38, 44], [244, 88, 88]],
                    "B": [[24, 24, 30], [50, 50, 62], [90, 90, 110]],
                    "C": [[24, 88, 52], [48, 148, 92], [112, 214, 156]]},
        "alt_palette": {"A": [[24, 88, 52], [48, 148, 92], [112, 214, 156]], "B": [[130, 20, 26], [198, 38, 44], [244, 88, 88]]},
        "specials": [
            {"name": "ATLAS HOOK", "motion": "qcf", "pose_prep": {"base": "stance", "torso": 14, "near_fist": [18, 10]},
             "pose_hit": {"base": "stance", "torso": 24, "near_fist": [36, 2]}, "limb": "near_fist", "dx": 24},
            {"name": "DESERT CRESCENT", "motion": "dp", "pose_prep": {"base": "crouch", "torso": 10},
             "pose_hit": {"base": "jump", "torso": -14, "near_foot": [34, -48]}, "limb": "near_ankle", "dx": 22, "dy": 26, "anim_flags": "active|airborne"},
            {"name": "STEPPING KNEE", "motion": "qcb", "pose_prep": {"base": "stance", "torso": 8},
             "pose_hit": {"base": "stance", "torso": 22, "near_knee": [32, -18]}, "limb": "near_ankle", "dx": 26}
        ],
        "super_move": {"name": "CASABLANCA FURY", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 12},
                       "pose_hit": {"base": "stance", "torso": 28, "near_fist": [38, 0]}, "limb": "near_fist", "dx": 34},
        "cpu": {"aggression": 84, "range": 52, "jump": 24, "poke": "ATLAS HOOK", "sweep": "SWEEP", "anti_air": "DESERT CRESCENT", "special": "ATLAS HOOK", "rush": "STEPPING KNEE", "throw": "THROW", "super": "CASABLANCA FURY"}
    },

    # 12 Arun Dev (India, Kalaripayattu)
    {
        "fid": "arun", "name": "ARUN DEV", "origin": "INDIA",
        "style": "KALARIPAYATTU", "discipline": "KALARIPAYATTU", "stage": "SOUTHERN TEMPLE COURTYARD",
        "life": 970, "physics": {"walk_fwd": 2.3, "walk_back": 1.8, "dash": 5.4, "backdash": [4.3, 4.3], "jump": [3.4, 10.5], "gravity": 0.52},
        "push": [15, 88, 56], "close_range": 46, "guard_max": 95,
        "build": b_standard(scale=0.98, heaviness=0.94),
        "materials": {"thigh": "A", "shin": "A", "foot": "skin", "torso": "skin", "upper_arm": "skin", "forearm": "skin", "fist": "skin",
                      "trunks": 0.6, "shorts": "A", "waistband": "B", "hair": "C", "hair_style": "topknot", "muscle": True},
        "palette": {"skin": [[128, 74, 46], [186, 120, 80], [228, 168, 124]],
                    "A": [[160, 90, 16], [224, 138, 30], [254, 196, 88]],
                    "B": [[140, 20, 24], [210, 40, 44], [252, 94, 90]],
                    "C": [[20, 18, 24], [40, 38, 50], [74, 72, 92]]},
        "alt_palette": {"A": [[24, 76, 140], [48, 128, 214], [108, 186, 252]], "B": [[160, 90, 16], [224, 138, 30], [254, 196, 88]]},
        "specials": [
            {"name": "SERPENT STRIKE", "motion": "qcf", "pose_prep": {"base": "crouch", "torso": 28},
             "pose_hit": {"base": "crouch", "torso": 48, "near_fist": [40, 12]}, "limb": "near_fist", "dx": 34},
            {"name": "TEMPLE LEAP", "motion": "dp", "pose_prep": {"base": "crouch", "torso": 14},
             "pose_hit": {"base": "jump", "torso": -20, "far_foot": [36, -50]}, "limb": "far_ankle", "dx": 24, "dy": 30, "anim_flags": "active|airborne"},
            {"name": "COIL SWEEP", "motion": "qcb", "pose_prep": {"base": "crouch", "torso": 36},
             "pose_hit": {"base": "crouch", "torso": 54, "near_foot": [46, -2]}, "limb": "near_ankle", "dx": 18}
        ],
        "super_move": {"name": "KERALA DANCE", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": -10},
                       "pose_hit": {"base": "stance", "torso": 32, "near_fist": [38, 4]}, "limb": "near_fist", "dx": 34},
        "cpu": {"aggression": 76, "range": 58, "jump": 28, "poke": "SERPENT STRIKE", "sweep": "COIL SWEEP", "anti_air": "TEMPLE LEAP", "special": "SERPENT STRIKE", "rush": "SERPENT STRIKE", "throw": "THROW", "super": "KERALA DANCE"}
    },

    # 13 Ryuhei Genda (Japan, Judo)
    {
        "fid": "ryuhei", "name": "RYUHEI GENDA", "origin": "JAPAN",
        "style": "JUDO", "discipline": "JUDO", "stage": "MUNICIPAL DOJO",
        "life": 1050, "physics": {"walk_fwd": 1.9, "walk_back": 1.5, "dash": 4.6, "backdash": [3.6, 3.8], "jump": [2.8, 9.8], "gravity": 0.59},
        "push": [17, 92, 64], "close_range": 52, "guard_max": 115,
        "build": b_standard(scale=1.03, heaviness=1.12),
        "materials": {"thigh": "A", "shin": "A", "foot": "skin", "torso": "A", "upper_arm": "A", "forearm": "skin", "fist": "skin",
                      "gi_lapel": "A", "belt": "C", "waistband": "C", "sleeve": 0.85, "hair": "C", "hair_style": "default"},
        "palette": {"skin": [[150, 92, 62], [208, 144, 104], [242, 194, 152]],
                    "A": [[220, 218, 212], [240, 238, 234], [254, 254, 254]],
                    "B": [[24, 24, 30], [48, 48, 60], [86, 86, 104]],
                    "C": [[18, 18, 22], [34, 34, 42], [62, 62, 76]]},
        "alt_palette": {"A": [[20, 36, 96], [38, 70, 172], [76, 120, 240]], "B": [[220, 218, 212], [240, 238, 234], [254, 254, 254]]},
        "specials": [
            {"name": "SEOI NAGE", "motion": "qcf", "pose_prep": {"base": "stance", "torso": 20, "near_fist": [22, 10]},
             "pose_hit": {"base": "stance", "torso": 50, "near_fist": [36, 26]}, "limb": "near_fist", "dx": 22},
            {"name": "OSOTO GARI", "motion": "qcb", "pose_prep": {"base": "stance", "torso": 12},
             "pose_hit": {"base": "stance", "torso": 26, "far_foot": [42, -10]}, "limb": "far_ankle", "dx": 20},
            {"name": "DEFENSIVE COUNTER", "motion": "dp", "pose_prep": {"base": "guard", "torso": -4},
             "pose_hit": {"base": "stance", "torso": -34, "near_fist": [24, -20]}, "limb": "near_fist", "dx": 16}
        ],
        "super_move": {"name": "MASTER JUDO SLAM", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 18},
                       "pose_hit": {"base": "stance", "torso": -52, "near_fist": [20, -32]}, "limb": "near_fist", "dx": 26},
        "cpu": {"aggression": 68, "range": 44, "jump": 10, "poke": "STRAIGHT", "sweep": "OSOTO GARI", "anti_air": "DEFENSIVE COUNTER", "special": "SEOI NAGE", "rush": "SEOI NAGE", "throw": "SEOI NAGE", "super": "MASTER JUDO SLAM"}
    },

    # 14 Diego Cruz (Mexico, Lucha Libre)
    {
        "fid": "diego", "name": "DIEGO CRUZ", "origin": "MEXICO",
        "style": "LUCHA LIBRE", "discipline": "LUCHA LIBRE", "stage": "MEXICO CITY ARENA",
        "life": 990, "physics": {"walk_fwd": 2.3, "walk_back": 1.8, "dash": 5.4, "backdash": [4.3, 4.3], "jump": [3.5, 10.8], "gravity": 0.52},
        "push": [16, 90, 58], "close_range": 48, "guard_max": 95,
        "build": b_standard(scale=1.0, heaviness=0.98),
        "materials": {"thigh": "A", "shin": "A", "foot": "B", "torso": "skin", "upper_arm": "skin", "forearm": "skin", "fist": "B",
                      "trunks": 0.65, "shorts": "A", "waistband": "C", "shin_split": 0.5, "boot": "B", "hair": "C", "hair_style": "visor", "muscle": True},
        "palette": {"skin": [[148, 92, 64], [208, 144, 104], [244, 194, 154]],
                    "A": [[20, 88, 52], [44, 150, 92], [108, 216, 156]],
                    "B": [[140, 20, 24], [210, 40, 44], [252, 94, 90]],
                    "C": [[180, 140, 20], [236, 196, 40], [254, 238, 110]]},
        "alt_palette": {"A": [[140, 20, 24], [210, 40, 44], [252, 94, 90]], "B": [[20, 88, 52], [44, 150, 92], [108, 216, 156]]},
        "specials": [
            {"name": "HURRICANRANA", "motion": "qcf", "pose_prep": {"base": "stance", "torso": 8},
             "pose_hit": {"base": "jump", "torso": -60, "near_foot": [28, -60]}, "limb": "near_ankle", "dx": 34, "dy": 24, "anim_flags": "active|airborne"},
            {"name": "TORPEDO DIVE", "motion": "qcb", "pose_prep": {"base": "crouch", "torso": 24},
             "pose_hit": {"base": "jump", "torso": 80, "near_fist": [44, 14]}, "limb": "near_fist", "dx": 40, "dy": 10, "anim_flags": "active|airborne"},
            {"name": "MEXICAN TORNADO", "motion": "dp", "pose_prep": {"base": "crouch", "torso": 10},
             "pose_hit": {"base": "jump", "torso": -10, "near_fist": [20, -32]}, "limb": "near_fist", "dx": 20, "dy": 30, "anim_flags": "active|airborne"}
        ],
        "super_move": {"name": "AZTEC ECLIPSE", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": -10},
                       "pose_hit": {"base": "jump", "torso": 88, "near_fist": [46, 16]}, "limb": "near_fist", "dx": 42},
        "cpu": {"aggression": 85, "range": 52, "jump": 38, "poke": "TORPEDO DIVE", "sweep": "SWEEP", "anti_air": "MEXICAN TORNADO", "special": "HURRICANRANA", "rush": "TORPEDO DIVE", "throw": "THROW", "super": "AZTEC ECLIPSE"}
    },

    # 16 Chen Bao (China, Bajiquan)
    {
        "fid": "chenbao", "name": "CHEN BAO", "origin": "CHINA",
        "style": "BAJIQUAN", "discipline": "BAJIQUAN", "stage": "NORTHERN STONE COURTYARD",
        "life": 1020, "physics": {"walk_fwd": 2.1, "walk_back": 1.6, "dash": 5.0, "backdash": [3.9, 4.0], "jump": [3.1, 10.3], "gravity": 0.56},
        "push": [17, 91, 62], "close_range": 46, "guard_max": 105,
        "build": b_standard(scale=1.02, heaviness=1.06),
        "materials": {"thigh": "A", "shin": "A", "foot": "C", "torso": "B", "upper_arm": "B", "forearm": "skin", "fist": "skin",
                      "gi_lapel": "B", "belt": "C", "waistband": "C", "sleeve": 0.75, "hair": "C", "hair_style": "default"},
        "palette": {"skin": [[154, 96, 68], [214, 150, 114], [246, 198, 160]],
                    "A": [[24, 24, 30], [48, 48, 60], [86, 86, 104]],
                    "B": [[130, 24, 28], [196, 42, 46], [244, 92, 90]],
                    "C": [[180, 140, 20], [236, 196, 40], [254, 238, 110]]},
        "alt_palette": {"A": [[24, 24, 30], [48, 48, 60], [86, 86, 104]], "B": [[20, 36, 96], [40, 72, 172], [80, 124, 240]]},
        "specials": [
            {"name": "IRON SHOULDER", "motion": "qcf", "pose_prep": {"base": "stance", "torso": 24, "hip": [4, 6]},
             "pose_hit": {"base": "stance", "torso": 40, "near_fist": [34, 4]}, "limb": "near_fist", "dx": 32},
            {"name": "HEAVEN ELBOW", "motion": "dp", "pose_prep": {"base": "crouch", "torso": 12},
             "pose_hit": {"base": "stance", "torso": -16, "near_elbow": [26, -26]}, "limb": "near_fist", "dx": 20, "dy": 22},
            {"name": "GROUND RUPTURE", "motion": "qcb", "pose_prep": {"base": "stance", "torso": 8},
             "pose_hit": {"base": "crouch", "torso": 48, "near_fist": [30, 22]}, "limb": "near_fist", "dx": 18}
        ],
        "super_move": {"name": "BAJI EXPLOSION", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 16},
                       "pose_hit": {"base": "stance", "torso": 44, "near_fist": [40, 6]}, "limb": "near_fist", "dx": 36},
        "cpu": {"aggression": 80, "range": 46, "jump": 16, "poke": "IRON SHOULDER", "sweep": "SWEEP", "anti_air": "HEAVEN ELBOW", "special": "IRON SHOULDER", "rush": "IRON SHOULDER", "throw": "THROW", "super": "BAJI EXPLOSION"}
    },

    # 17 Aisha Diallo (Guinea, Dambe)
    {
        "fid": "aisha", "name": "AISHA DIALLO", "origin": "GUINEA",
        "style": "DAMBE", "discipline": "DAMBE", "stage": "WEST AFRICAN FESTIVAL ARENA",
        "life": 1030, "physics": {"walk_fwd": 2.1, "walk_back": 1.6, "dash": 5.0, "backdash": [3.9, 4.0], "jump": [3.1, 10.3], "gravity": 0.56},
        "push": [17, 91, 62], "close_range": 46, "guard_max": 105,
        "build": b_standard(scale=1.02, heaviness=1.04),
        "materials": {"thigh": "A", "shin": "A", "foot": "skin", "torso": "B", "upper_arm": "skin", "forearm": "skin", "fist": "C",
                      "trunks": 0.65, "shorts": "A", "waistband": "C", "wrist_wrap": "C", "hair": "C", "hair_style": "braids", "muscle": True},
        "palette": {"skin": [[96, 58, 38], [150, 96, 66], [196, 134, 98]],
                    "A": [[160, 96, 18], [224, 144, 32], [254, 202, 92]],
                    "B": [[140, 22, 26], [208, 42, 46], [250, 94, 92]],
                    "C": [[200, 180, 140], [230, 214, 180], [254, 246, 220]]},
        "alt_palette": {"A": [[24, 88, 52], [48, 148, 92], [112, 214, 156]], "B": [[160, 96, 18], [224, 144, 32], [254, 202, 92]]},
        "specials": [
            {"name": "THE SPEAR", "motion": "qcf", "pose_prep": {"base": "stance", "torso": 16, "near_fist": [16, 6]},
             "pose_hit": {"base": "stance", "torso": 26, "near_fist": [42, 0]}, "limb": "near_fist", "dx": 30},
            {"name": "LEAD HOOK", "motion": "qcb", "pose_prep": {"base": "stance", "torso": 10},
             "pose_hit": {"base": "stance", "torso": 24, "far_fist": [36, -4]}, "limb": "far_fist", "dx": 20},
            {"name": "WARRIOR STEP", "motion": "dp", "pose_prep": {"base": "crouch", "torso": 16},
             "pose_hit": {"base": "stance", "torso": 38, "near_fist": [34, 2]}, "limb": "near_fist", "dx": 28}
        ],
        "super_move": {"name": "DAMBE CRUSH", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 14},
                       "pose_hit": {"base": "stance", "torso": 36, "near_fist": [44, 2]}, "limb": "near_fist", "dx": 36},
        "cpu": {"aggression": 84, "range": 50, "jump": 18, "poke": "THE SPEAR", "sweep": "SWEEP", "anti_air": "WARRIOR STEP", "special": "THE SPEAR", "rush": "LEAD HOOK", "throw": "THROW", "super": "DAMBE CRUSH"}
    },

    # 18 Kiran Shah (UK, Modern Kickboxing)
    {
        "fid": "kiran", "name": "KIRAN SHAH", "origin": "UNITED KINGDOM",
        "style": "KICKBOXING", "discipline": "KICKBOXING", "stage": "LONDON FIGHT CLUB",
        "life": 990, "physics": {"walk_fwd": 2.2, "walk_back": 1.7, "dash": 5.2, "backdash": [4.1, 4.2], "jump": [3.3, 10.4], "gravity": 0.54},
        "push": [16, 90, 58], "close_range": 46, "guard_max": 100,
        "build": b_standard(scale=1.0, heaviness=0.98),
        "materials": {"thigh": "A", "shin": "A", "foot": "skin", "torso": "skin", "upper_arm": "skin", "forearm": "skin", "fist": "B",
                      "trunks": 0.65, "shorts": "A", "waistband": "B", "ankle_wrap": "B", "hair": "C", "hair_style": "spiky", "muscle": True},
        "palette": {"skin": [[140, 84, 54], [198, 132, 92], [238, 180, 136]],
                    "A": [[22, 34, 88], [44, 68, 160], [86, 118, 230]],
                    "B": [[140, 22, 28], [206, 42, 46], [248, 96, 92]],
                    "C": [[18, 18, 24], [36, 36, 48], [68, 68, 86]]},
        "alt_palette": {"A": [[140, 22, 28], [206, 42, 46], [248, 96, 92]], "B": [[22, 34, 88], [44, 68, 160], [86, 118, 230]]},
        "specials": [
            {"name": "CROSS-KNEE FLOW", "motion": "qcf", "pose_prep": {"base": "stance", "torso": 16, "near_fist": [24, 0]},
             "pose_hit": {"base": "stance", "torso": 24, "far_knee": [32, -18]}, "limb": "far_ankle", "dx": 28},
            {"name": "LONDON HIGH", "motion": "dp", "pose_prep": {"base": "crouch", "torso": 10},
             "pose_hit": {"base": "jump", "torso": -16, "near_foot": [32, -50]}, "limb": "near_ankle", "dx": 22, "dy": 26, "anim_flags": "active|airborne"},
            {"name": "PIVOT LOW", "motion": "qcb", "pose_prep": {"base": "stance", "torso": 8},
             "pose_hit": {"base": "crouch", "torso": 44, "far_foot": [42, -2]}, "limb": "far_ankle", "dx": 18}
        ],
        "super_move": {"name": "QUEEN'S KNOCKOUT", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 14},
                       "pose_hit": {"base": "stance", "torso": 28, "near_fist": [40, 0]}, "limb": "near_fist", "dx": 34},
        "cpu": {"aggression": 80, "range": 54, "jump": 25, "poke": "CROSS-KNEE FLOW", "sweep": "PIVOT LOW", "anti_air": "LONDON HIGH", "special": "CROSS-KNEE FLOW", "rush": "CROSS-KNEE FLOW", "throw": "THROW", "super": "QUEEN'S KNOCKOUT"}
    },

    # 19 Elena Petrova (Bulgaria, Sport Wrestling)
    {
        "fid": "elena", "name": "ELENA PETROVA", "origin": "BULGARIA",
        "style": "SPORT WRESTLING", "discipline": "WRESTLING", "stage": "NATIONAL TRAINING CENTER",
        "life": 1010, "physics": {"walk_fwd": 2.2, "walk_back": 1.7, "dash": 5.2, "backdash": [4.1, 4.2], "jump": [3.2, 10.4], "gravity": 0.55},
        "push": [16, 90, 58], "close_range": 48, "guard_max": 100,
        "build": b_standard(scale=1.0, heaviness=1.02),
        "materials": {"thigh": "A", "shin": "A", "foot": "C", "torso": "A", "upper_arm": "skin", "forearm": "skin", "fist": "skin",
                      "trunks": 0.65, "shorts": "A", "waistband": "B", "shin_split": 0.55, "boot": "C", "hair": "C", "hair_style": "topknot"},
        "palette": {"skin": [[152, 94, 68], [212, 148, 114], [246, 198, 162]],
                    "A": [[20, 88, 52], [44, 150, 92], [108, 216, 156]],
                    "B": [[140, 20, 24], [210, 40, 44], [252, 94, 90]],
                    "C": [[24, 24, 30], [50, 50, 62], [90, 90, 110]]},
        "alt_palette": {"A": [[140, 20, 24], [210, 40, 44], [252, 94, 90]], "B": [[20, 88, 52], [44, 150, 92], [108, 216, 156]]},
        "specials": [
            {"name": "DOUBLE LEG BLAST", "motion": "qcf", "pose_prep": {"base": "crouch", "torso": 34, "hip": [4, 18]},
             "pose_hit": {"base": "crouch", "torso": 52, "near_fist": [36, 16]}, "limb": "near_fist", "dx": 34},
            {"name": "SUPLEX RUSH", "motion": "qcb", "pose_prep": {"base": "stance", "torso": 16, "near_fist": [22, 10]},
             "pose_hit": {"base": "stance", "torso": -44, "near_fist": [18, -28]}, "limb": "near_fist", "dx": 20},
            {"name": "ANKLE PICK", "motion": "dp", "pose_prep": {"base": "crouch", "torso": 24},
             "pose_hit": {"base": "crouch", "torso": 56, "near_fist": [34, 28]}, "limb": "near_fist", "dx": 26}
        ],
        "super_move": {"name": "GOLD MEDAL COMBO", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 18},
                       "pose_hit": {"base": "stance", "torso": -48, "near_fist": [16, -30]}, "limb": "near_fist", "dx": 32},
        "cpu": {"aggression": 80, "range": 46, "jump": 18, "poke": "DOUBLE LEG BLAST", "sweep": "SWEEP", "anti_air": "SUPLEX RUSH", "special": "DOUBLE LEG BLAST", "rush": "DOUBLE LEG BLAST", "throw": "SUPLEX RUSH", "super": "GOLD MEDAL COMBO"}
    },

    # 20 Fahri Yilmaz (Turkey, Oil Wrestling)
    {
        "fid": "fahri", "name": "FAHRI YILMAZ", "origin": "TURKEY",
        "style": "OIL WRESTLING", "discipline": "OIL WRESTLING", "stage": "TRADITIONAL FIELD",
        "life": 1090, "physics": {"walk_fwd": 1.9, "walk_back": 1.4, "dash": 4.6, "backdash": [3.6, 3.8], "jump": [2.8, 9.8], "gravity": 0.60},
        "push": [18, 94, 68], "close_range": 52, "guard_max": 115,
        "build": b_standard(scale=1.08, heaviness=1.22),
        "materials": {"thigh": "A", "shin": "A", "foot": "skin", "torso": "skin", "upper_arm": "skin", "forearm": "skin", "fist": "skin",
                      "trunks": 0.8, "shorts": "A", "waistband": "B", "hair": "C", "hair_style": "default", "muscle": True},
        "palette": {"skin": [[150, 94, 66], [210, 146, 108], [244, 196, 158]],
                    "A": [[20, 20, 24], [38, 38, 46], [68, 68, 80]],
                    "B": [[150, 110, 20], [216, 168, 36], [250, 218, 96]],
                    "C": [[18, 18, 22], [36, 36, 44], [64, 64, 78]]},
        "alt_palette": {"A": [[120, 30, 20], [170, 50, 30], [210, 90, 60]], "B": [[20, 20, 24], [38, 38, 46], [68, 68, 80]]},
        "specials": [
            {"name": "KISBET GRIP", "motion": "qcf", "pose_prep": {"base": "stance", "torso": 22, "near_fist": [24, 8]},
             "pose_hit": {"base": "stance", "torso": 48, "near_fist": [36, 18]}, "limb": "near_fist", "dx": 26},
            {"name": "ANATOLIAN BEAR", "motion": "dp", "pose_prep": {"base": "stance", "torso": 10},
             "pose_hit": {"base": "stance", "torso": -20, "near_fist": [22, -18], "far_fist": [18, -16]}, "limb": "near_fist", "dx": 20},
            {"name": "BULL TACKLE", "motion": "qcb", "pose_prep": {"base": "crouch", "torso": 32},
             "pose_hit": {"base": "stance", "torso": 42, "near_fist": [36, 4]}, "limb": "near_fist", "dx": 34}
        ],
        "super_move": {"name": "GRAND OTTOMAN SLAM", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 24},
                       "pose_hit": {"base": "stance", "torso": -52, "near_fist": [18, -32]}, "limb": "near_fist", "dx": 32},
        "cpu": {"aggression": 80, "range": 44, "jump": 12, "poke": "STRAIGHT", "sweep": "SWEEP", "anti_air": "ANATOLIAN BEAR", "special": "KISBET GRIP", "rush": "BULL TACKLE", "throw": "KISBET GRIP", "super": "GRAND OTTOMAN SLAM"}
    },

    # 21 Maya Santos (Philippines, Panantukan)
    {
        "fid": "maya", "name": "MAYA SANTOS", "origin": "PHILIPPINES",
        "style": "PANANTUKAN", "discipline": "PANANTUKAN", "stage": "MANILA DOCKSIDE YARD",
        "life": 980, "physics": {"walk_fwd": 2.3, "walk_back": 1.8, "dash": 5.3, "backdash": [4.2, 4.3], "jump": [3.3, 10.5], "gravity": 0.53},
        "push": [15, 88, 56], "close_range": 46, "guard_max": 95,
        "build": b_standard(scale=0.98, heaviness=0.95),
        "materials": {"thigh": "A", "shin": "A", "foot": "B", "torso": "B", "upper_arm": "skin", "forearm": "skin", "fist": "C",
                      "trunks": 0.6, "shorts": "A", "waistband": "C", "shin_split": 0.55, "boot": "B", "hair": "C", "hair_style": "topknot"},
        "palette": {"skin": [[140, 84, 56], [198, 134, 94], [238, 182, 138]],
                    "A": [[22, 28, 52], [44, 58, 104], [84, 106, 174]],
                    "B": [[130, 24, 28], [196, 42, 46], [244, 92, 90]],
                    "C": [[180, 140, 20], [236, 196, 40], [254, 238, 110]]},
        "alt_palette": {"A": [[130, 24, 28], [196, 42, 46], [244, 92, 90]], "B": [[22, 28, 52], [44, 58, 104], [84, 106, 174]]},
        "specials": [
            {"name": "BICEP DESTRUCTION", "motion": "qcb", "pose_prep": {"base": "guard", "torso": 4},
             "pose_hit": {"base": "stance", "torso": 20, "near_elbow": [30, 0]}, "limb": "near_fist", "dx": 20},
            {"name": "MANILA ELBOW", "motion": "qcf", "pose_prep": {"base": "stance", "torso": 14},
             "pose_hit": {"base": "stance", "torso": 24, "near_elbow": [34, -2]}, "limb": "near_fist", "dx": 26},
            {"name": "LOW TRAP", "motion": "dp", "pose_prep": {"base": "crouch", "torso": 18},
             "pose_hit": {"base": "crouch", "torso": 40, "near_fist": [34, 16]}, "limb": "near_fist", "dx": 22}
        ],
        "super_move": {"name": "KALI STORM", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 10},
                       "pose_hit": {"base": "stance", "torso": 26, "near_fist": [38, 0]}, "limb": "near_fist", "dx": 34},
        "cpu": {"aggression": 84, "range": 48, "jump": 24, "poke": "MANILA ELBOW", "sweep": "SWEEP", "anti_air": "MANILA ELBOW", "special": "MANILA ELBOW", "rush": "BICEP DESTRUCTION", "throw": "THROW", "super": "KALI STORM"}
    },

    # 22 Aditya Surya (Indonesia, Pencak Silat)
    {
        "fid": "aditya", "name": "ADITYA SURYA", "origin": "INDONESIA",
        "style": "PENCAK SILAT", "discipline": "SILAT", "stage": "RAINFOREST SCHOOL",
        "life": 970, "physics": {"walk_fwd": 2.3, "walk_back": 1.8, "dash": 5.4, "backdash": [4.3, 4.3], "jump": [3.4, 10.6], "gravity": 0.52},
        "push": [15, 88, 56], "close_range": 46, "guard_max": 95,
        "build": b_standard(scale=0.98, heaviness=0.94),
        "materials": {"thigh": "A", "shin": "A", "foot": "skin", "torso": "A", "upper_arm": "A", "forearm": "skin", "fist": "skin",
                      "belt": "B", "waistband": "B", "hair": "C", "hair_style": "topknot"},
        "palette": {"skin": [[136, 82, 52], [194, 128, 86], [236, 176, 132]],
                    "A": [[20, 20, 26], [42, 42, 54], [78, 78, 96]],
                    "B": [[150, 110, 20], [216, 168, 36], [250, 218, 96]],
                    "C": [[16, 16, 20], [32, 32, 40], [58, 58, 72]]},
        "alt_palette": {"A": [[130, 22, 26], [196, 40, 44], [244, 90, 88]], "B": [[150, 110, 20], [216, 168, 36], [250, 218, 96]]},
        "specials": [
            {"name": "TIGER POUNCE", "motion": "qcf", "pose_prep": {"base": "crouch", "torso": 24},
             "pose_hit": {"base": "jump", "torso": 30, "near_fist": [42, 10]}, "limb": "near_fist", "dx": 38, "dy": 16, "anim_flags": "active|airborne"},
            {"name": "HARIMAU SWEEP", "motion": "qcb", "pose_prep": {"base": "crouch", "torso": 36},
             "pose_hit": {"base": "crouch", "torso": 62, "far_foot": [48, -2]}, "limb": "far_ankle", "dx": 24},
            {"name": "SILAT PARRY", "motion": "dp", "pose_prep": {"base": "guard", "torso": -4},
             "pose_hit": {"base": "stance", "torso": 28, "near_fist": [34, -4]}, "limb": "near_fist", "dx": 18}
        ],
        "super_move": {"name": "NUSANTARA WRATH", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 12},
                       "pose_hit": {"base": "stance", "torso": 36, "near_fist": [44, 2]}, "limb": "near_fist", "dx": 36},
        "cpu": {"aggression": 76, "range": 56, "jump": 30, "poke": "TIGER POUNCE", "sweep": "HARIMAU SWEEP", "anti_air": "SILAT PARRY", "special": "TIGER POUNCE", "rush": "TIGER POUNCE", "throw": "THROW", "super": "NUSANTARA WRATH"}
    },

    # 23 Sokha Vann (Cambodia, Pradal Serey)
    {
        "fid": "sokha", "name": "SOKHA VANN", "origin": "CAMBODIA",
        "style": "PRADAL SEREY", "discipline": "PRADAL SEREY", "stage": "TEMPLE FESTIVAL ARENA",
        "life": 1000, "physics": {"walk_fwd": 2.2, "walk_back": 1.7, "dash": 5.2, "backdash": [4.0, 4.2], "jump": [3.2, 10.5], "gravity": 0.54},
        "push": [16, 90, 58], "close_range": 46, "guard_max": 100,
        "build": b_standard(scale=1.0, heaviness=0.98),
        "materials": {"thigh": "A", "shin": "A", "foot": "skin", "torso": "skin", "upper_arm": "skin", "forearm": "skin", "fist": "skin",
                      "trunks": 0.65, "shorts": "A", "waistband": "B", "ankle_wrap": "B", "hair": "C", "hair_style": "spiky", "muscle": True},
        "palette": {"skin": [[136, 82, 52], [194, 128, 86], [236, 176, 132]],
                    "A": [[20, 36, 96], [38, 70, 172], [76, 120, 240]],
                    "B": [[150, 110, 20], [216, 168, 36], [250, 218, 96]],
                    "C": [[18, 18, 22], [36, 36, 44], [64, 64, 78]]},
        "alt_palette": {"A": [[130, 20, 26], [198, 38, 44], [244, 88, 88]], "B": [[20, 36, 96], [38, 70, 172], [76, 120, 240]]},
        "specials": [
            {"name": "SPINNING ELBOW", "motion": "qcb", "pose_prep": {"base": "stance", "torso": -20},
             "pose_hit": {"base": "stance", "torso": 28, "far_elbow": [34, 0]}, "limb": "far_fist", "dx": 24},
            {"name": "ANGKOR RISING", "motion": "dp", "pose_prep": {"base": "crouch", "torso": 10},
             "pose_hit": {"base": "jump", "torso": -10, "near_elbow": [28, -28]}, "limb": "near_fist", "dx": 20, "dy": 26, "anim_flags": "active|airborne"},
            {"name": "LOW CHOP KICK", "motion": "qcf", "pose_prep": {"base": "stance", "torso": 8},
             "pose_hit": {"base": "crouch", "torso": 38, "near_foot": [42, -4]}, "limb": "near_ankle", "dx": 22}
        ],
        "super_move": {"name": "TEMPLE TEMPEST", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 12},
                       "pose_hit": {"base": "stance", "torso": 32, "near_elbow": [38, -2]}, "limb": "near_fist", "dx": 34},
        "cpu": {"aggression": 82, "range": 50, "jump": 24, "poke": "SPINNING ELBOW", "sweep": "LOW CHOP KICK", "anti_air": "ANGKOR RISING", "special": "SPINNING ELBOW", "rush": "SPINNING ELBOW", "throw": "THROW", "super": "TEMPLE TEMPEST"}
    },

    # 24 Lucas Ferreira (Brazil, BJJ)
    {
        "fid": "lucas", "name": "LUCAS FERREIRA", "origin": "BRAZIL",
        "style": "BRAZILIAN JIU-JITSU", "discipline": "JIU-JITSU", "stage": "BEACHSIDE ACADEMY",
        "life": 1010, "physics": {"walk_fwd": 2.1, "walk_back": 1.6, "dash": 4.9, "backdash": [3.9, 4.0], "jump": [3.1, 10.3], "gravity": 0.56},
        "push": [16, 90, 60], "close_range": 50, "guard_max": 105,
        "build": b_standard(scale=1.01, heaviness=1.02),
        "materials": {"thigh": "A", "shin": "A", "foot": "skin", "torso": "A", "upper_arm": "A", "forearm": "skin", "fist": "skin",
                      "gi_lapel": "A", "belt": "C", "waistband": "C", "sleeve": 0.85, "hair": "C", "hair_style": "default"},
        "palette": {"skin": [[144, 88, 58], [204, 138, 98], [242, 188, 146]],
                    "A": [[22, 40, 102], [42, 76, 180], [84, 128, 244]],
                    "B": [[220, 218, 212], [240, 238, 234], [254, 254, 254]],
                    "C": [[18, 18, 22], [34, 34, 42], [62, 62, 76]]},
        "alt_palette": {"A": [[220, 218, 212], [240, 238, 234], [254, 254, 254]], "B": [[22, 40, 102], [42, 76, 180], [84, 128, 244]]},
        "specials": [
            {"name": "GUARD PULL", "motion": "qcf", "pose_prep": {"base": "crouch", "torso": 28},
             "pose_hit": {"base": "crouch", "torso": 54, "near_fist": [36, 18]}, "limb": "near_fist", "dx": 28},
            {"name": "ARMBAR SNAP", "motion": "dp", "pose_prep": {"base": "stance", "torso": 10},
             "pose_hit": {"base": "jump", "torso": 40, "near_fist": [32, 2]}, "limb": "near_fist", "dx": 24, "dy": 22, "anim_flags": "active|airborne"},
            {"name": "BUTTERFLY SWEEP", "motion": "qcb", "pose_prep": {"base": "crouch", "torso": 34},
             "pose_hit": {"base": "crouch", "torso": 52, "far_foot": [44, -2]}, "limb": "far_ankle", "dx": 20}
        ],
        "super_move": {"name": "GRACIE FINISHER", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 16},
                       "pose_hit": {"base": "crouch", "torso": 56, "near_fist": [38, 16]}, "limb": "near_fist", "dx": 32},
        "cpu": {"aggression": 74, "range": 46, "jump": 14, "poke": "STRAIGHT", "sweep": "BUTTERFLY SWEEP", "anti_air": "ARMBAR SNAP", "special": "GUARD PULL", "rush": "GUARD PULL", "throw": "THROW", "super": "GRACIE FINISHER"}
    },

    # 25 Noah Williams (Canada, Kyokushin)
    {
        "fid": "noah", "name": "NOAH WILLIAMS", "origin": "CANADA",
        "style": "KYOKUSHIN KARATE", "discipline": "KARATE", "stage": "MOUNTAIN LODGE",
        "life": 1050, "physics": {"walk_fwd": 2.0, "walk_back": 1.5, "dash": 4.8, "backdash": [3.8, 4.0], "jump": [3.0, 10.2], "gravity": 0.58},
        "push": [17, 92, 64], "close_range": 46, "guard_max": 110,
        "build": b_standard(scale=1.04, heaviness=1.10),
        "materials": {"thigh": "A", "shin": "A", "foot": "skin", "torso": "A", "upper_arm": "A", "forearm": "skin", "fist": "skin",
                      "gi_lapel": "A", "belt": "C", "waistband": "C", "open_chest": True, "hair": "C", "hair_style": "default"},
        "palette": {"skin": [[154, 96, 68], [214, 150, 114], [246, 198, 160]],
                    "A": [[220, 218, 212], [240, 238, 234], [254, 254, 254]],
                    "B": [[130, 24, 28], [196, 42, 46], [244, 92, 90]],
                    "C": [[18, 18, 22], [34, 34, 42], [62, 62, 76]]},
        "alt_palette": {"A": [[24, 28, 52], [46, 54, 96], [82, 96, 150]], "B": [[150, 110, 10], [214, 170, 30], [248, 222, 96]]},
        "specials": [
            {"name": "IRON CHEST", "motion": "qcf", "pose_prep": {"base": "stance", "torso": 14, "hip": [2, 2]},
             "pose_hit": {"base": "stance", "torso": 26, "near_fist": [38, 0]}, "limb": "near_fist", "dx": 28},
            {"name": "SEIKEN LOW", "motion": "qcb", "pose_prep": {"base": "crouch", "torso": 20},
             "pose_hit": {"base": "crouch", "torso": 38, "near_fist": [34, 14]}, "limb": "near_fist", "dx": 22},
            {"name": "ROUGH ROUNDHOUSE", "motion": "dp", "pose_prep": {"base": "stance", "torso": -8},
             "pose_hit": {"base": "stance", "torso": -22, "far_foot": [44, -38]}, "limb": "far_ankle", "dx": 24}
        ],
        "super_move": {"name": "MOUNTAIN SPIRIT", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 18},
                       "pose_hit": {"base": "stance", "torso": 32, "near_fist": [42, 0]}, "limb": "near_fist", "dx": 34},
        "cpu": {"aggression": 80, "range": 48, "jump": 16, "poke": "IRON CHEST", "sweep": "SWEEP", "anti_air": "ROUGH ROUNDHOUSE", "special": "IRON CHEST", "rush": "SEIKEN LOW", "throw": "THROW", "super": "MOUNTAIN SPIRIT"}
    },

    # 26 Zara Khan (Pakistan, Pehlwani)
    {
        "fid": "zara", "name": "ZARA KHAN", "origin": "PAKISTAN",
        "style": "PEHLWANI", "discipline": "PEHLWANI", "stage": "AKHARA COURTYARD",
        "life": 1040, "physics": {"walk_fwd": 2.0, "walk_back": 1.5, "dash": 4.9, "backdash": [3.8, 4.0], "jump": [3.0, 10.2], "gravity": 0.58},
        "push": [17, 92, 64], "close_range": 50, "guard_max": 110,
        "build": b_standard(scale=1.03, heaviness=1.08),
        "materials": {"thigh": "A", "shin": "A", "foot": "skin", "torso": "B", "upper_arm": "skin", "forearm": "skin", "fist": "skin",
                      "trunks": 0.65, "shorts": "A", "waistband": "C", "hair": "C", "hair_style": "braids", "muscle": True},
        "palette": {"skin": [[140, 84, 54], [198, 132, 92], [238, 180, 136]],
                    "A": [[24, 88, 52], [48, 148, 92], [112, 214, 156]],
                    "B": [[140, 22, 28], [206, 42, 46], [248, 96, 92]],
                    "C": [[180, 140, 20], [236, 196, 40], [254, 238, 110]]},
        "alt_palette": {"A": [[140, 22, 28], [206, 42, 46], [248, 96, 92]], "B": [[24, 88, 52], [48, 148, 92], [112, 214, 156]]},
        "specials": [
            {"name": "KUSTHI LIFT", "motion": "qcf", "pose_prep": {"base": "stance", "torso": 20, "near_fist": [22, 8]},
             "pose_hit": {"base": "stance", "torso": -38, "near_fist": [18, -26]}, "limb": "near_fist", "dx": 26},
            {"name": "GADA CRUSH", "motion": "dp", "pose_prep": {"base": "stance", "torso": -10},
             "pose_hit": {"base": "crouch", "torso": 46, "near_fist": [32, 24], "far_fist": [28, 22]}, "limb": "near_fist", "dx": 22},
            {"name": "CLINCH TOSS", "motion": "qcb", "pose_prep": {"base": "stance", "torso": 12},
             "pose_hit": {"base": "stance", "torso": 38, "near_fist": [34, 14]}, "limb": "near_fist", "dx": 20}
        ],
        "super_move": {"name": "LAHORE AVALANCHE", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 18},
                       "pose_hit": {"base": "stance", "torso": -50, "near_fist": [16, -30]}, "limb": "near_fist", "dx": 32},
        "cpu": {"aggression": 78, "range": 46, "jump": 14, "poke": "STRAIGHT", "sweep": "SWEEP", "anti_air": "GADA CRUSH", "special": "KUSTHI LIFT", "rush": "KUSTHI LIFT", "throw": "CLINCH TOSS", "super": "LAHORE AVALANCHE"}
    },

    # 27 Thabo Nkosi (South Africa, Boxing)
    {
        "fid": "thabo", "name": "THABO NKOSI", "origin": "SOUTH AFRICA",
        "style": "BOXING", "discipline": "BOXING", "stage": "JOHANNESBURG BOXING CLUB",
        "life": 990, "physics": {"walk_fwd": 2.3, "walk_back": 1.8, "dash": 5.4, "backdash": [4.3, 4.3], "jump": [3.2, 10.4], "gravity": 0.54},
        "push": [16, 90, 58], "close_range": 46, "guard_max": 100,
        "build": b_standard(scale=1.0, heaviness=0.98),
        "materials": {"thigh": "A", "shin": "A", "foot": "C", "torso": "skin", "upper_arm": "skin", "forearm": "skin", "fist": "B",
                      "trunks": 0.65, "shorts": "A", "waistband": "B", "shin_split": 0.55, "boot": "C", "hair": "C", "hair_style": "default", "muscle": True},
        "palette": {"skin": [[92, 56, 36], [146, 92, 64], [192, 130, 96]],
                    "A": [[22, 90, 54], [46, 150, 92], [110, 216, 156]],
                    "B": [[150, 110, 20], [216, 168, 36], [250, 218, 96]],
                    "C": [[18, 18, 22], [36, 36, 44], [64, 64, 78]]},
        "alt_palette": {"A": [[140, 22, 28], [206, 42, 46], [248, 96, 92]], "B": [[22, 90, 54], [46, 150, 92], [110, 216, 156]]},
        "specials": [
            {"name": "FLICKER JAB", "motion": "qcf", "pose_prep": {"base": "stance", "torso": 10, "near_fist": [18, 2]},
             "pose_hit": {"base": "stance", "torso": 18, "near_fist": [38, -4]}, "limb": "near_fist", "dx": 26},
            {"name": "PIVOT CROSS", "motion": "qcb", "pose_prep": {"base": "stance", "torso": -6},
             "pose_hit": {"base": "stance", "torso": 24, "far_fist": [36, 0]}, "limb": "far_fist", "dx": 22},
            {"name": "SHUFFLE HOOK", "motion": "dp", "pose_prep": {"base": "crouch", "torso": 12},
             "pose_hit": {"base": "stance", "torso": 26, "near_fist": [34, -8]}, "limb": "near_fist", "dx": 24}
        ],
        "super_move": {"name": "JOBURG LIGHTNING", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 12},
                       "pose_hit": {"base": "stance", "torso": 30, "near_fist": [42, -2]}, "limb": "near_fist", "dx": 36},
        "cpu": {"aggression": 82, "range": 56, "jump": 16, "poke": "FLICKER JAB", "sweep": "SWEEP", "anti_air": "SHUFFLE HOOK", "special": "FLICKER JAB", "rush": "FLICKER JAB", "throw": "THROW", "super": "JOBURG LIGHTNING"}
    },

    # 28 Maia Korhonen (Finland, Combat Wrestling)
    {
        "fid": "maia", "name": "MAIA KORHONEN", "origin": "FINLAND",
        "style": "COMBAT WRESTLING", "discipline": "WRESTLING", "stage": "FROZEN HARBOR",
        "life": 1020, "physics": {"walk_fwd": 2.1, "walk_back": 1.6, "dash": 5.0, "backdash": [3.9, 4.0], "jump": [3.1, 10.3], "gravity": 0.56},
        "push": [16, 90, 58], "close_range": 48, "guard_max": 105,
        "build": b_standard(scale=1.0, heaviness=1.02),
        "materials": {"thigh": "A", "shin": "A", "foot": "C", "torso": "A", "upper_arm": "A", "forearm": "skin", "fist": "skin",
                      "vest": "B", "shin_split": 0.55, "boot": "C", "hair": "C", "hair_style": "default"},
        "palette": {"skin": [[156, 98, 70], [216, 152, 116], [248, 200, 164]],
                    "A": [[24, 38, 92], [46, 72, 164], [88, 122, 234]],
                    "B": [[210, 206, 200], [236, 234, 230], [254, 254, 254]],
                    "C": [[20, 22, 28], [40, 42, 54], [74, 76, 94]]},
        "alt_palette": {"A": [[20, 22, 28], [40, 42, 54], [74, 76, 94]], "B": [[24, 38, 92], [46, 72, 164], [88, 122, 234]]},
        "specials": [
            {"name": "COLD COUNTER", "motion": "qcb", "pose_prep": {"base": "guard", "torso": -4},
             "pose_hit": {"base": "stance", "torso": 30, "near_fist": [32, 6]}, "limb": "near_fist", "dx": 18},
            {"name": "HIP TOSS", "motion": "qcf", "pose_prep": {"base": "stance", "torso": 16, "near_fist": [22, 8]},
             "pose_hit": {"base": "stance", "torso": 46, "near_fist": [36, 22]}, "limb": "near_fist", "dx": 24},
            {"name": "NORDIC TRAP", "motion": "dp", "pose_prep": {"base": "crouch", "torso": 14},
             "pose_hit": {"base": "jump", "torso": 20, "near_fist": [28, -20]}, "limb": "near_fist", "dx": 20, "dy": 24, "anim_flags": "active|airborne"}
        ],
        "super_move": {"name": "HELSINKI LOCK", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 14},
                       "pose_hit": {"base": "crouch", "torso": 54, "near_fist": [38, 18]}, "limb": "near_fist", "dx": 32},
        "cpu": {"aggression": 72, "range": 48, "jump": 16, "poke": "STRAIGHT", "sweep": "SWEEP", "anti_air": "NORDIC TRAP", "special": "HIP TOSS", "rush": "COLD COUNTER", "throw": "HIP TOSS", "super": "HELSINKI LOCK"}
    },

    # 29 Tomas Vega (Spain, Full-Contact Karate)
    {
        "fid": "tomas", "name": "TOMAS VEGA", "origin": "SPAIN",
        "style": "FULL-CONTACT KARATE", "discipline": "KARATE", "stage": "BARCELONA TOURNAMENT HALL",
        "life": 990, "physics": {"walk_fwd": 2.2, "walk_back": 1.7, "dash": 5.2, "backdash": [4.1, 4.2], "jump": [3.3, 10.4], "gravity": 0.54},
        "push": [16, 90, 58], "close_range": 46, "guard_max": 100,
        "build": b_standard(scale=1.0, heaviness=0.98),
        "materials": {"thigh": "A", "shin": "A", "foot": "skin", "torso": "skin", "upper_arm": "skin", "forearm": "skin", "fist": "skin",
                      "trunks": 0.65, "shorts": "A", "waistband": "B", "hair": "C", "hair_style": "spiky", "muscle": True},
        "palette": {"skin": [[150, 92, 62], [208, 144, 104], [242, 194, 152]],
                    "A": [[22, 22, 28], [44, 44, 56], [82, 82, 100]],
                    "B": [[150, 20, 24], [220, 40, 44], [254, 96, 92]],
                    "C": [[18, 18, 22], [36, 36, 44], [64, 64, 78]]},
        "alt_palette": {"A": [[150, 20, 24], [220, 40, 44], [254, 96, 92]], "B": [[180, 140, 20], [236, 196, 40], [254, 238, 110]]},
        "specials": [
            {"name": "TORO PUNCH", "motion": "qcf", "pose_prep": {"base": "stance", "torso": 16, "hip": [4, 2]},
             "pose_hit": {"base": "stance", "torso": 28, "near_fist": [40, 0]}, "limb": "near_fist", "dx": 32},
            {"name": "USHIRO GERI", "motion": "qcb", "pose_prep": {"base": "stance", "torso": -16},
             "pose_hit": {"base": "stance", "torso": -32, "far_foot": [46, -34]}, "limb": "far_ankle", "dx": 26},
            {"name": "FLAME AXE", "motion": "dp", "pose_prep": {"base": "crouch", "torso": 10},
             "pose_hit": {"base": "jump", "torso": 16, "near_foot": [32, 4]}, "limb": "near_ankle", "dx": 22, "dy": 26, "anim_flags": "active|airborne"}
        ],
        "super_move": {"name": "IBERIAN TEMPEST", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 14},
                       "pose_hit": {"base": "stance", "torso": 32, "near_fist": [42, 0]}, "limb": "near_fist", "dx": 36},
        "cpu": {"aggression": 86, "range": 52, "jump": 26, "poke": "TORO PUNCH", "sweep": "SWEEP", "anti_air": "FLAME AXE", "special": "TORO PUNCH", "rush": "TORO PUNCH", "throw": "THROW", "super": "IBERIAN TEMPEST"}
    },

    # 30 Leila Ben Ammar (Tunisia, Kickboxing/Boxing)
    {
        "fid": "leila", "name": "LEILA BEN AMMAR", "origin": "TUNISIA",
        "style": "KICKBOXING", "discipline": "KICKBOXING", "stage": "MEDITERRANEAN CLUB",
        "life": 970, "physics": {"walk_fwd": 2.3, "walk_back": 1.8, "dash": 5.3, "backdash": [4.2, 4.3], "jump": [3.3, 10.5], "gravity": 0.53},
        "push": [15, 88, 56], "close_range": 46, "guard_max": 95,
        "build": b_standard(scale=0.98, heaviness=0.95),
        "materials": {"thigh": "A", "shin": "A", "foot": "skin", "torso": "B", "upper_arm": "skin", "forearm": "skin", "fist": "B",
                      "trunks": 0.65, "shorts": "A", "waistband": "C", "ankle_wrap": "C", "hair": "C", "hair_style": "topknot"},
        "palette": {"skin": [[144, 88, 58], [204, 138, 98], [242, 188, 146]],
                    "A": [[130, 22, 28], [196, 40, 44], [244, 90, 88]],
                    "B": [[210, 206, 200], [236, 234, 230], [254, 254, 254]],
                    "C": [[18, 18, 24], [36, 36, 48], [68, 68, 86]]},
        "alt_palette": {"A": [[20, 36, 96], [38, 70, 172], [76, 120, 240]], "B": [[130, 22, 28], [196, 40, 44], [244, 90, 88]]},
        "specials": [
            {"name": "TIMED JAB", "motion": "qcf", "pose_prep": {"base": "stance", "torso": 10, "near_fist": [18, 0]},
             "pose_hit": {"base": "stance", "torso": 18, "near_fist": [36, -2]}, "limb": "near_fist", "dx": 26},
            {"name": "SAHARA KNEE", "motion": "dp", "pose_prep": {"base": "crouch", "torso": 12},
             "pose_hit": {"base": "jump", "torso": -8, "near_knee": [30, -26]}, "limb": "near_ankle", "dx": 20, "dy": 26, "anim_flags": "active|airborne"},
            {"name": "MEDITERRANEAN HOOK", "motion": "qcb", "pose_prep": {"base": "crouch", "torso": 14},
             "pose_hit": {"base": "stance", "torso": 24, "far_fist": [34, -4]}, "limb": "far_fist", "dx": 22}
        ],
        "super_move": {"name": "CARTHAGE ECLIPSE", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 12},
                       "pose_hit": {"base": "stance", "torso": 28, "near_fist": [40, 0]}, "limb": "near_fist", "dx": 34},
        "cpu": {"aggression": 80, "range": 56, "jump": 22, "poke": "TIMED JAB", "sweep": "SWEEP", "anti_air": "SAHARA KNEE", "special": "TIMED JAB", "rush": "MEDITERRANEAN HOOK", "throw": "THROW", "super": "CARTHAGE ECLIPSE"}
    },

    # 31 Kai Mana (Samoa, Polynesian Wrestling)
    {
        "fid": "kaimana", "name": "KAI MANA", "origin": "SAMOA",
        "style": "POLYNESIAN WRESTLING", "discipline": "WRESTLING", "stage": "COASTAL CEREMONIAL ARENA",
        "life": 1100, "physics": {"walk_fwd": 1.9, "walk_back": 1.4, "dash": 4.6, "backdash": [3.6, 3.8], "jump": [2.8, 9.8], "gravity": 0.60},
        "push": [18, 94, 68], "close_range": 52, "guard_max": 120,
        "build": b_standard(scale=1.08, heaviness=1.24),
        "materials": {"thigh": "A", "shin": "A", "foot": "skin", "torso": "skin", "upper_arm": "skin", "forearm": "skin", "fist": "skin",
                      "trunks": 0.75, "shorts": "A", "waistband": "B", "hair": "C", "hair_style": "default", "muscle": True},
        "palette": {"skin": [[138, 84, 54], [196, 130, 88], [236, 178, 134]],
                    "A": [[130, 20, 26], [196, 38, 44], [244, 88, 88]],
                    "B": [[150, 110, 20], [216, 168, 36], [250, 218, 96]],
                    "C": [[18, 18, 22], [36, 36, 44], [64, 64, 78]]},
        "alt_palette": {"A": [[20, 36, 96], [38, 70, 172], [76, 120, 240]], "B": [[130, 20, 26], [196, 38, 44], [244, 88, 88]]},
        "specials": [
            {"name": "ISLAND TACKLE", "motion": "qcf", "pose_prep": {"base": "crouch", "torso": 32, "hip": [4, 14]},
             "pose_hit": {"base": "stance", "torso": 44, "near_fist": [38, 4]}, "limb": "near_fist", "dx": 36},
            {"name": "PACIFIC SLAP", "motion": "dp", "pose_prep": {"base": "stance", "torso": 10},
             "pose_hit": {"base": "stance", "torso": 28, "near_fist": [36, -10]}, "limb": "near_fist", "dx": 22},
            {"name": "SAMOAN DROP", "motion": "qcb", "pose_prep": {"base": "stance", "torso": 18, "near_fist": [22, 10]},
             "pose_hit": {"base": "stance", "torso": -52, "near_fist": [16, -28]}, "limb": "near_fist", "dx": 20}
        ],
        "super_move": {"name": "VOLCANO TSUNAMI", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 22},
                       "pose_hit": {"base": "stance", "torso": 54, "near_fist": [42, 6]}, "limb": "near_fist", "dx": 38},
        "cpu": {"aggression": 84, "range": 44, "jump": 12, "poke": "PACIFIC SLAP", "sweep": "SWEEP", "anti_air": "PACIFIC SLAP", "special": "ISLAND TACKLE", "rush": "ISLAND TACKLE", "throw": "SAMOAN DROP", "super": "VOLCANO TSUNAMI"}
    },

    # 32 Aleksei Moroz (Kazakhstan, Sambo)
    {
        "fid": "aleksei", "name": "ALEKSEI MOROZ", "origin": "KAZAKHSTAN",
        "style": "SAMBO", "discipline": "SAMBO", "stage": "MOUNTAIN COMPOUND",
        "life": 1030, "physics": {"walk_fwd": 2.1, "walk_back": 1.6, "dash": 4.9, "backdash": [3.9, 4.0], "jump": [3.1, 10.3], "gravity": 0.57},
        "push": [17, 91, 62], "close_range": 48, "guard_max": 105,
        "build": b_standard(scale=1.03, heaviness=1.08),
        "materials": {"thigh": "A", "shin": "A", "foot": "C", "torso": "A", "upper_arm": "A", "forearm": "skin", "fist": "skin",
                      "trunks": 0.7, "shorts": "B", "waistband": "C", "shin_split": 0.5, "boot": "C", "gi_lapel": "A", "hair": "C", "hair_style": "default"},
        "palette": {"skin": [[152, 94, 66], [210, 146, 108], [244, 196, 158]],
                    "A": [[130, 20, 26], [196, 38, 44], [244, 88, 88]],
                    "B": [[22, 34, 88], [44, 68, 160], [86, 118, 230]],
                    "C": [[20, 20, 26], [42, 42, 54], [78, 78, 96]]},
        "alt_palette": {"A": [[22, 34, 88], [44, 68, 160], [86, 118, 230]], "B": [[130, 20, 26], [196, 38, 44], [244, 88, 88]]},
        "specials": [
            {"name": "STEPPING THROW", "motion": "qcf", "pose_prep": {"base": "stance", "torso": 18, "near_fist": [22, 8]},
             "pose_hit": {"base": "stance", "torso": 48, "near_fist": [36, 20]}, "limb": "near_fist", "dx": 26},
            {"name": "STEP KICK", "motion": "qcb", "pose_prep": {"base": "stance", "torso": 8},
             "pose_hit": {"base": "stance", "torso": 24, "far_foot": [42, -14]}, "limb": "far_ankle", "dx": 24},
            {"name": "STEPPING ARMLOCK", "motion": "dp", "pose_prep": {"base": "crouch", "torso": 14},
             "pose_hit": {"base": "stance", "torso": -30, "near_fist": [26, -18]}, "limb": "near_fist", "dx": 18}
        ],
        "super_move": {"name": "STEPPE TITAN", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 16},
                       "pose_hit": {"base": "stance", "torso": -48, "near_fist": [18, -28]}, "limb": "near_fist", "dx": 32},
        "cpu": {"aggression": 78, "range": 46, "jump": 16, "poke": "STEP KICK", "sweep": "SWEEP", "anti_air": "STEPPING ARMLOCK", "special": "STEPPING THROW", "rush": "STEPPING THROW", "throw": "STEPPING THROW", "super": "STEPPE TITAN"}
    },

    # 33 Master Hayato (Japan, Old-School Karate/Jujutsu) - Bonus 1
    {
        "fid": "hayato", "name": "MASTER HAYATO", "origin": "JAPAN",
        "style": "OLD KARATE / JUJUTSU", "discipline": "KARATE", "stage": "ARATA MOUNTAIN DOJO",
        "life": 950, "physics": {"walk_fwd": 1.7, "walk_back": 1.3, "dash": 4.2, "backdash": [3.4, 3.6], "jump": [2.6, 9.4], "gravity": 0.62},
        "push": [16, 88, 58], "close_range": 44, "guard_max": 120,
        "build": b_standard(scale=0.96, heaviness=0.96),
        "materials": {"thigh": "A", "shin": "A", "foot": "skin", "torso": "A", "upper_arm": "A", "forearm": "skin", "fist": "skin",
                      "gi_lapel": "A", "belt": "C", "waistband": "C", "hair": "B", "hair_style": "topknot"},
        "palette": {"skin": [[140, 84, 54], [198, 132, 92], [238, 180, 136]],
                    "A": [[22, 22, 28], [44, 44, 56], [82, 82, 100]],
                    "B": [[200, 198, 194], [228, 226, 222], [254, 254, 254]],
                    "C": [[18, 18, 22], [34, 34, 42], [62, 62, 76]]},
        "alt_palette": {"A": [[200, 198, 194], [228, 226, 222], [254, 254, 254]], "B": [[22, 22, 28], [44, 44, 56], [82, 82, 100]]},
        "specials": [
            {"name": "ONE PUNCH KNOCKOUT", "motion": "qcf", "pose_prep": {"base": "stance", "torso": 16, "hip": [2, 2]},
             "pose_hit": {"base": "stance", "torso": 24, "near_fist": [42, 0]}, "limb": "near_fist", "dx": 30, "dmg_heavy": 120, "dmg_light": 95},
            {"name": "OLD JUJUTSU PARRY", "motion": "dp", "pose_prep": {"base": "guard", "torso": -4},
             "pose_hit": {"base": "stance", "torso": 34, "near_fist": [30, 20]}, "limb": "near_fist", "dx": 16},
            {"name": "ANCIENT SWEEP", "motion": "qcb", "pose_prep": {"base": "crouch", "torso": 22},
             "pose_hit": {"base": "crouch", "torso": 46, "far_foot": [46, -2]}, "limb": "far_ankle", "dx": 18}
        ],
        "super_move": {"name": "TRUE MASTER ESSENCE", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 12},
                       "pose_hit": {"base": "stance", "torso": 28, "near_fist": [44, 0]}, "limb": "near_fist", "dx": 34},
        "cpu": {"aggression": 65, "range": 48, "jump": 6, "poke": "ONE PUNCH KNOCKOUT", "sweep": "ANCIENT SWEEP", "anti_air": "OLD JUJUTSU PARRY", "special": "ONE PUNCH KNOCKOUT", "rush": "ONE PUNCH KNOCKOUT", "throw": "OLD JUJUTSU PARRY", "super": "TRUE MASTER ESSENCE"}
    },

    # 34 Mad Dog Mason (USA, Bare-Knuckle) - Bonus 2
    {
        "fid": "mason", "name": "MAD DOG MASON", "origin": "UNITED STATES",
        "style": "BARE-KNUCKLE BRAWLING", "discipline": "BRAWLING", "stage": "OLD BROOKLYN GYM",
        "life": 1050, "physics": {"walk_fwd": 2.2, "walk_back": 1.6, "dash": 5.1, "backdash": [3.9, 4.0], "jump": [3.1, 10.3], "gravity": 0.57},
        "push": [17, 91, 62], "close_range": 46, "guard_max": 105,
        "build": b_standard(scale=1.03, heaviness=1.08),
        "materials": {"thigh": "A", "shin": "A", "foot": "C", "torso": "B", "upper_arm": "skin", "forearm": "skin", "fist": "skin",
                      "vest": "B", "shin_split": 0.55, "boot": "C", "hair": "C", "hair_style": "spiky", "muscle": True},
        "palette": {"skin": [[150, 92, 62], [208, 144, 104], [242, 194, 152]],
                    "A": [[22, 34, 88], [44, 68, 160], [86, 118, 230]],
                    "B": [[210, 206, 200], [236, 234, 230], [254, 254, 254]],
                    "C": [[20, 20, 24], [40, 40, 48], [72, 72, 86]]},
        "alt_palette": {"A": [[20, 20, 24], [40, 40, 48], [72, 72, 86]], "B": [[140, 22, 28], [206, 42, 46], [248, 96, 92]]},
        "specials": [
            {"name": "HEADBUTT", "motion": "qcf", "pose_prep": {"base": "stance", "torso": -10},
             "pose_hit": {"base": "stance", "torso": 38, "head": 22}, "limb": "near_fist", "dx": 28},
            {"name": "DIRTY LOW", "motion": "qcb", "pose_prep": {"base": "crouch", "torso": 16},
             "pose_hit": {"base": "crouch", "torso": 40, "near_fist": [36, 12]}, "limb": "near_fist", "dx": 24},
            {"name": "MAD RUSH", "motion": "dp", "pose_prep": {"base": "stance", "torso": 14},
             "pose_hit": {"base": "stance", "torso": 28, "near_fist": [36, -6]}, "limb": "near_fist", "dx": 26}
        ],
        "super_move": {"name": "BRAWL APOCALYPSE", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 16},
                       "pose_hit": {"base": "stance", "torso": 46, "near_fist": [42, 4]}, "limb": "near_fist", "dx": 36},
        "cpu": {"aggression": 88, "range": 46, "jump": 20, "poke": "HEADBUTT", "sweep": "DIRTY LOW", "anti_air": "MAD RUSH", "special": "HEADBUTT", "rush": "MAD RUSH", "throw": "THROW", "super": "BRAWL APOCALYPSE"}
    },

    # 35 Lotus (Southeast Asia, Silat Assassin) - Bonus 3
    {
        "fid": "lotus", "name": "LOTUS", "origin": "SOUTHEAST ASIA",
        "style": "ASSASSIN SILAT", "discipline": "SILAT", "stage": "CHIMERA FACILITY",
        "life": 950, "physics": {"walk_fwd": 2.4, "walk_back": 1.9, "dash": 5.6, "backdash": [4.5, 4.4], "jump": [3.6, 10.7], "gravity": 0.50},
        "push": [15, 86, 54], "close_range": 44, "guard_max": 90,
        "build": b_standard(scale=0.96, heaviness=0.90),
        "materials": {"thigh": "A", "shin": "A", "foot": "skin", "torso": "A", "upper_arm": "A", "forearm": "skin", "fist": "skin",
                      "belt": "B", "waistband": "B", "hair": "C", "hair_style": "topknot"},
        "palette": {"skin": [[138, 84, 54], [196, 130, 88], [236, 178, 134]],
                    "A": [[20, 20, 24], [36, 36, 44], [64, 64, 78]],
                    "B": [[140, 20, 24], [210, 40, 44], [252, 94, 90]],
                    "C": [[16, 16, 20], [32, 32, 40], [58, 58, 72]]},
        "alt_palette": {"A": [[40, 24, 64], [72, 44, 114], [128, 86, 192]], "B": [[20, 20, 24], [36, 36, 44], [64, 64, 78]]},
        "specials": [
            {"name": "SHADOW STEP", "motion": "qcf", "pose_prep": {"base": "crouch", "torso": 32},
             "pose_hit": {"base": "stance", "torso": 24, "near_fist": [38, 0]}, "limb": "near_fist", "dx": 42},
            {"name": "VIPER CUT", "motion": "dp", "pose_prep": {"base": "crouch", "torso": 12},
             "pose_hit": {"base": "jump", "torso": -12, "near_fist": [26, -30]}, "limb": "near_fist", "dx": 20, "dy": 28, "anim_flags": "active|airborne"},
            {"name": "DEFECTION PARRY", "motion": "qcb", "pose_prep": {"base": "guard", "torso": -4},
             "pose_hit": {"base": "stance", "torso": 26, "far_fist": [36, -2]}, "limb": "far_fist", "dx": 22}
        ],
        "super_move": {"name": "SILENT LOTUS", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 8},
                       "pose_hit": {"base": "stance", "torso": 32, "near_fist": [44, 0]}, "limb": "near_fist", "dx": 38},
        "cpu": {"aggression": 86, "range": 58, "jump": 34, "poke": "SHADOW STEP", "sweep": "SWEEP", "anti_air": "VIPER CUT", "special": "SHADOW STEP", "rush": "SHADOW STEP", "throw": "THROW", "super": "SILENT LOTUS"}
    },

    # 36 El Toro Rojo (Mexico, Heavy Lucha) - Bonus 4
    {
        "fid": "tororojo", "name": "EL TORO ROJO", "origin": "MEXICO",
        "style": "HEAVY LUCHA LIBRE", "discipline": "LUCHA LIBRE", "stage": "MEXICO CITY ARENA",
        "life": 1100, "physics": {"walk_fwd": 1.9, "walk_back": 1.4, "dash": 4.6, "backdash": [3.6, 3.8], "jump": [2.8, 9.8], "gravity": 0.60},
        "push": [18, 94, 68], "close_range": 52, "guard_max": 120,
        "build": b_standard(scale=1.08, heaviness=1.24),
        "materials": {"thigh": "A", "shin": "A", "foot": "B", "torso": "skin", "upper_arm": "skin", "forearm": "skin", "fist": "B",
                      "trunks": 0.7, "shorts": "A", "waistband": "B", "shin_split": 0.55, "boot": "B", "hair": "A", "hair_style": "visor", "muscle": True},
        "palette": {"skin": [[148, 92, 64], [208, 144, 104], [244, 194, 154]],
                    "A": [[140, 20, 24], [210, 40, 44], [252, 94, 90]],
                    "B": [[180, 140, 20], [236, 196, 40], [254, 238, 110]],
                    "C": [[20, 20, 24], [40, 40, 48], [72, 72, 86]]},
        "alt_palette": {"A": [[20, 36, 96], [38, 70, 172], [76, 120, 240]], "B": [[140, 20, 24], [210, 40, 44], [252, 94, 90]]},
        "specials": [
            {"name": "TORO PRESS", "motion": "qcf", "pose_prep": {"base": "stance", "torso": 24},
             "pose_hit": {"base": "stance", "torso": 50, "near_fist": [38, 6]}, "limb": "near_fist", "dx": 34},
            {"name": "BULL BOMB", "motion": "qcb", "pose_prep": {"base": "stance", "torso": 16, "near_fist": [22, 10]},
             "pose_hit": {"base": "stance", "torso": 44, "near_fist": [36, 20]}, "limb": "near_fist", "dx": 24},
            {"name": "FLYING BULL", "motion": "dp", "pose_prep": {"base": "crouch", "torso": 16},
             "pose_hit": {"base": "jump", "torso": 70, "near_fist": [40, 14]}, "limb": "near_fist", "dx": 32, "dy": 16, "anim_flags": "active|airborne"}
        ],
        "super_move": {"name": "ROJO MEGAPLEX", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 20},
                       "pose_hit": {"base": "stance", "torso": -52, "near_fist": [18, -32]}, "limb": "near_fist", "dx": 32},
        "cpu": {"aggression": 84, "range": 44, "jump": 18, "poke": "TORO PRESS", "sweep": "SWEEP", "anti_air": "FLYING BULL", "special": "TORO PRESS", "rush": "TORO PRESS", "throw": "BULL BOMB", "super": "ROJO MEGAPLEX"}
    },

    # 37 Irina Volkova (Russia, Systema) - Bonus 5
    {
        "fid": "irina", "name": "IRINA VOLKOVA", "origin": "RUSSIA",
        "style": "SYSTEMA", "discipline": "SYSTEMA", "stage": "CHIMERA FACILITY",
        "life": 980, "physics": {"walk_fwd": 2.2, "walk_back": 1.7, "dash": 5.1, "backdash": [4.1, 4.2], "jump": [3.2, 10.4], "gravity": 0.54},
        "push": [15, 88, 56], "close_range": 46, "guard_max": 100,
        "build": b_standard(scale=0.99, heaviness=0.96),
        "materials": {"thigh": "A", "shin": "A", "foot": "C", "torso": "B", "upper_arm": "B", "forearm": "skin", "fist": "skin",
                      "vest": "B", "shin_split": 0.55, "boot": "C", "hair": "C", "hair_style": "topknot"},
        "palette": {"skin": [[154, 96, 68], [214, 150, 114], [246, 198, 160]],
                    "A": [[24, 30, 44], [44, 54, 78], [80, 96, 134]],
                    "B": [[140, 22, 28], [206, 42, 46], [248, 96, 92]],
                    "C": [[18, 18, 22], [36, 36, 44], [64, 64, 78]]},
        "alt_palette": {"A": [[18, 18, 22], [36, 36, 44], [64, 64, 78]], "B": [[24, 30, 44], [44, 54, 78], [80, 96, 134]]},
        "specials": [
            {"name": "WAVE DEFLECTION", "motion": "qcb", "pose_prep": {"base": "guard", "torso": -6},
             "pose_hit": {"base": "stance", "torso": 22, "near_fist": [34, 4]}, "limb": "near_fist", "dx": 20},
            {"name": "FLOWING PALM", "motion": "qcf", "pose_prep": {"base": "stance", "torso": 12},
             "pose_hit": {"base": "stance", "torso": 24, "near_fist": [38, 0]}, "limb": "near_fist", "dx": 26},
            {"name": "BIO-SPIRAL", "motion": "dp", "pose_prep": {"base": "crouch", "torso": 16},
             "pose_hit": {"base": "stance", "torso": 36, "near_fist": [32, 16]}, "limb": "near_fist", "dx": 22}
        ],
        "super_move": {"name": "SYSTEMA CASCADE", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 10},
                       "pose_hit": {"base": "stance", "torso": 30, "near_fist": [40, 2]}, "limb": "near_fist", "dx": 34},
        "cpu": {"aggression": 74, "range": 52, "jump": 18, "poke": "FLOWING PALM", "sweep": "SWEEP", "anti_air": "WAVE DEFLECTION", "special": "FLOWING PALM", "rush": "BIO-SPIRAL", "throw": "THROW", "super": "SYSTEMA CASCADE"}
    },

    # 38 Kofi Mensah (Ghana, Traditional Boxing) - Bonus 6
    {
        "fid": "kofi", "name": "KOFI MENSAH", "origin": "GHANA",
        "style": "TRADITIONAL BOXING", "discipline": "BOXING", "stage": "ACCRA STADIUM",
        "life": 1060, "physics": {"walk_fwd": 2.0, "walk_back": 1.5, "dash": 4.8, "backdash": [3.8, 4.0], "jump": [3.0, 10.2], "gravity": 0.58},
        "push": [17, 92, 64], "close_range": 48, "guard_max": 110,
        "build": b_standard(scale=1.04, heaviness=1.12),
        "materials": {"thigh": "A", "shin": "A", "foot": "C", "torso": "skin", "upper_arm": "skin", "forearm": "skin", "fist": "B",
                      "trunks": 0.65, "shorts": "A", "waistband": "B", "shin_split": 0.55, "boot": "C", "hair": "C", "hair_style": "default", "muscle": True},
        "palette": {"skin": [[92, 54, 34], [144, 88, 60], [190, 124, 90]],
                    "A": [[160, 100, 20], [224, 150, 32], [254, 206, 94]],
                    "B": [[140, 22, 28], [206, 42, 46], [248, 96, 92]],
                    "C": [[18, 18, 22], [36, 36, 44], [64, 64, 78]]},
        "alt_palette": {"A": [[22, 90, 54], [46, 150, 92], [110, 216, 156]], "B": [[160, 100, 20], [224, 150, 32], [254, 206, 94]]},
        "specials": [
            {"name": "ACCRA HAMMER", "motion": "qcf", "pose_prep": {"base": "stance", "torso": -8},
             "pose_hit": {"base": "crouch", "torso": 40, "near_fist": [36, 18]}, "limb": "near_fist", "dx": 28},
            {"name": "HEAVY HOOK", "motion": "qcb", "pose_prep": {"base": "stance", "torso": 10},
             "pose_hit": {"base": "stance", "torso": 24, "near_fist": [36, -2]}, "limb": "near_fist", "dx": 24},
            {"name": "IRON GUARD", "motion": "dp", "pose_prep": {"base": "guard", "torso": 2},
             "pose_hit": {"base": "stance", "torso": 26, "near_fist": [34, -4]}, "limb": "near_fist", "dx": 22}
        ],
        "super_move": {"name": "GOLD COAST BLAST", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 14},
                       "pose_hit": {"base": "stance", "torso": 32, "near_fist": [42, 0]}, "limb": "near_fist", "dx": 34},
        "cpu": {"aggression": 82, "range": 50, "jump": 14, "poke": "HEAVY HOOK", "sweep": "SWEEP", "anti_air": "IRON GUARD", "special": "ACCRA HAMMER", "rush": "HEAVY HOOK", "throw": "THROW", "super": "GOLD COAST BLAST"}
    },

    # 39 Xiao Mei (China, Wing Chun) - Bonus 7
    {
        "fid": "xiaomei", "name": "XIAO MEI", "origin": "CHINA",
        "style": "WING CHUN", "discipline": "WING CHUN", "stage": "CHIMERA FACILITY",
        "life": 960, "physics": {"walk_fwd": 2.3, "walk_back": 1.8, "dash": 5.4, "backdash": [4.3, 4.3], "jump": [3.3, 10.5], "gravity": 0.52},
        "push": [15, 86, 54], "close_range": 44, "guard_max": 95,
        "build": b_standard(scale=0.96, heaviness=0.90),
        "materials": {"thigh": "A", "shin": "A", "foot": "C", "torso": "B", "upper_arm": "B", "forearm": "skin", "fist": "skin",
                      "gi_lapel": "B", "sleeve": 0.8, "hair": "C", "hair_style": "topknot"},
        "palette": {"skin": [[154, 96, 68], [214, 150, 114], [246, 198, 160]],
                    "A": [[22, 22, 28], [44, 44, 56], [82, 82, 100]],
                    "B": [[220, 218, 212], [240, 238, 234], [254, 254, 254]],
                    "C": [[18, 18, 22], [34, 34, 42], [62, 62, 76]]},
        "alt_palette": {"A": [[22, 22, 28], [44, 44, 56], [82, 82, 100]], "B": [[140, 22, 28], [206, 42, 46], [248, 96, 92]]},
        "specials": [
            {"name": "CHAIN PUNCH", "motion": "qcf", "pose_prep": {"base": "stance", "torso": 12, "far_fist": [22, 2]},
             "pose_hit": {"base": "stance", "torso": 18, "near_fist": [36, -2]}, "limb": "near_fist", "dx": 24},
            {"name": "PAK SAO PARRY", "motion": "dp", "pose_prep": {"base": "guard", "torso": -2},
             "pose_hit": {"base": "stance", "torso": 20, "near_fist": [32, 2]}, "limb": "near_fist", "dx": 18},
            {"name": "CENTER KICK", "motion": "qcb", "pose_prep": {"base": "stance", "torso": 4},
             "pose_hit": {"base": "crouch", "torso": 28, "far_foot": [40, -4]}, "limb": "far_ankle", "dx": 20}
        ],
        "super_move": {"name": "WING CHUN FLURRY", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 10},
                       "pose_hit": {"base": "stance", "torso": 24, "near_fist": [38, 0]}, "limb": "near_fist", "dx": 32},
        "cpu": {"aggression": 84, "range": 46, "jump": 22, "poke": "CHAIN PUNCH", "sweep": "CENTER KICK", "anti_air": "PAK SAO PARRY", "special": "CHAIN PUNCH", "rush": "CHAIN PUNCH", "throw": "THROW", "super": "WING CHUN FLURRY"}
    },

    # 40 Zero (Chimera Test Subject) - Hidden 1
    {
        "fid": "zero", "name": "ZERO", "origin": "UNKNOWN",
        "style": "CONDITIONED HYBRID", "discipline": "UNSTABLE", "stage": "CHIMERA FACILITY",
        "life": 1000, "physics": {"walk_fwd": 2.2, "walk_back": 1.7, "dash": 5.4, "backdash": [4.2, 4.3], "jump": [3.3, 10.5], "gravity": 0.54},
        "push": [16, 90, 58], "close_range": 46, "guard_max": 100,
        "build": b_standard(scale=1.0, heaviness=0.98),
        "materials": {"thigh": "A", "shin": "A", "foot": "skin", "torso": "A", "upper_arm": "skin", "forearm": "skin", "fist": "skin",
                      "trunks": 0.6, "shorts": "A", "waistband": "B", "hair": "C", "hair_style": "spiky", "muscle": True},
        "palette": {"skin": [[130, 80, 56], [186, 128, 96], [226, 172, 138]],
                    "A": [[200, 196, 190], [226, 224, 220], [250, 250, 250]],
                    "B": [[150, 20, 24], [220, 40, 44], [254, 96, 92]],
                    "C": [[20, 20, 24], [38, 38, 46], [68, 68, 80]]},
        "alt_palette": {"A": [[36, 40, 50], [60, 68, 84], [100, 114, 140]], "B": [[40, 140, 220], [80, 190, 254], [160, 230, 254]]},
        "specials": [
            {"name": "FRACTURED STRIKE", "motion": "qcf", "pose_prep": {"base": "stance", "torso": -6},
             "pose_hit": {"base": "stance", "torso": 32, "near_fist": [40, 2]}, "limb": "near_fist", "dx": 32},
            {"name": "MEMORY SURGE", "motion": "dp", "pose_prep": {"base": "crouch", "torso": 10},
             "pose_hit": {"base": "jump", "torso": -16, "near_fist": [28, -28]}, "limb": "near_fist", "dx": 20, "dy": 26, "anim_flags": "active|airborne"},
            {"name": "UNSTABLE WARP", "motion": "qcb", "pose_prep": {"base": "crouch", "torso": 24},
             "pose_hit": {"base": "stance", "torso": 36, "far_foot": [44, -10]}, "limb": "far_ankle", "dx": 34}
        ],
        "super_move": {"name": "PROJECT OVERLOAD", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": -10},
                       "pose_hit": {"base": "stance", "torso": 40, "near_fist": [44, 0]}, "limb": "near_fist", "dx": 38},
        "cpu": {"aggression": 88, "range": 50, "jump": 26, "poke": "FRACTURED STRIKE", "sweep": "SWEEP", "anti_air": "MEMORY SURGE", "special": "FRACTURED STRIKE", "rush": "UNSTABLE WARP", "throw": "THROW", "super": "PROJECT OVERLOAD"}
    },

    # 41 Echo (Synthetic Lifeform) - Hidden 2
    {
        "fid": "echo", "name": "ECHO", "origin": "PROJECT CHIMERA",
        "style": "IMITATION MATRIX", "discipline": "SYNTHETIC", "stage": "CHIMERA FACILITY",
        "life": 1000, "physics": {"walk_fwd": 2.3, "walk_back": 1.8, "dash": 5.4, "backdash": [4.3, 4.3], "jump": [3.4, 10.6], "gravity": 0.52},
        "push": [16, 90, 58], "close_range": 46, "guard_max": 100,
        "build": b_standard(scale=1.0, heaviness=0.96),
        "materials": {"thigh": "A", "shin": "A", "foot": "B", "torso": "A", "upper_arm": "A", "forearm": "B", "fist": "B",
                      "armor": "A", "hair": "C", "hair_style": "visor"},
        "palette": {"skin": [[30, 40, 60], [54, 72, 104], [94, 124, 172]],
                    "A": [[24, 28, 40], [44, 52, 74], [78, 92, 128]],
                    "B": [[20, 120, 200], [50, 180, 244], [140, 230, 254]],
                    "C": [[180, 140, 20], [236, 196, 40], [254, 238, 110]]},
        "alt_palette": {"A": [[36, 24, 48], [68, 44, 90], [118, 78, 154]], "B": [[180, 40, 120], [236, 80, 180], [254, 160, 230]]},
        "specials": [
            {"name": "MIMIC WAVE", "motion": "qcf", "pose_prep": {"base": "stance", "torso": 14},
             "pose_hit": {"base": "stance", "torso": 26, "near_fist": [40, 0]}, "limb": "near_fist", "dx": 30},
            {"name": "ECHO UPPER", "motion": "dp", "pose_prep": {"base": "crouch", "torso": 10},
             "pose_hit": {"base": "jump", "torso": -12, "near_fist": [28, -28]}, "limb": "near_fist", "dx": 22, "dy": 28, "anim_flags": "active|airborne"},
            {"name": "WARP DRIVE", "motion": "qcb", "pose_prep": {"base": "crouch", "torso": 28},
             "pose_hit": {"base": "crouch", "torso": 50, "far_foot": [46, -2]}, "limb": "far_ankle", "dx": 32}
        ],
        "super_move": {"name": "SYNTHETIC REFLECTION", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 10},
                       "pose_hit": {"base": "stance", "torso": 32, "near_fist": [44, 0]}, "limb": "near_fist", "dx": 36},
        "cpu": {"aggression": 85, "range": 55, "jump": 30, "poke": "MIMIC WAVE", "sweep": "WARP DRIVE", "anti_air": "ECHO UPPER", "special": "MIMIC WAVE", "rush": "MIMIC WAVE", "throw": "THROW", "super": "SYNTHETIC REFLECTION"}
    },

    # 43 Viper (Humanoid Droid, Speed) - Droid 02
    {
        "fid": "viper", "name": "VIPER", "origin": "PROJECT CHIMERA",
        "style": "COMBAT DROID SPEED", "discipline": "CYBERNETIC", "stage": "CHIMERA FACILITY",
        "life": 980, "physics": {"walk_fwd": 2.5, "walk_back": 2.0, "dash": 5.8, "backdash": [4.6, 4.5], "jump": [3.6, 10.8], "gravity": 0.50},
        "push": [15, 86, 54], "close_range": 46, "guard_max": 95,
        "build": b_standard(scale=0.98, heaviness=0.92),
        "materials": {"thigh": "A", "shin": "A", "foot": "B", "torso": "A", "upper_arm": "A", "forearm": "B", "fist": "B",
                      "armor": "A", "shin_split": 0.55, "boot": "B", "hair": "C", "hair_style": "visor"},
        "palette": {"skin": [[28, 32, 44], [48, 56, 76], [84, 98, 130]],
                    "A": [[130, 20, 26], [198, 38, 44], [244, 88, 88]],
                    "B": [[22, 24, 30], [46, 50, 62], [84, 90, 110]],
                    "C": [[20, 140, 220], [60, 196, 254], [150, 236, 254]]},
        "alt_palette": {"A": [[20, 36, 96], [38, 70, 172], [76, 120, 240]], "B": [[130, 20, 26], [198, 38, 44], [244, 88, 88]]},
        "specials": [
            {"name": "VIPER FLASH", "motion": "qcf", "pose_prep": {"base": "stance", "torso": -10},
             "pose_hit": {"base": "stance", "torso": -24, "far_foot": [48, -44]}, "limb": "far_ankle", "dx": 32},
            {"name": "CYBER SPIRAL", "motion": "dp", "pose_prep": {"base": "crouch", "torso": 12},
             "pose_hit": {"base": "jump", "torso": 6, "near_foot": [34, -54]}, "limb": "near_ankle", "dx": 22, "dy": 30, "anim_flags": "active|airborne"},
            {"name": "PREDICTIVE EVADE", "motion": "qcb", "pose_prep": {"base": "crouch", "torso": 24},
             "pose_hit": {"base": "stance", "torso": 18, "far_foot": [38, -6]}, "limb": "far_ankle", "dx": 36}
        ],
        "super_move": {"name": "ALGORITHM VELOCITY", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": -14},
                       "pose_hit": {"base": "stance", "torso": -30, "far_foot": [52, -50]}, "limb": "far_ankle", "dx": 38},
        "cpu": {"aggression": 90, "range": 62, "jump": 36, "poke": "VIPER FLASH", "sweep": "SWEEP", "anti_air": "CYBER SPIRAL", "special": "VIPER FLASH", "rush": "VIPER FLASH", "throw": "THROW", "super": "ALGORITHM VELOCITY"}
    },

    # 44 Mirage (Humanoid Droid, Adaptation) - Droid 03
    {
        "fid": "mirage", "name": "MIRAGE", "origin": "PROJECT CHIMERA",
        "style": "ADAPTIVE COMBAT", "discipline": "CYBERNETIC", "stage": "CHIMERA FACILITY",
        "life": 1020, "physics": {"walk_fwd": 2.3, "walk_back": 1.8, "dash": 5.4, "backdash": [4.3, 4.3], "jump": [3.4, 10.6], "gravity": 0.52},
        "push": [16, 90, 58], "close_range": 48, "guard_max": 105,
        "build": b_standard(scale=1.01, heaviness=0.98),
        "materials": {"thigh": "A", "shin": "A", "foot": "B", "torso": "A", "upper_arm": "A", "forearm": "B", "fist": "B",
                      "armor": "A", "shin_split": 0.55, "boot": "B", "hair": "C", "hair_style": "visor"},
        "palette": {"skin": [[32, 38, 52], [54, 66, 90], [92, 114, 150]],
                    "A": [[16, 88, 76], [32, 148, 130], [80, 214, 192]],
                    "B": [[24, 28, 36], [48, 54, 70], [86, 96, 122]],
                    "C": [[200, 160, 40], [244, 208, 70], [254, 244, 140]]},
        "alt_palette": {"A": [[88, 24, 80], [148, 48, 138], [214, 98, 202]], "B": [[24, 28, 36], [48, 54, 70], [86, 96, 122]]},
        "specials": [
            {"name": "ADAPTIVE BLADE", "motion": "qcf", "pose_prep": {"base": "stance", "torso": 16},
             "pose_hit": {"base": "stance", "torso": 28, "near_fist": [42, -2]}, "limb": "near_fist", "dx": 32},
            {"name": "OPTICAL REFLECTION", "motion": "qcb", "pose_prep": {"base": "guard", "torso": -6},
             "pose_hit": {"base": "stance", "torso": 26, "far_fist": [36, 0]}, "limb": "far_fist", "dx": 22},
            {"name": "NEURAL LEAP", "motion": "dp", "pose_prep": {"base": "crouch", "torso": 12},
             "pose_hit": {"base": "jump", "torso": -14, "near_fist": [28, -30]}, "limb": "near_fist", "dx": 20, "dy": 28, "anim_flags": "active|airborne"}
        ],
        "super_move": {"name": "CHIMERA SIMULATION", "motion": "dqcf", "pose_prep": {"base": "stance", "torso": 12},
                       "pose_hit": {"base": "stance", "torso": 34, "near_fist": [44, 0]}, "limb": "near_fist", "dx": 38},
        "cpu": {"aggression": 86, "range": 58, "jump": 32, "poke": "ADAPTIVE BLADE", "sweep": "SWEEP", "anti_air": "NEURAL LEAP", "special": "ADAPTIVE BLADE", "rush": "ADAPTIVE BLADE", "throw": "THROW", "super": "CHIMERA SIMULATION"}
    }
]

def main():
    print(f"Generating {len(ROSTER_DATA)} fighter definitions...")
    for item in ROSTER_DATA:
        f = make_fighter(
            fid=item["fid"], name=item["name"], origin=item["origin"],
            style=item["style"], discipline=item["discipline"], stage=item["stage"],
            life=item["life"], physics=item["physics"], push=item["push"],
            close_range=item["close_range"], guard_max=item["guard_max"],
            build=item["build"], materials=item["materials"], palette=item["palette"],
            alt_palette=item["alt_palette"], specials=item["specials"],
            super_move=item["super_move"], throws=[], cpu=item["cpu"]
        )
        out_path = F_DIR / f"{item['fid']}.json"
        out_path.write_text(json.dumps(f, indent=2))
        print(f"  [+] Wrote {out_path.name}")
    print(f"All {len(ROSTER_DATA)} fighters successfully written!")

if __name__ == "__main__":
    main()
