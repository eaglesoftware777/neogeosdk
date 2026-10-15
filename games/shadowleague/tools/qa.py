"""Run the built Shadow League cartridge headless in MAME and check a match plays.

    make GAME=shadowleague all            (or PLATFORM=aes)
    python3 games/shadowleague/tools/qa.py [--platform aes] [--seconds 100]
"""

import argparse
import json
import os
import subprocess
import shutil
import hashlib
from pathlib import Path

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
FIELDS = {"match": "&s_match", "state": "&((NGFMatch*)0)->state", "round": "&((NGFMatch*)0)->round",
          "seconds": "&((NGFMatch*)0)->seconds", "winner": "&((NGFMatch*)0)->winner",
          "frame": "&((NGFMatch*)0)->frame",
          "super_freeze": "&((NGFMatch*)0)->super_freeze",
          "life0": "&((NGFMatch*)0)->f[0].life", "life1": "&((NGFMatch*)0)->f[1].life",
          "x0": "&((NGFMatch*)0)->f[0].x", "x1": "&((NGFMatch*)0)->f[1].x",
          "st0": "&((NGFMatch*)0)->f[0].state", "st1": "&((NGFMatch*)0)->f[1].state",
          "wins0": "&((NGFMatch*)0)->f[0].wins", "wins1": "&((NGFMatch*)0)->f[1].wins",
          "mode": "&((NGFMatch*)0)->match_mode", "active0": "&((NGFMatch*)0)->team_active[0]",
          "active1": "&((NGFMatch*)0)->team_active[1]", "count0": "&((NGFMatch*)0)->team_count[0]",
          "count1": "&((NGFMatch*)0)->team_count[1]", "assist0": "&((NGFMatch*)0)->assist_uses[0]",
          "assist1": "&((NGFMatch*)0)->assist_uses[1]",
          "def0": "&((NGFMatch*)0)->f[0].def", "def1": "&((NGFMatch*)0)->f[1].def",
          "def_life": "&((NGFFighterDef*)0)->life",
          "cpu0": "&((NGFMatch*)0)->f[0].cpu", "cpu1": "&((NGFMatch*)0)->f[1].cpu"}
RUNTIME_FIELDS={"bank_seen":"&slf_rom_banks_seen", "bank_errors":"&slf_rom_errors",
                "bank_copies":"&slf_rom_copies", "presentation":"&slf_presentation_current",
                "card_buffer":"&s_story_card", "palette_state":"&ng_palfx_scr",
                "select_live":"&s_portraits_live", "def_kenji":"&slf_kenji",
                "def_jack":"&slf_jack", "def_amara":"&slf_amara"}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--platform", choices=("mvs", "aes"), default="mvs")
    ap.add_argument("--script",type=Path,default=GAME / "tools/review_capture.lua",help="Native capture/performance wrapper")
    ap.add_argument("--record",action="store_true",help="Record real cartridge frames as native AVI")
    ap.add_argument("--motion-frames",action="store_true",help="Save every native frame during the review motion interval")
    ap.add_argument("--team", action="store_true", help="Select team battle and verify a teammate enters")
    ap.add_argument("--fighter",choices=('kenji','jack','amara'),help="Select a showcase fighter using actual menu inputs")
    ap.add_argument("--attract-only",action="store_true",help="No input; verify all story cards and both physical data banks")
    ap.add_argument("--seconds", type=int, default=100)
    ap.add_argument("--mame", default="mame")
    ap.add_argument("--bios-name", help="MAME BIOS name; defaults to euro/asia")
    ap.add_argument("--output", type=Path, default=GAME / "build/qa")
    ap.add_argument("--elf",type=Path,default=ROOT/"out/game")
    ap.add_argument("--roms",type=Path,default=ROOT/"roms")
    ap.add_argument("--bios",type=Path,default=ROOT/"bios/test_roms")
    ap.add_argument("--hash",type=Path,default=ROOT/"hash_eagle/shadowleague")
    args = ap.parse_args()
    if args.team and args.fighter:ap.error('--fighter selects a duel; use the regular team scenario for reserves and assists')
    windows=args.mame.lower().endswith(".exe")
    def native(p):
        p = p.resolve()
        return subprocess.check_output(["wslpath","-w",str(p)],text=True).strip() if windows else str(p)
    out = args.output.resolve() / args.platform
    out.mkdir(parents=True, exist_ok=True)
    for old in out.glob("shot_*.png"):
        old.unlink()
    for name in ('telemetry.jsonl','bank-telemetry.jsonl','performance.jsonl'):
        old=out/name
        if old.exists():old.unlink()
    gdb = next(ROOT.parent.glob("x-tools-v*/m68k-unknown-elf/bin/m68k-unknown-elf-gdb"))
    gcc = next(ROOT.parent.glob("x-tools-v3/m68k-unknown-elf/bin/m68k-unknown-elf-gcc"))
    # Field offsets from a debug object built by the cartridge's compiler;
    # the match's address from the cartridge itself.
    probe = out / "layout_probe.c"
    probe.write_text('#include "sdk/2d_engine/ng_fight.h"\nNGFMatch qa_match; NGFFighterDef qa_def;\n')
    subprocess.run([str(gcc), "-g", "-c", "-DNG_FIGHT=1", "-I" + str(ROOT), "-I" + str(ROOT / "sdk"),
                    "-I" + str(ROOT / "sdk/2d_engine"), str(probe), "-o", str(out / "layout_probe.o")], check=True)
    layout = {}
    for binary, names in ((args.elf, ["match"]), (out / "layout_probe.o", [n for n in FIELDS if n != "match"])):
        cmd = [str(gdb), "-batch", str(binary)]
        for name in names:
            cmd += ["-ex", f'printf "{name}=%lu\\n", (unsigned long){FIELDS[name]}']
        layout.update(line.split("=", 1) for line in subprocess.check_output(cmd, text=True).splitlines() if "=" in line)
    cmd=[str(gdb),'-batch',str(args.elf)]
    for name,expression in RUNTIME_FIELDS.items():cmd+=['-ex',f'printf "{name}=%lu\\n", (unsigned long){expression}']
    runtime=subprocess.run(cmd,text=True,capture_output=True)
    if runtime.returncode==0:
        layout.update(line.split('=',1) for line in runtime.stdout.splitlines() if '=' in line)
    elif args.attract_only:raise RuntimeError('Candidate ELF lacks bank/story QA symbols: '+runtime.stderr)
    (out / "layout.lua").write_text("return {" + ",".join(f"{k}={int(v)}" for k, v in layout.items()) + "}\n")
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy", DISPLAY="",
               SLF_CAPTURE_DIR=native(out).replace("\\","/"), SLF_PLATFORM=args.platform, SLF_TEAM="1" if args.team else "0",
               SLF_SCRIPT_ROOT=native(GAME / "tools").replace("\\","/"), SLF_MOTION_FRAMES="1" if args.motion_frames else "0",
               SLF_ATTRACT_ONLY='1' if args.attract_only else '0')
    env['SLF_FIGHTER']=str({'kenji':0,'jack':3,'amara':2}[args.fighter]) if args.fighter else ''
    if windows:env["WSLENV"]=env.get("WSLENV","")+":SLF_CAPTURE_DIR:SLF_PLATFORM:SLF_TEAM:SLF_SCRIPT_ROOT:SLF_MOTION_FRAMES:SLF_ATTRACT_ONLY:SLF_FIGHTER"
    aes = args.platform == "aes"
    script = args.script.resolve()
    if windows:
        script = out / "qa.lua"
        shutil.copy2(args.script, script)
    command = [args.mame, "aes" if aes else "neogeo", "-noreadconfig", "-rompath",
               f"{native(args.bios)};{native(args.roms)}", "-hashpath", native(args.hash),
               "-cart1", "shadowleague", "-bios", args.bios_name or ("asia" if aes else "euro"), "-video", "none", "-sound", "none",
               "-nothrottle", "-nonvram_save", "-seconds_to_run", str(args.seconds), "-skip_gameinfo",
               "-cfg_directory", native(out / "cfg"), "-nvram_directory", native(out / "nvram"),
               "-snapshot_directory", native(out), "-autoboot_delay", "0",
               "-autoboot_script", native(script)]
    if args.record:command += ["-aviwrite", native(out / "cartridge.avi")]
    provenance={'platform':args.platform,'bios_name':args.bios_name or ('asia' if aes else 'euro'),'team':args.team,'fighter':args.fighter,'attract_only':args.attract_only,
                'seconds':args.seconds,'elf_sha256':hashlib.sha256(args.elf.read_bytes()).hexdigest(),
                'rom_sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest()
                              for p in sorted((args.roms/'shadowleague').glob('781-*'))}}
    (out/'run-manifest.json').write_text(json.dumps(provenance,indent=2)+'\n')
    with (out / "mame.log").open("w") as log:
        subprocess.run(command, env=env, cwd=out, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=900)
    samples = [json.loads(line) for line in (out / "telemetry.jsonl").read_text().splitlines()]
    banks=[json.loads(line) for line in (out/'bank-telemetry.jsonl').read_text().splitlines()] if (out/'bank-telemetry.jsonl').exists() else []
    performance=[json.loads(line) for line in (out/'performance.jsonl').read_text().splitlines()] if (out/'performance.jsonl').exists() else []
    assert performance, 'Missing live hardware performance observer'
    assert 'palette_pending_max' in performance[-1], 'Missing palette-starvation measurement'
    assert performance[-1].get('palette_pending_max',0)<=30, 'Atomic palette upload starved'
    if banks:
        assert all(s['errors']==0 for s in banks),'Runtime P-ROM copy error'
    if args.attract_only:
        assert banks and banks[-1]['seen']==3,'Both data banks were not exercised'
        assert set(range(7))<={s['card'] for s in banks},'Story sequence did not show all seven cards'
        perf=out/'performance.jsonl'
        if perf.exists():
            last=json.loads(perf.read_text().splitlines()[-1])
            assert last.get('presentation_checks',0)>=7,'Not all live C-ROM addresses/palette assignments were verified'
        assert 'LUA ERROR' not in (out/'mame.log').read_text(),'Capture script failed'
        print(f'PASS {args.platform} attract: seven cards, both aligned P banks, no copy errors; {banks[-1]}')
        return
    fighting = [s for s in samples if s["state"] == 1 and s["cpu0"] == 0 and 0 < s["life0"] <= s["maxlife0"] and 0 < s["life1"] <= s["maxlife1"]]
    assert fighting, "No match reached the fight state"
    assert any(s["life1"] < s["maxlife1"] for s in fighting), "Player 1 never hit the opponent"
    assert len({s["x0"] for s in fighting}) > 3, "Player 1 never moved"
    if args.fighter:
        assert any(s.get('def0_pointer')==int(layout['def_'+args.fighter]) for s in fighting), 'Requested showcase fighter was not selected'
    assert "LUA ERROR" not in (out / "mame.log").read_text(), "Capture script failed"
    if args.team:
        assert all(s["mode"] == 1 and s["count0"] == 3 and s["count1"] == 3 for s in fighting), "Team selection failed"
        assert any(s["active0"] or s["active1"] for s in fighting), "No reserve fighter entered"
        assert any(s["assist0"] < 3 for s in fighting), "Player 1 never called an assist"
    print(f"PASS {args.platform} {'team' if args.team else 'duel'}: {len(fighting)} fight samples, last {samples[-1]}")


if __name__ == "__main__":
    main()
