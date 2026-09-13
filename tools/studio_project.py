"""Project and document contracts shared by the desktop authoring tools."""

from dataclasses import dataclass
import hashlib
from pathlib import Path
import re
import sys


def assignments(path):
    """Read literal project settings without evaluating make recipes or shell code."""
    values = {}
    if not path.exists():
        return values
    for line in path.read_text(encoding="utf-8-sig").splitlines():
        match = re.match(r"^\s*([A-Z_]+)\s*([?:]?=)\s*(.*?)\s*(?:#.*)?$", line)
        if match:
            key, operator, value = match.groups()
            if operator != "?=" or key not in values:
                values[key] = value.strip()
    return values


def game_names(root):
    return sorted(p.parent.name for p in (Path(root) / "games").glob("*/game.mk"))


@dataclass(frozen=True)
class StudioProject:
    root: Path
    game: str
    game_id: str
    art_owner: str
    sound_owner: str

    @classmethod
    def open(cls, root, game=None):
        root = Path(root).resolve()
        game = game or assignments(root / "game.cfg").get("CURRENT_GAME", "demo")
        if game not in game_names(root):
            raise ValueError(f"Unknown game {game!r} in {root / 'games'}")
        config = assignments(root / "games" / game / "game.mk")
        game_id = config.get("GAME_ID", "")
        if not game_id.isdecimal():
            raise ValueError("Studio requires a literal numeric GAME_ID in game.mk")
        art = config.get("GAME_ART_FROM") or game
        sound = config.get("GAME_SOUND_FROM") or game
        for name in (art, sound):
            if name not in game_names(root):
                raise ValueError(f"Unknown asset source {name!r}; use a literal game name")
        return cls(root, game, game_id, art, sound)

    @property
    def art_data(self):
        return self.root / "games" / self.game / "artbox"

    @property
    def art_source(self):
        return self.root / "games" / self.art_owner / "artbox"

    @property
    def sound(self):
        return self.root / "games" / self.sound_owner / "sound"

    @property
    def roms(self):
        return self.root / "roms" / self.game

    @property
    def sound_dir(self):
        """Active sound directory, falling back to repo root sound/ if empty."""
        return self.sound if self.sound.exists() else self.root / "sound"

    def c_rom_paths(self):
        """Return (c1_path, c2_path) for the project, checking game artbox then roms then demo fallback."""
        candidates = [
            (self.art_data / f"{self.game_id}-c1.c1", self.art_data / f"{self.game_id}-c2.c2"),
            (self.art_data / "777-c1.c1", self.art_data / "777-c2.c2"),
            (self.art_data / "1c.c1", self.art_data / "2c.c2"),
            (self.roms / f"{self.game_id}-c1.c1", self.roms / f"{self.game_id}-c2.c2"),
            (self.root / "games" / "demo" / "artbox" / "777-c1.c1", self.root / "games" / "demo" / "artbox" / "777-c2.c2"),
        ]
        for c1, c2 in candidates:
            if c1.exists() and c2.exists():
                return c1, c2
        return candidates[0]

    def manifest_path(self):
        for p in (self.art_data / "assets_manifest.json",
                  self.art_source / "assets_manifest.json",
                  self.root / "games" / "demo" / "artbox" / "assets_manifest.json"):
            if p.exists():
                return p
        return self.art_data / "assets_manifest.json"

    def neopal_path(self):
        for p in (self.art_data / "neopal.bin",
                  self.art_source / "neopal.bin",
                  self.root / "games" / "demo" / "artbox" / "neopal.bin"):
            if p.exists():
                return p
        return self.art_data / "neopal.bin"

    def patches_path(self):
        for p in (self.sound_dir / "fm" / "patches.fm",
                  self.root / "sound" / "fm" / "patches.fm"):
            if p.exists():
                return p
        return self.sound_dir / "fm" / "patches.fm"

    def ssg_config_path(self):
        for p in (self.sound_dir / "ssg" / "config.ssg",
                  self.root / "sound" / "ssg" / "config.ssg"):
            if p.exists():
                return p
        return self.sound_dir / "ssg" / "config.ssg"

    def samples_dir(self, channel="a"):
        name = f"in_wav_{channel.lower()}"
        for d in (self.sound_dir / "samples" / name,
                  self.root / "sound" / "samples" / name):
            if d.exists():
                return d
        return self.sound_dir / "samples" / name

    def mml_dir(self):
        for d in (self.sound_dir / "mml",
                  self.sound_dir / "fm",
                  self.root / "sound" / "mml",
                  self.root / "sound" / "fm"):
            if d.exists():
                return d
        return self.sound_dir / "mml"

    def make_command(self, target, windows=None):
        windows = sys.platform == "win32" if windows is None else windows
        allowed = {"art", "sfix", "sound", "samples", "vrom", "fm", "fmpatches",
                   "mml", "ssg", "ssgconfig", "m1rom"}
        if target not in allowed:
            raise ValueError(f"Unsupported studio build target: {target}")
        return ["make", "-f", "MakefileWin32.mak" if windows else "Makefile",
                f"GAME={self.game}", target]


class FileSnapshot:
    """Detect stale editors before saving over work changed outside the studio."""

    def __init__(self, path):
        self.path = Path(path)
        self.digest = self.fingerprint()

    def fingerprint(self):
        return hashlib.sha256(self.path.read_bytes()).digest() if self.path.exists() else None

    def check(self):
        if self.fingerprint() != self.digest:
            raise ValueError(f"Changed on disk: {self.path}\nReload or export a separate copy.")

    def refresh(self):
        self.digest = self.fingerprint()
