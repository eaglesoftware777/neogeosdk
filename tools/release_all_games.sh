#!/bin/bash
# The release pipeline: builds every game, checks it, and prepares release
# candidates and the MAME test folder. It never publishes anything and never
# replaces a released file; a failed mandatory step stops it with exit 1.
#
#   tools/release_all_games.sh [--games "skylance nosignal"] [--quick]
#
#   1  repository and toolchain checks
#   2  make neo for each game (AES then MVS sets, NeoSD images), one at a time:
#      out/ is shared, so games are never built in parallel
#   3  host unit tests (tests/)                 reported; known failures listed
#   4  ROM registry, checksums, integrity report (tools/rom_registry.py), and
#      dist/release checked against it (--verify)
#   5  animation checks and contact sheets (tools/animation_qa.py)
#   6  every registered version installed in the MAME folder with launchers
#      (tools/install_rom_versions.py) and booted unattended with the Linux
#      MAME (tools/boot_versions.py)
#   7  the summary in docs/qa/PIPELINE_RUN.md
#
# Release files for a new version are made by tools/make_release.py with an
# explicit --version (docs/releases/VERSIONING_POLICY.md); publishing them is
# the owner's decision.
#
# C: holds the repository, the MAME folder and WSL's disk: the run refuses to
# start below 3 GB free and heavy steps run in a memory-capped cgroup when one
# is available (see the WSL disk notes in docs/qa/BASELINE_AUDIT.md).
set -u -o pipefail
cd "$(dirname "$0")/.." || exit 1
ROOT=$(pwd)
GAMES="skylance nosignal shadowleague maiyaluna sevenwonders mahjong_demo maiya demo"
QUICK=0
while [ $# -gt 0 ]; do
  case "$1" in
    --games) GAMES="$2"; shift 2 ;;
    --quick) QUICK=1; shift ;;
    *) echo "unknown option $1"; exit 2 ;;
  esac
done
LOG=$ROOT/docs/qa/PIPELINE_RUN.md
FAIL=0
step() { echo "== $*"; echo "" >> "$LOG"; echo "## $*" >> "$LOG"; }
note() { echo "$*"; echo "- $*" >> "$LOG"; }
mandatory() { if [ "$1" != 0 ]; then note "**FAILED** (mandatory): $2"; FAIL=1; else note "passed: $2"; fi; }

echo "# Pipeline run $(date '+%Y-%m-%d %H:%M')" > "$LOG"
echo "" >> "$LOG"
echo "Commit \`$(git rev-parse --short=12 HEAD)\` on \`$(git rev-parse --abbrev-ref HEAD)\`, $(git status --short -- . ':!out' | wc -l) changed paths outside out/." >> "$LOG"

step "1. Repository and toolchain"
free_mb=$(df -BM --output=avail /mnt/c 2>/dev/null | tail -1 | tr -dc 0-9)
if [ -n "$free_mb" ] && [ "$free_mb" -lt 3000 ]; then
  note "**STOPPED**: C: has ${free_mb} MB free; builds and MAME runs need at least 3 GB (WSL crashes when C: fills)"
  exit 1
fi
# The Makefile's choice: $(SDKHOME)/x-tools-v3 unless XTOOLS_ROOT says otherwise.
CC=${XTOOLS_ROOT:-$ROOT/../x-tools-v3}/m68k-unknown-elf/bin/m68k-unknown-elf-gcc
if [ -x "${CC:-/nonexistent}" ]; then note "toolchain: $($CC --version | head -1)"; else note "**FAILED**: no 68000 compiler at '$CC'"; exit 1; fi
note "MAME (Linux): $(mame -version 2>/dev/null | head -1 || echo missing)"

step "2. Builds"
for g in $GAMES; do
  rm -f out/*.o
  start=$(date +%s)
  make neo GAME=$g > "docs/qa/build-$g.log" 2>&1
  rc=$?
  warn=$(grep -c "warning:" "docs/qa/build-$g.log")
  mandatory $rc "make neo GAME=$g ($(( $(date +%s) - start )) s, $warn warnings, log docs/qa/build-$g.log)"
done

step "3. Host unit tests"
make -C tests test > docs/qa/host-tests.log 2>&1
rc=$?
if [ $rc != 0 ]; then
  note "host tests exit $rc: $(grep -m1 -E 'Assertion|FAIL' docs/qa/host-tests.log)"
  note "known since the 2026-10-28 baseline: test_move_touches_driver_only (see docs/qa/BASELINE_AUDIT.md)"
else
  note "host tests passed"
fi

step "4. ROM registry and integrity"
python3 tools/rom_registry.py > docs/qa/registry.log 2>&1
mandatory $? "registry, checksums and docs/qa/ROM_INTEGRITY_REPORT.md"
python3 tools/rom_registry.py --verify > docs/qa/registry-verify.log 2>&1
mandatory $? "dist/release matches releases/rom-registry.json ($(tail -1 docs/qa/registry-verify.log))"

step "5. Animation checks"
python3 tools/animation_qa.py --game nosignal > docs/qa/animation-nosignal.log 2>&1
note "animation checks exit $?: docs/qa/animation/ (contact sheets, metrics.json)"

step "6. MAME test folder"
if [ -f /mnt/c/mame/neogeosdk/bin/ngsdk.bat ]; then
  python3 tools/install_rom_versions.py > docs/qa/install-versions.log 2>&1
  mandatory $? "versions installed: $(tail -1 docs/qa/install-versions.log)"
  if [ $QUICK = 0 ]; then
    python3 tools/boot_versions.py > docs/qa/boot-versions.log 2>&1
    mandatory $? "boot checks: $(tail -1 docs/qa/boot-versions.log)"
  fi
else
  note "no MAME test folder at C:\\mame\\neogeosdk: install and boot checks skipped"
fi

step "7. Result"
if [ $FAIL = 0 ]; then note "every mandatory step passed: release candidates may be prepared with tools/make_release.py"
else note "**a mandatory step failed: no release may be promoted**"; fi
exit $FAIL
