#!/usr/bin/env python3
"""
The ROM release registry: every build of every release file in dist/release/,
from the working tree and from all of git history, with the hashes of each
archive and of each ROM inside it. Each build is checked against the GitHub
release assets, the MAME definition shipped in its own set, the software
list in hash_eagle/, and the HBMAME and FinalBurn Neo entries for the game.

releases/rom-registry.json is the one source of truth; everything else is
generated from it:

  releases/rom-registry.json        every build, its files and hashes
  releases/SHA256SUMS.txt           current dist/release files (sha256sum -c)
  releases/SHA1SUMS.txt             current dist/release files (sha1sum -c)
  releases/CRC32SUMS.txt            current dist/release files
  releases/ROM_COMPONENTS.tsv       each ROM inside each current package
  docs/qa/ROM_INTEGRITY_REPORT.md   findings

    python3 tools/rom_registry.py              # rebuild registry and report
    python3 tools/rom_registry.py --offline    # use the cached GitHub asset list
    python3 tools/rom_registry.py --verify     # check dist/release against the registry
    python3 tools/rom_registry.py --external hbmame=<neogeohb.cpp> fbneo=<d_neogeo.cpp>

--external re-reads the emulator sources (HBMAME's neogeohb.cpp, FinalBurn
Neo's d_neogeo.cpp) into releases/external/*.json; without it the stored
snapshots are used. Nothing here edits a release file.
"""

import argparse
import datetime
import hashlib
import io
import json
import re
import subprocess
import sys
import xml.etree.ElementTree as ET
import zipfile
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DIST = ROOT / "dist" / "release"
OUT = ROOT / "releases"
EXT = OUT / "external"
REPORT = ROOT / "docs" / "qa" / "ROM_INTEGRITY_REPORT.md"
REPO = "eaglesoftware777/neogeosdk"

# Release prefix -> game folder (games/, hash_eagle/), MAME software list
# name, and the set names the emulator databases use.
GAMES = {
    "Maiya": ("maiya", "Maiya: Super Nature Girl", {"hbmame": "maiya", "fbneo": "maiya"}),
    "MaiyaLuna": ("maiyaluna", "Maiya & Luna", {"hbmame": "maiyaluna"}),
    "NoSignal": ("nosignal", "No Signal Z Alpha", {"hbmame": "nosignal"}),
    "ShadowLeague": ("shadowleague", "Shadow League Fighting", {"hbmame": "shadowlf", "fbneo": "shadowlf"}),
    "SevenWonders": ("sevenwonders", "Seven Wonders Puzzles", {"hbmame": "7wonders"}),
    "MahjongDemo": ("mahjong_demo", "Mahjong Demo", {"hbmame": "mahjongd"}),
    "SkyLance": ("skylance", "Sky Lance", {"hbmame": "skylance", "fbneo": "skylance"}),
    "EagleBIOS": ("bios", "EagleBIOS", {}),
}
# The SDK demo has no dist/release files; it is checked against the
# databases through the software list built from the tree.
EXTRA_SETS = {"demo": ("SDK Demo", {"hbmame": "sdkdemo"})}

KINDS = {
    "WIP-EagleBIOS": "mame-mvs",
    "WIP-AES-EagleBIOS": "mame-aes",
    "WIP-NeoSD_MVS": "neosd-mvs",
    "WIP-NeoSD_AES": "neosd-aes",
    "Darksoft-MVS": "darksoft-mvs",
    "Darksoft-AES": "darksoft-aes",
    "BackBit-MVS": "backbit-mvs",
    "BackBit-AES": "backbit-aes",
    "NeoGeoCD": "neogeocd",
}
DOC_KINDS = ("MANIFEST", "README", "CHANGELOG", "SHA256", "NeoGeoCD-README", "NeoGeoCD-SHA256")
NAME_RE = re.compile(r"^(?P<prefix>[A-Za-z]+)-(?P<kind>.+?)_(?P<label>v\d+(?:\.\d+){0,2}[A-Za-z0-9_]*)\.(?P<ext>zip|neo|txt|json)$")
PARTS = ("p1", "p2", "s1", "m1", "v1", "v2", "c1", "c2", "c3", "c4")


def git(*args, data=None):
    return subprocess.run(["git", "-C", str(ROOT), *args], input=data, capture_output=True, check=True).stdout


def hashes(data):
    return {
        "size": len(data),
        "crc32": f"{zlib.crc32(data) & 0xffffffff:08x}",
        "sha1": hashlib.sha1(data).hexdigest(),
        "sha256": hashlib.sha256(data).hexdigest(),
    }


def semver(label):
    """v4 -> 4.0.0, v2.0.1 -> 2.0.1, v3_TEST -> 3.0.0-test, v3_RC2_TEST -> 3.0.0-rc2.test"""
    m = re.match(r"v(\d+(?:\.\d+){0,2})(?:_(.+))?$", label)
    base = ".".join((m.group(1).split(".") + ["0", "0"])[:3])
    return base + ("-" + m.group(2).lower().replace("_", ".") if m.group(2) else "")


def rom_part(name):
    ext = name.rsplit(".", 1)[-1].lower()
    return ext if ext in PARTS else None


def inspect_package(name, data):
    """Hashes of the archive and, for a zip, of every file inside it."""
    pkg = {"archive": hashes(data), "files": [], "mame_definition": None, "build_timestamp": None}
    if not name.endswith(".zip"):
        return pkg
    try:
        z = zipfile.ZipFile(io.BytesIO(data))
        bad = z.testzip()
    except zipfile.BadZipFile as e:
        pkg["zip_error"] = str(e)
        return pkg
    if bad:
        pkg["zip_error"] = f"CRC error in {bad}"
    stamps = []
    hash_xml = None
    for info in z.infolist():
        if info.is_dir():
            continue
        body = z.read(info)
        entry = {"name": info.filename, **hashes(body)}
        part = rom_part(info.filename)
        if part and "/neogeo/" not in info.filename and "/aes/" not in info.filename:
            entry["part"] = part
        pkg["files"].append(entry)
        stamps.append(info.date_time)
        if info.filename.endswith("hash/neogeo.xml"):
            hash_xml = body
    if stamps:
        pkg["build_timestamp"] = datetime.datetime(*max(stamps)).isoformat()
    if hash_xml is not None:
        pkg["mame_definition"] = check_definition(hash_xml, pkg["files"])
    return pkg


def parse_softlist(xml_bytes):
    """{software name: {rom name: (size, crc, sha1)}} from a MAME software list."""
    out = {}
    root = ET.fromstring(xml_bytes)
    for sw in root.iter("software"):
        roms = {}
        for rom in sw.iter("rom"):
            if rom.get("name"):
                roms[rom.get("name")] = (int(rom.get("size"), 0), (rom.get("crc") or "").lower(), (rom.get("sha1") or "").lower())
        out[sw.get("name")] = roms
    return out


def check_definition(xml_bytes, files):
    """Does the set's own hash/neogeo.xml describe the ROMs packed beside it?"""
    try:
        lists = parse_softlist(xml_bytes)
    except ET.ParseError as e:
        return {"ok": False, "problems": [f"hash/neogeo.xml does not parse: {e}"]}
    by_name = {f["name"].rsplit("/", 1)[-1]: f for f in files if f.get("part")}
    problems = []
    for sw, roms in lists.items():
        for rname, (size, crc, sha1) in roms.items():
            f = by_name.get(rname)
            if f is None:
                problems.append(f"{sw}: {rname} listed but not packed")
            elif (f["size"], f["crc32"], f["sha1"]) != (size, crc, sha1):
                problems.append(f"{sw}: {rname} differs from its definition")
    return {"ok": not problems, "software": sorted(lists), "problems": problems}


def build_id(pkg):
    """The identity of a build: the hashes of its ROM parts, ignoring names."""
    parts = sorted((f["part"], f["sha256"]) for f in pkg["files"] if f.get("part"))
    if not parts:
        return pkg["archive"]["sha256"][:12]
    return hashlib.sha256("".join(f"{p}:{h}\n" for p, h in parts).encode()).hexdigest()[:12]


# --- emulator databases -------------------------------------------------------

MACRO_SIZES = {"64K": 0x10000, "128K": 0x20000, "256K": 0x40000, "512K": 0x80000}


def parse_hbmame(text):
    sets = {}
    for m in re.finditer(r"ROM_START\(\s*(\w+)\s*\)(.*?)ROM_END", text, re.S):
        name, body = m.group(1), m.group(2)
        roms = []
        for line in body.splitlines():
            r = re.search(r'(\w+)\(\s*"([^"]+)"(.*?)CRC\(([0-9a-fA-F]{8})\)\s*SHA1\(([0-9a-fA-F]{40})\)', line)
            if not r:
                continue
            macro, fname, args = r.group(1), r.group(2), r.group(3)
            sizes = [t.strip() for t in args.split(",") if t.strip()]
            if len(sizes) >= 2:
                size = int(sizes[1], 0)
            else:
                size = next((v for k, v in MACRO_SIZES.items() if macro.endswith(k)), None)
            roms.append({"name": fname, "part": rom_part(fname), "size": size, "crc32": r.group(4).lower(), "sha1": r.group(5).lower()})
        sets[name] = {"roms": roms, "block": m.group(0)}
    for m in re.finditer(r'GAME\(\s*\d+,\s*(\w+),.*?"Eagle Software",\s*"([^"]+)".*', text):
        if m.group(1) in sets:
            sets[m.group(1)]["title"] = m.group(2)
            sets[m.group(1)]["game_line"] = m.group(0)
    return {k: v for k, v in sets.items() if "title" in v}


def parse_fbneo(text):
    sets = {}
    for m in re.finditer(r"static struct BurnRomInfo (\w+)RomDesc\[\] = \{(.*?)\};", text, re.S):
        roms = []
        for r in re.finditer(r'\{\s*"([^"]+)",\s*(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+)', m.group(2)):
            roms.append({"name": r.group(1), "part": rom_part(r.group(1)), "size": int(r.group(2), 16), "crc32": f"{int(r.group(3), 16):08x}"})
        sets[m.group(1)] = {"roms": roms, "block": m.group(0)}
    for m in re.finditer(r'struct BurnDriver BurnDrv\w+ = \{\s*"(\w+)",[^\n]*\n\s*"([^"]+?)(?:\\0)?",\s*NULL,\s*"Eagle Software"', text):
        if m.group(1) in sets:
            sets[m.group(1)]["title"] = m.group(2)
    return {k: v for k, v in sets.items() if "title" in v}


def load_external(refresh):
    EXT.mkdir(parents=True, exist_ok=True)
    sources = {"hbmame": ("src/hbmame/drivers/neogeohb.cpp", parse_hbmame), "fbneo": ("src/burn/drv/neogeo/d_neogeo.cpp", parse_fbneo)}
    out = {}
    for db, (src, parse) in sources.items():
        path = EXT / f"{db}.json"
        if db in refresh:
            sets = parse(Path(refresh[db]).read_text(errors="replace"))
            path.write_text(json.dumps({"database": db, "source_file": src, "read": datetime.date.today().isoformat(), "sets": sets}, indent=1) + "\n")
        out[db] = json.loads(path.read_text()) if path.exists() else {"sets": {}}
    return out


def match_external(entry, builds):
    """The builds whose ROM parts carry the database's sizes and CRCs."""
    want = {r["part"]: r for r in entry["roms"] if r["part"]}
    results = []
    for b in builds:
        have = {f["part"]: f for f in b["files"] if f.get("part")}
        same = [p for p, r in want.items() if p in have and have[p]["crc32"] == r["crc32"] and (r["size"] is None or have[p]["size"] == r["size"])
                and ("sha1" not in r or have[p]["sha1"] == r["sha1"])]
        if same:
            results.append((len(same) == len(want), sorted(same), b))
    return results


# --- collection ---------------------------------------------------------------

def github_assets(offline):
    cache = EXT / "github-assets.json"
    if not offline:
        try:
            raw = subprocess.run(["gh", "api", f"repos/{REPO}/releases?per_page=100", "--paginate"], capture_output=True, check=True, timeout=180).stdout
            rel = json.loads(raw)
            releases = sorted(({"tag": r["tag_name"], "published": r["published_at"], "prerelease": r["prerelease"]} for r in rel if r.get("published_at")),
                              key=lambda r: r["published"])
            assets = [{"tag": r["tag_name"], "prerelease": r["prerelease"], "name": a["name"], "size": a["size"],
                       "sha256": (a.get("digest") or "").removeprefix("sha256:") or None, "updated": a["updated_at"]}
                      for r in rel for a in r["assets"]]
            cache.write_text(json.dumps({"read": datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="seconds"),
                                         "releases": releases, "assets": assets}, indent=1) + "\n")
        except (subprocess.SubprocessError, OSError, json.JSONDecodeError) as e:
            print(f"GitHub asset list not refreshed ({e}); using the cached one", file=sys.stderr)
    return json.loads(cache.read_text()) if cache.exists() else {"read": None, "releases": [], "assets": []}


def history():
    """Every content each dist/release path ever had: [(path, blob, commit, date, old)], oldest first.

    --all also walks branches left on the history from before the 2026-10-27
    rewrite. A content is credited to its first commit on the current history
    (HEAD, tags, remote branches); old marks one found only on the old history.
    """
    current = set(git("rev-list", "HEAD", "--tags", "--remotes", "--", "dist/release").decode().split())
    log = git("log", "--all", "--reverse", "--format=C %H %cI", "--raw", "--no-abbrev", "--no-renames", "--", "dist/release").decode()
    first, commit, date = {}, None, None
    for line in log.splitlines():
        if line.startswith("C "):
            _, commit, date = line.split(" ", 2)
        elif line.startswith(":"):
            meta, path = line.split("\t", 1)
            blob = meta.split()[3]
            if set(blob) == {"0"}:
                continue
            key = (path, blob)
            if key not in first or (commit in current and first[key][2]):
                first[key] = (commit, date, commit not in current)
    rows = [(p, b, c, d, old) for (p, b), (c, d, old) in first.items()]
    return sorted(rows, key=lambda r: (r[3], r[0]))


def read_blobs(blobs):
    """Stream blob contents through one git cat-file process."""
    proc = subprocess.Popen(["git", "-C", str(ROOT), "cat-file", "--batch"], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    for blob in blobs:
        proc.stdin.write(blob.encode() + b"\n")
        proc.stdin.flush()
        header = proc.stdout.readline().split()
        if len(header) < 3:
            raise SystemExit(f"git has no object {blob}")
        size = int(header[2])
        data = proc.stdout.read(size)
        proc.stdout.read(1)
        yield blob, data
    proc.stdin.close()
    proc.wait()


def sdk_for(date, releases):
    """The newest SDK release published on GitHub before a build's commit.

    Tags cannot answer this: the 2026-10-27 rewrite moved v1.7.7 to the new
    tip, so `git describe` credits builds made after its release to v1.7.5.
    """
    when = datetime.datetime.fromisoformat(date)
    best = None
    for r in releases:
        if re.fullmatch(r"v\d+(\.\d+)*", r["tag"]) and datetime.datetime.fromisoformat(r["published"].replace("Z", "+00:00")) <= when:
            best = r["tag"]
    return best.lstrip("v") if best else None


def commit_cell(r):
    if r["commit_date"] is None:
        return "not in git"
    cell = r["git_commit"][:10]
    if r["only_in_pre_rewrite_history"]:
        cell += " (pre-rewrite history only)"
    if not r["tracked"]:
        cell += ", file now ignored"
    return cell


def collect(offline, refresh):
    external = load_external(refresh)
    gh = github_assets(offline)
    tracked = set(git("ls-files", "-z", "dist/release").decode().split("\0")) - {""}
    rows = history()
    # Inspect each blob as it streams in: the history is GBs, only hashes are kept.
    blob_name = {}
    for path, blob, *_ in rows:
        blob_name.setdefault(blob, path.rsplit("/", 1)[-1])
    pkgs = {blob: inspect_package(blob_name[blob], data) for blob, data in read_blobs(sorted(blob_name))}
    print(f"{len(rows)} historical versions of {len({p for p, *_ in rows})} paths read from git", file=sys.stderr)

    current = {}
    for f in sorted(DIST.iterdir()):
        if f.is_file():
            current[f"dist/release/{f.name}"] = inspect_package(f.name, f.read_bytes())

    # One record per distinct content of each path.
    records = []
    by_path = {}
    for path, blob, commit, date, old in rows:
        by_path.setdefault(path, []).append({"commit": commit, "date": date, "old": old, "blob": blob, "pkg": pkgs[blob]})
    for path, pkg in current.items():
        versions = by_path.setdefault(path, [])
        if not any(v["pkg"]["archive"]["sha256"] == pkg["archive"]["sha256"] for v in versions):
            versions.append({"commit": None, "date": None, "old": False, "blob": None, "pkg": pkg})

    pushed = set(git("rev-list", "--remotes", "--", "dist/release").decode().split())
    head = git("rev-parse", "HEAD").decode().strip()
    head_date = git("show", "-s", "--format=%cI", "HEAD").decode().strip()
    gh_by_sha = {}
    for a in gh["assets"]:
        if a["sha256"]:
            gh_by_sha.setdefault(a["sha256"], []).append(a)

    for path, versions in sorted(by_path.items()):
        fname = path.rsplit("/", 1)[-1]
        m = NAME_RE.match(fname)
        cur = current.get(path)
        cur_sha = cur["archive"]["sha256"] if cur is not None else None
        for n, v in enumerate(versions, 1):
            pkg = v["pkg"]
            sha = pkg["archive"]["sha256"]
            status = "current" if sha == cur_sha else ("removed" if cur is None else "replaced")
            rec = {
                "path": path,
                "file": fname,
                "content_no": n,
                "contents_under_this_name": len(versions),
                "status": status,
                "tracked": path in tracked,
                "git_commit": v["commit"] or f"working tree (HEAD {head})",
                "only_in_pre_rewrite_history": v["old"],
                "git_blob": v["blob"],
                "pushed": v["commit"] in pushed,
                "commit_date": v["date"],
                "sdk_version": sdk_for(v["date"] or head_date, gh.get("releases", [])),
                "build_timestamp": pkg["build_timestamp"],
                "archive": pkg["archive"],
                "files": pkg["files"],
                "mame_definition": pkg["mame_definition"],
                "published": [{"tag": a["tag"], "name": a["name"]} for a in gh_by_sha.get(sha, [])],
            }
            if pkg.get("zip_error"):
                rec["zip_error"] = pkg["zip_error"]
            if m:
                prefix, kind, label = m.group("prefix"), m.group("kind"), m.group("label")
                game = GAMES.get(prefix)
                rec.update({
                    "prefix": prefix,
                    "game": game[0] if game else prefix.lower(),
                    "title": game[1] if game else prefix,
                    "version": label,
                    "semver": semver(label),
                    "target": KINDS.get(kind, "document" if kind in DOC_KINDS else kind.lower()),
                    "romset": game[0] if game else None,
                    "external_names": game[2] if game else {},
                })
            rec["build_id"] = build_id(pkg)
            rec["game_id"] = next((re.match(r"(\d+)-", f["name"].rsplit("/", 1)[-1]).group(1) for f in rec["files"]
                                   if f.get("part") and re.match(r"\d+-", f["name"].rsplit("/", 1)[-1])), None)
            records.append(rec)

    # The software list built from the tree, for sets with no release files.
    tree_lists = {}
    for d in sorted((ROOT / "hash_eagle").iterdir()):
        x = d / "neogeo.xml"
        if x.is_file():
            tree_lists[d.name] = parse_softlist(x.read_bytes()).get(d.name, {})

    # Which builds an emulator database describes, part for part.
    for prefix, (game, _, names) in GAMES.items():
        builds = [r for r in records if r.get("game") == game and r.get("target") == "mame-mvs"]
        for db, setname in names.items():
            entry = external.get(db, {}).get("sets", {}).get(setname)
            for full, _, b in match_external(entry, builds) if entry else ():
                if full:
                    b.setdefault("external_matches", []).append(f"{db}:{setname}")
    for r in records:
        r.setdefault("external_matches", [])

    return {"external": external, "github": gh, "records": records, "tree_lists": tree_lists, "head": head}


# --- output -------------------------------------------------------------------

def registry_json(c):
    return {
        "schema": 1,
        "generated_from_commit": c["head"],
        "github_assets_read": c["github"]["read"],
        "note": "Built by tools/rom_registry.py from dist/release and its git history. Every hash is computed from the bytes; "
                "a build is identified by build_id, the hash of its ROM parts.",
        "builds": c["records"],
    }


def write_sums(c):
    cur = sorted((r for r in c["records"] if r["status"] == "current"), key=lambda r: r["file"])
    (OUT / "SHA256SUMS.txt").write_text("".join(f"{r['archive']['sha256']}  {r['path']}\n" for r in cur))
    (OUT / "SHA1SUMS.txt").write_text("".join(f"{r['archive']['sha1']}  {r['path']}\n" for r in cur))
    (OUT / "CRC32SUMS.txt").write_text("".join(f"{r['archive']['crc32']}  {r['archive']['size']:>10}  {r['path']}\n" for r in cur))
    lines = ["archive\tentry\tsize\tcrc32\tsha1\tsha256\n"]
    for r in cur:
        for f in r["files"]:
            lines.append(f"{r['file']}\t{f['name']}\t{f['size']}\t{f['crc32']}\t{f['sha1']}\t{f['sha256']}\n")
    (OUT / "ROM_COMPONENTS.tsv").write_text("".join(lines))


def manifest_problems(c):
    """Current MANIFEST/SHA256 text files against the files they list."""
    cur = {r["file"]: r for r in c["records"] if r["status"] == "current"}
    problems = []
    for name, r in sorted(cur.items()):
        if r.get("target") != "document" or not re.search(r"MANIFEST|SHA256", name):
            continue
        text = (DIST / name).read_text(errors="replace")
        for line in text.splitlines():
            parts = line.split()
            if line.startswith("#") or len(parts) < 2:
                continue
            listed = parts[-1].lstrip("*")
            sha = next((p for p in parts if re.fullmatch(r"[0-9a-f]{64}", p)), None)
            if not sha or listed not in cur:
                if sha and listed not in cur:
                    problems.append(f"{name}: lists {listed}, which is not in dist/release")
                continue
            if cur[listed]["archive"]["sha256"] != sha:
                problems.append(f"{name}: {listed} sha256 {sha[:12]}... but the file is {cur[listed]['archive']['sha256'][:12]}...")
    return problems


def md_table(head, rows):
    out = ["| " + " | ".join(head) + " |", "|" + "---|" * len(head)]
    out += ["| " + " | ".join(str(x) for x in row) + " |" for row in rows]
    return "\n".join(out)


def write_report(c):
    recs = c["records"]
    games = [r for r in recs if r.get("prefix") in GAMES]
    mame = [r for r in games if r["target"] in ("mame-mvs", "mame-aes")]
    binaries = [r for r in games if r["target"] != "document"]
    L = []
    L.append("# ROM Integrity Report\n")
    L.append(f"Generated by `tools/rom_registry.py` from commit `{c['head'][:12]}`; GitHub asset list read {c['github']['read']}. "
             "Every hash below was computed from the files; nothing is copied from a filename or a manifest.\n")
    L.append("A **build** is one distinct set of ROM bytes, named by `build_id` (the first 12 hex digits of the SHA-256 over its ROM parts' "
             "SHA-256s). The same release name with more than one build means the name was reused for different binaries.\n")

    # 1. Current releases
    L.append("## 1. Current release sets (MAME, MVS)\n")
    rows = []
    for r in sorted((r for r in mame if r["status"] == "current" and r["target"] == "mame-mvs"), key=lambda r: (r["game"], r["semver"])):
        hist = [x for x in mame if x["path"] == r["path"]]
        pub = ", ".join(sorted({p["tag"] for p in r["published"]})) or "not published"
        rows.append([r["title"], r["version"], r["game_id"] or "", f"`{r['build_id']}`", len(hist), r["build_timestamp"] or "",
                     commit_cell(r), r["sdk_version"] or "", pub,
                     "ok" if (r["mame_definition"] or {}).get("ok") else "**MISMATCH**"])
    L.append(md_table(["Game", "Version", "ID", "Build", "Builds under this name", "Packed", "Commit", "SDK", "On GitHub as", "Own MAME definition"], rows))
    L.append("")

    # 2. Reused names
    L.append("## 2. Release names reused for different binaries\n")
    reused = {}
    for r in binaries:
        if r["contents_under_this_name"] > 1:
            reused.setdefault(r["file"], []).append(r)
    rows = []
    for name, rs in sorted(reused.items()):
        ids = []
        for r in rs:
            tag = "+".join(sorted({p["tag"] for p in r["published"]}))
            ids.append(f"`{r['build_id']}`" + (f" ({tag})" if tag else "") + (" current" if r["status"] == "current" else ""))
        rows.append([name, len(rs), (rs[0]["commit_date"] or "")[:10], (rs[-1]["commit_date"] or "working tree")[:10], "<br>".join(ids)])
    L.append(f"{len(reused)} release files had more than one content. Builds are listed oldest first; a tag in brackets means "
             "that exact content is attached to that GitHub release.\n")
    L.append(md_table(["File", "Contents", "First", "Last", "Builds"], rows))
    L.append("")

    # 3. GitHub assets
    L.append("## 3. GitHub release assets against local files\n")
    by_sha = {}
    for r in recs:
        by_sha.setdefault(r["archive"]["sha256"], []).append(r)
    rows, counts = [], {}
    for a in c["github"]["assets"]:
        hit = by_sha.get(a["sha256"] or "", [])
        if not a["sha256"]:
            st = "no digest on GitHub"
        elif any(h["status"] == "current" for h in hit):
            st = "matches the current file"
        elif hit:
            st = "matches an older content (" + ", ".join(sorted({f"{h['file']} #{h['content_no']}" for h in hit})) + ")"
        else:
            st = "**not in this repository**"
        key = st.split(" (")[0]
        counts[key] = counts.get(key, 0) + 1
        if not st.startswith("matches the current"):
            rows.append([a["tag"], a["name"], a["size"], st])
    L.append(md_table(["Result", "Assets"], sorted(counts.items(), key=lambda x: -x[1])))
    L.append("\nAssets that do not match the file now in `dist/release`:\n")
    L.append(md_table(["Tag", "Asset", "Size", "Local copy"], rows))
    L.append("")

    # 4. Manifests
    L.append("## 4. Manifests in dist/release\n")
    probs = manifest_problems(c)
    L.append("Every current MANIFEST and SHA256 file matches the files it lists.\n" if not probs else "\n".join(f"- {p}" for p in probs) + "\n")

    # 5. Definitions
    L.append("## 5. MAME definitions\n")
    bad = [r for r in mame if r["mame_definition"] and not r["mame_definition"]["ok"]]
    L.append(f"{len(mame)} MAME sets checked (all builds). Each set's `hash/neogeo.xml` against the ROMs packed with it: "
             + ("all match.\n" if not bad else f"{len(bad)} do not:\n"))
    for r in bad:
        L.append(f"- {r['file']} #{r['content_no']} ({r['status']}): " + "; ".join(r["mame_definition"]["problems"][:4]))
    rows = []
    for r in sorted((r for r in mame if r["status"] == "current" and r["target"] == "mame-mvs"), key=lambda r: r["game"]):
        tl = c["tree_lists"].get(r["game"], {})
        have = {f["name"].rsplit("/", 1)[-1]: f for f in r["files"] if f.get("part")}
        same = tl and all(n in have and (have[n]["size"], have[n]["crc32"], have[n]["sha1"]) == v for n, v in tl.items())
        rows.append([r["game"], r["version"], "describes this release" if same else "describes a different build (a newer tree build or none)"])
    L.append("\nThe software list in `hash_eagle/<game>/neogeo.xml` (written by the last tree build) against the current MVS release:\n")
    L.append(md_table(["Game", "Release", "hash_eagle list"], rows))
    L.append("")

    # 6. Duplicates
    L.append("## 6. Identical builds under different names\n")
    by_build = {}
    for r in mame:
        by_build.setdefault((r["target"], r["build_id"]), set()).add(f"{r['prefix']} {r['version']}")
    dups = [(t, b, sorted(n)) for (t, b), n in by_build.items() if len(n) > 1]
    L.append("None.\n" if not dups else md_table(["Target", "Build", "Release names"], [[t, f"`{b}`", ", ".join(n)] for t, b, n in sorted(dups)]))
    L.append("")

    # 7. External databases
    L.append("## 7. Emulator databases\n")
    L.append("Each database entry is compared part by part (size and CRC32, plus SHA-1 where the database has one) with every MVS "
             "build we have, current or historical.\n")
    rows = []
    for prefix, (game, title, names) in list(GAMES.items()) + [(k, (k, v[0], v[1])) for k, v in EXTRA_SETS.items()]:
        for db, setname in names.items():
            entry = c["external"].get(db, {}).get("sets", {}).get(setname)
            if not entry:
                rows.append([title, db, setname, "not in the database snapshot", ""])
                continue
            builds = [r for r in mame if r.get("game") == game and r["target"] == "mame-mvs"]
            if game in c["tree_lists"] and not builds:
                tl = c["tree_lists"][game]
                builds = [{"files": [{"part": rom_part(n), "size": s, "crc32": cr, "sha1": sh} for n, (s, cr, sh) in tl.items()],
                           "version": "tree build", "build_id": "hash_eagle", "status": "tree", "file": f"hash_eagle/{game}/neogeo.xml", "content_no": 1}]
            hits = match_external(entry, builds)
            full = [h for h in hits if h[0]]
            if full:
                b = full[0][2]
                res = f"matches {b['version']} build `{b['build_id']}` ({b['file']} #{b['content_no']}, {b['status']})"
            elif hits:
                best = max(hits, key=lambda h: len(h[1]))
                res = f"no full match; closest {best[2]['version']} `{best[2]['build_id']}` shares {', '.join(best[1])}"
            else:
                res = "**matches no build we have**"
            rows.append([title, db, setname, entry.get("title", ""), res])
    L.append(md_table(["Game", "Database", "Set", "Database title", "Result"], rows))
    L.append("")
    # 8. Next versions
    L.append("## 8. Next free version per game\n")
    L.append("A version is **out** once any of its files is a GitHub release asset, sits in the public repository "
             "(pushed to a remote branch), or matches an emulator database entry. A build that differs from every build "
             "that went out under its name cannot keep that name: it needs the next version (docs/releases/VERSIONING_POLICY.md).\n")
    rows, proposals = [], {}
    for prefix in GAMES:
        mv = [r for r in mame if r["prefix"] == prefix and r["target"] == "mame-mvs"]
        if not mv:
            continue
        labels = sorted({r["version"] for r in mv}, key=lambda x: [int(n) for n in re.findall(r"\d+", semver(x).split("-")[0])])
        for label in labels:
            rs = [r for r in mv if r["version"] == label]
            out_ids = {r["build_id"] for r in rs if r["published"] or r["pushed"] or r["external_matches"]}
            # A database entry under this version name that is none of our builds
            # is another binary out there with this name.
            for db, setname in GAMES[prefix][2].items():
                entry = c["external"].get(db, {}).get("sets", {}).get(setname)
                t = re.search(r"\bv(\d+(?:\.\d+)*)\b", entry["title"]) if entry else None
                if t and f"v{t.group(1)}" == label and not any(full for full, _, _ in match_external(entry, rs)):
                    out_ids.add(f"{db}:{setname}")
            cur = next((r for r in rs if r["status"] == "current"), None)
            if cur is None:
                continue
            maj, mi, pa = (semver(label).split("-")[0].split(".") + ["0", "0"])[:3]
            nxt = f"**v{maj}.{mi}.{int(pa) + 1}**"
            if not out_ids:
                verdict = "not out yet: may be released as is"
            elif cur["build_id"] in out_ids and len(out_ids) == 1:
                verdict = "unambiguous: the one build that went out is the current file"
            elif cur["build_id"] in out_ids:
                verdict = f"ambiguous: the name went out as {len(out_ids)} builds; re-release the current build as {nxt}"
                proposals[(prefix, label)] = nxt.strip("*")
            else:
                verdict = f"the current build never went out under this name: release it as {nxt}"
                proposals[(prefix, label)] = nxt.strip("*")
            rows.append([GAMES[prefix][1], label, len({r['build_id'] for r in rs}), len(out_ids), f"`{cur['build_id']}`", verdict])
    L.append(md_table(["Game", "Version", "Builds", "Builds that went out", "Current build", "Verdict"], rows))
    L.append("")
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text("\n".join(L) + "\n")
    return proposals


def emit_submissions(c, proposals):
    """For each database set whose game has a newer current build: the
    database's own entry with that build's sizes and hashes put in."""
    dest = OUT / "db-submissions"
    dest.mkdir(exist_ok=True)
    written = []
    for prefix, (game, title, names) in GAMES.items():
        cur = next((r for r in c["records"] if r.get("prefix") == prefix and r.get("target") == "mame-mvs" and r["status"] == "current"
                    and r["version"] == max((x["version"] for x in c["records"] if x.get("prefix") == prefix and x.get("target") == "mame-mvs"
                                             and x["status"] == "current"), key=lambda v: [int(n) for n in re.findall(r"\d+", v)])), None)
        if cur is None:
            continue
        have = {f["part"]: f for f in cur["files"] if f.get("part")}
        version = proposals.get((prefix, cur["version"]), cur["version"])
        out = [f"{title} {version}: build {cur['build_id']} ({cur['file']}, packed {cur['build_timestamp']})",
               f"Proposed entries for the emulator databases, made by tools/rom_registry.py from the bytes of {cur['file']}.",
               "Each is the database's current entry with the new sizes and hashes; layout and macros are the maintainers' own.", ""]
        changed = False
        for db, setname in names.items():
            entry = c["external"].get(db, {}).get("sets", {}).get(setname)
            if not entry or "block" not in entry:
                continue
            block = entry["block"]
            rows = []
            for rom in entry["roms"]:
                f = have.get(rom["part"])
                if f is None:
                    continue
                rows.append(f"  {rom['name']:18} {rom['size'] or 0:#09x} {rom['crc32']}  ->  {f['size']:#09x} {f['crc32']}")
                if db == "hbmame":
                    pat = re.compile(r'("%s"[^\n]*?)(0x[0-9a-fA-F]+)?(\s*,\s*CRC\()[0-9a-fA-F]{8}(\)\s*SHA1\()[0-9a-fA-F]{40}' % re.escape(rom["name"]))
                    def sub(m, f=f):
                        size = m.group(2)
                        size = f"0x{f['size']:x}" if size and m.group(1).count(",") >= 2 else (size or "")
                        return f"{m.group(1)}{size}{m.group(3)}{f['crc32']}{m.group(4)}{f['sha1']}"
                    block = pat.sub(sub, block, count=1)
                else:
                    pat = re.compile(r'(\{\s*"%s",\s*)0x[0-9a-fA-F]+(,\s*)0x[0-9a-fA-F]+' % re.escape(rom["name"]))
                    block = pat.sub(lambda m, f=f: f"{m.group(1)}0x{f['size']:06x}{m.group(2)}0x{f['crc32']}", block, count=1)
            if block == entry["block"]:
                out += [f"{db} {setname}: the entry already describes this build.", ""]
                continue
            changed = True
            title_line = entry.get("game_line", "")
            new_title = re.sub(r"\bv\d+(?:\.\d+)*\b", version, entry["title"])
            out += [f"=== {db}: {setname} (now \"{entry['title']}\") ===", "", "Changed ROMs:", *rows, "", block, ""]
            if title_line:
                out += [title_line.replace(entry["title"], new_title), ""]
            else:
                out += [f"Description: {new_title}", ""]
        if changed:
            f = dest / f"{game}_{version}.txt"
            f.write_text("\n".join(out) + "\n")
            written.append(f.name)
    return written


def verify():
    reg = json.loads((OUT / "rom-registry.json").read_text())
    want = {r["path"]: r for r in reg["builds"] if r["status"] == "current"}
    bad = 0
    for path, r in sorted(want.items()):
        f = ROOT / path
        if not f.is_file():
            print(f"MISSING  {path}")
            bad += 1
            continue
        if hashlib.sha256(f.read_bytes()).hexdigest() != r["archive"]["sha256"]:
            print(f"CHANGED  {path}  (registry {r['archive']['sha256'][:12]}..., build {r.get('build_id')})")
            bad += 1
    extra = sorted(f"dist/release/{f.name}" for f in DIST.iterdir() if f.is_file() and f"dist/release/{f.name}" not in want)
    for p in extra:
        print(f"NEW      {p}  (not in the registry)")
    print(f"{len(want) - bad} of {len(want)} registered files match; {len(extra)} unregistered")
    return 1 if bad or extra else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--verify", action="store_true", help="check dist/release against releases/rom-registry.json")
    ap.add_argument("--offline", action="store_true", help="use the cached GitHub asset list")
    ap.add_argument("--external", nargs="*", default=[], metavar="DB=FILE", help="re-read hbmame=<neogeohb.cpp> fbneo=<d_neogeo.cpp>")
    args = ap.parse_args()
    if args.verify:
        sys.exit(verify())
    refresh = dict(x.split("=", 1) for x in args.external)
    OUT.mkdir(exist_ok=True)
    c = collect(args.offline, refresh)
    (OUT / "rom-registry.json").write_text(json.dumps(registry_json(c), indent=1) + "\n")
    write_sums(c)
    proposals = write_report(c)
    subs = emit_submissions(c, proposals)
    n = len(c["records"])
    print(f"database submissions: {', '.join(subs) or 'none needed'}")
    print(f"{n} release file contents registered; report in {REPORT.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
